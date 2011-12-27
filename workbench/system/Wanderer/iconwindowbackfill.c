/*
  Copyright  2004-2011, The AROS Development Team. All rights reserved.
  $Id$
*/

#include "portable_macros.h"

#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

#define WANDERER_MODULE_BACKFILL_ENABLED

#ifdef __AROS__
#define DEBUG 0
#include <aros/debug.h>
#endif

#include <exec/types.h>
#include <libraries/mui.h>

#ifdef __AROS__
#include <zune/customclasses.h>
#else
#include <zune_AROS/customclasses.h>
#endif


#include <proto/utility.h>

#include <proto/graphics.h>

#include <proto/exec.h>
#include <proto/datatypes.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include <intuition/screens.h>
#include <datatypes/pictureclass.h>
#include <clib/macros.h>

#ifdef __AROS__
#include <clib/alib_protos.h>
#endif

#include <graphics/scale.h>

#ifdef __AROS__
#include <prefs/wanderer.h>
#else
#include <prefs_AROS/wanderer.h>
#endif

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#ifndef _PROTO_INTUITION_H
#include <proto/intuition.h>
#endif
#include <proto/muimaster.h>

#include "wanderer.h"
#include "wandererprefs.h"
#include "iconwindow.h"
#include "iconwindow_iconlist.h"
#include "iconwindowbackfill.h"
#include "Classes/iconlistview.h"
#include "Classes/iconlist_attributes.h"


#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

/*** Global Data **********************************************************/

static struct IconWindow_BackFill_Descriptor   image_backfill_descriptor;
static char                                  image_backfill_name[] = "Default Image BackFill\0";
static struct List                             image_backfill_images;

/*** Internal functions *********************************************************/

///ImageBackFill_FindSourceRecord()
static struct BackFillSourceImageRecord *ImageBackFill_FindSourceRecord(char *source_name, IPTR source_mode)
{
  struct BackFillSourceImageRecord *source_record = NULL;
  #ifdef __AROS__
  ForeachNode(&image_backfill_images, source_record)
  #else
  Foreach_Node(&image_backfill_images, source_record);
  #endif
  {
    if ((strcmp(source_record->bfsir_SourceImage, source_name)==0) && (source_record->bfsir_BackGroundRenderMode == source_mode)) return source_record;
  }
  return NULL;
}
///

///ImageBackFill_FindBufferRecord()
static struct BackFillSourceImageBuffer *ImageBackFill_FindBufferRecord(struct BackFillSourceImageRecord *source_record, WORD buffer_width, WORD buffer_height)
{
  struct BackFillSourceImageBuffer *buffer_record = NULL;
  
  #ifdef __AROS__
  ForeachNode(&source_record->bfsir_Buffers, buffer_record)
  #else
  Foreach_Node(&source_record->bfsir_Buffers, buffer_record);
  #endif
  {
    if ((buffer_record->bfsib_BitMapWidth == buffer_width) && (buffer_record->bfsib_BitMapHeight == buffer_height)) return buffer_record;
  }
  return NULL;
}
///

///ImageBackFill_CloseSourceRecord()
static void ImageBackFill_CloseSourceRecord(struct BackFillSourceImageRecord *this_Record)
{
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CloseSourceRecord()\n"));
  if (this_Record->bfsir_OpenerCount >= 2)
  {
    this_Record->bfsir_OpenerCount -= 1;
    D(bug("[IconWindow.ImageBackFill] ImageBackFill_CloseSourceRecord: %d Openers Remaining for '%s'\n", this_Record->bfsir_OpenerCount, this_Record->bfsir_SourceImage));
    return;
  }
  else
  {
    Remove((struct Node *)this_Record);
    this_Record->bfsir_OpenerCount = 0;

    D(bug("[IconWindow.ImageBackFill] ImageBackFill_CloseSourceRecord: Closing Record for '%s'\n", this_Record->bfsir_SourceImage));

    this_Record->bfsir_DTRastPort->BitMap = NULL;
    FreeRastPort(this_Record->bfsir_DTRastPort);
    DisposeDTObject(this_Record->bfsir_DTPictureObject);
    FreeVec(this_Record->bfsir_SourceImage);
    FreeMem(this_Record, sizeof(struct BackFillSourceImageRecord));
  }
}
///

///ImageBackFill_CloseSourceBuffer()
static void ImageBackFill_CloseSourceBuffer(struct BackFillSourceImageRecord *this_Record, struct BackFillSourceImageBuffer *this_Buffer)
{
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CloseSourceBuffer()\n"));
  if (this_Buffer->bfsib_OpenerCount >= 2)
  {
    this_Buffer->bfsib_OpenerCount -= 1;
    D(bug("[IconWindow.ImageBackFill] ImageBackFill_CloseSourceBuffer: %d Openers Remaining\n", this_Buffer->bfsib_OpenerCount));
    return;
  }
  else
  {
    Remove((struct Node *)this_Buffer);
    this_Buffer->bfsib_OpenerCount = 0;
    D(bug("[IconWindow.ImageBackFill] ImageBackFill_CloseSourceBuffer: Closing Buffer [%d x %d] of '%s'\n", this_Buffer->bfsib_BitMapWidth, this_Buffer->bfsib_BitMapHeight, this_Record->bfsir_SourceImage));

    this_Buffer->bfsib_BitMapRastPort->BitMap = NULL;
    FreeRastPort(this_Buffer->bfsib_BitMapRastPort);
    FreeBitMap(this_Buffer->bfsib_BitMap);
    FreeMem(this_Buffer, sizeof(struct BackFillSourceImageBuffer));
  }
}
///

