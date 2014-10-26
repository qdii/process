#ifndef PS_COMMON_H
#define PS_COMMON_H

namespace ps
{
enum OS
{
    WIN32,
    WIN64,
    IOS_SIMULATOR,
    IOS_DEVICE,
    MAC_OSX,
    LINUX,
    UNIX,
    POSIX,
    UNKNOWN
};

static constexpr 
OS TARGET_OS()
{
#ifdef _WIN32
#   ifdef _WIN64
        return WIN64;
#   else
        return WIN32;
#   endif
#elif __APPLE__
#   include "TargetConditionals.h"
#   if TARGET_IPHONE_SIMULATOR
        return IOS_SIMULATOR; 
#   elif TARGET_OS_IPHONE
        return IOS_DEVICE;
#   elif TARGET_OS_MAC
        return MAC_OSX;     
#   else
        return UNKNOWN; 
#   endif
#elif __linux
    return LINUX; 
#elif __unix // all unices not caught above
    return UNIX; 
#elif __posix
    return POSIX; 
#else
    return UNKNOWN;
#endif
}

static constexpr
bool TARGET_OS_WINDOWS()
{
    return TARGET_OS() == WIN32 || TARGET_OS() == WIN64;
}

} // namespace ps
#endif // PS_COMMON_H
