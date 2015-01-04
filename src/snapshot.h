#ifndef PS_SNAPSHOT_H
#define PS_SNAPSHOT_H

#include "config.h"
#include "common.h"
#include "process.h"
#include "cocoa.h"
#include "icon.h"

namespace ps
{
/**@struct snapshot
 * @brief A snapshot of all running processes at a given moment
 */
typedef ::std::vector< process > snapshot;
enum flags
{
    ENUMERATE_DESKTOP_APPS = 0x1,
    ENUMERATE_BSD_APPS     = 0x2,
    ENUMERATE_ALL          = 0x3
};

snapshot get_entries_from_window_manager()
{
    snapshot processes;
#if HAVE_LIBWNCK

    if ( !gdk_init_check( NULL, NULL ) )
        return snapshot();

    WnckScreen * const screen
        = wnck_screen_get_default();

    if ( !screen )
        return snapshot();

    wnck_screen_force_update( screen );

    for ( GList * window_l = wnck_screen_get_windows ( screen ); window_l != NULL;
            window_l = window_l->next )
    {
        WnckWindow * window = WNCK_WINDOW( window_l->data );
        if ( !window )
            continue;

        const pid_t pid = wnck_window_get_pid( window );
        assert( pid != ps::INVALID_PID );
        processes.emplace_back(
            pid,
            get_cmdline_from_pid( pid ),
            wnck_window_get_name( window )
        );
    }
#elif HAVE_WINUSER_H

    std::vector< pid_t > pids;
    if ( !EnumWindows( &details::find_pid_from_window,
                       reinterpret_cast<LPARAM>( &pids ) ) )
        return snapshot();

    for ( const pid_t pid : pids )
        processes.emplace_back( pid );

#else
    const int nb_of_applications =
        get_desktop_applications( nullptr, nullptr, nullptr, 0 );

    std::vector< pid_t > pid_array( nb_of_applications );
    std::vector< char * > bundle_identifier_array( nb_of_applications );
    std::vector< char * > bundle_name_array( nb_of_applications );
    const int success = get_desktop_applications(
                            const_cast<pid_t *>( pid_array.data() ),
                            const_cast<char **>( bundle_identifier_array.data() ),
                            const_cast<char **>( bundle_name_array.data() ),
                            nb_of_applications
                        );
    if ( success < 0 )
        goto exit;

    for ( int i = 0; i < success; ++i )
    {
        if ( pid_array[i] == INVALID_PID )
            continue;

        processes.emplace_back( pid_array[i] );
    }

exit:
    std::for_each( bundle_identifier_array.begin(), bundle_identifier_array.end(),
                   &free );
    std::for_each( bundle_name_array.begin(), bundle_name_array.end(), &free );

#endif
    return processes;
}

static inline
std::vector<pid_t> get_running_process_ids( const unsigned max_pids = 500 )
{
    std::vector<pid_t> pids;
    pids.resize( max_pids, INVALID_PID );

#if HAVE_LIBPROC_H
    proc_listpids( PROC_ALL_PIDS, 0,
                   const_cast<pid_t *>( pids.data() ),
                   max_pids );
#elif HAVE_ENUMPROCESSES
    DWORD nbBytes;
    EnumProcesses( const_cast<pid_t *>( pids.data() ),
                   max_pids * sizeof( pid_t ), &nbBytes );
    pids.resize( nbBytes / sizeof( pid_t ) );
#endif

    return pids;
}

snapshot get_entries_from_syscall()
{
#if HAVE_LIBPROC_H
    const unsigned max_pids = proc_listpids( PROC_ALL_PIDS, 0, nullptr, 0 );
#else
    const unsigned max_pids = 500;
#endif

    std::vector<pid_t> running_pids
        = get_running_process_ids( max_pids );

    return snapshot(
               running_pids.begin(),
               std::remove( running_pids.begin(), running_pids.end(), INVALID_PID )
           );
}

static inline
bool read_entry_from_procfs( boost::filesystem::directory_iterator pos,
                             std::string & cmdline, pid_t & pid )
{
    using namespace boost::filesystem;

    // the path of a directory entry of /proc, something like /proc/12113
    const path entry_path = absolute( pos->path() );

    if ( !is_directory( entry_path ) )
        return false;

    // we store the pid, which is the *filename* of the entry (/proc/12113)
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
    std::ifstream cmdline_file( path_to_cmdline.c_str() );
    if ( !cmdline_file.is_open() )
        return false;

    // /proc/11241/cmdline contains the full name of the executable
    std::getline( cmdline_file, cmdline );

    return true;
}

static inline
bool read_entry_from_procfs(
    boost::filesystem::directory_iterator pos,
    process & out )
{
    std::string cmdline;
    pid_t pid;

    if ( read_entry_from_procfs( pos, cmdline, pid ) )
    {
        out = process( pid, cmdline );
        return true;
    }

    return false;
}


inline
std::string get_cmdline_from_pid( const pid_t pid )
{
    using namespace ps::details;
#if HAVE_LIBPROC_H
    if ( pid == INVALID_PID )
        return "";

    std::string cmdline;
    cmdline.resize( PROC_PIDPATHINFO_MAXSIZE );

    const int ret = proc_pidpath( pid, ( char * )cmdline.c_str(), cmdline.size() );
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
              MAX_PATH );

