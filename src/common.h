#ifndef PS_COMMON_H
#define PS_COMMON_H

#include <string>
#include <assert.h>
#include <boost/filesystem.hpp>
#include <vector>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/current_function.hpp>
#include <utility>
#include <sys/sysctl.h>
#include <pwd.h>

#ifdef __APPLE__
#   include <TargetConditionals.h>
#   ifdef TARGET_OS_MAC
#       include <sys/proc_info.h>
#       include <sys/sysctl.h>
#       include <libproc.h>
#   endif
#endif

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
OS target_os()
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
bool target_os_windows()
{
    return target_os() == WIN32 || target_os() == WIN64;
}

} // namespace ps
#endif // PS_COMMON_H
