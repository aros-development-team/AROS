/*
 * Extra data for m68k-all
 */
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

#include <exec/types.h>

#define SCHEDQUANTUM_VALUE      4

struct Exec_PlatformData
{
	APTR  realRawDoFmt;     /* AOS 3.1 locale.library workaround - see
	                           exec.library/SetFunction() */
	struct TagItem *BootMsg;
};

#ifdef AROS_NO_ATOMIC_OPERATIONS
#define IDNESTCOUNT_INC                 SysBase->IDNestCnt++
#define IDNESTCOUNT_DEC                 SysBase->IDNestCnt--
#define TDNESTCOUNT_INC                 SysBase->TDNestCnt++
#define TDNESTCOUNT_DEC                 SysBase->TDNestCnt--
#define FLAG_SCHEDQUANTUM_CLEAR         SysBase->SysFlags &= ~SFF_QuantumOver
#define FLAG_SCHEDQUANTUM_SET           SysBase->SysFlags |= SFF_QuantumOver
#define FLAG_SCHEDSWITCH_CLEAR          SysBase->AttnResched &= ~ARF_AttnSwitch
#define FLAG_SCHEDSWITCH_SET            SysBase->AttnResched |= ARF_AttnSwitch
#define FLAG_SCHEDDISPATCH_CLEAR        SysBase->AttnResched &= ~ARF_AttnDispatch
#define FLAG_SCHEDDISPATCH_SET          SysBase->AttnResched |= ARF_AttnDispatch
#else
#define IDNESTCOUNT_INC                 AROS_ATOMIC_INC(SysBase->IDNestCnt)
#define IDNESTCOUNT_DEC                 AROS_ATOMIC_DEC(SysBase->IDNestCnt)
#define TDNESTCOUNT_INC                 AROS_ATOMIC_INC(SysBase->TDNestCnt)
#define TDNESTCOUNT_DEC                 AROS_ATOMIC_DEC(SysBase->TDNestCnt)
#define FLAG_SCHEDQUANTUM_CLEAR         AROS_ATOMIC_AND(SysBase->SysFlags, ~SFF_QuantumOver)
#define FLAG_SCHEDQUANTUM_SET           AROS_ATOMIC_OR(SysBase->SysFlags, SFF_QuantumOver)
#define FLAG_SCHEDSWITCH_CLEAR          AROS_ATOMIC_AND(SysBase->AttnResched, ~ARF_AttnSwitch)
#define FLAG_SCHEDSWITCH_SET            AROS_ATOMIC_OR(SysBase->AttnResched, ARF_AttnSwitch)
#define FLAG_SCHEDDISPATCH_CLEAR        AROS_ATOMIC_AND(SysBase->AttnResched, ~ARF_AttnDispatch)
#define FLAG_SCHEDDISPATCH_SET          AROS_ATOMIC_OR(SysBase->AttnResched, ARF_AttnDispatch)
#endif
#define SCHEDQUANTUM_SET(val)           (SysBase->Quantum=(val))
#define SCHEDQUANTUM_GET                (SysBase->Quantum)
#define SCHEDELAPSED_SET(val)           (SysBase->Elapsed=(val))
#define SCHEDELAPSED_GET                (SysBase->Elapsed)
#define IDNESTCOUNT_GET                 (SysBase->IDNestCnt)
#define IDNESTCOUNT_SET(val)            (SysBase->IDNestCnt=(val))
#define TDNESTCOUNT_GET                 (SysBase->TDNestCnt)
#define TDNESTCOUNT_SET(val)            (SysBase->TDNestCnt=(val))
#define FLAG_SCHEDQUANTUM_ISSET         (SysBase->SysFlags & SFF_QuantumOver)
#define FLAG_SCHEDSWITCH_ISSET          (SysBase->AttnResched & ARF_AttnSwitch)
#define FLAG_SCHEDDISPATCH_ISSET        (SysBase->AttnResched & ARF_AttnDispatch)

#define GET_THIS_TASK                   (SysBase->ThisTask)
#define SET_THIS_TASK(x)                (SysBase->ThisTask=(x))

/* Use a dummy A4 on m68k, because some badly
 * behaving programs may not preserve it.
 */
#define HAVE_ExecDoInitResident
#define ExecDoInitResident(a,b,c) \
({ \
        a = AROS_UFC4(struct Library *, (b), \
        AROS_UFCA(struct Library *,  0L, D0), \
        AROS_UFCA(BPTR,              (c), A0), \
        AROS_UFCA(ULONG,             0L, A4), \
        AROS_UFCA(struct ExecBase *, SysBase, A6) \
    ); \
})
#endif
