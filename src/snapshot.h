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
struct snapshot : public std::vector< process< T > > 
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

    explicit snapshot( flags = ENUMERATE_ALL );
};

#if defined(WIN32)

template<typename T> static
BOOL CALLBACK find_pid_from_window( HWND window_handle, LPARAM param )
{
    if ( !param )
        return FALSE;

    std::vector< pid_t > & process_container
        = *reinterpret_cast< std::vector< pid_t >* >( param );

    pid_t pid;
    const auto threadId 
        = GetWindowThreadProcessId( GetTopWindow( window_handle ), &pid );

    if ( threadId != 0 )
        process_container.push_back( pid );

    return TRUE;
}
#endif


template< typename CONTAINER, typename T >
CONTAINER get_entries_from_window_manager()
{
    CONTAINER processes;
#if defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(PS_COCOA)
    const int nbApplications =
        get_desktop_applications( nullptr, nullptr, nullptr, 0 );

    std::vector< pid_t > pidArray( nbApplications );
    std::vector< char* > bundleIdentifierArray( nbApplications );
    std::vector< char* > bundleNameArray( nbApplications );
    int success = get_desktop_applications( 
            const_cast<pid_t*>( pidArray.data() ),
            const_cast<char **>( bundleIdentifierArray.data() ),
            const_cast<char **>( bundleNameArray.data() ),
            nbApplications
            );
    if ( success < 0 )
        goto exit;

    for ( int i=0; i<success; ++i )
    {
        if ( pidArray[i] == INVALID_PID )
            continue;

        processes.emplace_back( pidArray[i] );
    }

exit:
    std::for_each( bundleIdentifierArray.begin(), bundleIdentifierArray.end(), &free );
    std::for_each( bundleNameArray.begin(), bundleNameArray.end(), &free );

#elif defined __linux && defined( PS_GNOME )
    if (!gdk_init_check(NULL, NULL))
        return CONTAINER();

    WnckScreen * const screen
        = wnck_screen_get_default();

    if ( !screen )
        return CONTAINER();

    wnck_screen_force_update(screen);

    for (GList * window_l = wnck_screen_get_windows (screen); window_l != NULL; window_l = window_l->next)
    {
        WnckWindow * window = WNCK_WINDOW(window_l->data);
        if (!window)
            continue;

        const pid_t pid = wnck_window_get_pid( window );
        assert( pid != ps::INVALID_PID );
        processes.emplace_back(
            pid,
            get_cmdline_from_pid( pid ),
            wnck_window_get_name( window )
            );
    }

#elif defined(WIN32)
    std::vector< pid_t > pids;
    if ( !EnumWindows( &find_pid_from_window<T>, reinterpret_cast<LPARAM>(&pids) ) )
        return CONTAINER();

    for ( const pid_t pid : pids )
        processes.emplace_back( pid );
    
#endif
    return processes;
}

static inline
std::vector<pid_t> get_running_process_ids( const unsigned max_pids = 500 )
{
    std::vector<pid_t> pids;
    pids.resize( max_pids, INVALID_PID );

#if defined(__APPLE__) && defined(TARGET_OS_MAC)
    proc_listpids( PROC_ALL_PIDS, 0,
                   const_cast<pid_t*>(pids.data()),
                   max_pids );
#elif defined( _WIN32 ) || defined( _WIN64 )
    DWORD nbBytes;
    EnumProcesses( const_cast<pid_t*>(pids.data()), 
                   max_pids * sizeof( pid_t ), &nbBytes );
    pids.resize( nbBytes / sizeof( pid_t ) );
#endif

    return pids;
}

template< typename CONTAINER, typename T >
CONTAINER get_entries_from_syscall()
{
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
    const unsigned max_pids = proc_listpids( PROC_ALL_PIDS, 0, nullptr, 0 );
#else
    const unsigned max_pids = 500;
#endif

    std::vector<pid_t> running_pids 
        = get_running_process_ids( max_pids );

    return CONTAINER(
        running_pids.begin(),
        std::remove( running_pids.begin(), running_pids.end(), INVALID_PID )
    );
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
    using namespace ps::details;
    CONTAINER all_processes;

    if ( target_os() == OS_MAC_OSX || target_os() == OS_LINUX )
    {
        if ( flags & snapshot<T>::ENUMERATE_BSD_APPS )
        { 
            const CONTAINER bsd_processes = get_entries_from_syscall< CONTAINER, T >();
            all_processes.insert( all_processes.end(), bsd_processes.cbegin(), bsd_processes.cend() );

            if ( target_os() == OS_LINUX )
            {
                const CONTAINER procfs_entries = get_entries_from_procfs< CONTAINER, T >();
                all_processes.insert( all_processes.end(), procfs_entries.cbegin(), procfs_entries.cend() );
            }
        }

        if ( flags & snapshot<T>::ENUMERATE_DESKTOP_APPS )
        {
            const CONTAINER gui_applications = get_entries_from_window_manager< CONTAINER, T >();
            all_processes.insert( all_processes.end(), gui_applications.cbegin(), gui_applications.cend() );
        }
    }

	else if ( target_os_windows() )
    {
        if ( flags & snapshot<T>::ENUMERATE_DESKTOP_APPS )
        {
            const CONTAINER gui_applications = get_entries_from_window_manager< CONTAINER, T >();
            all_processes.insert( all_processes.end(), gui_applications.cbegin(), gui_applications.cend() );
        }

        if ( flags & snapshot<T>::ENUMERATE_BSD_APPS )
        {
		    const CONTAINER regular_processes = get_entries_from_syscall< CONTAINER, T >();
            all_processes.insert( all_processes.end(), regular_processes.cbegin(), regular_processes.cend() );
        }
    }
	
    return all_processes;
}

template< typename T >
snapshot<T>::snapshot( typename snapshot<T>::flags flags )
    : std::vector< process<T> >( capture_processes< container, T >( flags ) )
{
}

} //namespace 

#endif

