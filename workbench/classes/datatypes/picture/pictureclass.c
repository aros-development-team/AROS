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

#include <proto/datatypes.h>

#include "compilerspecific.h"
#include "debug.h"
#include "pictureclass.h"

const IPTR SupportedMethods[] =
{
 OM_NEW,
 OM_DISPOSE,
 OM_UPDATE,
 OM_SET,
 OM_GET,
 GM_RENDER,
 GM_LAYOUT,
 GM_GOACTIVE,
 GM_HANDLEINPUT,
 DTM_CLEARSELECTED,
 DTM_COPY,
 DTM_SELECT,
 DTM_WRITE,
 DTM_PRINT,
 GM_HITTEST,
 DTM_PROCLAYOUT,
 DTM_FRAMEBOX,
 (~0)
};

STATIC struct Gadget *DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
 struct Gadget *g;
 struct TagItem *attrs;
 struct TagItem *ti;
 IPTR st;
 struct Picture_Data *pd;

 g=NULL;

 attrs=msg->ops_AttrList;

 st=GetTagData(DTA_SourceType, DTST_FILE, attrs);
 
 /*
  * DTST_RAM and DTST_HOTLINK not supported (yet??)
  */
 if(!((st==DTST_CLIPBOARD) || (st==DTST_FILE)))
 {
  D(bug("picture.datatype/OM_NEW: wrong DTA_SourceType\n"));

  SetIoErr(ERROR_OBJECT_WRONG_TYPE);
  return(FALSE);
 }
 
 g=(struct Gadget *) DoSuperMethodA(cl, o, (Msg) msg);
 if(!g)
 {
  return(FALSE);
 }

 pd=(struct Picture_Data *) INST_DATA(cl, g);

 memset(pd, 0, sizeof(struct Picture_Data));

 pd->Precision=PRECISION_IMAGE;
 pd->ModeID=INVALID_ID;

 while((ti=NextTagItem(&attrs)))
 {
  switch (ti->ti_Tag)
  {
   case OBP_Precision:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: OBP_Precision\n"));

#if 0
    D(bug("picture.datatype/OM_NEW: Tag ID: \n"));
#endif

    pd->Precision=ti->ti_Data;
    break;
   }

   case PDTA_ModeID:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_ModeID\n"));

    pd->ModeID=ti->ti_Data;
    break;
   }

   case PDTA_BitMap:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_BitMap\n"));

    pd->bm=(struct BitMap *) ti->ti_Data;
    break;
   }

   case PDTA_NumColors:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_NumColors\n"));

    pd->NumColors=ti->ti_Data;
    break;
   }

   case PDTA_Remap:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_Remap\n"));

    pd->Remap=ti->ti_Data;
    break;
   }

   case PDTA_Screen:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_Screen\n"));

    pd->TheScreen=(struct Screen *) ti->ti_Data;
    break;
   }

   case PDTA_FreeSourceBitMap:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_FreeSourceBitMap\n"));

    pd->FreeSourceBitMap=ti->ti_Data;
    break;
   }

   case PDTA_Grab:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_Grab\n"));

    pd->Grab=(Point *) ti->ti_Data;
    break;
   }

   case PDTA_ClassBitMap:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_ClassBitMap\n"));

    pd->ClassBM=(struct BitMap *) ti->ti_Data;
    break;
   }

   case PDTA_NumSparse:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_NumSparse\n"));

    pd->NumSparse=ti->ti_Data;
    break;
   }

   case PDTA_SparseTable:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_SparseTable\n"));

    pd->SparseTable=(UBYTE *) ti->ti_Data;
    break;
   }
  }
 }

 return(g);
}

