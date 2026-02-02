/*
 * Copyright (c) 2002-2008 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2008 Atheros Communications, Inc.
 * Copyright (c) 2010-2026 Neil Cafferkey
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id$
 */
#ifndef _ATH_AH_OSDEP_H_
#define _ATH_AH_OSDEP_H_
/*
 * Atheros Hardware Access Layer (HAL) OS Dependent Definitions.
 */

#include <exec/types.h>

#define	__ahdecl
#ifndef __packed
#define	__packed	__attribute__((__packed__))
#endif

typedef BYTE int8_t;
typedef WORD int16_t;
typedef LONG int32_t;
typedef long long int64_t;

typedef UBYTE uint8_t;
typedef UWORD uint16_t;
typedef ULONG uint32_t;
typedef unsigned long long uint64_t;

#ifndef __size_t
typedef unsigned int size_t;
#endif
typedef unsigned int u_int;
#ifndef _VA_LIST_
typedef	void *va_list;
#endif

/*
 * Bus i/o type definitions.
 */
typedef void* HAL_SOFTC;                /* pointer to driver/OS state */
typedef void* HAL_BUS_TAG;              /* opaque bus i/o id tag */
typedef void* HAL_BUS_HANDLE;           /* opaque bus i/o handle */

/*
 * Linux/BSD gcc compatibility shims.
 */
#define	__printflike(_a,_b) \
	__attribute__ ((__format__ (__printf__, _a, _b)))
#define	__va_list	va_list 
#define	OS_INLINE	__inline

/*
 * Delay n microseconds.
 */
extern	void __ahdecl ath_hal_delay(int);
#define	OS_DELAY(_n)	ath_hal_delay(_n)

#define	OS_MEMZERO(_a, _n)	ath_hal_memzero((_a), (_n))
extern void __ahdecl ath_hal_memzero(void *, size_t);
#define	OS_MEMCPY(_d, _s, _n)	ath_hal_memcpy(_d,_s,_n)
extern void * __ahdecl ath_hal_memcpy(void *, const void *, size_t);

#ifndef abs
#define	abs(_a)		__builtin_abs(_a)
#endif

struct ath_hal;
extern	uint32_t __ahdecl ath_hal_getuptime(struct ath_hal *);
#define	OS_GETUPTIME(_ah)	ath_hal_getuptime(_ah)

/*
 * Byte order/swapping support.
 */
#define	AH_LITTLE_ENDIAN	1234
#define	AH_BIG_ENDIAN		4321

#ifndef AH_BYTE_ORDER
#ifdef __AROS__
#include <aros/cpu.h>
#if AROS_BIG_ENDIAN
#define	AH_BYTE_ORDER	AH_BIG_ENDIAN
#else
#define	AH_BYTE_ORDER	AH_LITTLE_ENDIAN
#endif
#else
#define	AH_BYTE_ORDER	AH_BIG_ENDIAN
#endif
#endif /* AH_BYTE_ORDER */

#ifdef __MORPHOS__
#include <hardware/byteswap.h>
#define __bswap16	SWAPWORD
#define __bswap32	SWAPLONG
#else
#if AH_BYTE_ORDER == AH_BIG_ENDIAN
/*
 * This could be optimized but since we only use it for
 * a few registers there's little reason to do so.
 */
static __inline__ uint16_t
__bswap16(uint16_t _x)
{
 	return ((uint16_t)(
	      (((const uint8_t *)(&_x))[0]   ) |
	      (((const uint8_t *)(&_x))[1]<<8))
	);
}
static __inline__ uint32_t
__bswap32(uint32_t _x)
{
 	return ((uint32_t)(
	      (((const uint8_t *)(&_x))[0]    ) |
	      (((const uint8_t *)(&_x))[1]<< 8) |
	      (((const uint8_t *)(&_x))[2]<<16) |
	      (((const uint8_t *)(&_x))[3]<<24))
	);
}
#else
/*
 * This could be optimized but since we only use it for
 * a few registers there's little reason to do so.
 */
