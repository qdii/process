#ifndef PS_COCOA_H
#define PS_COCOA_H

#include <sys/types.h>

int getDesktopApplicationPIDs( pid_t * pidArray, unsigned * length );
int getDesktopApplicationBundleIdentifiers( char ** bundleIdentifiers, unsigned * length );


#endif // PS_COCOA_H
