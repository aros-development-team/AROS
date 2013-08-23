/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2007-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#if !defined(__AROS__) && (defined(__VBCC__) || defined(NO_INLINE_STDARG))
#if defined(_M68000) || defined(__M68000) || defined(__mc68000)

#include <exec/types.h>

/* FIX V45 breakage... */
#if INCLUDE_VERSION < 45
#define MY_CONST_STRPTR CONST_STRPTR
#else
#define MY_CONST_STRPTR CONST STRPTR
#endif

#include <proto/intuition.h>
APTR NewObject( struct IClass *classPtr, CONST_STRPTR classID, Tag tag1, ... )
{ return NewObjectA(classPtr, classID, (struct TagItem *)&tag1); }
ULONG SetAttrs( APTR object, ULONG tag1, ... )
{ return SetAttrsA(object, (struct TagItem *)&tag1); }

#include <proto/graphics.h>
LONG ObtainBestPen( struct ColorMap *cm, ULONG r, ULONG g, ULONG b, Tag tag1, ... )
{ return ObtainBestPenA(cm, r, g, b, (struct TagItem *)&tag1); }

#include <proto/datatypes.h>
Object *NewDTObject( APTR name, Tag tag1, ... )
{ return NewDTObjectA(name, (struct TagItem *)&tag1); }
ULONG SetDTAttrs( Object *o, struct Window *win, struct Requester *req, Tag tag1, ... )
{ return SetDTAttrsA(o, win, req, (struct TagItem *)&tag1); }
ULONG GetDTAttrs( Object *o, Tag tag1, ... )
{ return GetDTAttrsA(o, (struct TagItem *)&tag1); }
ULONG DoDTMethod( Object *o, struct Window *win, struct Requester *req, ULONG data, ... )
{ return DoDTMethodA(o, win, req, (Msg)&data); }

#else
  #error "VARGS stubs are only save on m68k systems!"
#endif
#endif
