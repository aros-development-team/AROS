/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007-2013 by NList Open Source Team

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

// ansi includes
#include <string.h>

// system includes
#include <proto/exec.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <mui/NBitmap_mcc.h>

// system
#include <datatypes/pictureclass.h>

#if defined(__amigaos4__)
#include <hardware/blit.h>
#else
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#endif

#if defined(__MORPHOS__)
#include <exec/execbase.h>
#include <exec/system.h>
#endif

// libraries
#include <libraries/mui.h>

// local includes
#include "private.h"
#include "NBitmap.h"
#include "Chunky2Bitmap.h"
#include "DitherImage.h"
#include "version.h"
#include "Debug.h"

// constant data
/// default color map
const uint32 defaultColorMap[256] =
{
  0x00000000, 0x00000055, 0x000000aa, 0x000000ff, 0x00002400, 0x00002455, 0x000024aa, 0x000024ff, 0x00004900, 0x00004955, 0x000049aa, 0x000049ff, 0x00006d00, 0x00006d55, 0x00006daa, 0x00006dff,
  0x00009200, 0x00009255, 0x000092aa, 0x000092ff, 0x0000b600, 0x0000b655, 0x0000b6aa, 0x0000b6ff, 0x0000db00, 0x0000db55, 0x0000dbaa, 0x0000dbff, 0x0000ff00, 0x0000ff55, 0x0000ffaa, 0x0000ffff,
  0x00240000, 0x00240055, 0x002400aa, 0x002400ff, 0x00242400, 0x00242455, 0x002424aa, 0x002424ff, 0x00244900, 0x00244955, 0x002449aa, 0x002449ff, 0x00246d00, 0x00246d55, 0x00246daa, 0x00246dff,
  0x00249200, 0x00249255, 0x002492aa, 0x002492ff, 0x0024b600, 0x0024b655, 0x0024b6aa, 0x0024b6ff, 0x0024db00, 0x0024db55, 0x0024dbaa, 0x0024dbff, 0x0024ff00, 0x0024ff55, 0x0024ffaa, 0x0024ffff,
  0x00490000, 0x00490055, 0x004900aa, 0x004900ff, 0x00492400, 0x00492455, 0x004924aa, 0x004924ff, 0x00494900, 0x00494955, 0x004949aa, 0x004949ff, 0x00496d00, 0x00496d55, 0x00496daa, 0x00496dff,
  0x00499200, 0x00499255, 0x004992aa, 0x004992ff, 0x0049b600, 0x0049b655, 0x0049b6aa, 0x0049b6ff, 0x0049db00, 0x0049db55, 0x0049dbaa, 0x0049dbff, 0x0049ff00, 0x0049ff55, 0x0049ffaa, 0x0049ffff,
  0x006d0000, 0x006d0055, 0x006d00aa, 0x006d00ff, 0x006d2400, 0x006d2455, 0x006d24aa, 0x006d24ff, 0x006d4900, 0x006d4955, 0x006d49aa, 0x006d49ff, 0x006d6d00, 0x006d6d55, 0x006d6daa, 0x006d6dff,
  0x006d9200, 0x006d9255, 0x006d92aa, 0x006d92ff, 0x006db600, 0x006db655, 0x006db6aa, 0x006db6ff, 0x006ddb00, 0x006ddb55, 0x006ddbaa, 0x006ddbff, 0x006dff00, 0x006dff55, 0x006dffaa, 0x006dffff,
  0x00920000, 0x00920055, 0x009200aa, 0x009200ff, 0x00922400, 0x00922455, 0x009224aa, 0x009224ff, 0x00924900, 0x00924955, 0x009249aa, 0x009249ff, 0x00926d00, 0x00926d55, 0x00926daa, 0x00926dff,
  0x00929200, 0x00929255, 0x009292aa, 0x009292ff, 0x0092b600, 0x0092b655, 0x0092b6aa, 0x0092b6ff, 0x0092db00, 0x0092db55, 0x0092dbaa, 0x0092dbff, 0x0092ff00, 0x0092ff55, 0x0092ffaa, 0x0092ffff,
  0x00b60000, 0x00b60055, 0x00b600aa, 0x00b600ff, 0x00b62400, 0x00b62455, 0x00b624aa, 0x00b624ff, 0x00b64900, 0x00b64955, 0x00b649aa, 0x00b649ff, 0x00b66d00, 0x00b66d55, 0x00b66daa, 0x00b66dff,
  0x00b69200, 0x00b69255, 0x00b692aa, 0x00b692ff, 0x00b6b600, 0x00b6b655, 0x00b6b6aa, 0x00b6b6ff, 0x00b6db00, 0x00b6db55, 0x00b6dbaa, 0x00b6dbff, 0x00b6ff00, 0x00b6ff55, 0x00b6ffaa, 0x00b6ffff,
  0x00db0000, 0x00db0055, 0x00db00aa, 0x00db00ff, 0x00db2400, 0x00db2455, 0x00db24aa, 0x00db24ff, 0x00db4900, 0x00db4955, 0x00db49aa, 0x00db49ff, 0x00db6d00, 0x00db6d55, 0x00db6daa, 0x00db6dff,
  0x00db9200, 0x00db9255, 0x00db92aa, 0x00db92ff, 0x00dbb600, 0x00dbb655, 0x00dbb6aa, 0x00dbb6ff, 0x00dbdb00, 0x00dbdb55, 0x00dbdbaa, 0x00dbdbff, 0x00dbff00, 0x00dbff55, 0x00dbffaa, 0x00dbffff,
  0x00ff0000, 0x00ff0055, 0x00ff00aa, 0x00ff00ff, 0x00ff2400, 0x00ff2455, 0x00ff24aa, 0x00ff24ff, 0x00ff4900, 0x00ff4955, 0x00ff49aa, 0x00ff49ff, 0x00ff6d00, 0x00ff6d55, 0x00ff6daa, 0x00ff6dff,
  0x00ff9200, 0x00ff9255, 0x00ff92aa, 0x00ff92ff, 0x00ffb600, 0x00ffb655, 0x00ffb6aa, 0x00ffb6ff, 0x00ffdb00, 0x00ffdb55, 0x00ffdbaa, 0x00ffdbff, 0x00ffff00, 0x00ffff55, 0x00ffffaa, 0x00ffffff
};