///ImageBackFill_CopyScaledBitMap()
static void ImageBackFill_CopyScaledBitMap
(
  struct RastPort *SrcRast,
  WORD SrcOffsetX, WORD SrcOffsetY,
  WORD SrcSizeX, WORD SrcSizeY,
  struct RastPort *DstRast,
  struct Rectangle *DstAreaBounds,
  struct Rectangle *DstFillBounds,
  ULONG blit_MODE
)
{
  struct BitScaleArgs   Scale_Args;
D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyScaledBitMap()\n"));

  Scale_Args.bsa_SrcX = SrcOffsetX;
  Scale_Args.bsa_SrcY = SrcOffsetY;   
  Scale_Args.bsa_SrcWidth = SrcSizeX;
  Scale_Args.bsa_SrcHeight = SrcSizeY;
  Scale_Args.bsa_XSrcFactor = SrcSizeX;
  Scale_Args.bsa_YSrcFactor = SrcSizeY;
  Scale_Args.bsa_DestX = DstFillBounds->MinX;
  Scale_Args.bsa_DestY = DstFillBounds->MinY;
  Scale_Args.bsa_XDestFactor = (DstFillBounds->MaxX - DstFillBounds->MinX) + 1;
  Scale_Args.bsa_YDestFactor = (DstFillBounds->MaxY - DstFillBounds->MinY) + 1;

  Scale_Args.bsa_SrcBitMap = SrcRast->BitMap;
  Scale_Args.bsa_DestBitMap = DstRast->BitMap;
  
  BitMapScale(&Scale_Args);
}
///

