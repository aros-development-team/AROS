/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2014 codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 Most of the code included in this file was relicensed from GPL to LGPL
 from the source code of SimpleMail (http://www.sf.net/projects/simplemail)
 with full permissions by its authors.

 $Id$

***************************************************************************/

#if defined(__VBCC__) || defined(NO_INLINE_STDARG)
#if !defined(__PPC__)

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

#include <proto/codesets.h>
STRPTR *CodesetsSupported(Tag tag1, ...)
{ return CodesetsSupportedA((struct TagItem *)&tag1); }
struct codeset *CodesetsFind(STRPTR name, Tag tag1, ...)
{ return CodesetsFindA(name, (struct TagItem *)&tag1); }
struct codeset *CodesetsFindBest(Tag tag1, ...)
{ return CodesetsFindBestA((struct TagItem *)&tag1); }
STRPTR CodesetsConvertStr(Tag tag1, ...)
{ return CodesetsConvertStrA((struct TagItem *)&tag1); }
BOOL CodesetsListDelete(Tag tag1, ...)
{ return CodesetsListDeleteA((struct TagItem *)&tag1); }
STRPTR CodesetsUTF8ToStr(Tag tag1, ...)
{ return CodesetsUTF8ToStrA((struct TagItem *)&tag1); }
UTF8 *CodesetsUTF8Create(Tag tag1, ...)
{ return CodesetsUTF8CreateA((struct TagItem *)&tag1); }
ULONG CodesetsDecodeB64(Tag tag1, ...)
{ return CodesetsDecodeB64A((struct TagItem *)&tag1); }
ULONG CodesetsEncodeB64(Tag tag1, ...)
{ return CodesetsEncodeB64A((struct TagItem *)&tag1); }

#else
  #error "VARGS stubs are only save on m68k systems!"
#endif
#endif
