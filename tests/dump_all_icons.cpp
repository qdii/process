#include "../src/snapshot.h"
#include "../src/process.h"
#include "../src/cocoa.h"

int main()
{
    ps::snapshot all_processes = ps::capture();
    for ( const ps::process & p : all_processes )
    {
        std::vector< unsigned char > icon_data = p.icon();

        if ( icon_data.empty() )
        {
            std::cout << "warning: empty icon file for: ";
            ps::describe( std::cout, p );
            continue;
        }

        std::ofstream( p.title(), std::ios_base::binary )
        .write( reinterpret_cast< char * >( &icon_data[0] ), icon_data.size() );
    }
}