///

#if defined(__MORPHOS__) || defined(__AROS__)
// MorphOS and AROS always have working WPA() and WPAA() functions
#define WPA(src, srcx, srcy, srcmod, rp, destx, desty, width, height, fmt) \
  WritePixelArray(src, srcx, srcy, srcmod, rp, destx, desty, width, height, fmt)
#define WPAA(src, srcx, srcy, srcmod, rp, destx, desty, width, height, globalalpha) \
  WritePixelArrayAlpha(src, srcx, srcy, srcmod, rp, destx, desty, width, height, globalalpha)
#elif !defined(__amigaos4__)
// for AmigaOS3 this is only true for CGX V43+
#define WPA(src, srcx, srcy, srcmod, rp, destx, desty, width, height, fmt) \
{ \
  if(CyberGfxBase != NULL) \
    WritePixelArray(src, srcx, srcy, srcmod, rp, destx, desty, width, height, fmt); \
  else \
    _WPA(src, srcx, srcy, srcmod, rp, destx, desty, width, height, fmt); \
}
#define WPAA(src, srcx, srcy, srcmod, rp, destx, desty, width, height, globalalpha) \
{ \
  if(CyberGfxBase != NULL && CyberGfxBase->lib_Version >= 43) \
    WritePixelArrayAlpha(src, srcx, srcy, srcmod, rp, destx, desty, width, height, globalalpha); \
  else \
    _WPAA(src, srcx, srcy, srcmod, rp, destx, desty, width, height, globalalpha); \
}
#endif

// functions
/// GetConfigItem()
//
ULONG GetConfigItem(Object *obj, ULONG configitem, ULONG defaultsetting)
{
  IPTR value;
  ULONG result = defaultsetting;

  ENTER();

  if(DoMethod(obj, MUIM_GetConfigItem, configitem, &value))
    result = *(ULONG *)value;

  /* XXX: On 64-bit AROS I'm getting for the line above the warning "cast to pointer from integer of different size". */

  RETURN(result);
  return result;
}

///
/// InitConfig()
//
static void InitConfig(Object *obj, struct InstData *data)
{
  ENTER();

  if(obj != NULL && data != NULL)
  {
    data->prefs.show_label = 0;
    data->prefs.overlay_type = 0;
    data->prefs.overlay_r = 10;
    data->prefs.overlay_g = 36;
    data->prefs.overlay_b = 106;
    data->prefs.overlay_shadeover = 1500; // = 1.5 if divided by 1000
    data->prefs.overlay_shadepress = 2500; // = 2.5 if divided by 1000
    data->prefs.spacing_horiz = 2;
    data->prefs.spacing_vert = 2;
  }

  LEAVE();
}

///
/// FreeConfig()
//
static void FreeConfig(struct InstData *data)
{
  ENTER();

  if(data != NULL)
  {
    // nothing yet
  }

  LEAVE();
}

///
/// NBitmap_LoadImage()
//
BOOL NBitmap_LoadImage(STRPTR filename, uint32 item, struct IClass *cl, Object *obj)
{
  BOOL result = FALSE;
  struct InstData *data;

  ENTER();

  SHOWSTRING(DBF_DATATYPE, filename);

  if((data = INST_DATA(cl, obj)) != NULL && filename != NULL)
  {
    data->dt_obj[item] = NewDTObject(filename,
                                     DTA_GroupID,           GID_PICTURE,
                                     OBP_Precision,         PRECISION_EXACT,
                                     PDTA_FreeSourceBitMap, TRUE,
                                     PDTA_DestMode,         PMODE_V43,
                                     PDTA_UseFriendBitMap,  TRUE,
                                     TAG_DONE);
    SHOWVALUE(DBF_DATATYPE, data->dt_obj[item]);
    if(data->dt_obj[item] != NULL)
      result = TRUE;
  }

  RETURN(result);
  return result;
}

