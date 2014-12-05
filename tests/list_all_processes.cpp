#include <boost/filesystem.hpp>
#include <iostream>

#include "../src/process.h"
#include "../src/snapshot.h"
#include "../src/cocoa.h"
#if defined __linux || defined __APPLE__
#include <signal.h>
#endif

#define LAUNCH_TEST( X ) \
    launch_test( X, #X )

static std::string own_name;

#if defined __linux || defined __APPLE__
static volatile bool signaled = false;
void sig_handler(int signo)
{
    if ( signo == SIGTERM )
        signaled = true;
}
#endif

static void
launch_test( bool( * test_function )(), std::string name )
{
    std::cout << name << ": ";

    try
    {
        if ( test_function() )
            std::cout << "OK\n";
        else
            std::cout << "FAIL\n";
    }
    catch( ... )
    {
        std::cout << "FAIL\n";
    }
}

bool count_processes()
{
    unsigned count = 0;
    ps::snapshot<int> all_processes;
    for ( auto first( all_processes.cbegin() ),
               last ( all_processes.cend()   );
               first != last; first++        )
    {
        if (!(*first).valid())
            continue;

        count++;
    }
    return count != 0;
}

bool test_desktop_processes()
{
    ps::snapshot<int> all_processes( ps::snapshot<int>::ENUMERATE_DESKTOP_APPS );
    return !all_processes.empty();
}

bool test_bsd_processes()
{
    ps::snapshot<int> all_processes( ps::snapshot<int>::ENUMERATE_BSD_APPS );
    return !all_processes.empty();
}

bool find_myself()
{
    using boost::filesystem::path;
    ps::snapshot<int> all_processes;
    return std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process<int> &t ) { return equivalent(path(t.cmdline()), path(own_name)); } 
    ) != all_processes.cend();
}

bool test_version()
{
    // find at least one process which version is not null
    ps::snapshot<int> all_processes;
    return std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process<int> &t ) { return !t.version().empty(); } 
    ) != all_processes.cend();
}

bool test_title()
{
    // find at least one process which title is not null
    ps::snapshot<int> all_processes;
    return std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process<int> &t ) { return !t.title().empty(); } 
    ) != all_processes.cend();
}

bool test_icon()
{
    // find at least one process which icon is not an empty array
    ps::snapshot<int> all_processes;
    return std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process<int> &t ) { return !t.icon().empty(); } 
    ) != all_processes.cend();
}

bool test_soft_kill()
{
#if defined __linux || defined __APPLE__
    // try to kill oneself but catch the signal when it arrives
    signal( SIGTERM, sig_handler );
    ps::snapshot<int> all_processes;

    auto myself = std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process<int> &t ) { return t.pid() == getpid(); } 
    );

    myself->kill( true );
    sleep(1);
    return signaled;
#elif defined _WIN32
    ShellExecute(NULL, NULL, "notepad.exe", NULL, NULL, SW_SHOWNORMAL);
    Sleep(1);
    ps::snapshot<int> all_processes;
    auto notepad = std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process<int> &t ) { return t.name() == "Notepad"; }
    );

    if ( notepad == all_processes.cend() )
        return false;

    notepad->kill( true );
    Sleep(1);
    ps::snapshot<int> all_processes_after;
    if (std::find_if(
        all_processes_after.cbegin(),
        all_processes_after.cend(),
        []( const ps::process<int> &t ) { return t.name() == "Notepad"; }
    ) == all_processes_after.cend())
        return true;

#endif
    return false;
}

bool test_hard_kill()
{
#if defined __linux
    const pid_t pid = fork();
    if ( pid == 0 )
    {
        char * const argv[] = { (char*)"/usr/bin/sleep",(char*)"5", NULL };
        execve( "/usr/bin/sleep", argv, nullptr );
    }

    sleep(1);
    ps::process<int> child_process( pid );
    if ( !child_process.valid() )
        return false;

    if ( child_process.kill( true ) == 0 )
        return true;
#else if defined(_WIN32)
    if ( test_soft_kill() )
        return true;
#endif
    return false;
}

int main( int, char* argv[] )
{
    own_name = boost::filesystem::canonical( argv[0] ).string();
    LAUNCH_TEST( count_processes );
    LAUNCH_TEST( find_myself );
    LAUNCH_TEST( test_desktop_processes );
    LAUNCH_TEST( test_bsd_processes );
    LAUNCH_TEST( test_version );
    LAUNCH_TEST( test_title );
    LAUNCH_TEST( test_icon );
    LAUNCH_TEST( test_soft_kill );
    LAUNCH_TEST( test_hard_kill );
}
