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

#define GET_THIS_TASK           (SysBase->ThisTask)
#define SET_THIS_TASK(x)        (SysBase->ThisTask=(x))

#endif /* __EXEC_PLATFORM_H */
