/*
 * This file can be overriden in arch/all-$(ARCH)/exec. 
 */
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

struct Exec_PlatformData
{
    /* No platform-specific data by default */
};
#define TDNESTCOUNT_GET                 (SysBase->TDNestCnt)
#define TDNESTCOUNT_SET(val)            (SysBase->TDNestCnt=(x))
#define FLAG_SCHEDSWITCH_CLEAR          SysBase->AttnResched &= ~ARF_AttnSwitch
#define FLAG_SCHEDSWITCH_SET            SysBase->AttnResched |= ARF_AttnSwitch
#define FLAG_SCHEDSWITCH_ISSET          (SysBase->AttnResched & ARF_AttnSwitch)
#define FLAG_SCHEDDISPATCH_CLEAR        SysBase->AttnResched &= ~ARF_AttnDispatch
#define FLAG_SCHEDDISPATCH_SET          SysBase->AttnResched |= ARF_AttnDispatch
#define FLAG_SCHEDDISPATCH_ISSET        (SysBase->AttnResched & ARF_AttnDispatch)

#define GET_THIS_TASK                   (SysBase->ThisTask)
#define SET_THIS_TASK(x)                (SysBase->ThisTask=(x))

#endif /* __EXEC_PLATFORM_H */
