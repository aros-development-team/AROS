/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: internal definitions for BOOPSI classes.
    Lang: english
*/

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef DOS_BPTR_H
#include <dos/bptr.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#include "icclass.h"

struct IntBOOPSIBase
{
    struct Library		 bb_LibNode;
    struct ExecBase	       * bb_SysBase;
    struct Library	       * bb_UtilityBase;
    BPTR			 bb_SegList;

    struct SignalSemaphore	 bb_ClassListLock;
    struct MinList		 bb_ClassList;
};

#define GetBBase(lib)           ((struct IntBOOPSIBase *)(lib))
#define SysBase 		(GetBBase(BOOPSIBase)->bb_SysBase)
#define UtilityBase		(GetBBase(BOOPSIBase)->bb_UtilityBase)

#ifndef NO_INLINE_STDARG
#   define NO_INLINE_STDARG 1
#endif
