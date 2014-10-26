#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include <string>
#ifdef __APPLE__
#   include <TargetConditionals.h>
#   ifdef TARGET_OS_MAC
#       include <sys/proc_info.h>
#       include <sys/sysctl.h>
#       include <libproc.h>
#   endif
#endif

namespace ps
{
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
std::string get_cmdline_from_pid( const pid_t pid )
{
    std::string cmdline;
    cmdline.resize( PROC_PIDPATHINFO_MAXSIZE);

    const int ret = proc_pidpath(pid, (char*)cmdline.c_str(), cmdline.size());
    if ( ret <= 0 ) 
        return "";
    
    cmdline.resize( ret );
    return cmdline; 
}
#endif

/* @brief Describes a process */
template< typename T >
struct process
{
    explicit
    process( pid_t pid, const std::string & cmdline );

#if defined(__APPLE__) && defined(TARGET_OS_MAC)
    explicit
    process( const kinfo_proc & );
#endif

    process();

    /**@brief This is the name that makes most sense to a human
     *        It could be the title bar, or the product name as
     *        advertized by its creator */
    std::string title() const;

    /**@brief Checks whether this object is valid and describes
     *        a process (even a non-running, or non-existing one) */
    bool valid() const;


private:
    pid_t       m_pid;
    std::string m_cmdline;
};

template< typename T >
process<T>::process( pid_t pid, const std::string & cmdline )
    : m_pid( pid )
    , m_cmdline( cmdline )
{
}

#if defined(__APPLE__) && defined(TARGET_OS_MAC)
template< typename T >
process<T>::process( const kinfo_proc & kproc )
    : m_pid( kproc.kp_proc.p_pid )
    , m_cmdline( get_cmdline_from_pid( m_pid ) )
{
} 
#endif

template< typename T >
process<T>::process()
    : process( 0, "" )
{
}

template< typename T >
std::string process<T>::title() const
{
    assert( valid() );
    return m_cmdline;
}

template< typename T >
bool process<T>::valid() const
{
    return m_pid != 0;
}

} // namespace ps

#endif // PS_PROCESS_H

