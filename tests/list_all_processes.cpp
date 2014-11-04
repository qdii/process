#include <boost/filesystem.hpp>
#include <iostream>

#include "../src/process.h"
#include "../src/snapshot.h"
#include "../src/cocoa.h"

#define LAUNCH_TEST( X ) \
    launch_test( X, #X )

static std::string own_name;

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
    // find at least one process which version is not null
    ps::snapshot<int> all_processes;
    return std::find_if(
        all_processes.cbegin(),
        all_processes.cend(),
        []( const ps::process<int> &t ) { return !t.title().empty(); } 
    ) != all_processes.cend();
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
}
