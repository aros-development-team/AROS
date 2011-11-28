/*

Author: Neil Cafferkey
Copyright (C) 2003 Neil Cafferkey

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

#include <exec/types.h>
#include <datatypes/datatypes.h>

#include "datatype.h"


#define HEADER_SIZE 34



BOOL Main(struct DTHookContext *context REG("a0"))
{
   BOOL success = TRUE;
   UWORD *buffer, i;

   if(context->dthc_BufferLength < HEADER_SIZE)
      success = FALSE;
   if(success)
   {
      buffer = (UWORD *)context->dthc_Buffer;
      if((BEWord(*buffer++) & ~0x8003) != 0)
         success = FALSE;
   }

   return success;
}



