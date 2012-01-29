/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2007 by codesets.library Open Source Team

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

/* Automatically generated header! Do not edit! */

#ifndef _PPCINLINE_CODESETS_H
#define _PPCINLINE_CODESETS_H

#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif /* !__PPCINLINE_MACROS_H */

#ifndef CODESETS_BASE_NAME
#define CODESETS_BASE_NAME CodesetsBase
#endif /* !CODESETS_BASE_NAME */

#define CodesetsSupportedA(__p0) \
	LP1(96, STRPTR *, CodesetsSupportedA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsIsLegalUTF8(__p0, __p1) \
	LP2(54, BOOL , CodesetsIsLegalUTF8, \
		const UTF8 *, __p0, a0, \
		ULONG , __p1, d0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsListRemoveA(__p0) \
	LP1(186, BOOL , CodesetsListRemoveA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsFindBestA(__p0) \
	LP1(108, struct codeset *, CodesetsFindBestA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsListCreateA(__p0) \
	LP1(168, struct codesetList *, CodesetsListCreateA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsDecodeB64A(__p0) \
	LP1(138, ULONG , CodesetsDecodeB64A, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsIsValidUTF8(__p0) \
	LP1(150, BOOL , CodesetsIsValidUTF8, \
		STRPTR , __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsStrLenA(__p0, __p1) \
	LP2(144, ULONG , CodesetsStrLenA, \
		STRPTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsConvertStrA(__p0) \
	LP1(162, STRPTR , CodesetsConvertStrA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsFindA(__p0, __p1) \
	LP2(102, struct codeset *, CodesetsFindA, \
		STRPTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsFreeA(__p0, __p1) \
	LP2NR(90, CodesetsFreeA, \
		APTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsUTF8ToStrA(__p0) \
	LP1(120, STRPTR , CodesetsUTF8ToStrA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsFreeVecPooledA(__p0, __p1, __p2) \
	LP3NR(156, CodesetsFreeVecPooledA, \
		APTR , __p0, a0, \
		APTR , __p1, a1, \
		struct TagItem *, __p2, a2, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsUTF8Len(__p0) \
	LP1(114, ULONG , CodesetsUTF8Len, \
		const UTF8 *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsListAddA(__p0, __p1) \
	LP2(180, BOOL , CodesetsListAddA, \
		struct codesetList *, __p0, a0, \
		struct TagItem *, __p1, a1, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsConvertUTF32toUTF16(__p0, __p1, __p2, __p3, __p4) \
	LP5(36, ULONG , CodesetsConvertUTF32toUTF16, \
		const UTF32 **, __p0, a0, \
		const UTF32 *, __p1, a1, \
		UTF16 **, __p2, a2, \
		UTF16 *, __p3, a3, \
		ULONG , __p4, d0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsConvertUTF16toUTF32(__p0, __p1, __p2, __p3, __p4) \
	LP5(42, ULONG , CodesetsConvertUTF16toUTF32, \
		const UTF16 **, __p0, a0, \
		const UTF16 *, __p1, a1, \
		UTF32 **, __p2, a2, \
		UTF32 *, __p3, a3, \
		ULONG , __p4, d0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsListDeleteA(__p0) \
	LP1(174, BOOL , CodesetsListDeleteA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsSetDefaultA(__p0, __p1) \
	LP2(84, struct codeset *, CodesetsSetDefaultA, \
		STRPTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsEncodeB64A(__p0) \
	LP1(132, ULONG , CodesetsEncodeB64A, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsUTF8CreateA(__p0) \
	LP1(126, UTF8 *, CodesetsUTF8CreateA, \
		struct TagItem *, __p0, a0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsConvertUTF32toUTF8(__p0, __p1, __p2, __p3, __p4) \
	LP5(72, ULONG , CodesetsConvertUTF32toUTF8, \
		const UTF32 **, __p0, a0, \
		const UTF32 *, __p1, a1, \
		UTF8 **, __p2, a2, \
		UTF8 *, __p3, a3, \
		ULONG , __p4, d0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsConvertUTF8toUTF32(__p0, __p1, __p2, __p3, __p4) \
	LP5(78, ULONG , CodesetsConvertUTF8toUTF32, \
		const UTF8 **, __p0, a0, \
		const UTF8 *, __p1, a1, \
		UTF32 **, __p2, a2, \
		UTF32 *, __p3, a3, \
		ULONG , __p4, d0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsConvertUTF16toUTF8(__p0, __p1, __p2, __p3, __p4) \
	LP5(48, ULONG , CodesetsConvertUTF16toUTF8, \
		const UTF16 **, __p0, a0, \
		const UTF16 *, __p1, a1, \
		UTF8 **, __p2, a2, \
		UTF8 *, __p3, a3, \
		ULONG , __p4, d0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsIsLegalUTF8Sequence(__p0, __p1) \
	LP2(60, BOOL , CodesetsIsLegalUTF8Sequence, \
		const UTF8 *, __p0, a0, \
		const UTF8 *, __p1, a1, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CodesetsConvertUTF8toUTF16(__p0, __p1, __p2, __p3, __p4) \
	LP5(66, ULONG , CodesetsConvertUTF8toUTF16, \
		const UTF8 **, __p0, a0, \
		const UTF8 *, __p1, a1, \
		UTF16 **, __p2, a2, \
		UTF16 *, __p3, a3, \
		ULONG , __p4, d0, \
		, CODESETS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#if defined(USE_INLINE_STDARG) && !defined(__STRICT_ANSI__)

#include <stdarg.h>

#define CodesetsSupported(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsSupportedA((struct TagItem *)_tags);})

#define CodesetsListRemove(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsListRemoveA((struct TagItem *)_tags);})

#define CodesetsListDelete(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsListDeleteA((struct TagItem *)_tags);})

#define CodesetsDecodeB64(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsDecodeB64A((struct TagItem *)_tags);})

#define CodesetsListCreate(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsListCreateA((struct TagItem *)_tags);})

#define CodesetsSetDefault(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsSetDefaultA(__p0, (struct TagItem *)_tags);})

#define CodesetsStrLen(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsStrLenA(__p0, (struct TagItem *)_tags);})

#define CodesetsConvertStr(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsConvertStrA((struct TagItem *)_tags);})

#define CodesetsUTF8Create(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsUTF8CreateA((struct TagItem *)_tags);})

#define CodesetsFree(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsFreeA(__p0, (struct TagItem *)_tags);})

#define CodesetsFindBest(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsFindBestA((struct TagItem *)_tags);})

#define CodesetsEncodeB64(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsEncodeB64A((struct TagItem *)_tags);})

#define CodesetsFind(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsFindA(__p0, (struct TagItem *)_tags);})

#define CodesetsFreeVecPooled(__p0, __p1, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsFreeVecPooledA(__p0, __p1, (struct TagItem *)_tags);})

#define CodesetsUTF8ToStr(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsUTF8ToStrA((struct TagItem *)_tags);})

#define CodesetsListAdd(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	CodesetsListAddA(__p0, (struct TagItem *)_tags);})

#endif

#endif /* !_PPCINLINE_CODESETS_H */
