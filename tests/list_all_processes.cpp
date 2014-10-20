#include <iostream>

#include "../src/process.h"
#include "../src/list.h"

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
    for (ps::iterator<void> first( ps::begin<void>() ),
                            last ( ps::end<void>()   ); 
                            first != last; first++      )
    {
        count++;
    }
    return count != 0;
}

int main()
{
    LAUNCH_TEST( count_processes );
}
