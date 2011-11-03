/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IA-32 APIC hardware definitions.
    Lang: english
*/

/* Local APIC base address register (MSR #27) */
#define MSR_LAPIC_BASE 0x1B

#define APIC_BOOTSTRAP (1 << 8)
#define APIC_ENABLE    (1 << 11)

/* APIC hardware registers */

#define APIC_ID		 0x20
#define APIC_VERSION	 0x30
#define APIC_TPR	 0x80	/* Task Priority Register			*/
#define APIC_APR	 0x90	/* Arbitration Priority Register		*/
#define APIC_PPR	 0xA0	/* Processor Priority Register			*/
#define APIC_EOI	 0xB0	/* End Of Interrupt Register			*/
#define APIC_REMOTE_READ 0xC0
#define APIC_LDR	 0xD0	/* Logical Destination Register			*/
#define APIC_DFR	 0xE0	/* Destination Format Register			*/
#define APIC_SVR	 0xF0	/* Spurious Interrupt Vector Register		*/
#define APIC_ISR	 0x100	/* In Service Register				*/
#define APIC_TMR	 0x180	/* Trigger Mode Register			*/
#define APIC_IRR	 0x200	/* Interrupt Request Register			*/
#define APIC_ESR	 0x280	/* Error Status Register			*/
#define APIC_ICRL	 0x300	/* Interrupt Command Register low part		*/
#define APIC_ICRH	 0x310	/* Interrupt Command Register high part 	*/
#define APIC_TIMER_VEC	 0x320	/* Timer local vector table entry		*/
#define APIC_THERMAL_VEC 0x330	/* Thermal local vector table entry		*/
#define APIC_PCOUNT_VEC	 0x340	/* Performance counter local vector table entry	*/
#define APIC_LINT0_VEC	 0x350	/* Local interrupt 0 vector table entry		*/
#define APIC_LINT1_VEC	 0x360	/* Local interrupt 1 vector table entry		*/
#define APIC_ERROR_VEC	 0x370	/* Error vector table entry			*/
#define APIC_TIMER_ICR	 0x380	/* Timer initial count				*/
#define APIC_TIMER_CCR	 0x390	/* Timer current count				*/
#define APIC_TIMER_DIV	 0x3E0	/* Timer divide configuration register		*/

/* ID shift value */
#define APIC_ID_SHIFT 24

/* Version register */
#define APIC_VERSION_MASK 0x000000FF	/* The actual version number			     */
#define APIC_LVT_MASK 	  0x00FF0000	/* Number of entries in local vector table minus one */
#define APIC_LVT_SHIFT	  16
#define APIC_EAS	  (1 << 31)	/* Whether this APIC has extended address space	     */

/* Macros to help parsing version */
#define APIC_INTEGRATED(ver) (ver & 0x000000F0)
#define APIC_LVT(ver)	     ((ver & APIC_LVT_MASK) >> APIC_LVT_SHIFT)

/* LDR shift value */
#define LDR_ID_SHIFT 24

/* Destination format (interrupt model) */
#define DFR_CLUSTER (0x0 << 28)
#define DFR_FLAT    (0xF << 28)

#define SVR_VEC_MASK 0xFF
#define SVR_ASE	     (1 << 8)
#define SVR_FCC	     (1 << 9)

/* Error register */
#define ERR_SAE (1 << 2) /* Sent accept error	     */
#define ERR_RAE (1 << 3) /* Receive accept error     */
#define ERR_SIV (1 << 5) /* Sent illegal vector	     */
#define ERR_RIV (1 << 6) /* Received illegal vector  */
#define ERR_IRA (1 << 7) /* Illegal register address */

/* ICRL register */
#define ICR_VEC_MASK	  0x000000FF	/* Vector number (request argument) mask	*/
#define ICR_DM_INIT       0x0500	/* INIT request (reset the CPU)			*/
#define ICR_DM_STARTUP    0x0600	/* STARTUP request (run from specified address)	*/
#define ICR_DS		  0x1000	/* Delivery status flag				*/
#define ICR_INT_LEVELTRIG 0x8000	/* Send level-triggered interrupt		*/
#define ICR_INT_ASSERT    0x4000	/* Assert (set) or deassert (reset)		*/

/* Local vector table entry fields */
#define LVT_VEC_MASK   0x0000FF		/* Vector no				*/
#define LVT_MT_MASK    0x000700		/* Message type				*/
#define LVT_MT_FIXED   0x000000
#define LVT_MT_SMI     0x000200
#define LVT_MT_NMI     0x000400
#define LVT_MT_EXT     0x000700
#define LVT_DS	       0x001000		/* Delivery status bit 			*/
#define LVT_ACTIVE_LOW 0x002000		/* Polarity flag (1 = low active)	*/
#define LVT_RIR	       0x004000		/* Remote IRR				*/
#define LVT_TGM_LEVEL  0x008000		/* Level-trigger mode			*/
#define LVT_MASK       0x010000		/* Mask bit				*/
#define LVT_TMM_PERIOD 0x020000		/* Periodic timer mode			*/

/* Timer divisors */
#define TIMER_DIV_1   0x0B
#define TIMER_DIV_2   0x00
#define TIMER_DIV_4   0x01
#define TIMER_DIV_8   0x02
#define TIMER_DIV_16  0x03
#define TIMER_DIV_32  0x08
#define TIMER_DIV_64  0x09
#define TIMER_DIV_128 0x0A

/* Register access macro to make the code more readable */
#define APIC_REG(base, reg) *((volatile ULONG *)(base + reg))
