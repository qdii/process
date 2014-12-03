#ifndef PS_ICON_H
#define PS_ICON_H

#include "common.h"

namespace ps
{
namespace details
{

#ifdef _WIN32
struct input_stream
{
    input_stream()
        : m_istream( nullptr )
    {
        auto created =
            CreateStreamOnHGlobal( NULL, TRUE, &m_istream );

        if ( created == E_OUTOFMEMORY )
            throw std::bad_alloc();

        assert( created != E_INVALIDARG );
    }

    operator IStream*() { return m_istream; }
    operator const IStream*() const { return m_istream; }

    HGLOBAL get_hglobal()
    {
        HGLOBAL hglobal = 0;
        auto success = GetHGlobalFromStream( m_istream, &hglobal );
        assert( success == S_OK );
        return hglobal;
    }

    ~input_stream()
    {
        m_istream->Release();
    }

private:
    IStream * m_istream;
};

struct cannot_lock_hglobal : std::exception
{
    const char * what() const override
    {
        return "Cannot lock HGLOBAL";
    }
};

struct hglobal_lock
{
    hglobal_lock( HGLOBAL hglobal )
        : m_hglobal( hglobal )
        , m_is_locked( false )
    {
    }

    void * lock()
    {
        assert( !m_is_locked );
        void * const mem = GlobalLock( m_hglobal );
        m_is_locked = (mem != nullptr);
        return mem;
    }

    void unlock()
    {
        if ( m_is_locked )
        {
            GlobalUnlock( m_hglobal );
            assert( GetLastError() == NO_ERROR );
            m_is_locked = false;
        }
    }

    ~hglobal_lock()
    {
        unlock();
    }

private:
    HGLOBAL     m_hglobal;
    bool        m_is_locked;
};

struct hicon
{
    explicit
    hicon( const std::string & path )
    {
    	if( !SHGetFileInfo( path.c_str(), 0, &m_file_info, sizeof(m_file_info),
                            SHGFI_ICON|SHGFI_LARGEICON ) )
        {
            throw cannot_find_icon( path.c_str() );
        }
    }

    operator HICON() const
    {
        return m_file_info.hIcon;
    }

    ~hicon()
    {
        DestroyIcon( m_file_info.hIcon );
    }

private:
    SHFILEINFO m_file_info;
};
struct gdiplus_context
{
    gdiplus_context()
    {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup( &m_token, &input, NULL );
    }

