/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/layers.h>

#if 0
#ifndef _AROS
#include <libraries/reqtools.h>
#include <proto/reqtools.h>
#endif
#endif /* 0 */

#ifdef COMPILE_DATATYPE
#include <proto/datatypes.h>
#endif

#include "debug.h"
#include "compilerspecific.h"
#include "support.h"
#include "pictureclass.h"

STATIC struct Gadget *DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
 return(NULL);
}

STATIC VOID DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
 return;
}

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
 return(0);
}

STATIC IPTR DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
 return(0);
}

STATIC IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
{
 return(0);
}

STATIC LONG DT_HandleInputMethod(struct IClass * cl, struct Gadget * g, struct gpInput * msg)
{
 return(0);
}

STATIC VOID DT_Write(struct IClass *cl, struct Gadget *g, struct dtWrite *msg)
{
 return;
}

STATIC VOID DT_Print(struct IClass *cl, struct Gadget *g, struct dtPrint *msg)
{
 return;
}




#ifdef _AROS
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(STACKULONG *, msg, A1))
#else
ASM ULONG DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *o, register __a1 LONG *msg)
#endif
{
 IPTR RetVal;

 putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);   /* Small Data */

 RetVal=(IPTR) 0;

 switch (*msg)
 {
  case OM_NEW:
  {
   D(bug("Dispatcher called (MethodID: OM_NEW)!\n"));
   RetVal=(IPTR) DT_NewMethod(cl, o, (struct opSet *) msg);
   break;
  }

  case OM_DISPOSE:
  {
   D(bug("Dispatcher called (MethodID: OM_DISPOSE)!\n"));
   DT_DisposeMethod(cl, o, (Msg) msg);
   break;
  }

  case OM_UPDATE:
  case OM_SET:
  {
#if 0
   D(bug("Dispatcher called (MethodID: %s)!\n", (msg==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));
#endif
   RetVal=(IPTR) DT_SetMethod(cl, (struct Gadget *) o, (struct opSet *) msg);
   break;
  }

  case OM_GET:
  {
   D(bug("Dispatcher called (MethodID: OM_GET)!\n"));
  
   break;
  }

  case GM_RENDER:
  {
   D(bug("Dispatcher called (MethodID: GM_RENDER)!\n"));

   break;
  }

  case GM_LAYOUT:
  {
   D(bug("Dispatcher called (MethodID: GM_LAYOUT)!\n"));
  
   break;
  }

  case GM_GOACTIVE:
  {
   D(bug("Dispatcher called (MethodID: GM_GOACTIVE)!\n"));

   break;
  }

  case GM_HANDLEINPUT:
  {
   D(bug("Dispatcher called (MethodID: GM_HANDLEINPUT)!\n"));
  
   break;
  }

  case DTM_CLEARSELECTED:
  {
   D(bug("Dispatcher called (MethodID: DTM_CLEARSELECTED)!\n"));

   break;
  }

  case DTM_COPY:
  {
   D(bug("Dispatcher called (MethodID: DTM_COPY)!\n"));

   break;
  }

  case DTM_SELECT:
  {
   D(bug("Dispatcher called (MethodID: DTM_SELECT)!\n"));

   break;
  }

  case DTM_WRITE:
  {
   D(bug("Dispatcher called (MethodID: DTM_WRITE)!\n"));

   break;
  }

  case DTM_PRINT:
  {
   D(bug("Dispatcher called (MethodID: DTM_PRINT)!\n"));

   break;
  }

  case GM_HITTEST:
  {
   D(bug("Dispatcher called (MethodID: GM_HITTEST)!\n"));

   break;
  }

  case DTM_PROCLAYOUT:
  {
   D(bug("Dispatcher called (MethodID: DTM_PROCLAYOUT)!\n"));
  
   break;
  }

  case DTM_FRAMEBOX:
  {
   D(bug("Dispatcher called (MethodID: DTM_FRAMEBOX)!\n"));
  
   break;
  }
 
  default:
  {
   D(bug("Dispatcher called (MethodID: %ld=0x%lx)!\n", *msg, *msg));
   RetVal=DoSuperMethodA(cl, o, (Msg) msg);
   break;
  }
 }
    
 return(RetVal);
}

struct IClass *DT_MakeClass(struct Library *picturebase)
{
    struct IClass *cl = MakeClass("picture.datatype", DATATYPESCLASS, NULL, sizeof(struct Picture_Data), NULL);

    if (cl)
    {
#ifdef _AROS
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR) picturebase;  /* Required by datatypes */
    }

    return cl;
}

