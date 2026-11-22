#ifndef DOS_BPTR_H
#define DOS_BPTR_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS version of BPTRs
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif


#ifndef AROS_BPTR_TYPE
#   ifdef AROS_FAST_BPTR
#       define AROS_BPTR_TYPE   APTR
#   else
#       define AROS_BPTR_TYPE   IPTR
#   endif
#endif
#ifndef AROS_BSTR_TYPE
#   ifdef AROS_FAST_BSTR
#       define AROS_BSTR_TYPE   STRPTR
#   else
#       define AROS_BSTR_TYPE   IPTR
#   endif
#endif

typedef AROS_BPTR_TYPE BPTR;
typedef AROS_BSTR_TYPE BSTR;

/* In the BCPL language memory is addressed in chunks of long (e.g. 32 bit) and also
 * the BPTRs on used by DOS on amigaos used this addressing. This is contrary to byte
 * (e.g. 8 bit) addressing as is common now for most architectures.
 * 
 * AROS can be configured to have BPTRs also use byte addressing by setting the
 * AROS_FAST_BPTR preprocessor symbol in the cpu.h file.
 * For source code compatibility AROS__FAST_BPTR should only be used for non-standard
 * AROS implementations.
 * 
 * MKBADDR and BADDR macros are defined to access BPTRs in an implementation
 * independent way.
*/
#ifdef AROS_FAST_BPTR
#   define MKBADDR(a)               ((BPTR)(a))
#   define BADDR(a)                 ((APTR)a)
#   define BNULL                    (NULL)
#else
#   define MKBADDR(a)               ((BPTR)(((IPTR)a)>>2))
#   define BADDR(a)                 ((APTR)(((IPTR)a)<<2))
#   define BNULL                    (0)
#endif

/* A DOS segment (a "seglist" node) is a single allocation carrying the size of
 * that allocation, the link to the next segment of the program, and the
 * segment's payload:
 *
 *                <---- AROS_SEG_PREFIX ---->
 *     +----------+----------+--------------+-------------------+
 *     | seg_Pad  | seg_Size |   seg_Next   |   seg_Data ...    |
 *     +----------+----------+--------------+-------------------+
 *     ^                     ^              ^
 *     allocation base       BADDR(seg)     AROS_SEG_DATA(seg)
 *
 * The size is the ULONG immediately in front of the link, and the link is one
 * BPTR into the allocation. A BPTR-sized prefix therefore keeps both the link
 * and the payload at the allocator's natural alignment on 64-bit targets. On
 * 32-bit targets sizeof(ULONG) == sizeof(BPTR), the pad is absent, and the
 * layout is the classic BCPL [size][next][data].
 *
 * dos.library/UnLoadSeg() releases a segment at its allocation base using the
 * stored size, so anything that builds or walks a seglist by hand must go
 * through these definitions instead of open-coding the offsets.
 *
 * A stored size of zero marks a segment that does not own its memory - the ELF
 * loader uses that for hunks carved out of one arena - and UnLoadSeg() leaves
 * those alone. AROS_SEG_DATALEN() reports no payload for them rather than
 * wrapping around.
 *
 * The accessors evaluate their argument more than once, so pass a plain
 * lvalue, not an expression with side effects.
 */
struct AROSSegment
{
#if __WORDSIZE > 32
    ULONG   seg_Pad;        /* Keeps seg_Size directly in front of seg_Next */
#endif
    ULONG   seg_Size;       /* Size of the whole allocation, in bytes       */
    BPTR    seg_Next;       /* Next segment, BNULL at the end of the list   */
    char    seg_Data[0];    /* The segment's payload                        */
} __attribute__((packed));

#define AROS_SEG_PREFIX             ((IPTR)sizeof(BPTR))
#define AROS_SEG_HEADER             ((IPTR)sizeof(struct AROSSegment))
#define AROS_SEG_MEMSIZE4LEN(len)   ((len) + AROS_SEG_HEADER)

