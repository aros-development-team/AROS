/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2005 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <string.h>

#include <exec/execbase.h>
#include <exec/memory.h>
#include <intuition/pointerclass.h>
#include <datatypes/pictureclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/layers.h>

#include "private.h"

#include "NList_func.h"

#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
LONG ObtainBestPen( struct ColorMap *cm, ULONG r, ULONG g, ULONG b, ULONG tag1Type, ... );
#endif

/****************************************************************************************/
/****************************************************************************************/
/******************************                    **************************************/
/******************************     NList Class    **************************************/
/******************************                    **************************************/
/****************************************************************************************/
/****************************************************************************************/

BOOL NL_OnWindow(struct NLData *data,LONG x,LONG y)
{
  BOOL onWindow = FALSE;
  Object *obj = data->this;

  #if defined(__MORPHOS__)
  /* No need for hit test in MorphOS 2.0 */
  if (SysBase->LibNode.lib_Version >= 51)
    return TRUE;
  #endif

  if (data->SHOW && muiRenderInfo(obj) && _screen(obj) != NULL && _window(obj) != NULL)
  {
    // first check whether the mouse is over the object itself
    if(x >= _mleft(obj) && x <= _mright(obj) && y >= _mtop(obj) && y <= _mbottom(obj))
    {
      // now check if the mouse is currently over the object and over the object's window
      struct Layer_Info *li = &(_screen(obj)->LayerInfo);
      struct Layer *layer;

      // get the layer that belongs to the current mouse coordinates
      LockLayerInfo(li);
      layer = WhichLayer(li, _window(obj)->LeftEdge + x, _window(obj)->TopEdge + y);
      UnlockLayerInfo(li);

      if(layer != NULL && layer->Window == _window(obj))
        onWindow = TRUE;
    }
/*
    struct Window *win;
    struct Layer *wlay,*lay;
    ULONG lkib = LockIBase( 0 );
    x += _window(obj)->LeftEdge;
    y += _window(obj)->TopEdge;
    wlay = _window(obj)->WLayer;
    for (lay = wlay->front; wlay && lay; lay = lay->front)
    { win = (struct Window *) lay->Window;
      for (win = _screen(obj)->FirstWindow; win ; win = win->NextWindow)
      { if (win->WLayer == lay)
        { if ((x >= win->LeftEdge) && (y >= win->TopEdge) &&
              (x < win->LeftEdge + win->Width) && (y < win->TopEdge + win->Height))
          { UnlockIBase( lkib );
            return (FALSE);
          }
        }
      }
    }
    UnlockIBase( lkib );
*/
  }
  return onWindow;
}




struct NImgList *GetNImage(struct NLData *data,char *ImgName)
{
  struct NImgList *nimg = &data->NImage;
  int nameLen = strlen(ImgName)+2;

  while (nimg && nimg->ImgName && strcmp(nimg->ImgName,ImgName))
    nimg = nimg->next;
  if (!nimg && (nimg = (struct NImgList *)AllocVecPooled(data->Pool, sizeof(struct NImgList)+nameLen)) != NULL)
  { nimg->ImgName = (char *) nimg;
    nimg->ImgName = &nimg->ImgName[sizeof(struct NImgList)];
    strlcpy(nimg->ImgName, ImgName, nameLen);
    nimg->NImgObj = NULL;
    nimg->width = -1;
    nimg->height = -1;
    nimg->dx = 0;
    nimg->dy = 0;
    nimg->next = data->NImage.next;
    data->NImage.next = nimg;
  }
  if (nimg && !nimg->NImgObj && data->adding_member && (nimg->width != -1000))
  { if ((data->adding_member == 1) && !data->NList_Quiet && !data->NList_Disabled && data->SETUP && data->SHOW && DoMethod(data->NL_Group,MUIM_Group_InitChange))
      data->adding_member = 2;
    if (data->adding_member >= 2)
    {
      nimg->NImgObj = MUI_NewObject(MUIC_Image,
                        MUIA_FillArea,FALSE,
                        MUIA_Image_Spec,nimg->ImgName,
/*
 *                         MUIA_InputMode,MUIV_InputMode_Toggle,
 *                         MUIA_Image_State, IDS_NORMAL,
 *                         MUIA_Selected, FALSE,
 *                         MUIA_Image_FontMatch, TRUE,
 *                         MUIA_Font, Topaz_8,
 */
                      TAG_DONE);
      if (nimg->NImgObj)
      {
/*D(bug("%lx|OM_ADDMEMBER,nimg->NImgObj %ld\n",obj,data->adding_member));*/
        DoMethod(data->NL_Group,OM_ADDMEMBER,nimg->NImgObj);
        set(nimg->NImgObj, MUIA_Disabled, FALSE);
        if (data->SHOW)
        {
          nimg->width = _defwidth(nimg->NImgObj);
          nimg->height = _defheight(nimg->NImgObj);
        }
        else
          data->do_images = TRUE;
      }
      else
      { nimg->width = -1000;
        nimg->height = 2;
      }
    }
    else
      data->do_images = TRUE;
  }
  return (nimg);
}


