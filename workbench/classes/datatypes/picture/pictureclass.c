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

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg);

STATIC ULONG NotifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
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
  *  DTST_RAM and DTST_HOTLINK not supported (yet??)
  */
 if(!((st==DTST_CLIPBOARD) || (st==DTST_FILE)))
 {
  D(bug("picture.datatype/OM_NEW: wrong DTA_SourceType\n"));

  SetIoErr(ERROR_OBJECT_WRONG_TYPE);
  return(NULL);
 }
 
 g=(struct Gadget *) DoSuperMethodA(cl, o, (Msg) msg);
 if(!g)
 {
  return(NULL);
 }

 pd=(struct Picture_Data *) INST_DATA(cl, g);

 memset(pd, 0, sizeof(struct Picture_Data));

 pd->Precision=PRECISION_IMAGE;
 pd->ModeID=INVALID_ID;
 pd->Remap=TRUE;

 while((ti=NextTagItem(&attrs)))
 {
  switch (ti->ti_Tag)
  {
   case OBP_Precision:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: OBP_Precision\n"));

    pd->Precision=ti->ti_Data;
    break;
   }

   case PDTA_Remap:
   {
    D(bug("picture.datatype/OM_NEW: Tag ID: PDTA_Remap\n"));

    pd->Remap=ti->ti_Data;
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

    if(!(pd->NumSparse && ti->ti_Data))
    {
     break;
    }

    CopyMem((APTR) ti->ti_Data, (APTR) pd->SparseTable, pd->NumSparse);

    break;
   }
  }
 }

 DT_SetMethod(cl, g, msg);

 return(g);
}

STATIC IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
 struct Picture_Data *pd;
 IPTR RetVal;
 register int i;

 RetVal=1;

 pd=(struct Picture_Data *) INST_DATA(cl, o);

 if(pd)
 {
  if(pd->ChunkyBuffer)
  {
   FreeVec((void *) pd->ChunkyBuffer);
  }

  if(pd->NumAlloc)
  {
   for(i=0; i<pd->NumAlloc; i++)
   {
    ReleasePen(pd->TheScreen->ViewPort.ColorMap, pd->ColTable[i]);
   }

   pd->NumAlloc=0;
  }

  if(pd->DestBM && (pd->DestBM != pd->bm))
  {
   FreeBitMap(pd->DestBM);
  }

  if(pd->bm)
  {
   FreeBitMap((void *) pd->bm);
  }
 }

 RetVal+=DoSuperMethodA(cl, o, msg);

 return(RetVal);
}

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
 struct Picture_Data *pd;
 struct TagItem *tl;
 struct TagItem *ti;
 IPTR RetVal;
 struct RastPort *rp;

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

    pd->TheScreen=(struct Screen *) ti->ti_Data;

    break;
   }

   case PDTA_FreeSourceBitMap:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_FreeSourceBitMap\n"));

    pd->FreeSourceBitMap=(BOOL) ti->ti_Data;

    break;
   }

   case PDTA_Grab:
   {
    Point *ThePoint;

    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_Grab\n"));

    ThePoint=(Point *) ti->ti_Data;
    if(!ThePoint)
    {
     break;
    }

    pd->Grab.x=ThePoint->x;
    pd->Grab.y=ThePoint->y;

    break;
   }

   case PDTA_ClassBitMap:
   {
    D(bug("picture.datatype/OM_SET: Tag ID: PDTA_ClassBitMap\n"));

    pd->ClassBM=(struct BitMap *) ti->ti_Data;

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

#endif /* MYDEBUG */
   }
  }
 }

#if 0
 if(msg->ops_GInfo)
 {
  DoMethod((Object *) g, GM_LAYOUT, msg->ops_GInfo, TRUE);
 }