    if ( length == 0 )
        return "";

    return convert_kernel_drive_to_msdos_drive(
               std::string( buffer.get(), buffer.get() + length ) );
#else
    boost::filesystem::directory_iterator pos( "/proc" );
    for ( ; pos != boost::filesystem::directory_iterator(); ++pos )
    {
        std::string cmdline;
        pid_t current_pid;
        if ( read_entry_from_procfs( pos, cmdline, current_pid ) && pid == current_pid )
            return cmdline;
    }
    return "";
#endif
}

inline
snapshot get_entries_from_procfs()
{
    using namespace boost::filesystem;
    snapshot all_processes;

    try
    {
        directory_iterator pos( "/proc" );
        // until this is the last iterator, try and retrieve the process info
        for ( ; pos != directory_iterator(); ++pos )
        {
            process next_process;
            if ( read_entry_from_procfs( pos, next_process ) )
                all_processes.push_back( PS_MOVE( next_process ) );
        }
    }
    catch ( boost::filesystem::filesystem_error & )
    {
    }

    return all_processes;
}

inline
snapshot capture( const ps::flags flags = ps::ENUMERATE_ALL )
{
    using namespace ps::details;
    snapshot all_processes;

    if ( flags & ps::ENUMERATE_BSD_APPS )
    {
        const snapshot bsd_processes = get_entries_from_syscall();
        all_processes.insert( all_processes.end(), bsd_processes.begin(),
                              bsd_processes.end() );

        const snapshot procfs_entries = get_entries_from_procfs();
        all_processes.insert( all_processes.end(), procfs_entries.begin(),
                              procfs_entries.end() );
    }

    if ( flags & ps::ENUMERATE_DESKTOP_APPS )
    {
        const snapshot gui_applications =
            get_entries_from_window_manager();
        all_processes.insert( all_processes.end(), gui_applications.begin(),
                              gui_applications.end() );
    }

    return all_processes;
}

#if !HAVE_APPKIT_NSRUNNINGAPPLICATION_H || !HAVE_APPKIT_NSWORKSPACE_H || !HAVE_FOUNDATION_FOUNDATION_H
pid_t get_foreground_pid()
{
#if HAVE_WINUSER_H
    return details::get_pid_from_top_window( GetForegroundWindow() );
#else
    return INVALID_PID;
#endif
}
#endif

inline
process get_application_in_foreground()
{
#if HAVE_LIBWNCK
    WnckScreen * const screen = wnck_screen_get_default();
    if ( !screen )
        return process();

    WnckWindow * const window = wnck_screen_get_active_window( screen );
    if ( !window )
        return process();

    const pid_t pid = wnck_window_get_pid( window );
    return process(
               pid,
               get_cmdline_from_pid( pid ),
               wnck_window_get_name( window )
           );
#else
    return process( get_foreground_pid() );
#endif
}

} //namespace

#endif

