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

#ifndef _LIB_PROTOS_H
#define _LIB_PROTOS_H

#include "SDI_lib.h"

/* init.c */
ULONG freeBase(struct LibraryHeader* lib);
ULONG initBase(struct LibraryHeader* lib);

/* utils.c */
#if defined(__amigaos4__)
  #define HAVE_ALLOCVECPOOLED 1
  #define HAVE_FREEVECPOOLED  1
  #define HAVE_GETHEAD        1
  #define HAVE_GETTAIL        1
  #define HAVE_GETPRED        1
  #define HAVE_GETSUCC        1
#elif defined(__MORPHOS__)
  #define HAVE_ALLOCVECPOOLED 1
  #define HAVE_FREEVECPOOLED  1
#elif defined(__AROS__)
  #define HAVE_ALLOCVECPOOLED 1
  #define HAVE_FREEVECPOOLED  1
  #define HAVE_GETHEAD        1
  #define HAVE_GETTAIL        1
  #define HAVE_GETPRED        1
  #define HAVE_GETSUCC        1
#endif

#if defined(HAVE_ALLOCVECPOOLED)
#define allocVecPooled(pool,size) AllocVecPooled(pool,size)
#else
APTR allocVecPooled(APTR pool, ULONG size);
#endif
#if defined(HAVE_FREEVECPOOLED)
#define freeVecPooled(pool,mem)   FreeVecPooled(pool,mem)
#else
void freeVecPooled(APTR pool, APTR mem);
#endif
APTR reallocVecPooled(APTR pool, APTR mem, ULONG oldSize, ULONG newSize);
APTR allocArbitrateVecPooled(ULONG size);
void freeArbitrateVecPooled(APTR mem);
APTR reallocArbitrateVecPooled(APTR mem, ULONG oldSize, ULONG newSize);
ULONG utf16_strlen(UTF16 *ptr);
ULONG utf32_strlen(UTF32 *ptr);
#if !defined(HAVE_GETHEAD)
struct Node *GetHead(struct List *list);
#endif
#if !defined(HAVE_GETPRED)
struct Node *GetPred(struct Node *node);
#endif
#if !defined(HAVE_GETSUCC)
struct Node *GetSucc(struct Node *node);
#endif
#if !defined(HAVE_GETTAIL)
struct Node *GetTail(struct List *list);
#endif

#include "base.h"

/* base64.c */
LIBPROTO(CodesetsEncodeB64A, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsEncodeB64, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsEncodeB64, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsDecodeB64A, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsDecodeB64, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsDecodeB64, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif


/* convertUTF.c */
LIBPROTO(CodesetsConvertUTF32toUTF16, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF32 **sourceStart), REG(a1, const UTF32 *sourceEnd), REG(a2, UTF16 **targetStart), REG(a3, UTF16 *targetEnd), REG(d0, ULONG flags));
LIBPROTO(CodesetsConvertUTF16toUTF32, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF16 **sourceStart), REG(a1, const UTF16 *sourceEnd), REG(a2, UTF32 **targetStart), REG(a3, UTF32 *targetEnd), REG(d0, ULONG flags));
LIBPROTO(CodesetsConvertUTF16toUTF8,  ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF16 **sourceStart), REG(a1, const UTF16 *sourceEnd) , REG(a2, UTF8 **targetStart), REG(a3, UTF8 *targetEnd), REG(d0, ULONG flags));
LIBPROTO(CodesetsIsLegalUTF8,         BOOL,  REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF8 *source), REG(d0, ULONG length));
LIBPROTO(CodesetsIsLegalUTF8Sequence, BOOL,  REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF8 *source), REG(a1, const UTF8 *sourceEnd));
LIBPROTO(CodesetsConvertUTF8toUTF16,  ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF8 **sourceStart), REG(a1, const UTF8 *sourceEnd), REG(a2, UTF16 **targetStart), REG(a3, UTF16 *targetEnd), REG(d0, ULONG flags));
LIBPROTO(CodesetsConvertUTF32toUTF8,  ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF32 **sourceStart), REG(a1, const UTF32 *sourceEnd), REG(a2, UTF8 **targetStart), REG(a3, UTF8 *targetEnd), REG(d0, ULONG flags));
LIBPROTO(CodesetsConvertUTF8toUTF32,  ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, const UTF8 **sourceStart), REG(a1, const UTF8 *sourceEnd), REG(a2, UTF32 **targetStart), REG(a3, UTF32 *targetEnd), REG(d0, ULONG flags));