///
/// NBitmap_ExamineData()
//
static BOOL NBitmap_ExamineData(Object *dt_obj, uint32 item, struct IClass *cl, Object *obj)
{
  BOOL result = FALSE;
  ULONG arraysize;

  struct pdtBlitPixelArray pbpa;
  struct InstData *data = INST_DATA(cl, obj);

  if(dt_obj != NULL)
  {
    /* bitmap header */
    GetDTAttrs(dt_obj, PDTA_BitMapHeader, &data->dt_header[item], TAG_DONE);
    D(DBF_DATATYPE, "examine: BMHD dimensions %ldx%ldx%ld", data->dt_header[item]->bmh_Width, data->dt_header[item]->bmh_Height, data->dt_header[item]->bmh_Depth);
    data->depth = data->dt_header[item]->bmh_Depth;

    if(data->depth>0 && data->depth<=8)
    {
      /* colour lookup bitmap */
      data->fmt = PBPAFMT_LUT8;

      /* bitmap header */
      data->width =  data->dt_header[0]->bmh_Width;
      data->height =  data->dt_header[0]->bmh_Height;

      result = TRUE;
      D(DBF_DATATYPE, "examine: using LUT8 bitmaps");
    }
    else if(data->depth >=24)
    {
      #if defined(__MORPHOS__)
      /* XXX: Check out is this needed in OS 3 and AROS */
      IPTR use_alpha;

      GetDTAttrs(dt_obj, PDTA_AlphaChannel, (IPTR)&use_alpha, TAG_DONE);

      if (use_alpha)
        data->depth = 32;
      #endif

      /* true colour bitmap */
      if(data->depth == 24)
      {
        data->fmt = PBPAFMT_RGB;
        D(DBF_DATATYPE, "examine: using 24bit RGB data");
      }
      else if(data->depth == 32)
      {
        data->fmt = PBPAFMT_ARGB;
        D(DBF_DATATYPE, "examine: using 32bit ARGB data");
      }

      data->width =  data->dt_header[0]->bmh_Width;
      data->height =  data->dt_header[0]->bmh_Height;
      data->arraybpp = data->depth/8;
      data->arraybpr = data->arraybpp * data->width;

      #if defined(__MORPHOS__)
      if (SysBase->LibNode.lib_Version >= 51)
      {
        ULONG altivec_align = 0;

        NewGetSystemAttrs(&altivec_align, sizeof(altivec_align), SYSTEMINFOTYPE_PPC_ALTIVEC, TAG_DONE);

        if (altivec_align)
          data->arraybpr = (data->arraybpr + 15) & ~15;
      }
      #endif

      arraysize = (data->arraybpr) * data->height;

      /* get array of pixels */
      if((data->arraypixels[item] = AllocVecShared(arraysize, MEMF_ANY|MEMF_CLEAR)) != NULL)
      {
        ULONG error;

        memset(&pbpa, 0, sizeof(struct pdtBlitPixelArray));

        pbpa.MethodID = PDTM_READPIXELARRAY;
        pbpa.pbpa_PixelData = data->arraypixels[item];
        pbpa.pbpa_PixelFormat = data->fmt;
        pbpa.pbpa_PixelArrayMod = data->arraybpr;
        pbpa.pbpa_Left = 0;
        pbpa.pbpa_Top = 0;
        pbpa.pbpa_Width = data->width;
        pbpa.pbpa_Height = data->height;

        error = DoMethodA(dt_obj, (Msg)(VOID*)&pbpa);
        (void)error;
        D(DBF_DATATYPE, "examine: READPIXELARRAY returned %ld", error);

        result = TRUE;
      }
    }
  }

  return(result);
}

///
/// NBitmap_UpdateImage()
//
VOID NBitmap_UpdateImage(uint32 item, STRPTR filename, struct IClass *cl, Object *obj)
{
  struct InstData *data = NULL;

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    if(filename != NULL)
    {
      if(data->dt_obj[item] != NULL)
      {
        /* free old image data */
        if(data->fmt == PBPAFMT_LUT8)
          SetDTAttrs(data->dt_obj[item], NULL, NULL, PDTA_Screen, NULL, TAG_DONE);

        DisposeDTObject(data->dt_obj[item]);
        data->dt_obj[item] = NULL;

        if(data->arraypixels[item] != NULL)
        {
          FreeVec(data->arraypixels[item]);
          data->arraypixels[item] = NULL;
        }

        /* load new image */
        if((NBitmap_LoadImage(filename, item, cl, obj)) != FALSE)
        {
          /* setup new image */
          if((NBitmap_ExamineData(data->dt_obj[item], item, cl, obj)) != FALSE)
          {
            if(data->fmt == PBPAFMT_LUT8)
            {
              /* layout image */
              SetDTAttrs(data->dt_obj[item], NULL, NULL, PDTA_Screen, _screen(obj), TAG_DONE);
              if(DoMethod(data->dt_obj[item], DTM_PROCLAYOUT, NULL, 1))
              {
                GetDTAttrs(data->dt_obj[item], PDTA_CRegs, &data->dt_colours[item], TAG_DONE);
                GetDTAttrs(data->dt_obj[item], PDTA_MaskPlane, &data->dt_mask[item], TAG_DONE);
                GetDTAttrs(data->dt_obj[item], PDTA_DestBitMap, &data->dt_bitmap[item], TAG_DONE);

                if(data->dt_bitmap[item] == NULL) GetDTAttrs(data->dt_obj[item], PDTA_BitMap, &data->dt_bitmap[item], TAG_DONE);
              }
            }
          }
        }

      }
    }
  }
}

///
/// NBitmap_SetupShades()
// create an ARGB shade
BOOL NBitmap_SetupShades(struct InstData *data)
{
  uint32 pixel, altivec_align;

  ENTER();

  altivec_align = 0;

  #if defined(__MORPHOS__)
  if (SysBase->LibNode.lib_Version >= 51)
  {
    NewGetSystemAttrs(&altivec_align, sizeof(altivec_align), SYSTEMINFOTYPE_PPC_ALTIVEC, TAG_DONE);
  }
  #endif

  data->shadeWidth = data->width + data->border_horiz - 2;
  data->shadeHeight = data->height + data->border_vert - 2;
  data->shadeBytesPerRow = data->shadeWidth * 4;

  if (altivec_align)
    data->shadeBytesPerRow = (data->shadeBytesPerRow + 15) & ~15;

  // the shades pixel color
  pixel = ((ULONG)data->prefs.overlay_r << 16) | ((ULONG)data->prefs.overlay_g << 8) | (ULONG)data->prefs.overlay_b;

  if((data->pressedShadePixels = AllocVecAligned(data->shadeBytesPerRow * data->shadeHeight, MEMF_ANY, altivec_align ? 16 : 8, 0)) != NULL)
  {
    uint32 w, h;
    uint32 alpha;
    uint32 *p = data->pressedShadePixels;

    // calculate the alpha channel value
    alpha = (255L - (((255L * 1000L) / (uint32)data->prefs.overlay_shadepress) & 0xff)) << 24;

    // fill the array with the pixel and alpha channel value
    // the border will be the 100% opaque pixel color
    for(h = 0; h < data->shadeHeight; h++)
    {
      for(w = 0; w < data->shadeWidth; w++)
      {
        if(h == 0 || h == data->shadeHeight-1 || w == 0 || w == data->shadeWidth-1)
          *p++ = 0xff000000 | pixel;
        else
          *p++ = alpha | pixel;
      }

      p += (data->shadeBytesPerRow - data->shadeWidth * 4) / 4;
    }
  }

  if((data->overShadePixels = AllocVecAligned(data->shadeBytesPerRow * data->shadeHeight, MEMF_ANY, altivec_align ? 16 : 8, 0)) != NULL)
  {
    uint32 w, h;
    uint32 alpha;
    uint32 *p = data->overShadePixels;

    // calculate the alpha channel value
    alpha = (255L - (((255L * 1000L) / (uint32)data->prefs.overlay_shadeover) & 0xff)) << 24;

    // fill the array with the pixel and alpha channel value
    // the border will be the 100% opaque pixel color
    for(h = 0; h < data->shadeHeight; h++)
    {
      for(w = 0; w < data->shadeWidth; w++)
      {
        if(h == 0 || h == data->shadeHeight-1 || w == 0 || w == data->shadeWidth-1)
          *p++ = 0xff000000 | pixel;
        else
          *p++ = alpha | pixel;
      }

      p += (data->shadeBytesPerRow - data->shadeWidth * 4) / 4;
    }
  }

  RETURN((data->pressedShadePixels != NULL && data->overShadePixels != NULL));
  return (data->pressedShadePixels != NULL && data->overShadePixels != NULL);
}

