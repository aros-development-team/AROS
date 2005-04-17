/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor_mcc

 $Id: mcc_common.h,v 1.1 2005/03/28 11:29:48 damato Exp $

***************************************************************************/

#include "SDI_compiler.h"
#include "SDI_hook.h"
#include "SDI_lib.h"
#include "SDI_stdarg.h"

#ifdef __amigaos4__
/* redefine some defines to allow complexer macro use later on */
#define DoMethod				IDoMethod
#define DoMethodA 			IDoMethodA
#define DoSuperMethod 	IDoSuperMethod
#define DoSuperMethodA 	IDoSuperMethodA
#define SetSuperAttrs 	ISetSuperAttrs
#define CoerceMethod 		ICoerceMethod
#define CoerceMethodA 	ICoerceMethodA
#define CallHookA       CallHookPkt

#ifdef OpenWindowTags
#undef OpenWindowTags
#define OpenWindowTags IIntuition->OpenWindowTags
#endif

#ifdef NewObject
#undef NewObject
#define NewObject IIntuition->NewObject
#endif

#define GETINTERFACE(iface, base)	(iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)

#else

#define GETINTERFACE(iface, base)	TRUE
#define DROPINTERFACE(iface)

#endif /* __amigaos4__ */