/* codesets.c */
BOOL codesetsInit(struct codesetList *csList);
void codesetsCleanup(struct codesetList *csList);
struct codeset *codesetsFind(struct codesetList *csList, const char *name);

LIBPROTO(CodesetsSupportedA,  STRPTR *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsSupported, STRPTR *, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsSupported, STRPTR *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsFreeA,       void,  REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, APTR obj), REG(a1, struct TagItem *attrs));
LIBPROTOVA(CodesetsFree,      void,  REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, APTR obj), ...);
LIBPROTO(CodesetsSetDefaultA, struct codeset *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, STRPTR name), REG(a1, struct TagItem *attrs));
LIBPROTOVA(CodesetsSetDefault,struct codeset *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, STRPTR name), ...);
LIBPROTO(CodesetsFindA,       struct codeset *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, STRPTR name), REG(a1, struct TagItem *attrs));
LIBPROTOVA(CodesetsFind,      struct codeset *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, STRPTR name), ...);
LIBPROTO(CodesetsFindBestA,   struct codeset *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsFindBest,  struct codeset *, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsFindBest,  struct codeset *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsStrLenA,     ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, STRPTR str), REG(a1, struct TagItem *attrs));
LIBPROTOVA(CodesetsStrLen,    ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, STRPTR str), ...);
LIBPROTO(CodesetsConvertStrA, STRPTR, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsConvertStr, STRPTR, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsConvertStr, STRPTR, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsUTF8ToStrA,  STRPTR, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsUTF8ToStr, STRPTR, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsUTF8ToStr, STRPTR, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsUTF8CreateA, UTF8 *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsUTF8Create, UTF8 *, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsUTF8Create, UTF8 *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsFreeVecPooledA, void, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, APTR pool), REG(a1, APTR mem), REG(a2, struct TagItem *attrs));
LIBPROTOVA(CodesetsFreeVecPooled, void, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, APTR pool), REG(a1, APTR mem), ...);
LIBPROTO(CodesetsIsValidUTF8, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, STRPTR s));
LIBPROTO(CodesetsUTF8Len,     ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, UTF8 *str));
LIBPROTO(CodesetsListCreateA, struct codesetList *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsListCreate, struct codesetList *, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsListCreate, struct codesetList *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsListDeleteA, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsListDelete, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsListDelete, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif
LIBPROTO(CodesetsListAddA, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct codesetList *csList), REG(a1, struct TagItem *attrs));
LIBPROTOVA(CodesetsListAdd, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct codesetList *csList), ...);
LIBPROTO(CodesetsListRemoveA, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(CodesetsListRemove, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), ...);
#else
LIBPROTOVA(CodesetsListRemove, BOOL, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, Tag tag1), ...);
#endif

