/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_UNALIGNED_H_
#define _LINUX_UNALIGNED_H_

#include <linux/types.h>
#include <string.h>
#include <linux/byteorder/generic.h>

#define __get_unaligned_t(type, ptr) ({ const struct { type x; } __attribute__((__packed__)) *__pptr = (typeof(__pptr))(ptr); __pptr->x; })
#define __put_unaligned_t(type, val, ptr) do { struct { type x; } __attribute__((__packed__)) *__pptr = (typeof(__pptr))(ptr); __pptr->x = (val); } while (0)
#define get_unaligned(ptr)      __get_unaligned_t(typeof(*(ptr)), (ptr))
#define put_unaligned(val, ptr) __put_unaligned_t(typeof(*(ptr)), (val), (ptr))

static inline u16 get_unaligned_le16(const void *p) { u16 v = __get_unaligned_t(u16, p); return AROS_LE2WORD(v); }
static inline u32 get_unaligned_le32(const void *p) { u32 v = __get_unaligned_t(u32, p); return AROS_LE2LONG(v); }
static inline u64 get_unaligned_le64(const void *p) { u64 v = __get_unaligned_t(u64, p); return AROS_LE2QUAD(v); }
static inline u16 get_unaligned_be16(const void *p) { u16 v = __get_unaligned_t(u16, p); return AROS_BE2WORD(v); }
static inline u32 get_unaligned_be32(const void *p) { u32 v = __get_unaligned_t(u32, p); return AROS_BE2LONG(v); }
static inline u64 get_unaligned_be64(const void *p) { u64 v = __get_unaligned_t(u64, p); return AROS_BE2QUAD(v); }
static inline void put_unaligned_le16(u16 val, void *p) { __put_unaligned_t(u16, AROS_WORD2LE(val), p); }
static inline void put_unaligned_le32(u32 val, void *p) { __put_unaligned_t(u32, AROS_LONG2LE(val), p); }
static inline void put_unaligned_le64(u64 val, void *p) { __put_unaligned_t(u64, AROS_QUAD2LE(val), p); }
static inline void put_unaligned_be16(u16 val, void *p) { __put_unaligned_t(u16, AROS_WORD2BE(val), p); }
static inline void put_unaligned_be32(u32 val, void *p) { __put_unaligned_t(u32, AROS_LONG2BE(val), p); }
static inline void put_unaligned_be64(u64 val, void *p) { __put_unaligned_t(u64, AROS_QUAD2BE(val), p); }
static inline u32 __get_unaligned_be24(const u8 *p) { return p[0] << 16 | p[1] << 8 | p[2]; }
static inline u32 get_unaligned_be24(const void *p) { return __get_unaligned_be24(p); }
static inline u32 __get_unaligned_le24(const u8 *p) { return p[0] | p[1] << 8 | p[2] << 16; }
static inline u32 get_unaligned_le24(const void *p) { return __get_unaligned_le24(p); }
static inline void __put_unaligned_be24(const u32 val, u8 *p) { *p++ = (val >> 16) & 0xff; *p++ = (val >> 8) & 0xff; *p++ = val & 0xff; }
static inline void put_unaligned_be24(const u32 val, void *p) { __put_unaligned_be24(val, p); }
static inline void __put_unaligned_le24(const u32 val, u8 *p) { *p++ = val & 0xff; *p++ = (val >> 8) & 0xff; *p++ = (val >> 16) & 0xff; }
static inline void put_unaligned_le24(const u32 val, void *p) { __put_unaligned_le24(val, p); }

#endif /* _LINUX_UNALIGNED_H_ */
