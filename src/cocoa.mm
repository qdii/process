#import <Foundation/Foundation.h>
#import <AppKit/NSRunningApplication.h>
#import <AppKit/NSWorkspace.h>
#include "cocoa.h"

int getDesktopApplications( pid_t * pidArray, 
                            char ** bundleIdentifierArray,
                            char ** bundleNameArray,
                            int length )
{
    NSAutoreleasePool *p = [NSAutoreleasePool new];

    NSWorkspace * ws = [NSWorkspace sharedWorkspace];
    NSArray * apps = [ws runningApplications];
    NSUInteger count = [apps count];

    if ( pidArray == 0 && bundleIdentifierArray == 0
                       && bundleNameArray == 0 )
    {
        [p release];
        return count; 
    }

    NSUInteger max = (count > length) ? length:count;
    for (NSUInteger i = 0; i < max; i++)
    {

        NSRunningApplication *app = [apps objectAtIndex: i];

        if(app.activationPolicy == NSApplicationActivationPolicyRegular)
        {
            if ( pidArray )
                pidArray[i] = [app processIdentifier];

            if ( bundleIdentifierArray )
                bundleIdentifierArray[i] = strdup( (char*)[[ app bundleIdentifier ] UTF8String ] );

            if ( bundleNameArray )
                bundleNameArray[i] = strdup( [[ app localizedName ] UTF8String ] );
        }
    }
    [ p release ];
    return max;
}