///
/// CleanupShades()
// delete the ARGB shades
void NBitmap_CleanupShades(struct InstData *data)
{
  ENTER();

  if(data->pressedShadePixels != NULL)
  {
    FreeVec(data->pressedShadePixels);
    data->pressedShadePixels = NULL;
  }
  if(data->overShadePixels != NULL)
  {
    FreeVec(data->overShadePixels);
    data->overShadePixels = NULL;
  }

  LEAVE();
}

///
/// NBitmap_NewImage()
//
BOOL NBitmap_NewImage(struct IClass *cl, Object *obj)
{
  BOOL result = FALSE;
  struct InstData *data;

  ENTER();

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    switch(data->type)
    {
      case MUIV_NBitmap_Type_File:
      case MUIV_NBitmap_Type_DTObject:
      {
        if(data->dt_obj[0] != NULL)
        {
          ULONG i;

          // assume success for the moment
          result = TRUE;

          for(i=0;i<3;i++)
          {
            if(data->dt_obj[i] != NULL)
              result &= NBitmap_ExamineData(data->dt_obj[i], i, cl, obj);
          }
        }
      }
      break;

      case MUIV_NBitmap_Type_CLUT8:
      case MUIV_NBitmap_Type_RGB24:
      case MUIV_NBitmap_Type_ARGB32:
      {
        // no further requirements, instant success
        result = TRUE;
      }
      break;
    }
  }

  RETURN(result);
  return result;
}

BOOL NBitmap_OldNewImage(struct IClass *cl, Object *obj)
{
  BOOL result = FALSE;
  struct InstData *data;

  /* need at least the normal image */
  if((data = INST_DATA(cl, obj)) !=NULL && data->dt_obj[0] != NULL)
  {
    ULONG i;

    for(i = 0; i < 3; i++)
    {
      if(data->dt_obj[i] != NULL)
      {
        struct FrameInfo fri;

        memset(&fri, 0, sizeof(struct FrameInfo));
        DoMethod(data->dt_obj[0], DTM_FRAMEBOX, NULL, &fri, &fri, sizeof(struct FrameInfo), 0);
        data->depth = fri.fri_Dimensions.Depth;
        D(DBF_DATATYPE, "new: framebox dimensions %ldx%ldx%ld", fri.fri_Dimensions.Width, fri.fri_Dimensions.Height, fri.fri_Dimensions.Depth);

        if(data->maxwidth == 0 || (data->maxwidth <= data->dt_header[i]->bmh_Width))
        {
          if(data->maxheight == 0 || (data->maxheight <= data->dt_header[i]->bmh_Height))
          {
            if(data->depth > 0 && data->depth <= 8)
            {
              /* colour lookup bitmap */
              data->fmt = PBPAFMT_LUT8;

              /* bitmap header */
              GetDTAttrs(data->dt_obj[i], PDTA_BitMapHeader, &data->dt_header[i], TAG_DONE);
              data->width =  data->dt_header[0]->bmh_Width;
              data->height =  data->dt_header[0]->bmh_Height;
              D(DBF_DATATYPE, "new: using LUT8 bitmaps");

              result = TRUE;
            }
            else if(data->depth > 8)
            {
              ULONG arraysize;

              /* correct read buffer */
              if(data->depth == 24)
              {
                data->fmt = PBPAFMT_RGB;
                D(DBF_DATATYPE, "new: using 24bit RGB data");
              }
              else
              {
                data->fmt = PBPAFMT_ARGB;
                D(DBF_DATATYPE, "new: using 32bit ARGB data");
              }

              /* bitmap header */
              GetDTAttrs(data->dt_obj[i], PDTA_BitMapHeader, &data->dt_header[i], TAG_DONE);
              data->width =  data->dt_header[0]->bmh_Width;
              data->height =  data->dt_header[0]->bmh_Height;
              data->arraybpp = data->depth / 8;
              data->arraybpr = data->arraybpp * data->width;
              arraysize = (data->arraybpr) * data->height;

              /* get array of pixels */
              if((data->arraypixels[i] = AllocVecShared(arraysize, MEMF_ANY|MEMF_CLEAR)) != NULL)
              {
                ULONG error;

                error = DoMethod(data->dt_obj[i], PDTM_READPIXELARRAY, data->arraypixels[i], data->fmt, data->arraybpr, 0, 0, data->width, data->height);
                (void)error;
                D(DBF_DATATYPE, "new: READPIXELARRAY returned %ld", error);

                // finally create the shades
                result = NBitmap_SetupShades(data);
              }
            }
          }
        }
      }
    }
  }

  RETURN(result);
  return result;
}

