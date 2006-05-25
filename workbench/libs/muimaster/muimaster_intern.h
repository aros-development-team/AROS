/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef MUIMASTER_INTERN_H
#define MUIMASTER_INTERN_H

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
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#ifdef __AROS__
#   ifndef AROS_ASMCALL_H
#       include <aros/asmcall.h>
#   endif
#else
#   include "support_amigaos.h"
#endif

#include "mui.h"
#include "textengine.h"
#include "prefs.h"
#include "penspec.h"

/****************************************************************************************/

struct MUIMasterBase_intern
{
    struct Library		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;

    /* On AROS autoopened libraries are used */
#ifndef __AROS__
    struct DosLibrary  	    	*dosbase;
    struct UtilityBase		*utilitybase;
    struct Library  	    	*aslbase;
    struct GfxBase  	    	*gfxbase;
    struct Library  	    	*layersbase;
    struct IntuitionBase    	*intuibase;
    struct Library  	    	*cxbase;
    struct Library  	    	*keymapbase;
    struct Library		*gadtoolsbase;
    struct Library  	    	*iffparsebase;
    struct Library  	    	*diskfontbase;
    struct Library  	    	*iconbase;
    struct Library  	    	*cybergfxbase;
#ifdef HAVE_COOLIMAGES
    struct Library  	    	*coolimagesbase;
#endif
    
/*  struct Library  	    	*datatypesbase; */
#endif /* __AROS__ */
    
    struct SignalSemaphore ZuneSemaphore; /* Used when accessing global data */

    struct MinList BuiltinClasses;
    struct MinList Applications;
};

/****************************************************************************************/

#undef MUIMB
#define MUIMB(b)	((struct MUIMasterBase_intern *)b)

#ifndef __AROS__

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

#undef IFFParseBase
#define IFFParseBase  	(MUIMB(MUIMasterBase)->iffparsebase)

#undef DiskfontBase
#define DiskfontBase  	(MUIMB(MUIMasterBase)->diskfontbase)

#undef IconBase
#define IconBase  	(MUIMB(MUIMasterBase)->iconbase)

#undef CyberGfxBase
#define CyberGfxBase  	(MUIMB(MUIMasterBase)->cybergfxbase)

#undef CoolImagesBase
#define CoolImagesBase	(MUIMB(MUIMasterBase)->coolimagesbase)

#endif /* __AROS__ */

#endif /* MUIMASTER_INTERN_H */
