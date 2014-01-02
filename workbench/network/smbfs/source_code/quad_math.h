/*
 * $Id$
 *
 * :ts=4
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2009 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _QUAD_MATH_H
#define _QUAD_MATH_H

/****************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

/****************************************************************************/

typedef struct
{
	ULONG High;
	ULONG Low;
} QUAD;

/****************************************************************************/

ULONG divide_64_by_32(QUAD * dividend,ULONG divisor,QUAD * quotient);
ULONG subtract_64_from_64_to_64(const QUAD * const minuend,const QUAD * const subtrahend,QUAD * difference);

/****************************************************************************/

#endif /* _QUAD_MATH_H */
