/*

Author: Neil Cafferkey
Copyright (C) 2002-2011 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include <exec/memory.h>
#include <utility/hooks.h>
#include <datatypes/pictureclass.h>
#include <intuition/classusr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/alib.h>

#include "datatype.h"

#include "gemimage_protos.h"


#define MIN_HEADER_SIZE 16
#define VDI_ENTRY_SIZE 6
#define XBIOS_ENTRY_SIZE 2
#define XIMG_ID ('X' << 24 | 'I' << 16 | 'M' << 8 | 'G')
#define STTT_ID ('S' << 24 | 'T' << 16 | 'T' << 8 | 'T')

enum
{
   NO_PALETTE,
   BUILTIN_PALETTE,
   GREYSCALE_PALETTE,
   XBIOS_PALETTE,
   VDI_PALETTE
};


#define SCALEVDI(A) ((A) * ((1 << 29) / 125))
#define SCALEXBIOS(A) ((A) * 0x24924924)


static UPINT DispatchOp(struct IClass *class REG("a0"),
   Object *object REG("a2"), Msg op REG("a1"));
static UPINT OMNew(struct IClass *class, Object *object, struct opSet *op,
   struct GemImgBase *base);


static const struct ColorRegister bilevel_palette[] =
{
   {0xff, 0xff, 0xff},
   {0x00, 0x00, 0x00}
};


static const struct ColorRegister pc_palette[]=
{
   {0xff, 0xff, 0xff},
   {0xff, 0x00, 0x00},
   {0x00, 0xff, 0x00},
   {0xff, 0xff, 0x00},
   {0x00, 0x00, 0xff},
   {0xff, 0x00, 0xff},
   {0x00, 0xff, 0xff},
   {0xaa, 0xaa, 0xaa},
   {0x55, 0x55, 0x55},
   {0xaa, 0x00, 0x00},
   {0x00, 0xaa, 0x00},
   {0xaa, 0xaa, 0x00},
   {0x00, 0x00, 0xaa},
   {0xaa, 0x00, 0xaa},
   {0x00, 0xaa, 0xaa},
   {0x00, 0x00, 0x00}
};



/****i* gemimage.datatype/MakeGemImgClass **********************************
*
*   NAME
*	MakeGemImgClass
*
*   SYNOPSIS
*	class = MakeGemImgClass()
*
*	struct IClass *MakeGemImgClass(VOID);
*
****************************************************************************
*
*/

struct IClass *MakeGemImgClass(struct GemImgBase *base)
{
   BOOL success = TRUE;
   struct IClass *class;

   class =
      MakeClass(base->library.lib_Node.ln_Name, PICTUREDTCLASS, NULL, 0, 0);
   if(class == NULL)
      success = FALSE;

   if(success)
   {
      class->cl_Dispatcher.h_Entry = (HOOKFUNC)DispatchOp;
      class->cl_UserData = (ULONG)base;
      AddClass(class);
   }

   return class;
}



/****i* gemimage.datatype/DispatchOp ***************************************
*
*   NAME
*	DispatchOp
*
*   SYNOPSIS
*	result = DispatchOp(class, object, op)
*
*	UPINT DispatchOp(struct IClass, Object, Msg);
*
****************************************************************************
*
*/

static UPINT DispatchOp(struct IClass *class REG("a0"),
   Object *object REG("a2"), Msg op REG("a1"))
{
   struct GemImgBase *base;
   UPINT result;

   base = (APTR)class->cl_UserData;
   switch(op->MethodID)
   {
   case OM_NEW:
      result = OMNew(class, object, (APTR)op, base);
      break;

   default:
      result = DoSuperMethodA(class, object, op);
   }

   return result;
}



/****i* gemimage.datatype/OMNew ********************************************
*
*   NAME
*	OMNew
*
*   SYNOPSIS
*	result = OMNew(class, object, op)
*
*	UPINT OMNew(struct IClass, Object, struct opSet);
*
****************************************************************************
*
*/