#if defined(__AROS__)
AROS_LD5(ULONG, CodesetsConvertUTF32toUTF16,
    AROS_LDA(const UTF32 **, ___sourceStart, A0),
    AROS_LDA(const UTF32 *, ___sourceEnd, A1),
    AROS_LDA(UTF16 **, ___targetStart, A2),
    AROS_LDA(UTF16 *, ___targetEnd, A3),
    AROS_LDA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD5(ULONG, CodesetsConvertUTF16toUTF32,
    AROS_LDA(const  UTF16 **, ___sourceStart, A0),
    AROS_LDA(const UTF16 *, ___sourceEnd, A1),
    AROS_LDA(UTF32 **, ___targetStart, A2),
    AROS_LDA(UTF32 *, ___targetEnd, A3),
    AROS_LDA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD5(ULONG, CodesetsConvertUTF16toUTF8,
    AROS_LDA(const UTF16 **, ___sourceStart, A0),
    AROS_LDA(const UTF16 *, ___sourceEnd, A1),
    AROS_LDA(UTF8 **, ___targetStart, A2),
    AROS_LDA(UTF8 *, ___targetEnd, A3),
    AROS_LDA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD2(BOOL, CodesetsIsLegalUTF8,
    AROS_LDA(const UTF8 *, ___source, A0),
    AROS_LDA(ULONG, ___length, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD2(BOOL, CodesetsIsLegalUTF8Sequence,
    AROS_LDA(const UTF8 *, ___source, A0),
    AROS_LDA(const UTF8 *, ___sourceEnd, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD5(ULONG, CodesetsConvertUTF8toUTF16,
    AROS_LDA(const UTF8 **, ___sourceStart, A0),
    AROS_LDA(const UTF8 *, ___sourceEnd, A1),
    AROS_LDA(UTF16 **, ___targetStart, A2),
    AROS_LDA(UTF16 *, ___targetEnd, A3),
    AROS_LDA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD5(ULONG, CodesetsConvertUTF32toUTF8,
    AROS_LDA(const UTF32 **, ___sourceStart, A0),
    AROS_LDA(const UTF32 *, ___sourceEnd, A1),
    AROS_LDA(UTF8 **, ___targetStart, A2),
    AROS_LDA(UTF8 *, ___targetEnd, A3),
    AROS_LDA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD5(ULONG, CodesetsConvertUTF8toUTF32,
    AROS_LDA(const UTF8 **, ___sourceStart, A0),
    AROS_LDA(const UTF8 *, ___sourceEnd, A1),
    AROS_LDA(UTF32 **, ___targetStart, A2),
    AROS_LDA(UTF32 *, ___targetEnd, A3),
    AROS_LDA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD2(struct codeset *, CodesetsSetDefaultA,
    AROS_LDA(STRPTR, ___name, A0),
    AROS_LDA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD2(void, CodesetsFreeA,
    AROS_LDA(APTR, ___obj, A0),
    AROS_LDA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(STRPTR *, CodesetsSupportedA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD2(struct codeset *, CodesetsFindA,
    AROS_LDA(STRPTR, ___name, A0),
    AROS_LDA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(struct codeset *, CodesetsFindBestA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(ULONG, CodesetsUTF8Len,
    AROS_LDA(const UTF8 *, ___str, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(STRPTR, CodesetsUTF8ToStrA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(UTF8 *, CodesetsUTF8CreateA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(ULONG, CodesetsEncodeB64A,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(ULONG, CodesetsDecodeB64A,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD2(ULONG, CodesetsStrLenA,
    AROS_LDA(STRPTR, ___str, A0),
    AROS_LDA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(BOOL, CodesetsIsValidUTF8,
    AROS_LDA(STRPTR, ___str, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD3(void, CodesetsFreeVecPooledA,
    AROS_LDA(APTR, ___pool, A0),
    AROS_LDA(APTR, ___mem, A1),
    AROS_LDA(struct TagItem *, ___attrs, A2),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(STRPTR, CodesetsConvertStrA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(struct codesetList *, CodesetsListCreateA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(BOOL, CodesetsListDeleteA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD2(BOOL, CodesetsListAddA,
    AROS_LDA(struct codesetList *, ___codesetsList, A0),
    AROS_LDA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

AROS_LD1(BOOL, CodesetsListRemoveA,
    AROS_LDA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB);

#endif

#endif /* _LIB_PROTOS_H */
