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
#include "SDI_stdarg.h"

LIBSTUB(CodesetsEncodeB64A, ULONG, REG(a0, struct TagItem *attrs))
{
  return CodesetsEncodeB64A(attrs);
}

LIBSTUBVA(CodesetsEncodeB64, ULONG, ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, self);
  res = CodesetsEncodeB64A(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsDecodeB64A, ULONG, REG(a0, struct TagItem *attrs))
{
  return CodesetsDecodeB64A(attrs);
}

LIBSTUBVA(CodesetsDecodeB64, ULONG, ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, self);
  res = CodesetsDecodeB64A(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsSupportedA, STRPTR*, REG(a0, struct TagItem *attrs))
{
  return CodesetsSupportedA(attrs);
}

LIBSTUBVA(CodesetsSupported, STRPTR*, ...)
{
  STRPTR* res;
  VA_LIST args;

  VA_START(args, self);
  res = CodesetsSupportedA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsFreeA, void, REG(a0, APTR obj), REG(a1, struct TagItem *attrs))
{
  return CodesetsFreeA(obj, attrs);
}

LIBSTUBVA(CodesetsFree, void, REG(a0, APTR obj), ...)
{
  VA_LIST args;

  VA_START(args, obj);
  CodesetsFreeA(obj, VA_ARG(args, struct TagItem *));
  VA_END(args);
}

LIBSTUB(CodesetsSetDefaultA, struct codeset *, REG(a0, STRPTR name), REG(a1, struct TagItem *attrs))
{
  return CodesetsSetDefaultA(name, attrs);
}

LIBSTUBVA(CodesetsSetDefault, struct codeset *, REG(a0, STRPTR name), ...)
{
  struct codeset *cs;
  VA_LIST args;

  VA_START(args, name);
  cs = CodesetsSetDefaultA(name, VA_ARG(args, struct TagItem *));
  VA_END(args);

  return cs;
}

LIBSTUB(CodesetsFindA, struct codeset *, REG(a0, STRPTR name), REG(a1, struct TagItem *attrs))
{
  return CodesetsFindA(name, attrs);
}

LIBSTUBVA(CodesetsFind, struct codeset *, REG(a0, STRPTR name), ...)
{
  struct codeset *cs;
  VA_LIST args;

  VA_START(args, name);
  cs = CodesetsFindA(name, VA_ARG(args, struct TagItem *));
  VA_END(args);

  return cs;
}

LIBSTUB(CodesetsFindBestA, struct codeset *, REG(a0, struct TagItem *attrs))
{
  return CodesetsFindBestA(attrs);
}

LIBSTUBVA(CodesetsFindBest, struct codeset *, ...)
{
  struct codeset *cs;
  VA_LIST args;

  VA_START(args, self);
  cs = CodesetsFindBestA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return cs;
}

LIBSTUB(CodesetsUTF8Len, ULONG, REG(a0, UTF8 *str))
{
  return CodesetsUTF8Len(str);
}

LIBSTUB(CodesetsStrLenA, ULONG, REG(a0, STRPTR str),
                                REG(a1, struct TagItem *attrs))
{
  return CodesetsStrLenA(str, attrs);
}

LIBSTUBVA(CodesetsStrLen, ULONG, REG(a0, STRPTR str), ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, str);
  res = CodesetsStrLenA(str, VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsUTF8ToStrA, STRPTR, REG(a0, struct TagItem *attrs))
{
  return CodesetsUTF8ToStrA(attrs);
}

LIBSTUBVA(CodesetsUTF8ToStr, STRPTR, ...)
{
  STRPTR res;
  VA_LIST args;

  VA_START(args, self);
  res = CodesetsUTF8ToStrA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsUTF8CreateA, UTF8*, REG(a0, struct TagItem *attrs))
{
  return CodesetsUTF8CreateA(attrs);
}

LIBSTUBVA(CodesetsUTF8Create, UTF8*, ...)
{
  UTF8 *res;
  VA_LIST args;

  VA_START(args, self);
  res = CodesetsUTF8CreateA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsIsValidUTF8, BOOL, REG(a0, STRPTR s))
{
  return CodesetsIsValidUTF8(s);
}

LIBSTUB(CodesetsConvertStrA, STRPTR, REG(a0, struct TagItem *attrs))
{
  return CodesetsConvertStrA(attrs);
}

LIBSTUBVA(CodesetsConvertStr, STRPTR, ...)
{
  STRPTR res;
  VA_LIST args;

  VA_START(args, self);
  res = CodesetsConvertStrA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsFreeVecPooledA, void, REG(a0, APTR pool),
                                      REG(a1, APTR mem),
                                      REG(a2, struct TagItem *attrs))
{
  return CodesetsFreeVecPooledA(pool, mem, attrs);
}