void DeleteNImages(struct NLData *data)
{
  struct NImgList *nimg = data->NImage.next;

  while (nimg)
  {
    data->NImage.next = nimg->next;
    FreeVecPooled(data->Pool, nimg);
    nimg = data->NImage.next;
  }
}


struct NImgList *GetNImage2(struct NLData *data,APTR imgobj)
{
  struct NImgList *nimg2 = data->NImage2;
  while (nimg2 && (nimg2->NImgObj != imgobj) && (nimg2->ImgName != imgobj))
    nimg2 = nimg2->next;
  if (!nimg2 && (nimg2 = (struct NImgList *)AllocVecPooled(data->Pool, sizeof(struct NImgList))) != NULL)
  { nimg2->NImgObj = NULL;
    nimg2->width = -1000;
    nimg2->height = 2;
    nimg2->dx = 0;
    nimg2->dy = 0;
    nimg2->ImgName = imgobj;
    nimg2->next = data->NImage2;
    data->NImage2 = nimg2;
  }
  if (nimg2 && nimg2->ImgName && !nimg2->NImgObj && (nimg2->width == -1000) && data->adding_member)
  {
    if(xget((Object *)nimg2->ImgName, MUIA_Parent))
    {
/*
 * D(bug("%lx|GetNImage2 %lx already attached !\n",obj,nimg2->ImgName));
 */
    }
    else
    { if ((data->adding_member == 1) && !data->NList_Quiet && !data->NList_Disabled && data->SETUP && data->SHOW && DoMethod(data->NL_Group,MUIM_Group_InitChange))
        data->adding_member = 2;
      if (data->adding_member >= 2)
      { nimg2->NImgObj = (APTR) nimg2->ImgName;
        nimg2->ImgName = NULL;
/*D(bug("%lx|OM_ADDMEMBER,nimg2->NImgObj %ld\n",obj,data->adding_member));*/
        DoMethod(data->NL_Group,OM_ADDMEMBER,nimg2->NImgObj);
        set(nimg2->NImgObj, MUIA_Disabled, FALSE);

        if (data->SHOW)
        { nimg2->width = _defwidth(nimg2->NImgObj);
          nimg2->height = _defheight(nimg2->NImgObj);
        }
        else
        { nimg2->width = -1;
          nimg2->height = -1;
          data->do_images = TRUE;
        }
      }
      else
        data->do_images = TRUE;
    }
  }
  return (nimg2);
}


void DeleteNImages2(struct NLData *data)
{
  struct NImgList *nimg2 = data->NImage2;

  while (nimg2)
  {
    data->NImage2 = nimg2->next;
    if (nimg2->ImgName && !nimg2->NImgObj)
      MUI_DisposeObject((APTR) nimg2->ImgName);
    FreeVecPooled(data->Pool, nimg2);
    nimg2 = data->NImage2;
  }
}


