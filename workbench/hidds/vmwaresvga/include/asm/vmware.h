/* SPDX-License-Identifier: GPL-2.0 or MIT */
/*
    VMware hypercall ABI, reduced to the I/O port method (Linux
    arch/x86/include/asm/vmware.h without the vmcall/vmmcall and TDX
    alternatives; the SVGA device is only ever driven from a guest where
    the port interface works).
*/
#ifndef _ASM_X86_VMWARE_H
#define _ASM_X86_VMWARE_H

#include <linux/types.h>
#include <linux/bits.h>

#define VMWARE_HYPERVISOR_HB		BIT(0)
#define VMWARE_HYPERVISOR_OUT		BIT(1)

#define VMWARE_HYPERVISOR_PORT		0x5658
#define VMWARE_HYPERVISOR_PORT_HB	(VMWARE_HYPERVISOR_PORT | \
					 VMWARE_HYPERVISOR_HB)

#define VMWARE_HYPERVISOR_MAGIC		0x564d5868U

#define VMWARE_CMD_GETVERSION		10
#define VMWARE_CMD_GETHZ		45
#define VMWARE_CMD_GETVCPU_INFO		68
#define VMWARE_CMD_STEALCLOCK		91
#define VMWARE_CMD_MASK			0xf007fU

#if defined(__i386__) || defined(__x86_64__)

#define VMWARE_HYPERCALL	"movw %[port], %%dx\n\tinl (%%dx), %%eax"

static inline
unsigned long vmware_hypercall1(unsigned long cmd, unsigned long in1)
{
	unsigned long out0;

	asm volatile (VMWARE_HYPERCALL
		: "=a" (out0)
		: [port] "i" (VMWARE_HYPERVISOR_PORT),
		  "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (in1),
		  "c" (cmd),
		  "d" (0)
		: "cc", "memory");
	return out0;
}

static inline
unsigned long vmware_hypercall3(unsigned long cmd, unsigned long in1,
				u32 *out1, u32 *out2)
{
	unsigned long out0;

	asm volatile (VMWARE_HYPERCALL
		: "=a" (out0), "=b" (*out1), "=c" (*out2)
		: [port] "i" (VMWARE_HYPERVISOR_PORT),
		  "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (in1),
		  "c" (cmd),
		  "d" (0)
		: "di", "si", "cc", "memory");
	return out0;
}

static inline
unsigned long vmware_hypercall4(unsigned long cmd, unsigned long in1,
				u32 *out1, u32 *out2, u32 *out3)
{
	unsigned long out0;

	asm volatile (VMWARE_HYPERCALL
		: "=a" (out0), "=b" (*out1), "=c" (*out2), "=d" (*out3)
		: [port] "i" (VMWARE_HYPERVISOR_PORT),
		  "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (in1),
		  "c" (cmd),
		  "d" (0)
		: "di", "si", "cc", "memory");
	return out0;
}

static inline
unsigned long vmware_hypercall5(unsigned long cmd, unsigned long in1,
				unsigned long in3, unsigned long in4,
				unsigned long in5, u32 *out2)
{
	unsigned long out0;

	asm volatile (VMWARE_HYPERCALL
		: "=a" (out0), "=c" (*out2)
		: [port] "i" (VMWARE_HYPERVISOR_PORT),
		  "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (in1),
		  "c" (cmd),
		  "d" (in3),
		  "S" (in4),
		  "D" (in5)
		: "cc", "memory");
	return out0;
}

static inline
unsigned long vmware_hypercall6(unsigned long cmd, unsigned long in1,
				unsigned long in3, u32 *out2,
				u32 *out3, u32 *out4, u32 *out5)
{
	unsigned long out0;

	asm volatile (VMWARE_HYPERCALL
		: "=a" (out0), "=c" (*out2), "=d" (*out3), "=S" (*out4),
		  "=D" (*out5)
		: [port] "i" (VMWARE_HYPERVISOR_PORT),
		  "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (in1),
		  "c" (cmd),
		  "d" (in3)
		: "cc", "memory");
	return out0;
}

static inline
unsigned long vmware_hypercall7(unsigned long cmd, unsigned long in1,
				unsigned long in3, unsigned long in4,
				unsigned long in5, u32 *out1,
				u32 *out2, u32 *out3)
{
	unsigned long out0;

	asm volatile (VMWARE_HYPERCALL
		: "=a" (out0), "=b" (*out1), "=c" (*out2), "=d" (*out3)
		: [port] "i" (VMWARE_HYPERVISOR_PORT),
		  "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (in1),
		  "c" (cmd),
		  "d" (in3),
		  "S" (in4),
		  "D" (in5)
		: "cc", "memory");
	return out0;
}

#ifdef __x86_64__
#define VMW_BP_CONSTRAINT "r"
#define _ASM_BP "rbp"
#else
#define VMW_BP_CONSTRAINT "m"
#define _ASM_BP "ebp"
#endif

static inline
unsigned long vmware_hypercall_hb_out(unsigned long cmd, unsigned long in2,
				      unsigned long in3, unsigned long in4,
				      unsigned long in5, unsigned long in6,
				      u32 *out1)
{
	unsigned long out0;

	asm volatile (
		"push %%" _ASM_BP "\n\t"
		"mov %[in6], %%" _ASM_BP "\n\t"
		"rep outsb\n\t"
		"pop %%" _ASM_BP "\n\t"
		: "=a" (out0), "=b" (*out1)
		: "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (cmd),
		  "c" (in2),
		  "d" (in3 | VMWARE_HYPERVISOR_PORT_HB),
		  "S" (in4),
		  "D" (in5),
		  [in6] VMW_BP_CONSTRAINT (in6)
		: "cc", "memory");
	return out0;
}

static inline
unsigned long vmware_hypercall_hb_in(unsigned long cmd, unsigned long in2,
				     unsigned long in3, unsigned long in4,
				     unsigned long in5, unsigned long in6,
				     u32 *out1)
{
	unsigned long out0;

	asm volatile (
		"push %%" _ASM_BP "\n\t"
		"mov %[in6], %%" _ASM_BP "\n\t"
		"rep insb\n\t"
		"pop %%" _ASM_BP "\n\t"
		: "=a" (out0), "=b" (*out1)
		: "a" (VMWARE_HYPERVISOR_MAGIC),
		  "b" (cmd),
		  "c" (in2),
		  "d" (in3 | VMWARE_HYPERVISOR_PORT_HB),
		  "S" (in4),
		  "D" (in5),
		  [in6] VMW_BP_CONSTRAINT (in6)
		: "cc", "memory");
	return out0;
}
#undef VMW_BP_CONSTRAINT
#undef VMWARE_HYPERCALL

#endif /* x86 */

#endif
