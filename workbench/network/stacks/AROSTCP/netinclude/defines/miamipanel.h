#ifndef DEFINES_MIAMIPANEL_PROTOS_H
#define DEFINES_MIAMIPANEL_PROTOS_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
*/

/*
    Desc: Defines for miamipanel
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <aros/preprocessor/variadic/cast2iptr.hpp>


#define __MiamiPanelInit_WB(__MiamiPanelBase, __arg1, __arg2, __arg3, __arg4, __arg5, __arg6, __arg7, __arg8) \
        AROS_LC8(LONG, MiamiPanelInit, \
                  AROS_LCA(IPTR,(__arg1),A0), \
                  AROS_LCA(IPTR,(__arg2),A1), \
                  AROS_LCA(LONG,(__arg3),D0), \
                  AROS_LCA(STRPTR,(__arg4),A2), \
                  AROS_LCA(STRPTR,(__arg5),A3), \
                  AROS_LCA(LONG,(__arg6),D1), \
                  AROS_LCA(LONG,(__arg7),D2), \
                  AROS_LCA(IPTR,(__arg8),A4), \
        struct Library *, (__MiamiPanelBase), 5, MiamiPanel)

#define MiamiPanelInit(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
    __MiamiPanelInit_WB(MiamiPanelBase, (arg1), (arg2), (arg3), (arg4), (arg5), (arg6), (arg7), (arg8))

#define __MiamiPanelCleanup_WB(__MiamiPanelBase) \
        AROS_LC0NR(void, MiamiPanelCleanup, \
        struct Library *, (__MiamiPanelBase), 6, MiamiPanel)

#define MiamiPanelCleanup() \
    __MiamiPanelCleanup_WB(MiamiPanelBase)

#define __MiamiPanelAddInterface_WB(__MiamiPanelBase, __arg1, __arg2, __arg3, __arg4, __arg5) \
        AROS_LC5NR(void, MiamiPanelAddInterface, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(STRPTR,(__arg2),A0), \
                  AROS_LCA(LONG,(__arg3),D1), \
                  AROS_LCA(LONG,(__arg4),D2), \
                  AROS_LCA(STRPTR,(__arg5),A1), \
        struct Library *, (__MiamiPanelBase), 7, MiamiPanel)

#define MiamiPanelAddInterface(arg1, arg2, arg3, arg4, arg5) \
    __MiamiPanelAddInterface_WB(MiamiPanelBase, (arg1), (arg2), (arg3), (arg4), (arg5))

#define __MiamiPanelDelInterface_WB(__MiamiPanelBase, __arg1) \
        AROS_LC1NR(void, MiamiPanelDelInterface, \
                  AROS_LCA(LONG,(__arg1),D0), \
        struct Library *, (__MiamiPanelBase), 8, MiamiPanel)

#define MiamiPanelDelInterface(arg1) \
    __MiamiPanelDelInterface_WB(MiamiPanelBase, (arg1))

#define __MiamiPanelSetInterfaceState_WB(__MiamiPanelBase, __arg1, __arg2, __arg3) \
        AROS_LC3NR(void, MiamiPanelSetInterfaceState, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(LONG,(__arg2),D1), \
                  AROS_LCA(LONG,(__arg3),D2), \
        struct Library *, (__MiamiPanelBase), 9, MiamiPanel)

#define MiamiPanelSetInterfaceState(arg1, arg2, arg3) \
    __MiamiPanelSetInterfaceState_WB(MiamiPanelBase, (arg1), (arg2), (arg3))

#define __MiamiPanelSetInterfaceSpeed_WB(__MiamiPanelBase, __arg1, __arg2) \
        AROS_LC2NR(void, MiamiPanelSetInterfaceSpeed, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(UBYTE *,(__arg2),A0), \
        struct Library *, (__MiamiPanelBase), 10, MiamiPanel)

#define MiamiPanelSetInterfaceSpeed(arg1, arg2) \
    __MiamiPanelSetInterfaceSpeed_WB(MiamiPanelBase, (arg1), (arg2))

#define __MiamiPanelInterfaceReport_WB(__MiamiPanelBase, __arg1, __arg2, __arg3, __arg4, __arg5) \
        AROS_LC5NR(void, MiamiPanelInterfaceReport, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(LONG,(__arg2),D1), \
                  AROS_LCA(LONG,(__arg3),D2), \
                  AROS_LCA(LONG,(__arg4),D3), \
                  AROS_LCA(ULONG,(__arg5),D4), \
        struct Library *, (__MiamiPanelBase), 11, MiamiPanel)

#define MiamiPanelInterfaceReport(arg1, arg2, arg3, arg4, arg5) \
    __MiamiPanelInterfaceReport_WB(MiamiPanelBase, (arg1), (arg2), (arg3), (arg4), (arg5))

#define __MiamiPanelToFront_WB(__MiamiPanelBase) \
        AROS_LC0NR(void, MiamiPanelToFront, \
        struct Library *, (__MiamiPanelBase), 12, MiamiPanel)

#define MiamiPanelToFront() \
    __MiamiPanelToFront_WB(MiamiPanelBase)

#define __MiamiPanelInhibitRefresh_WB(__MiamiPanelBase, __arg1) \
        AROS_LC1NR(void, MiamiPanelInhibitRefresh, \
                  AROS_LCA(LONG,(__arg1),D0), \
        struct Library *, (__MiamiPanelBase), 13, MiamiPanel)

#define MiamiPanelInhibitRefresh(arg1) \
    __MiamiPanelInhibitRefresh_WB(MiamiPanelBase, (arg1))

#define __MiamiPanelGetCoord_WB(__MiamiPanelBase, __arg1, __arg2) \
        AROS_LC2NR(void, MiamiPanelGetCoord, \
                  AROS_LCA(LONG *,(__arg1),A0), \
                  AROS_LCA(LONG *,(__arg2),A1), \
        struct Library *, (__MiamiPanelBase), 14, MiamiPanel)

#define MiamiPanelGetCoord(arg1, arg2) \
    __MiamiPanelGetCoord_WB(MiamiPanelBase, (arg1), (arg2))

#define __MiamiPanelEvent_WB(__MiamiPanelBase, __arg1) \
        AROS_LC1NR(void, MiamiPanelEvent, \
                  AROS_LCA(ULONG,(__arg1),D0), \
        struct Library *, (__MiamiPanelBase), 15, MiamiPanel)

#define MiamiPanelEvent(arg1) \
    __MiamiPanelEvent_WB(MiamiPanelBase, (arg1))

#define __MiamiPanelRefreshName_WB(__MiamiPanelBase, __arg1, __arg2) \
        AROS_LC2NR(void, MiamiPanelRefreshName, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(UBYTE *,(__arg2),A0), \
        struct Library *, (__MiamiPanelBase), 16, MiamiPanel)

#define MiamiPanelRefreshName(arg1, arg2) \
    __MiamiPanelRefreshName_WB(MiamiPanelBase, (arg1), (arg2))

#define __MiamiPanelGetVersion_WB(__MiamiPanelBase) \
        AROS_LC0(LONG, MiamiPanelGetVersion, \
        struct Library *, (__MiamiPanelBase), 17, MiamiPanel)

#define MiamiPanelGetVersion() \
    __MiamiPanelGetVersion_WB(MiamiPanelBase)

#define __MiamiPanelKill_WB(__MiamiPanelBase) \
        AROS_LC0(ULONG, MiamiPanelKill, \
        struct Library *, (__MiamiPanelBase), 26, MiamiPanel)

#define MiamiPanelKill() \
    __MiamiPanelKill_WB(MiamiPanelBase)

#endif /* DEFINES_MIAMIPANEL_PROTOS_H*/
