#ifndef PS_COMMON_H
#define PS_COMMON_H

#include <string>
#include <assert.h>
#include <boost/filesystem.hpp>
#include <vector>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/current_function.hpp>
#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <utility>
#include <sstream>
#include <iterator>
#include <assert.h>
#include <iomanip>

#ifdef __APPLE__
#   include <TargetConditionals.h>
#   ifdef TARGET_OS_MAC
#       include <sys/proc_info.h>
#       include <sys/sysctl.h>
#       include <libproc.h>
#   endif
#endif

#ifdef __unix
#	include <sys/sysctl.h>
#	include <pwd.h>
#endif

#ifdef PS_GNOME
#   define WNCK_I_KNOW_THIS_IS_UNSTABLE
#   include <libwnck/libwnck.h>
#endif

#ifdef BOOST_NO_CXX11_CONSTEXPR
#	define PS_CONSTEXPR const
#else
#	define PS_CONSTEXPR constexpr
#endif

#if defined( WIN32 ) || defined( WIN64 )
#	include <windows.h>
#   include <gdiplus.h>
#   include <psapi.h>
#   include <codecvt>
typedef DWORD pid_t;
#endif

namespace ps
{
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

static PS_CONSTEXPR pid_t INVALID_PID = 0;

namespace details
{
    
enum OS
{
    OS_WIN32,
    OS_WIN64,
    OS_IOS_SIMULATOR,
    OS_IOS_DEVICE,
    OS_MAC_OSX,
    OS_LINUX,
    OS_UNIX,
    OS_POSIX,
    OS_UNKNOWN
};
static PS_CONSTEXPR
OS target_os()
{
#ifdef _WIN32
#   ifdef _WIN64
        return OS_WIN64;
#   else
        return OS_WIN32;
#   endif
#elif __APPLE__
#   include "TargetConditionals.h"
#   if TARGET_IPHONE_SIMULATOR
        return OS_IOS_SIMULATOR; 
#   elif TARGET_OS_IPHONE
        return OS_IOS_DEVICE;
#   elif TARGET_OS_MAC
        return OS_MAC_OSX;     
#   else
        return OS_UNKNOWN; 
#   endif
#elif __linux
    return OS_LINUX;
#elif __unix // all unices not caught above
    return OS_UNIX; 
#elif __posix
    return OS_POSIX; 
#else
    return UNKNOWN;
#endif
}

#if defined( _WIN32 ) || defined( _WIN64 )
struct handle : public boost::noncopyable
{
	handle(HANDLE h)
		: m_h( h )
	{
	}

    handle( handle&& h )
        : m_h( h.m_h )
    {
        h.m_h = NULL;
    }

    handle& operator=( handle & ); // non-defined
    handle& operator=( handle && other )
    {
        m_h = other.m_h;
        other.m_h = NULL;
        return *this;
    }

    handle& operator=(HANDLE h)
    {
        m_h = h;
        return *this;
    }

	~handle() { if (m_h) CloseHandle( m_h ); }
	operator HANDLE() const { return m_h; }

private:
	HANDLE m_h;
};

inline
handle create_handle_from_pid( const pid_t pid )
{
    HANDLE h =
        OpenProcess( PROCESS_QUERY_INFORMATION |
                       PROCESS_VM_READ,
                       FALSE, pid );

    if ( h == NULL )
        h = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION |
                         PROCESS_VM_READ,
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
    for( const char *ptr = buffer.get(); *ptr != '\0'; 
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
    const auto length = QueryDosDeviceA( msdos_drive.c_str(), buffer.get(), MAX_PATH );
    return length ? std::string( buffer.get() ) : "";
}

// replace the kernel drive part of the path with a msdos one
// like: \Device\HarddiskVolume2\boot.ini -> C:\boot.ini
inline
std::string convert_kernel_drive_to_msdos_drive( std::string user_path )
{
    const std::vector< std::string > all_msdos_drives(
        get_all_msdos_drives() );

    for ( std::string drive : all_msdos_drives )
    {
        boost::trim_right_if( drive, []( char c ){ return c == '\\'; } );
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

static PS_CONSTEXPR
bool target_os_windows()
{
    return target_os() == OS_WIN32 || target_os() == OS_WIN64;
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

} // namespace details
} // namespace ps
#endif // PS_COMMON_H
