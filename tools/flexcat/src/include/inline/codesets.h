#ifndef _INLINE_CODESETS_H
#define _INLINE_CODESETS_H

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

 $Id$

***************************************************************************/

#ifndef CLIB_CODESETS_PROTOS_H
#define CLIB_CODESETS_PROTOS_H
#endif

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef  LIBRARIES_CODESETS_H
#include <libraries/codesets.h>
#endif

#ifndef CODESETS_BASE_NAME
#define CODESETS_BASE_NAME CodesetsBase
#endif

#define CodesetsConvertUTF32toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags) \
	LP5(0x24, ULONG, CodesetsConvertUTF32toUTF16, const , sourceStart, a0, const , sourceEnd, a1, UTF16 **, targetStart, a2, UTF16 *, targetEnd, a3, ULONG, flags, d0, \
	, CODESETS_BASE_NAME)

#define CodesetsConvertUTF16toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags) \
	LP5(0x2a, ULONG, CodesetsConvertUTF16toUTF32, const , sourceStart, a0, const , sourceEnd, a1, UTF32 **, targetStart, a2, UTF32 *, targetEnd, a3, ULONG, flags, d0, \
	, CODESETS_BASE_NAME)

#define CodesetsConvertUTF16toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags) \
	LP5(0x30, ULONG, CodesetsConvertUTF16toUTF8, const , sourceStart, a0, const , sourceEnd, a1, UTF8 **, targetStart, a2, UTF8 *, targetEnd, a3, ULONG, flags, d0, \
	, CODESETS_BASE_NAME)

#define CodesetsIsLegalUTF8(source, length) \
	LP2(0x36, BOOL, CodesetsIsLegalUTF8, const , source, a0, ULONG, length, d0, \
	, CODESETS_BASE_NAME)

#define CodesetsIsLegalUTF8Sequence(source, sourceEnd) \
	LP2(0x3c, BOOL, CodesetsIsLegalUTF8Sequence, const , source, a0, const , sourceEnd, a1, \
	, CODESETS_BASE_NAME)

#define CodesetsConvertUTF8toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags) \
	LP5(0x42, ULONG, CodesetsConvertUTF8toUTF16, const , sourceStart, a0, const , sourceEnd, a1, UTF16 **, targetStart, a2, UTF16 *, targetEnd, a3, ULONG, flags, d0, \
	, CODESETS_BASE_NAME)

#define CodesetsConvertUTF32toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags) \
	LP5(0x48, ULONG, CodesetsConvertUTF32toUTF8, const , sourceStart, a0, const , sourceEnd, a1, UTF8 **, targetStart, a2, UTF8 *, targetEnd, a3, ULONG, flags, d0, \
	, CODESETS_BASE_NAME)

#define CodesetsConvertUTF8toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags) \
	LP5(0x4e, ULONG, CodesetsConvertUTF8toUTF32, const , sourceStart, a0, const , sourceEnd, a1, UTF32 **, targetStart, a2, UTF32 *, targetEnd, a3, ULONG, flags, d0, \
	, CODESETS_BASE_NAME)