static __inline__ uint16_t
__bswap16(uint16_t _x)
{
 	return ((uint16_t)(
	      (((const uint8_t *)(&_x))[1]   ) |
	      (((const uint8_t *)(&_x))[0]<<8))
	);
}
static __inline__ uint32_t
__bswap32(uint32_t _x)
{
 	return ((uint32_t)(
	      (((const uint8_t *)(&_x))[3]    ) |
	      (((const uint8_t *)(&_x))[2]<< 8) |
	      (((const uint8_t *)(&_x))[1]<<16) |
	      (((const uint8_t *)(&_x))[0]<<24))
	);
}
#endif
#endif

#ifdef __MORPHOS__
#define SYNCIO	__asm("eieio");\
		__asm("sync");
#else
#define SYNCIO
#endif

/*
 * Register read/write; we assume the registers will always
 * be memory-mapped.  Note that register accesses are done
 * using target-specific functions when debugging is enabled
 * (AH_DEBUG) or we are explicitly configured this way.  The
 * latter is used on some platforms where the full i/o space
 * cannot be directly mapped.
 *
 * The hardware registers are native little-endian byte order.
 * Big-endian hosts are handled by enabling hardware byte-swap
 * of register reads and writes at reset.  But the PCI clock
 * domain registers are not byte swapped!  Thus, on big-endian
 * platforms we have to byte-swap those registers specifically.
 * Most of this code is collapsed at compile time because the
 * register values are constants.
 */
#if AH_BYTE_ORDER == AH_BIG_ENDIAN
#define	OS_REG_UNSWAPPED(_reg) \
	(((_reg) >= 0x4000 && (_reg) < 0x5000) || \
	 ((_reg) >= 0x7000 && (_reg) < 0x8000))
#define _OS_REG_WRITE(_ah, _reg, _val) do {				    \
	if (OS_REG_UNSWAPPED(_reg))					    \
		*((volatile uint32_t *)((_ah)->ah_sh + (_reg))) =	    \
			__bswap32((_val));				    \
	else								    \
		*((volatile uint32_t *)((_ah)->ah_sh + (_reg))) = (_val);  \
	SYNCIO; \
} while (0)
#define _OS_REG_READ(_ah, _reg) \
	(OS_REG_UNSWAPPED(_reg) ? \
		__bswap32(*((volatile uint32_t *)((_ah)->ah_sh + (_reg)))) : \
		*((volatile uint32_t *)((_ah)->ah_sh + (_reg))))
#else /* AH_LITTLE_ENDIAN */
#define	OS_REG_UNSWAPPED(_reg)	(0)
#define _OS_REG_WRITE(_ah, _reg, _val) do { \
	*((volatile uint32_t *)((_ah)->ah_sh + (_reg))) = (_val); \
} while (0)
#define _OS_REG_READ(_ah, _reg) \
	*((volatile uint32_t *)((_ah)->ah_sh + (_reg)))
#endif /* AH_BYTE_ORDER */

#if 0 && defined(AH_DEBUG) || defined(AH_REGOPS_FUNC) || defined(AH_DEBUG_ALQ)
/* use functions to do register operations */
#define	OS_REG_WRITE(_ah, _reg, _val)	ath_hal_reg_write(_ah, _reg, _val)
#define	OS_REG_READ(_ah, _reg)		ath_hal_reg_read(_ah, _reg)

extern	void __ahdecl ath_hal_reg_write(struct ath_hal *ah,
		u_int reg, uint32_t val);
extern	uint32_t __ahdecl ath_hal_reg_read(struct ath_hal *ah, u_int reg);
#else
/* inline register operations */
#define OS_REG_WRITE(_ah, _reg, _val)	_OS_REG_WRITE(_ah, _reg, _val)
#define OS_REG_READ(_ah, _reg)		_OS_REG_READ(_ah, _reg)
#endif /* AH_DEBUG || AH_REGFUNC || AH_DEBUG_ALQ */

#ifdef AH_DEBUG_ALQ
extern	void __ahdecl OS_MARK(struct ath_hal *, u_int id, uint32_t value);
#else
#define	OS_MARK(_ah, _id, _v)
#endif

#define OS_SET_DECLARE(set, ptype)
#define OS_DATA_SET(set, sym)
#define OS_SET_FOREACH(pvar, set)       for (pvar = (void *)set; *pvar != NULL; pvar++)

extern void *ah_chips[];
extern void *ah_rfs[];

#endif /* _ATH_AH_OSDEP_H_ */
