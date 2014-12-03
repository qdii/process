#include "../src/snapshot.h"
#include "../src/process.h"
#include "../src/cocoa.h"

#if defined __APPLE__ && defined PS_COCOA
static const char extension[] = ".icns";
#else
static const char extension[] = ".png";
#endif


int main()
{
    ps::snapshot<int> all_processes;
    for ( const ps::process<int> & p : all_processes )
    {
        std::string filename = p.title() + extension;
        std::ofstream icon_file( filename, std::ios_base::binary );

        std::vector< unsigned char > icon_data = p.icon();
        icon_file.write( reinterpret_cast< char* >(&icon_data[0]), icon_data.size() );
    }
}
