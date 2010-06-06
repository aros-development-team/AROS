/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_INTERN_H
#define AROSMESA_INTERN_H

#include <exec/memory.h>
#include <exec/libraries.h>
#include <aros/asmcall.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <cybergraphx/cybergraphics.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <graphics/view.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>

#include <stddef.h>

#include <string.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

/****************************************************************************************/

/* Constants */

struct MesaBase
{
    struct Library              mglb_Lib;
    void                        *mglb_CurrentContext;
    void                        *mglb_Dispatch;
};

#endif /* AROSMESA_INTERN_H */
