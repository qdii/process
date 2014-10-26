#ifndef PS_SNAPSHOT_H
#define PS_SNAPSHOT_H

#include <vector>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <utility>
#include <sys/sysctl.h>
#include <pwd.h>
#ifdef __APPLE__
#   include "TargetConditionals.h"
#endif

#include "process.h"
#include "common.h"

namespace ps
{

/**@struct snapshot
 * @brief A snapshot of all running processes at a given moment
 */
template< typename T >
struct snapshot
{
private:
    typedef std::vector< process< T > > container;

public:
    snapshot();

    typename container::iterator       begin()
    {
        return m_processes.begin();
    };
    typename container::iterator       end()
    {
        return m_processes.end();
    };
    typename container::const_iterator cbegin()
    {
        return m_processes.cbegin();
    };
    typename container::const_iterator cend()
    {
        return m_processes.cend();
    };

private:
    container m_processes;
};



#if defined(__APPLE__) && defined(TARGET_OS_MAC)
template< typename CONTAINER, typename T >
CONTAINER get_entries_from_syscall()
{
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
    std::vector< pid_t > bsd_processes;
    const int length = proc_listpids( PROC_ALL_PIDS, 0, nullptr, 0 );
    bsd_processes.resize( length );
    const int success 
        = proc_listpids( PROC_ALL_PIDS, 0,
                         const_cast<pid_t*>(bsd_processes.data()),
                         length );

    return CONTAINER(
        bsd_processes.cbegin(),
        bsd_processes.cend()
    );

#else
    return std::vector< process<T> >();
#endif
}

#endif

template < typename T > static
bool read_entry_from_procfs( boost::filesystem::directory_iterator pos,
        process<T> & out )
{
    using namespace boost::filesystem;

    // the path of a directory entry of /proc, something like /proc/12113
    const path entry_path = absolute( pos->path() );

    if ( !is_directory( entry_path ) )
        return false;

    // we store the pid, which is the *filename* of the entry (/proc/12113)
    pid_t pid = 0;
    try
    {
        pid = boost::lexical_cast<pid_t>( entry_path.filename().string().c_str() );
    }
    catch( const boost::bad_lexical_cast & )
    {
        // /proc also contains directory entries which are not processes, such as "dri"
        return false;
    }

    // we make it an absolute entry, because we are not necessarily executing from /proc
    // /proc/12113/cmdline contains the name of the process
    const path path_to_cmdline = entry_path / "cmdline";

    // if cmdline does not exist, it might be that we just don't have the rights to read /proc/12113
    if ( !exists( path_to_cmdline ) )
        return false;

    // if we do not have the rights to read cmdline, then creating the ifstream may fail
    std::ifstream cmdline( path_to_cmdline.c_str() );
    if ( !cmdline.is_open() )
        return false;

    // /proc/11241/cmdline contains the full name of the executable
    std::string process_name;
    std::getline( cmdline, process_name );

    // kernel threads, for instance, have empty cmdline
    if ( process_name.empty() )
        return false;

    out = process<T>( pid, process_name );
    return true;
}

template < typename CONTAINER, typename T > static
CONTAINER get_entries_from_procfs()
{
    using namespace boost::filesystem;
    CONTAINER all_processes;

    directory_iterator pos( "/proc" );
    // until this is the last iterator, try and retrieve the process info
    for ( ; pos != directory_iterator(); ++pos )
    {
        process<T> next_process;
        if ( read_entry_from_procfs( pos, next_process ) )
            all_processes.push_back( std::move( next_process ) );
    }

    return all_processes;
}

template< typename CONTAINER, typename T >
CONTAINER capture_processes()
{
    CONTAINER all_processes;

    if ( TARGET_OS() == LINUX )
        all_processes = get_entries_from_procfs< CONTAINER, T >();
    else if ( TARGET_OS() == MAC_OSX )
        all_processes = get_entries_from_syscall< CONTAINER, T >();

    return all_processes;
}

template< typename T >
snapshot<T>::snapshot()
    : m_processes( capture_processes< container, T >() )
{
}

} //namespace 

#endif