LIBSTUBVA(CodesetsFreeVecPooled, void, REG(a0, APTR pool),
                                       REG(a1, APTR mem), ...)
{
  VA_LIST args;

  VA_START(args, mem);
  CodesetsFreeVecPooledA(pool, mem, VA_ARG(args, struct TagItem *));
  VA_END(args);
}

LIBSTUB(CodesetsListCreateA, struct codesetList *, REG(a0, struct TagItem *attrs))
{
  return CodesetsListCreateA(attrs);
}

LIBSTUBVA(CodesetsListCreate, struct codesetList *, ...)
{
  struct codesetList *res;
  VA_LIST args;

  VA_START(args, self);
  res = CodesetsListCreateA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(CodesetsListDeleteA, BOOL, REG(a0, struct TagItem *attrs))
{
  return CodesetsListDeleteA(attrs);
}

LIBSTUBVA(CodesetsListDelete, BOOL, ...)
{
  BOOL result;
  VA_LIST args;

  VA_START(args, self);
  result = CodesetsListDeleteA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return result;
}

LIBSTUB(CodesetsListAddA, BOOL, REG(a0, struct codesetList *csList), REG(a1, struct TagItem *attrs))
{
  return CodesetsListAddA(csList, attrs);
}

LIBSTUBVA(CodesetsListAdd, BOOL, struct codesetList *csList, ...)
{
  BOOL result;
  VA_LIST args;

  VA_START(args, csList);
  result = CodesetsListAddA(csList, VA_ARG(args, struct TagItem *));
  VA_END(args);

  return result;
}

LIBSTUB(CodesetsListRemoveA, BOOL, REG(a0, struct TagItem *attrs))
{
  return CodesetsListRemoveA(attrs);
}

LIBSTUBVA(CodesetsListRemove, BOOL, ...)
{
  BOOL result;
  VA_LIST args;

  VA_START(args, self);
  result = CodesetsListRemoveA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return result;
}

LIBSTUB(CodesetsConvertUTF32toUTF16, ULONG, REG(a0, const UTF32 ** sourceStart),
                                            REG(a1, const UTF32 * sourceEnd),
                                            REG(a2, UTF16 ** targetStart),
                                            REG(a3, UTF16 * targetEnd),
                                            REG(d0, ULONG flags))
{
  return CodesetsConvertUTF32toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags);
}


LIBSTUB(CodesetsConvertUTF16toUTF32, ULONG, REG(a0, const UTF16 ** sourceStart),
                                            REG(a1, const UTF16 * sourceEnd),
                                            REG(a2, UTF32 ** targetStart),
                                            REG(a3, UTF32 * targetEnd),
                                            REG(d0, ULONG flags))
{
  return CodesetsConvertUTF16toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags);
}

LIBSTUB(CodesetsConvertUTF16toUTF8, ULONG, REG(a0, const UTF16 ** sourceStart),
                                           REG(a1, const UTF16 * sourceEnd),
                                           REG(a2, UTF8 ** targetStart),
                                           REG(a3, UTF8 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF16toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags);
}

LIBSTUB(CodesetsIsLegalUTF8, BOOL, REG(a0, const UTF8 * source),
                                   REG(d0, ULONG length))
{
  return CodesetsIsLegalUTF8(source, length);
}

LIBSTUB(CodesetsIsLegalUTF8Sequence, BOOL, REG(a0, const UTF8 * source),
                                            REG(a1, const UTF8 * sourceEnd))
{
  return CodesetsIsLegalUTF8Sequence(source, sourceEnd);
}

LIBSTUB(CodesetsConvertUTF8toUTF16, ULONG, REG(a0, const UTF8 ** sourceStart),
                                           REG(a1, const UTF8 * sourceEnd),
                                           REG(a2, UTF16 ** targetStart),
                                           REG(a3, UTF16 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF8toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags);
}

LIBSTUB(CodesetsConvertUTF32toUTF8, ULONG, REG(a0, const UTF32 ** sourceStart),
                                           REG(a1, const UTF32 * sourceEnd),
                                           REG(a2, UTF8 ** targetStart),
                                           REG(a3, UTF8 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF32toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags);
}

LIBSTUB(CodesetsConvertUTF8toUTF32, ULONG, REG(a0, const UTF8 ** sourceStart),
                                           REG(a1, const UTF8 * sourceEnd),
                                           REG(a2, UTF32 ** targetStart),
                                           REG(a3, UTF32 * targetEnd),
                                           REG(d0, ULONG flags))
{
  return CodesetsConvertUTF8toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags);
}
