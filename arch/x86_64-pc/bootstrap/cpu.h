#ifndef CPU_H_
#define CPU_H_

/*
    Copyright (C) 2006 The AROS Development Team. All rights reserved.
    $Id:$

    Desc: Nice macros for low-level assembly support in C/C++ languages
*/

/* Segment registers */
#define SEG_SUPER_CS    0x08
#define SEG_SUPER_DS    0x10
#define SEG_USER_CS32   0x18
#define SEG_USER_CS64   0x28
#define SEG_USER_DS     0x20
#define SEG_TSS         0x30

/* CR0 bits */
#define _CR0_PE_B  0    /* RW: Protected mode enable */
#define _CR0_MP_B  1    /* RW: Monitor FPU? If 1 then #NM exception may be generated */
#define _CR0_EM_B  2    /* RW: Eulate FPU */
#define _CR0_TS_B  3    /* RW: Task switched */
#define _CR0_ET_B  4    /* RO: Exception type */
#define _CR0_NE_B  5    /* RW: Numeric error */
#define _CR0_WP_B 16    /* RW: Write protect for RO pages in supervisor mode */
#define _CR0_AM_B 18    /* RW: Require data alignment */
#define _CR0_NW_B 29    /* RW: IGNORED: Not writethrough */
#define _CR0_CD_B 30    /* RW: Cache disable */
#define _CR0_PG_B 31    /* RW: Paging enable */

#define _CR0_PE (1 << _CRO_PE_B)
#define _CR0_MP (1 << _CR0_MP_B)
#define _CR0_EM (1 << _CR0_EM_B)
#define _CR0_TS (1 << _CR0_TS_B)
#define _CR0_ET (1 << _CR0_ET_B)
#define _CR0_NE (1 << _CR0_NE_B)
#define _CR0_WP (1 << _CR0_WP_B)
#define _CR0_AM (1 << _CR0_AM_B)
#define _CR0_NW (1 << _CR0_NW_B)
#define _CR0_CD (1 << _CR0_CD_B)
#define _CR0_PG (1 << _CR0_PG_B)

/* CR3 bits */
#define _CR3_PWT_B  3   /* RW: Page writethrough */
#define _CR3_PCD_B  4   /* RW: Cache disable */

#define _CR3_PWT (1 << _CR3_PWT_B)
#define _CR3_PCD (1 << _CR3_PCD_B)

/* CR4 bits */
#define _CR4_VME_B       0  /* RW: Virtual-8086 enable */
#define _CR4_PVI_B       1  /* RW: Protected mode virtual interrupts */
#define _CR4_TSD_B       2  /* RW: Time stamp disable for usermode */
#define _CR4_DE_B        3  /* RW: Debug extensions */
#define _CR4_PSE_B       4  /* RW: Page size extensions */
#define _CR4_PAE_B       5  /* RW: Physical-address extensions */
#define _CR4_MCE_B       6  /* RW: Machine check enable */
#define _CR4_PGE_B       7  /* RW: Page-Global enable */
#define _CR4_PCE_B       8  /* RW: Performance monitoring counter enable */
#define _CR4_OSFXSR_B    9  /* RW: Operating system fxsave/fsrstor support */
#define _CR4_OSXMMEXCPT_B 10 /*RW: Operating system unmasked exception support */

#define _CR4_VME (1 << _CR4_VME_B)
#define _CR4_PVI (1 << _CR4_PVI_B)
#define _CR4_TSD (1 << _CR4_TSD_B)
#define _CR4_DE  (1 << _CR4_DE_B)
#define _CR4_PSE (1 << _CR4_PSE_B)
#define _CR4_PAE (1 << _CR4_PAE_B)
#define _CR4_MCE (1 << _CR4_MCE_B)
#define _CR4_PGE (1 << _CR4_PGE_B)
#define _CR4_PCE (1 << _CR4_PCE_B)
#define _CR4_OSFXSR (1 << _CR4_OSFXSR_B)
#define _CR4_OSXMMEXCPT (1 << _CR4_OSXMMEXCPT_B)

/* EFER */
#define EFER        0xc0000080  /* EFER number for rsmsr/wrmsr */
#define _EFER_SCE_B      0      /* RW: System call extensions */
#define _EFER_LME_B      8      /* RW: Long mode enable */
#define _EFER_LMA_B     10      /* RW: Long mode activated */
#define _EFER_NXE_B     11      /* RW: No-execute bit enable */
#define _EFER_FFXSR_B   14      /* RW: Fast fxsave/fxrstor */

