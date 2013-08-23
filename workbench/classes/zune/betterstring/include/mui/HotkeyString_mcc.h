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

#ifndef HOTKEYSTRING_MCC_H
#define HOTKEYSTRING_MCC_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define MUIC_HotkeyString   "HotkeyString.mcc"
#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define HotkeyStringObject  MUIOBJMACRO_START(MUIC_HotkeyString)
#else
#define HotkeyStringObject  MUI_NewObject(MUIC_HotkeyString
#endif

#define MUIA_HotkeyString_Snoop 0xad001000UL
#define MUIA_HotkeyString_IX    0xad001002UL	/* V12 IS. */

#endif /* HOTKEYSTRING_MCC_H */
