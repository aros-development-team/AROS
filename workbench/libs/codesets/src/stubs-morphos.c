/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2013 by codesets.library Open Source Team

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

#include "lib.h"

LIBSTUB(CodesetsEncodeB64A, ULONG, REG(a0, struct TagItem *attrs))
{
  return CodesetsEncodeB64A((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsDecodeB64A, ULONG, REG(a0, struct TagItem *attrs))
{
  return CodesetsDecodeB64A((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsSupportedA, STRPTR*, REG(a0, struct TagItem *attrs))
{
  return CodesetsSupportedA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsFreeA, void, REG(a0, APTR obj), REG(a1, struct TagItem *attrs))
{
  return CodesetsFreeA((APTR)REG_A0,(struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsSetDefaultA, struct codeset *, REG(a0, STRPTR name), REG(a1, struct TagItem *attrs))
{
  return CodesetsSetDefaultA((STRPTR)REG_A0,(struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsFindA, struct codeset *, REG(a0, STRPTR name), REG(a1, struct TagItem *attrs))
{
  return CodesetsFindA((STRPTR)REG_A0,(struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsFindBestA, struct codeset *, REG(a0, struct TagItem *attrs))
{
  return CodesetsFindBestA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsUTF8Len, ULONG, REG(a0, UTF8 *str))
{
  return CodesetsUTF8Len((UTF8 *)REG_A0);
}

LIBSTUB(CodesetsStrLenA, ULONG, REG(a0, STRPTR str),
                                REG(a1, struct TagItem *attrs))
{
  return CodesetsStrLenA((STRPTR)REG_A0,(struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsUTF8ToStrA, STRPTR, REG(a0, struct TagItem *attrs))
{
  return CodesetsUTF8ToStrA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsUTF8CreateA, UTF8*, REG(a0, struct TagItem *attrs))
{
  return CodesetsUTF8CreateA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsIsValidUTF8, BOOL, REG(a0, STRPTR s))
{
  return CodesetsIsValidUTF8((STRPTR)REG_A0);
}

LIBSTUB(CodesetsConvertStrA, STRPTR, REG(a0, struct TagItem *attrs))
{
  return CodesetsConvertStrA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsFreeVecPooledA, void, REG(a0, APTR pool),
                                      REG(a1, APTR mem),
                                      REG(a2, struct TagItem *attrs))
{
  return CodesetsFreeVecPooledA((APTR)REG_A0,(APTR)REG_A1,(struct TagItem *)REG_A2);
}

LIBSTUB(CodesetsListCreateA, struct codesetList *, REG(a0, struct TagItem *attrs))
{
  return CodesetsListCreateA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsListDeleteA, BOOL, REG(a0, struct TagItem *attrs))
{
  return CodesetsListDeleteA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsListAddA, BOOL, REG(a0, struct codesetList *csList), REG(a1, struct TagItem *attrs))
{
  return CodesetsListAddA((struct codesetList *)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsListRemoveA, BOOL, REG(a0, struct TagItem *attrs))
{
  return CodesetsListRemoveA((struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsConvertUTF32toUTF16, ULONG, REG(a0, const UTF32 ** sourceStart),
                                            REG(a1, const UTF32 * sourceEnd),
                                            REG(a2, UTF16 ** targetStart),
                                            REG(a3, UTF16 * targetEnd),
                                            REG(d0, ULONG flags))
{
  return CodesetsConvertUTF32toUTF16((const UTF32 **)REG_A0, (const UTF32 *)REG_A1, (UTF16 **)REG_A2, (UTF16 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF16toUTF32, ULONG, REG(a0, const UTF16 ** sourceStart),
                                            REG(a1, const UTF16 * sourceEnd),
                                            REG(a2, UTF32 ** targetStart),
                                            REG(a3, UTF32 * targetEnd),
                                            REG(d0, ULONG flags))
{
  return CodesetsConvertUTF16toUTF32((const UTF16 **)REG_A0, (const UTF16 *)REG_A1, (UTF32 **)REG_A2, (UTF32 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF16toUTF8, ULONG, REG(a0, const UTF16 ** sourceStart),
                                           REG(a1, const UTF16 * sourceEnd),
                                           REG(a2, UTF8 ** targetStart),
                                           REG(a3, UTF8 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF16toUTF8((const UTF16 **)REG_A0, (const UTF16 *)REG_A1, (UTF8 **)REG_A2, (UTF8 *)REG_A3, (ULONG)REG_D0);
}


LIBSTUB(CodesetsIsLegalUTF8, BOOL, REG(a0, const UTF8 * source),
                                   REG(d0, ULONG length))
{
  return CodesetsIsLegalUTF8((const UTF8 *)REG_A0,(ULONG)REG_D0);
}

LIBSTUB(CodesetsIsLegalUTF8Sequence, BOOL, REG(a0, const UTF8 * source),
                                            REG(a1, const UTF8 * sourceEnd))
{
  return CodesetsIsLegalUTF8Sequence((const UTF8 *)REG_A0,(const UTF8 *)REG_A1);
}

LIBSTUB(CodesetsConvertUTF8toUTF16, ULONG, REG(a0, const UTF8 ** sourceStart),
                                           REG(a1, const UTF8 * sourceEnd),
                                           REG(a2, UTF16 ** targetStart),
                                           REG(a3, UTF16 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF8toUTF16((const UTF8 **)REG_A0, (const UTF8 *)REG_A1, (UTF16 **)REG_A2, (UTF16 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF32toUTF8, ULONG, REG(a0, const UTF32 ** sourceStart),
                                           REG(a1, const UTF32 * sourceEnd),
                                           REG(a2, UTF8 ** targetStart),
                                           REG(a3, UTF8 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF32toUTF8((const UTF32 **)REG_A0, (const UTF32 *)REG_A1, (UTF8 **)REG_A2, (UTF8 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF8toUTF32, ULONG, REG(a0, const UTF8 ** sourceStart),
                                           REG(a1, const UTF8 * sourceEnd),
                                           REG(a2, UTF32 ** targetStart),
                                           REG(a3, UTF32 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF8toUTF32((const UTF8 **)REG_A0, (const UTF8 *)REG_A1, (UTF32 **)REG_A2, (UTF32 *)REG_A3, (ULONG)REG_D0);
}
