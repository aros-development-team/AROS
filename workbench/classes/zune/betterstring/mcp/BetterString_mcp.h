/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: BetterString_mcp.h,v 1.1 2005/04/21 20:52:04 damato Exp $

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

#ifdef __GNUC__
  #ifdef __PPC__
    #pragma pack(2)
  #endif
#elif defined(__VBCC__)
  #pragma amiga-align
#endif

#define MUIC_BetterString_mcp "BetterString.mcp"
#define BetterStringMcpObject MUI_NewObject(MUIC_BetterString_mcp

#define MUICFG_BetterString_ActiveBack			0xad000302
#define MUICFG_BetterString_ActiveText			0xad000303
#define MUICFG_BetterString_InactiveBack		0xad000300
#define MUICFG_BetterString_InactiveText		0xad000301
#define MUICFG_BetterString_Cursor				  0xad000304
#define MUICFG_BetterString_MarkedBack			0xad000305
#define MUICFG_BetterString_MarkedText			0xad000308
#define MUICFG_BetterString_Font					  0xad000306
#define MUICFG_BetterString_Frame				    0xad000307

#ifdef __GNUC__
  #ifdef __PPC__
    #pragma pack()
  #endif
#elif defined(__VBCC__)
  #pragma default-align
#endif

#ifdef __cplusplus
}
#endif

#endif /* MUI_BETTERSTRING_MCP_H */