#endif

 /*
  *  Do not call the SuperMethod if you come from OM_NEW!
  */

 if(!(msg->MethodID == OM_NEW))
 {
  RetVal += (IPTR) DoSuperMethodA(cl, (Object *) g, (Msg) msg);
 }

 if(msg->ops_GInfo)
 {
  if(OCLASS((Object *) g) == cl)
  {
   rp=ObtainGIRPort(msg->ops_GInfo);
   if(rp)
   {
    DoMethod((Object *) g, GM_RENDER, msg->ops_GInfo, rp, GREDRAW_UPDATE);

    ReleaseGIRPort (rp);
   }
  }

  if(msg->MethodID == OM_UPDATE)
  {
    DoMethod((Object *) g, OM_NOTIFY, msg->ops_AttrList, msg->ops_GInfo, 0);
  }
 }

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

   *(msg->opg_Storage)=(ULONG) &pd->bmhd;

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

   *(msg->opg_Storage)=(ULONG) &pd->ColMap;

   break;
  }

  case PDTA_CRegs:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_CRegs\n"));

   *(msg->opg_Storage)=(ULONG) &pd->CRegs;

   break;
  }

  case PDTA_GRegs:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_GRegs\n"));

   *(msg->opg_Storage)=(ULONG) &pd->GRegs;

   break;
  }

  case PDTA_ColorTable:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_ColorTable\n"));

   *(msg->opg_Storage)=(ULONG) &pd->ColTable;

   break;
  }

  case PDTA_ColorTable2:
  {
   D(bug("picture.datatype/OM_GET: Tag ID: PDTA_ColorTable2\n"));

   *(msg->opg_Storage)=(ULONG) &pd->ColTable2;

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

   *(msg->opg_Storage)=(ULONG) &pd->Grab;

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

   return(DoSuperMethodA(cl, (Object *) g, (Msg) msg));
  }
 }

 return(TRUE);
}

STATIC IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
{
 struct Picture_Data *pd;
 struct DTSpecialInfo *si;

 struct IBox *domain;
 IPTR TopVert, TopHoriz, NominalWidth, NominalHeight;

 WORD SrcX, SrcY, DestX, DestY, SizeX, SizeY;

 pd=(struct Picture_Data *) INST_DATA(cl, g);
 si=(struct DTSpecialInfo *) g->SpecialInfo;

 if(si->si_Flags & DTSIF_LAYOUT)
 {
  D(bug("picture.datatype/GM_RENDER: In layout process\n"));
  return(0);
 }

 if(!pd->DestBM)
 {
  D(bug("picture.datatype/GM_RENDER: No DestBM set!\n"));
  return(0);
 }

 if(!(GetDTAttrs((Object *) g, DTA_Domain, &domain,
			       DTA_TopVert, &TopVert,
			       DTA_TopHoriz, &TopHoriz,
			       DTA_NominalHoriz, &NominalWidth,
			       DTA_NominalVert, &NominalHeight,
			       TAG_DONE) == 5))
 {
  D(bug("picture.datatype/GM_RENDER: Couldn't get dimensions\n"));
  return(0);
 }

#if 0
 D(bug("picture.datatype/GM_RENDER: Domain: %ld %ld %ld %ld\n", domain->Left, domain->Top, domain->Width, domain->Height));
 D(bug("picture.datatype/GM_RENDER: TopVert      : %lu\n", (unsigned long) TopVert));
 D(bug("picture.datatype/GM_RENDER: TopHoriz     : %lu\n", (unsigned long) TopHoriz));
 D(bug("picture.datatype/GM_RENDER: Width        : %lu\n", (unsigned long) NominalWidth));
 D(bug("picture.datatype/GM_RENDER: Height       : %lu\n", (unsigned long) NominalHeight));
#endif

#if 0
 ObtainSemaphore(&(si->si_Lock));
#endif

 SrcX=TopHoriz;
 SrcY=TopVert;
 DestX=domain->Left;
 DestY=domain->Top;

 SizeX=((NominalWidth - TopHoriz) < domain->Width) ? (NominalWidth - TopHoriz) : domain->Width;
 SizeY=((NominalHeight - TopVert) < domain->Height) ? (NominalHeight - TopVert) : domain->Height;

 BltBitMapRastPort(pd->DestBM, SrcX, SrcY, msg->gpr_RPort, DestX, DestY, SizeX, SizeY, 0xC0);

#if 0
 ReleaseSemaphore(&(si->si_Lock));
#endif

 return(0);
}

STATIC IPTR DT_Layout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;

 NotifyAttrChanges((Object *) g, msg->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,
				 DTA_Busy, TRUE,
				 TAG_DONE);

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 RetVal += (IPTR) DoAsyncLayout((Object *) g, msg);

 return(RetVal);
}

