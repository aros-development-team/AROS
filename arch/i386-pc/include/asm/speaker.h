/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SPEAKER_H
#define _SPEAKER_H

#define inb(port) \
    ({	char __value;	\
	__asm__ __volatile__ ("inb $" #port ",%%al":"=a"(__value));	\
	__value;	})

#define outb(val,port) \
    ({	char __value=(val);	\
	__asm__ __volatile__ ("outb %%al,$" #port::"a"(__value)); })


#define SetSpkFreq(freq)                                  \
    do                                                    \
    {                                                     \
	WORD counter = 0x1234DD / freq;                   \
 	outb (0xB6, 0x43);                                \
	outb ((UBYTE) (counter & 0x00FF), 0x42);          \
	outb ((UBYTE) ((counter & 0xFF00) >> 8), 0x42);   \
    }                                                     \
    while (0)

#define SpkOn() outb (inb(0x61) | 3, 0x61)

#define SpkOff() outb (inb(0x61) & ~3, 0x61)

#define Sound(freq,loop)				\
	SetSpkFreq(freq);				\
	SpkOn();					\
	{						\
		int i, dummy;				\
		for (i = 0; i < loop; dummy = i*i, i++);\
	}						\
	SpkOff();

#endif /* _SPEAKER_H */
