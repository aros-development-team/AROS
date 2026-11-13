/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2018 BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

#ifndef BETTERSTRING_MCP_PRIV_H
#define BETTERSTRING_MCP_PRIV_H

#include "BetterString_mcp.h"
#include "Debug.h"

#include <mcc_common.h>

#define MCPMAXRAWBUF 64

#define IEQUALIFIER_SHIFT   0x0200
#define IEQUALIFIER_ALT     0x0400
#define IEQUALIFIER_COMMAND 0x0800

enum
{
  ActiveBack = 0,
  ActiveText,
  InactiveBack,
  InactiveText,
  Cursor,
  MarkedBack,
  MarkedText,
  SelectOnActive,
  SelectPointer,

  NumberOfObject
};

struct InstData_MCP
{
  BOOL mui4x;
  Object *Objects[NumberOfObject];
};

Object *CreatePrefsGroup(struct InstData_MCP *data);

#define LIBVER(lib) ((struct Library *)lib)->lib_Version
#define LIBREV(lib) ((struct Library *)lib)->lib_Revision
#define VERSION_IS_AT_LEAST(ver, rev, minver, minrev) (((ver) > (minver)) || ((ver) == (minver) && (rev) == (minrev)) || ((ver) == (minver) && (rev) > (minrev)))
#define LIB_VERSION_IS_AT_LEAST(lib, minver, minrev)  VERSION_IS_AT_LEAST(((struct Library *)(lib))->lib_Version, ((struct Library *)(lib))->lib_Revision, minver, minrev)

/// xget()
//  Gets an attribute value from a MUI object
IPTR xget(Object *obj, const IPTR attr);
#if defined(__GNUC__)
  // please note that we do not evaluate the return value of GetAttr()
  // as some attributes (e.g. MUIA_Selected) always return FALSE, even
  // when they are supported by the object. But setting b=0 right before
  // the GetAttr() should catch the case when attr doesn't exist at all
  #define xget(OBJ, ATTR) ({IPTR b=0; GetAttr(ATTR, OBJ, &b); b;})
#endif
///

#ifndef MUIA_PointerType
#define MUIA_PointerType        0x8042b467 /* V20 isg LONG              */
#endif

#ifndef MUIM_WhichPointerType
#define MUIM_WhichPointerType   0x8042e212 /* V20 */
struct  MUIP_WhichPointerType   { ULONG MethodID; LONG mx; LONG my; };
#endif

#ifndef MUIV_PointerType_Normal
#define MUIV_PointerType_Normal 0
#endif

#ifndef MUIV_PointerType_Text
#define MUIV_PointerType_Text   30
#endif

#endif /* BETTERSTRING_MCP_PRIV_H */