STATIC IPTR DT_ProcLayout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;

 NotifyAttrChanges((Object *) g, msg->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,
				 DTA_Busy, TRUE,
				 TAG_DONE);

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 return(RetVal);
}

STATIC IPTR DT_AsyncLayout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;
 struct Picture_Data *pd;
 struct DTSpecialInfo *si;

 struct IBox *domain;
 IPTR Width, Height;

 STRPTR Title;

 RetVal=1;
 pd=(struct Picture_Data *) INST_DATA(cl, g);
 si=(struct DTSpecialInfo *) g->SpecialInfo;

 /*
  *  BitMap noch nicht gesetzt!
  */
 if(!(pd->bm))
 {
  return(0);
 }

 /*
  *  Attribute holen
  */
 if(!(GetDTAttrs((Object *) g, DTA_Domain, (IPTR) &domain,
			       DTA_ObjName, (IPTR) &Title,
			       DTA_NominalHoriz, &Width,
			       DTA_NominalVert, &Height,
			       TAG_DONE) == 4))
 {
  return(0);
 }

 /*
  *  Sperren
  */
 ObtainSemaphore(&(si->si_Lock));

 /*
  *  Wir brauchen nur einmal taetig zu werden.
  */
 if(msg->gpl_Initial)
 {
  /*
   *  Wir muessen das Bild remappen
   */
  if(pd->Remap)
  {
   unsigned int BM_Width, BM_Height;
   unsigned int DestDepth;
   unsigned int DestNumColors;
   struct RastPort SrcRP, DestRP;

   register int i, j;

   /*
    *  Groessen bestimmen
    */
   if(!pd->TheScreen)
   {
    pd->TheScreen=msg->gpl_GInfo->gi_Screen;
   }

   BM_Width=pd->bmhd.bmh_Width;
   BM_Height=pd->bmhd.bmh_Height;

   DestDepth=GetBitMapAttr(pd->TheScreen->RastPort.BitMap, BMA_DEPTH);

   if(DestDepth > 8)
   {
    DestDepth=8;
    DestNumColors=256;
   }
   else
   {
    DestNumColors=1<<DestDepth;
   }

   pd->NumSparse=pd->NumColors;

   /*
    *  Aufraeumen was vielleicht uebriggeblieben ist
    */
   if(pd->DestBM && (pd->DestBM != pd->bm))
   {
    FreeBitMap(pd->DestBM);
    pd->DestBM=NULL;
   }

   if(pd->NumAlloc)
   {
    for(i=0; i<pd->NumAlloc; i++)
    {
     ReleasePen(pd->TheScreen->ViewPort.ColorMap, pd->ColTable[i]);
    }

    pd->NumAlloc=0;
   }

   if(pd->ChunkyBuffer)
   {
    FreeVec((void *) pd->ChunkyBuffer);
   }

   /*
    *  pd->GRegs vorbelegen
    */
   memset(pd->GRegs, 0xFF, 768*sizeof(ULONG));

   /*
    *  SparseTable ausfuellen
    */
   memset(pd->SparseTable, 0x0, 256);

   /*
    *  Neu allozieren
    */
   pd->DestBM=AllocBitMap(BM_Width, BM_Height, DestDepth,
			  (BMF_CLEAR | BMF_DISPLAYABLE | BMF_INTERLEAVED | BMF_MINPLANES),
			  pd->TheScreen->RastPort.BitMap);
   if(!pd->DestBM)
   {
    ReleaseSemaphore(&si->si_Lock);

    return(0);
   }

   pd->ChunkyBuffer=AllocVec(BM_Width*BM_Height, MEMF_ANY | MEMF_CLEAR);
   if(!pd->ChunkyBuffer)
   {
    ReleaseSemaphore(&si->si_Lock);

    return(0);
   }

   InitRastPort(&SrcRP);
   SrcRP.BitMap=pd->bm;

   /*
    *  planare BitMap in einen chunky Buffer auslesen
    */
   for(i=0; i<BM_Height; i++)
   {
    for(j=0; j<BM_Width; j++)
    {
     pd->ChunkyBuffer[i*BM_Width+j]=ReadPixel(&SrcRP, j, i);
    }
   }

   /*
    *  ColorMap bestimmen
    */
   if(DestNumColors >= pd->NumColors)
   {
    CopyMem(pd->CRegs, pd->GRegs, pd->NumColors*3*sizeof(ULONG));
   }
   else
   {
    struct HistEntry TheHist[256];

    /*
     *  Farben im Histogramm ausfuellen
     */
    for(i=0; i<pd->NumColors; i++)
    {
     TheHist[i].Count=0;
     TheHist[i].Red   = pd->CRegs[i*3+0];
     TheHist[i].Green = pd->CRegs[i*3+1];
     TheHist[i].Blue  = pd->CRegs[i*3+2];
    }

    /*
     *  Farbanzahl im Histogramm ermitteln
     */
    for(i=0; i<(BM_Width*BM_Height); i++)
    {
     TheHist[pd->ChunkyBuffer[i]].Count++;
    }

    /*
     *  Duplikate im Histogramm ausmerzen
     */
    for(i=0; i<(pd->NumColors-1); i++)
    {
     for(j=i+1; j<pd->NumColors; j++)
     {
      if((TheHist[j].Red == TheHist[i].Red) &&
	 (TheHist[j].Green == TheHist[i].Green) &&
	 (TheHist[j].Blue == TheHist[i].Blue))
      {
       TheHist[i].Count+=TheHist[j].Count;
       TheHist[j].Count=0;
      }
     }
    }

    /*
     *  Histogramm nach Haeufigkeit sortieren
     */
    qsort((void *) TheHist, pd->NumColors, sizeof(struct HistEntry), HistSort);

    /*
     *  Es werden die DestNumColors meistvorhandenen Farben benutzt
     */
    for(i=0; i<DestNumColors; i++)
    {
     pd->GRegs[i*3+0] = TheHist[i].Red;
     pd->GRegs[i*3+1] = TheHist[i].Green;
     pd->GRegs[i*3+2] = TheHist[i].Blue;
    }
   }

   /*
    *  Pens fuer GRegs obtainen
    */
   for(i=0; i<DestNumColors; i++)
   {
    pd->ColTable[i]=ObtainBestPen(pd->TheScreen->ViewPort.ColorMap,
				  pd->GRegs[i*3+0], pd->GRegs[i*3+1], pd->GRegs[i*3+2],
				  OBP_Precision, pd->Precision,
				  OBP_FailIfBad, FALSE,
				  TAG_DONE);

    pd->NumAlloc++;
   }

   /*
    *  Die wirklichen Farben der Pens holen
    */
   for(i=0; i<DestNumColors; i++)
   {
    GetRGB32(pd->TheScreen->ViewPort.ColorMap, pd->ColTable[i], 1, pd->GRegs+(3*i));
   }

   /*
    *  SparseTable nach der "Geringster Abstand" Methode bestimmen
    */
   for(i=0; i<pd->NumColors; i++)
   {
    unsigned int Diff, LastDiff;
    short CRed, GRed, CGreen, GGreen, CBlue, GBlue;

    LastDiff=0xFFFFFFFF;

    CRed   = pd->CRegs[i*3+0]>>17;
    CGreen = pd->CRegs[i*3+1]>>17;
    CBlue  = pd->CRegs[i*3+2]>>17;

    for(j=0; j<DestNumColors; j++)
    {
     GRed   = pd->GRegs[j*3+0]>>17;
     GGreen = pd->GRegs[j*3+1]>>17;
     GBlue  = pd->GRegs[j*3+2]>>17;

     Diff=abs(CRed - GRed) +
	  abs(CGreen - GGreen) +
	  abs(CBlue - GBlue);

     if(Diff <= LastDiff)
     {
      pd->SparseTable[i]=pd->ColTable[j];

      LastDiff=Diff;
     }

     if(LastDiff==0)
     {
      break;
     }
    }
   }

   /*
    *  ChunkyBuffer remappen
    */
   for(i=0; i<(BM_Width*BM_Height); i++)
   {
    pd->ChunkyBuffer[i]=pd->SparseTable[pd->ChunkyBuffer[i]];
   }

   /*
    *  C2P vom ChunkyBuffer auf DestBM
    */
   InitRastPort(&DestRP);
   DestRP.BitMap=pd->DestBM;

   WriteChunkyPixels(&DestRP, 0, 0, BM_Width-1, BM_Height-1, pd->ChunkyBuffer, BM_Width);

#ifdef AROS
   DeinitRastPort(&SrcRP);
   DeinitRastPort(&DestRP);
#endif
  }
  else
  {
   /*
    *  Bild soll nicht remapped werden
    */

   pd->DestBM=pd->bm;
  }
 }

 /*
  *  Wieder entsperren
  */
 ReleaseSemaphore(&si->si_Lock);

 NotifyAttrChanges((Object *) g, msg->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,

				 DTA_VisibleVert, domain->Height,
				 DTA_TotalVert, Height,
				 DTA_NominalVert, Height,
				 DTA_VertUnit, 1,

				 DTA_VisibleHoriz, domain->Width,
				 DTA_TotalHoriz, Width,
				 DTA_NominalHoriz, Width,
				 DTA_HorizUnit, 1,

				 DTA_Title, (IPTR) Title,
				 DTA_Busy, TRUE,
				 DTA_Sync, TRUE,
				 TAG_DONE);

 return(RetVal);
}

