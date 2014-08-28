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

#include <libraries/codesets.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <aros/libcall.h>
#include <SDI_lib.h>

#include "lib.h"

AROS_LH5(ULONG, CodesetsConvertUTF32toUTF16,
    AROS_LHA(const UTF32 **, ___sourceStart, A0),
    AROS_LHA(const UTF32 *, ___sourceEnd, A1),
    AROS_LHA(UTF16 **, ___targetStart, A2),
    AROS_LHA(UTF16 *, ___targetEnd, A3),
    AROS_LHA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsConvertUTF32toUTF16,___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(ULONG, CodesetsConvertUTF16toUTF32,
    AROS_LHA(const  UTF16 **, ___sourceStart, A0),
    AROS_LHA(const UTF16 *, ___sourceEnd, A1),
    AROS_LHA(UTF32 **, ___targetStart, A2),
    AROS_LHA(UTF32 *, ___targetEnd, A3),
    AROS_LHA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsConvertUTF16toUTF32,___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(ULONG, CodesetsConvertUTF16toUTF8,
    AROS_LHA(const UTF16 **, ___sourceStart, A0),
    AROS_LHA(const UTF16 *, ___sourceEnd, A1),
    AROS_LHA(UTF8 **, ___targetStart, A2),
    AROS_LHA(UTF8 *, ___targetEnd, A3),
    AROS_LHA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsConvertUTF16toUTF8,___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(BOOL, CodesetsIsLegalUTF8,
    AROS_LHA(const UTF8 *, ___source, A0),
    AROS_LHA(ULONG, ___length, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsIsLegalUTF8,___source, ___length);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(BOOL, CodesetsIsLegalUTF8Sequence,
    AROS_LHA(const UTF8 *, ___source, A0),
    AROS_LHA(const UTF8 *, ___sourceEnd, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsIsLegalUTF8Sequence,___source, ___sourceEnd);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(ULONG, CodesetsConvertUTF8toUTF16,
    AROS_LHA(const UTF8 **, ___sourceStart, A0),
    AROS_LHA(const UTF8 *, ___sourceEnd, A1),
    AROS_LHA(UTF16 **, ___targetStart, A2),
    AROS_LHA(UTF16 *, ___targetEnd, A3),
    AROS_LHA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsConvertUTF8toUTF16,___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(ULONG, CodesetsConvertUTF32toUTF8,
    AROS_LHA(const UTF32 **, ___sourceStart, A0),
    AROS_LHA(const UTF32 *, ___sourceEnd, A1),
    AROS_LHA(UTF8 **, ___targetStart, A2),
    AROS_LHA(UTF8 *, ___targetEnd, A3),
    AROS_LHA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsConvertUTF32toUTF8,___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(ULONG, CodesetsConvertUTF8toUTF32,
    AROS_LHA(const UTF8 **, ___sourceStart, A0),
    AROS_LHA(const UTF8 *, ___sourceEnd, A1),
    AROS_LHA(UTF32 **, ___targetStart, A2),
    AROS_LHA(UTF32 *, ___targetEnd, A3),
    AROS_LHA(ULONG, ___flags, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsConvertUTF8toUTF32,___sourceStart, ___sourceEnd, ___targetStart, ___targetEnd, ___flags);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct codeset *, CodesetsSetDefaultA,
    AROS_LHA(STRPTR, ___name, A0),
    AROS_LHA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsSetDefaultA,___name, ___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, CodesetsFreeA,
    AROS_LHA(APTR, ___obj, A0),
    AROS_LHA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsFreeA,___obj, ___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(STRPTR *, CodesetsSupportedA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsSupportedA,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct codeset *, CodesetsFindA,
    AROS_LHA(STRPTR, ___name, A0),
    AROS_LHA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsFindA,___name, ___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct codeset *, CodesetsFindBestA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsFindBestA,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, CodesetsUTF8Len,
    AROS_LHA(const UTF8 *, ___str, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsUTF8Len,___str);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(STRPTR, CodesetsUTF8ToStrA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsUTF8ToStrA,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(UTF8 *, CodesetsUTF8CreateA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsUTF8CreateA,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, CodesetsEncodeB64A,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsEncodeB64A,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, CodesetsDecodeB64A,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsDecodeB64A,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, CodesetsStrLenA,
    AROS_LHA(STRPTR, ___str, A0),
    AROS_LHA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsStrLenA,___str, ___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, CodesetsIsValidUTF8,
    AROS_LHA(STRPTR, ___str, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsIsValidUTF8,___str);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, CodesetsFreeVecPooledA,
    AROS_LHA(APTR, ___pool, A0),
    AROS_LHA(APTR, ___mem, A1),
    AROS_LHA(struct TagItem *, ___attrs, A2),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsFreeVecPooledA,___pool, ___mem, ___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(STRPTR, CodesetsConvertStrA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsConvertStrA,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct codesetList *, CodesetsListCreateA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsListCreateA,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, CodesetsListDeleteA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsListDeleteA,___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(BOOL, CodesetsListAddA,
    AROS_LHA(struct codesetList *, ___codesetsList, A0),
    AROS_LHA(struct TagItem *, ___attrs, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsListAddA,___codesetsList, ___attrs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, CodesetsListRemoveA,
    AROS_LHA(struct TagItem *, ___attrs, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(CodesetsListRemoveA,___attrs);
    AROS_LIBFUNC_EXIT
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
