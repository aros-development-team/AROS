#ifndef INTUITION_INTERN_H
#define INTUITION_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/10/15 15:45:32  digulla
    Two new functions: LockIBase() and UnlockIBase()
    Modified code to make sure that it is impossible to access illegal data (ie.
    	fields of a window which is currently beeing closed).

    Revision 1.6  1996/09/21 15:53:28  digulla
    IntScree structure to store private fields in a screen

    Revision 1.5  1996/09/21 14:18:59  digulla
    Use intuition_debug.h

    Revision 1.4  1996/09/17 16:14:06  digulla
    OpenWindowTagList() needs Utility.library

    Revision 1.3  1996/08/29 13:33:31  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.2  1996/08/28 17:55:37  digulla
    Proportional gadgets
    BOOPSI

    Revision 1.1  1996/08/13 15:37:26  digulla
    First function for intuition.library

    Desc:
    Lang:
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#include "intuition_debug.h"

struct IntIntuitionBase
{
    struct IntuitionBase IBase;

    /* Put local shit here, invisible for the user */
    struct GfxBase	   * GfxBase;
    struct ExecBase	   * SysBase;
    struct UtilityBase	   * UtilBase;
    struct MinList	     ClassList;
    struct Screen	   * WorkBench;
    struct SignalSemaphore * SigSem;

    APTR		     DriverData; /* Pointer which the driver may use */

/*    struct MinList	   PublicScreenList;
    struct Screen      * DefaultPublicScreen; */
};

struct IntScreen
{
    struct Screen Screen;

    /* Private fields */
    struct DrawInfo DInfo;
    UWORD  Pens[NUMDRIPENS];
};

extern struct IntuitionBase * IntuitionBase;

#define GetPubIBase(ib)   ((struct IntuitionBase *)ib)
#define GetPrivIBase(ib)  ((struct IntIntuitionBase *)ib)

#ifdef GfxBase
#undef GfxBase
#endif
#define GfxBase     (GetPrivIBase(IntuitionBase)->GfxBase)
#ifdef SysBase
#undef SysBase
#endif
#define SysBase     (GetPrivIBase(IntuitionBase)->SysBase)
#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase (GetPrivIBase(IntuitionBase)->UtilBase)

#define PublicClassList ((struct List *)&(GetPrivIBase(IntuitionBase)->ClassList))

/* Window-Flags */
#define EWFLG_DELAYCLOSE	0x00000001L /* Delay CloseWindow() */
#define EWFLG_CLOSEWINDOW	0x00000002L /* Call CloseWindow() */

/* Needed for close() */
#define expunge() \
__AROS_LC0(BPTR, expunge, struct IntuitionBase *, IntuitionBase, 3, Intuition)

#endif /* INTUITION_INTERN_H */
