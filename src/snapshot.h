#ifndef PS_SNAPSHOT_H
#define PS_SNAPSHOT_H

#include "common.h"
#if defined( __APPLE__ ) && defined( TARGET_OS_MAC ) && defined(PS_COCOA)
#   include "cocoa.h"
#endif
#include "process.h"
#include "cocoa.h"

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
    enum flags
    {
        ENUMERATE_DESKTOP_APPS = 0x1,
        ENUMERATE_BSD_APPS     = 0x2,
        ENUMERATE_ALL          = 0x3,
    };

public:
    explicit snapshot( flags = ENUMERATE_ALL );

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

template< typename CONTAINER, typename T >
CONTAINER get_entries_from_window_manager()
{
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
    CONTAINER processes;
    const int nbApplications =
        getDesktopApplications( nullptr, nullptr, nullptr, 0 );

    std::vector< pid_t > pidArray( nbApplications );
    std::vector< char* > bundleIdentifierArray( nbApplications );
    std::vector< char* > bundleNameArray( nbApplications );
    int success = getDesktopApplications( 
            const_cast<pid_t*>( pidArray.data() ),
            const_cast<char **>( bundleIdentifierArray.data() ),
            const_cast<char **>( bundleNameArray.data() ),
            nbApplications
            );
    if ( success < 0 )
        goto exit;

    for ( int i=0; i<success; ++i )
    {
        if ( pidArray[i] == 0 )
            continue;

        processes.emplace_back( 
                pidArray[i], 
                get_cmdline_from_pid( pidArray[i] ),
                bundleIdentifierArray[i],
                bundleNameArray[i]
        );
    }

exit:
    std::for_each( bundleIdentifierArray.begin(), bundleIdentifierArray.end(), &free );
    std::for_each( bundleNameArray.begin(), bundleNameArray.end(), &free );
    
    return processes;

#else
    return CONTAINER();
#endif
}

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

    if ( !success )
        return CONTAINER();

    return CONTAINER(
        bsd_processes.begin(),
        std::remove( bsd_processes.begin(), bsd_processes.end(), 0 )
    );

#else
    return CONTAINER();
#endif
}

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
CONTAINER capture_processes( typename snapshot<T>::flags flags )
{
    CONTAINER all_processes;

    if ( target_os() == LINUX )
        all_processes = get_entries_from_procfs< CONTAINER, T >();

    else if ( target_os() == MAC_OSX )
    {
        if ( flags & snapshot<T>::ENUMERATE_BSD_APPS )
        { 
            const CONTAINER bsd_processes = get_entries_from_syscall< CONTAINER, T >();
            all_processes.insert( all_processes.end(), bsd_processes.cbegin(),    bsd_processes.cend() );
        }

        if ( flags & snapshot<T>::ENUMERATE_DESKTOP_APPS )
        {
            const CONTAINER gui_applications = get_entries_from_window_manager< CONTAINER, T >();
            all_processes.insert( all_processes.end(), gui_applications.cbegin(), gui_applications.cend() );
        }
    }

    return all_processes;
}

template< typename T >
snapshot<T>::snapshot( typename snapshot<T>::flags flags )
    : m_processes( capture_processes< container, T >( flags ) )
{
}

} //namespace 

#endif

