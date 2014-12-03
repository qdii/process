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
    using namespace ps::details;
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

