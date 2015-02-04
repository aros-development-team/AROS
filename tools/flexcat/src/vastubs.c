/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

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

#include <proto/locale.h>
struct Catalog *OpenCatalog(struct Locale *locale, STRPTR name, Tag tag1, ...)
{ return OpenCatalogA(locale, name, (struct TagItem *)&tag1); }

#include <proto/codesets.h>
struct codeset *CodesetsFind(STRPTR name, Tag tag1, ...)
{ return CodesetsFindA(name, (struct TagItem *)&tag1); }
STRPTR CodesetsConvertStr(Tag tag1, ...)
{ return CodesetsConvertStrA((struct TagItem *)&tag1); }
STRPTR CodesetsUTF8ToStr(Tag tag1, ...)
{ return CodesetsUTF8ToStrA((struct TagItem *)&tag1); }

#else
  #error "VARGS stubs are only save on m68k systems!"
#endif // !defined(__PPC__)

#elif defined(__AROS__)

#include <proto/codesets.h>
STRPTR *CodesetsSupported(Tag tag1, ...)
{
#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE	  STRPTR *
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (STRPTR *)CodesetsSupportedA((struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

struct codeset *CodesetsFind(STRPTR name, Tag tag1, ...)
{
#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE	  struct codeset *
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (struct codeset *)CodesetsFindA(name, (struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

struct codeset *CodesetsFindBest(Tag tag1, ...)
{
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (struct codeset *)CodesetsFindBestA((struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

STRPTR CodesetsConvertStr(Tag tag1, ...)
{
#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE	  STRPTR
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (STRPTR)CodesetsConvertStrA((struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

struct codesetList *CodesetsListCreate(Tag tag1, ...)
{
#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE	  struct codesetList *
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (struct codesetList *)CodesetsListCreateA((struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

BOOL CodesetsListDelete(Tag tag1, ...)
{
#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE	  BOOL
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (BOOL)CodesetsListDeleteA((struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

STRPTR CodesetsUTF8ToStr(Tag tag1, ...)
{
#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE	  STRPTR
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (STRPTR)CodesetsUTF8ToStrA((struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

UTF8 *CodesetsUTF8Create(Tag tag1, ...)
{
#undef AROS_TAGRETURNTYPE
#define AROS_TAGRETURNTYPE	  UTF8 *
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = (UTF8 *)CodesetsUTF8CreateA((struct TagItem *)AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}

#endif // defined(__VBCC__) || defined(NO_INLINE_STDARG)
