/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _ASM_SEGMENTS_H
#define _ASM_SEGMENTS_H

#undef __STR
#undef STR
#define __STR(x) #x
#define STR(x) __STR(x)

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

#define USER_CS   0x1b
#define USER_DS   0x23

#define IN_USER_MODE \
	({  short __value; \
	__asm__ __volatile__ ("mov %%cs,%%ax":"=a"(__value)); \
	(__value & 0x03);	})

#endif /* _ASM_SEGMENTS_H */