void GetNImage_Sizes(struct NLData *data)
{
  if (data->SHOW)
  {
    struct NImgList *nimg = &data->NImage;
    struct NImgList *nimg2 = data->NImage2;

    while (nimg)
    {
      if (nimg->NImgObj)
      {
        nimg->width = _defwidth(nimg->NImgObj);
        nimg->height = _defheight(nimg->NImgObj);
        if (data->MinImageHeight < _minheight(nimg->NImgObj))
        {
          if (_minheight(nimg->NImgObj) > 20)
          {
            if (data->MinImageHeight < 20)
              data->MinImageHeight = 20;
          }
          else
            data->MinImageHeight = _minheight(nimg->NImgObj);
        }
        _left(nimg->NImgObj) = _left(data->NL_Group);
        _top(nimg->NImgObj) = _top(data->NL_Group);
      }
      nimg = nimg->next;
    }

    while (nimg2)
    {
      if (nimg2->NImgObj && (nimg2->width != -1000))
      {
        nimg2->width = _defwidth(nimg2->NImgObj);
        nimg2->height = _defheight(nimg2->NImgObj);
        if (data->MinImageHeight < _minheight(nimg2->NImgObj))
        {
          if (_minheight(nimg2->NImgObj) > 30)
          {
            if (data->MinImageHeight < 30)
              data->MinImageHeight = 30;
          }
          else
            data->MinImageHeight = _minheight(nimg2->NImgObj);
        }
        _left(nimg2->NImgObj) = _left(data->NL_Group);
        _top(nimg2->NImgObj) = _top(data->NL_Group);
      }
      nimg2 = nimg2->next;
    }
  }
  else
    data->do_images = TRUE;
}


void GetNImage_End(struct NLData *data)
{
  if (data->adding_member == 2)
  {
    data->adding_member = 0;
/*D(bug("%lx|GNIE Group_ExitChange\n",obj));*/
    data->nodraw++;
    DoMethod(data->NL_Group,MUIM_Group_ExitChange);
    data->nodraw--;
    GetNImage_Sizes(data);
  }
  data->adding_member = 0;
}


void GetImages(struct NLData *data)
{
  if (data->SHOW && data->do_images)
  {
    struct NImgList *nimg = &data->NImage;
    struct NImgList *nimg2 = data->NImage2;

    if (data->adding_member != 2)
      data->adding_member = 1;

    while (nimg)
    {
      if (!nimg->NImgObj)
        GetNImage(data,nimg->ImgName);
      nimg = nimg->next;
    }

    while (nimg2)
    {
      if (nimg2->ImgName && !nimg2->NImgObj && (nimg2->width == -1000))
        GetNImage2(data,nimg2->ImgName);
      nimg2 = nimg2->next;
    }
    data->do_images = FALSE;
  }
  GetNImage_End(data);
}






static void disposeBitMap(struct NLData *data,struct BitMap *bitmap, LONG width, LONG height)
{
  if (bitmap)
  { WORD ktr;
    for (ktr = 0; ktr < bitmap->Depth; ktr++)
    { if (bitmap->Planes[ktr])
      {
/*D(bug("FreeRaster(%lx) (dispBM_planes)\n",bitmap->Planes[ktr]));*/
        FreeRaster(bitmap->Planes[ktr], width, height);
      }
    }
    FreeVecPooled(data->Pool, bitmap);
  }
}


static void disposeBitMapImage(struct NLData *data,struct BitMapImage *bmimg)
{
  if (bmimg && data->SETUP)
  {
    WORD ktr;

    for (ktr = 0; ktr < bmimg->imgbmp.Depth; ktr++)
    {
      if (bmimg->imgbmp.Planes[ktr])
      {
/*D(bug("FreeRaster(%lx) (dispBMI_planes)\n",bmimg->imgbmp.Planes[ktr]));*/
        FreeRaster(bmimg->imgbmp.Planes[ktr], bmimg->width, bmimg->height);
      }
    }
    if (bmimg->mask)
    {
/*D(bug("FreeRaster(%lx) (dispBMI_mask)\n",bmimg->mask));*/
      FreeRaster(bmimg->mask, bmimg->width, bmimg->height);
    }
    if (bmimg->obtainpens)
    {
      ktr = 0;
      while (bmimg->obtainpens[ktr] != -3)
      {
        if ((bmimg->obtainpens[ktr] >= 0) || (bmimg->obtainpens[ktr] < -3))
          ReleasePen(_screen(data->this)->ViewPort.ColorMap, (ULONG) bmimg->obtainpens[ktr]);
        ktr++;
      }
      FreeVecPooled(data->Pool, bmimg->obtainpens);
    }
    FreeVecPooled(data->Pool, bmimg);
  }
}


