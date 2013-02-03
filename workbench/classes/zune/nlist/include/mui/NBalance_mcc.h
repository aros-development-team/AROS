#ifndef NBALANCE_MCC_H
#define NBALANCE_MCC_H

/***************************************************************************

 NBalance.mcc - New Balance MUI Custom Class
 Copyright (C) 2008-2009 by NList Open Source Team

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

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define MUIC_NBalance  "NBalance.mcc"
#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define NBalanceObject MUIOBJMACRO_START(MUIC_NBalance)
#else
#define NBalanceObject MUI_NewObject(MUIC_NBalance
#endif

/* attributes */
#define MUIA_NBalance_Pointer           0xa95f0000UL

/* attribute values */
#define MUIV_NBalance_Pointer_Off       0
#define MUIV_NBalance_Pointer_Standard  1

/* methods */

#endif /* NBALANCE_MCC_H */
