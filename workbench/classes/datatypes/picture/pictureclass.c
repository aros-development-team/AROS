/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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
#include "colorhandling.h"

#include "methods.h"

const IPTR SupportedMethods[] =
{
 OM_NEW,
 OM_GET,
 OM_SET,
 OM_UPDATE,
 OM_DISPOSE,

 GM_LAYOUT,
 GM_HITTEST,
 GM_GOACTIVE,
 GM_HANDLEINPUT,
 GM_RENDER,

 DTM_PROCLAYOUT,
 DTM_ASYNCLAYOUT,
 DTM_FRAMEBOX,
 DTM_SELECT,
 DTM_CLEARSELECTED,
 DTM_COPY,
 DTM_PRINT,
 DTM_WRITE,
 
 (~0)
};

STATIC ULONG notifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
 return(DoMethod(o, OM_NOTIFY, &tag1, ginfo, flags));
}

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

    /*
     *  Currently we don't allow Sparse to be changeable by user
     *  because we use it for our own purposes.
     *
     *  So if you are a application developer: NEVER USE IT!
     */

#if 0
    pd->NumSparse=ti->ti_Data;
#endif /* 0 */
    break;
   }

   case PDTA_SparseTable:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_SparseTable\n"));

    /*
     *  See above!
     */

#if 0
    pd->SparseTable=(UBYTE *) ti->ti_Data;
#endif /* 0 */
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

#ifdef MYDEBUG

    register int i;
    int Known;

    Known=FALSE;

    for(i=0; i<NumAttribs; i++)
    {
     if(ti->ti_Tag==KnownAttribs[i])
     {
      Known=TRUE;

      D(bug("picture.datatype/OM_SET: Tag ID: %s\n", AttribNames[i]));
     }
    }

    if(!Known)
    {
     D(bug("picture.datatype/OM_SET: Tag ID 0x%lx\n", ti->ti_Tag));
    }

    if(ti->ti_Tag==GA_RelWidth)
    {
     D(bug("picture.datatype/OM_SET: GA_RelWidth=%ld\n", (long) ti->ti_Data));
    }

#endif /* MYDEBUG */

#if 0
    D(bug("picture.datatype/OM_SET: Tag ID: 0x%lx\n", ti->ti_Tag));
#endif /* 0 */
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

#ifdef MYDEBUG

    register int i;
    int Known;

    Known=FALSE;

    for(i=0; i<NumAttribs; i++)
    {
     if(msg->opg_AttrID==KnownAttribs[i])
     {
      Known=TRUE;

      D(bug("picture.datatype/OM_GET: Tag ID: %s\n", AttribNames[i]));
     }
    }

    if(!Known)
    {
     D(bug("picture.datatype/OM_GET: Tag ID 0x%lx\n", msg->opg_AttrID));
    }

#endif /* MYDEBUG */

#if 0

   D(bug("picture.datatype/OM_GET: Tag ID: 0x%lx\n", msg->opg_AttrID));

#endif /* 0 */

   return(DoSuperMethodA(cl, (Object *) g, (Msg) msg));
  }
 }

 return(TRUE);
}

STATIC IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
{
 struct Picture_Data *pd;
 struct IBox *domain;
 IPTR TopVert, VisibleVert, TotalVert;
 IPTR TopHoriz, VisibleHoriz, TotalHoriz;

 pd=(struct Picture_Data *) INST_DATA(cl, g);

 if(!(GetDTAttrs((Object *) g, DTA_Domain, &domain,
			       DTA_TopVert, &TopVert,
			       DTA_VisibleVert, &VisibleVert,
			       DTA_TotalVert, &TotalVert,
			       DTA_TopHoriz, &TopHoriz,
			       DTA_VisibleHoriz, &VisibleHoriz,
			       DTA_TotalHoriz, &TotalHoriz,
			       TAG_DONE) == 7))
 {
  D(bug("picture.datatype/GM_RENDER: Couldn't get dimensions\n"));
  return(FALSE);
 }

 D(bug("picture.datatype/GM_RENDER: Domain: %ld %ld %ld %ld\n", domain->Left, domain->Top, domain->Width, domain->Height));
 D(bug("picture.datatype/GM_RENDER: TopVert      : %lu\n", (unsigned long) TopVert));
 D(bug("picture.datatype/GM_RENDER: VisibleVert  : %lu\n", (unsigned long) VisibleVert));
 D(bug("picture.datatype/GM_RENDER: TotalVert    : %lu\n", (unsigned long) TotalVert));
 D(bug("picture.datatype/GM_RENDER: TopHoriz     : %lu\n", (unsigned long) TopHoriz));
 D(bug("picture.datatype/GM_RENDER: VisibleHoriz : %lu\n", (unsigned long) VisibleHoriz));
 D(bug("picture.datatype/GM_RENDER: TotalHoriz   : %lu\n", (unsigned long) TotalHoriz));

#if 0
 BltBitMapRastPort(pd->DestBM, Left, Top, msg->gpr_RPort, Left, Top, RelWidth, RelHeight, 0xC0);
#endif

 return(TRUE);
}