IPTR NL_CreateImage2(struct NLData *data,Object *imgobj,ULONG flags)
{
  struct BitMapImage *bmimg = NULL;

  if (imgobj)
  {
    if((bmimg = AllocVecPooled(data->Pool ,sizeof(struct BitMapImage))) != NULL)
    {
      if (GetNImage2(data,imgobj))
      {
        bmimg->control = MUIA_Image_Spec;
        bmimg->width = 0;
        bmimg->height = 0;
        bmimg->obtainpens = (APTR) imgobj;
        if (flags != (ULONG)~0L)
          nnset(imgobj,MUIA_FillArea,FALSE);
/*
 *         nnset(imgobj,MUIA_Image_FontMatch,FALSE);
 *         nnset(imgobj,MUIA_Image_FontMatchHeight,FALSE);
 *         nnset(imgobj,MUIA_Image_FontMatchWidth,FALSE);
 *         nnset(imgobj,MUIA_Image_State,IDS_SELECTED);
 *         nnset(imgobj,MUIA_Selected,FALSE);
 *         nnset(imgobj,MUIA_InputMode,MUIV_InputMode_Toggle);
 */
      }
      else
      {
        bmimg->control = 0L;
        FreeVecPooled(data->Pool, bmimg);
        bmimg = NULL;
      }
    }
    if (!bmimg)
      MUI_DisposeObject((APTR) imgobj);
  }
  return ((IPTR) bmimg);
}


