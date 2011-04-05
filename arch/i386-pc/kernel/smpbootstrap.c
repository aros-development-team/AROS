/*
 * smpbootstrap.c
 *
 *  Created on: Nov 19, 2009
 *      Author: misc
 */

/* Only bad boys use copy&paste of own code... */

extern unsigned long smp_mmu, smp_arg1, smp_arg2, smp_arg3, smp_arg4, smp_baseaddr, smp_sp, smp_ip;

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
"smp_sp:        .long 0			\n"     /* 0x0010 */
"smp_ip:        .long do_jump+1	\n"     /* 0x0014 */
"smp_baseaddr:  .long 0         \n"
"smp_gdt:       .short 0; .short 0; .short 0; .short 0 \n"
"               .short 0x1000; .short 0x0000; .short 0x9a00; .short 0x0040 \n" /* 0x08: small CS selector */
"               .short 0x1000; .short 0x0000; .short 0x9200; .short 0x0040 \n" /* 0x10: small DS selector */
"				.short 0xffff; .short 0x0000; .short 0x9a00; .short 0x00cf \n" /* 0x18: 4GB CS selector */
"				.short 0xffff; .short 0x0000; .short 0x9200; .short 0x00cf \n" /* 0x20: 4GB DS selector */
"smp_gdt_sel:   .short 39       \n"
"               .long 0         \n"
"                               \n"
"1:                             \n"
"       mov %cs,%ax             \n"     /* Find out where the code resides */
"       shl $4, %eax            \n"
"       movl %eax,%cs:smp_baseaddr \n"  /* Store base address of the trampoline */
"       leal smp_gdt(%eax), %ebx \n"    /* Load physical address of 32-bit gdt */
"       movl %ebx, %cs:smp_gdt_sel+2\n" /* Set up 32-bit gdt address */
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

static void __attribute__((used, noreturn)) smp_c_trampoline()
{
    asm volatile("mov %1, %%ds; mov %1, %%es; movl %0,%%esp; \ndo_jump: ljmp $0x18,$0"::"r"(smp_sp), "r"(0x20), "a"(smp_arg1),"d"(smp_arg2),"c"(smp_arg3));
    while(1);
}
