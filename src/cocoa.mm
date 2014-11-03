#include "cocoa.h"

int getDesktopApplicationPIDs( pid_t * pidArray, unsigned * length )
{
    NSWorkspace * ws = [NSWorkspace sharedWorkspace];
    NSArray * apps = [ws runningApplications];
    NSUInteger count = [apps count];

    if ( pidArray == 0 && length == 0 )
        return count; 

    if ( length == 0 )
        return -1;

    NSUInteger max = (count > *length) ? *length:count;
    for (NSUInteger i = 0; i < max; i++)
    {
        NSRunningApplication *app = [apps objectAtIndex: i];

        if(app.activationPolicy == NSApplicationActivationPolicyRegular)
            pidArray[i] = [app processIdentifier];
    }
    *length = max;
    return 0;
}

int getDesktopApplicationBundleIdentifiers( char ** bundleIdentifiers, unsigned * length )
{
    NSWorkspace * ws = [NSWorkspace sharedWorkspace];
    NSArray * apps = [ws runningApplications];
    NSUInteger count = [apps count];

    if ( pidArray == 0 && length == 0 )
        return count; 

    if ( length == 0 )
        return -1;

    NSUInteger max = (count > *length) ? *length:count;
    for (NSUInteger i = 0; i < max; i++)
    {
        NSRunningApplication *app = [apps objectAtIndex: i];

        if(app.activationPolicy == NSApplicationActivationPolicyRegular)
        {
            unsigned length = [[ app bundleIdentifier ] length ] + 1;
            char * buffer = malloc( length );
            strcpy( buffer, [[ app bundleIdentifier ] cStringUsingEncoding ] );
            bundleIdentifiers[i] = buffer;
        }
    }
    *length = max;
    return 0;
}
