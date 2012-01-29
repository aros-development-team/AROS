/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2007 by codesets.library Open Source Team

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

#ifndef _MACROS_H
#define _MACROS_H

/****************************************************************************/
/*
** Some useful macros
*/

#include <SDI/SDI_compiler.h>

#if defined(__amigaos4__)
#define GETINTERFACE(iface, base)	(iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)
#else
#define GETINTERFACE(iface, base)	TRUE
#define DROPINTERFACE(iface)
#endif

// special flagging macros
#define isFlagSet(v,f)      (((v) & (f)) == (f))  // return TRUE if the flag is set
#define hasFlag(v,f)        (((v) & (f)) != 0)    // return TRUE if one of the flags in f is set in v
#define isFlagClear(v,f)    (((v) & (f)) == 0)    // return TRUE if flag f is not set in v
#define SET_FLAG(v,f)       ((v) |= (f))          // set the flag f in v
#define CLEAR_FLAG(v,f)     ((v) &= ~(f))         // clear the flag f in v
#define MASK_FLAG(v,f)      ((v) &= (f))          // mask the variable v with flag f bitwise

// transforms a define into a string
#define STR(x)  STR2(x)
#define STR2(x) #x

/****************************************************************************/

#endif /* _MACROS_H */
