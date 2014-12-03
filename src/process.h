#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include "common.h"
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
#   include "cocoa.h"
#endif

namespace ps
{
namespace details
{
#   if defined(WIN32)
    inline 
    std::string to_utf8( const std::wstring & text )
    {
        std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > convert;
        return convert.to_bytes( text );
    }

    static inline
    std::string get_specific_file_info(
        unsigned char * const data,
        const WORD language, const WORD codepage,
        const std::string & info_type )
    {
        unsigned size;
        char* info;
        std::ostringstream subblock;
        subblock << "\\StringFileInfo\\"
                    << std::hex << std::setw(4) << std::setfill('0') << language
                    << std::hex << std::setw(4) << std::setfill('0') << codepage
                    << "\\" << info_type;

        if (!VerQueryValue(data,
                subblock.str().c_str(),
                reinterpret_cast<LPVOID*>(&info),
                &size 
            ))
            return "";
        assert( size != 0 );

        return std::string( info, size-1 );
    }

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
    std::unique_ptr< Gdiplus::Bitmap > get_bitmap_from_hicon( HICON hIcon, const Gdiplus::PixelFormat pixel_format )
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

#   endif

    std::vector<unsigned char> get_icon_from_file( const std::string & path )
    {
#   ifdef _WIN32
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
#   elif defined __APPLE__ && defined TARGET_OS_MAC
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
#   endif

        return std::vector< unsigned char >();
	}

    std::vector< unsigned char > get_icon_from_pid( const pid_t pid )
    {
        std::vector< unsigned char > contents;

#   if defined __linux && defined PS_GNOME
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
#   endif
        return contents;
    }

} // ns details


inline
std::string get_cmdline_from_pid( const pid_t pid )
{
    using namespace ps::details;
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
    if ( pid == INVALID_PID )
        return "";

    std::string cmdline;
    cmdline.resize( PROC_PIDPATHINFO_MAXSIZE);

    const int ret = proc_pidpath(pid, (char*)cmdline.c_str(), cmdline.size());
    if ( ret <= 0 ) 
        return "";
    
    cmdline.resize( ret );
    return cmdline; 
#elif defined( _WIN32 ) || defined( _WIN64 )
    handle process_handle( create_handle_from_pid( pid ) );

    if ( process_handle == NULL )
        return "";

    std::unique_ptr<char[]> buffer( new char[MAX_PATH] );

    const DWORD length 
        = GetProcessImageFileNameA(
            process_handle,
            buffer.get(),
            MAX_PATH);
    
    if ( length == 0 )
        return "";

    return convert_kernel_drive_to_msdos_drive( 
        std::string( buffer.get(), buffer.get() + length ) );
#else
    (void)pid;
    return "";
#endif
}

#if defined(__APPLE__) && defined(TARGET_OS_MAC)
static inline
bool string_ends_in( const std::string & str, const std::string & suffix )
{
    return str.find( suffix ) + suffix.size() == str.size();
}

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


/* @brief Describes a process */
template< typename T >

/**@struct process
 * @brief describes a process */
struct process
{
    /**@brief Constructs a process from the given pid
     *
     * This constructor will automatically try and retrieve the data for 
     * title, name, etc. from the pid
     * @param[in] pid The process id given by the OS */
    explicit
    process( pid_t pid );

    /**@brief Constructs a process, and manually assign all the information to it
     * @param[in] cmdline The absolute path to the binary executable 
     * @param[in] title The title of the process. The most human-friendly one. Like "Skype".
     * @param[in] name The name of the process, as perceived by the OS. Could be something like Microsoft.Skype
     * @param[in] version The version being run */
    process( pid_t pid, 
             const std::string & cmdline,
             const std::string & title   = "",
             const std::string & name    = "",
             const std::string & version = "" );

    /**@brief Creates an invalid process */
    process();

    /**@brief Destructs this object. Does not kill or terminate the running process though */
    ~process();

    /**@brief Constructs a copy of a process */
    process( const process & ) = default;

    /**@brief Move-constructs a process */
    process( process && ) = default;

    /**@brief Assigns a copy of a process */
    process & operator=( const process & ) = default;

    /**@brief Move-assigns a copy of a process */
    process & operator=( process && ) = default;

    /**@brief This is the name that makes most sense to a human
     *        It could be the title bar, or the product name as
     *        advertized by its creator */
    std::string title() const;

    /**@brief Returns the command line used to run the binary executable */
    std::string cmdline() const;

    /**@brief Returns the name of the application as seen by the OS
     *
     * For instance, on linux it could be "gnu-tar" */
    std::string name() const;
    
    /**@brief Returns the version of the application, if it was provided */
    std::string version() const;

    /**@brief Returns the main icon of the process
     *
     * On Windows, this function returns a PNG file.
     * On Mac, this function returns a ICNS file.
     * @warning This function is SLOW. 
             It reads from the disk and perform all sorts of slow things.
     * @throw cannot_find_icon When file information from the executable cannot be accessed. */
    std::vector< unsigned char > icon() const;