/* Conversions between a segment, its BPTR, the link that BPTR addresses, and
 * the payload pointer an allocator hands out.
 */
#define AROS_SEG_FROMLINK(link) ((struct AROSSegment *)((UBYTE *)(link) - AROS_SEG_PREFIX))
#define AROS_SEG_FROMDATA(data) ((struct AROSSegment *)((UBYTE *)(data) - AROS_SEG_HEADER))
#define AROS_SEG_FROMBPTR(seg)  AROS_SEG_FROMLINK(BADDR(seg))
#define AROS_SEG_TOLINK(s)      (&(s)->seg_Next)
#define AROS_SEG_TOBPTR(s)      MKBADDR(AROS_SEG_TOLINK(s))

/* Accessors taking a segment's BPTR */
#define AROS_SEG_BASE(seg)      ((APTR)AROS_SEG_FROMBPTR(seg))
#define AROS_SEG_SIZE(seg)      (AROS_SEG_FROMBPTR(seg)->seg_Size)
#define AROS_SEG_NEXT(seg)      (AROS_SEG_FROMBPTR(seg)->seg_Next)
#define AROS_SEG_DATA(seg)      ((APTR)AROS_SEG_FROMBPTR(seg)->seg_Data)
#define AROS_SEG_DATALEN(seg)   ((ULONG)(AROS_SEG_SIZE(seg) > (ULONG)AROS_SEG_HEADER ? \
                                 AROS_SEG_SIZE(seg) - (ULONG)AROS_SEG_HEADER : 0))

/* BCPL strings used the first byte as the length of the string followed by the string.
 * Strings then also had a maximum length of 255. The normal C convention is to have
 * strings of any length but ended with a byte value of 0
 * 
 * AROS can be configured to have BSTRs implemented as C strings by setting the
 * AROS_FAST_BSTR preprocessor symbol in the cpu.h file.
 * For source code compatibility AROS_FAST_BSTR should only be used for non-standard
 * AROS implementations.
 * 
 * The AROS_BSTR_ADDR, AROS_BSTR_strlen, AROS_BSTR_setstrlen and AROS_BSTR_MEMSIZE4LEN
 * preprocessor macros are provided to work with BSTRs in an implementation independent
 * way.
 */
#ifdef AROS_FAST_BSTR
#   define AROS_BSTR_ADDR(s)        ((STRPTR)BADDR(s))
#   define AROS_BSTR_strlen(s)      (strlen(AROS_BSTR_ADDR(s)))
#   define AROS_BSTR_setstrlen(s,l) (AROS_BSTR_ADDR(s)[l] = 0)
#   define AROS_BSTR_MEMSIZE4LEN(l) ((l)+1)
#else
#   define AROS_BSTR_ADDR(s)        (((STRPTR)BADDR(s))+1)
#   define AROS_BSTR_strlen(s)      (AROS_BSTR_ADDR(s)[-1])
#   define AROS_BSTR_setstrlen(s,l) do { \
    STRPTR _s = AROS_BSTR_ADDR(s); \
    _s[-1] = l; \
    _s[l]=0; \
} while(0)
#   define AROS_BSTR_MEMSIZE4LEN(l) ((l)+2)
#endif
#define AROS_BSTR_getchar(s,l)   (AROS_BSTR_ADDR(s)[l])
#define AROS_BSTR_putchar(s,l,c) (AROS_BSTR_ADDR(s)[l] = c)

/* Convenience macro for declaring const BSTRs
 */
#ifdef AROS_FAST_BSTR
#define AROS_CONST_BSTR(string) ((BSTR)MKBADDR(string))
#else
#define AROS_CONST_BSTR(string) ({ \
    static const struct { UBYTE len; UBYTE str[sizeof(string)]; } \
    	const __tmp_bstr __attribute__((__aligned__(4)))= { .len = sizeof(string)-1, .str = string }; \
    (BSTR)MKBADDR(&__tmp_bstr);})
#endif

#endif /* DOS_BPTR_H */
