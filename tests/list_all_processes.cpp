#include <iostream>

#include "../src/process.h"
#include "../src/snapshot.h"

#define LAUNCH_TEST( X ) \
    launch_test( X, #X )

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

int main()
{
    LAUNCH_TEST( count_processes );
}