///ImageBackFill_CopyTiledBitMap()
static void ImageBackFill_CopyTiledBitMap
(
  struct RastPort *SrcRast,
  WORD SrcOffsetX, WORD SrcOffsetY,
  WORD SrcSizeX, WORD SrcSizeY,
  struct RastPort *DstRast,
  struct Rectangle *DstAreaBounds,
  struct Rectangle *DstFillBounds,
  ULONG blit_MODE
)
{
  
  WORD FirstSizeX;  // the width of the rectangle to blit as the first column
  WORD FirstSizeY;  // the height of the rectangle to blit as the first row
  WORD SecondMinX;  // the left edge of the second column
  WORD SecondMinY;  // the top edge of the second column
  WORD SecondSizeX; // the width of the second column
  WORD SecondSizeY; // the height of the second column
  WORD PosX, PosY;         // used as starting position in the "exponential" blit
  WORD SizeX, SizeY;        // used as bitmap size in the "exponential" blit
  WORD SrcX, SrcY;        // used as bitmap size in the "exponential" blit

  struct BitMap *Src = SrcRast->BitMap;
  struct BitMap *Dst = DstRast->BitMap; 

  #if defined(DEBUG)
  int xcount;
  int ycount;
  #endif
    

D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap(mode %d)\n", blit_MODE));

  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: SrcRast @ %x, DstRast @ %x\n", SrcRast, DstRast));
    
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: AreaBounds @ %x, FillBounds @ %x\n", DstAreaBounds, DstFillBounds));
  
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: AreaBounds.MinX %d\n", DstAreaBounds->MinX));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: AreaBounds.MinY %d\n", DstAreaBounds->MinY));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: AreaBounds.MaxX %d\n", DstAreaBounds->MaxX));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: AreaBounds.MaxY %d\n", DstAreaBounds->MaxY));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: AreaBounds Width %d Height %d\n", (DstAreaBounds->MaxX - DstAreaBounds->MinX) + 1, (DstAreaBounds->MaxY - DstAreaBounds->MinY) + 1));
    
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: FillBounds.MinX %d\n", DstFillBounds->MinX));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: FillBounds.MinY %d\n", DstFillBounds->MinY));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: FillBounds.MaxX %d\n", DstFillBounds->MaxX));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: FillBounds.MaxY %d\n", DstFillBounds->MaxY));
  D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: FillBounds Width %d Height %d\n", (DstAreaBounds->MaxX - DstAreaBounds->MinX) + 1, (DstAreaBounds->MaxY - DstAreaBounds->MinY) + 1));

  FirstSizeX = MIN(SrcSizeX - SrcOffsetX, RECTSIZEX(DstFillBounds)); // the width of the first tile, this is either the rest of the tile right to SrcOffsetX or the width of the dest rect, if the rect is narrow
  SecondMinX = DstFillBounds->MinX + FirstSizeX; // the start for the second tile (if used)
  SecondSizeX = MIN(SrcOffsetX, DstFillBounds->MaxX-SecondMinX + 1); // the width of the second tile (we want the whole tile to be SrcSizeX pixels wide, if we use SrcSizeX-SrcOffsetX pixels for the left part we'll use SrcOffsetX for the right part)

  FirstSizeY = MIN(SrcSizeY - SrcOffsetY, RECTSIZEY(DstFillBounds)); // the same values are calculated for y direction
  SecondMinY = DstFillBounds->MinY + FirstSizeY;
  SecondSizeY = MIN(SrcOffsetY, DstFillBounds->MaxY - SecondMinY + 1);

  if (blit_MODE == blit_MODE_Blit)  // blit the first piece of the tile
  {
    D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: 1st Tile Part @ %d,%d [%d x %d]\n", DstFillBounds->MinX, DstFillBounds->MinY, FirstSizeX, FirstSizeY));
    BltBitMap(Src, 
        SrcOffsetX, SrcOffsetY,
        Dst,
        DstFillBounds->MinX, DstFillBounds->MinY,
        FirstSizeX, FirstSizeY,
        0xC0, -1, NULL); 


    if (SecondSizeX > 0) // if SrcOffset was 0 or the dest rect was too narrow, we won't need a second column
    {
      D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: 2nd Tile Part @ %d,%d [%d x %d]\n", SecondMinX, DstFillBounds->MinY, SecondSizeX, FirstSizeY));
      BltBitMap(Src,
            0, SrcOffsetY,
            Dst,
            SecondMinX, DstFillBounds->MinY,
            SecondSizeX, FirstSizeY,
            0xC0, -1, NULL);
    }

    if (SecondSizeY > 0) // is a second row necessary?
    {
      D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: 3rd Tile Part @ %d,%d [%d x %d]\n", DstFillBounds->MinX, SecondMinY, FirstSizeX, SecondSizeY));
      BltBitMap(Src,
            SrcOffsetX, 0,
            Dst,
            DstFillBounds->MinX, SecondMinY,
            FirstSizeX, SecondSizeY,
            0xC0, -1, NULL);

      if (SecondSizeX > 0)
      {
        D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: 4th Tile Part @ %d,%d [%d x %d]\n", SecondMinX, SecondMinY, SecondSizeX, SecondSizeY));
        BltBitMap(Src,
            0, 0,
            Dst,
            SecondMinX, SecondMinY,
            SecondSizeX, SecondSizeY,
            0xC0, -1, NULL);
      }
    }

#if defined(DEBUG)
    xcount = 2;
#endif
    //Generates the first row of the tiles ....
    for (PosX = DstFillBounds->MinX + SrcSizeX, SizeX = MIN(SrcSizeX, (DstFillBounds->MaxX - PosX) + 1);PosX <= DstFillBounds->MaxX;)
    {
#if defined(DEBUG)
      D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: Row 1 Tile %d @ %d,%d [%d x %d]\n", xcount, PosX, DstFillBounds->MinY, SizeX, MIN(SrcSizeY, RECTSIZEY(DstFillBounds))));
      xcount++;
#endif
      BltBitMap(Dst,
          DstFillBounds->MinX, DstFillBounds->MinY,
          Dst,
          PosX, DstFillBounds->MinY,
          SizeX, MIN(SrcSizeY, RECTSIZEY(DstFillBounds)),
          0xC0, -1, NULL);

      PosX += SizeX;
      SizeX = MIN(SrcSizeX, (DstFillBounds->MaxX - PosX) + 1);
    }

#if defined(DEBUG)
    ycount = 2;
#endif
    // .. now Blit the first row down several times to fill the whole dest rect
    for (PosY = DstFillBounds->MinY + SrcSizeY, SizeY = MIN(SrcSizeY, (DstFillBounds->MaxY - PosY) + 1);PosY <= DstFillBounds->MaxY;)
    {
#if defined(DEBUG)
      D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: Row %d @ %d,%d [%d x %d]\n", ycount, DstFillBounds->MinX, PosY, MIN(SrcSizeX, RECTSIZEX(DstFillBounds)), SizeY));
      ycount++;
#endif
      BltBitMap(Dst,
            DstFillBounds->MinX, DstFillBounds->MinY,
            Dst,
            DstFillBounds->MinX, PosY,
            MIN(SrcSizeX, RECTSIZEX(DstFillBounds)), SizeY,
            0xC0, -1, NULL);

      PosY += SizeY;
      SizeY = MIN(SrcSizeY, (DstFillBounds->MaxY - PosY) + 1);
    }
  }
  else if (blit_MODE == blit_MODE_Clip)
  {
    for (SrcY = MOD(SrcOffsetY + MOD((DstFillBounds->MinY - DstAreaBounds->MinY), SrcSizeY - 1), SrcSizeY - 1), PosY = DstFillBounds->MinY, SizeY = MIN((SrcSizeY - SrcY), (DstFillBounds->MaxY - DstFillBounds->MinY ) + 1); PosY <= DstFillBounds->MaxY;)
    {
      for (SrcX = MOD(SrcOffsetX + MOD((DstFillBounds->MinX - DstAreaBounds->MinX), SrcSizeX - 1), SrcSizeX - 1), PosX = DstFillBounds->MinX, SizeX = MIN((SrcSizeX - SrcX), (DstFillBounds->MaxX - DstFillBounds->MinX ) + 1); PosX <= DstFillBounds->MaxX;)
      {
        D(bug("[IconWindow.ImageBackFill] ImageBackFill_CopyTiledBitMap: ClipBlit %d, %d -> %d,%d [%d x %d]\n", SrcX, SrcY, PosX, PosY, SizeX, SizeY));

        ClipBlit(SrcRast,
          SrcX, SrcY,
          DstRast,
          PosX, PosY,
          SizeX, SizeY,
          0xC0);

        if (SrcX != 0) SrcX = 0;

        PosX += SizeX;
        SizeX = MIN(SrcSizeX, (DstFillBounds->MaxX - PosX) + 1);
      }

      if (SrcY != 0) SrcY = 0;

      PosY += SizeY;
      SizeY = MIN(SrcSizeY, (DstFillBounds->MaxY - PosY) + 1);
    }
  }
}
///