///
/// NBitmap_DisposeImage()
//
VOID NBitmap_DisposeImage(struct IClass *cl, Object *obj)
{
  struct InstData *data;

  ENTER();

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    ULONG i;

    /* free datatype object */
    if(data->type == MUIV_NBitmap_Type_File)
    {
      for(i =0 ; i < 3; i++)
      {
        SHOWVALUE(DBF_DATATYPE, data->dt_obj[i]);
        if(data->dt_obj[i] != NULL)
        {
          DisposeDTObject(data->dt_obj[i]);
          data->dt_obj[i] = NULL;
        }
      }
    }

    if(data->label != NULL)
    {
      FreeVec(data->label);
      data->label = NULL;
    }

    /* free pixel memory */
    for(i = 0; i < 3; i++)
    {
      if(data->arraypixels[i] != NULL)
      {
        FreeVec(data->arraypixels[i]);
        data->arraypixels[i] = NULL;
      }
    }

    NBitmap_CleanupShades(data);
  }

  LEAVE();
}

///
/// NBitmap_SetupImage()
//
BOOL NBitmap_SetupImage(struct IClass *cl, Object *obj)
{
  struct InstData *data;
  BOOL result = FALSE;

  ENTER();

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    /* stored config */
    InitConfig(obj, data);

    data->scrdepth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);

    /* input */
    if(data->button)
    {
      data->ehnode.ehn_Priority = 0;
      data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
      data->ehnode.ehn_Object = obj;
      data->ehnode.ehn_Class = cl;
      data->ehnode.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE;

      DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
    }

    switch(data->type)
    {
      case MUIV_NBitmap_Type_File:
      case MUIV_NBitmap_Type_DTObject:
      {

        /* 8-bit data */
        if(data->fmt == PBPAFMT_LUT8 && data->dt_obj[0] != NULL)
        {
          ULONG i;

          /* layout image */
          for(i = 0; i < 3; i++)
          {
            // set the new screen for this object
            SetDTAttrs(data->dt_obj[i], NULL, NULL, PDTA_Screen, _screen(obj), TAG_DONE);
            if(DoMethod(data->dt_obj[i], DTM_PROCLAYOUT, NULL, 1))
            {
              GetDTAttrs(data->dt_obj[i], PDTA_CRegs, &data->dt_colours[i],
                                          PDTA_MaskPlane, &data->dt_mask[i],
                                          PDTA_DestBitMap, &data->dt_bitmap[i],
                                          TAG_DONE);
              if(data->dt_bitmap[i] == NULL)
                GetDTAttrs(data->dt_obj[i], PDTA_BitMap, &data->dt_bitmap[i], TAG_DONE);
              SHOWVALUE(DBF_DATATYPE, data->dt_bitmap[i]);
            }
          }

          result = TRUE;
        }
        else if(data->depth > 8)
          result = NBitmap_SetupShades(data);
        }
      break;

      case MUIV_NBitmap_Type_CLUT8:
      case MUIV_NBitmap_Type_RGB24:
      case MUIV_NBitmap_Type_ARGB32:
      {
        SHOWVALUE(DBF_ALWAYS, data->scrdepth);
        // in case we are to be displayed on a colormapped screen we have to create
        // dithered copies of the images
        #if defined(__amigaos4__)
        if(data->scrdepth <= 8)
        #else
        SHOWVALUE(DBF_ALWAYS, CyberGfxBase);
        if(CyberGfxBase != NULL)
          SHOWVALUE(DBF_ALWAYS, CyberGfxBase->lib_Version);
        if(data->scrdepth <= 8 || CyberGfxBase == NULL)
        #endif
        {
          ULONG i;
          const uint32 *colorMap;

          // use a user definable colormap or the default color map
          if(data->clut != NULL)
          {
            D(DBF_ALWAYS, "using user defined color map");
            colorMap = data->clut;
          }
          else
          {
            D(DBF_ALWAYS, "using default color map");
            colorMap = defaultColorMap;
          }

          D(DBF_ALWAYS, "obtaining pens");
          // allocate all pens
          for(i = 0; i < 256; i++)
          {
            data->ditherPenMap[i] = ObtainBestPen(_screen(obj)->ViewPort.ColorMap, ((colorMap[i] >> 16) & 0x000000ffUL) << 24,
                                                                                   ((colorMap[i] >>  8) & 0x000000ffUL) << 24,
                                                                                   ((colorMap[i] >>  0) & 0x000000ffUL) << 24,
                                                                                   OBP_Precision, PRECISION_IMAGE,
                                                                                   TAG_DONE);
            if(data->ditherPenMap[i] == -1)
              E(DBF_ALWAYS, "failed to obtain pen %ld RGB=%06lx", i, colorMap[i]);
          }

          for(i = 0; i < 3; i++)
          {
            if(data->data[i] != NULL)
            {
              D(DBF_ALWAYS, "dithering image %ld", i);
              // create a dithered copy of the raw image
              data->ditheredImage[i] = DitherImage((CONST_APTR)data->data[i], DITHERA_Width, data->width,
                                                                              DITHERA_Height, data->height,
                                                                              DITHERA_Format, data->type,
                                                                              DITHERA_ColorMap, (IPTR)colorMap,
                                                                              DITHERA_PenMap, (IPTR)data->ditherPenMap,
                                                                              DITHERA_MaskPlane, (IPTR)&data->ditheredMask[i],
                                                                              TAG_DONE);

              #if !defined(__amigaos4__)
              // CyberGraphics cannot blit raw data through a mask, thus we have to
              // use this ugly workaround and take the detour using a bitmap.
              D(DBF_ALWAYS, "setting up dithered bitmap %ld", i);
              data->ditheredBitmap[i] = Chunky2Bitmap(data->ditheredImage[i], data->width, data->height, data->scrdepth);
              #endif // !__amigaos4__
            }
          }
        }
        // no further requirements, instant success
        result = TRUE;
      }
      break;
    }
  }

  RETURN(result);
  return result;
}