static UPINT OMNew(struct IClass *class, Object *object, struct opSet *op,
   struct GemImgBase *base)
{
   LONG error = 0;
   struct TagItem *tags;
   UWORD header_size, extension_size, pattern_size, pixel_width,
      pixel_height, in_line_size, out_line_size, repeat_count = 0,
      colour_count, i, j, k, entry, version;
   UBYTE datum, *buffer = NULL, *buffer_end, *in, *out, *in_stop, *out_stop,
      *end_of_line, fill, *pattern, palette_type;
   BPTR file;
   struct FileInfoBlock *info = NULL;
   struct BitMap *bitmap = NULL;
   struct BitMapHeader *bitmap_header;
   struct ColorRegister *colour_registers;
   const struct ColorRegister *palette;
   BOOL repeat = FALSE, il = TRUE, reverse_planes = FALSE;
   ULONG screen_mode, *c_regs, *c_reg, level;

   /* Get a new picture object */

   object = (Object *)DoSuperMethodA(class, object, (Msg)op);
   if(object == NULL)
      error = IoErr();

   /* Examine file */

   tags = op->ops_AttrList;
   if(GetTagData(DTA_SourceType, DTST_FILE, tags) != DTST_FILE)
      error = ERROR_OBJECT_WRONG_TYPE;

   if(error == 0)
   {
      GetDTAttrs(object, DTA_Handle, (UPINT)&file,
         PDTA_BitMapHeader, (UPINT)&bitmap_header, TAG_END);
      if(Seek(file, 0, OFFSET_BEGINNING) == -1)
         error = IoErr();
      info = AllocDosObject(DOS_FIB, NULL);
      if(info == NULL)
         error = IoErr();
   }

   if(error == 0)
   {
      if(!ExamineFH(file, info))
         error = IoErr();
   }

   if(error == 0)
   {
      if(info->fib_Size < MIN_HEADER_SIZE)
         error = ERROR_OBJECT_WRONG_TYPE;

      buffer = AllocVec(info->fib_Size, MEMF_ANY);
      buffer_end = buffer + info->fib_Size;
      if(buffer == NULL)
         error = IoErr();
   }

   if(error == 0)
   {
      /* Read entire file */

      if(Read(file, buffer, info->fib_Size) == -1)
         error = IoErr();
   }

   if(error == 0)
   {
      /* Interpret header */

      in = buffer;
      version = BEWord(*(UWORD *)in);
      in += 2;
      header_size = BEWord(*(UWORD *)in) * sizeof(UWORD);
      in += 2;
      bitmap_header->bmh_Depth = BEWord(*(UWORD *)in);
      in += 2;
      pattern_size = BEWord(*(UWORD *)in);
      in += 2;
      pixel_width = BEWord(*(UWORD *)in);
      in += 2;
      pixel_height = BEWord(*(UWORD *)in);
      in += 2;
      bitmap_header->bmh_Width = BEWord(*(UWORD *)in);
      in += 2;
      bitmap_header->bmh_Height = BEWord(*(UWORD *)in);
      in += 2;

      if(info->fib_Size < header_size)
         error = ERROR_OBJECT_WRONG_TYPE;

      in_line_size = (bitmap_header->bmh_Width - 1) / 8 + 1;
      colour_count = 1 << bitmap_header->bmh_Depth;
      SetDTAttrs(object, NULL, NULL, PDTA_NumColors, colour_count, TAG_END);
      GetDTAttrs(object, PDTA_ColorRegisters, (UPINT)&colour_registers,
         PDTA_CRegs, (UPINT)&c_regs, TAG_END);

      /* Allocate picture's bitmap */

      bitmap = AllocBitMap(bitmap_header->bmh_Width,
         bitmap_header->bmh_Height, bitmap_header->bmh_Depth, BMF_CLEAR,
         NULL);
      if(bitmap == NULL)
         error = IoErr();
   }

