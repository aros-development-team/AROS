#ifndef ENDIAN_H
#define ENDIAN_H

#include <sys/types.h>

#if !defined(__AROS__)
#if defined(__X86__)
#define CPU_IS_LITTLE_ENDIAN 1
#else
#undef CPU_IS_LITTLE_ENDIAN
#endif
#else
#if defined(__i386__) || defined(__x86_64__)
#define CPU_IS_LITTLE_ENDIAN 1
#else
#undef CPU_IS_LITTLE_ENDIAN
#endif
#endif

// generic C routines

#ifndef IFF_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

// override with endian aware version
#undef MAKE_ID
#ifndef CPU_IS_LITTLE_ENDIAN
#define MAKE_ID(a,b,c,d)	\
	((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#else
#define MAKE_ID(a,b,c,d)	\
	((ULONG) (d)<<24 | (ULONG) (c)<<16 | (ULONG) (b)<<8 | (ULONG) (a))
#endif

#define read_le16(P) le2nat16(*(UWORD *)(P))
#define read_le32(P) le2nat32(*(ULONG *)(P))
#define read_le64(P) le2nat64(*(UQUAD *)(P))

#define write_le16(P,V) *(UWORD *)(P)=nat2le16(V)
#define write_le32(P,V) *(ULONG *)(P)=nat2le32(V)
#define write_le64(P,V) *(UQUAD *)(P)=nat2le64(V)

#define read_be16(P) be2nat16(*(UWORD *)(P))
#define read_be32(P) be2nat32(*(ULONG *)(P))
#define read_be64(P) be2nat64(*(UQUAD *)(P))

#define write_be16(P,V) *(UWORD *)(P)=nat2be16(V)
#define write_be32(P,V) *(ULONG *)(P)=nat2be32(V)
#define write_be64(P,V) *(UQUAD *)(P)=nat2be64(V)

#define endian16(A) \
( ( ((UWORD)(A)&0xFF00)>>8 )+( ((UWORD)(A)&0xFF)<<8 ) )

#define endian32(A) \
( ( ((ULONG)(A)&0xFF000000)>>24 )+( ((ULONG)(A)&0xFF0000)>>8 )+( ((ULONG)(A)&0xFF00)<<8 ) \
+( ((ULONG)(A)&0xFF)<<24 ) )

#define endian64(A) \
endian32((UQUAD)(A) >> 32)+((UQUAD)endian32(A) << 32)

#if defined(__M68K__) && defined(__VBCC__) // use VBCC/m68k inline asm

#undef endian32
#undef endian16

LONG endian32(__reg("d0") LONG) =
	"\trol.w\t#8,d0\n"
	"\tswap\td0\n"
	"\trol.w\t#8,d0";

WORD endian16(__reg("d0") WORD) =
	"\trol.w\t#8,d0";

#elif defined(__PPC__) && defined(__VBCC__) // use VBCC/PPC inline asm

#undef endian64
#undef endian32
#undef endian16

QUAD endian64(__reg("r5/r6") QUAD) =
	"\trlwinm\t4,5,24,0,31\n"
	"\trlwinm\t3,6,24,0,31\n"
	"\trlwimi\t4,5,8,8,15\n"
	"\trlwimi\t3,6,8,8,15\n"
	"\trlwimi\t4,5,8,24,31\n"
	"\trlwimi\t3,6,8,24,31";

LONG endian32(__reg("r4") LONG) =
	"\trlwinm\t3,4,24,0,31\n"
	"\trlwimi\t3,4,8,8,15\n"
	"\trlwimi\t3,4,8,24,31";

WORD endian16(__reg("r4") WORD) =
	"\trlwinm\t3,4,8,16,24\n"
	"\trlwimi\t3,4,24,24,31";

#undef read_le64
#undef read_le32
#undef read_le16

QUAD read_le64(__reg("r3") void *) =
	"\taddi\t5,3,4\n" // r5 = r3 + 4
	"\tlwbrx\t4,0,3\n"
	"\tlwbrx\t3,0,5";

LONG read_le32(__reg("r3") void *) =
	"\tlwbrx\t3,0,3";

WORD read_le16(__reg("r3") void *) =
	"\tlhbrx\t3,0,3";

#undef write_le64
#undef write_le32
#undef write_le16

void write_le64(__reg("r3") void *, __reg("r5/r6") QUAD) =
	"\taddi\t4,3,4\n" // r4 = r3 + 4
	"\tstwbrx\t6,0,3\n"
	"\tstwbrx\t5,0,4";

void write_le32(__reg("r3") void *, __reg("r4") LONG) =
	"\tstwbrx\t4,0,3";

void write_le16(__reg("r3") void *, __reg("r4") WORD) =
	"\tsthbrx\t4,0,3";

#endif

#ifndef CPU_IS_LITTLE_ENDIAN

#define be2nat16(A) (A)
#define be2nat32(A) (A)
#define be2nat64(A) (A)

#define le2nat16(A) endian16(A)
#define le2nat32(A) endian32(A)
#define le2nat64(A) endian64(A)

#define nat2be16(A) (A)
#define nat2be32(A) (A)
#define nat2be64(A) (A)

#define nat2le16(A) endian16(A)
#define nat2le32(A) endian32(A)
#define nat2le64(A) endian64(A)

#else

#define be2nat16(A) endian16(A)
#define be2nat32(A) endian32(A)
#define be2nat64(A) endian64(A)

#define le2nat16(A) (A)
#define le2nat32(A) (A)
#define le2nat64(A) (A)

#define nat2be16(A) endian16(A)
#define nat2be32(A) endian32(A)
#define nat2be64(A) endian64(A)

#define nat2le16(A) (A)
#define nat2le32(A) (A)
#define nat2le64(A) (A)

#endif

#endif /* ENDIAN_H */
