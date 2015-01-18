#ifndef PS_COMMON_H
#define PS_COMMON_H

#include "config.h"

#if HAVE_BOOST
#if HAVE_BOOST_FILESYSTEM_PATH_HPP
#   include <boost/filesystem.hpp>
#endif
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>
#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#endif

#if HAVE_VECTOR
#   include <vector>
#endif

#if HAVE_MEMORY
#   include <memory>
#endif

#if HAVE_STRING
#   include <string>
#endif

#if HAVE_FSTREAM
#   include <fstream>
#endif

#if HAVE_UTILITY
#   include <utility>
#endif

#if HAVE_SSTREAM
#   include <sstream>
#endif

#if HAVE_ITERATOR
#   include <iterator>
#endif

#if HAVE_IOMANIP
#   include <iomanip>
#endif

#if HAVE_ASSERT_H
#   include <assert.h>
#else
#   define assert(X) (void(0))
#endif

#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif

#if HAVE_SIGNAL_H
#   include <signal.h>
#endif

#if HAVE_PWD_H
#   include <pwd.h>
#endif

#if HAVE_SYS_SYSCTL_H
#   include <sys/sysctl.h>
#endif

#if HAVE_LIBPROC_H
#   include <libproc.h>
#endif

#if HAVE_SYS_PROC_INFO_H
#   include <sys/proc_info.h>
#endif

#if HAVE_LIBWNCK
#   define WNCK_I_KNOW_THIS_IS_UNSTABLE
#   include <libwnck/libwnck.h>
#endif

#if HAVE_LIBPNG
#   include <png.h>
#endif

#if HAVE_LIBICNS
extern "C"
{
#   include <icns.h>
    int icns_image_to_png( icns_image_t *, icns_size_t *, icns_byte_t ** );
}
#endif

#if HAVE_GDK_PIXBUF_GDK_PIXBUF_H
#   include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#ifdef BOOST_NO_CXX11_CONSTEXPR
#   define PS_CONSTEXPR const
#else
#   define PS_CONSTEXPR constexpr
#endif

#if HAVE_WINDOWS_H
#   include <windows.h>
#endif

#if HAVE_GDIPLUS_H
#   include <gdiplus.h>
#endif

#if HAVE_SHELLAPI_H
#   include <shellapi.h>
#endif

#if HAVE_WINBASE_H
#   include <winbase.h>
#endif

#if HAVE_OLE2_H
#   include <ole2.h>
#endif

#if HAVE_WINVER_H
#   include <winver.h>
#endif

#if HAVE_PSAPI_H
#   include <psapi.h>
#endif

#if HAVE_PROCESSTHREADSAPI_H
#   include <processthreadsapi.h>
#endif

#if HAVE_CODECVT
#   include <codecvt>
#endif

#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#if HAVE_SYS_SYSCTL_H
#   include <sys/sysctl.h>
#endif

#if HAVE_APPMODEL_H
#   include <appmodel.h>
#endif

#if !defined(HAVE_PID_T) || (HAVE_PID_T != 1)
#   if HAVE_DWORD
typedef DWORD pid_t;
#   else
typedef unsigned pid_t;
#   endif
#endif

#ifdef _WIN32
#   define PS_NOEXCEPT
#else
#   define PS_NOEXCEPT noexcept
#endif

#if HAVE_STD__MOVE
#   define PS_MOVE(X) (::std::move(X))
#else
#   define PS_MOVE(X) (X)
#endif


namespace ps
{

#if HAVE_UNIQUE_PTR
#   ifdef BOOST_NO_CXX11_TEMPLATE_ALIASES
using std::unique_ptr;
#   else
template<typename T, typename U = std::default_delete<T>>
using unique_ptr = ::std::unique_ptr<T, U>;
#   endif
#elif HAVE_BOOST
template<typename T, typename U>
using unique_ptr = ::boost::shared_ptr<T>;
#endif

struct cannot_find_icon : std::exception
{
    cannot_find_icon( const char * file_name )
        : error_message( std::string( "cannot find icon for path: \"" ) + file_name +
                         "\"" )
    {
    }

    const char * what() const PS_NOEXCEPT override
    {
        return error_message.c_str();
    }
private:
    std::string error_message;
};

static PS_CONSTEXPR pid_t INVALID_PID = 0;

namespace details
{

#if HAVE_WINUSER_H
pid_t get_pid_from_top_window( const HWND window )
{
    pid_t pid = INVALID_PID;
    GetWindowThreadProcessId( window, &pid );
    return pid;
}

static
BOOL CALLBACK find_pid_from_window( HWND window_handle, LPARAM param )
{
    if ( !param )
        return FALSE;

    std::vector< pid_t > & process_container
        = *reinterpret_cast< std::vector< pid_t >* >( param );

    const pid_t pid = get_pid_from_top_window( GetTopWindow( window_handle ) );

    if ( pid != INVALID_PID )
        process_container.push_back( pid );

    return TRUE;
}
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
struct handle : public boost::noncopyable
{
    handle( HANDLE h )
        : m_h( h )
    {
    }

