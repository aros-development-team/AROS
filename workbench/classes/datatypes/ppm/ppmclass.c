/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
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

 D(bug("ppm.datatype/OM_NEW: Reached\n"));

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

 bm=AllocBitMap(bmhd->bmh_Width, bmhd->bmh_Height, bmhd->bmh_Depth, BMF_CLEAR, scr->RastPort.BitMap);
#if 0
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
{
    AROS_USERFUNC_INIT
#else
ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
{
#endif
    IPTR retval;

#ifdef MYDEBUG
    register int i;
    int Known;

    Known=FALSE;
#endif /* MYDEBUG */

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

#if 0
    D(bug("ppm.datatype/DT_Dispatcher: Reached\n"));
#endif /* 0 */

    switch(msg->MethodID)
    {
	case OM_NEW:

	    D(bug("ppm.datatype/DT_Dispatcher: Method OM_NEW\n"));

	    retval = PPM_New(cl, o, (struct opSet *)msg);
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

	if(msg->MethodID==OM_NOTIFY)
	{
	 struct opUpdate *opUp;
	 struct TagItem *tl;
	 struct TagItem *ti;

	 opUp=(struct opUpdate *) msg;

	 tl=opUp->opu_AttrList;

	 Known=FALSE;

	 while(ti=NextTagItem(&tl))
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
	}


#endif /* MYDEBUG */

#if 0
	    D(bug("ppm.datatype/DT_Dispatcher: Method 0x%lx %lu\n", (unsigned long) msg->MethodID, (unsigned long) msg->MethodID));
#endif /* 0 */

	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch(msg->MethodID) */

#if 0
    D(bug("ppm.datatype/DT_Dispatcher: Leaving\n"));
#endif /* 0 */

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

