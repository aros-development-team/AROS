#ifndef CLIB_CODESETS_PROTOS_H
#define CLIB_CODESETS_PROTOS_H

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

#ifndef LIBRARIES_CODESETS_H
#include <libraries/codesets.h>
#endif

STRPTR *CodesetsSupportedA(struct TagItem *attrs);
STRPTR *CodesetsSupported(Tag tag1 , ...);
void CodesetsFreeA(APTR obj, struct TagItem *attrs);
void CodesetsFree(APTR obj, Tag tag1, ...);
struct codeset *CodesetsSetDefaultA(STRPTR name, struct TagItem *attrs);
struct codeset *CodesetsSetDefault(STRPTR name, Tag tag1, ...);
struct codeset *CodesetsFindA(STRPTR name, struct TagItem *attrs);
struct codeset *CodesetsFind(STRPTR name, Tag tag1, ...);
struct codeset *CodesetsFindBestA(struct TagItem *attrs);
struct codeset *CodesetsFindBest(Tag tag1, ...);
ULONG CodesetsUTF8Len(const UTF8 *str);
ULONG CodesetsStrLenA(STRPTR str, struct TagItem *attrs);
ULONG CodesetsStrLen(STRPTR str, Tag tag1, ...);
STRPTR CodesetsUTF8ToStrA(struct TagItem *attrs);
STRPTR CodesetsUTF8ToStr(Tag tag1, ...);
UTF8 *CodesetsUTF8CreateA(struct TagItem *attrs);
UTF8 *CodesetsUTF8Create(Tag tag1, ...);
BOOL CodesetsIsValidUTF8(STRPTR str);
void CodesetsFreeVecPooledA(APTR pool, APTR mem, struct TagItem *attrs);
void CodesetsFreeVecPooled(APTR pool, APTR mem, Tag tag1, ...);
STRPTR CodesetsConvertStrA(struct TagItem *attrs);
STRPTR CodesetsConvertStr(Tag tag1, ...);
struct codesetList *CodesetsListCreateA(struct TagItem *attrs);
struct codesetList *CodesetsListCreate(Tag tag1, ...);
BOOL CodesetsListDeleteA(struct TagItem *attrs);
BOOL CodesetsListDelete(Tag tag1, ...);
BOOL CodesetsListAddA(struct codesetList *csList, struct TagItem *attrs);
BOOL CodesetsListAdd(struct codesetList *csList, ...);
BOOL CodesetsListRemoveA(struct TagItem *attrs);
BOOL CodesetsListRemove(Tag tag1, ...);

ULONG CodesetsConvertUTF32toUTF16(const UTF32 **sourceStart, const UTF32 *sourceEnd, UTF16 **targetStart, UTF16 *targetEnd, ULONG flags);
ULONG CodesetsConvertUTF16toUTF32(const UTF16 **sourceStart, const UTF16 *sourceEnd, UTF32 **targetStart, UTF32 *targetEnd, ULONG flags);
ULONG CodesetsConvertUTF16toUTF8(const UTF16 **sourceStart, const UTF16 *sourceEnd, UTF8 **targetStart, UTF8 *targetEnd, ULONG flags);
BOOL CodesetsIsLegalUTF8(const UTF8 *source, ULONG length);
BOOL CodesetsIsLegalUTF8Sequence(const UTF8 *source, const UTF8 *sourceEnd);
ULONG CodesetsConvertUTF8toUTF16(const UTF8 **sourceStart, const UTF8 *sourceEnd, UTF16 **targetStart, UTF16 *targetEnd, ULONG flags);
ULONG CodesetsConvertUTF32toUTF8(const UTF32 **sourceStart, const UTF32 *sourceEnd, UTF8 **targetStart, UTF8 *targetEnd, ULONG flags);
ULONG CodesetsConvertUTF8toUTF32(const UTF8 **sourceStart, const UTF8 *sourceEnd, UTF32 **targetStart, UTF32 *targetEnd, ULONG flags);

ULONG CodesetsEncodeB64A(struct TagItem *attrs);
ULONG CodesetsEncodeB64(Tag tag1, ...);
ULONG CodesetsDecodeB64A(struct TagItem *attrs);
ULONG CodesetsDecodeB64(Tag tag1, ...);

#endif /* CLIB_CODESETS_PROTOS_H */
