#include <boost/filesystem.hpp>
#include <iostream>

#include "../config.h"
#include "../src/process.h"
#include "../src/snapshot.h"
#include "../src/cocoa.h"
#include "../src/java.h"
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_WINNT_H
#include <winnt.h>
#endif

#define LAUNCH_TEST( X ) \
    launch_test( X, #X )

static std::string own_name;
static std::string argv_java_minecraft="/System/Library/Java/JavaVirtualMachines/1.6.0.jdk/Contents/Home/bin/java -Xdock:icon=/Users/sdomingues/Library/Application Support/minecraft/assets/objects/99/991b421dfd401f115241601b2b373140a8d78572 -Xdock:name=Minecraft -Xmx1G -XX:+UseConcMarkSweepGC -XX:+CMSIncrementalMode -XX:-UseAdaptiveSizePolicy -Xmn128M -Djava.library.path=/Users/sdomingues/Library/Application Support/minecraft/versions/1.7.10/1.7.10-natives-1407418625799030000 -cp /Users/sdomingues/Library/Application Support/minecraft/libraries/com/mojang/realms/1.3.3/realms-1.3.3.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/apache/commons/commons-compress/1.8.1/commons-compress-1.8.1.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/apache/httpcomponents/httpclient/4.3.3/httpclient-4.3.3.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/commons-logging/commons-logging/1.1.3/commons-logging-1.1.3.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/apache/httpcomponents/httpcore/4.3.2/httpcore-4.3.2.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/java3d/vecmath/1.3.1/vecmath-1.3.1.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/net/sf/trove4j/trove4j/3.0.3/trove4j-3.0.3.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/ibm/icu/icu4j-core-mojang/51.2/icu4j-core-mojang-51.2.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/net/sf/jopt-simple/jopt-simple/4.5/jopt-simple-4.5.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/paulscode/codecjorbis/20101023/codecjorbis-20101023.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/paulscode/codecwav/20101023/codecwav-20101023.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/paulscode/libraryjavasound/20101123/libraryjavasound-20101123.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/paulscode/librarylwjglopenal/20100824/librarylwjglopenal-20100824.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/paulscode/soundsystem/20120107/soundsystem-20120107.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/io/netty/netty-all/4.0.10.Final/netty-all-4.0.10.Final.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/google/guava/guava/15.0/guava-15.0.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/apache/commons/commons-lang3/3.1/commons-lang3-3.1.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/commons-io/commons-io/2.4/commons-io-2.4.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/commons-codec/commons-codec/1.9/commons-codec-1.9.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/net/java/jinput/jinput/2.0.5/jinput-2.0.5.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/net/java/jutils/jutils/1.0.0/jutils-1.0.0.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/google/code/gson/gson/2.2.4/gson-2.2.4.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/com/mojang/authlib/1.5.16/authlib-1.5.16.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/apache/logging/log4j/log4j-api/2.0-beta9/log4j-api-2.0-beta9.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/apache/logging/log4j/log4j-core/2.0-beta9/log4j-core-2.0-beta9.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/lwjgl/lwjgl/lwjgl/2.9.1/lwjgl-2.9.1.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/org/lwjgl/lwjgl/lwjgl_util/2.9.1/lwjgl_util-2.9.1.jar:/Users/sdomingues/Library/Application Support/minecraft/libraries/tv/twitch/twitch/5.16/twitch-5.16.jar:/Users/sdomingues/Library/Application Support/minecraft/versions/1.7.10/1.7.10.jar net.minecraft.client.main.Main --username Player --version 1.7.10 --gameDir /Users/sdomingues/Library/Application Support/minecraft --assetsDir /Users/sdomingues/Library/Application Support/minecraft/assets --assetIndex 1.7.10 --uuid 00000000-0000-0000-0000-000000000000 --accessToken 83b2f48ee15640ee82f0428636f2207c --userProperties {} --userType legacy --demo";

#if HAVE_SIGNAL_H
static volatile bool signaled = false;
void sig_handler( int signo )
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
    ps::snapshot all_processes;
    for ( auto first( all_processes.begin() ),
            last ( all_processes.end()   );
            first != last; first++        )
    {
        if ( !( *first ).valid() )
            continue;

        count++;
    }
    return count != 0;
}

bool test_desktop_processes()
{
    ps::snapshot all_processes( ps::snapshot::ENUMERATE_DESKTOP_APPS );
    return !all_processes.empty();
}

bool test_bsd_processes()
{
    ps::snapshot all_processes( ps::snapshot::ENUMERATE_BSD_APPS );
    return !all_processes.empty();
}

bool find_myself()
{
    using boost::filesystem::path;
    ps::snapshot all_processes;
    return std::find_if(
               all_processes.begin(),
               all_processes.end(),
               []( const ps::process & t )
    {
        return equivalent( path( t.cmdline() ), path( own_name ) );
    }
           ) != all_processes.end();
}

bool test_version()
{
    // find at least one process which version is not null
    ps::snapshot all_processes;
    return std::find_if(
               all_processes.begin(),
               all_processes.end(),
               []( const ps::process & t )
    {
        return !t.version().empty();
    }
           ) != all_processes.end();
}

bool test_title()
{
    // find at least one process which title is not null
    ps::snapshot all_processes;
    return std::find_if(
               all_processes.begin(),
               all_processes.end(),
               []( const ps::process & t )
    {
        return !t.title().empty();
    }
           ) != all_processes.end();
}

