/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/**********************************************************************/

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
#include <proto/datatypes.h>

#include "compilerspecific.h"
#include "debug.h"

#include "methods.h"

/**************************************************************************************************/

static IPTR PPM_New(Class *cl, Object *o, struct opSet *msg)
{
 IPTR RetVal;
 struct TagItem *Attrs;
 char *Title;
 BPTR FileHandle;
 struct BitMapHeader *bmhd;
 char LineBuffer[128];
 long Width, Height, NumChars;
 unsigned char *RGBBuffer;
 unsigned char *ChunkyBuffer;
 unsigned long ModeID;
 short nColors;
 struct ColorRegister *ColMap;
 long *ColRegs;
 unsigned int i, j, k, Counter;
 unsigned int Col7, Col3;
 struct Screen *scr;
 struct BitMap *bm;
 struct RastPort rp;

#ifdef MYDEBUG
 struct TagItem *ti;
 int Known;

 Known=FALSE;
#endif /* MYDEBUG */

 D(bug("ppm.datatype/OM_NEW: Entering\n"));

 D(bug("ppm.datatype/OM_NEW: cl: 0x%lx o: 0x%lx msg: 0x%lx\n", (unsigned long) cl, (unsigned long) o, (unsigned long) msg));

 RetVal=DoSuperMethodA(cl, o, (Msg) msg);
 if(!RetVal)
 {
  D(bug("ppm.datatype/OM_NEW: DoSuperMethod failed\n"));
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: DoSuperMethod: 0x%lx\n", (unsigned long) RetVal));

 Attrs=((struct opSet *) msg)->ops_AttrList;

 D(bug("ppm.datatype/OM_NEW: Attrs: 0x%lx\n", (unsigned long) Attrs));

#ifdef MYDEBUG
 while((ti=NextTagItem(&Attrs)))
 {
  for(i=0; i<NumAttribs; i++)
  {
   if(ti->ti_Tag==KnownAttribs[i])
   {
    Known=TRUE;

    D(bug("ppm.datatype/OM_NEW: Tag ID: %s\n", AttribNames[i]));
   }
  }

  if(!Known)
  {
   D(bug("ppm.datatype/OM_NEW: Tag ID 0x%lx\n", ti->ti_Tag));
  }
 }

 Attrs=((struct opSet *) msg)->ops_AttrList;
#endif /* MYDEBUG */

 Title=(char *) GetTagData(DTA_Name, NULL, Attrs);

 D(bug("ppm.datatype/OM_NEW: Title: %s\n", Title?Title:"none"));

 if(!(GetDTAttrs((Object *) RetVal, DTA_Handle, &FileHandle, PDTA_BitMapHeader, &bmhd, TAG_DONE)==2))
 {
  D(bug("ppm.datatype/OM_NEW: GetDTAttrs(DTA_Handle, PDTA_BitMapHeader) failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 if(!(FileHandle && bmhd))
 {
  D(bug("ppm.datatype/OM_NEW: FileHandle and bmhd failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: Now Seek'ing\n"));

 Seek(FileHandle, 0, OFFSET_BEGINNING);

 D(bug("ppm.datatype/OM_NEW: Seek successfull\n"));

 if(!FGets(FileHandle, LineBuffer, 128))
 {
  D(bug("ppm.datatype/OM_NEW: FGets line 1 failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 if(!(LineBuffer[0]=='P' && LineBuffer[1]=='6'))
 {
  D(bug("ppm.datatype/OM_NEW: Not a P6 PPM\n"));

  SetIoErr(ERROR_OBJECT_WRONG_TYPE);
  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: It's a P6 PPM\n"));

 if(!FGets(FileHandle, LineBuffer, 128))
 {
  D(bug("ppm.datatype/OM_NEW: FGets line 2 failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 if(LineBuffer[0]=='#')
 {
  D(bug("ppm.datatype/OM_NEW: Line 2 is a comment\n"));

  if(!FGets(FileHandle, LineBuffer, 128))
  {
   D(bug("ppm.datatype/OM_NEW: FGets line 3 after comment failed\n"));

   CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
   return(0);
  }
 }

 NumChars=StrToLong(LineBuffer, &Width);

 if(!((NumChars>0) && (Width>0)))
 {
  D(bug("ppm.datatype/OM_NEW: StrToLong(Width) failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: Width: %ld\n", (long) Width));
 D(bug("ppm.datatype/OM_NEW: NumChars: %ld\n", (long) NumChars));

 NumChars=StrToLong(LineBuffer+NumChars, &Height);

 if(!((NumChars>0) && (Height>0)))
 {
  D(bug("ppm.datatype/OM_NEW: StrToLong(Height) failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: Height: %ld\n", (long) Height));
 D(bug("ppm.datatype/OM_NEW: NumChars: %ld\n", (long) NumChars));

 if(!FGets(FileHandle, LineBuffer, 128))
 {
  D(bug("ppm.datatype/OM_NEW: FGets line 3 (4) failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 if(!(LineBuffer[0]=='2' && LineBuffer[1]=='5' && LineBuffer[2]=='5'))
 {
  D(bug("ppm.datatype/OM_NEW: Wrong depth\n"));

  SetIoErr(ERROR_OBJECT_WRONG_TYPE);
  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: Header successfull read\n"));

 bmhd->bmh_Width  = Width;
 bmhd->bmh_Height = Height;

 bmhd->bmh_PageWidth = bmhd->bmh_Width;
 bmhd->bmh_PageHeight = bmhd->bmh_Height;

 bmhd->bmh_Depth  = 8;

 nColors=1<<8;
 SetDTAttrs((Object *) RetVal, NULL, NULL, PDTA_NumColors, nColors, TAG_DONE);

 D(bug("ppm.datatype/OM_NEW: nColors set\n"));

 ColMap=NULL;
 ColRegs=NULL;

 if(!(GetDTAttrs((Object *) RetVal, PDTA_ColorRegisters, (ULONG) &ColMap,
				    PDTA_CRegs, &ColRegs,
				    TAG_DONE)==2))
 {
  D(bug("ppm.datatype/OM_NEW: GetDTAttrs(PDTA_ColorRegisters, PDTA_CRegs) failed\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 if(!(ColMap && ColRegs))
 {
  D(bug("ppm.datatype/OM_NEW: ColMap or ColRegs missing\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 Counter=0;
 Col7=0xFFFFFFFF/7;
 Col3=0xFFFFFFFF/3;

 for(i=0; i<=3; i++) /* blue */
 {
  for(j=0; j<=7; j++) /* green */
  {
   for(k=0; k<=7; k++) /* red */
   {
    ColMap[(i*64)+(j*8)+k].red=k*36;
    ColMap[(i*64)+(j*8)+k].green=j*36,
    ColMap[(i*64)+(j*8)+k].blue=i*85;

    ColRegs[Counter++]=k*Col7;
    ColRegs[Counter++]=j*Col7;
    ColRegs[Counter++]=i*Col3;
   }
  }
 }

 D(bug("ppm.datatype/OM_NEW: ColMap and ColRegs set\n"));

 /*
  *  Workaround for AROS' WriteChunkyPixels() quirk
  */

 scr=LockPubScreen(NULL);
 if(!scr)
 {
  D(bug("ppm.datatype/OM_NEW: Screen missing\n"));

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: Screen 0x%lx\n", (unsigned long) scr));

#if 0
 bm=AllocBitMap(bmhd->bmh_Width, bmhd->bmh_Height, bmhd->bmh_Depth, BMF_CLEAR, scr->RastPort.BitMap);
#else
 bm=AllocBitMap(bmhd->bmh_Width, bmhd->bmh_Height, bmhd->bmh_Depth, BMF_CLEAR, NULL);
#endif /* 0 */
 if(!bm)
 {
  D(bug("ppm.datatype/OM_NEW: AllocBitMap failed\n"));

  UnlockPubScreen(NULL, scr);

  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 UnlockPubScreen(NULL, scr);

 InitRastPort(&rp);
 rp.BitMap=bm;

 D(bug("ppm.datatype/OM_NEW: We have now a BitMap\n"));

 ModeID=BestModeID(BIDTAG_NominalWidth, bmhd->bmh_Width,
		   BIDTAG_NominalHeight, bmhd->bmh_Height,
		   BIDTAG_Depth, bmhd->bmh_Depth,
		   TAG_END);

 D(bug("ppm.datatype/OM_NEW: ModeID: 0x%lx\n", (unsigned long) ModeID));

 RGBBuffer=AllocVec(Width*3, MEMF_ANY | MEMF_CLEAR);
 if(!RGBBuffer)
 {
  D(bug("ppm.datatype/OM_NEW: AllocVec(RGBBuffer) failed\n"));

#ifdef _AROS
  DeinitRastPort(&rp);
#endif /* _AROS */
  FreeBitMap(bm);
  SetIoErr(ERROR_NO_FREE_STORE);
  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: RGBBuffer successfully allocated\n"));

 ChunkyBuffer=AllocVec(Width, MEMF_ANY | MEMF_CLEAR);
 if(!ChunkyBuffer)
 {
  D(bug("ppm.datatype/OM_NEW: AllocVec(ChunkyBuffer) failed\n"));

#ifdef _AROS
  DeinitRastPort(&rp);
#endif /* _AROS */
  FreeBitMap(bm);
  FreeVec(RGBBuffer);
  SetIoErr(ERROR_NO_FREE_STORE);
  CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
  return(0);
 }

 D(bug("ppm.datatype/OM_NEW: ChunkyBuffer successfully allocated\n"));

 Flush(FileHandle);

 for(i=0; i<Height; i++)
 {
  if(!(Read(FileHandle, RGBBuffer, (Width*3))==(Width*3)))
  {
   D(bug("ppm.datatype/OM_NEW: Read(RGBBuffer) failed\n"));

#ifdef _AROS
   DeinitRastPort(&rp);
#endif /* _AROS */
   FreeBitMap(bm);
   FreeVec(ChunkyBuffer);
   FreeVec(RGBBuffer);
   CoerceMethod(cl, (Object *) RetVal, OM_DISPOSE);
   return(0);
  }

  for(j=0; j<Width; j++)
  {
   ChunkyBuffer[j]=((RGBBuffer[j*3] & 0xE0)>>5) | ((RGBBuffer[j*3+1] & 0xE0)>>2) | (RGBBuffer[j*3+2] & 0xC0);
  }

  WriteChunkyPixels(&rp, 0, i, Width-1, i, ChunkyBuffer, Width);

 }

 D(bug("ppm.datatype/OM_NEW: C2P done\n"));

 SetDTAttrs((Object *) RetVal, NULL, NULL, DTA_ObjName,      Title,
					   DTA_NominalHoriz, bmhd->bmh_Width,
					   DTA_NominalVert,  bmhd->bmh_Height,
					   PDTA_BitMap,      bm,
					   PDTA_ModeID,      ModeID,
					   TAG_DONE);

 D(bug("ppm.datatype/OM_NEW: Yes!!! We have done it!\n"));

#ifdef _AROS
 DeinitRastPort(&rp);
#endif /* _AROS */
 FreeVec(ChunkyBuffer);
 FreeVec(RGBBuffer);

  D(bug("ppm.datatype/OM_NEW: Leaving\n"));

 return(RetVal);
}

STATIC IPTR DT_NotifyMethod(struct IClass *cl, struct Gadget *g, struct opUpdate *msg)
{
#ifdef MYDEBUG
 struct TagItem *tl;
 struct TagItem *ti;

 register int i;
 int Known;

 D(bug("ppm.datatype/OM_NOTIFY: Entering\n"));

 Known=FALSE;

 tl=msg->opu_AttrList;

 while((ti=NextTagItem(&tl)))
 {
  for(i=0; i<NumAttribs; i++)
  {
   if(ti->ti_Tag==KnownAttribs[i])
   {
    Known=TRUE;

    D(bug("ppm.datatype/OM_NOTIFY: %s %ld\n", AttribNames[i], ti->ti_Data));
   }
  }

  if(!Known)
  {
   D(bug("ppm.datatype/OM_NOTIFY: 0x%lx %ld\n", ti->ti_Tag, ti->ti_Data));
  }
 }

 D(bug("ppm.datatype/OM_NOTIFY: Leaving\n"));

#endif /* MYDEBUG */

 return(DoSuperMethodA(cl, (Object *) g, (Msg) msg));
}

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
 IPTR RetVal;

#ifdef MYDEBUG
 struct TagItem *tl;
 struct TagItem *ti;

 register int i;
 int Known;
#endif /* MYDEBUG */

 D(bug("ppm.datatype/OM_SET: Entering\n"));

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

#ifdef MYDEBUG
 Known=FALSE;

 tl=msg->ops_AttrList;

 while((ti=NextTagItem(&tl)))
 {
  switch (ti->ti_Tag)
  {
   case PDTA_ModeID:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: PDTA_ModeID\n"));

    break;
   }

   case PDTA_BitMap:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: PDTA_BitMap\n"));

    break;
   }

   case PDTA_NumColors:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: PDTA_NumColors\n"));

    break;
   }

   case PDTA_Screen:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: PDTA_Screen\n"));

    break;
   }

   case PDTA_FreeSourceBitMap:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: PDTA_FreeSourceBitMap\n"));

    break;
   }

   case PDTA_Grab:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: PDTA_Grab\n"));

    break;
   }

   case PDTA_ClassBitMap:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: PDTA_ClassBitMap\n"));

    break;
   }

   case DTA_TopHoriz:
   {
    D(bug("ppm.datatype/OM_SET: Tag ID: DTA_TopHoriz\n"));

    D(bug("ppm.datatype/OM_SET: DTA_TopHoriz %lx\n", (int) ti->ti_Data));

    break;
   }

   default:
   {
    for(i=0; i<NumAttribs; i++)
    {
     if(ti->ti_Tag==KnownAttribs[i])
     {
      Known=TRUE;

      D(bug("ppm.datatype/OM_SET: Tag ID: %s\n", AttribNames[i]));
     }
    }

    if(!Known)
    {
     D(bug("ppm.datatype/OM_SET: Tag ID 0x%lx\n", ti->ti_Tag));
    }
   }
  }
 }

 D(bug("ppm.datatype/OM_SET: Leaving\n"));

#endif /* MYDEBUG */

 return(RetVal);
}

STATIC IPTR DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
 IPTR RetVal;
#ifdef MYDEBUG
 register int i;
 int Known;
#endif /* MYDEBUG */

 D(bug("ppm.datatype/OM_GET: Entering\n"));

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

#ifdef MYDEBUG
 Known=FALSE;

 switch(msg->opg_AttrID)
 {
  case PDTA_ModeID:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_ModeID\n"));

   break;
  }

  case PDTA_BitMapHeader:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_BitMapHeader\n"));

   break;
  }

  case PDTA_BitMap:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_BitMap\n"));

   break;
  }

  case PDTA_ColorRegisters:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_ColorRegisters\n"));

   break;
  }

  case PDTA_CRegs:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_CRegs\n"));

   break;
  }

  case PDTA_GRegs:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_GRegs\n"));

   break;
  }

  case PDTA_ColorTable:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_ColorTable\n"));

   break;
  }

  case PDTA_ColorTable2:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_ColorTable2\n"));

   break;
  }

  case PDTA_Allocated:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_Allocated\n"));

   break;
  }

  case PDTA_NumColors:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_NumColors\n"));

   break;
  }

  case PDTA_NumAlloc:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_NumAlloc\n"));

   break;
  }

  case PDTA_Grab:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_Grab\n"));

   break;
  }

  case PDTA_DestBitMap:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_DestBitMap\n"));

   break;
  }

  case PDTA_ClassBitMap:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: PDTA_ClassBitMap\n"));

   break;
  }

  case DTA_Methods:
  {
   D(bug("ppm.datatype/OM_GET: Tag ID: DTA_Methods\n"));

   break;
  }

  case DTA_FrameInfo:
  {
   struct FrameInfo *FI;

   D(bug("ppm.datatype/OM_GET: Tag ID: DTA_FrameInfo\n"));

   FI=(struct FrameInfo *) *(msg->opg_Storage);

   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_PropertyFlags     : 0x%lx\n", (unsigned int) FI->fri_PropertyFlags));

   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_Resolution.x      : %ld\n", (int) FI->fri_Resolution.x));
   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_Resolution.y      : %ld\n", (int) FI->fri_Resolution.y));

   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_RedBits           : 0x%lx\n", (unsigned int) FI->fri_RedBits));
   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_GreenBits         : 0x%lx\n", (unsigned int) FI->fri_GreenBits));
   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_BlueBits          : 0x%lx\n", (unsigned int) FI->fri_BlueBits));

   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_Dimensions.Width  : %lu\n", (unsigned int) FI->fri_Dimensions.Width));
   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_Dimensions.Height : %lu\n", (unsigned int) FI->fri_Dimensions.Height));
   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_Dimensions.Depth  : %lu\n", (unsigned int) FI->fri_Dimensions.Depth));

   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_Screen            : 0x%lx\n", (unsigned int) FI->fri_Screen));
   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_ColorMap          : 0x%lx\n", (unsigned int) FI->fri_ColorMap));

   D(bug("ppm.datatype/OM_GET: FrameInfo->fri_Flags             : 0x%lx\n", (unsigned int) FI->fri_Flags));

   break;
  }

  case DTA_Domain:
  {
   struct IBox *IB;

   D(bug("ppm.datatype/OM_GET: Tag ID: DTA_Domain\n"));

   IB=(struct IBox *) *(msg->opg_Storage);

   D(bug("ppm.datatype/OM_GET: Domain %ld %ld %ld %ld\n", (int) IB->Left, (int) IB->Top, (int) IB->Width, (int) IB->Height));

   break;
  }

  default:
  {
   for(i=0; i<NumAttribs; i++)
   {
    if(msg->opg_AttrID==KnownAttribs[i])
    {
     Known=TRUE;

     D(bug("ppm.datatype/OM_GET: Tag ID: %s\n", AttribNames[i]));
    }
   }

   if(!Known)
   {
    D(bug("ppm.datatype/OM_GET: Tag ID 0x%lx\n", msg->opg_AttrID));
   }
  }
 }

 D(bug("ppm.datatype/OM_GET: Leaving\n"));

#endif /* MYDEBUG */

 return(RetVal);
}

STATIC IPTR DT_LayoutMethod(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;
 const char L[]="GM_LAYOUT";
 const char P[]="DTM_PROCLAYOUT";
 const char A[]="DTM_ASYNCLAYOUT";
 const char U[]="Unknown Method";
 char *MethodName;

#ifdef MYDEBUG
 switch(msg->MethodID)
 {
  case GM_LAYOUT:
  {
   MethodName=L;

   break;
  }

  case DTM_PROCLAYOUT:
  {
   MethodName=P;

   break;
  }

  case DTM_ASYNCLAYOUT:
  {
   MethodName=A;

   break;
  }

  default:
  {
   MethodName=U;
  }
 }

 D(bug("ppm.datatype/%s: Entering\n", MethodName));

 D(bug("ppm.datatype/%s: MethodID 0x%lx\n", MethodName, msg->MethodID));

if(msg->gpl_GInfo)
{
 D(bug("ppm.datatype/%s: GagdetInfo->Screen 0x%lx\n", MethodName, (unsigned int) msg->gpl_GInfo->gi_Screen));
 D(bug("ppm.datatype/%s: GagdetInfo->Window 0x%lx\n", MethodName, (unsigned int) msg->gpl_GInfo->gi_Window));
 D(bug("ppm.datatype/%s: GagdetInfo->Requester 0x%lx\n", MethodName, (unsigned int) msg->gpl_GInfo->gi_Requester));
 D(bug("ppm.datatype/%s: GagdetInfo->RastPort 0x%lx\n", MethodName, (unsigned int) msg->gpl_GInfo->gi_RastPort));
 D(bug("ppm.datatype/%s: GagdetInfo->Layer 0x%lx\n", MethodName, (unsigned int) msg->gpl_GInfo->gi_Layer));
 D(bug("ppm.datatype/%s: GagdetInfo->Domain %ld %ld %ld %ld\n", MethodName, (int) msg->gpl_GInfo->gi_Domain.Left, (int) msg->gpl_GInfo->gi_Domain.Top, (int) msg->gpl_GInfo->gi_Domain.Width, (int) msg->gpl_GInfo->gi_Domain.Height));
}
else
{
 D(bug("ppm.datatype/%s: GadgetInfo is NULL\n", MethodName));
}

 D(bug("ppm.datatype/%s: gpl_Initial 0x%lx\n", MethodName, msg->gpl_Initial));
#endif /* MYDEBUG */

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 D(bug("ppm.datatype/%s: RetVal 0x%lx\n", MethodName, (unsigned int) RetVal));

 D(bug("ppm.datatype/%s: Leaving\n", MethodName));

 return(RetVal);
}

/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/

#ifdef _AROS
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(Msg, msg, A1))
#else
ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
#endif
{
#ifdef _AROS
    AROS_USERFUNC_INIT
#endif

    IPTR retval;

#ifdef MYDEBUG
    register int i;
    int Known;

    Known=FALSE;
#endif /* MYDEBUG */

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

    D(bug("ppm.datatype/DT_Dispatcher: Entering\n"));

    switch(msg->MethodID)
    {
	case OM_NEW:

	    D(bug("ppm.datatype/DT_Dispatcher: Method OM_NEW\n"));

	    retval = PPM_New(cl, o, (struct opSet *)msg);
	    break;

	case OM_UPDATE:
	case OM_SET:

	    D(bug("ppm.datatype/DT_Dispatcher: Method %s\n", (msg->MethodID==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));

	    retval = DT_SetMethod(cl, (struct Gadget *) o, (struct opSet *) msg);

	    break;

	case OM_GET:

	    D(bug("ppm.datatype/DT_Dispatcher: Method OM_GET\n"));

	    retval = DT_GetMethod(cl, (struct Gadget *) o, (struct opGet *) msg);

	    break;

	case OM_NOTIFY:

	    D(bug("ppm.datatype/DT_Dispatcher: Method OM_NOTIFY\n"));

	    retval = DT_NotifyMethod(cl, (struct Gadget *) o, (struct opUpdate *) msg);

	    break;

	case GM_LAYOUT: 

	    D(bug("ppm.datatype/DT_Dispatcher: Method GM_LAYOUT\n"));

	    retval = DT_LayoutMethod(cl, (struct Gadget *) o, (struct gpLayout *) msg);

	    break;


	case DTM_PROCLAYOUT:

	    D(bug("ppm.datatype/DT_Dispatcher: Method DTM_PROCLAYOUT\n"));

	    retval = DT_LayoutMethod(cl, (struct Gadget *) o, (struct gpLayout *) msg);

	    break;


	case DTM_ASYNCLAYOUT:

	    D(bug("ppm.datatype/DT_Dispatcher: Method DTM_ASYNCLAYOUT\n"));

	    retval = DT_LayoutMethod(cl, (struct Gadget *) o, (struct gpLayout *) msg);

	    break;

	case GM_RENDER:

	    D(bug("ppm.datatype/DT_Dispatcher: Method GM_RENDER\n"));

	    D(bug("ppm.datatype/GM_RENDER: Entering\n"));

	    retval = DoSuperMethodA(cl, o, msg);

	    D(bug("ppm.datatype/GM_RENDER: Leaving\n"));

	    break;

	default:

#ifdef MYDEBUG
	    for(i=0; i<NumMethods; i++)
	    {
	     if(msg->MethodID==KnownMethods[i])
	     {
	      Known=TRUE;

	      D(bug("ppm.datatype/DT_Dispatcher: Method %s\n", MethodNames[i]));
	     }
	    }

	    if(!Known)
	    {
	     D(bug("ppm.datatype/DT_Dispatcher: Method 0x%lx\n", (unsigned long) msg->MethodID));
	    }
#endif /* MYDEBUG */

#if 0
	    D(bug("ppm.datatype/DT_Dispatcher: Method 0x%lx %lu\n", (unsigned long) msg->MethodID, (unsigned long) msg->MethodID));
#endif /* 0 */

	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch(msg->MethodID) */

    D(bug("ppm.datatype/DT_Dispatcher: Leaving\n"));

    return retval;
#ifdef _AROS
    AROS_USERFUNC_EXIT
#endif
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *ppmbase)
{
    struct IClass *cl = MakeClass("ppm.datatype", "picture.datatype", 0, 0, 0);

    D(bug("ppm.datatype/DT_MakeClass: DT_Dispatcher 0x%lx\n", (unsigned long) DT_Dispatcher));

    if (cl)
    {
#ifdef _AROS
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR)ppmbase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}

/**************************************************************************************************/

