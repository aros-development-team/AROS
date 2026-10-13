/*

Copyright (C) 2005-2025 Neil Cafferkey

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

#ifndef ENDIAN_H
#define ENDIAN_H


#include <exec/types.h>
#include <aros/macros.h>


/* Endianness macros */

#define FlipWord(A) \
   ({ \
      UWORD _FlipWord_A = (A); \
      _FlipWord_A = (_FlipWord_A << 8) | (_FlipWord_A >> 8); \
   })

#define FlipLong(A) \
   ({ \
      ULONG _FlipLong_A = (A); \
      _FlipLong_A = \
         (FlipWord(_FlipLong_A) << 16) | FlipWord(_FlipLong_A >> 16); \
   })

/* Network values are big-endian; device register/descriptor values are
   little-endian.  Map both to and from host order with the AROS byte-order
   macros so this is correct on every architecture, not just x86.  Each is an
   identity on the matching host and a byte swap otherwise, and since a swap is
   its own inverse the same macro serves both directions. */

#define BEWord(A) \
   AROS_BE2WORD(A)

#define BELong(A) \
   AROS_BE2LONG(A)

#define LEWord(A) \
   AROS_LE2WORD(A)

#define LELong(A) \
   AROS_LE2LONG(A)

#define MakeBEWord(A) \
   BEWord(A)

#define MakeBELong(A) \
   BELong(A)

#define MakeLEWord(A) \
   LEWord(A)

#define MakeLELong(A) \
   LELong(A)


#endif
