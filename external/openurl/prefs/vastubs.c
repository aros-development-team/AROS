/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

/*
   Stubs for the variable argument functions of the shared libraries used.
   Please note that these stubs should only be used if the compiler
   suite/SDK doesn't come with own stubs/inline functions.

   Also note that these stubs are only safe on m68k machines as it
   requires a linear stack layout!
*/

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

#include <proto/dos.h>
LONG Printf( CONST_STRPTR format, ... )
{ return VPrintf(format, (APTR)(&format+1)); }

#include <proto/openurl.h>
ULONG URL_Open(STRPTR url, Tag tag1, ...)
{ return URL_OpenA(url, (struct TagItem *)&tag1); }
struct URL_Prefs *URL_GetPrefs(Tag tag1, ...)
{ return URL_GetPrefsA((struct TagItem *)&tag1); }
ULONG URL_SetPrefs(struct URL_Prefs *prefs, Tag tag1, ...)
{ return URL_SetPrefsA(prefs, (struct TagItem *)&tag1); }

#include <proto/locale.h>
struct Catalog *OpenCatalog(struct Locale *locale, STRPTR name, Tag tag1, ...)
{ return OpenCatalogA(locale, name, (struct TagItem *)&tag1); }

#else
  #error "VARGS stubs are only save on m68k systems!"
#endif // !defined(__PPC__)

#endif // defined(__VBCC__) || defined(NO_INLINE_STDARG)
