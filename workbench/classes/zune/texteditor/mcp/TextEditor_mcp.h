/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#ifndef MUI_TEXTEDITOR_MCP_H
#define MUI_TEXTEDITOR_MCP_H

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

#define MUIC_TextEditor_mcp "TextEditor.mcp"
#define TextEditorMcpObject MUI_NewObject(MUIC_TextEditor_mcp

#define MUICFG_TextEditor_Background       0xad000051UL
#define MUICFG_TextEditor_BlinkSpeed       0xad000052UL
#define MUICFG_TextEditor_BlockQual        0xad000053UL
#define MUICFG_TextEditor_CheckWord        0xad000050UL
#define MUICFG_TextEditor_CursorColor      0xad000054UL
#define MUICFG_TextEditor_CursorTextColor  0xad000055UL
#define MUICFG_TextEditor_CursorWidth      0xad000056UL
#define MUICFG_TextEditor_FixedFont        0xad000057UL
#define MUICFG_TextEditor_Frame            0xad000058UL
#define MUICFG_TextEditor_HighlightColor   0xad000059UL
#define MUICFG_TextEditor_MarkedColor      0xad00005aUL
#define MUICFG_TextEditor_NormalFont       0xad00005bUL
#define MUICFG_TextEditor_SetMaxPen        0xad00005cUL
#define MUICFG_TextEditor_Smooth           0xad00005dUL
#define MUICFG_TextEditor_TabSize          0xad00005eUL
#define MUICFG_TextEditor_TextColor        0xad00005fUL
#define MUICFG_TextEditor_UndoSize         0xad000060UL
#define MUICFG_TextEditor_TypeNSpell       0xad000061UL
#define MUICFG_TextEditor_LookupCmd        0xad000062UL
#define MUICFG_TextEditor_SuggestCmd       0xad000063UL
#define MUICFG_TextEditor_Keybindings      0xad000064UL
#define MUICFG_TextEditor_SuggestKey       0xad000065UL /* OBSOLETE! */
#define MUICFG_TextEditor_SeparatorShine   0xad000066UL
#define MUICFG_TextEditor_SeparatorShadow  0xad000067UL
#define MUICFG_TextEditor_ConfigVersion    0xad000068UL
#define MUICFG_TextEditor_InactiveCursor   0xad000069UL
#define MUICFG_TextEditor_SelectPointer    0xad00006aUL
#define MUICFG_TextEditor_InactiveColor    0xad00006bUL

#if !defined(__AROS__) && defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack(2)
  #elif defined(__VBCC__)
    #pragma amiga-align
  #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* MUI_TEXTEDITOR_MCP_H */
