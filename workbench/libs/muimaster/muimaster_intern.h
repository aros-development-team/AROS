#ifndef MUIMASTER_INTERN_H
#define MUIMASTER_INTERN_H

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Internal definitions for muimaster.library
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif

#ifdef _AROS
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef CLIB_BOOPSISTUBS_H
#   include <clib/boopsistubs.h>
#endif

#else

#define AROS_LIBFUNC_INIT
#define AROS_LIBBASE_EXT_DECL(a,b) extern a b;
#define AROS_LIBFUNC_EXIT

#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif


/* Sometype defs in AROS */
#ifndef _AROS
#ifndef _AROS_TYPES_DEFINED
typedef unsigned long IPTR;
typedef long STACKLONG;
typedef unsigned long STACKULONG;
#define _AROS_TYPES_DEFINED
#endif
#endif

/****************************************************************************************/

struct MUIMasterBase_intern
{
    struct Library		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;

    struct DosLibrary  	    	*dosbase;
    struct UtilityBase		*utilitybase;
    struct Library  	    	*aslbase;
    struct GfxBase  	    	*gfxbase;
    struct Library  	    	*layersbase;
    struct IntuitionBase    	*intuibase;
    struct Library  	    	*cxbase;
    struct Library  	    	*keymapbase;
    struct Library		*gadtoolsbase;
    struct SignalSemaphore ClassSempahore;
    struct IClass **Classes;
    int     ClassCount;
    int     ClassSpace;
};

/****************************************************************************************/

#undef MUIMB
#define MUIMB(b)	((struct MUIMasterBase_intern *)b)

#ifdef _AROS

#undef SysBase
#define SysBase     	(MUIMB(MUIMasterBase)->sysbase)

#undef DOSBase
#define DOSBase     	(MUIMB(MUIMasterBase)->dosbase)

#undef UtilityBase
#define UtilityBase	(MUIMB(MUIMasterBase)->utilitybase)

#undef AslBase
#define AslBase     	(MUIMB(MUIMasterBase)->aslbase)

#undef GfxBase
#define GfxBase     	(MUIMB(MUIMasterBase)->gfxbase)

#undef LayersBase
#define LayersBase     	(MUIMB(MUIMasterBase)->layersbase)

#undef IntuitionBase
#define IntuitionBase  	(MUIMB(MUIMasterBase)->intuibase)

#undef CxBase
#define CxBase	    	(MUIMB(MUIMasterBase)->cxbase)

#undef KeymapBase
#define KeymapBase  	(MUIMB(MUIMasterBase)->keymapbase)

#undef GadToolsBase
#define GadToolsBase  	(MUIMB(MUIMasterBase)->gadtoolsbase)

#else

#undef SysBase
#define SysBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->sysbase)

#undef DOSBase
#define DOSBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->dosbase)

#undef UtilityBase
#define UtilityBase	(((struct MUIMasterBase_intern *)MUIMasterBase)->utilitybase)

#undef AslBase
#define AslBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->aslbase)

#undef GfxBase
#define GfxBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->gfxbase)

#undef LayersBase
#define LayersBase     	(((struct MUIMasterBase_intern *)MUIMasterBase)->layersbase)

#undef IntuitionBase
#define IntuitionBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->intuibase)

#undef CxBase
#define CxBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->cxbase)

#undef KeymapBase
#define KeymapBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->keymapbase)

#undef GadToolsBase
#define GadToolsBase  	(((struct MUIMasterBase_intern *)MUIMasterBase)->gadtoolsbase)

#endif

/****************************************************************************************/

#endif /* MUIMASTER_INTERN_H */
