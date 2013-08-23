/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by BetterString.mcc Open Source Team

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

#ifndef MUI_BETTERSTRING_MCP_H
#define MUI_BETTERSTRING_MCP_H

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#include <devices/inputevent.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__AROS__) && defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack(2)
  #elif defined(__VBCC__)
    #pragma amiga-align
  #endif
#endif

/***********************************************************************/

// STACKED ensures proper alignment on AROS 64 bit systems
#if !defined(__AROS__) && !defined(STACKED)
#define STACKED
#endif

/***********************************************************************/

#define MUIC_BetterString_mcp "BetterString.mcp"

#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define BetterStringMcpObject MUIOBJMACRO_START(MUIC_BetterString_mcp)
#else
#define BetterStringMcpObject MUI_NewObject(MUIC_BetterString_mcp
#endif

#define MUICFG_BetterString_ActiveBack      0xad000302UL
#define MUICFG_BetterString_ActiveText      0xad000303UL
#define MUICFG_BetterString_InactiveBack    0xad000300UL
#define MUICFG_BetterString_InactiveText    0xad000301UL
#define MUICFG_BetterString_Cursor          0xad000304UL
#define MUICFG_BetterString_MarkedBack      0xad000305UL
#define MUICFG_BetterString_MarkedText      0xad000308UL
#define MUICFG_BetterString_Font            0xad000306UL // obsolete
#define MUICFG_BetterString_Frame           0xad000307UL // obsolete
#define MUICFG_BetterString_SelectOnActive  0xad00030aUL
#define MUICFG_BetterString_SelectPointer   0xad000309UL

#define CFG_BetterString_ActiveBack_Def     "2:m1"
#define CFG_BetterString_ActiveText_Def     "m5"
#define CFG_BetterString_InactiveBack_Def   "2:m2"
#define CFG_BetterString_InactiveText_Def   "m4"
#define CFG_BetterString_Cursor_Def         "m0"
#if defined(__amigaos4__)
#define CFG_BetterString_MarkedBack_Def     "m7"
#define CFG_BetterString_MarkedText_Def     "m8"
#else
#define CFG_BetterString_MarkedBack_Def     "m5"
#define CFG_BetterString_MarkedText_Def     "m0"
#endif
#define CFG_BetterString_SelectOnActive_Def FALSE
#define CFG_BetterString_SelectPointer_Def  TRUE

#if !defined(__AROS__) && defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack()
  #elif defined(__VBCC__)
    #pragma default-align
  #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* MUI_BETTERSTRING_MCP_H */
