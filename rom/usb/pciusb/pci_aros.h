#ifndef PCI_AROS_H
#define PCI_AROS_H

/*
 *----------------------------------------------------------------------------
 *             Includes for AROS PCI handling
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>

 *
 */

/*
 * Memory barrier before MMIO/register updates:
 * ensure descriptors and software bookkeeping are globally visible before
 * notifying the controller. Keep consistent across EHCI/OHCI/UHCI drivers.
 */
#undef SYNC
#if defined(__GNUC__) || defined(__clang__)
/* PowerPC: enforce ordering for device/MMIO writes. */
# if defined(__powerpc__)
#  define PCIUSB_MMIO_BARRIER() __asm__ __volatile__("eieio" ::: "memory")

/* ARM (AArch64 / ARMv7+): ensure all prior writes are observable before MMIO. */
# elif defined(__aarch64__) || defined(__arm__)
#  define PCIUSB_MMIO_BARRIER() __asm__ __volatile__("dmb ishst" ::: "memory")

/* RISC-V: order all IO + memory accesses around MMIO stores. */
# elif defined(__riscv)
#  define PCIUSB_MMIO_BARRIER() __asm__ __volatile__("fence iorw, iorw" ::: "memory")

/* Other architectures: use a release fence (compiler + arch barrier as needed). */
# else
#  define PCIUSB_MMIO_BARRIER() __atomic_thread_fence(__ATOMIC_RELEASE)
# endif
#else
/* Fallback: at least prevent compiler reordering if nothing better exists. */
# define PCIUSB_MMIO_BARRIER() do { } while (0)
#endif

#define SYNC PCIUSB_MMIO_BARRIER()

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
