#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include <string>
#include <assert.h>
#include <boost/filesystem.hpp>

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
    if ( pid == 0 )
        return "";

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
    process( pid_t pid, const std::string & cmdline );

    process( pid_t pid, 
             const std::string & cmdline,
             const std::string & title    );

    process( pid_t pid, 
             const std::string & cmdline,
             const std::string & title,
             const std::string & name    );

#if defined(__APPLE__) && defined(TARGET_OS_MAC)
    explicit
    process( pid_t pid );
#endif

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

#if defined(__APPLE__) && defined(TARGET_OS_MAC)
template< typename T >
process<T>::process( const pid_t pid )
    : m_pid( pid )
    , m_cmdline( get_cmdline_from_pid( pid ) )
    , m_title()
    , m_name()
{
}
#endif

template< typename T >
process<T>::process()
    : process( 0, "" )
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
    return m_pid != 0;
}

template< typename T >
void describe( std::ostream & ostr, const process<T> & proc )
{
    ostr << "title: " << proc.title() << "name: " << proc.name() << "cmdline: " << proc.cmdline() << '\n';
}

} // namespace ps

#endif // PS_PROCESS_H

