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

 $Id: BetterString_mcc.h,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#ifndef BETTERSTRING_MCC_H
#define BETTERSTRING_MCC_H

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

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define MUIC_BetterString     "BetterString.mcc"
#define BetterStringObject    MUI_NewObject(MUIC_BetterString

#define MUIA_BetterString_Columns               0xad001005
#define MUIA_BetterString_NoInput               0xad001007
#define MUIA_BetterString_SelectSize            0xad001001
#define MUIA_BetterString_StayActive            0xad001003
#define MUIA_BetterString_KeyUpFocus            0xad001008
#define MUIA_BetterString_KeyDownFocus          0xad001009

#define MUIM_BetterString_ClearSelected         0xad001004
#define MUIM_BetterString_FileNameStart         0xad001006
#define MUIM_BetterString_Insert                0xad001002

#define MUIV_BetterString_Insert_StartOfString  0x00000000
#define MUIV_BetterString_Insert_EndOfString    0xfffffffe
#define MUIV_BetterString_Insert_BufferPos      0xffffffff

#define MUIV_BetterString_BufferPos_End         0xffffffff

#define MUIR_BetterString_FileNameStart_Volume  -1

struct MUIP_BetterString_ClearSelected {ULONG MethodID; };
struct MUIP_BetterString_FileNameStart {ULONG MethodID; STRPTR buffer; LONG pos; };
struct MUIP_BetterString_Insert        {ULONG MethodID; STRPTR text; LONG pos; };

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

#endif /* BETTERSTRING_MCC_H */
