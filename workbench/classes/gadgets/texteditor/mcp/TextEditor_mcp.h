/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: TextEditor_mcp.h,v 1.1 2005/03/28 11:29:49 damato Exp $

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

#ifdef __GNUC__
  #ifdef __PPC__
    #pragma pack(2)
  #endif
#elif defined(__VBCC__)
  #pragma amiga-align
#endif

#define MUIC_TextEditor_mcp "TextEditor.mcp"
#define TextEditorMcpObject MUI_NewObject(MUIC_TextEditor_mcp

#define MUICFG_TextEditor_Background       0xad000051
#define MUICFG_TextEditor_BlinkSpeed       0xad000052
#define MUICFG_TextEditor_BlockQual        0xad000053
#define MUICFG_TextEditor_CheckWord        0xad000050
#define MUICFG_TextEditor_CursorColor      0xad000054
#define MUICFG_TextEditor_CursorTextColor  0xad000055
#define MUICFG_TextEditor_CursorWidth      0xad000056
#define MUICFG_TextEditor_FixedFont        0xad000057
#define MUICFG_TextEditor_Frame            0xad000058
#define MUICFG_TextEditor_HighlightColor   0xad000059
#define MUICFG_TextEditor_MarkedColor      0xad00005a
#define MUICFG_TextEditor_NormalFont       0xad00005b
#define MUICFG_TextEditor_SetMaxPen        0xad00005c
#define MUICFG_TextEditor_Smooth           0xad00005d
#define MUICFG_TextEditor_TabSize          0xad00005e
#define MUICFG_TextEditor_TextColor        0xad00005f
#define MUICFG_TextEditor_UndoSize         0xad000060
#define MUICFG_TextEditor_TypeNSpell       0xad000061
#define MUICFG_TextEditor_LookupCmd        0xad000062
#define MUICFG_TextEditor_SuggestCmd       0xad000063
#define MUICFG_TextEditor_Keybindings      0xad000064
#define MUICFG_TextEditor_SuggestKey       0xad000065 /* OBSOLETE! */
#define MUICFG_TextEditor_SeparatorShine   0xad000066
#define MUICFG_TextEditor_SeparatorShadow  0xad000067
#define MUICFG_TextEditor_ConfigVersion    0xad000068

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

#endif /* MUI_TEXTEDITOR_MCP_H */
