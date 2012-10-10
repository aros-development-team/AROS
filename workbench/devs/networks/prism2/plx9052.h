/*

Copyright (C) 2008 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef PLX9052_H
#define PLX9052_H


/* Registers */
/* ========= */

#define PLX9052_INTS 0x4c
#define PLX9052_CNTRL 0x50


/* Register Details */
/* ================ */

/* Control Register */

#define PLX9052_CNTRLB_PCI21   14
#define PLX9052_CNTRLB_RETRIES 19

#define PLX9052_CNTRLF_PCI21   (1 << PLX9052_CNTRLB_PCI21)
#define PLX9052_CNTRLF_RETRIES (0xf << PLX9052_CNTRLB_RETRIES)

#endif
