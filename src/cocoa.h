#ifndef PS_COCOA_H
#define PS_COCOA_H

#include <sys/types.h>

// the caller is responsible for freeing the memory of every string
int getDesktopApplications( pid_t * pidArray, 
                            char ** bundleIdentifierArray,
                            char ** bundleNameArray,
                            int length );

// the caller is responsible for freeing the memory of every string
int get_info_from_pid( pid_t pid,
                       char ** title,
                       char ** name );

#endif // PS_COCOA_H
