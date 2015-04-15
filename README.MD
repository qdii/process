Process is a very simple library to quickly find information about running processes. It works on Mac, Linux and Windows.

Here is a program that writes current running desktop application's pids and titlebars:

```c++
#include <iostream>
#include "snapshot.h"
int main()
{
    ps::snapshot running_processes = ps::capture();

    // writes the applications titles to the standard output
    for ( const auto & process : running_processes )
        std::cout << "[" << process.pid() << "]: " << process.title() << '\n';

    // dumps application' icons to their file
    for ( const auto & process : running_processes )
    {
        const auto icon = process.icon();
        std::ofstream( std::string( "" ) + process.pid() + std::string( ".png" ) )
            .write( (char*)&icon[0], icon.size() );
    }
}

```