/*** Methods ****************************************************************/
///ImageBackFill__MUIM_IconWindow_BackFill_ProcessBackground()
IPTR ImageBackFill__MUIM_IconWindow_BackFill_ProcessBackground
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_ProcessBackground *message
)
{

  LONG        Depth = 0;
  Object      *_IconWindows_PrefsObj    = NULL,
              *_IconWindows_WindowObj   = NULL;
  Object      *_IconWindows_IconListObj = NULL;

  BOOL        options_changed = FALSE;

  IPTR        prefs_processing        = 0;
  
  IPTR        BackGround_Attrib       = 0,
              BackGround_Base         = 0,
              BackGround_RenderMode   = 0,
              BackGround_TileMode     = 0,
              BackGround_XOffset      = 0,
              BackGround_YOffset      = 0;
  
  struct BackFillInfo   *this_BFI =(struct BackFillInfo   *) message->BackFill_Data;

  UBYTE                  *this_bgtype;
  char                  *this_ImageName;

  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground()\n"));
  
  GET(_app(self), MUIA_Wanderer_Prefs, &_IconWindows_PrefsObj);

  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: PrefsObj @ %x\n", _IconWindows_PrefsObj));
  
  GET(self, MUIA_IconWindow_Window, &_IconWindows_WindowObj);
  GET(self, MUIA_IconWindow_IconList, &_IconWindows_IconListObj);

  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: MUIA_IconWindow_Window = %x\n", _IconWindows_WindowObj));

  if ((_IconWindows_PrefsObj == NULL) || (_IconWindows_WindowObj == NULL) || (_IconWindows_IconListObj == NULL)) return FALSE;

  GET(_IconWindows_PrefsObj, MUIA_WandererPrefs_Processing, &prefs_processing);

#if defined(DEBUG)
  if (prefs_processing)
  {
    D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Wanderer Prefs (re)loading detected\n"));
  }
#endif
  
  GET(_IconWindows_WindowObj, MUIA_IconWindow_BackgroundAttrib, &BackGround_Attrib);
  
  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Background Attrib = '%s'\n", BackGround_Attrib));

  if (BackGround_Attrib == (IPTR) NULL) return FALSE;
  
  if ((BackGround_Base = DoMethod(_IconWindows_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                  BackGround_Attrib, MUIA_Background)) == -1)
    return FALSE;

  if ((BackGround_RenderMode = DoMethod(_IconWindows_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                  BackGround_Attrib, MUIA_IconWindowExt_ImageBackFill_BGRenderMode)) == -1)
    BackGround_RenderMode = IconWindowExt_ImageBackFill_RenderMode_Tiled;

  this_bgtype    = (UBYTE *)BackGround_Base;
  this_ImageName = (char *)(BackGround_Base + 2);
  
  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: BackFillInfo @ %x\n", this_BFI));
  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Background '%s', mode %d\n", BackGround_Base, BackGround_RenderMode));

  if ((this_bgtype[0] - 48) != 5)
  {
    D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Background is NOT an image - letting our windowclass handle it...\n"));
    
    goto pb_cleanup_buffer;
  }
  
  GET(self, MUIA_Window_Screen, &this_BFI->bfi_Screen);
  GET(_IconWindows_IconListObj, MUIA_IconList_BufferRastport, &this_BFI->bfi_RastPort);

  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Wanderers Screen @ %x, Using RastPort @ %x\n", this_BFI->bfi_Screen, this_BFI->bfi_RastPort));
  
  if (this_BFI->bfi_Source)
  {
    D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: BackFillInfo has existing source record @ %x\n", this_BFI->bfi_Source));
    if ((strcmp(this_BFI->bfi_Source->bfsir_SourceImage, this_ImageName) == 0) && (this_BFI->bfi_Source->bfsir_BackGroundRenderMode == BackGround_RenderMode))
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: existing BackFillInfo Using the same background / mode\n"));
      goto check_imagebuffer;
    }
    else
    {
      if (this_BFI->bfi_Buffer)
      {
        ImageBackFill_CloseSourceBuffer(this_BFI->bfi_Source, this_BFI->bfi_Buffer);
        this_BFI->bfi_Buffer = NULL;
      }
      ImageBackFill_CloseSourceRecord(this_BFI->bfi_Source);
      this_BFI->bfi_Source = NULL;
    }
  }

  if (!(this_BFI->bfi_Source))
  {
    /* If the previous source is not existing, it means there was a change with the buffer */
    options_changed = TRUE;
  }

  if (!(this_BFI->bfi_Source)) this_BFI->bfi_Source = ImageBackFill_FindSourceRecord(this_ImageName, BackGround_RenderMode);

  if (!(this_BFI->bfi_Source))
  {
    D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Creating NEW ImageSource Record\n"));
    if (!(this_BFI->bfi_Source = AllocMem(sizeof(struct BackFillSourceImageRecord), MEMF_CLEAR|MEMF_PUBLIC)))
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Couldnt allocate enough mem for source record!\n"));      
      return FALSE;
    }

    if (!(this_BFI->bfi_Source->bfsir_SourceImage = AllocVec(strlen(this_ImageName) +1, MEMF_CLEAR|MEMF_PUBLIC)))
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Couldnt allocate enough mem for source image name store\n"));     
      FreeMem(this_BFI->bfi_Source, sizeof(struct BackFillSourceImageRecord));
      return FALSE;
    }
    strcpy(this_BFI->bfi_Source->bfsir_SourceImage, this_ImageName);
    
    if ((this_BFI->bfi_Source->bfsir_DTPictureObject = NewDTObject(this_BFI->bfi_Source->bfsir_SourceImage,
                          DTA_SourceType,         DTST_FILE,
                          DTA_GroupID,            GID_PICTURE,
                          PDTA_DestMode,          PMODE_V43,
                          PDTA_Remap,             TRUE,
                          PDTA_Screen,            this_BFI->bfi_Screen,
                          PDTA_FreeSourceBitMap,  TRUE,
                          OBP_Precision,          PRECISION_IMAGE,
                          PDTA_DitherQuality,     2,
                          TAG_DONE)))
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Opened Datatype Object @ %x for image '%s'\n", this_BFI->bfi_Source->bfsir_DTPictureObject, this_BFI->bfi_Source->bfsir_SourceImage));
      if (DoMethod(this_BFI->bfi_Source->bfsir_DTPictureObject, DTM_PROCLAYOUT, NULL, 1))
      {
        D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Caused Datatype Object LAYOUT\n"));

        GetDTAttrs(this_BFI->bfi_Source->bfsir_DTPictureObject, PDTA_BitMapHeader, &this_BFI->bfi_Source->bfsir_DTBitMapHeader, TAG_DONE);
        GetDTAttrs(this_BFI->bfi_Source->bfsir_DTPictureObject, PDTA_DestBitMap, &this_BFI->bfi_Source->bfsir_DTBitMap, TAG_DONE);

        if (!this_BFI->bfi_Source->bfsir_DTBitMap)
          GetDTAttrs(this_BFI->bfi_Source->bfsir_DTPictureObject, PDTA_BitMap, &this_BFI->bfi_Source->bfsir_DTBitMap, TAG_DONE);
      
        if (this_BFI->bfi_Source->bfsir_DTBitMap)
        {
          D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Datatype Object BitMap @ %x\n", this_BFI->bfi_Source->bfsir_DTBitMap));

          if ((this_BFI->bfi_Source->bfsir_DTRastPort = CreateRastPort()))
          {
            this_BFI->bfi_Source->bfsir_DTRastPort->BitMap = this_BFI->bfi_Source->bfsir_DTBitMap;
            this_BFI->bfi_Source->bfsir_BackGroundRenderMode = BackGround_RenderMode;
            this_BFI->bfi_Source->bfsir_OpenerCount = 0x01;

            NewList(&this_BFI->bfi_Source->bfsir_Buffers);

            AddTail(&image_backfill_images, &this_BFI->bfi_Source->bfsir_Node);
            goto check_imagebuffer;
          }
        }
        /* Failed to obtain datatype object's BM */
      }
      /* Failed to Layout datatype object */
    }
    /* Failed to open datatype object */
