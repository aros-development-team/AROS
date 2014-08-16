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

#include "lib.h"

LIBSTUB(CodesetsEncodeB64A, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsEncodeB64A, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsDecodeB64A, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsDecodeB64A, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsSupportedA, STRPTR *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsSupportedA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsFreeA, void)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsFreeA, (APTR)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsSetDefaultA, struct codeset *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsSetDefaultA, (STRPTR)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsFindA, struct codeset *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsFindA, (STRPTR)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsFindBestA, struct codeset *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsFindBestA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsUTF8Len, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsUTF8Len, (UTF8 *)REG_A0);
}

LIBSTUB(CodesetsStrLenA, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsStrLenA, (STRPTR)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsUTF8ToStrA, STRPTR)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsUTF8ToStrA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsUTF8CreateA, UTF8 *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsUTF8CreateA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsIsValidUTF8, BOOL)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsIsValidUTF8, (STRPTR)REG_A0);
}

LIBSTUB(CodesetsConvertStrA, STRPTR)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsConvertStrA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsFreeVecPooledA, void)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsFreeVecPooledA, (APTR)REG_A0,(APTR)REG_A1, (struct TagItem *)REG_A2);
}

LIBSTUB(CodesetsListCreateA, struct codesetList *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsListCreateA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsListDeleteA, BOOL)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsListDeleteA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsListAddA, BOOL)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsListAddA, (struct codesetList *)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(CodesetsListRemoveA, BOOL)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsListRemoveA, (struct TagItem *)REG_A0);
}

LIBSTUB(CodesetsConvertUTF32toUTF16, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsConvertUTF32toUTF16, (const UTF32 **)REG_A0, (const UTF32 *)REG_A1, (UTF16 **)REG_A2, (UTF16 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF16toUTF32, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsConvertUTF16toUTF32, (const UTF16 **)REG_A0, (const UTF16 *)REG_A1, (UTF32 **)REG_A2, (UTF32 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF16toUTF8, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsConvertUTF16toUTF8, (const UTF16 **)REG_A0, (const UTF16 *)REG_A1, (UTF8 **)REG_A2, (UTF8 *)REG_A3, (ULONG)REG_D0);
}


LIBSTUB(CodesetsIsLegalUTF8, BOOL)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsIsLegalUTF8, (const UTF8 *)REG_A0, (ULONG)REG_D0);
}

LIBSTUB(CodesetsIsLegalUTF8Sequence, BOOL)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsIsLegalUTF8Sequence, (const UTF8 *)REG_A0,(const UTF8 *)REG_A1);
}

LIBSTUB(CodesetsConvertUTF8toUTF16, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsConvertUTF8toUTF16, (const UTF8 **)REG_A0, (const UTF8 *)REG_A1, (UTF16 **)REG_A2, (UTF16 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF32toUTF8, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsConvertUTF32toUTF8, (const UTF32 **)REG_A0, (const UTF32 *)REG_A1, (UTF8 **)REG_A2, (UTF8 *)REG_A3, (ULONG)REG_D0);
}

LIBSTUB(CodesetsConvertUTF8toUTF32, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(CodesetsConvertUTF8toUTF32, (const UTF8 **)REG_A0, (const UTF8 *)REG_A1, (UTF32 **)REG_A2, (UTF32 *)REG_A3, (ULONG)REG_D0);
}
