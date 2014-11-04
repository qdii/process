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

char * get_specific_info( CFDictionaryRef dictionary, CFStringRef info_type )
{
    NSAutoreleasePool *p = [NSAutoreleasePool new];
    char * information = (char*) 0;

    const NSString * data 
        = reinterpret_cast<const NSString*>(CFDictionaryGetValue( dictionary, info_type ));

    if ( !data )
        information = strdup( "" );
    else
    { 
        information = strdup( [ data UTF8String ] );
        CFRelease( data );
    }

    assert( information != 0 );
    [ p release ];
    return information;
}

char * get_specific_numeric_info( CFDictionaryRef dictionary, CFStringRef info_type )
{
    NSAutoreleasePool *p = [NSAutoreleasePool new];
    char * information = (char*) 0;

    const NSNumber * data 
        = reinterpret_cast<const NSNumber *>(CFDictionaryGetValue( dictionary, info_type ));

    if ( !data )
        information = strdup( "" );
    else
    { 
        information = strdup( [ [ data stringValue ] UTF8String ] );
        CFRelease( data );
    }

    assert( information != 0 );
    [ p release ];
    return information;
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

    CFDictionaryRef info 
        = ProcessInformationCopyDictionary(&psn, kProcessDictionaryIncludeAllInformationMask);

    if ( !info )
    {
        [ p release ];
        return -2;
    }

    if (title) 
        *title = get_specific_info( info, CFSTR("CFBundleIdentifier") );

    if (name)
    {
        char * bundleName = get_specific_info( info, CFSTR("CFBundleName") );
        if ( strcmp( bundleName, "" ) == 0 )
        {
            free( bundleName );
            *name = get_specific_info( info, CFSTR("CFBundleExecutable") );
        }
        else
            *name = bundleName;
    }

    if (version)
        *version = get_specific_info( info, CFSTR("CFBundleShortVersionString") );

    [ p release ];
    return 0;
}