IPTR NL_CreateImage(struct NLData *data,Object *imgobj,ULONG flags)
{
  SIPTR CI_BM_Width = 0;

  if (!imgobj)
    return(0);

  // check if the Bitmap_Width attribute doesn't exists
  if((flags == (ULONG)~0L) || GetAttr(MUIA_Bitmap_Width, imgobj, (IPTR *)&CI_BM_Width) == FALSE)
    return (NL_CreateImage2(data,imgobj,flags));

  if (imgobj && data->SETUP)
  {
    LONG last_numpen,last_newnumpen;
    struct BitMap *CI_BM_Bitmap = NULL;
    struct BitMap *bm_src = NULL;
    struct BitMapImage *bmimg = NULL;
    WORD  *obtainpens = NULL;
    UBYTE  newdepth = 0;
    UBYTE *CI_BC_Body = NULL;
    LONG   CI_BC_Depth = 0;
    LONG   CI_BC_Compression = 0;
    LONG   CI_BC_Masking = 0;
    LONG   CI_BM_Height = 0;
    UBYTE *CI_BM_MappingTable = NULL;
    ULONG *CI_BM_SourceColors = NULL;
    LONG   CI_BM_Precision = 0;
    LONG   CI_BM_Transparent = 0;

    CI_BM_Bitmap = (struct BitMap *)xget(imgobj, MUIA_Bitmap_Bitmap);
    CI_BM_Width = xget(imgobj, MUIA_Bitmap_Width);
    CI_BM_Height = xget(imgobj, MUIA_Bitmap_Height);
    CI_BM_MappingTable = (UBYTE *)xget(imgobj, MUIA_Bitmap_MappingTable);
    CI_BM_Transparent = xget(imgobj, MUIA_Bitmap_Transparent);
    CI_BM_SourceColors = (ULONG *)xget(imgobj, MUIA_Bitmap_SourceColors);
    CI_BM_Precision = xget(imgobj, MUIA_Bitmap_Precision);

    if (!CI_BM_Bitmap)
    {
      CI_BC_Body = (UBYTE *)xget(imgobj, MUIA_Bodychunk_Body);
      CI_BC_Depth = xget(imgobj, MUIA_Bodychunk_Depth);
      CI_BC_Compression = xget(imgobj, MUIA_Bodychunk_Compression);
      CI_BC_Masking = xget(imgobj, MUIA_Bodychunk_Masking);

      if (CI_BC_Body)
      {
        if((bm_src = AllocVecPooled(data->Pool, sizeof(struct BitMap))) != NULL)
        {
          WORD ktr;
          BOOL bit_map_failed = FALSE;

          InitBitMap(bm_src,CI_BC_Depth,CI_BM_Width,CI_BM_Height);

          for (ktr = 0; ktr < bm_src->Depth; ktr++)
          {
            if (!(bm_src->Planes[ktr] = (PLANEPTR) AllocRaster(CI_BM_Width,CI_BM_Height)))
              bit_map_failed = TRUE;
          }
          if (bit_map_failed)
          {
            disposeBitMap(data,bm_src,CI_BM_Width,CI_BM_Height);
            bm_src = NULL;
          }
          else
          {
            WORD cptb,cptr,cptd,offr;
            WORD num = 0;
            UBYTE val = 0;
            UBYTE type = 0;
            UBYTE *body = CI_BC_Body;

            for (cptr = 0; cptr < bm_src->Rows; cptr++)
            {
              offr = cptr * bm_src->BytesPerRow;
              for (cptd = 0; cptd < bm_src->Depth; cptd++)
              {
                for (cptb = 0; cptb < bm_src->BytesPerRow; cptb++)
                {
                  if (CI_BC_Compression == cmpByteRun1)
                  {
                    if (num > 0)
                    {
                      if (type == 0)
                        bm_src->Planes[cptd][offr+cptb] = val;
                      else
                        bm_src->Planes[cptd][offr+cptb] = *body;
                      num--;
                    }
                    else
                    {
                      while (*body == 128)
                        body++;
                      if (*body < 128)
                      {
                        num = (WORD) *body; type = 1; body++;
                        bm_src->Planes[cptd][offr+cptb] = *body;
                      }
                      else
                      {
                        num = 256 - ((WORD) *body); type = 0; body++; val = *body;
                        bm_src->Planes[cptd][offr+cptb] = val;
                      }
                    }
                  }
                  else
                    bm_src->Planes[cptd][offr+cptb] = *body;
                  body++;
                }
              }
              if (CI_BC_Masking == mskHasMask)
              {
                for (cptb = 0; cptb < bm_src->BytesPerRow; cptb++)
                {
                  if (CI_BC_Compression == cmpByteRun1)
                  {
                    if (num > 0)
                      num--;
                    else
                    {
                      while (*body == 128)
                        body++;
                      if (*body < 128)
                      {
                        num = (WORD) *body;
                        type = 1;
                        body++;
                      }
                      else
                      {
                        num = 256 - ((WORD) *body);
                        type = 0;
                        body++;
                        val = *body;
                      }
                    }
                  }
                  body++;
                }
              }
            }
          }
        }
      }
      else
        bm_src = NULL;
    }
    else
      bm_src = CI_BM_Bitmap;

    if (bm_src != NULL)
    {
      UBYTE mypen,bit1,bit2;
      WORD cptb,cptr,cptd,offr,offr1,cptpen;

      last_numpen = (1 << bm_src->Depth);
      last_newnumpen = 0;
      newdepth = bm_src->Depth;

      if (CI_BM_MappingTable != NULL)
      {
        LONG num;

        for (num = 0;num < last_numpen;num++)
        {
          if (last_newnumpen < CI_BM_MappingTable[num])
            last_newnumpen = CI_BM_MappingTable[num];
        }
        newdepth = 0;
        while (last_newnumpen)
        {
          newdepth++;
          last_newnumpen = last_newnumpen >> 1;
        }
      }
      else if (CI_BM_SourceColors != NULL)
      {
        ULONG *mycolor;

        obtainpens = AllocVecPooled(data->Pool, (last_numpen+1)*sizeof(*obtainpens));
        if (obtainpens != NULL)
        {
          obtainpens[last_numpen] = -3;
          for (cptpen = 0; cptpen < last_numpen; cptpen++)
            obtainpens[cptpen] = -1;
          for (cptr = 0; cptr < bm_src->Rows; cptr++)
          {
            offr = cptr * bm_src->BytesPerRow;
            for (cptb = 0; cptb < bm_src->BytesPerRow; cptb++)
            {
              bit1 = 0x80;
              while (bit1)
              { /* get the source dot pen number */
                mypen = 0;
                bit2 = 0x01;
                for (cptd = 0; cptd < bm_src->Depth; cptd++)
                {
                  if (bm_src->Planes[cptd][offr+cptb] & bit1)
                    mypen |= bit2;

                  bit2 = bit2 << 1;
                }
                /* map the pen number */
                if (mypen == CI_BM_Transparent)
                  obtainpens[mypen] = -2;
                else if (obtainpens[mypen] == -1)
                {
                  mycolor = &CI_BM_SourceColors[mypen*3];

                  obtainpens[mypen] = (WORD) ObtainBestPen(_screen(data->this)->ViewPort.ColorMap,
                                                           mycolor[0],mycolor[1],mycolor[2],
                                                           OBP_Precision, CI_BM_Precision,
                                                           TAG_END);
                  if (obtainpens[mypen] == -1)
                  {
                    obtainpens[mypen] = (WORD) ObtainBestPen(_screen(data->this)->ViewPort.ColorMap,
                                                             mycolor[0],mycolor[1],mycolor[2],
                                                             OBP_Precision, PRECISION_GUI,
                                                             TAG_END);
                  }
                  if(last_newnumpen < obtainpens[mypen])
                    last_newnumpen = obtainpens[mypen];
                }
                bit1 = bit1 >> 1;
              }
            }
          }
          newdepth = 0;
          while (last_newnumpen)
          {
            newdepth++;
            last_newnumpen = last_newnumpen >> 1;
          }
        }
      }
      if (newdepth > 8)
        newdepth = 8;
      last_newnumpen = 1 << newdepth;

      if ((newdepth > 0) && (bmimg = AllocVecPooled(data->Pool, sizeof(struct BitMapImage))) != NULL)
      {
        WORD ktr;
        BOOL bmimg_failed = FALSE;

        InitBitMap(&(bmimg->imgbmp),newdepth,CI_BM_Width,CI_BM_Height);
        bmimg->control = MUIM_NList_CreateImage;
        bmimg->width = CI_BM_Width;
        bmimg->height = CI_BM_Height;
        bmimg->obtainpens = obtainpens;
        for (ktr = 0; ktr < bmimg->imgbmp.Depth; ktr++)
        {
          if (!(bmimg->imgbmp.Planes[ktr] = (PLANEPTR) AllocRaster(CI_BM_Width,CI_BM_Height)))
            bmimg_failed = TRUE;
        }
        if (!(bmimg->mask = (PLANEPTR) AllocRaster(CI_BM_Width,CI_BM_Height)))
          bmimg_failed = TRUE;
        if (bmimg_failed)
        {
          disposeBitMapImage(data,bmimg);
          bmimg = NULL;
        }
        else
        {
/*
int kprintf( const char *, ... );
kprintf( "SrcBitMap Width %ld Height %ld Depth %ld BytesPerRow %ld Rows %ld Depth %ld.\n",
GetBitMapAttr( bm_src, BMA_WIDTH ),
GetBitMapAttr( bm_src, BMA_HEIGHT ),
GetBitMapAttr( bm_src, BMA_DEPTH ),
bm_src->BytesPerRow,
bm_src->Rows,
bm_src->Depth );
kprintf( "BytesPerRow %ld Rows %ld.\n", bmimg->imgbmp.BytesPerRow, bmimg->imgbmp.Rows );
*/
          for (cptr = 0; cptr < bmimg->imgbmp.Rows; cptr++)
          {
            offr = cptr * bmimg->imgbmp.BytesPerRow;
            offr1 = cptr * bm_src->BytesPerRow;
            for (cptb = 0; cptb < bmimg->imgbmp.BytesPerRow; cptb++)
            {
              bit1 = 0x80;
              while (bit1)
              { /* get the source dot pen number */
                mypen = 0;
                bit2 = 0x01;
                for (cptd = 0; cptd < bm_src->Depth; cptd++)
                {
                  if (bm_src->Planes[cptd][offr1+cptb] & bit1) // here was bug!!!
                    mypen |= bit2;
                  bit2 = bit2 << 1;
                }
                /* map the pen number */
                if (mypen == CI_BM_Transparent)
                {
                  bmimg->mask[offr+cptb] &= ~bit1;
                  mypen = 0;
                }
                else
                {
                  bmimg->mask[offr+cptb] |= bit1;
                  if (CI_BM_MappingTable)
                    mypen = (UBYTE) CI_BM_MappingTable[mypen];
                  else if (CI_BM_SourceColors && obtainpens && ((obtainpens[mypen] >= 0) || (obtainpens[mypen] < -3)))
                    mypen = (UBYTE) obtainpens[mypen];
                }
                /* set the dest dot pen number */
                bit2 = 0x01;
                for (cptd = 0; cptd < bmimg->imgbmp.Depth; cptd++)
                {
                  if (mypen & bit2)
                    bmimg->imgbmp.Planes[cptd][offr+cptb] |= bit1;
                  else
                    bmimg->imgbmp.Planes[cptd][offr+cptb] &= ~bit1;
                  bit2 = bit2 << 1;
                }
                bit1 = bit1 >> 1;
              }
            }
          }
        }
      }
    }

    if (!bmimg && obtainpens)
      FreeVecPooled(data->Pool, obtainpens);

    if (!CI_BM_Bitmap)
    {
      disposeBitMap(data,bm_src,CI_BM_Width,CI_BM_Height);
      bm_src = NULL;
    }
    if (bmimg && (data->MinImageHeight < bmimg->height) && !flags)
      data->MinImageHeight = bmimg->height;

    return ((IPTR) bmimg);
  }

  return (0);
}


