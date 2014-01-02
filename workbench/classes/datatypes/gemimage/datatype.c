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


#include <exec/types.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <graphics/gfxbase.h>
#include "initializers.h"

#include <proto/exec.h>
#include <proto/intuition.h>

#include "datatype.h"

#include "datatype_protos.h"
#include "gemimage_protos.h"

#define SUPERCLASS_VERSION 0
#define INTUITION_VERSION 36
#define UTILITY_VERSION 36
#define DATATYPES_VERSION 0
#define GRAPHICS_VERSION 39
#define DOS_VERSION 36


/* Private prototypes */

static VOID DeleteDataType(struct GemImgBase *base);


/* Return an error immediately if someone tries to run the datatype */

LONG Main()
{
   return -1;
}



const TEXT datatype_name[] = DATATYPE_NAME;
const TEXT version_string[] =
   DATATYPE_NAME " " STR(VERSION) "." STR(REVISION) " (" DATE ")\n";
static const TEXT superclass_name[] = "datatypes/picture.datatype";
static const TEXT intuition_name[] = "intuition.library";
static const TEXT utility_name[] = UTILITYNAME;
static const TEXT datatypes_name[] = "datatypes.library";
static const TEXT graphics_name[] = GRAPHICSNAME;
static const TEXT dos_name[] = DOSNAME;


static const APTR vectors[] =
{
   (APTR)LibOpen,
   (APTR)LibClose,
   (APTR)LibExpunge,
   (APTR)LibReserved,
   (APTR)ObtainClass,
   (APTR)-1
};


static const struct
{
   SMALLINITBYTEDEF(type);
   SMALLINITPINTDEF(name);
   SMALLINITBYTEDEF(flags);
   SMALLINITWORDDEF(version);
   SMALLINITWORDDEF(revision);
   SMALLINITPINTDEF(id_string);
   INITENDDEF;
}
init_data =
{
   SMALLINITBYTE(OFFSET(Node, ln_Type), NT_LIBRARY),
   SMALLINITPINT(OFFSET(Node, ln_Name), datatype_name),
   SMALLINITBYTE(OFFSET(Library, lib_Flags), LIBF_SUMUSED | LIBF_CHANGED),
   SMALLINITWORD(OFFSET(Library, lib_Version), VERSION),
   SMALLINITWORD(OFFSET(Library, lib_Revision), REVISION),
   SMALLINITPINT(OFFSET(Library, lib_IdString), version_string),
   INITEND
};


static const APTR init_table[] =
{
   (APTR)sizeof(struct GemImgBase),
   (APTR)vectors,
   (APTR)&init_data,
   (APTR)LibInit
};


const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT,
   VERSION,
   NT_LIBRARY,
   0,
   (TEXT *)datatype_name,
   (TEXT *)version_string,
   (APTR)init_table
};



/****i* gemimage.datatype/LibInit ******************************************
*
*   NAME
*	LibInit
*
*   SYNOPSIS
*	lib_base = LibInit(lib_base, seg_list)
*
*	struct GemImgBase *LibInit(struct GemImgBase *, APTR);
*
****************************************************************************
*
*/

struct GemImgBase *LibInit(struct GemImgBase *lib_base REG("d0"),
   APTR seg_list REG("a0"), struct GemImgBase *base REG(BASE_REG))
{
   BOOL success = TRUE;

   lib_base->sys_base = (APTR)base;
   base = lib_base;
   base->seg_list = seg_list;

   base->superclass_base = OpenLibrary(superclass_name, SUPERCLASS_VERSION);
   base->intuition_base = (APTR)OpenLibrary(intuition_name,
      INTUITION_VERSION);
   base->utility_base = (APTR)OpenLibrary(utility_name, UTILITY_VERSION);
   base->datatypes_base = (APTR)OpenLibrary(datatypes_name,
      DATATYPES_VERSION);
   base->graphics_base = (APTR)OpenLibrary(graphics_name, GRAPHICS_VERSION);
   base->dos_base = (APTR)OpenLibrary(dos_name, DOS_VERSION);
   if(base->superclass_base == NULL || base->intuition_base == NULL
      || base->utility_base == NULL || base->datatypes_base == NULL
      || base->graphics_base == NULL || base->dos_base == NULL)
      success = FALSE;

   if(success)
   {
      base->class = MakeGemImgClass(base);
      if(base->class == NULL)
         success = FALSE;
   }

   if(!success)
   {
      DeleteDataType(base);
      base = NULL;
   }

   return base;
}



