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

#ifndef DATATYPE_H
#define DATATYPE_H


#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/classes.h>

#define DATATYPE_NAME "degas.datatype"
#define VERSION 1
#define REVISION 3
#define DATE "24.7.2011"

#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif
#endif

#define _STR(A) #A
#define STR(A) _STR(A)

struct DTBase
{
   struct Library library;
   APTR seg_list;
   struct IClass *class;
   struct ExecBase *sys_base;
   struct Library *superclass_base;
   struct IntuitionBase *intuition_base;
   struct UtilityBase *utility_base;
   struct Library *datatypes_base;
   struct GfxBase *graphics_base;
   struct DosLibrary *dos_base;
};


#ifdef __AROS__
#define BEWord(A) AROS_BE2WORD(A)
#define BELong(A) AROS_BE2LONG(A)
#else
#define BEWord(A) (A)
#define BELong(A) (A)
#endif


#define SysBase (base->sys_base)
#define IntuitionBase (base->intuition_base)
#define UtilityBase (base->utility_base)
#define DataTypesBase (base->datatypes_base)
#define GfxBase (base->graphics_base)
#define DOSBase (base->dos_base)

#ifndef BASE_REG
#define BASE_REG "a6"
#endif


#if defined(__mc68000) && !defined(__AROS__)
#define REG(A) __asm(A)
#else
#define REG(A)
#endif

#endif
