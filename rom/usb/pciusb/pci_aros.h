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

#ifndef SYNC
#define SYNC
#endif

#ifndef EIEIO
#define EIEIO
#endif

#if 1

#define READMEM16_LE(rb)                 AROS_WORD2LE(*((UWORD *) (rb)))
#define READMEM32_LE(rb)                 AROS_LONG2LE(*((ULONG *) (rb)))
#define	WRITEMEM32_LE(adr, value)	     *((ULONG *) (adr)) = AROS_LONG2LE(value)
#define CONSTWRITEMEM32_LE(adr, value)   *((ULONG *) (adr)) = AROS_LONG2LE(value)

#define CONSTWRITEREG16_LE(rb, offset, value) *((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_WORD2LE(value)
#define CONSTWRITEREG32_LE(rb, offset, value) *((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_LONG2LE(value)
#define WRITEREG16_LE(rb, offset, value) *((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_WORD2LE(value)
#define WRITEREG32_LE(rb, offset, value) *((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_LONG2LE(value)

#define READREG16_LE(rb, offset)         AROS_WORD2LE(*((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))
#define READREG32_LE(rb, offset)         AROS_LONG2LE(*((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))

#else

#define READMEM16_LE(rb) inw(rb)
#define READMEM32_LE(rb) inl(rb)

#define WRITEMEM32_LE(adr, value) outl(value, adr)
#define CONSTWRITEMEM32_LE(adr, value) outl(value, adr)

#define CONSTWRITEREG16_LE(rb, offset, value) outw(value, (((UBYTE *) (rb)) + ((ULONG) (offset))))
#define CONSTWRITEREG32_LE(rb, offset, value) outl(value, (((UBYTE *) (rb)) + ((ULONG) (offset))))
#define WRITEREG16_LE(rb, offset, value) outw(value, (((UBYTE *) (rb)) + ((ULONG) (offset))))
#define WRITEREG32_LE(rb, offset, value) outl(value, (((UBYTE *) (rb)) + ((ULONG) (offset))))

#define READREG16_LE(rb, offset) inw(((UBYTE *) (rb)) + ((ULONG) (offset))))
#define READREG32_LE(rb, offset) inl((((UBYTE *) (rb)) + ((ULONG) (offset))))

#endif

#endif // PCI_AROS_H