bool test_icon()
{
    // find at least one process which icon is not an empty array
    ps::snapshot all_processes;
    return std::find_if(
               all_processes.begin(),
               all_processes.end(),
               []( const ps::process & t )
    {
        return !t.icon().empty();
    }
           ) != all_processes.end();
}

bool test_soft_kill()
{
#if HAVE_SIGNAL_H
    // try to kill oneself but catch the signal when it arrives
    signal( SIGTERM, sig_handler );
    ps::snapshot all_processes;

    auto myself = std::find_if(
                      all_processes.begin(),
                      all_processes.end(),
                      []( const ps::process & t )
    {
        return t.pid() == getpid();
    }
                  );

    myself->kill( true );
    sleep( 1 );
    return signaled;
#elif HAVE_SHELLEXECUTE
    ShellExecute( NULL, NULL, "notepad.exe", NULL, NULL, SW_SHOWNORMAL );
    Sleep( 1000 );
    ps::snapshot all_processes;
    auto notepad = std::find_if(
                       all_processes.begin(),
                       all_processes.end(),
                       []( const ps::process & t )
    {
        return t.name() == "Notepad";
    }
                   );

    if ( notepad == all_processes.end() )
        return false;

    notepad->kill( true );
    Sleep( 1000 );
    ps::snapshot all_processes_after;
    if ( std::find_if(
                all_processes_after.begin(),
                all_processes_after.end(),
                []( const ps::process & t )
{
    return t.name() == "Notepad";
    }
            ) == all_processes_after.end() )
    return true;

#endif
    return false;
}

bool test_hard_kill()
{
#if HAVE_EXECVE && HAVE_SLEEP && HAVE_FORK
    const pid_t pid = fork();
    if ( pid == 0 )
    {
        char * const argv[] = { ( char * )"/usr/bin/sleep", ( char * )"5", NULL };
        execve( "/usr/bin/sleep", argv, nullptr );
    }

    sleep( 1 );
    ps::process child_process( pid );
    if ( !child_process.valid() )
        return false;

    if ( child_process.kill( true ) == 0 )
        return true;
#elif HAVE_SHELLEXECUTE
    ShellExecute( NULL, NULL, "notepad.exe", NULL, NULL, SW_SHOWNORMAL );
    Sleep( 1000 );
    ps::snapshot all_processes;
    auto notepad = std::find_if(
                       all_processes.begin(),
                       all_processes.end(),
                       []( const ps::process & t )
    {
        return t.name() == "Notepad";
    }
                   );

    if ( notepad == all_processes.end() )
        return false;

    notepad->kill( false );
    Sleep( 1000 );
    ps::snapshot all_processes_after;
    if ( std::find_if(
                all_processes_after.begin(),
                all_processes_after.end(),
                []( const ps::process & t )
{
    return t.name() == "Notepad";
    }
            ) == all_processes_after.end() )
    return true;
#endif
    return false;
}

bool test_foreground_process()
{
    return ps::get_application_in_foreground().valid();
}

bool test_foreground_process_has_icon()
{
    const auto foreground_process = ps::get_application_in_foreground();
    return !foreground_process.icon().empty();
}

bool test_get_argv_from_pid()
{
    using boost::filesystem::path;

    ps::snapshot all_processes;
    const auto myself = std::find_if( 
            all_processes.cbegin(),
            all_processes.cend(),
            []( const ps::process & p ) { 
                return equivalent( path( p.cmdline() ), path( own_name ) );
        }
    );

    const auto argv =
        ps::details::get_argv_from_pid( myself->pid() );

    if ( argv.empty() )
        return false;

    return true;
}

bool test_extract_name_and_icon_from_argv()
{
    const auto & name_and_icon =
        ps::extract_name_and_icon_from_argv( argv_java_minecraft );

    if ( name_and_icon.first.empty() || name_and_icon.second.empty() )
        return false;

    return true;
}

bool test_get_package_id()
{
#if HAVE_WINNT_H
    const auto windows_version = GetVersion();
    const auto major = windows_version.dwMajorVersion;
    const auto minor = windows_version.dwMinorVersion;

    if ( major != 6 || !( major == 2 || minor == 3 ) )
        return true;

    ps::snapshot all_processes;
    const auto metro = std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process & p ) {
            return !ps::details::get_package_id( p.pid() ).empty();
        }
    );
#else
    return true;
#endif
}

int main( int, char * argv[] )
{
    own_name = boost::filesystem::canonical( argv[0] ).string();
    LAUNCH_TEST( count_processes );
    LAUNCH_TEST( find_myself );
    LAUNCH_TEST( test_desktop_processes );
    LAUNCH_TEST( test_bsd_processes );
    LAUNCH_TEST( test_version );
    LAUNCH_TEST( test_title );
    LAUNCH_TEST( test_icon );
    LAUNCH_TEST( test_foreground_process );
    LAUNCH_TEST( test_foreground_process_has_icon );
    LAUNCH_TEST( test_get_argv_from_pid );
    LAUNCH_TEST( test_extract_name_and_icon_from_argv );
    LAUNCH_TEST( test_get_package_id );
    LAUNCH_TEST( test_soft_kill );
    LAUNCH_TEST( test_hard_kill );
}
