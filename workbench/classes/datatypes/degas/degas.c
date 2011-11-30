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

#include "degas_protos.h"


#define HEADER_SIZE 34
#define TAIL_SIZE 32
#define XBIOS_ENTRY_SIZE 2
#define ASPECT_RATIO_X 8
#define ASPECT_RATIO_Y 5


#define SCALEXBIOS(A) ((A) * 0x24924924)


static UPINT DispatchOp(struct IClass *class REG("a0"),
   Object *object REG("a2"), Msg op REG("a1"));
static UPINT OMNew(struct IClass *class, Object *object, struct opSet *op,
   struct DTBase *base);



/****i* degas.datatype/MakeDegasClass **************************************
*
*   NAME
*	MakeDegasClass
*
*   SYNOPSIS
*	class = MakeDegasClass()
*
*	struct IClass *MakeDegasClass(VOID);
*
****************************************************************************
*
*/

struct IClass *MakeDegasClass(struct DTBase *base)
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
      class->cl_UserData = (IPTR)base;
      AddClass(class);
   }

   return class;
}



/****i* degas.datatype/DispatchOp ******************************************
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
   struct DTBase *base;
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



/****i* degas.datatype/OMNew ***********************************************
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
   struct DTBase *base)
{
   LONG error = 0;
   struct TagItem *tags;
   UWORD in_line_size, out_line_size, count, colour_count, i, j, k, entry,
      type, width, height, depth;
   UBYTE *buffer = NULL, *buffer_end, *in, *out, *in_stop, *out_stop,
      *end_of_line, fill;
   BPTR file;
   struct FileInfoBlock *info = NULL;
   struct BitMap *bitmap = NULL;
   struct BitMapHeader *bitmap_header;
   struct ColorRegister *colour_registers;
   BOOL compressed;
   ULONG screen_mode, *c_regs, *c_reg;
   BYTE control;

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
      if(info->fib_Size < HEADER_SIZE)
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
      type = BEWord(*(UWORD *)in);
      in += 2;

      compressed = (type & 0x8000) != 0;

      switch(type & 0x3)
      {
      case 0:
         width = 320;
         height = 200;
         depth = 4;
         break;
      case 1:
         width = 640;
         height = 200;
         depth = 2;
         break;
      case 2:
         width = 640;
         height = 400;
         depth = 1;
         break;
      default:
         error = ERROR_OBJECT_WRONG_TYPE;
      }

      bitmap_header->bmh_Depth = depth;
      bitmap_header->bmh_Width = width;
      bitmap_header->bmh_Height = height;

      in_line_size = bitmap_header->bmh_Width / 8 ;
      colour_count = 1 << bitmap_header->bmh_Depth;
      SetDTAttrs(object, NULL, NULL, PDTA_NumColors, colour_count, TAG_END);
      GetDTAttrs(object, PDTA_ColorRegisters, (UPINT)&colour_registers,
         PDTA_CRegs, (UPINT)&c_regs, TAG_END);

      /* Allocate picture's bitmap */

      bitmap = AllocBitMap(bitmap_header->bmh_Width,
         bitmap_header->bmh_Height, bitmap_header->bmh_Depth, 0, NULL);
      if(bitmap == NULL)
         error = IoErr();

      /* Check an uncompressed file has enough data */

      if(!compressed
         && info->fib_Size < HEADER_SIZE + height * in_line_size * depth)
         error = ERROR_OBJECT_WRONG_TYPE;
   }

   if(error == 0)
   {
      /* Fill in bitmap's palette */

      c_reg = c_regs;
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

      /* Make lower-precision copy of palette */

      for(i = 0; i < colour_count; i++, colour_registers++)
      {
         colour_registers->red = *(c_regs++) >> 24;
         colour_registers->green = *(c_regs++) >> 24;
         colour_registers->blue = *(c_regs++) >> 24;
      }

      /* Read image data into bitmap */

      out_line_size = bitmap->BytesPerRow;
      in = buffer + HEADER_SIZE;

      for(i = 0; i < bitmap_header->bmh_Height; i++)
      {
         if(compressed)
         {
            for(j = 0; j < bitmap_header->bmh_Depth; j++)
            {
               out = bitmap->Planes[j] + out_line_size * i;
               end_of_line = out + in_line_size;

               while(error == 0 && out < end_of_line)
               {
                  if(in + 1 < buffer_end)
                  {
                     control = (BYTE)*in++;
                     if(control < 0)
                     {
                        fill = *in++;
                        count = 1 - control;
                        out_stop = out + count;
                        if(out_stop <= end_of_line)
                        {
                           while(out < out_stop)
                              *out++ = fill;
                        }
                        else
                           error = ERROR_OBJECT_WRONG_TYPE;
                     }
                     else
                     {
                        count = control + 1;
                        in_stop = in + count;
                        if(in_stop <= buffer_end
                           && out + count <= end_of_line)
                        {
                           while(in < in_stop)
                              *out++ = *in++;
                        }
                        else
                           error = ERROR_OBJECT_WRONG_TYPE;
                     }
                  }
                  else
                     error = ERROR_OBJECT_WRONG_TYPE;
               }
            }
         }
         else
         {
            for(j = 0; j < in_line_size; j += 2)
            {
               for(k = 0; k < bitmap_header->bmh_Depth; k++)
               {
                  out = bitmap->Planes[k] + out_line_size * i + j;
                  end_of_line = out + in_line_size;

                  *out++ = *in++;
                  *out++ = *in++;

               }
            }
         }
      }

      /* Check there isn't any unused picture data */

      if(in != buffer_end && in != buffer_end - TAIL_SIZE)
         error = ERROR_OBJECT_WRONG_TYPE;
   }

   if(error == 0)
   {

#ifndef AMIGAOS
      screen_mode = BestModeID(
         BIDTAG_DesiredWidth, bitmap_header->bmh_Width,
         BIDTAG_DesiredHeight, bitmap_header->bmh_Height,
         BIDTAG_NominalWidth, ASPECT_RATIO_X,
         BIDTAG_NominalHeight, ASPECT_RATIO_Y,
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



