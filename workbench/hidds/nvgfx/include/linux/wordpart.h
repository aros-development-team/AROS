/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_WORDPART_H_
#define _LINUX_WORDPART_H_

#define upper_32_bits(n)        ((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n)        ((u32)((n) & 0xffffffff))
#define upper_16_bits(n)        ((u16)((n) >> 16))
#define lower_16_bits(n)        ((u16)((n) & 0xffff))
#define REPEAT_BYTE(x)          ((~0ul / 0xff) * (x))

#endif /* _LINUX_WORDPART_H_ */
