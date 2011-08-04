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
#define APIC_BASE_MASK 0x000FFFFFFFFFF000

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
#define APIC_TIMER_DIV	 0x3E0	/* Timer divide configuration register		*/

/* Version register */
#define APIC_VERSION_MASK 0x000000FF	/* The actual version number			     */
#define APIC_LVT_MASK 	  0x00FF0000	/* Number of entries in local vector table minus one */
#define APIC_LVT_SHIFT	  16
#define APIC_EAS	  (1 << 31)	/* Whether this APIC has extended address space	     */

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

/* Register access macro to make the code more readable */
#define APIC_REG(base, reg) *((volatile ULONG *)(base + reg))