#if 0
STATIC IPTR DT_FrameBox(struct IClass *cl, struct Gadget *g, struct dtFrameBox *msg)
{
 struct Picture_Data *pd;
 ULONG Width, Height, Depth;
 IPTR RetVal;

 pd=(struct Picture_Data *) INST_DATA(cl, g);

 RetVal=0;

 Width=pd->bmhd.bmh_Width;
 Height=pd->bmhd.bmh_Height;
 Depth=pd->bmhd.bmh_Depth;

 D(bug("picture.datatype/DTM_FRAMEBOX: Width %ld\n", (long) Width));
 D(bug("picture.datatype/DTM_FRAMEBOX: Height %ld\n", (long) Height));
 D(bug("picture.datatype/DTM_FRAMEBOX: Depth %ld\n", (long) Depth));

 if(msg->dtf_FrameInfo)
 {
  msg->dtf_FrameInfo->fri_Dimensions.Height = Height;
  msg->dtf_FrameInfo->fri_Dimensions.Width  = Width;
  msg->dtf_FrameInfo->fri_Dimensions.Depth  = Depth;
  msg->dtf_FrameInfo->fri_Flags             = FIF_SCROLLABLE;

  RetVal = 1;
 }

 return(RetVal);
}
#endif

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

  case OM_GET:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method OM_GET\n"));
   RetVal=(IPTR) DT_GetMethod(cl, (struct Gadget *) o, (struct opGet *) msg);
   break;
  }

  case OM_SET:
  case OM_UPDATE:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method %s\n", (msg->MethodID==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));
   RetVal=(IPTR) DT_SetMethod(cl, (struct Gadget *) o, (struct opSet *) msg);
   break;
  }

  case OM_DISPOSE:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method OM_DISPOSE\n"));
   RetVal=(IPTR) DT_DisposeMethod(cl, o, (Msg) msg);
   break;
  }

  case GM_LAYOUT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method GM_LAYOUT\n"));
   RetVal=(IPTR) DT_Layout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
   break;
  }

  case DTM_PROCLAYOUT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_PROCLAYOUT\n"));
   RetVal=(IPTR) DT_ProcLayout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
   /*
    *  Yes, here is no break!
    */
  }

  case DTM_ASYNCLAYOUT:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_ASYNCLAYOUT\n"));
   RetVal=(IPTR) DT_AsyncLayout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
   break;
  }

  case GM_RENDER:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method GM_RENDER\n"));
   RetVal=(IPTR) DT_Render(cl, (struct Gadget *) o, (struct gpRender *) msg);
   break;
  }

#if 0
  case DTM_FRAMEBOX:
  {
   D(bug("picture.datatype/DT_Dispatcher: Method DTM_FRAMEBOX\n"));
   RetVal=(IPTR) DT_FrameBox(cl, (struct Gadget *) o, (struct dtFrameBox *) msg);
   break;
  }
#endif

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