   if(error == 0)
   {
      /* Determine subformat */

      extension_size = header_size - MIN_HEADER_SIZE;

      if(extension_size == colour_count * XBIOS_ENTRY_SIZE)
      {
         /* "No Sig" subformat */

         palette_type = XBIOS_PALETTE;
         il = FALSE;
      }
      else if(extension_size == 2 + colour_count * XBIOS_ENTRY_SIZE
         && BEWord(*(UWORD *)in) == 0x0080)
      {
         /* HyperPaint subformat */

         palette_type = XBIOS_PALETTE;
         in += 2;
      }
      else if(extension_size == (1 + colour_count) * VDI_ENTRY_SIZE
         && BELong(*(ULONG *)in) == XIMG_ID
         && BEWord(*(UWORD *)(in + 4)) == 0x0000
         && (version == 2 || version == 1))
      {
         /* XIMG subformat */

         palette_type = VDI_PALETTE;
         in += VDI_ENTRY_SIZE;
      }
      else if(extension_size == 2 + colour_count * XBIOS_ENTRY_SIZE
         && BELong(*(ULONG *)in) == STTT_ID
         && BEWord(*(UWORD *)(in + 4)) == 0x0010)
      {
         /* ST/TT subformat */

         il = FALSE;
         palette_type = XBIOS_PALETTE;
         in += 6;
      }
      else if(extension_size == 0)
      {
         /* Original format or a paletteless subformat */

         switch(bitmap_header->bmh_Depth)
         {
         case 1:
            palette = bilevel_palette;
            palette_type = BUILTIN_PALETTE;
            break;
         case 4:
            palette = pc_palette;
            palette_type = BUILTIN_PALETTE;
            break;
         case 8:
            palette_type = GREYSCALE_PALETTE;
            reverse_planes = TRUE;
            break;
         case 16:
         case 24:
            il = FALSE;
            palette_type = NO_PALETTE;
         default:
            error = ERROR_OBJECT_WRONG_TYPE;
         }
      }
      else
         error = ERROR_OBJECT_WRONG_TYPE;
   }