STATIC IPTR DT_Layout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;

 notifyAttrChanges((Object *) g, ((struct gpLayout *) msg)->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,
				 DTA_Busy, TRUE,
				 TAG_DONE);

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 D(bug("picture.datatype/GM_LAYOUT: RetVal: 0x%lx\n", RetVal));





 notifyAttrChanges((Object *) g, ((struct gpLayout *) msg)->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,
				 DTA_Busy, FALSE,
				 TAG_DONE);

 return(RetVal);
}

STATIC IPTR DT_Layout_old(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 struct Picture_Data *pd;
 unsigned int DestNumColors;
 register int i, j;
 ULONG *ColRegsPtr;
 struct RastPort SrcRP, DestRP;
 unsigned int BM_Width, BM_Height, BM_Depth;
 struct HistEntry Hist[256];

 pd=(struct Picture_Data *) INST_DATA(cl, g);

 if(!(pd->bm && pd->ColRegs))
 {
  D(bug("picture.datatype/GM_LAYOUT: No bitmap and colortable provided, returning failure\n"));

  return(FALSE);
 }

 if(!pd->TheScreen)
 {
  D(bug("picture.datatype/GM_LAYOUT: No screen set\n"));

  pd->TheScreen=msg->gpl_GInfo->gi_Screen;
 }

 if(pd->DestBM)
 {
  D(bug("picture.datatype/GM_LAYOUT: DestBitMap already exists\n"));

  FreeBitMap(pd->DestBM);
  pd->DestBM=NULL;
 }

 BM_Width=GetBitMapAttr(pd->bm, BMA_WIDTH);
 BM_Height=GetBitMapAttr(pd->bm, BMA_HEIGHT);
 BM_Depth=GetBitMapAttr(pd->TheScreen->RastPort.BitMap, BMA_DEPTH);

 /*
  *  Here we must take attention to PDTA_Remap
  *  If it is FALSE we should not remap at all.
  */

 pd->DestBM=AllocBitMap(BM_Width, BM_Height, BM_Depth,
			(BMF_CLEAR | BMF_DISPLAYABLE | BMF_INTERLEAVED | BMF_MINPLANES),
			pd->TheScreen->RastPort.BitMap);

 if(!pd->DestBM)
 {
  D(bug("picture.datatype/GM_LAYOUT: Unable to allocate DestBitMap\n"));

  return(FALSE);
 }

 DestNumColors=1<<BM_Depth;

 if(pd->SparseTable)
 {
  D(bug("picture.datatype/GM_LAYOUT: SparseTable already exists\n"));

  FreeVec((void *) pd->SparseTable);

  pd->SparseTable=NULL;
  pd->NumSparse=0;
 }

 pd->NumSparse=pd->NumColors;

 pd->SparseTable = AllocVec(pd->NumSparse, MEMF_ANY | MEMF_CLEAR);
 if(!pd->SparseTable)
 {
  D(bug("picture.datatype/GM_LAYOUT: Unable to allocate SparseTable\n"));

  return(FALSE);
 }

 if(pd->NumAlloc)
 {
  D(bug("picture.datatype/GM_LAYOUT: Some colors allready allocated\n"));

  /*
   *  We should not have allocated colors here.
   */

  pd->NumAlloc=0;
 }

 if(pd->GRegs)
 {
  D(bug("picture.datatype/GM_LAYOUT: GRegs already exists\n"));

  FreeVec((void *) pd->GRegs);

  pd->GRegs=NULL;
 }

 pd->GRegs = AllocVec(DestNumColors*3*sizeof(ULONG), MEMF_ANY | MEMF_CLEAR);
 if(!pd->GRegs)
 {
  D(bug("picture.datatype/GM_LAYOUT: Unable to allocate GRegs\n"));

  return(FALSE);
 }

 memset(pd->GRegs, 0, (DestNumColors*3*sizeof(ULONG)));

 if(pd->NumColors<=DestNumColors)
 {
  ColRegsPtr=pd->ColRegs;

  /*
   *  OK, I know this is not very elegant.
   *  So come with something better;
   */
  for(i=0; i<pd->NumColors; i++)
  {
   pd->SparseTable[i]=(UBYTE) ObtainBestPen(pd->TheScreen->ViewPort.ColorMap,
					    *(ColRegsPtr++), *(ColRegsPtr++), *(ColRegsPtr++),
					    TAG_DONE);

   pd->NumAlloc++;
  }

  ColRegsPtr=pd->GRegs;

  for(i=0; i<pd->NumColors; i++)
  {
   GetRGB32(pd->TheScreen->ViewPort.ColorMap, pd->SparseTable[i], 1, ColRegsPtr);

   ColRegsPtr+=3;
  }
 }
 else
 {
  

  ColRegsPtr=pd->ColRegs;

  for(i=0; i<pd->NumColors; i++)
  {
   Hist[i].Count=0;
   Hist[i].Red=*(ColRegsPtr++);
   Hist[i].Green=*(ColRegsPtr++);
   Hist[i].Blue=*(ColRegsPtr++);
  }

  for(i=0; i<BM_Height; i++)
  {
   for(j=0; j<BM_Width; j++)
   {
    Hist[ReadPixel(&SrcRP, j, i)].Count++;
   }
  }

  for(i=0; i<(pd->NumColors-1); i++)
  {
   for(j=i+1; j<pd->NumColors; j++)
   {
    if((Hist[j].Red == Hist[i].Red) &&
       (Hist[j].Green == Hist[i].Green) &&
       (Hist[j].Blue == Hist[i].Blue))
    {
     Hist[i].Count+=Hist[j].Count;
     Hist[j].Count=0;
    }
   }
  }

  qsort((void *) Hist, pd->NumColors, sizeof(struct HistEntry), HistSort);

  for(i=0; i<DestNumColors; i++)
  {


  }


 }

 InitRastPort(&SrcRP);
 SrcRP.BitMap=pd->bm;

 InitRastPort(&DestRP);
 DestRP.BitMap=pd->DestBM;

 for(i=0; i<GetBitMapAttr(pd->bm, BMA_HEIGHT); i++)
 {
  for(j=0; j<GetBitMapAttr(pd->bm, BMA_WIDTH); j++)
  {
   SetAPen(&DestRP, pd->SparseTable[ReadPixel(&SrcRP, j, i)]);
   WritePixel(&DestRP, j, i);
  }
 }

#ifdef AROS
 DeinitRastPort(&SrcRP);
 DeinitRastPort(&DestRP);
#endif
 return(TRUE);
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
#if 0
 if(msg->dtf_GInfo)
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: Before: Has GadgetInfo\n"));
 }
 else
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: Before: Doesn't has GadgetInfo\n"));
 }
