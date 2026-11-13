/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef ENDIAN_H
#define ENDIAN_H

#include <stdint.h>

#ifndef WORDS_BIGENDIAN
#ifdef __AROS__
#include_next <endian.h>
#if _BYTE_ORDER == _BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#elif _BYTE_ORDER == _LITTLE_ENDIAN
#define WORDS_BIGENDIAN 0
#else
#error "Unsupported endian format."
#endif
#else
#define WORDS_BIGENDIAN 1
#endif
#endif

#define swap16(x) ((((uint16_t)(x) & 0x00ff)<<8)| \
	(((uint16_t)(x) & 0xff00)>>8))
#define swap32(x) ((((uint32_t)(x) & 0x000000ff)<<24)| \
	(((uint32_t)(x) & 0x0000ff00)<<8)| \
	(((uint32_t)(x) & 0x00ff0000)>>8)| \
	(((uint32_t)(x) & 0xff000000)>>24))
#define swap64(x) ((uint64_t)swap32((uint64_t)(x) >> 32)|((uint64_t)swap32(x) << 32))
#define rswap16(p) swap16(*(uint16_t *)(p))
#define rswap32(p) swap32(*(uint32_t *)(p))
#define rswap64(p) swap64(*(uint64_t *)(p))
#define wswap16(p,x) (*(uint16_t *)(p) = swap16(x))
#define wswap32(p,x) (*(uint32_t *)(p) = swap32(x))
#define wswap64(p,x) (*(uint64_t *)(p) = swap64(x))

#if WORDS_BIGENDIAN
#define h2be16(x)  (x)
#define h2be32(x)  (x)
#define h2be64(x)  (x)
#define h2le16(x)  swap16(x)
#define h2le32(x)  swap32(x)
#define h2le64(x)  swap64(x)
#define rbe16(p)   (*(uint16_t *)(p))
#define rbe32(p)   (*(uint32_t *)(p))
#define rbe64(p)   (*(uint64_t *)(p))
#define rle16(p)   rswap16(p)
#define rle32(p)   rswap32(p)
#define rle64(p)   rswap64(p)
#define wbe16(p,x) (*(uint16_t *)(p) = (x))
#define wbe32(p,x) (*(uint32_t *)(p) = (x))
#define wbe64(p,x) (*(uint64_t *)(p) = (x))
#define wle16(p,x) wswap16(p,x)
#define wle32(p,x) wswap32(p,x)
#define wle64(p,x) wswap64(p,x)
#else
#define h2le16(x)  (x)
#define h2le32(x)  (x)
#define h2le64(x)  (x)
#define h2be16(x)  swap16(x)
#define h2be32(x)  swap32(x)
#define h2be64(x)  swap64(x)
#define rle16(p)   (*(uint16_t *)(p))
#define rle32(p)   (*(uint32_t *)(p))
#define rle64(p)   (*(uint64_t *)(p))
#define rbe16(p)   rswap16(p)
#define rbe32(p)   rswap32(p)
#define rbe64(p)   rswap64(p)
#define wle16(p,x) (*(uint16_t *)(p) = (x))
#define wle32(p,x) (*(uint32_t *)(p) = (x))
#define wle64(p,x) (*(uint64_t *)(p) = (x))
#define wbe16(p,x) wswap16(p,x)
#define wbe32(p,x) wswap32(p,x)
#define wbe64(p,x) wswap64(p,x)
#endif

#if defined(__VBCC__) && defined(__PPC__)

#undef swap16
#undef swap32
#undef swap64

uint16_t swap16(__reg("r4") uint16_t) =
	"\trlwinm\t3,4,8,16,24\n"
	"\trlwimi\t3,4,24,24,31";

uint32_t swap32(__reg("r4") uint32_t) =
	"\trlwinm\t3,4,24,0,31\n"
	"\trlwimi\t3,4,8,8,15\n"
	"\trlwimi\t3,4,8,24,31";

uint64_t swap64(__reg("r5/r6") uint64_t) =
	"\trlwinm\t4,5,24,0,31\n"
	"\trlwinm\t3,6,24,0,31\n"
	"\trlwimi\t4,5,8,8,15\n"
	"\trlwimi\t3,6,8,8,15\n"
	"\trlwimi\t4,5,8,24,31\n"
	"\trlwimi\t3,6,8,24,31";

#undef rswap16
#undef rswap32
#undef rswap64

uint16_t rswap16(__reg("r3") void *) =
	"\tlhbrx\t3,0,3";

uint32_t rswap32(__reg("r3") void *) =
	"\tlwbrx\t3,0,3";

uint64_t rswap64(__reg("r3") void *) =
	"\taddi\t5,3,4\n" // r5 = r3 + 4
	"\tlwbrx\t4,0,3\n"
	"\tlwbrx\t3,0,5";

#undef wswap16
#undef wswap32
#undef wswap64

void wswap16(__reg("r3") void *, __reg("r4") uint16_t) =
	"\tsthbrx\t4,0,3";

void wswap32(__reg("r3") void *, __reg("r4") uint32_t) =
	"\tstwbrx\t4,0,3";

void wswap64(__reg("r3") void *, __reg("r5/r6") uint64_t) =
	"\taddi\t4,3,4\n" // r4 = r3 + 4
	"\tstwbrx\t6,0,3\n"
	"\tstwbrx\t5,0,4";

#endif /* defined(__VBCC__) && defined(__PPC__) */

#if defined(__GNUC__) && defined(__PPC__)