   if(error == 0)
   {
      /* Fill in bitmap's palette */

      c_reg = c_regs;
      switch(palette_type)
      {
      case VDI_PALETTE:
         for(i = 0; i < colour_count * 3; i++)
         {
            *(c_reg++) = SCALEVDI(BEWord(*(UWORD *)in));
            in += 2;
         }
         break;
      case XBIOS_PALETTE:
         for(i = 0; i < colour_count; i++)
         {
            c_reg += 3;
            entry = BEWord(*(UWORD *)in);
            for(j = 1; j <= 3; j++)
            {
               *(c_reg - j) = SCALEXBIOS(entry & 0x7);
               entry = entry >> 4;
            }
            in += 2;
         }
         break;
      case BUILTIN_PALETTE:
         for(i = 0; i < colour_count; i++, palette++)
         {
            *(c_reg++) = palette->red << 24;
            *(c_reg++) = palette->green <<24;
            *(c_reg++) = palette->blue << 24;
         }
         break;
      case GREYSCALE_PALETTE:
         level = 0xff << 24;
         for(i = 0; i < colour_count; i++)
         {
            for(j = 0; j < 3; j++)
               *(c_reg++) = level;
            level -= 1 << 24;
         }
      }

      /* Make lower-precision copy of palette */

      if(palette_type != NO_PALETTE)
      {
         for(i = 0; i < colour_count; i++, colour_registers++)
         {
            colour_registers->red = *(c_regs++) >> 24;
            colour_registers->green = *(c_regs++) >> 24;
            colour_registers->blue = *(c_regs++) >> 24;
         }
      }

      /* Read image data into bitmap */

      out_line_size = bitmap->BytesPerRow;
      in = buffer + header_size;

      for(il? (i = 0): (j = 0);
         il? i < bitmap_header->bmh_Height: j < bitmap_header->bmh_Depth;
         il? i++: j++)
      {
         if(!repeat)
         {
            if(in + 1 < buffer_end)
            {
               if(*((UWORD *)in) == 0)
               {
                  if(in + 3 < buffer_end)
                  {
                     in += 3;
                     repeat_count = *(in++);
                     if(repeat_count != 0)
                        repeat_count--;
                  }
                  else
                     error = ERROR_OBJECT_WRONG_TYPE;
               }
            }
         }

         for(il? (j = 0): (i = 0);
            il? j < bitmap_header->bmh_Depth: i < bitmap_header->bmh_Height;
            il? j++: i++)
         {
            if(reverse_planes)
               k = bitmap_header->bmh_Depth - (j + 1);
            else
               k = j;
            out = bitmap->Planes[k] + out_line_size * i;
            end_of_line = out + in_line_size;

            if(repeat)
               CopyMem(out - out_line_size, out, out_line_size);
            else
            {
               while(error == 0 && out < end_of_line)
               {
                  if(in == buffer_end)
                     error = ERROR_OBJECT_WRONG_TYPE;

                  if(error == 0)
                  {
                     datum = *in++;
                     if((datum & ~0x80) == 0)
                     {
                        if(in == buffer_end)
                           error = ERROR_OBJECT_WRONG_TYPE;
                        if(error == 0)
                        {
                           if(datum == 0x00)
                           {
                              datum = *in++;
                              in_stop = in + pattern_size;
                              out_stop = out + pattern_size * datum;
                              if(in_stop <= buffer_end
                                 && out_stop <= end_of_line)
                              {
                                 pattern = in; in++;
                                 while(out < out_stop)
                                 {
                                    in = pattern;
                                    while(in < in_stop)
                                       *out++ = *in++;
                                 }
                              }
                              else
                                 error = ERROR_OBJECT_WRONG_TYPE;
                           }
                           else
                           {
                              datum = *in++;
                              in_stop = in + datum;
                              if(in_stop <= buffer_end
                                 && out + datum <= end_of_line)
                              {
                                 while(in < in_stop)
                                    *out++ = *in++;
                              }
                              else
                                 error = ERROR_OBJECT_WRONG_TYPE;
                           }
                        }
                     }
                     else
                     {
                        if((datum & 0x80) != 0)
                           fill = 0xff;
                        else
                           fill = 0x00;
                        datum &= ~0x80;
                        out_stop = out + datum;
                        if(out_stop <= end_of_line)
                        {
                           while(out < out_stop)
                              *out++ = fill;
                        }
                        else
                           error = ERROR_OBJECT_WRONG_TYPE;
                     }
                  }
               }
            }
         }

         if(repeat_count != 0)
         {
            repeat = TRUE;
            repeat_count--;
         }
         else
            repeat = FALSE;
      }

      /* Check there isn't any unused picture data */

      if(in < buffer_end)
         error = ERROR_OBJECT_WRONG_TYPE;
   }

   if(error == 0)
   {
#ifndef AMIGAOS
      screen_mode = BestModeID(
         BIDTAG_DesiredWidth, bitmap_header->bmh_Width,
         BIDTAG_DesiredHeight, bitmap_header->bmh_Height,
         BIDTAG_NominalWidth, (pixel_width * bitmap_header->bmh_Width),
         BIDTAG_NominalHeight, (pixel_height * bitmap_header->bmh_Height),
         BIDTAG_Depth, bitmap_header->bmh_Depth,
         TAG_END);
#else
      screen_mode = BestModeID(
         BIDTAG_NominalWidth, bitmap_header->bmh_Width,
         BIDTAG_NominalHeight, bitmap_header->bmh_Height,
         BIDTAG_Depth, bitmap_header->bmh_Depth,
         TAG_END);
#endif

      SetDTAttrs(object, NULL, NULL,
         DTA_ObjName,
            (UPINT)FilePart((APTR)GetTagData(DTA_Name, (UPINT)NULL, tags)),
         DTA_NominalHoriz, bitmap_header->bmh_Width,
         DTA_NominalVert, bitmap_header->bmh_Height,
         PDTA_BitMap, (UPINT)bitmap,
         PDTA_ModeID, screen_mode,
         TAG_END);
   }

   /* Free Resources */

   FreeVec(buffer);
   FreeDosObject(DOS_FIB, info);

   if(error != 0)
   {
      FreeBitMap(bitmap);
      CoerceMethod(class, object, OM_DISPOSE);
      object = NULL;
   }

   /* Return new object */

   SetIoErr(error);
   return (UPINT)object;
}