///
/// NBitmap_CleanupImage()
//
VOID NBitmap_CleanupImage(struct IClass *cl, Object *obj)
{
  struct InstData *data;

  ENTER();

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    // input
    if(data->button)
      DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);

    switch(data->type)
    {
      case MUIV_NBitmap_Type_File:
      case MUIV_NBitmap_Type_DTObject:
      {
        if(data->fmt == PBPAFMT_LUT8 && data->dt_obj[0] != NULL)
        {
          ULONG i;

          /* layout image */
          for(i = 0; i < 3; i++)
          {
            // reset the screen pointer
            SetDTAttrs(data->dt_obj[i], NULL, NULL, PDTA_Screen, NULL, TAG_DONE);
          }
        }
      }
      break;

      case MUIV_NBitmap_Type_CLUT8:
      case MUIV_NBitmap_Type_RGB24:
      case MUIV_NBitmap_Type_ARGB32:
      {
        // nothing to do
        ULONG i;
        struct Screen *scr = _screen(obj);

        // free the possibly dithered image copies
        for(i = 0; i < 3; i++)
        {
          #if !defined(__amigaos4__)
          if(data->ditheredBitmap[i] != NULL)
          {
            D(DBF_ALWAYS, "freeing dithered bitmap %ld", i);
            FreeBitMap(data->ditheredBitmap[i]);
            data->ditheredBitmap[i] = NULL;
          }
          #endif // !__amigaos4__
          if(data->ditheredImage[i] != NULL)
          {
            D(DBF_ALWAYS, "freeing dithered image %ld", i);
            FreeDitheredImage(data->ditheredImage[i], data->ditheredMask[i]);
            data->ditheredImage[i] = NULL;
          }
        }

        // release all allocated pens
        if(data->scrdepth <= 8)
        {
          D(DBF_ALWAYS, "releasing pens");
          for(i = 0; i < 256; i++)
          {
            if(data->ditherPenMap[i] != -1)
              ReleasePen(scr->ViewPort.ColorMap, data->ditherPenMap[i]);
          }
        }
      }
      break;
    }

    NBitmap_CleanupShades(data);

    // stored config
    FreeConfig(data);
  }

  LEAVE();
}

///
/// NBitmap_DrawSimpleFrame()
//
static void NBitmap_DrawSimpleFrame(Object *obj, uint32 x, uint32 y, uint32 w, uint32 h)
{
  ENTER();

  SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
  Move(_rp(obj), x, y+(h+1));
  Draw(_rp(obj), x, y);
  Draw(_rp(obj), x+(w+1), y);

  SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
  Draw(_rp(obj), x+(w+1), y+(h+1));
  Draw(_rp(obj), x, y+(h+1));

  LEAVE();
}

