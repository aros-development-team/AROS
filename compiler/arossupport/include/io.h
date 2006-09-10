#ifndef AROS_IO_H
#define AROS_IO_H

/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: I/O macros
    Lang: english
*/

#ifndef AROS_MACROS_H
#   include <aros/macros.h>
#endif


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
   WORDOUT(address, AROS_WORD2BE(value))

#define LEWORDIN(address) \
   AROS_LE2WORD(WORDIN(address))

#define LEWORDOUT(address, value) \
   WORDOUT(address, AROS_WORD2LE(value))

#define LELONGIN(address) \
   AROS_LE2LONG(LONGIN(address))

#define LELONGOUT(address, value) \
   LONGOUT(address, AROS_LONG2LE(value))

#define LEWORDSIN(address, data, count) \
   ({ \
      ULONG _LEWORDSIN_i = (count); \
      UWORD *_LEWORDSIN_p = (data); \
      while(_LEWORDSIN_i-- != 0) \
         *_LEWORDSIN_p++ = LEWORDIN(address); \
      _LEWORDSIN_p; \
   })

#endif /* AROS_IO_H */