/****i* gemimage.datatype/LibOpen ******************************************
*
*   NAME
*	LibOpen
*
*   SYNOPSIS
*	datatype = LibOpen(version)
*
*	struct GemImgBase *LibOpen(ULONG);
*
****************************************************************************
*
*/

struct GemImgBase *LibOpen(ULONG version REG("d0"),
   struct GemImgBase *base REG(BASE_REG))
{
   ((struct Library *)base)->lib_OpenCnt++;
   ((struct Library *)base)->lib_Flags &= ~LIBF_DELEXP;

   return base;
}



/****i* gemimage.datatype/LibClose *****************************************
*
*   NAME
*	LibClose
*
*   SYNOPSIS
*	seg_list = LibClose()
*
*	APTR LibClose(VOID);
*
****************************************************************************
*
*/

APTR LibClose(struct GemImgBase *base REG(BASE_REG))
{
   APTR seg_list = NULL;

   /* Expunge the datatype if a delayed expunge is pending */

   if((--((struct Library *)base)->lib_OpenCnt) == 0)
   {
      if((((struct Library *)base)->lib_Flags & LIBF_DELEXP) != 0)
         seg_list = LibExpunge(base);
   }

   return seg_list;
}



/****i* gemimage.datatype/LibExpunge ***************************************
*
*   NAME
*	LibExpunge
*
*   SYNOPSIS
*	seg_list = LibExpunge()
*
*	APTR LibExpunge(VOID);
*
****************************************************************************
*
*/

APTR LibExpunge(struct GemImgBase *base REG(BASE_REG))
{
   APTR seg_list;

   if(((struct Library *)base)->lib_OpenCnt == 0)
   {
      seg_list = base->seg_list;
      Remove((APTR)base);
      DeleteDataType(base);
   }
   else
   {
      ((struct Library *)base)->lib_Flags |= LIBF_DELEXP;
      seg_list = NULL;
   }
   return seg_list;
}



/****i* gemimage.datatype/LibReserved **************************************
*
*   NAME
*	LibReserved
*
*   SYNOPSIS
*	result = LibReserved()
*
*	APTR LibReserved(VOID);
*
****************************************************************************
*
*/

APTR LibReserved(VOID)
{
   return NULL;
}



/****i* gemimage.datatype/ObtainClass **************************************
*
*   NAME
*	ObtainClass
*
*   SYNOPSIS
*	class = ObtainClass()
*
*	struct IClass *ObtainClass(VOID);
*
****************************************************************************
*
*/

struct IClass *ObtainClass(struct GemImgBase *base REG(BASE_REG))
{
   return base->class;
}



/****i* gemimage.datatype/DeleteDataType ***********************************
*
*   NAME
*	DeleteDataType
*
*   SYNOPSIS
*	DeleteDataType()
*
*	VOID DeleteDataType(VOID);
*
****************************************************************************
*
*/

static VOID DeleteDataType(struct GemImgBase *base)
{
   UWORD neg_size, pos_size;

   if(base->class != NULL)
      FreeClass(base->class);

   /* Close libraries */

   if(base->dos_base != NULL)
      CloseLibrary((APTR)base->dos_base);
   if(base->graphics_base != NULL)
      CloseLibrary((APTR)base->graphics_base);
   if(base->datatypes_base != NULL)
      CloseLibrary(base->datatypes_base);
   if(base->utility_base != NULL)
      CloseLibrary((APTR)base->utility_base);
   if(base->intuition_base != NULL)
      CloseLibrary((APTR)base->intuition_base);
   if(base->superclass_base != NULL)
      CloseLibrary(base->superclass_base);

   /* Free datatype's memory */

   neg_size = ((struct Library *)base)->lib_NegSize;
   pos_size = ((struct Library *)base)->lib_PosSize;
   FreeMem((UBYTE *)base - neg_size, pos_size + neg_size);

   return;
}



