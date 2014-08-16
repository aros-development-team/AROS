/***************************************************************************

 NFloattext.mcc - New Floattext MUI Custom Class
 Registered MUI class, Serial Number: 1d51 (0x9d5100a1 to 0x9d5100aF)

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#ifndef MUI_NFloattext_MCC_H
#define MUI_NFloattext_MCC_H

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#ifndef MUI_NListview_MCC_H
#include <mui/NListview_mcc.h>
#endif

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

#define MUIC_NFloattext "NFloattext.mcc"
#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define NFloattextObject MUIOBJMACRO_START(MUIC_NFloattext)
#else
#define NFloattextObject MUI_NewObject(MUIC_NFloattext
#endif

/* Attributes */

#define MUIA_NFloattext_Text                0x9d5100a1UL /* GM  isg STRPTR             */
#define MUIA_NFloattext_SkipChars           0x9d5100a2UL /* GM  isg char *             */
#define MUIA_NFloattext_TabSize             0x9d5100a3UL /* GM  isg ULONG              */
#define MUIA_NFloattext_Justify             0x9d5100a4UL /* GM  isg BOOL               */
#define MUIA_NFloattext_Align               0x9d5100a5UL /* GM  isg LONG               */

#define MUIM_NFloattext_GetEntry            0x9d5100aFUL /* GM */
struct  MUIP_NFloattext_GetEntry            { STACKED ULONG MethodID; STACKED LONG pos; STACKED APTR *entry; };

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

#endif /* MUI_NFloattext_MCC_H */
