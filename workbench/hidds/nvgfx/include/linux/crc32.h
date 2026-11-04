/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CRC32_H_
#define _LINUX_CRC32_H_

#include <linux/types.h>
#include <linux/bitrev.h>
u32 crc32_le(u32 crc, const void *p, size_t len);
u32 crc32_be(u32 crc, const void *p, size_t len);
#define crc32(seed, data, length)   crc32_le(seed, (const void *)(data), length)
#define ether_crc(l, d)             crc32_be(~0, d, l)
#define __crc32c_le(c, p, l)        crc32_le(c, p, l)

#endif /* _LINUX_CRC32_H_ */