#if defined(DEBUG)
    if (!this_BFI->bfi_Source->bfsir_DTRastPort)
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Failed to create ImageSource RastPort\n"));
    }
#endif
    
    if (this_BFI->bfi_Source->bfsir_DTBitMap)
    {
      FreeBitMap(this_BFI->bfi_Source->bfsir_DTBitMap);
      this_BFI->bfi_Source->bfsir_DTBitMap = NULL;
    }
#if defined(DEBUG)
    else
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Failed to create ImageSource BitMap\n"));
    }
#endif

    if (this_BFI->bfi_Source->bfsir_DTPictureObject)
    {
      DisposeDTObject(this_BFI->bfi_Source->bfsir_DTPictureObject);
      this_BFI->bfi_Source->bfsir_DTPictureObject = NULL;
    }
#if defined(DEBUG)
    else
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Failed to create ImageSource Datatype Object\n"));
    }
#endif
    D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Failed to create ImageSource Record\n"));
    if (this_BFI->bfi_Source->bfsir_SourceImage) FreeVec(this_BFI->bfi_Source->bfsir_SourceImage);
    FreeMem(this_BFI->bfi_Source, sizeof(struct BackFillSourceImageRecord));
    this_BFI->bfi_Source = NULL;
    return FALSE;
  }
  else
  {
    D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Using existing ImageSource Record\n"));
    this_BFI->bfi_Source->bfsir_OpenerCount += 1;
  }

