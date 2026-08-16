/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_HEX_H_
#define _LINUX_HEX_H_

#include <linux/types.h>

extern const char hex_asc[];
#define hex_asc_lo(x)   hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)   hex_asc[((x) & 0xf0) >> 4]
extern const char hex_asc_upper[];
#define hex_asc_upper_lo(x) hex_asc_upper[((x) & 0x0f)]
#define hex_asc_upper_hi(x) hex_asc_upper[((x) & 0xf0) >> 4]

static inline int hex_to_bin(unsigned char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}
static inline char *hex_byte_pack(char *buf, u8 byte)
{
    *buf++ = hex_asc_hi(byte);
    *buf++ = hex_asc_lo(byte);
    return buf;
}
int hex2bin(u8 *dst, const char *src, size_t count);
char *bin2hex(char *dst, const void *src, size_t count);

#endif /* _LINUX_HEX_H_ */
