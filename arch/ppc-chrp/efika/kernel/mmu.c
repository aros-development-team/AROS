/*
 * mmu.c
 *
 *  Created on: Aug 26, 2008
 *      Author: misc
 */

#include <inttypes.h>
#include <asm/mpc5200b.h>
#include <asm/io.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"

typedef struct {
	uint32_t 	vsid;
	uint32_t	rpn;
} pte_t;

int mmu_map_page(uint64_t virt, uint32_t phys, uint32_t prot)
{
	uint32_t mask = ((rdspr(SDR1) & 0x1ff) << 16) | 0xffc0;
	pte_t *pteg;
	pte_t *pte = NULL;
	pte_t local_pte;
	int ptenum;

//	D(bug("[KRN] mmu_map_page(%06x%07x, %08x, %08x)\n", (uint32_t)(virt >> 28), (uint32_t)(virt & 0x0fffffff), phys, prot));

	/* Calculate the hash function */
	uint32_t hash = (((uint32_t)(virt >> 12) & 0xffff) ^ (uint32_t)(virt >> 28)) & 0x7ffff;

	pteg = (pte_t *)((rdspr(SDR1) & ~0x1ff) | ((hash << 6) & mask));

//	D(bug("[KRN] primary hash: %08x, pteg: %08x\n", hash, pteg));

	for (ptenum = 0; ptenum < 8; ptenum++)
	{
		if (!(pteg[ptenum].vsid & 0x80000000))
		{
//			D(bug("[KRN] found free pteg at %08x\n", &pteg[ptenum]));
			pte = &pteg[ptenum];
			local_pte.vsid = 0;
			break;
		}
	}
	if (!pte)
	{
		uint32_t hash2 = (~hash) & 0x7ffff;
		pteg = (pte_t *)((rdspr(SDR1) & ~0x1ff) | ((hash2 << 6) & mask));
//		D(bug("[KRN] primary hash: %08x, pteg: %08x\n", hash, pteg));

		for (ptenum = 0; ptenum < 8; ptenum++)
		{
			if (!(pteg[ptenum].vsid & 0x80000000))
			{
//				D(bug("[KRN] found free pteg at %08x\n", &pteg[ptenum]));
				pte = &pteg[ptenum];
				local_pte.vsid = 0x40;
				break;
			}
		}
	}

	if (!pte)
	{
		D(bug("[KRN] mmu_map_page(%06x%07x, %08x, %08x)\n", (uint32_t)(virt >> 28), (uint32_t)(virt & 0x0fffffff), phys, prot));
		D(bug("[KRN] Run out of free page table entries\n"));
		return 0;
	}

	local_pte.vsid |= ((virt >> 28) << 7);
	local_pte.vsid |= ((virt >> 22) & 0x3f);
	local_pte.vsid |= 0x80000000;
	local_pte.rpn = (phys & ~0xfff) | (prot & 0xfff);

//	D(bug("[KRN] pte=%08x:%08x\n", local_pte.vsid, local_pte.rpn));
	*pte = local_pte;

	asm volatile("dcbst 0,%0; sync;"::"r"(pte));

	return 1;
}

int mmu_map_area(uint64_t virt, uint32_t phys, uint32_t length, uint32_t prot)
{
//	D(bug("[KRN] mmu_map_area(%06x%07x - %06x%07x, %08x, %08x)\n",
//			(uint32_t)(virt >> 28), (uint32_t)(virt & 0x0fffffff),
//			(uint32_t)((virt + length - 1) >> 28), (uint32_t)((virt + length - 1) & 0x0fffffff),
//			phys, prot
//			));
//
	while (length)
	{
		if (!mmu_map_page(virt, phys, prot))
			return 0;

		virt += 4096;
		phys += 4096;
		length -= 4096;
	}

	return 1;
}

void mmu_init(char *mmu_dir, uint32_t mmu_size)
{
	int i;

	D(bug("[KRN] Initializing MMU\n"));
	D(bug("[KRN] Location of MMU tables: %08x-%08x\n", mmu_dir, mmu_dir + mmu_size - 1));

	if ((intptr_t)mmu_dir & (mmu_size - 1))
	{
		D(bug("[KRN] WRONG! The MMU dir must be located on mmu length boundary\n"));
	}
	else
	{
		/* Clear the MMU tables */
		bzero(mmu_dir, mmu_size);

		uint32_t sdr = (intptr_t)mmu_dir | ((mmu_size >> 16) - 1);

		D(bug("[KRN] SDR1 = %08x\n", sdr));

		wrspr(SDR1, sdr);

		/* Prepare the segment registers. The proper values for virtual address
		 * are to be determined later */

		for (i=0; i < 16; i++)
		{
			asm volatile ("mtsrin %0,%1"::"r"(0x20000000 | i),"r"(i << 28));
		}
	}
}

