/*

Copyright (C) 2004,2005 Neil Cafferkey

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

#ifndef IO_H
#define IO_H


/* I/O macros */

#ifdef __i386__

#define BYTEIN(address) \
({ \
   UBYTE _BYTEIN_value; \
   __asm volatile ("inb %w1,%0":"=a" (_BYTEIN_value):"Nd" (address)); \
   _BYTEIN_value; \
})

#define WORDIN(address) \
({ \
   UWORD _WORDIN_value; \
   __asm volatile ("inw %w1,%0":"=a" (_WORDIN_value):"Nd" (address)); \
   _WORDIN_value; \
})

#define LONGIN(address) \
({ \
   ULONG _LONGIN_value; \
   __asm volatile ("inl %w1,%0":"=a" (_LONGIN_value):"Nd" (address)); \
   _LONGIN_value; \
})

#define BYTEOUT(address, value) \
({ \
   __asm volatile ("outb %b0,%w1": :"a" (value), "Nd" (address)); \
})

#define WORDOUT(address, value) \
({ \
   __asm volatile ("outw %w0,%w1": :"a" (value), "Nd" (address)); \
})

#define LONGOUT(address, value) \
({ \
   __asm volatile ("outl %0,%w1": :"a" (value), "Nd" (address)); \
})

#else

#define BYTEIN(address) \
   (*((volatile UBYTE *)(address)))

#define WORDIN(address) \
   (*((volatile UWORD *)(address)))

#define LONGIN(address) \
   (*((volatile ULONG *)(address)))

#define BYTEOUT(address, value) \
   *((volatile UBYTE *)(address)) = (value)

#define WORDOUT(address, value) \
   *((volatile UWORD *)(address)) = (value)

#define LONGOUT(address, value) \
   *((volatile ULONG *)(address)) = (value)

#endif

#define WORDSIN(address, data, count) \
   ({ \
      ULONG _WORDSIN_i = (count); \
      UWORD *_WORDSIN_p = (data); \
      while(_WORDSIN_i-- != 0) \
         *_WORDSIN_p++ = WORDIN(address); \
      _WORDSIN_p; \
   })

#define LONGSIN(address, data, count) \
   ({ \
      ULONG _LONGSIN_i = (count); \
      ULONG *_LONGSIN_p = (data); \
      while(_LONGSIN_i-- != 0) \
         *_LONGSIN_p++ = LONGIN(address); \
      _LONGSIN_p; \
   })

#define WORDSOUT(address, data, count) \
   do \
   { \
      ULONG _WORDSOUT_i = (count); \
      const UWORD *_WORDSOUT_p = (data); \
      while(_WORDSOUT_i-- != 0) \
         WORDOUT(address, *_WORDSOUT_p++); \
   } \
   while(0)

#define LONGSOUT(address, data, count) \
   do \
   { \
      ULONG _LONGSOUT_i = (count); \
      const ULONG *_LONGSOUT_p = (data); \
      while(_LONGSOUT_i-- != 0) \
         LONGOUT(address, *_LONGSOUT_p++); \
   } \
   while(0)

#define BEWORDOUT(address, value) \
   WORDOUT(address, MakeBEWord(value))

#define LEWORDIN(address) \
   LEWord(WORDIN(address))

#define LEWORDOUT(address, value) \
   WORDOUT(address, MakeLEWord(value))

#define LELONGIN(address) \
   LELong(LONGIN(address))

#define LELONGOUT(address, value) \
   LONGOUT(address, MakeLELong(value))

#define LEWORDSIN(address, data, count) \
   ({ \
      ULONG _LEWORDSIN_i = (count); \
      UWORD *_LEWORDSIN_p = (data); \
      while(_LEWORDSIN_i-- != 0) \
         *_LEWORDSIN_p++ = LEWORDIN(address); \
      _LEWORDSIN_p; \
   })


#endif
