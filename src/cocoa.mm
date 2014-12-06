#include "config.h"
#if HAVE_APPKIT_NSRUNNINGAPPLICATION_H && HAVE_APPKIT_NSWORKSPACE_H && HAVE_FOUNDATION_FOUNDATION_H
#import <Foundation/Foundation.h>
#import <AppKit/NSRunningApplication.h>
#import <AppKit/NSWorkspace.h>
#endif
#include "cocoa.h"

#if HAVE_APPKIT_NSRUNNINGAPPLICATION_H && HAVE_APPKIT_NSWORKSPACE_H && HAVE_FOUNDATION_FOUNDATION_H
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
#endif

#if HAVE_APPKIT_NSRUNNINGAPPLICATION_H && HAVE_APPKIT_NSWORKSPACE_H && HAVE_FOUNDATION_FOUNDATION_H
int get_info_from_pid( pid_t pid, char ** title,
                       char ** name,
                       char ** version,
                       char ** icon,
                       char ** path )
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

    NSString * bundlePath 
        = (NSString*) CFDictionaryGetValue( dictionary, CFSTR("BundlePath") );
    if ( !bundlePath )
    {
        [ p release ];
        return -3;
    }

    NSBundle * bundle = [NSBundle bundleWithPath:bundlePath];
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

    if (icon)
    {
        NSString * data = [ infoDic valueForKey:@"CFBundleIconFile"];
        *icon = strdup( data ? [ data UTF8String ] : "" );
    }

    if (path)
        *path = strdup( [ bundlePath UTF8String ] );
 
    [ p release ];
    return 0;
}
#endif

#if HAVE_APPKIT_NSRUNNINGAPPLICATION_H && HAVE_APPKIT_NSWORKSPACE_H && HAVE_FOUNDATION_FOUNDATION_H
pid_t get_foreground_pid()
{
    NSAutoreleasePool *p = [NSAutoreleasePool new];

    pid_t pid;
    ProcessSerialNumber psn = { kNoProcess, kNoProcess };
 
    if ( GetFrontProcess( &psn ) != noErr )
    {
        [ p release ];
        return 0;
    }

    if ( GetProcessPID( &psn, &pid ) != noErr )
    {
        [ p release ];
        return 0;
    }

    [ p release ];
    return pid;
}
#endif

