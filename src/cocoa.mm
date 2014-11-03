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
                bundleIdentifierArray[i] = strdup( [[ app bundleIdentifier ] UTF8String ] );

            if ( bundleNameArray )
                bundleNameArray[i] = strdup( [[ app localizedName ] UTF8String ] );
        }
    }
    [ p release ];
    return max;
}

int get_info_from_pid( pid_t pid, char ** title,
                                  char ** name )
{
    NSAutoreleasePool *p = [NSAutoreleasePool new];

    ProcessSerialNumber psn = { kNoProcess, kNoProcess };
    
    if ( GetProcessForPID(pid, &psn) != noErr )
    {
        [ p release ];
        return -1;
    }

    CFDictionaryRef info 
        = ProcessInformationCopyDictionary(&psn, kProcessDictionaryIncludeAllInformationMask);

    if ( !info )
    {
        [ p release ];
        return -2;
    }

    if (title) 
    {
        const NSString * bundleIdentifier 
            = reinterpret_cast<const NSString*>(CFDictionaryGetValue( info, CFSTR("CFBundleIdentifier") ));
        if ( !bundleIdentifier )
            *title = strdup("");
        else
        {
            *title = strdup( [ bundleIdentifier UTF8String ] );
            CFRelease( bundleIdentifier );
        }
    }

    if (name)
    {
        const NSString * bundleName 
            = reinterpret_cast<const NSString*>(CFDictionaryGetValue( info,  CFSTR("CFBundleName") ));
        if ( !bundleName )
            *name = strdup("");
        else
        { 
            *name = strdup( [ bundleName UTF8String ] );
            CFRelease( bundleName );
        }
    }

    [ p release ];
    return 0;
}