check_imagebuffer:

  Depth = GetBitMapAttr(this_BFI->bfi_Source->bfsir_DTBitMap, BMA_DEPTH);
  this_BFI->bfi_CopyWidth = this_BFI->bfi_Source->bfsir_DTBitMapHeader->bmh_Width;
  this_BFI->bfi_CopyHeight = this_BFI->bfi_Source->bfsir_DTBitMapHeader->bmh_Height;

  switch (BackGround_RenderMode)
  {
    case IconWindowExt_ImageBackFill_RenderMode_Scale:
    {
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED mode\n"));

      this_BFI->bfi_Options.bfo_TileMode = IconWindowExt_ImageBackFill_TileMode_Fixed;
      
      SET(_IconWindows_IconListObj, MUIA_IconListview_FixedBackground, TRUE);
      SET(_IconWindows_IconListObj, MUIA_IconListview_ScaledBackground, TRUE);

      if ((BOOL)XGET(self, MUIA_IconWindow_IsRoot))
      {
        struct BackFillSourceImageBuffer *this_Buffer;
        D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED - Root Window = TRUE!\n"));
        this_BFI->bfi_CopyWidth = GetBitMapAttr(this_BFI->bfi_Screen->RastPort.BitMap, BMA_WIDTH);
        this_BFI->bfi_CopyHeight = GetBitMapAttr(this_BFI->bfi_Screen->RastPort.BitMap, BMA_HEIGHT);

        D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED - Base Dimensions (%d x %d)\n", this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight));

        if (!((BOOL)XGET(self, MUIA_IconWindow_IsBackdrop)))
        {
          D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED - Adjusting for window border Dimensions ..\n"));
          D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED -      WBorTop %d, WBorLeft %d, WBorRight %d, WBorBottom %d\n", this_BFI->bfi_Screen->WBorTop, this_BFI->bfi_Screen->WBorLeft, this_BFI->bfi_Screen->WBorRight, this_BFI->bfi_Screen->WBorBottom));
          this_BFI->bfi_CopyWidth -= (this_BFI->bfi_Screen->WBorLeft + this_BFI->bfi_Screen->WBorRight);
          this_BFI->bfi_CopyHeight -= (this_BFI->bfi_Screen->WBorTop + this_BFI->bfi_Screen->WBorBottom);;
        }

        this_BFI->bfi_CopyHeight -= (this_BFI->bfi_Screen->BarHeight + 1);

        this_Buffer = NULL;

        if (this_BFI->bfi_Buffer)
        {
          if ((this_BFI->bfi_Buffer->bfsib_BitMapWidth == this_BFI->bfi_CopyWidth) && (this_BFI->bfi_Buffer->bfsib_BitMapHeight == this_BFI->bfi_CopyHeight))
          {
            goto pb_backfillsetup_complete;
          }
          else
          {
            ImageBackFill_CloseSourceBuffer(this_BFI->bfi_Source, this_BFI->bfi_Buffer);
            this_BFI->bfi_Buffer = NULL;
          }
        }
        
        if (!(this_Buffer = ImageBackFill_FindBufferRecord(this_BFI->bfi_Source, this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight)))
        {
          this_Buffer = AllocMem(sizeof(struct BackFillSourceImageBuffer), MEMF_CLEAR|MEMF_PUBLIC);
          this_Buffer->bfsib_BitMapWidth = this_BFI->bfi_CopyWidth;
          this_Buffer->bfsib_BitMapHeight = this_BFI->bfi_CopyHeight;

          if (!(this_Buffer->bfsib_BitMapRastPort = CreateRastPort()))
          {
            D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED - Failed to create RastPort for BackFill BitMap\n"));
            break;
          }

          if ((this_Buffer->bfsib_BitMap = AllocBitMap(this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight, Depth, Depth == 8 ? BMF_MINPLANES : 0L, this_BFI->bfi_Screen->RastPort.BitMap)))
          {
            struct Rectangle CopyBounds;

            D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED - Scale Dimensions (%d x %d)\n", this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight));

            
            this_Buffer->bfsib_BitMapRastPort->BitMap = this_Buffer->bfsib_BitMap;
            this_BFI->bfi_Buffer = this_Buffer;
            
            CopyBounds.MinX = 0;
            CopyBounds.MinY = 0;
            CopyBounds.MaxX = this_BFI->bfi_CopyWidth - 1;
            CopyBounds.MaxY = this_BFI->bfi_CopyHeight - 1;

            
            ImageBackFill_CopyScaledBitMap(this_BFI->bfi_Source->bfsir_DTRastPort,
                    0, 0, 
                    this_BFI->bfi_Source->bfsir_DTBitMapHeader->bmh_Width,
                    this_BFI->bfi_Source->bfsir_DTBitMapHeader->bmh_Height,
                    this_Buffer->bfsib_BitMapRastPort,
                    &CopyBounds, &CopyBounds, blit_MODE_Blit);

            this_Buffer->bfsib_OpenerCount = 0x01;

            AddTail(&this_BFI->bfi_Source->bfsir_Buffers, &this_Buffer->bfsib_Node);
            
            goto pb_backfillsetup_complete;
          }
          break;
        }
        else
        {
          this_BFI->bfi_Buffer = this_Buffer;
          this_Buffer->bfsib_OpenerCount += 1;
          goto pb_backfillsetup_complete;
        }
      }
      // We aren't the "ROOT" (desktop) window so only tile...
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: SCALED - Drawer window scaling unsupported ...\n"));
    }
    default:
    {
      struct BackFillSourceImageBuffer *this_Buffer;
      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: TILED mode\n"));
      if ((BackGround_TileMode = DoMethod(_IconWindows_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                                BackGround_Attrib, MUIA_IconWindowExt_ImageBackFill_BGTileMode)) == -1)
        BackGround_TileMode = IconWindowExt_ImageBackFill_TileMode_Float;

      if ((BackGround_XOffset = DoMethod(_IconWindows_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                                BackGround_Attrib, MUIA_IconWindowExt_ImageBackFill_BGXOffset)) == -1)
        BackGround_XOffset = 0;

      if ((BackGround_YOffset = DoMethod(_IconWindows_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                                BackGround_Attrib, MUIA_IconWindowExt_ImageBackFill_BGYOffset)) == -1)
        this_BFI->bfi_Options.bfo_OffsetY = 0;

      if (this_BFI->bfi_Options.bfo_TileMode != BackGround_TileMode)
      {
        this_BFI->bfi_Options.bfo_TileMode = BackGround_TileMode;
        options_changed = TRUE;
      }

      if (this_BFI->bfi_Options.bfo_OffsetX != BackGround_XOffset)
      {
        this_BFI->bfi_Options.bfo_OffsetX = BackGround_XOffset;
        options_changed = TRUE;
      }
      
      if (this_BFI->bfi_Options.bfo_OffsetY != BackGround_YOffset)
      {
        this_BFI->bfi_Options.bfo_OffsetY = BackGround_YOffset;
        options_changed = TRUE;
      }

      if (this_BFI->bfi_Options.bfo_TileMode == IconWindowExt_ImageBackFill_TileMode_Float)
        SET(_IconWindows_IconListObj, MUIA_IconListview_FixedBackground, FALSE);
      else
        SET(_IconWindows_IconListObj, MUIA_IconListview_FixedBackground, TRUE);

      SET(_IconWindows_IconListObj, MUIA_IconListview_ScaledBackground, FALSE);

      this_Buffer = NULL;

      if (this_BFI->bfi_Buffer)
      {
        if ((this_BFI->bfi_Buffer->bfsib_BitMapWidth == this_BFI->bfi_CopyWidth) && (this_BFI->bfi_Buffer->bfsib_BitMapHeight == this_BFI->bfi_CopyHeight))
        {
          goto pb_backfillsetup_complete;
        }
        else
        {
          ImageBackFill_CloseSourceBuffer(this_BFI->bfi_Source, this_BFI->bfi_Buffer);
          this_BFI->bfi_Buffer = NULL;
        }
      }

      D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Dimensions Width %d, Height %d\n", this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight));
      
      if (!(this_Buffer = ImageBackFill_FindBufferRecord(this_BFI->bfi_Source, this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight)))
      {
        this_Buffer = AllocMem(sizeof(struct BackFillSourceImageBuffer), MEMF_CLEAR|MEMF_PUBLIC);
        this_Buffer->bfsib_BitMapWidth = this_BFI->bfi_CopyWidth;
        this_Buffer->bfsib_BitMapHeight = this_BFI->bfi_CopyHeight;

        if ((this_Buffer->bfsib_BitMap = AllocBitMap(this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight, Depth, Depth == 8 ? BMF_MINPLANES : 0L, this_BFI->bfi_Screen->RastPort.BitMap)))
        {
          struct Rectangle CopyBounds;

          if (!(this_Buffer->bfsib_BitMapRastPort = CreateRastPort()))
          {
            D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: TILED - Failed to create RastPort for BackFill BitMap\n"));
            break;
          }

          this_Buffer->bfsib_BitMapRastPort->BitMap = this_Buffer->bfsib_BitMap;
          this_BFI->bfi_Buffer = this_Buffer;
          
          CopyBounds.MinX = 0;
          CopyBounds.MinY = 0;
          CopyBounds.MaxX = this_BFI->bfi_CopyWidth - 1;
          CopyBounds.MaxY = this_BFI->bfi_CopyHeight - 1;

          ImageBackFill_CopyTiledBitMap(this_BFI->bfi_Source->bfsir_DTRastPort,
                  0, 0, 
                  this_BFI->bfi_Source->bfsir_DTBitMapHeader->bmh_Width,
                  this_BFI->bfi_Source->bfsir_DTBitMapHeader->bmh_Height,
                  this_BFI->bfi_Buffer->bfsib_BitMapRastPort,
                  &CopyBounds, &CopyBounds, blit_MODE_Blit);

          this_Buffer->bfsib_OpenerCount = 0x01;
          
          AddTail(&this_BFI->bfi_Source->bfsir_Buffers, &this_BFI->bfi_Buffer->bfsib_Node);

          goto pb_backfillsetup_complete;
        }
        break;
      }
      else
      {
        this_BFI->bfi_Buffer = this_Buffer;
        this_Buffer->bfsib_OpenerCount += 1;
        goto pb_backfillsetup_complete;
      }
    }
  }

  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_ProcessBackground: Failed to create image datatype object\n"));  
  return FALSE;
  
