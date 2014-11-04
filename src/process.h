#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// details::VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include "common.h"
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
#   include "cocoa.h"
#endif

namespace ps
{
    namespace details
    {
        static PS_CONSTEXPR unsigned NAME = 0;
        static PS_CONSTEXPR unsigned TITLE = 1;
        static PS_CONSTEXPR unsigned VERSION = 2;
    }
#if defined(WIN32)
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

    return std::string( info, info+size );
}

static inline
std::tuple< std::string, std::string, std::string >
get_file_info( const std::string & path )
{
    if ( path.empty() )
        return std::tuple< std::string, std::string, std::string >();

    unsigned size = GetFileVersionInfoSize( path.c_str(), 0 );
    if ( !size )
        return std::tuple< std::string, std::string, std::string >();

    std::unique_ptr< unsigned char[] > data( new unsigned char[size] );
    if (!GetFileVersionInfo( path.c_str(), 0, size, data.get() ))
        return std::tuple< std::string, std::string, std::string >();

    struct LANGANDCODEPAGE 
    {
      WORD language;
      WORD codepage;
    } * translate;

    VerQueryValue(data.get(), 
                  "\\VarFileInfo\\Translation",
                  reinterpret_cast<LPVOID*>(&translate),
                  &size);

    std::tuple< std::string, std::string, std::string > ret;
    std::get<details::NAME>( ret ) = get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductName" );
    std::get<details::TITLE>( ret ) = ""; 
    std::get<details::VERSION>( ret ) = get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductVersion" );

    return ret;
}
#endif

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

static inline
std::tuple< std::string, std::string, std::string >
get_file_info( const pid_t pid )
{
#if defined(WIN32)
    return get_file_info( get_cmdline_from_pid( pid ) );
#elif defined(__APPLE__) && defined(TARGET_OS_MAC)
    char * title,
         * name,
         * version;
    const int success = 
        get_info_from_pid( pid, &title, &name, &version );

    if ( success != 0 )
        return std::tuple< std::string, std::string, std::string >();

    assert( title   != nullptr );
    assert( name    != nullptr );
    assert( version != nullptr );

    // RAII structures to make sure the memory is free when going out of scope
    std::unique_ptr< char, void (*)(void*) > title_ptr  ( title, &std::free );
    std::unique_ptr< char, void (*)(void*) > name_ptr   ( name, &std::free );
    std::unique_ptr< char, void (*)(void*) > version_ptr( version, &std::free );

    std::tuple< std::string, std::string, std::string > file_info;
    std::get<details::NAME>( file_info ).assign( name );
    std::get<details::TITLE>( file_info ).assign( title );
    std::get<details::VERSION>( file_info ).assign( version ); 

    return file_info;
#else
    return std::tuple< std::string, std::string, std::string >();
#endif
}

static inline
std::string get_title_from_pid( const pid_t pid )
{
    return std::get<details::TITLE>( get_file_info( pid ) );
}

static inline
std::string get_name_from_pid( const pid_t pid )
{
    return std::get<details::NAME>( get_file_info( pid ) );
}


static inline
std::string get_version_from_pid( const pid_t pid )
{
    return std::get<details::VERSION>( get_file_info( pid ) );
}

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
    , m_title( get_title_from_pid( pid ) )
    , m_name( get_name_from_pid( pid ) )
    , m_version( get_version_from_pid( pid ) )
{
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