    handle( handle && h )
        : m_h( h.m_h )
    {
        h.m_h = NULL;
    }

    handle & operator=( handle & ); // non-defined
    handle & operator=( handle && other )
    {
        m_h = other.m_h;
        other.m_h = NULL;
        return *this;
    }

    handle & operator=( HANDLE h )
    {
        m_h = h;
        return *this;
    }

    ~handle()
    {
        if ( m_h ) CloseHandle( m_h );
    }
    operator HANDLE() const
    {
        return m_h;
    }

private:
    HANDLE m_h;
};

inline
handle create_handle_from_pid( const pid_t pid,
                               const DWORD other_privileges = 0 )
{
    HANDLE h =
        OpenProcess( PROCESS_QUERY_INFORMATION |
                     PROCESS_VM_READ | other_privileges,
                     FALSE, pid );

    if ( h == NULL )
        h = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION |
                         PROCESS_VM_READ | other_privileges,
                         FALSE, pid );

    return h;
}

// returns the computers available drives
// "C:\", "D:\", etc.
inline
std::vector< std::string > get_all_msdos_drives()
{
    std::unique_ptr<char[]> buffer( new char[256] );
    const auto length = GetLogicalDriveStrings( 256, buffer.get() );

    if ( length == 0 )
        return std::vector< std::string >();

    std::vector< std::string > drives;
    for( const char * ptr = buffer.get(); *ptr != '\0';
            ptr += drives.back().size() + 1 )
    {
        drives.push_back( std::string( ptr ) );
    }

    return drives;
}

// returns the kernel drive associated with the msdos drive, or "" if none
// C: -> \Device\HarddiskVolume2
inline
std::string get_kernel_drive( const std::string & msdos_drive )
{
    assert( msdos_drive.size() == 2 ); // "C:"

    std::unique_ptr<char[]> buffer( new char[MAX_PATH] );
    const auto length = QueryDosDeviceA( msdos_drive.c_str(), buffer.get(),
                                         MAX_PATH );
    return length ? std::string( buffer.get() ) : "";
}

#ifdef _WIN32
inline
std::string to_utf8( const std::wstring & text )
{
    std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > convert;
    return convert.to_bytes( text );
}
#endif

// replace the kernel drive part of the path with a msdos one
// like: \Device\HarddiskVolume2\boot.ini -> C:\boot.ini
inline
std::string convert_kernel_drive_to_msdos_drive( std::string user_path )
{
    const std::vector< std::string > all_msdos_drives(
        get_all_msdos_drives() );

    for ( std::string drive : all_msdos_drives )
    {
        boost::trim_right_if( drive, []( char c )
        {
            return c == '\\';
        } );
        const std::string kernel_drive = get_kernel_drive( drive );

        // some network drives are not necessarily mapped
        if ( kernel_drive.empty() )
            continue;

        if ( user_path.find( kernel_drive ) != 0 )
            continue;

        user_path.replace( 0, kernel_drive.size(), drive );
        return user_path;
    }

    return user_path;
}
#endif

static inline
bool string_ends_in( const std::string & str, const std::string & suffix )
{
    return str.find( suffix ) + suffix.size() == str.size();
}

inline
std::string get_argv_from_pid( const int pid )
{
#if HAVE_UNISTD_H && HAVE_SYS_SYSCTL_H && DEFINED_KERN_ARGMAX && DEFINED_KERN_PROCARGS2
    int    mib[3], argmax, nargs, c = 0;
    char  *  procargs, *sp, *np, *cp;
    int show_args = 1;

    mib[0] = CTL_KERN;
    mib[1] = KERN_ARGMAX;

    size_t size = sizeof( argmax );
    if ( sysctl( mib, 2, &argmax, &size, NULL, 0 ) == -1 )
    {
        goto ERROR_A;
    }

    /* Allocate space for the arguments. */
    procargs = ( char * )malloc( argmax );
    if ( procargs == NULL )
    {
        goto ERROR_A;
    }


    /*
     * Make a sysctl() call to get the raw argument space of the process.
     * The layout is documented in start.s, which is part of the Csu
     * project.  In summary, it looks like:
     *
     * /---------------\ 0x00000000
     * :               :
     * :               :
     * |---------------|
     * | argc          |
     * |---------------|
     * | arg[0]        |
     * |---------------|
     * :               :
     * :               :
     * |---------------|
     * | arg[argc - 1] |
     * |---------------|
     * | 0             |
     * |---------------|
     * | env[0]        |
     * |---------------|
     * :               :
     * :               :
     * |---------------|
     * | env[n]        |
     * |---------------|
     * | 0             |
     * |---------------| <-- Beginning of data returned by sysctl() is here.
     * | argc          |
     * |---------------|
     * | exec_path     |
     * |:::::::::::::::|
     * |               |
     * | String area.  |
     * |               |
     * |---------------| <-- Top of stack.
     * :               :
     * :               :
     * \---------------/ 0xffffffff
     */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pid;


    size = ( size_t )argmax;
    if ( sysctl( mib, 3, procargs, &size, NULL, 0 ) == -1 )
    {
        goto ERROR_B;
    }

    memcpy( &nargs, procargs, sizeof( nargs ) );
    cp = procargs + sizeof( nargs );

    /* Skip the saved exec_path. */
    for ( ; cp < &procargs[size]; cp++ )
    {
        if ( *cp == '\0' )
        {
            /* End of exec_path reached. */
            break;
        }
    }
    if ( cp == &procargs[size] )
    {
        goto ERROR_B;
    }

    /* Skip trailing '\0' characters. */
    for ( ; cp < &procargs[size]; cp++ )
    {
        if ( *cp != '\0' )
        {
            /* Beginning of first argument reached. */
            break;
        }
    }
    if ( cp == &procargs[size] )
    {
        goto ERROR_B;
    }
    /* Save where the argv[0] string starts. */
    sp = cp;

    /*
     * Iterate through the '\0'-terminated strings and convert '\0' to ' '
     * until a string is found that has a '=' character in it (or there are
     * no more strings in procargs).  There is no way to deterministically
     * know where the command arguments end and the environment strings
     * start, which is why the '=' character is searched for as a heuristic.
     */
    for ( np = NULL; c < nargs && cp < &procargs[size]; cp++ )
    {
        if ( *cp == '\0' )
        {
            c++;
            if ( np != NULL )
            {
                /* Convert previous '\0'. */
                *np = ' ';
            }
            else
            {
                /* *argv0len = cp - sp; */
            }
            /* Note location of current '\0'. */
            np = cp;

            if ( !show_args )
            {
                /*
                 * Don't convert '\0' characters to ' '.
                 * However, we needed to know that the
                 * command name was terminated, which we
                 * now know.
                 */
                break;
            }
        }
    }

    /*
     * sp points to the beginning of the arguments/environment string, and
     * np should point to the '\0' terminator for the string.
     */
    if ( np == NULL || np == sp )
    {
        /* Empty or unterminated string. */
        goto ERROR_B;
    }

    /* Make a copy of the string. */
    return std::string( sp );

    /* Clean up. */
    free( procargs );
    return "";

ERROR_B:
    free( procargs );
ERROR_A:

#else
    ( void )pid;
#endif
    return "";
}

#if HAVE_WINDOWS_H
struct library
{
    library( const std::string & dllname );
    ~library();

