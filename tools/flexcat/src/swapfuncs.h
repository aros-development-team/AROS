/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef  FLEXCAT_SWAPFUNCS_H
#define  FLEXCAT_SWAPFUNCS_H

/* Functions */
extern uint32   ( *SwapLong ) ( uint32 r );
extern unsigned short  ( *SwapWord ) ( unsigned short r );
unsigned short  SwapWord21 ( unsigned short r );
unsigned short  SwapWord12 ( unsigned short r );
uint32   SwapLong4321 ( uint32 r );
uint32   SwapLong1234 ( uint32 r );
int      SwapChoose ( void );

#endif  /* FLEXCAT_SWAPFUNCS_H */