void __attribute__((noreturn)) mmu_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct ExecBase *SysBase = getSysBase();

    D(bug("[KRN] Exception %d handler. Context @ %p, SysBase @ %p, KernelBase @ %p\n", exception, ctx, SysBase, KernelBase));
    if (SysBase)
    {
        struct Task *t = FindTask(NULL);
        D(bug("[KRN] %s %p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--"));
    }
    D(bug("[KRN] SRR0=%08x, SRR1=%08x\n",ctx->srr0, ctx->srr1));
    D(bug("[KRN] CTR=%08x LR=%08x XER=%08x CCR=%08x\n", ctx->ctr, ctx->lr, ctx->xer, ctx->ccr));
    D(bug("[KRN] DAR=%08x DSISR=%08x\n", ctx->dar, ctx->dsisr));

    D(bug("[KRN] HASH1=%08x HASH2=%08x IMISS=%08x DMISS=%08x ICMP=%08x DCMP=%08x\n",
        		rdspr(978), rdspr(979), rdspr(980), rdspr(976), rdspr(981), rdspr(977)));

    D(bug("[KRN] SPRG0=%08x SPRG1=%08x SPRG2=%08x SPRG3=%08x SPRG4=%08x SPRG5=%08x\n",
    		rdspr(SPRG0),rdspr(SPRG1),rdspr(SPRG2),rdspr(SPRG3),rdspr(SPRG4),rdspr(SPRG5)));

    D(bug("[KRN] GPR00=%08x GPR01=%08x GPR02=%08x GPR03=%08x\n",
             ctx->gpr[0],ctx->gpr[1],ctx->gpr[2],ctx->gpr[3]));
    D(bug("[KRN] GPR04=%08x GPR05=%08x GPR06=%08x GPR07=%08x\n",
             ctx->gpr[4],ctx->gpr[5],ctx->gpr[6],ctx->gpr[7]));
    D(bug("[KRN] GPR08=%08x GPR09=%08x GPR10=%08x GPR11=%08x\n",
             ctx->gpr[8],ctx->gpr[9],ctx->gpr[10],ctx->gpr[11]));
    D(bug("[KRN] GPR12=%08x GPR13=%08x GPR14=%08x GPR15=%08x\n",
             ctx->gpr[12],ctx->gpr[13],ctx->gpr[14],ctx->gpr[15]));

    D(bug("[KRN] GPR16=%08x GPR17=%08x GPR18=%08x GPR19=%08x\n",
             ctx->gpr[16],ctx->gpr[17],ctx->gpr[18],ctx->gpr[19]));
    D(bug("[KRN] GPR20=%08x GPR21=%08x GPR22=%08x GPR23=%08x\n",
             ctx->gpr[20],ctx->gpr[21],ctx->gpr[22],ctx->gpr[23]));
    D(bug("[KRN] GPR24=%08x GPR25=%08x GPR26=%08x GPR27=%08x\n",
             ctx->gpr[24],ctx->gpr[25],ctx->gpr[26],ctx->gpr[27]));
    D(bug("[KRN] GPR28=%08x GPR29=%08x GPR30=%08x GPR31=%08x\n",
             ctx->gpr[28],ctx->gpr[29],ctx->gpr[30],ctx->gpr[31]));

    D(bug("[KRN] Instruction dump:\n"));
    int i;
    ULONG *p = (ULONG*)ctx->srr0;
    for (i=0; i < 8; i++)
    {
        D(bug("[KRN] %08x: %08x\n", &p[i], p[i]));
    }

    D(bug("[KRN] **UNHANDLED EXCEPTION** stopping here...\n"));

    while(1) {
    	wrmsr(rdmsr() | MSR_POW);
    }
}