pb_cleanup_buffer:
  if (this_BFI->bfi_Buffer)
  {
    ImageBackFill_CloseSourceBuffer(this_BFI->bfi_Source, this_BFI->bfi_Buffer);
    this_BFI->bfi_Buffer = NULL;
  }

  if (this_BFI->bfi_Source)
  { 
    ImageBackFill_CloseSourceRecord(this_BFI->bfi_Source);
    this_BFI->bfi_Source = NULL;
  }
  
  return FALSE;
  
pb_backfillsetup_complete:
  if ((prefs_processing) && (options_changed))
    SET(self, MUIA_IconWindow_Changed, TRUE);

  return TRUE;
}
///

///ImageBackFill__MUIM_IconWindow_BackFill_Setup()
IPTR ImageBackFill__MUIM_IconWindow_BackFill_Setup
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Setup *message
)
{
  struct BackFillInfo     *this_BFI = NULL;

  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_Setup()\n"));

  this_BFI = AllocVec(sizeof(struct BackFillInfo), MEMF_CLEAR|MEMF_PUBLIC);
  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_Setup: Allocated BackFillInfo @ %x\n", this_BFI));

  return (IPTR)this_BFI;
}
///

///ImageBackFill__MUIM_IconWindow_BackFill_Cleanup()
IPTR ImageBackFill__MUIM_IconWindow_BackFill_Cleanup
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Cleanup *message
)
{
  struct BackFillInfo   *this_BFI = (struct BackFillInfo   *)message->BackFill_Data;

D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_Cleanup()\n"));

  if (this_BFI->bfi_Buffer)
  {
    ImageBackFill_CloseSourceBuffer(this_BFI->bfi_Source, this_BFI->bfi_Buffer);
    this_BFI->bfi_Buffer = NULL;
  }
  if (this_BFI->bfi_Source)
  {
    ImageBackFill_CloseSourceRecord(this_BFI->bfi_Source);
    this_BFI->bfi_Source = NULL;
  }

  FreeVec(this_BFI);
  return TRUE;
}
///