#define _EFER_SCE   (1 << _EFER_SCE_B)
#define _EFER_LME   (1 << _EFER_LME_B)
#define _EFER_LMA   (1 << _EFER_LMA_B)
#define _EFER_NXE   (1 << _EFER_NXE_B)
#define _EFER_FFXSR (1 << _EFER_FFXSR_B)

struct int_gate_64bit {
    unsigned short       offset_low;
    unsigned short       selector;
    unsigned    ist:3, __pad0:5, type:5, dpl:2, p:1;
    unsigned short       offset_mid;
    unsigned int       offset_high;
    unsigned int       __pad1;
} __attribute__((packed));

struct segment_desc {
    unsigned short       limit_low;
    unsigned short       base_low;
    unsigned    base_mid:8, type:5, dpl:2, p:1;
    unsigned    limit_high:4, avl:1, l:1, d:1, g:1, base_high:8;
} __attribute__((packed));

struct segment_ext {
    unsigned int       base_ext;
    unsigned int       __pad0;
} __attribute__((packed));

struct tss_64bit {
    unsigned int       __pad0;
    unsigned long long       rsp0;
    unsigned long long       rsp1;
    unsigned long long       rsp2;
    unsigned long long       __pad1;
    unsigned long long       ist1;
    unsigned long long       ist2;
    unsigned long long       ist3;
    unsigned long long       ist4;
    unsigned long long       ist5;
    unsigned long long       ist6;
    unsigned long long       ist7;
    unsigned long long       __pad2;
    unsigned short       __pad3;
    unsigned short       iopb;
    unsigned int       bmp[];
} __attribute__((packed));

struct PML4E {
    unsigned p:1,rw:1,us:1,pwt:1,pcd:1,a:1,__pad0:1,mbz:2,avl:3,base_low:20;
    unsigned base_high:20,avail:11,nx:1;
} __attribute__((packed));

struct PDPE {
    unsigned p:1,rw:1,us:1,pwt:1,pcd:1,a:1,__pad0:1,mbz:2,avl:3,base_low:20;
    unsigned base_high:20,avail:11,nx:1;
} __attribute__((packed));

struct PDE4K {
    unsigned p:1,rw:1,us:1,pwt:1,pcd:1,a:1,__pad0:1,ps:1,_pad1:1,avl:3,base_low:20;
    unsigned base_high:20,avail:11,nx:1;
} __attribute__((packed));

struct PDE2M {
    unsigned p:1,rw:1,us:1,pwt:1,pcd:1,a:1,d:1,ps:1,g:1,avl:3,pat:1,base_low:19;
    unsigned base_high:20,avail:11,nx:1;
} __attribute__((packed));

struct PTE {
    unsigned p:1,rw:1,us:1,pwt:1,pcd:1,a:1,d:1,pat:1,g:1,avl:3,base_low:20;
    unsigned base_high:20,avail:11,nx:1;
} __attribute__((packed));

#define _ljmp(seg, addr) \
    do { asm volatile("ljmp $" #seg ", $" #addr); }while(0)
#define ljmp(s, a) _ljmp(s, a)

#define rdcr(reg) \
    ({ long val; asm volatile("mov %%" #reg ",%0":"=r"(val)); val; })

#define wrcr(reg, val) \
    do { asm volatile("mov %0,%%" #reg::"r"(val)); } while(0)

inline void rdmsr(unsigned int msr_no, unsigned int *ret_lo, unsigned int *ret_hi)
{
    unsigned int ret1,ret2;
    asm volatile("rdmsr":"=a"(ret1),"=d"(ret2):"c"(msr_no));
    *ret_lo=ret1;
    *ret_hi=ret2;
}

inline long long rdmsrq(unsigned int msr_no)
{
    unsigned int ret1,ret2;
    asm volatile("rdmsr":"=a"(ret1),"=d"(ret2):"c"(msr_no));
    return ((long long)ret1 | ((long long)ret2 << 32));
}

inline void wrmsr(unsigned int msr_no, unsigned int val_lo, unsigned int val_hi)
{
    asm volatile("wrmsr"::"a"(val_lo),"d"(val_hi),"c"(msr_no));
}

inline void wrmsrq(unsigned int msr_no, unsigned long long val)
{
    asm volatile("wrmsr"::"a"((unsigned int)(val & 0xffffffff)),"d"((unsigned int)(val >> 32)),"c"(msr_no));
}

#endif /*CPU_H_*/