STATIC VOID DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
 struct Picture_Data *pd;

 pd=(struct Picture_Data *) INST_DATA(cl, o);

 if(pd)
 {
  if(pd->bmhd)
  {
   FreeVec((void *) pd->bmhd);
  }

  if(pd->bm)
  {
   FreeBitMap((void *) pd->bm);
  }

  if(pd->ColMap)
  {
   FreeVec((void *) pd->ColMap);
  }

  if(pd->ColRegs)
  {
   FreeVec((void *) pd->ColRegs);
  }

  if(pd->GRegs)
  {
   FreeVec((void *) pd->GRegs);
  }

  if(pd->ColTable)
  {
   FreeVec((void *) pd->ColTable);
  }

  if(pd->ColTable2)
  {
   FreeVec((void *) pd->ColTable2);
  }

  if(pd->Grab)
  {
   FreeVec((void *) pd->Grab);
  }

  if(pd->DestBM)
  {
   FreeBitMap(pd->DestBM);
  }

  if(pd->ClassBM)
  {
   FreeBitMap(pd->ClassBM);
  }

  if(pd->SparseTable)
  {
   FreeVec((void *) pd->SparseTable);
  }
 }

 DoSuperMethodA(cl, o, msg);

 return;
}

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
 struct Picture_Data *pd;
 struct TagItem *tl;
 struct TagItem *ti;
 IPTR RetVal;

 pd=(struct Picture_Data *) INST_DATA(cl, g);
 tl=msg->ops_AttrList;
 RetVal=1;

 while((ti=NextTagItem(&tl)))
 {
  switch (ti->ti_Tag)
  {
   case PDTA_ModeID:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_ModeID\n"));

    pd->ModeID=ti->ti_Data;

    break;
   }

   case PDTA_BitMap:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_BitMap\n"));

    pd->bm=(struct BitMap *) ti->ti_Data;

    break;
   }

   case PDTA_NumColors:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_NumColors\n"));

    pd->NumColors=ti->ti_Data;

    break;
   }

   case PDTA_Screen:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_Screen\n"));

    break;
   }

   case PDTA_FreeSourceBitMap:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_FreeSourceBitMap\n"));

    break;
   }

   case PDTA_Grab:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_Grab\n"));

    break;
   }

   case PDTA_ClassBitMap:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_ClassBitMap\n"));

    break;
   }

   default:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: 0x%lx\n", ti->ti_Tag));

   }
  }
 }

 RetVal += (IPTR) DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 return(RetVal);
}

STATIC IPTR DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
 struct Picture_Data *pd;

 pd=(struct Picture_Data *) INST_DATA(cl, g);

 switch(msg->opg_AttrID)
 {
  case PDTA_ModeID:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_ModeID\n"));


   *(msg->opg_Storage)=pd->ModeID;

   break;
  }

  case PDTA_BitMapHeader:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_BitMapHeader\n"));

   if(!pd->bmhd)
   {
    pd->bmhd=AllocVec(sizeof(struct BitMapHeader), MEMF_ANY | MEMF_CLEAR);
    if(!pd->bmhd)
    {
     return(FALSE);
    }
   }

   *(msg->opg_Storage)=(ULONG) pd->bmhd;

   break;
  }

  case PDTA_BitMap:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_BitMap\n"));

   *(msg->opg_Storage)=(ULONG) pd->bm;

   break;
  }

  case PDTA_ColorRegisters:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_ColorRegisters\n"));

   if(!pd->ColMap)
   {
    pd->ColMap=AllocVec(pd->NumColors*sizeof(struct ColorRegister), MEMF_ANY | MEMF_CLEAR);
    if(!pd->ColMap)
    {
     return(FALSE);
    }
   }

   *(msg->opg_Storage)=(ULONG) pd->ColMap;

   break;
  }

  case PDTA_CRegs:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_CRegs\n"));

   if(!pd->ColRegs)
   {
    pd->ColRegs=AllocVec(pd->NumColors*3*sizeof(ULONG), MEMF_ANY | MEMF_CLEAR);
    if(!pd->ColRegs)
    {
     return(FALSE);
    }
   }

   *(msg->opg_Storage)=(ULONG) pd->ColRegs;

   break;
  }

  case PDTA_GRegs:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_GRegs\n"));

   *(msg->opg_Storage)=(ULONG) pd->GRegs;

   break;
  }

  case PDTA_ColorTable:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_ColorTable\n"));

   *(msg->opg_Storage)=(ULONG) pd->ColTable;

   break;
  }

  case PDTA_ColorTable2:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_ColorTable2\n"));

   *(msg->opg_Storage)=(ULONG) pd->ColTable2;

   break;
  }

  case PDTA_Allocated:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_Allocated\n"));

   *(msg->opg_Storage)=(ULONG) pd->Allocated;

   break;
  }

  case PDTA_NumColors:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_NumColors\n"));

   *(msg->opg_Storage)=(ULONG) pd->NumColors;

   break;
  }

  case PDTA_NumAlloc:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_NumAlloc\n"));

   *(msg->opg_Storage)=(ULONG) pd->NumAlloc;

   break;
  }

  case PDTA_Grab:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_Grab\n"));

   *(msg->opg_Storage)=(ULONG) pd->Grab;

   break;
  }

  case PDTA_DestBitMap:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_DestBitMap\n"));

   *(msg->opg_Storage)=(ULONG) pd->DestBM;

   break;
  }

  case PDTA_ClassBitMap:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_ClassBitMap\n"));

   *(msg->opg_Storage)=(ULONG) pd->ClassBM;

   break;
  }

  case DTA_Methods:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: DTA_Methods\n"));

   *(msg->opg_Storage)=(ULONG) SupportedMethods;

   break;
  }

  default:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: 0x%lx\n", msg->opg_AttrID));
   return(DoSuperMethodA(cl, (Object *) g, (Msg) msg));
  }
 }

 return(TRUE);
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