#undef swap16
#undef swap32
#undef swap64

static inline uint32_t swap16(uint16_t x) {
	uint32_t res;
	asm("rlwinm %0,%1,8,16,23\n"
		"\trlwimi %0,%1,24,24,31"
		: "=&r" (res)
		: "r" (x));
	return res;
}

static inline uint32_t swap32(uint32_t x) {
	uint32_t res;
	asm("rlwinm %0,%1,24,0,31\n"
		"\trlwimi %0,%1,8,8,15\n"
		"\trlwimi %0,%1,8,24,31"
		: "=&r" (res)
		: "r" (x));
	return res;
}

static inline uint64_t swap64(uint64_t x) {
	uint64_t res;
	asm("rlwinm %L0,%H1,24,0,31\n"
		"\trlwinm %H0,%L1,24,0,31\n"
		"\trlwimi %L0,%H1,8,8,15\n"
		"\trlwimi %H0,%L1,8,8,15\n"
		"\trlwimi %L0,%H1,8,24,31\n"
		"\trlwimi %H0,%L1,8,24,31"
		: "=&r" (res)
		: "r" (x));
	return res;
}

#undef rswap16
#undef rswap32
#undef rswap64

static inline uint32_t rswap16(const void *p) {
	uint32_t res;
	const uint16_t *p0 = p;
	asm("lhbrx %0,%y1"
		: "=r" (res)
		: "Z" (*p0));
	return res;
}

static inline uint32_t rswap32(const void *p) {
	uint32_t res;
	const uint32_t *p0 = p;
	asm("lwbrx %0,%y1"
		: "=r" (res)
		: "Z" (*p0));
	return res;
}

static inline uint64_t rswap64(const void *p) {
	uint64_t res;
	const uint32_t *p0 = p;
	const uint32_t *p1 = p0+1;
	asm("lwbrx %L0,%y1\n"
		"\tlwbrx %H0,%y2"
		: "=&r" (res)
		: "Z" (*p0), "Z" (*p1));
	return res;
}

#undef wswap16
#undef wswap32
#undef wswap64

static inline void wswap16(void *p, uint16_t x) {
	uint16_t *p0 = p;
	asm("sthbrx %1,%y0"
		:
		: "Z" (*p0), "r" (x));
}

static inline void wswap32(void *p, uint32_t x) {
	uint32_t *p0 = p;
	asm("stwbrx %1,%y0"
		:
		: "Z" (*p0), "r" (x));
}

static inline void wswap64(void *p, uint64_t x) {
	uint32_t *p0 = p;
	uint32_t *p1 = p0+1;
	asm("stwbrx %L2,%y0\n"
		"\tstwbrx %H2,%y1"
		:
		: "Z" (*p0), "Z" (*p1), "r" (x));
}
#endif /* defined(__GNUC__) && defined(__PPC__) */

#if defined(__GNUC__) && (defined(__i386__) || defined(__i486__))

#ifndef USE_BSWAP
#define USE_BSWAP 1
#endif

#undef swap16
#undef swap32

static inline uint16_t swap16(uint16_t x) {
	uint16_t res;
	asm("rorw $8,%w0"
		: "=r" (res)
		: "0" (x)
		: "cc");
	return res;
}

static inline uint32_t swap32(uint32_t x) {
	uint32_t res;
#if USE_BSWAP
	asm("bswap %0"
		: "=r" (res)
		: "0" (x));
#else
	asm("rorw $8,%w0\n"
		"\trorl $16,%0\n"
		"\trorw $8,%w0"
		: "=r" (res)
		: "0" (x)
		: "cc");
#endif
	return res;
}

#endif

#if defined(__VBCC__) && defined(__M68K__)

#undef swap16
#undef swap32
#undef swap64

static inline uint16_t swap16(__reg("d0") uint16_t) =
	"\trol.w\t#8,d0";

static inline uint32_t swap32(__reg("d0") uint32_t) =
	"\trol.w\t#8,d0\n"
	"\tswap\td0\n"
	"\trol.w\t#8,d0";

static inline uint64_t swap64(__reg("d0/d1") uint64_t) =
	"\trol.w\t#8,d0\n"
	"\trol.w\t#8,d1\n"
	"\tswap\td0\n"
	"\tswap\td1\n"
	"\trol.w\t#8,d0\n"
	"\trol.w\t#8,d1\n"
	"\teor.l\td0,d1\n"
	"\teor.l\td1,d0\n"
	"\teor.l\td0,d1";

#undef rswap64

static inline uint64_t rswap64(__reg("a0") void *) =
	"\tmove.l\t4(a0),d0\n"
	"\tmove.l\t(a0),d1\n"
	"\trol.w\t#8,d0\n"
	"\trol.w\t#8,d1\n"
	"\tswap\td0\n"
	"\tswap\td1\n"
	"\trol.w\t#8,d0\n"
	"\trol.w\t#8,d1";

#undef wswap64

static inline uint64_t wswap64(__reg("a0") void *, __reg("d0/d1") uint64_t) =
	"\trol.w\t#8,d0\n"
	"\trol.w\t#8,d1\n"
	"\tswap\td0\n"
	"\tswap\td1\n"
	"\trol.w\t#8,d0\n"
	"\trol.w\t#8,d1\n"
	"\tmove.l\td0,4(a0)\n"
	"\tmove.l\td1,(a0)";

#endif /* defined(__VBCC__) && defined(__M68K__) */

#endif /* ENDIAN_H */