///ImageBackFill__MUIM_IconWindow_BackFill_DrawBackground()
IPTR ImageBackFill__MUIM_IconWindow_BackFill_DrawBackground
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_DrawBackground *message
)
{
  struct BackFillInfo   *this_BFI = NULL;

D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_DrawBackground()\n"));
    
  if ((this_BFI = (struct BackFillInfo   *)message->BackFill_Data) != NULL)
  {
    D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_DrawBackground: Got BackFill_Data @ %x\n", this_BFI));
    this_BFI->bfi_RastPort = (struct RastPort *)message->draw_RastPort;

    if ((this_BFI->bfi_Buffer) && (this_BFI->bfi_RastPort))
    {

/* TODO: Make Base Tile Offset preference settable */
      WORD OffsetX = this_BFI->bfi_Options.bfo_OffsetX;         // the offset within the tile in x direction
      WORD OffsetY = this_BFI->bfi_Options.bfo_OffsetY;         // the offset within the tile in y direction

D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_DrawBackground: BackFill_Data has suitable Buffer and RastPort ..\n"));

      if (this_BFI->bfi_Options.bfo_TileMode == IconWindowExt_ImageBackFill_TileMode_Float)
      {
        D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_DrawBackground: Rendering using floating backdrop mode\n"));
        OffsetX += message->draw_BFM->OffsetX;
        OffsetY += message->draw_BFM->OffsetY;
      }

      ImageBackFill_CopyTiledBitMap(this_BFI->bfi_Buffer->bfsib_BitMapRastPort,
              OffsetX, OffsetY,
              this_BFI->bfi_CopyWidth, this_BFI->bfi_CopyHeight,
              this_BFI->bfi_RastPort,
              &message->draw_BFM->AreaBounds,
              &message->draw_BFM->DrawBounds,
              blit_MODE_Clip);

      return (IPTR)TRUE;
    }
  }
  D(bug("[IconWindow.ImageBackFill] MUIM_IconWindow_BackFill_DrawBackground: Causing parent to render .. \n"));
  return (IPTR)FALSE;
}
///

/*** SPECIAL : Early Setup ****************************************************************/
///ImageBackFill__SetupClass()
IPTR ImageBackFill__SetupClass()
{
  struct MUIP_IconWindow_BackFill_Register message;

  D(bug("[IconWindow.ImageBackFill] ImageBackFill__SetupClass()\n"));
  
  image_backfill_descriptor.bfd_BackFillID = image_backfill_name;

  image_backfill_descriptor.bfd_MUIM_IconWindow_BackFill_Setup = ImageBackFill__MUIM_IconWindow_BackFill_Setup;
  image_backfill_descriptor.bfd_MUIM_IconWindow_BackFill_Cleanup = ImageBackFill__MUIM_IconWindow_BackFill_Cleanup;
  image_backfill_descriptor.bfd_MUIM_IconWindow_BackFill_ProcessBackground = ImageBackFill__MUIM_IconWindow_BackFill_ProcessBackground;
  image_backfill_descriptor.bfd_MUIM_IconWindow_BackFill_DrawBackground = ImageBackFill__MUIM_IconWindow_BackFill_DrawBackground;

  message.register_Node = &image_backfill_descriptor;
  
  NewList(&image_backfill_images);

#if defined(WANDERER_MODULE_BACKFILL_ENABLED)
  IconWindow__MUIM_IconWindow_BackFill_Register(NULL, NULL, &message);
#endif
  
  return TRUE;
}
///

/* Make sure and run _AFTER_ IconWindow__SetupClass() */
ADD2INIT(ImageBackFill__SetupClass, 10);
