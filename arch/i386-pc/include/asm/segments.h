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

#endif /* _ASM_SEGMENTS_H */