    bool is_loaded() const;
    void * get_function( const std::string & name ) const;

private:
    const HMODULE m_handle;
};

library::library( const std::string & dllname )
    : m_handle( LoadLibrary( dllname.c_str() ) )
{
}

library::~library()
{
    if ( is_loaded() )
        FreeLibrary( m_handle );
}

bool library::is_loaded() const
{
    return m_handle != NULL;
}

void * library::get_function( const std::string & name ) const
{
    assert( is_loaded() );
    assert( !name.empty() );

    return GetProcAddress( m_handle, name.c_str() );
}
#endif

struct cfile
{
    cfile( const std::string & path )
        : m_file( fopen( path.c_str(), "r" ) )
    {
    }

    cfile( const cfile & );
    cfile & operator=( const cfile & );

    bool is_open() const
    {
        return m_file != NULL;
    }

    ~cfile() noexcept
    {
        if ( m_file )
            fclose( m_file );
    }

    operator FILE * ()
    {
        return m_file;
    }

private:
    FILE * m_file;
};

inline
bool is_png_header( unsigned char header[8] )
{
    if ( header[0] != 137    ) return false;
    if ( header[1] != 'P'    ) return false;
    if ( header[2] != 'N'    ) return false;
    if ( header[3] != 'G'    ) return false;
    if ( header[4] != '\r'   ) return false;
    if ( header[5] != '\n'   ) return false;
    if ( header[6] != '\032' ) return false;
    if ( header[7] != '\n'   ) return false;

    return true;
}

template< typename T >
bool is_png( const T& data )
{
    if ( data.size() < 8 )
        return false;

    return is_png_header(
        reinterpret_cast<unsigned char*>(
            const_cast<char*>( data.data() ) ) );
}

inline
bool is_icns_header( unsigned char header[4] )
{
    return header[0] == 'i'
        && header[1] == 'c'
        && header[2] == 'n'
        && header[3] == 's';
}

template< typename T >
bool is_icns( const T& data )
{
    if ( data.size() < 8 )
        return false;

    return is_icns_header(
        reinterpret_cast<unsigned char*>(
            const_cast<char*>( data.data() ) ) );
}



} // namespace details
} // namespace ps
#endif // PS_COMMON_H
