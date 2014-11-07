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
#endif
}

static inline
std::string get_title_from_pid( const pid_t pid )
{
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
    char * buffer;
    const int success = get_info_from_pid( pid, &buffer, nullptr );
    if ( success != 0 )
        return "";

    std::string title( buffer );
    free( buffer );
    return title;
#else
    return "";
#endif
}

static inline
std::string get_name_from_pid( const pid_t pid )
{
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
    char * buffer;
    const int success = get_info_from_pid( pid, nullptr, &buffer);
    if ( success != 0 )
        return "";

    std::string name( buffer );
    free( buffer );
    return name;
#elif defined(_WIN32) || defined( _WIN64 )
    handle process_handle( create_handle_from_pid( pid ) );
        
    HMODULE hMod;
    DWORD cbNeeded;
    std::unique_ptr<char[]> buffer( new char[MAX_PATH] );

    if ( !EnumProcessModules( process_handle, 
                              &hMod, sizeof(hMod), 
                              &cbNeeded) )
        return "";

    
    GetModuleBaseNameA( process_handle, hMod, buffer.get(), 
                        MAX_PATH );

    return std::string( buffer.get() );
#endif
}

/* @brief Describes a process */
template< typename T >
struct process
{
    process( pid_t pid, const std::string & cmdline );

    process( pid_t pid, 
             const std::string & cmdline,
             const std::string & title    );

    process( pid_t pid, 
             const std::string & cmdline,
             const std::string & title,
             const std::string & name    );

    explicit
    process( pid_t pid );

    process();

    /**@brief This is the name that makes most sense to a human
     *        It could be the title bar, or the product name as
     *        advertized by its creator */
    std::string title() const;

    /**@brief Returns the command line used to run the binary executable */
    std::string cmdline() const;

    /**@brief Returns the name of the application as seen by the OS
     * For instance, on linux it could be "gnu-tar" */
    std::string name() const;

    /**@brief Checks whether this object is valid and describes
     *        a process (even a non-running, or non-existing one) */
    bool valid() const;

    pid_t pid() const { return m_pid; }

private:
    pid_t       m_pid;
    std::string m_cmdline;
    std::string m_title;
    std::string m_name;
};

template< typename T >
process<T>::process( pid_t pid, const std::string & cmdline )
    : m_pid( pid )
    , m_cmdline( cmdline )
    , m_title()
    , m_name()
{
}

template< typename T >
process<T>::process( pid_t pid, 
                     const std::string & cmdline,
                     const std::string & title,
                     const std::string & name )
    : m_pid( pid )
    , m_cmdline( cmdline )
    , m_title( title )
    , m_name( name )
{
}

template< typename T >
process<T>::process( const pid_t pid )
    : m_pid( pid )
    , m_cmdline( get_cmdline_from_pid( pid ) )
    , m_title( get_title_from_pid( pid ) )
    , m_name( get_name_from_pid( pid ) )
{
}

template< typename T >
process<T>::process()
    : m_pid( INVALID_PID )
    , m_cmdline( "" )
    , m_title( "" )
    , m_name( "" )
{
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
bool process<T>::valid() const
{
    return m_pid != INVALID_PID;
}

template< typename T >
void describe( std::ostream & ostr, const process<T> & proc )
{
    ostr << "pid: " << proc.pid() << ", title: \"" << proc.title() << "\", name: \"" << proc.name() << "\", cmdline: \"" << proc.cmdline() << "\"\n";
}

} // namespace ps

#endif // PS_PROCESS_H