///
/// NBitmap_DrawImage()
//
void NBitmap_DrawImage(struct IClass *cl, Object *obj)
{
  struct InstData *data;

  ENTER();

  if((data = INST_DATA(cl, obj)) != NULL)
  {
    LONG item;
    ULONG x, y, twidth;

    /* coordinates */
    item = 0;
    x = _left(obj);
    y = _top(obj);
    twidth = (data->width + data->border_horiz) - 2;      /* subtract standard 1 pixel border */

    // clear the background first, otherwise a multiply applied alpha channel
    // will become darker and darker every time
    if(data->button != FALSE)
      DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), _left(obj), _top(obj), 0);

    /* label */
    if(data->label != NULL && data->button != FALSE)
    {
      uint32 labelx;

      SetFont(_rp(obj), _font(obj));
      SetAPen(_rp(obj), 1);

      labelx = (twidth/2) - (data->labelte.te_Width/2);

      Move(_rp(obj), x + labelx, _bottom(obj) - 3);
      Text(_rp(obj), data->label, strlen(data->label));
    }

    /* draw image */
    switch(data->type)
    {
      case MUIV_NBitmap_Type_File:
      case MUIV_NBitmap_Type_DTObject:
      {
        if(data->dt_obj[0] != NULL)
        {
          if(data->fmt == PBPAFMT_LUT8)
          {
            #if defined(__amigaos4__)
            uint32 error;
            #endif

            /* select bitmap */
            if(data->button && data->pressed && data->overlay && data->dt_bitmap[2])
              item = 2;

            SHOWVALUE(DBF_DRAW, item);
            SHOWVALUE(DBF_DRAW, data->dt_bitmap[item]);
            SHOWVALUE(DBF_DRAW, data->dt_mask[item]);

            #if defined(__amigaos4__)
            error = BltBitMapTags(BLITA_Source, data->dt_bitmap[item],
                                  BLITA_Dest, _rp(obj),
                                  BLITA_SrcX, 0,
                                  BLITA_SrcY, 0,
                                  BLITA_DestX, x + (data->border_horiz / 2),
                                  BLITA_DestY, y + ((data->border_vert / 2) - (data->label_vert/2)),
                                  BLITA_Width, data->width,
                                  BLITA_Height, data->height,
                                  BLITA_SrcType, BLITT_BITMAP,
                                  BLITA_DestType, BLITT_RASTPORT,
                                  BLITA_MaskPlane, data->dt_mask[item],
                                  TAG_DONE);
            SHOWVALUE(DBF_DRAW, error);

            #else

            if(data->dt_mask[item] != NULL)
            {
              BltMaskBitMapRastPort(data->dt_bitmap[item], 0, 0, _rp(obj),
                _left(obj) + (data->border_horiz / 2),
                _top(obj) + (data->border_vert / 2),
                data->width,
                data->height,
                0xc0,
                (APTR)data->dt_mask[item]);
            }
            else
            {
              BltBitMapRastPort(data->dt_bitmap[item], 0, 0, _rp(obj),
                _left(obj) + (data->border_horiz / 2),
                _top(obj) + (data->border_vert / 2),
                data->width,
                data->height,
                0xc0);
            }
            #endif
          }
          else
          {
            /* select bitmap */
            if(data->button && data->pressed && data->overlay && data->arraypixels[2] != NULL)
              item = 2;

            SHOWVALUE(DBF_DRAW, item);
            SHOWVALUE(DBF_DRAW, data->arraypixels[item]);

            if(data->arraypixels[item] != NULL)
            {
              #if defined(__amigaos4__)
              int32 srctype;
              uint32 error;

              if(data->depth == 24)
               srctype = BLITT_RGB24;
              else
               srctype = BLITT_ARGB32;

              error = BltBitMapTags(BLITA_Source, data->arraypixels[item],
                                    BLITA_Dest, _rp(obj),
                                    BLITA_SrcX, 0,
                                    BLITA_SrcY, 0,
                                    BLITA_DestX, x + (data->border_horiz / 2),
                                    BLITA_DestY, y + ((data->border_vert / 2) - (data->label_vert/2)),
                                    BLITA_Width, data->width,
                                    BLITA_Height, data->height,
                                    BLITA_SrcType, srctype,
                                    BLITA_DestType, BLITT_RASTPORT,
                                    BLITA_SrcBytesPerRow, data->arraybpr,
                                    BLITA_UseSrcAlpha, TRUE,
                                    TAG_DONE);

              SHOWVALUE(DBF_DRAW, error);

              #else

              if(data->depth == 24)
              {
                WPA(data->arraypixels[item], 0, 0, data->arraybpr, _rp(obj), _left(obj) + (data->border_horiz / 2), _top(obj) + (data->border_vert / 2), data->width, data->height, RECTFMT_RGB);
              }
              else
              {
                WPAA(data->arraypixels[item], 0, 0, data->arraybpr, _rp(obj), _left(obj) + (data->border_horiz / 2), _top(obj) + (data->border_vert / 2), data->width, data->height, 0xffffffff);
              }

              #endif
            }
          }
        }
      }
      break;

      case MUIV_NBitmap_Type_CLUT8:
      case MUIV_NBitmap_Type_RGB24:
      case MUIV_NBitmap_Type_ARGB32:
      {
        int w = min((uint32)_mwidth (obj), data->width );
        int h = min((uint32)_mheight(obj), data->height);

        /* select bitmap */
        if(data->button && data->pressed && data->overlay && data->data[2] != NULL)
          item = 2;

        SHOWVALUE(DBF_ALWAYS, data->scrdepth);
        SHOWVALUE(DBF_ALWAYS, data->ditheredImage[item]);
        SHOWVALUE(DBF_ALWAYS, data->ditheredMask[item]);
        #if !defined(__amigaos4__)
        SHOWVALUE(DBF_ALWAYS, data->ditheredBitmap[item]);
        #endif

        if(data->data[item] != NULL)
        {
          #if defined(__amigaos4__)
          if(data->scrdepth <= 8 && data->ditheredImage[item] != NULL)
          {
            if(data->ditheredMask[item] != NULL)
            {
              D(DBF_ALWAYS, "drawing remapped/dithered image with mask");
              BltBitMapTags(BLITA_Source, data->ditheredImage[item],
                            BLITA_Dest, _rp(obj),
                            BLITA_SrcX, 0,
                            BLITA_SrcY, 0,
                            BLITA_DestX, x + (data->border_horiz / 2),
                            BLITA_DestY, y + ((data->border_vert / 2) - (data->label_vert/2)),
                            BLITA_Width, w,
                            BLITA_Height, h,
                            BLITA_SrcType, BLITT_CHUNKY,
                            BLITA_DestType, BLITT_RASTPORT,
                            BLITA_SrcBytesPerRow, data->width,
                            BLITA_MaskPlane, data->ditheredMask[item],
                            BLITA_Minterm, (ABC|ABNC|ANBC),
                            TAG_DONE);
            }
            else
            {
              D(DBF_ALWAYS, "drawing remapped/dithered image without mask");
              BltBitMapTags(BLITA_Source, data->ditheredImage[item],
                            BLITA_Dest, _rp(obj),
                            BLITA_SrcX, 0,
                            BLITA_SrcY, 0,
                            BLITA_DestX, x + (data->border_horiz / 2),
                            BLITA_DestY, y + ((data->border_vert / 2) - (data->label_vert/2)),
                            BLITA_Width, w,
                            BLITA_Height, h,
                            BLITA_SrcType, BLITT_CHUNKY,
                            BLITA_DestType, BLITT_RASTPORT,
                            BLITA_SrcBytesPerRow, data->width,
                            BLITA_Minterm, (ABC|ABNC),
                            TAG_DONE);
            }
          }
          #else // __amigaos4__
          if((data->scrdepth <= 8 || CyberGfxBase == NULL) && data->ditheredBitmap[item] != NULL)
          {
            // CyberGraphics cannot blit raw data through a mask, thus we have to
            // take this ugly workaround and take the detour using a bitmap.
            if(data->ditheredMask[item] != NULL)
            {
              D(DBF_ALWAYS, "drawing remapped/dithered image with mask");
              BltMaskBitMapRastPort(data->ditheredBitmap[item], 0, 0, _rp(obj), x + (data->border_horiz / 2), y + ((data->border_vert / 2) - (data->label_vert/2)), w, h, (ABC|ABNC|ANBC), data->ditheredMask[item]);
            }
            else
            {
              D(DBF_ALWAYS, "drawing remapped/dithered image without mask");
              BltBitMapRastPort(data->ditheredBitmap[item], 0, 0, _rp(obj), x + (data->border_horiz / 2), y + ((data->border_vert / 2) - (data->label_vert/2)), w, h, (ABC|ABNC));
            }
          }
          #endif // __amigaos4__
          else
          {
            #if defined(__amigaos4__)
            switch(data->type)
            {
              case MUIV_NBitmap_Type_CLUT8:
                BltBitMapTags(BLITA_Source, data->data[item],
                              BLITA_Dest, _rp(obj),
                              BLITA_SrcX, 0,
                              BLITA_SrcY, 0,
                              BLITA_DestX, x + (data->border_horiz / 2),
                              BLITA_DestY, y + ((data->border_vert / 2) - (data->label_vert/2)),
                              BLITA_Width, w,
                              BLITA_Height, h,
                              BLITA_SrcType, BLITT_CHUNKY,
                              BLITA_DestType, BLITT_RASTPORT,
                              BLITA_SrcBytesPerRow, data->width,
                              BLITA_CLUT, data->clut,
                              TAG_DONE);
              break;

              case MUIV_NBitmap_Type_RGB24:
                D(DBF_ALWAYS, "drawing RGB image");
                BltBitMapTags(BLITA_Source, data->data[item],
                              BLITA_Dest, _rp(obj),
                              BLITA_SrcX, 0,
                              BLITA_SrcY, 0,
                              BLITA_DestX, x + (data->border_horiz / 2),
                              BLITA_DestY, y + ((data->border_vert / 2) - (data->label_vert/2)),
                              BLITA_Width, w,
                              BLITA_Height, h,
                              BLITA_SrcType, BLITT_RGB24,
                              BLITA_DestType, BLITT_RASTPORT,
                              BLITA_SrcBytesPerRow, data->width*3,
                              BLITA_Alpha, data->alpha,
                              TAG_DONE);
              break;

              case MUIV_NBitmap_Type_ARGB32:
                D(DBF_ALWAYS, "drawing ARGB image");
                BltBitMapTags(BLITA_Source, data->data[item],
                              BLITA_Dest, _rp(obj),
                              BLITA_SrcX, 0,
                              BLITA_SrcY, 0,
                              BLITA_DestX, x + (data->border_horiz / 2),
                              BLITA_DestY, y + ((data->border_vert / 2) - (data->label_vert/2)),
                              BLITA_Width, w,
                              BLITA_Height, h,
                              BLITA_SrcType, BLITT_ARGB32,
                              BLITA_DestType, BLITT_RASTPORT,
                              BLITA_SrcBytesPerRow, data->width*4,
                              BLITA_UseSrcAlpha, TRUE,
                              BLITA_Alpha, data->alpha,
                              TAG_DONE);
              break;
            }
            #else // __amigaos4__
            switch(data->type)
            {
              case MUIV_NBitmap_Type_CLUT8:
                WriteLUTPixelArray(data->data[item], 0, 0, data->width, _rp(obj), (APTR)data->clut, x + (data->border_horiz / 2), y + ((data->border_vert / 2) - (data->label_vert/2)), w, h, CTABFMT_XRGB8);
              break;

              case MUIV_NBitmap_Type_RGB24:
                D(DBF_ALWAYS, "drawing RGB image");
                WPA(data->data[item], 0, 0, data->width*3, _rp(obj), x + (data->border_horiz / 2), y + ((data->border_vert / 2) - (data->label_vert/2)), w, h, RECTFMT_RGB);
              break;

              case MUIV_NBitmap_Type_ARGB32:
                D(DBF_ALWAYS, "drawing ARGB image");
                WPAA(data->data[item], 0, 0, data->width*4, _rp(obj), x + (data->border_horiz / 2), y + ((data->border_vert / 2) - (data->label_vert/2)), w, h, data->alpha);
              break;
            }
            #endif // __amigaos4__
          }
        }
      }
      break;
    }

    /* overlay */
    if(data->button && data->overlay)
    {
      if(data->prefs.overlay_type == 1 || data->scrdepth <= 8)
      {
        /* standard overlay */
        if(data->pressed)
          NBitmap_DrawSimpleFrame(obj, x + (data->border_horiz / 2), y + ((data->border_vert / 2) - (data->label_vert/2)), data->width, data->height);
        else
          NBitmap_DrawSimpleFrame(obj, x + (data->border_horiz / 2), y + ((data->border_vert / 2) - (data->label_vert/2)), data->width, data->height);
      }
      else
      {
        #if defined(__amigaos4__)
        uint32 error;

        if(data->pressed)
          error = BltBitMapTags(BLITA_Source, data->pressedShadePixels,
                                BLITA_Dest, _rp(obj),
                                BLITA_SrcX, 0,
                                BLITA_SrcY, 0,
                                BLITA_DestX, x+1,
                                BLITA_DestY, y+1,
                                BLITA_Width, data->shadeWidth,
                                BLITA_Height, data->shadeHeight,
                                BLITA_SrcType, BLITT_ARGB32,
                                BLITA_DestType, BLITT_RASTPORT,
                                BLITA_SrcBytesPerRow, data->shadeBytesPerRow,
                                BLITA_UseSrcAlpha, TRUE,
                                TAG_DONE);
        else
          error = BltBitMapTags(BLITA_Source, data->overShadePixels,
                                BLITA_Dest, _rp(obj),
                                BLITA_SrcX, 0,
                                BLITA_SrcY, 0,
                                BLITA_DestX, x+1,
                                BLITA_DestY, y+1,
                                BLITA_Width, data->shadeWidth,
                                BLITA_Height, data->shadeHeight,
                                BLITA_SrcType, BLITT_ARGB32,
                                BLITA_DestType, BLITT_RASTPORT,
                                BLITA_SrcBytesPerRow, data->shadeBytesPerRow,
                                BLITA_UseSrcAlpha, TRUE,
                                TAG_DONE);

        SHOWVALUE(DBF_DRAW, error);

        #else

        if(data->pressed)
        {
          WPAA(data->pressedShadePixels, 0, 0, data->shadeBytesPerRow, _rp(obj), x+1, y+1, data->shadeWidth, data->shadeHeight, 0xffffffff);
        }
        else
        {
          WPAA(data->overShadePixels, 0, 0, data->shadeBytesPerRow, _rp(obj), x+1, y+1, data->shadeWidth, data->shadeHeight, 0xffffffff);
        }

        #endif
      }
    }
  }

  LEAVE();
}

///