#endif /* 0 */

 DoSuperMethodA(cl, (Object *) g, (Msg) msg);

#if 0
 if(msg->dtf_GInfo)
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: After: Has GadgetInfo\n"));
 }
 else
 {
  D(bug("picture.datatype/DTM_FRAMEBOX: After: Doesn't has GadgetInfo\n"));
 }
#endif /* 0 */

 return;
}

#ifdef _AROS
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
#else
ASM ULONG DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *o, register __a1 Msg msg)
{
#endif
 IPTR RetVal;
#ifdef MYDEBUG
 register int i;
 int Known;

 Known=FALSE;
#endif /* MYDEBUG */

 putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);   /* Small Data */

 RetVal=(IPTR) 0;

 switch(msg->MethodID)
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
   D(bug("picture.datatype/DT_Dispatcher: Method %s\n", (msg->MethodID==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));
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
   RetVal=(IPTR) DT_Layout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
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

  case DTM_ASYNCLAYOUT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_ASYNCLAYOUT\n"));

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
#ifdef MYDEBUG
   for(i=0; i<NumMethods; i++)
   {
    if(msg->MethodID==KnownMethods[i])
    {
     Known=TRUE;

     D(bug("picture.datatype/DT_Dispatcher: Method %s\n", MethodNames[i]));
    }
   }

   if(!Known)
   {
    D(bug("picture.datatype/DT_Dispatcher: Method 0x%lx\n", (unsigned long) msg->MethodID));
   }
#endif /* MYDEBUG */

#if 0
   D(bug("picture.datatype/DT_Dispatcher: Method %ld=0x%lx\n", *msg, *msg));
#endif /* 0 */
   RetVal=DoSuperMethodA(cl, o, (Msg) msg);
   break;
  }
 }
    
 return(RetVal);

#ifdef _AROS
    AROS_USERFUNC_EXIT
#endif
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

