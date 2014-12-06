#ifndef PS_COCOA_H
#define PS_COCOA_H

#include "config.h"

#if HAVE_APPKIT_NSRUNNINGAPPLICATION_H && HAVE_APPKIT_NSWORKSPACE_H && HAVE_FOUNDATION_FOUNDATION_H
// the caller is responsible for freeing the memory of every string
int get_desktop_applications( pid_t * pidArray, 
                              char ** bundleIdentifierArray,
                              char ** bundleNameArray,
                              int length );

// the caller is responsible for freeing the memory of every string
int get_info_from_pid( pid_t pid,
                       char ** title,
                       char ** name,
                       char ** version,
                       char ** icon,
                       char ** path );

pid_t get_foreground_pid();
#endif // HAVE_APPKIT...

#endif // PS_COCOA_H