STATIC VOID DT_FrameBox(struct IClass *cl, struct Gadget *g, struct dtFrameBox *msg)
{
 if(msg->dtf_GInfo)
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: Before: Has GadgetInfo\n"));
 }
 else
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: Before: Doesn't has GadgetInfo\n"));
 }

 DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 if(msg->dtf_GInfo)
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: After: Has GadgetInfo\n"));
 }
 else
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: After: Doesn't has GadgetInfo\n"));
 }

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
   D(bug("picture.datatype/DT_Dispatcher: Method OM_NEW\n"));
   RetVal=(IPTR) DT_NewMethod(cl, o, (struct opSet *) msg);
   break;
  }

  case OM_DISPOSE:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method OM_DISPOSE\n"));
   DT_DisposeMethod(cl, o, (Msg) msg);
   break;
  }

  case OM_UPDATE:
  case OM_SET:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method %s\n", (*msg==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));
   RetVal=(IPTR) DT_SetMethod(cl, (struct Gadget *) o, (struct opSet *) msg);
   break;
  }

  case OM_GET:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method OM_GET\n"));
   RetVal=(IPTR) DT_GetMethod(cl, (struct Gadget *) o, (struct opGet *) msg);  
   break;
  }

  case GM_RENDER:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method GM_RENDER\n"));
   RetVal=(IPTR) DT_Render(cl, (struct Gadget *) o, (struct gpRender *) msg);
   break;
  }

  case GM_LAYOUT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method GM_LAYOUT\n"));
   
   break;
  }

  case GM_GOACTIVE:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method GM_GOACTIVE\n"));

   break;
  }

  case GM_HANDLEINPUT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method GM_HANDLEINPUT\n"));
  
   break;
  }

  case DTM_CLEARSELECTED:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_CLEARSELECTED\n"));

   break;
  }

  case DTM_COPY:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_COPY\n"));

   break;
  }

  case DTM_SELECT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_SELECT\n"));

   break;
  }

  case DTM_WRITE:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_WRITE\n"));

   break;
  }

  case DTM_PRINT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_PRINT\n"));

   break;
  }

  case GM_HITTEST:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method GM_HITTEST\n"));

   break;
  }

  case DTM_PROCLAYOUT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_PROCLAYOUT\n"));
  
   break;
  }

  case DTM_FRAMEBOX:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_FRAMEBOX\n"));
   DT_FrameBox(cl, (struct Gadget *) o, (struct dtFrameBox *) msg);
   break;
  }
 
  default:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method %ld=0x%lx\n", *msg, *msg));
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