#define CodesetsSetDefaultA(name, attrs) \
	LP2(0x54, struct codeset *, CodesetsSetDefaultA, STRPTR, name, a0, struct TagItem *, attrs, a1, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsSetDefault(name, tags...) \
	({ULONG _tags[] = {tags}; CodesetsSetDefaultA((name), (struct TagItem *) _tags);})
#endif

#define CodesetsFreeA(obj, attrs) \
	LP2NR(0x5a, CodesetsFreeA, APTR, obj, a0, struct TagItem *, attrs, a1, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsFree(obj, tags...) \
	({ULONG _tags[] = {tags}; CodesetsFreeA((obj), (struct TagItem *) _tags);})
#endif

#define CodesetsSupportedA(attrs) \
	LP1(0x60, STRPTR *, CodesetsSupportedA, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsSupported(tags...) \
	({ULONG _tags[] = {tags}; CodesetsSupportedA((struct TagItem *) _tags);})
#endif

#define CodesetsFindA(name, attrs) \
	LP2(0x66, struct codeset *, CodesetsFindA, STRPTR, name, a0, struct TagItem *, attrs, a1, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsFind(name, tags...) \
	({ULONG _tags[] = {tags}; CodesetsFindA((name), (struct TagItem *) _tags);})
#endif

#define CodesetsFindBestA(attrs) \
	LP1(0x6c, struct codeset *, CodesetsFindBestA, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsFindBest(tags...) \
	({ULONG _tags[] = {tags}; CodesetsFindBestA((struct TagItem *) _tags);})
#endif

#define CodesetsUTF8Len(str) \
	LP1(0x72, ULONG, CodesetsUTF8Len, const , str, a0, \
	, CODESETS_BASE_NAME)

#define CodesetsUTF8ToStrA(attrs) \
	LP1(0x78, STRPTR, CodesetsUTF8ToStrA, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsUTF8ToStr(tags...) \
	({ULONG _tags[] = {tags}; CodesetsUTF8ToStrA((struct TagItem *) _tags);})
#endif

#define CodesetsUTF8CreateA(attrs) \
	LP1(0x7e, UTF8 *, CodesetsUTF8CreateA, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsUTF8Create(tags...) \
	({ULONG _tags[] = {tags}; CodesetsUTF8CreateA((struct TagItem *) _tags);})
#endif

#define CodesetsEncodeB64A(attrs) \
	LP1(0x84, ULONG, CodesetsEncodeB64A, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsEncodeB64(tags...) \
	({ULONG _tags[] = {tags}; CodesetsEncodeB64A((struct TagItem *) _tags);})
#endif

#define CodesetsDecodeB64A(attrs) \
	LP1(0x8a, ULONG, CodesetsDecodeB64A, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsDecodeB64(tags...) \
	({ULONG _tags[] = {tags}; CodesetsDecodeB64A((struct TagItem *) _tags);})
#endif

#define CodesetsStrLenA(str, attrs) \
	LP2(0x90, ULONG, CodesetsStrLenA, STRPTR, str, a0, struct TagItem *, attrs, a1, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsStrLen(str, tags...) \
	({ULONG _tags[] = {tags}; CodesetsStrLenA((str), (struct TagItem *) _tags);})
#endif

#define CodesetsIsValidUTF8(str) \
	LP1(0x96, BOOL, CodesetsIsValidUTF8, STRPTR, str, a0, \
	, CODESETS_BASE_NAME)

#define CodesetsFreeVecPooledA(pool, mem, attrs) \
	LP3NR(0x9c, CodesetsFreeVecPooledA, APTR, pool, a0, APTR, mem, a1, struct TagItem *, attrs, a2, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsFreeVecPooled(pool, mem, tags...) \
	({ULONG _tags[] = {tags}; CodesetsFreeVecPooledA((pool), (mem), (struct TagItem *) _tags);})
#endif

#define CodesetsConvertStrA(str) \
	LP1(0xa2, STRPTR, CodesetsConvertStrA, struct TagItem *, str, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsConvertStr(tags...) \
	({ULONG _tags[] = {tags}; CodesetsConvertStrA((struct TagItem *) _tags);})
#endif

#define CodesetsListCreateA(attrs) \
	LP1(0xa8, struct codesetList *, CodesetsListCreateA, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsListCreate(tags...) \
	({ULONG _tags[] = {tags}; CodesetsListCreateA((struct TagItem *) _tags);})
#endif

#define CodesetsListDeleteA(attrs) \
	LP1(0xae, BOOL, CodesetsListDeleteA, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsListDelete(tags...) \
	({ULONG _tags[] = {tags}; CodesetsListDeleteA((struct TagItem *) _tags);})
#endif

#define CodesetsListAddA(list, attrs) \
	LP2(0xb4, BOOL, CodesetsListAddA, struct codesetList *, list, a0, struct TagItem *, attrs, a1, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsListAdd(list, tags...) \
	({ULONG _tags[] = {tags}; CodesetsListAddA((list), (struct TagItem *) _tags);})
#endif

#define CodesetsListRemoveA(attrs) \
	LP1(0xba, BOOL, CodesetsListRemoveA, struct TagItem *, attrs, a0, \
	, CODESETS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CodesetsListRemove(tags...) \
	({ULONG _tags[] = {tags}; CodesetsListRemoveA((struct TagItem *) _tags);})
#endif

#endif /*  _INLINE_CODESETS_H  */
