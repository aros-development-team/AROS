/*
 * This file can be overriden in arch/all-$(ARCH)/exec. Currently
 * used only by Windows-hosted port
 */
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

struct Exec_PlatformData
{
    /* No platform-specific data by default */
};

#endif /* __EXEC_PLATFORM_H */