    /**@brief Checks whether this object is valid and describes
     *        a process (even a non-running, or non-existing one) */
    bool valid() const;

    pid_t pid() const { return m_pid; }

private:
    pid_t       m_pid;
    std::string m_cmdline;
    std::string m_title;
    std::string m_name;
    std::string m_version;

    ///< Used on mac to store the path to the icon
    std::string m_icon;
};

template< typename T >
process<T>::process( pid_t pid, 
                     const std::string & cmdline,
                     const std::string & title,
                     const std::string & name,
                     const std::string & version )
    : m_pid( pid )
    , m_cmdline( cmdline )
    , m_title( title )
    , m_name( name )
    , m_version( version )
    , m_icon( "" )
{
}

template< typename T >
process<T>::process( const pid_t pid )
    : m_pid( pid )
    , m_cmdline( get_cmdline_from_pid( pid ) )
    , m_title( "" )
    , m_name( "" )
    , m_version( "" )
    , m_icon( "" )
{
#ifdef _WIN32
    if ( m_cmdline.empty() )
        return;

    unsigned size = GetFileVersionInfoSize( m_cmdline.c_str(), 0 );
    if ( !size )
        return;

    std::unique_ptr< unsigned char[] > data( new unsigned char[size] );
    if (!GetFileVersionInfo( m_cmdline.c_str(), 0, size, data.get() ))
        return;

    struct LANGANDCODEPAGE 
    {
      WORD language;
      WORD codepage;
    } * translate;

    if ( !VerQueryValue(data.get(), 
                        "\\VarFileInfo\\Translation",
                        reinterpret_cast<LPVOID*>(&translate),
                        &size) )
        return;

    m_name      = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "InternalName"   );
    m_title     = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductName"    ); 
    m_version   = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductVersion" );

#elif defined(__APPLE__) && defined(TARGET_OS_MAC)

    char * title,
         * name,
         * version,
         * icon,
         * path;
    const int success = 
        get_info_from_pid( pid, &title, &name, &version, &icon, &path );

    if ( success != 0 )
        return;

    assert( title   != nullptr );
    assert( name    != nullptr );
    assert( version != nullptr );
    assert( path    != nullptr );
    assert( icon    != nullptr );

    // RAII structures to make sure the memory is free when going out of scope
    std::unique_ptr< char, void (*)(void*) > title_ptr  ( title,   &std::free );
    std::unique_ptr< char, void (*)(void*) > name_ptr   ( name,    &std::free );
    std::unique_ptr< char, void (*)(void*) > version_ptr( version, &std::free );
    std::unique_ptr< char, void (*)(void*) > icon_ptr   ( icon,    &std::free );
    std::unique_ptr< char, void (*)(void*) > path_ptr   ( path,    &std::free );

    m_name.assign( name );
    m_title.assign( title );
    m_version.assign( version ); 

    std::string bundle_path( path );
    std::string icon_name( icon );

    if ( !bundle_path.empty() && !icon_name.empty() )
        m_icon.assign( 
            get_icon_path_from_icon_name( std::move( bundle_path ), 
                                          std::move( icon_name ) ) ); 
#endif
}

template< typename T >
process<T>::process()
    : m_pid( INVALID_PID )
    , m_cmdline( "" )
    , m_title( "" )
    , m_name( "" )
    , m_version( "" )
{
}

template< typename T >
process<T>::~process()
{
    //assert( !valid() || m_cmdline.empty() ||
    //    boost::filesystem::exists( m_cmdline ) );
}

template< typename T >
std::string process<T>::cmdline() const
{
    assert( valid() );
    return m_cmdline;
}

template< typename T >
std::string process<T>::title() const
{
    assert( valid() );
    return m_title;
}

template< typename T >
std::string process<T>::name() const
{
    assert( valid() );
    return m_name;
}

template< typename T >
std::vector< unsigned char > process<T>::icon() const
{
    assert( valid() );
#if defined _WIN32 || defined __APPLE__
#   ifdef _WIN32
    const std::string & icon_file = cmdline();
#   else
    const std::string & icon_file = m_icon;
#   endif
    return ps::details::get_icon_from_file( icon_file );
#else
    return ps::details::get_icon_from_pid( pid() );
#endif
}

template< typename T >
std::string process<T>::version() const
{
    assert( valid() );
    return m_version;
}

template< typename T >
bool process<T>::valid() const
{
    return m_pid != INVALID_PID;
}

template< typename T >
void describe( std::ostream & ostr, const process<T> & proc )
{
    ostr << "pid: " << proc.pid() 
         << ", title: \"" << proc.title() 
         << "\", name: \"" << proc.name() 
         << "\", cmdline: \"" << proc.cmdline() 
         << "\", version: \"" << proc.version()
         << "\"\n";
}

} // namespace ps

#endif // PS_PROCESS_H