ULONG NL_DeleteImage(struct NLData *data,APTR listimg)
{
  struct BitMapImage *bmimg = (struct BitMapImage *) listimg;

  if (bmimg && (bmimg->control == MUIM_NList_CreateImage))
  {
    bmimg->control = 0L;
    disposeBitMapImage(data,bmimg);
  }
  else if (bmimg && (bmimg->control == MUIA_Image_Spec))
  {
    APTR imgobj = (APTR) bmimg->obtainpens;
    struct NImgList *nimg2 = data->NImage2,*nimgprec = NULL;

    while (nimg2 && (nimg2->NImgObj != imgobj) && (nimg2->ImgName != imgobj))
    {
      nimgprec = nimg2;
      nimg2 = nimg2->next;
    }
    bmimg->control = 0L;
    if (nimg2)
    {
      if (nimgprec)
        nimgprec->next = nimg2->next;
      else
        data->NImage2 = nimg2->next;
      if (nimg2->NImgObj == imgobj)
      {
        Object *parent;

        if((parent = (Object *)xget(imgobj, MUIA_Parent)) && (parent == data->NL_Group))
        {
          if (data->SETUP && DoMethod(data->NL_Group,MUIM_Group_InitChange))
          {
            DoMethod(data->NL_Group,OM_REMMEMBER,imgobj);
            DoMethod(data->NL_Group,MUIM_Group_ExitChange);
          }
          else
            DoMethod(data->NL_Group,OM_REMMEMBER,imgobj);
        }
      }
      FreeVecPooled(data->Pool, nimg2);
    }
    FreeVecPooled(data->Pool, bmimg);
  }
  return (TRUE);
}


