#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// details::VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include "common.h"
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
#   include "cocoa.h"
#endif

#   include <Gdiplusimagecodec.h>

namespace ps
{
    namespace details
    {
        static PS_CONSTEXPR unsigned NAME    = 0;
        static PS_CONSTEXPR unsigned TITLE   = 1;
        static PS_CONSTEXPR unsigned VERSION = 2;
        static PS_CONSTEXPR unsigned ICON    = 3;

#       if defined(WIN32)
        inline 
        std::string to_utf8( std::wstring text )
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
                new Bitmap( iconBmp.bmWidth, iconBmp.bmHeight, PixelFormat32bppARGB ) );
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

        struct cannot_find_icon : std::exception
        {
            cannot_find_icon( const char * file_name )
                : error_message( std::string("cannot find icon for path: \"") + file_name + "\"" )
            {
            }

            const char * what() const override
            {
                return error_message.c_str();
            }
        private:
            std::string error_message;
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

        std::vector<unsigned char> get_icon_from_file( const std::string & path )
        {
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
            hglobal_lock memory_lock( input_str.get_hglobal() );
            unsigned char * const ptr = static_cast<unsigned char*>( memory_lock.lock() );
            if ( ptr == nullptr )
                return std::vector<unsigned char>();

			std::vector<unsigned char> data( ptr, ptr + GlobalSize(input_str.get_hglobal()) );

            return data;
		}

#       endif
    }


inline
std::string get_cmdline_from_pid( const pid_t pid )
{
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
std::string get_icon_path_from_icon_name( std::string bundle_path, 
                                          std::string icon_name )
{
    assert( !bundle_path.empty() );
    assert( !icon_name.empty() );
    return bundle_path + "/Contents/Resources/" + icon_name + 
        (( icon_name.find( ".icns" ) + 5 != icon_name.size() ) ? ".icns" : "");
}
#endif


/* @brief Describes a process */
template< typename T >
struct process
{
    explicit
    process( pid_t pid );

    process( pid_t pid, 
             const std::string & cmdline,
             const std::string & title   = "",
             const std::string & name    = "",
             const std::string & version = "" );

    /**@brief Creates an invalid process */
    process();
    ~process();

    /**@brief This is the name that makes most sense to a human
     *        It could be the title bar, or the product name as
     *        advertized by its creator */
    std::string title() const;

    /**@brief Returns the command line used to run the binary executable */
    std::string cmdline() const;

    /**@brief Returns the name of the application as seen by the OS
     * For instance, on linux it could be "gnu-tar" */
    std::string name() const;
    
    /**@brief Returns the version of the application, if it was provided */
    std::string version() const;

    /**@brief Returns a the main icon of the process as a PNG file */
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
{
}

template< typename T >
process<T>::process( const pid_t pid )
    : m_pid( pid )
    , m_cmdline( get_cmdline_from_pid( pid ) )
    , m_title( "" )
    , m_name( "" )
    , m_version( "" )
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
    return ps::details::get_icon_from_file( cmdline() );
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
         << "\", icon: \"" << proc.icon() 
         << "\"\n";
}

} // namespace ps

#endif // PS_PROCESS_H

