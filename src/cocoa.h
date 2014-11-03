#ifndef PS_COCOA_H
#define PS_COCOA_H

#include <sys/types.h>

int getDesktopApplications( pid_t * pidArray, 
                            char ** bundleIdentifierArray,
                            char ** bundleNameArray,
                            int length );


#endif // PS_COCOA_H