    ~gdiplus_context()
    {
        Gdiplus::GdiplusShutdown( m_token );
    }
private:
    ULONG_PTR m_token;
};

static inline
CLSID get_encoder_from_mime_type( const std::string & mime )
{
    assert ( mime.find("/") != std::string::npos ); // mime types must contain a "/"
    using namespace Gdiplus;

    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes
    if ( GetImageEncodersSize( &num, &size ) != Status::Ok || size == 0 )
        return CLSID();

    std::unique_ptr< char[] > temporary_buffer( new char[size] );
    ImageCodecInfo * const encoders
        = reinterpret_cast< ImageCodecInfo * >( temporary_buffer.get() );

    GetImageEncoders( num, size, encoders );

    for( unsigned j = 0; j < num; ++j )
    {
        if( to_utf8( std::wstring( encoders[j].MimeType ) ) == mime )
            return encoders[j].Clsid;
    }

    return CLSID();
}
static inline
std::unique_ptr< Gdiplus::Bitmap > 
get_bitmap_from_hicon( HICON hIcon, const Gdiplus::PixelFormat pixel_format )
{
    using namespace Gdiplus;

    ICONINFO iconInfo;
    if (!GetIconInfo(hIcon, &iconInfo))
        return std::unique_ptr< Bitmap >();

    BITMAP iconBmp;
    if (GetObject(iconInfo.hbmColor, sizeof(BITMAP),&iconBmp) != sizeof(BITMAP))
        return std::unique_ptr< Bitmap >();

    std::unique_ptr< Bitmap > result( 
            new Bitmap( iconBmp.bmWidth, iconBmp.bmHeight, pixel_format ) );
    assert( result.get() ); // GdiPlus might not have been properly initialized

    bool hasAlpha = false;
    // We have to read the raw pixels of the bitmap to get proper transparency information
    // (not sure why, all we're doing is copying one bitmap into another)

    Bitmap colorBitmap( iconInfo.hbmColor, NULL );
    BitmapData bmpData;
    Rect bmBounds( 0, 0, colorBitmap.GetWidth(), colorBitmap.GetHeight() );

    colorBitmap.LockBits( &bmBounds, ImageLockModeRead, colorBitmap.GetPixelFormat(), &bmpData );
    for (DWORD y = 0; y < bmpData.Height; y++)
    {
        byte *pixelBytes = (byte*)bmpData.Scan0 + y*bmpData.Stride;
        DWORD *pixel   = (DWORD*) pixelBytes;
        for (DWORD x = 0; x < bmpData.Width; ++x, ++pixel)
        {
            result->SetPixel(x, y, Color(*pixel));
            DWORD alpha = (*pixel) >> 24;
            hasAlpha = hasAlpha || (alpha > 0 && alpha < 255);
        }
    }

    colorBitmap.UnlockBits(&bmpData);

    if (hasAlpha)
        return result;

    // If there's no alpha transparency information, we need to use the mask
    // to turn back on visible pixels

    Bitmap maskBitmap(iconInfo.hbmMask,NULL);
    Color  color, mask;

    for (DWORD y = 0; y < maskBitmap.GetHeight(); y++)
    {
        for (DWORD x = 0; x < maskBitmap.GetWidth(); ++x)
        {
            maskBitmap.GetPixel(x, y, &mask);
            result->GetPixel(x, y, &color);
            result->SetPixel(x, y, (color.GetValue()&0xFFFFFF) | (mask.GetValue()==0xffffffff ? 0 : 0xFF000000));				
        }
    }

    return result;
}
#endif // _WIN32

std::vector<unsigned char> get_icon_from_file( const std::string & path )
{
#ifdef _WIN32
    using namespace Gdiplus;
    gdiplus_context _;

    hicon icon( path );
    auto bitmap = get_bitmap_from_hicon( icon, PixelFormat32bppARGB );

    // get the encoder
    auto encoder = get_encoder_from_mime_type( "image/png" );
    if ( encoder == CLSID() )
        return std::vector<unsigned char>();

    // copy the bitmap into the input stream
    input_stream input_str;
    if ( bitmap->Save( input_str, &encoder, 0 ) != Status::Ok )
        return std::vector<unsigned char>();

    // copy the bits from global memory to our return vector
    try
    {
        hglobal_lock memory_lock( input_str.get_hglobal() );
        unsigned char * const ptr = static_cast<unsigned char*>( memory_lock.lock() );
        if ( ptr == nullptr )
            return std::vector<unsigned char>();

        return std::vector<unsigned char>( ptr, ptr + GlobalSize(input_str.get_hglobal()) );
    }
    catch( cannot_lock_hglobal & )
    {
        // if we cannot lock memory, we are screwed
        // ... but we still exit gracefully
    }
#elif defined __APPLE__ && defined TARGET_OS_MAC
    if ( path.empty() )
        return std::vector< unsigned char >();

    std::ifstream icon_file( path, std::ios_base::binary );
    if ( !icon_file )
        return std::vector< unsigned char >();

    icon_file.seekg(0,std::ios::end);
    const auto length = icon_file.tellg();
    icon_file.seekg(0,std::ios::beg);

    std::vector< unsigned char > contents( length );
    icon_file.read( reinterpret_cast<char*>(&contents[0]), length );

    if ( icon_file.gcount() == length )
        return contents;
#endif

    return std::vector< unsigned char >();
}

std::vector< unsigned char > get_icon_from_pid( const pid_t pid )
{
    std::vector< unsigned char > contents;

#if defined __linux && defined PS_GNOME
    if (!gdk_init_check(NULL, NULL))
        return std::vector<unsigned char>();

    WnckScreen * const screen
        = wnck_screen_get_default();

    if ( !screen )
        return std::vector<unsigned char>();

    wnck_screen_force_update(screen);

    for (GList * window_l = wnck_screen_get_windows (screen); window_l != NULL; window_l = window_l->next)
    {
        WnckWindow * window = WNCK_WINDOW(window_l->data);
        if (!window)
            continue;

        const pid_t window_pid = wnck_window_get_pid( window );
        if ( window_pid != pid )
            continue;

        GdkPixbuf * const icon = wnck_window_get_icon( window );
        if ( !icon )
            continue;

        gchar * buffer    = nullptr;
        gsize buffer_size = 0;
        GError * error    = nullptr;

        const auto saved =
            gdk_pixbuf_save_to_buffer( icon, &buffer, &buffer_size, "png", &error, NULL );

        if ( saved )
        {
            try
            {
                contents.assign( buffer, buffer + buffer_size );
            } catch( ... )
            {
            }
            g_free( buffer );
        }

        break;
    }
#endif
    return contents;
}

#if defined(__APPLE__) && defined(TARGET_OS_MAC)
static inline
std::string get_icon_path_from_icon_name( std::string bundle_path, 
                                          std::string icon_name )
{
    assert( !bundle_path.empty() );
    assert( !icon_name.empty() );
    std::string resources_folder = bundle_path + "/Contents/Resources/";

    // sometimes the file is already in PNG format
    if ( string_ends_in( icon_name, ".png" ) 
            || string_ends_in( icon_name, ".icns" )
            || string_ends_in( icon_name, ".PNG" ) )
       return resources_folder + icon_name;

    return bundle_path + "/Contents/Resources/" + icon_name + ".icns";
}
#endif


} // ns details
} // ns ps

#endif // PS_ICON_H