#import <Foundation/Foundation.h>
#import <AppKit/NSRunningApplication.h>
#import <AppKit/NSWorkspace.h>
#include "cocoa.h"

int get_desktop_applications( pid_t * pidArray, 
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
                       char ** name,
                       char ** version )
{
    NSAutoreleasePool *p = [NSAutoreleasePool new];

    ProcessSerialNumber psn = { kNoProcess, kNoProcess };

    if ( GetProcessForPID(pid, &psn) != noErr )
    {
        [ p release ];
        return -1;
    }

    CFDictionaryRef dictionary 
        = ProcessInformationCopyDictionary(&psn, kProcessDictionaryIncludeAllInformationMask);

    if ( !dictionary )
    {
        [ p release ];
        return -2;
    }

    NSString * path 
        = (NSString*) CFDictionaryGetValue( dictionary, CFSTR("BundlePath") );
    if ( !path )
    {
        [ p release ];
        return -3;
    }

    NSBundle * bundle = [NSBundle bundleWithPath:path];
    if ( !bundle )
    {
        [ p release ];
        return -4;
    }

    NSDictionary * infoDic = [bundle infoDictionary];
    if ( !infoDic )
    {
        [ p release ];
        return -5;
    }

    if (title)
    {
        NSString * data = [ infoDic valueForKey:@"CFBundleName"];
        *title = strdup( data ? [ data UTF8String ] : "" );
    }

    if (name)
    {
        NSString * data = [ infoDic valueForKey:@"CFBundleIdentifier"];
        *name = strdup( data ? [ data UTF8String ] : "" );
    }
    if (version)
    {
        NSString * data = [ infoDic valueForKey:@"CFBundleShortVersionString"];
        *version = strdup( data ? [ data UTF8String ] : "" );
    }

    [ p release ];
    return 0;
}

