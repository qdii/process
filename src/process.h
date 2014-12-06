#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include "config.h"
#include "common.h"
#include "icon.h"
#include "cocoa.h"

namespace ps
{
namespace details
{
#   if HAVE_WINVER_H
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
#   endif

bool is_cmdline_valid( const std::string & cmdline )
{
    if ( cmdline.empty() )
        return false;

#   if HAVE_BOOST_FILESYSTEM_PATH_HPP
    if ( !boost::filesystem::is_regular_file( boost::filesystem::path( cmdline ) ) )
        return false;
#endif

    return true;
}

} // ns details


inline
std::string get_cmdline_from_pid( const pid_t pid )
{
    using namespace ps::details;
#if HAVE_LIBPROC_H
    if ( pid == INVALID_PID )
        return "";

    std::string cmdline;
    cmdline.resize( PROC_PIDPATHINFO_MAXSIZE);

    const int ret = proc_pidpath(pid, (char*)cmdline.c_str(), cmdline.size());
    if ( ret <= 0 ) 
        return "";
    
    cmdline.resize( ret );
    return cmdline; 
#elif HAVE_PSAPI_H
    handle process_handle( create_handle_from_pid( pid ) );

    if ( process_handle == NULL )
        return "";

    unique_ptr<char[]> buffer( new char[MAX_PATH] );

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
    process( const process & );

#if HAVE_STD__MOVE
    /**@brief Move-constructs a process */
    process( process && );
#endif

    /**@brief Assigns a copy of a process */
    process & operator=( const process & );

#if HAVE_STD__MOVE
    /**@brief Move-assigns a copy of a process */
    process & operator=( process && );
#endif

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

    /**@brief Kills the process
     * @param[in] softly When set to true, calling this method will only notify the process that it should terminate. Otherwise it will send a fatal signal
     * @return 0 on success, -1 if insufficient privileges, -2 if the process could not be found */
    int kill( bool softly ) const;

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
process<T> & process<T>::operator=( const process & other )
{
    m_pid     = other.m_pid;
    m_cmdline = other.m_cmdline;
    m_title   = other.m_title;
    m_name    = other.m_name;
    m_version = other.m_version;
    m_icon    = other.m_icon;

    return *this;
}

#if HAVE_STD__MOVE
template< typename T >
process<T> & process<T>::operator=( process && other )
{
    m_pid     = std::move( other.m_pid );
    m_cmdline = std::move( other.m_cmdline );
    m_title   = std::move( other.m_title );
    m_name    = std::move( other.m_name );
    m_version = std::move( other.m_version );
    m_icon    = std::move( other.m_icon );

    return *this;
}
#endif

template< typename T >
process<T>::process( const process & copy )
    : m_pid(     copy.m_pid     )
    , m_cmdline( copy.m_cmdline )
    , m_title(   copy.m_title   )
    , m_name(    copy.m_name    )
    , m_version( copy.m_version )
    , m_icon(    copy.m_icon    )
{
}


#if HAVE_STD__MOVE
template< typename T >
process<T>::process( process && copy )
    : m_pid(        std::move( copy.m_pid ) )
    , m_cmdline(    std::move( copy.m_cmdline ) )
    , m_title(      std::move( copy.m_title ) )
    , m_name(       std::move( copy.m_name ) )
    , m_version(    std::move( copy.m_version ) )
    , m_icon(       std::move( copy.m_icon ) )
{
}
#endif

template< typename T >
process<T>::process( const pid_t pid )
    : m_pid( pid )
    , m_cmdline( get_cmdline_from_pid( pid ) )
    , m_title( "" )
    , m_name( "" )
    , m_version( "" )
    , m_icon( "" )
{
    using namespace ps::details;
#if HAVE_WINVER_H
    if ( m_cmdline.empty() )
        return;

    unsigned size = GetFileVersionInfoSize( m_cmdline.c_str(), 0 );
    if ( !size )
        return;

    unique_ptr< unsigned char[] > data( new unsigned char[size] );
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

    std::string name      = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "InternalName"   );
    std::string title     = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductName"    );
    std::string version   = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductVersion" );

    m_name.swap( name );
    m_title.swap( title ); 
    m_version.swap( version );

#elif HAVE_APPKIT_NSRUNNINGAPPLICATION_H && HAVE_APPKIT_NSWORKSPACE_H && HAVE_FOUNDATION_FOUNDATION_H
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
    unique_ptr< char, void (*)(void*) > title_ptr  ( title, &std::free );
    unique_ptr< char, void (*)(void*) > name_ptr   ( name, &std::free );
    unique_ptr< char, void (*)(void*) > version_ptr( version, &std::free );
    unique_ptr< char, void (*)(void*) > icon_ptr   ( icon, &std::free );
    unique_ptr< char, void (*)(void*) > path_ptr   ( path, &std::free );

    m_name.assign( name );
    m_title.assign( title );
    m_version.assign( version);

    std::string bundle_path ( path ),
                icon_name   ( icon );

    if ( !bundle_path.empty() && !icon_name.empty() )
        m_icon.assign( 
            get_icon_path_from_icon_name( bundle_path, 
                                          icon_name ) ); 
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
    using namespace ps::details;
    assert( valid() );

    std::vector< unsigned char > icon_data = get_icon_from_pid( pid() );

    if ( icon_data.empty() && !m_icon.empty() )
        icon_data = ps::details::get_icon_from_file( m_icon );

    if ( icon_data.empty() && is_cmdline_valid( cmdline() ) )
        icon_data = ps::details::get_icon_from_file( cmdline() );

    return icon_data;
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
int process<T>::kill( const bool softly ) const
{
    using namespace ps::details;
    assert( valid() );
#if HAVE_KILL
    const int killed = ::kill( m_pid, softly ? SIGTERM : SIGKILL );

    if ( killed == EPERM )
        return -1;

    if ( killed == ESRCH )
        return -2;

    assert( killed == 0 );
#elif HAVE_WINBASE_H || HAVE_PROCESSTHREADSAPI_H 
    (void)softly;
    const auto handle = create_handle_from_pid( m_pid, PROCESS_TERMINATE );
    if ( handle == NULL )
        return -1;

    const BOOL killed = TerminateProcess( handle, 0 );
    if ( killed != 0 )
        return -3;
#endif
    return 0;
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