AROS_LH4(int, KrnMapGlobal,
         AROS_LHA(void *, virtual, A0),
         AROS_LHA(void *, physical, A1),
         AROS_LHA(uint32_t, length, D0),
         AROS_LHA(KRN_MapAttr, flags, D1),
         struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    uint32_t ppc_prot = 2 << 3;	/* WIMG = 0010 */

    D(bug("[KRN] KrnMapGlobal(%08x->%08x %08x %04x)\n", virtual, physical, length, flags));

    if (flags & MAP_Readable)
    {
    	ppc_prot |= 0x03;
    }
    if (flags & MAP_Writable)
    {
    	ppc_prot = (ppc_prot | 2) & ~1;
    }

    if (flags & MAP_WriteThrough)
    {
    	ppc_prot |= 8 << 3;
    }
    if (flags & MAP_Guarded)
    {
    	ppc_prot |= 1 << 3;
    }
    if (flags & MAP_CacheInhibit)
    {
    	ppc_prot = (ppc_prot | 4 << 3) & ~ (8 << 3);
    }

    return mmu_map_area((uint64_t)virtual & 0xffffffff, physical, length, ppc_prot);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(int, KrnUnmapGlobal,
		AROS_LHA(void *, virtual, A0),
		AROS_LHA(uint32_t, length, D0),
		struct KernelBase *, KernelBase, 10, Kernel)
{
	AROS_LIBFUNC_INIT

	AROS_LIBFUNC_EXIT
}






/* MMU exception handlers follow the G2 core manual */
static void __attribute__((used)) __exception_template()
{
	asm volatile("\n"
			".set	dMiss, 976\n"
			".set	dCmp, 977\n"
			".set	hash1, 978\n"
			".set	hash2, 979\n"
			".set	iMiss, 980\n"
			".set	iCmp, 981\n"
			".set	rpa, 982\n"
			".set	c0, 0\n"
			".set	dar, 19\n"
			".set	dsisr, 18\n"
			".set	srr0, 26\n"
			".set	srr1, 27\n"
			);
	/*
	 * Instruction TB miss flow
	 * Entry:
	 *
	 * Vec = 1000
	 * srr0 -> address of instruction that missed
	 * srr1 -> 0:3=cr0 4=lru way bit 16:31 = saved MSR
	 * msr<tgpr> -> 1
	 * iMiss -> ea that missed
	 * iCmp -> the compare value for the va that missed
	 * hash1 -> pointer to first hash pteg
	 * hash2 -> pointer to second hash pteg
	 *
	 * Register usage:
	 *
	 * r0 is saved counter
	 * r1 is junk
	 * r2 is pointer to pteg
	 * r3 is current compare value
	 */
	asm volatile(".align 8; .globl __vector_imiss; .type __vector_imiss,@function\n"
"__vector_imiss:\n"
"		mfspr 	%r2,hash1	\n"
"		addi	%r1,0,8		\n"
"		mfctr	%r0			\n"
"		mfspr	%r3,iCmp	\n"
"		addi	%r2,%r2,-8	\n"
"im0:	mtctr	%r1			\n"
"im1:	lwzu	%r1,8(%r2)	\n"
"		cmp		c0,%r1,%r3	\n"
"		bdnzf	eq, im1		\n"
"		bne		instrSecHash\n"
"		l		%r1,+4(%r2)	\n"
"		andi.	%r3,%r1,8	\n"
"		bne		doISIp		\n"
"		mtctr	%r0			\n"
"		mfspr	%r0,iMiss	\n"
"		mfspr	%r3,srr1	\n"
"		mtcrf	0x80,%r3	\n"
"		mtspr	rpa,%r1		\n"
"		ori		%r1,%r1,0x100 \n"
"		srwi	%r1,%r1,8	\n"
"		tlbli	%r0			\n"
"		stb		%r1,+6(%r2)	\n"
"		rfi					\n"

"instrSecHash:	\n"
"		andi.	%r1,%r3,0x0040	\n"
"		bne		doISI		\n"
"		mfspr	%r2,hash2	\n"
"		ori		%r3,%r3,0x0040	\n"
"		addi	%r1,0,8		\n"
"		addi	%r2,%r2,-8	\n"
"		b		im0			\n"

"doISIp:					\n"
"		mfspr	%r3, srr1	\n"
"		andi.	%r2,%r3,0xffff	\n"
"		addis	%r2,%r2,0x0800	\n"
"		b		isi1		\n"
"doISI:						\n"
"		mfspr	%r3, srr1	\n"
"		andi.	%r2,%r3,0xffff\n"
"		addis	%r2,%r2,0x4000\n"
"isi1:						\n"
"		mtctr	%r0			\n"
"		mtspr	srr1,%r2	\n"
"		mfmsr	%r0			\n"
"		xoris	%r0,%r0,0x8002\n"
"		mtcrf	0x80,%r3	\n"
"		mtmsr	%r0			\n"
"		ba		0x0400		\n"
	);

	/*
	 * Data TLB miss flow
	 * Entry:
	 *
	 * Vec = 1100
	 * srr0 -> address of instruction that caused data tlb miss
	 * srr1 -> 0:3=cr0 4=lru way bit 5=1 if store 16:31 = saved MSR
	 * msr<tgpr> -> 1
	 * dMiss -> ea that missed
	 * dCmp -> the compare value for the va that missed
	 * hash1 -> pointer to first hash pteg
	 * hash2 -> pointer to second hash pteg
	 *
	 * Register usage:
	 *
	 * r0 is saved counter
	 * r1 is junk
	 * r2 is pointer to pteg
	 * r3 is current compare value
	 */
	asm volatile(".align 8; .globl __vector_dmiss; .type __vector_dmiss,@function\n"
"__vector_dmiss:\n"
"		mfspr 	%r2, hash1	\n"
"		addi	%r1, 0, 8	\n"
"		mfctr	%r0			\n"
"		mfspr	%r3, dCmp	\n"
"		addi	%r2, %r2, -8\n"
"dm0:	mtctr	%r1			\n"
"dm1:	lwzu	%r1, 8(%r2)	\n"
"		cmp		c0, %r1, %r3\n"
"		bdnzf	0, dm1		\n"
"		bne		dataSecHash	\n"
"		l		%r1, +4(%r2)\n"
"		mtctr	%r0			\n"
"		mfspr	%r0, dMiss	\n"
"		mfspr	%r3, srr1	\n"
"		mtcrf	0x80, %r3	\n"
"		mtspr	rpa, %r1	\n"
"		ori		%r1, %r1, 0x100\n"
"		srw		%r1, %r1, 8	\n"
"		tlbld	%r0			\n"
"		stb		%r1, +6(%r2)\n"
"		rfi					\n"

"dataSecHash:				\n"
"		andi.	%r1, %r3, 0x0040\n"
"		bne		doDSI		\n"
"		mfspr	%r2, hash2	\n"
"		ori		%r3, %r3, 0x0040\n"
"		addi	%r1, 0, 8	\n"
"		addi	%r2, %r2, -8\n"
"		b		dm0			\n"

	);

	asm volatile(".align 8; .globl __vector_dmissw; .type __vector_dmiss,@function\n"
"__vector_dmissw:\n"
"		mfspr	%r2, hash1	\n"
"		addi	%r1, 0, 8	\n"
"		mfctr	%r0			\n"
"		mfspr	%r3, dCmp	\n"
"		addi	%r2, %r2, -8\n"
"ceq0:	mtctr	%r1			\n"
"ceq1:	lwzu	%r1, 8(%r2)	\n"
"		cmp		c0, %r1, %r3\n"
"		bdnzf	0, ceq1		\n"
"		bne		cEq0SecHash	\n"
"		l		%r1, +4(%r2)\n"
"		andi.	%r3,%r1,0x80\n"
"		beq		cEq0ChkProt	\n"
"ceq2:	mtctr	%r0			\n"
"		mfspr	%r0, dMiss	\n"
"		mfspr	%r3, srr1	\n"
"		mtcrf	0x80, %r3	\n"
"		mtspr	rpa, %r1	\n"
"		tlbld	%r0			\n"
"		rfi					\n"

"cEq0SecHash:				\n"
"		andi.	%r1, %r3, 0x0040\n"
"		bne		doDSI		\n"
"		mfspr	%r2, hash2	\n"
"		ori		%r3, %r3, 0x0040\n"
"		addi	%r1, 0, 8	\n"
"		addi	%r2, %r2, -8\n"
"		b		ceq0		\n"

"cEq0ChkProt:				\n"
"		rlwinm.	%r3,%r1,30,0,1\n"
"		bge-	chk0		\n"
"		andi.	%r3,%r1,1	\n"
"		beq+	chk2		\n"
"		b		doDSIp		\n"
"chk0:	mfspr	%r3,srr1		\n"
"		andis.	%r3,%r3,0x0008\n"
"		beq		chk2		\n"
"		b		doDSIp		\n"

"chk2:	ori		%r1, %r1, 0x180\n"
"		sth		%r1, 6(%r2)	\n"
"		b		ceq2		\n"

"doDSI:						\n"
"		mfspr	%r3, srr1	\n"
"		rlwinm	%r1, %r3, 9,6,6	\n"
"		addis	%r1, %r1, 0x4000	\n"
"		b		dsi1		\n"

"doDSIp:					\n"
"		mfspr	%r3, srr1	\n"
"		rlwinm	%r1, %r3, 9,6,6	\n"
"		addis	%r1, %r1, 0x0800	\n"

"dsi1:						\n"
"		mtctr	%r0			\n"
"		andi.	%r2, %r3, 0xffff	\n"
"		mtspr	srr1, %r2	\n"
"		mtspr	dsisr, %r1	\n"
"		mfspr	%r1, dMiss	\n"
"		rlwinm.	%r2,%r2,0,31,31	\n"
"		beq		dsi2		\n"
"		xor		%r1,%r1,0x07	\n"

"dsi2:						\n"
"		mtspr	dar, %r1		\n"
"		mfmsr	%r0			\n"
"		xoris	%r0, %r0, 0x2	\n"
"		mtcrf	0x80, %r3	\n"
"		mtmsr	%r0			\n"
"		ba		0x0300		\n"

	);

	asm volatile(".align 8;");
}
