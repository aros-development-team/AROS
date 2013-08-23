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

#ifndef BETTERSTRING_MCC_H
#define BETTERSTRING_MCC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXEC_TYPES_H
#include <exec/types.h>
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

#define MUIC_BetterString  "BetterString.mcc"

#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define BetterStringObject MUIOBJMACRO_START(MUIC_BetterString)
#else
#define BetterStringObject MUI_NewObject(MUIC_BetterString
#endif

// attributes
#define MUIA_BetterString_SelectSize            0xad001001UL
#define MUIA_BetterString_StayActive            0xad001003UL
#define MUIA_BetterString_Columns               0xad001005UL
#define MUIA_BetterString_NoInput               0xad001007UL
#define MUIA_BetterString_KeyUpFocus            0xad001008UL
#define MUIA_BetterString_KeyDownFocus          0xad001009UL
#define MUIA_BetterString_InactiveContents      0xad00100aUL
#define MUIA_BetterString_NoShortcuts           0xad00100cUL
#define MUIA_BetterString_SelectOnActive        0xad00100dUL
#define MUIA_BetterString_NoNotify              0xad00100eUL

// methods
#define MUIM_BetterString_Insert                0xad001002UL
#define MUIM_BetterString_ClearSelected         0xad001004UL
#define MUIM_BetterString_FileNameStart         0xad001006UL
#define MUIM_BetterString_DoAction              0xad00100bUL

// values for MUIM_BetterString_Insert
#define MUIV_BetterString_Insert_StartOfString  0x00000000UL
#define MUIV_BetterString_Insert_EndOfString    0xfffffffeUL
#define MUIV_BetterString_Insert_BufferPos      0xffffffffUL

// result values of MUIM_BetterString_FileNameStart
#define MUIR_BetterString_FileNameStart_Volume  -1

// values for MUIM_BetterString_DoAction
enum MUIV_BetterString_DoActions
{
  MUIV_BetterString_DoAction_Cut            = 1,
  MUIV_BetterString_DoAction_Copy           = 2,
  MUIV_BetterString_DoAction_Paste          = 3,
  MUIV_BetterString_DoAction_SelectAll      = 4,
  MUIV_BetterString_DoAction_SelectNone     = 5,
  MUIV_BetterString_DoAction_Undo           = 6,
  MUIV_BetterString_DoAction_Redo           = 7,
  MUIV_BetterString_DoAction_Revert         = 8,
  MUIV_BetterString_DoAction_ToggleCase     = 9,
  MUIV_BetterString_DoAction_ToggleCaseWord = 10,
  MUIV_BetterString_DoAction_IncreaseNum    = 11,
  MUIV_BetterString_DoAction_DecreaseNum    = 12,
  MUIV_BetterString_DoAction_HexToDec       = 13,
  MUIV_BetterString_DoAction_DecToHex       = 14,
  MUIV_BetterString_DoAction_NextFileComp   = 15,
  MUIV_BetterString_DoAction_PrevFileComp   = 16,
  MUIV_BetterString_DoAction_Delete         = 17
};

// parameter structures for methods
struct MUIP_BetterString_Insert        { STACKED ULONG MethodID; STACKED STRPTR text; STACKED LONG pos; };
struct MUIP_BetterString_ClearSelected { STACKED ULONG MethodID; };
struct MUIP_BetterString_FileNameStart { STACKED ULONG MethodID; STACKED STRPTR buffer; STACKED LONG pos; };
struct MUIP_BetterString_DoAction      { STACKED ULONG MethodID; STACKED enum MUIV_BetterString_DoActions action; };

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

#endif /* BETTERSTRING_MCC_H */
