/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BYTEORDER_GENERIC_H_
#define _LINUX_BYTEORDER_GENERIC_H_

#include <linux/types.h>
#include <linux/swab.h>

#define cpu_to_le16(x)          ((__le16)AROS_WORD2LE(x))
#define cpu_to_le32(x)          ((__le32)AROS_LONG2LE(x))
#define cpu_to_le64(x)          ((__le64)AROS_QUAD2LE(x))
#define le16_to_cpu(x)          ((u16)AROS_LE2WORD(x))
#define le32_to_cpu(x)          ((u32)AROS_LE2LONG(x))
#define le64_to_cpu(x)          ((u64)AROS_LE2QUAD(x))
#define cpu_to_be16(x)          ((__be16)AROS_WORD2BE(x))
#define cpu_to_be32(x)          ((__be32)AROS_LONG2BE(x))
#define cpu_to_be64(x)          ((__be64)AROS_QUAD2BE(x))
#define be16_to_cpu(x)          ((u16)AROS_BE2WORD(x))
#define be32_to_cpu(x)          ((u32)AROS_BE2LONG(x))
#define be64_to_cpu(x)          ((u64)AROS_BE2QUAD(x))
#define __cpu_to_le16(x)        cpu_to_le16(x)
#define __cpu_to_le32(x)        cpu_to_le32(x)
#define __cpu_to_le64(x)        cpu_to_le64(x)
#define __le16_to_cpu(x)        le16_to_cpu(x)
#define __le32_to_cpu(x)        le32_to_cpu(x)
#define __le64_to_cpu(x)        le64_to_cpu(x)
#define __cpu_to_be16(x)        cpu_to_be16(x)
#define __cpu_to_be32(x)        cpu_to_be32(x)
#define __cpu_to_be64(x)        cpu_to_be64(x)
#define __be16_to_cpu(x)        be16_to_cpu(x)
#define __be32_to_cpu(x)        be32_to_cpu(x)
#define __be64_to_cpu(x)        be64_to_cpu(x)
#define le16_to_cpus(x)         do { *(x) = le16_to_cpu(*(x)); } while (0)
#define le32_to_cpus(x)         do { *(x) = le32_to_cpu(*(x)); } while (0)
#define le64_to_cpus(x)         do { *(x) = le64_to_cpu(*(x)); } while (0)
#define cpu_to_le16s(x)         le16_to_cpus(x)
#define cpu_to_le32s(x)         le32_to_cpus(x)
#define be32_to_cpus(x)         do { *(x) = be32_to_cpu(*(x)); } while (0)
static inline u16 le16_to_cpup(const __le16 *p) { return le16_to_cpu(*p); }
static inline u32 le32_to_cpup(const __le32 *p) { return le32_to_cpu(*p); }
static inline u64 le64_to_cpup(const __le64 *p) { return le64_to_cpu(*p); }
static inline u16 be16_to_cpup(const __be16 *p) { return be16_to_cpu(*p); }
static inline u32 be32_to_cpup(const __be32 *p) { return be32_to_cpu(*p); }
static inline u64 be64_to_cpup(const __be64 *p) { return be64_to_cpu(*p); }
#define le16_add_cpu(v, a)      (*(v) = cpu_to_le16(le16_to_cpu(*(v)) + (a)))
#define le32_add_cpu(v, a)      (*(v) = cpu_to_le32(le32_to_cpu(*(v)) + (a)))
#if AROS_BIG_ENDIAN
#define __BIG_ENDIAN            4321
#define __BYTE_ORDER            __BIG_ENDIAN
#else
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN         1234
#endif
#endif

#endif /* _LINUX_BYTEORDER_GENERIC_H_ */