ULONG NL_CreateImages(struct NLData *data)
{
  if (data->NList_UseImages && data->SETUP)
  {
    LONG pos = 0;

    while (pos < data->LastImage)
    {
      if (data->NList_UseImages[pos].imgobj)
        data->NList_UseImages[pos].bmimg = (struct BitMapImage *)
                                NL_CreateImage(data,data->NList_UseImages[pos].imgobj,data->NList_UseImages[pos].flags);
      else
        data->NList_UseImages[pos].bmimg = NULL;
      pos++;
    }
  }

  return(0);
}


ULONG NL_DeleteImages(struct NLData *data)
{
  if (data->NList_UseImages)
  {
    LONG pos = 0;

    while (pos < data->LastImage)
    {
      if (data->NList_UseImages[pos].bmimg)
        NL_DeleteImage(data,(APTR) data->NList_UseImages[pos].bmimg);
      data->NList_UseImages[pos].bmimg = NULL;
      pos++;
    }
  }

  return(0);
}


ULONG NL_UseImage(struct NLData *data,Object *imgobj,LONG imgnum,ULONG flags)
{
  BOOL redraw = FALSE;
  LONG retval = FALSE;

  if ((imgnum >= 0) && (imgnum < 8192))
  {
    if (imgobj)
    {
      if (!data->NList_UseImages || (imgnum >= data->LastImage))
      {
        LONG last = imgnum + 8;
        struct UseImage *useimages = (struct UseImage *)AllocVecPooled(data->Pool, (last+1)*sizeof(struct UseImage));

        if (useimages)
        {
          LONG pos = 0;

          while (pos < data->LastImage)
          { useimages[pos].bmimg = data->NList_UseImages[pos].bmimg;
            useimages[pos].imgobj = data->NList_UseImages[pos].imgobj;
            useimages[pos].flags = data->NList_UseImages[pos].flags;
            pos++;
          }

          while (pos < last)
          {
            useimages[pos].bmimg = 0;
            useimages[pos].imgobj = NULL;
            useimages[pos].flags = 0;
            pos++;
          }
          if (data->NList_UseImages)
            FreeVecPooled(data->Pool, data->NList_UseImages);
          data->NList_UseImages = useimages;
          data->LastImage = last;
        }
        else if (!data->NList_UseImages)
          data->LastImage = 0;
      }
      if (imgnum < data->LastImage)
      {
        if (data->NList_UseImages[imgnum].bmimg)
        {
          NL_DeleteImage(data,(APTR) data->NList_UseImages[imgnum].bmimg);
          redraw = TRUE;
        }

        data->NList_UseImages[imgnum].bmimg = NULL;
        data->NList_UseImages[imgnum].imgobj = imgobj;
        data->NList_UseImages[imgnum].flags = flags;
        if (data->SETUP)
        {
          data->NList_UseImages[imgnum].bmimg = (struct BitMapImage *)
                                NL_CreateImage(data,data->NList_UseImages[imgnum].imgobj,data->NList_UseImages[imgnum].flags);
          redraw = TRUE;
        }
        retval = TRUE;
      }
    }
    else if (imgnum < data->LastImage)
    {
      if (data->NList_UseImages[imgnum].bmimg)
      {
        NL_DeleteImage(data,(APTR) data->NList_UseImages[imgnum].bmimg);
        redraw = TRUE;
      }
      data->NList_UseImages[imgnum].bmimg = NULL;
      data->NList_UseImages[imgnum].imgobj = NULL;
      data->NList_UseImages[imgnum].flags = 0;
      retval = TRUE;
    }
  }
  else if (!imgobj && (imgnum == MUIV_NList_UseImage_All) && data->NList_UseImages)
  {
    LONG pos = 0;

    while (pos < data->LastImage)
    {
      if (data->NList_UseImages[pos].bmimg)
      {
        NL_DeleteImage(data,(APTR) data->NList_UseImages[pos].bmimg);
        redraw = TRUE;
      }
      data->NList_UseImages[pos].bmimg = NULL;
      data->NList_UseImages[pos].imgobj = NULL;
      data->NList_UseImages[pos].flags = 0;
      pos++;
    }
    retval = TRUE;
  }
  if (redraw)
  {
    data->do_draw_all = data->do_draw_title = data->do_draw = TRUE;
    data->do_parse = data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
  }
  return ((ULONG)retval);
}


IPTR mNL_CreateImage(struct IClass *cl,Object *obj,struct MUIP_NList_CreateImage *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_CreateImage(data,msg->obj,msg->flags));
}


IPTR mNL_DeleteImage(struct IClass *cl,Object *obj,struct MUIP_NList_DeleteImage *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_DeleteImage(data,msg->listimg));
}


IPTR mNL_UseImage(struct IClass *cl,Object *obj,struct MUIP_NList_UseImage *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  return (NL_UseImage(data,msg->obj,msg->imgnum,msg->flags));
}
