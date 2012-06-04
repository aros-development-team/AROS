#ifndef PCI_AROS_H
#define PCI_AROS_H

/*
 *----------------------------------------------------------------------------
 *             Includes for AROS PCI handling
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>

 *
 */

// hmmm, these were PPC specific barriers

#undef SYNC
#ifdef __powerpc__
#define SYNC asm volatile("eieio");
#else
#define SYNC
#endif

#include <aros/io.h>

#define READMEM16_LE(rb) AROS_WORD2LE(*((volatile UWORD *) (rb)))
#define READMEM32_LE(rb) AROS_LONG2LE(*((volatile ULONG *) (rb)))
#define	WRITEMEM32_LE(adr, value)	   *((volatile ULONG *) (adr)) = AROS_LONG2LE(value)
#define CONSTWRITEMEM32_LE(adr, value) *((volatile ULONG *) (adr)) = AROS_LONG2LE(value)

#define CONSTWRITEREG16_LE(rb, offset, value) *((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_WORD2LE(value)
#define CONSTWRITEREG32_LE(rb, offset, value) *((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_LONG2LE(value)
#define WRITEREG16_LE(rb, offset, value)      *((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_WORD2LE(value)
#define WRITEREG32_LE(rb, offset, value)      *((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_LONG2LE(value)
#define WRITEREG64_LE(rb, offset, value)      *((volatile UQUAD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_QUAD2LE(value)

#define READREG16_LE(rb, offset) AROS_WORD2LE(*((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))
#define READREG32_LE(rb, offset) AROS_LONG2LE(*((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))
#define READREG64_LE(rb, offset) AROS_QUAD2LE(*((volatile UQUAD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))

#define READIO16_LE(rb, offset) AROS_WORD2LE(WORDIN((((UBYTE *) (rb)) + ((ULONG) (offset)))))
#define WRITEIO16_LE(rb, offset, value) WORDOUT((((UBYTE *) (rb)) + ((ULONG) (offset))), AROS_WORD2LE(value))
#define WRITEIO32_LE(rb, offset, value) LONGOUT((((UBYTE *) (rb)) + ((ULONG) (offset))), AROS_WORD2LE(value))

#endif // PCI_AROS_H
