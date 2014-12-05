#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include "common.h"
#include "icon.h"
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
#   include "cocoa.h"
#endif

namespace ps
{
namespace details
{
#   if defined(WIN32)
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

    /**@brief Move-constructs a process */
    process( process && );

    /**@brief Assigns a copy of a process */
    process & operator=( const process & );

    /**@brief Move-assigns a copy of a process */
    process & operator=( process && );

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

template< typename T >
process<T>::process( const pid_t pid )
    : m_pid( INVALID_PID )
    , m_cmdline( "" )
    , m_title( "" )
    , m_name( "" )
    , m_version( "" )
    , m_icon( "" )
{
    using namespace ps::details;
#ifdef _WIN32
    std::string cmdline = get_cmdline_from_pid( pid );
    if ( cmdline.empty() )
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

    std::string name      = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "InternalName"   );
    std::string title     = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductName"    );
    std::string version   = details::get_specific_file_info( data.get(), translate[0].language, translate[0].codepage, "ProductVersion" );

    m_pid = pid;
    m_cmdline.swap( cmdline );
    m_name.swap( name );
    m_title.swap( title ); 
    m_version.swap( version );

#elif defined(__APPLE__) && defined(TARGET_OS_MAC)

    std::unique_ptr< char, void (*)(void*) > title_ptr   ( nullptr, &std::free );
    std::unique_ptr< char, void (*)(void*) > name_ptr    ( nullptr, &std::free );
    std::unique_ptr< char, void (*)(void*) > version_ptr ( nullptr, &std::free );
    std::unique_ptr< char, void (*)(void*) > icon_ptr    ( nullptr, &std::free );
    std::unique_ptr< char, void (*)(void*) > path_ptr    ( nullptr, &std::free );

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
    title_ptr  .reset( title );
    name_ptr   .reset( name );
    version_ptr.reset( version );
    icon_ptr   .reset( icon );
    path_ptr   .reset( path );

    std::string name_str    ( name ),
                title_str   ( title ),
                version_str ( version ),
                bundle_path ( path ),
                icon_name   ( icon );

    if ( !bundle_path.empty() && !icon_name.empty() )
        m_icon.assign( 
            get_icon_path_from_icon_name( std::move( bundle_path ), 
                                          std::move( icon_name ) ) ); 

    m_pid = pid;
    m_name.assign( std::move( name_str ) );
    m_title.assign( std::move( title_str ) );
    m_version.assign( std::move( version_str ) );
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
int process<T>::kill( const bool softly ) const
{
    using namespace ps::details;
    assert( valid() );
#if defined __linux || defined __APPLE__
    const int killed = ::kill( m_pid, softly ? SIGTERM : SIGKILL );

    if ( killed == EPERM )
        return -1;

    if ( killed == ESRCH )
        return -2;

    assert( killed == 0 );
#elif defined _WIN32
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

