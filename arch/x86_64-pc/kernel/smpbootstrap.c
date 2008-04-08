#include "../bootstrap/cpu.h"

extern const struct {
    unsigned short size __attribute__((packed));
    unsigned long addr __attribute__((packed));
} smp_gdt64_sel;

extern const struct {
    void *target_ip __attribute__((packed));
    unsigned short target_sel __attribute__((packed));
} smp_kernel_target;

extern unsigned long smp_mmu, smp_arg1, smp_arg2, smp_arg3, smp_arg4;

asm (".code16           \n"
"       .type   smpbootstrap_0,@function \n"
"smpbootstrap_0:                \n"
"       cli                     \n"
"       /* The config variables will be passed here... Do not execute them */ \n"
"       jmp 1f                  \n"
"                               \n"
"               .align 4        \n"
"smp_arg1:      .long 0         \n"     /* 0x0004 */
"smp_arg2:      .long 0         \n"     /* 0x0008 */
"smp_arg3:      .long 0         \n"     /* 0x000c */
"smp_arg4:      .long 0         \n"     /* 0x0010 */
"smp_mmu:       .long 0         \n"     /* 0x0014 */
"smp_gdt:       .short 0; .short 0; .short 0; .short 0 \n"
"               .short 0x1000; .short 0x0000; .short 0x9a00; .short 0x0040 \n"
"               .short 0x1000; .short 0x0000; .short 0x9200; .short 0x0040 \n"
"smp_gdt64:     .short 0; .short 0; .short 0; .short 0 \n"
"               .short 0xffff; .short 0x0000; .short 0x9a00; .short 0x00af \n"
"               .short 0xffff; .short 0x0000; .short 0x9200; .short 0x00af \n"
"smp_gdt_sel:   .short 23       \n"
"               .long 0         \n"
"smp_gdt64_sel: .short 23       \n"
"               .long 0         \n"
"smp_kernel_target: .long 0; .short 8\n"
"                               \n"
"1:                             \n"
"       mov %cs,%ax             \n"     /* Find out where the code resides */
"       shl $4, %eax            \n"
"       leal smp_gdt(%eax), %ebx \n"    /* Load physical address of 32-bit gdt */
"       movl %ebx, %cs:smp_gdt_sel+2\n" /* Set up 32-bit gdt address */
"       leal smp_gdt64(%eax), %ebx \n"  /* Load physical address of 64-bit gdt */
"       movl %ebx, %cs:smp_gdt64_sel+2\n"       /* Set up 64-bit gdt address */
"       leal boot64(%eax), %ebx \n"     /* Load physical address of 64-bit code */
"       movl %ebx, %cs:smp_kernel_target \n"    /* Set up 64-bit entry address */
"       movw %ax, %cs:smp_gdt+10 \n"    /* Set base address of 32-bit code segment (bits 0:15) */
"       movw %ax, %cs:smp_gdt+18 \n"    /* Set base address of 32-bit data segment (bits 0:15) */
"       shr $16, %eax           \n"
"       movb %al, %cs:smp_gdt+12 \n"    /* Set base address of 32-bit code segment (bits 16:23) */
"       movb %al, %cs:smp_gdt+20 \n"    /* Set base address of 32-bit data segment (bits 16:23) */
"       ADDR32 DATA32 lgdt %cs:smp_gdt_sel    \n"     /* Load gdt */
"                               \n"
"       movw $0x3f8,%dx         \n"
"       movb $'x',%al           \n"
"       outb %al,%dx            \n"
"                               \n"
"       movl %cr0,%eax          \n"     /* Enter protected mode */
"       orb $1,%al              \n"
"       movl %eax,%cr0          \n"
"                               \n"
"       ljmp $0x8,$2f   /* Please note that the %cs segment selector has to have it's base address properly adjusted */ \n"
"                               \n"
"       .code32                 \n"
"2:     movw $0x10,%ax          \n"     /* Setup the 32-bit data selectors */
"       movw %ax,%ds            \n"
"       movw %ax,%es            \n"
"       movw %ax,%ss            \n"
"       movl $4096-4,%esp       \n"     /* Stack at the top of this 4K page */
"                               \n"
"       jmp smp_c_trampoline    \n");

/*
 * This tiny procedure sets the complete 64-bit environment up - it loads the descriptors,
 * enables 64-bit mode, loads MMU tables and trhough paging it activates the 64-bit long mode.
 *
 * After that it is perfectly safe to jump into the pure 64-bit kernel.
 */
static void leave_32bit_mode()
{
    unsigned int v1, v2, v3, v4;
    asm volatile ("lgdt %0"::"m"(smp_gdt64_sel));

    asm volatile ("outb %b0,%w1"::"a"('b'),"Nd"(0x3f8));

    /* Enable PAE */
    wrcr(cr4, _CR4_PAE | _CR4_PGE);

    asm volatile ("outb %b0,%w1"::"a"('c'),"Nd"(0x3f8));

    /* enable pages */
    wrcr(cr3, smp_mmu);

    asm volatile ("outb %b0,%w1"::"a"('d'),"Nd"(0x3f8));

    /* enable long mode */
    rdmsr(EFER, &v1, &v2);
    v1 |= _EFER_LME;
    wrmsr(EFER, v1, v2);

    asm volatile ("outb %b0,%w1"::"a"('e'),"Nd"(0x3f8));

    /* enable paging and activate long mode */
    wrcr(cr0, _CR0_PG | _CR0_PE);
}
static void __attribute__((used, noreturn)) smp_c_trampoline()
{
    asm volatile ("outb %b0,%w1"::"a"('a'),"Nd"(0x3f8));
    leave_32bit_mode();

    asm volatile ("outb %b0,%w1"::"a"('f'),"Nd"(0x3f8));
    
    asm volatile("ljmp *%0"::"m"(smp_kernel_target),"D"(smp_arg1),"S"(smp_arg2),"d"(smp_arg3),"c"(smp_arg4));
    while(1);
}

asm (".code64\n"
"boot64:        \n"
"       mov $0x10,%ax   \n"
"       mov %ax,%ds     \n"
"       mov %ax,%es     \n"
"       mov %ax,%ss     \n"
"1:       hlt; jmp 1b       \n"
"       .code32\n"
);
