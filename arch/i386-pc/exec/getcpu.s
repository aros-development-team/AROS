#/*
#    Copyright © 2000, The AROS Development Team. All rights reserved.
#    $Id$
#
#    Desc: CPU detection routine
#    Lang: english
#*/

                .text

                .globl  cpu_params
                .globl  x86
                .globl  x86_vendor
                .globl  x86_model
                .globl  x86_mask
                .globl  x86_hard_math
                .globl  x86_cpuid
                .globl  x86_capability
                .globl  x86_vendor_id
                
                .set    cpu_params,     0x00000020
                .set    x86,            (cpu_params)
                .set    x86_vendor,     (cpu_params + 1)
                .set    x86_model,      (cpu_params + 2)
                .set    x86_mask,       (cpu_params + 3)
                .set    x86_hard_math,  (cpu_params + 6)
                .set    x86_cpuid,      (cpu_params + 8)
                .set    x86_capability, (cpu_params + 12)
                .set    x86_vendor_id,  (cpu_params + 16)
                
                .globl  exec_GetCPU
                .type   exec_GetCPU,@function

exec_GetCPU:    pushal
                movl    $-1,x86_cpuid   /* No cpuid allowed as far */

#/*
## Check whether it is i386 or i486. In i486 we chan change AC bit in EFLAGS
## register. We will use it.
#*/

                movl    $3,x86          /* We have at leas i386 if this code works */
                pushfl                  /* Get eflags */
                popl    %eax
                movl    %eax,%ecx
                xorl    $0x40000,%eax   /* Set AC flag. If it's possible, then we have 486 */
                pushl   %eax            /* Copy %eax to eflags and back to %eax */
                popfl
                pushfl
                popl    %eax
                xorl    %ecx,%eax       /* Mas everything but AC bit (if set) */
                andl    $0x40000,%eax
                je      exec_is386      /* Nope, AC was 0 - i386 only */

/* At this point it is sure that we have something better than i386. Assume for
## a while that it is i486. Check whether we can use cpuid instruction. If no,
## then we have i486. If cpuid may be used then get CPU type from this opcode.
## It can be i486 (last few models had cpuid implemented) or better */

                movl    $4,x86                      /* Update information */
                movl    %ecx,%eax                   /* we will try to change ID flag */
                xorl    $0x200000,%eax              /* if it is possible then cpuid */
                pushl   %eax                        /* is implemented */
                popfl
                pushfl
                popl    %eax
                xorl    %ecx,%eax
                pushl   %ecx                        /* Restore eflags register */
                popfl
                andl    $0x200000,%eax
                je      exec_is486                  /* Well, no cpuid. So we can leave now */

                xorl    %eax,%eax                   /* CPUID! */
                cpuid
                movl    %eax,x86_cpuid              /* The highest cpuid %eax value */
                movl    %ebx,x86_vendor_id          /* Processor's vendor ID */
                movl    %ecx,x86_vendor_id+4
                movl    %edx,x86_vendor_id+8

                orl     %eax,%eax                   /* Can we use more than that above? */
                je      exec_is486                  /* Nope, it was i486 */

                movl    $1,%eax                     /* Get CPU informations (model, stepping etc) */
                cpuid
                movb    %al,%cl
                andb    $0x0f,%ah
                movb    %ah,x86                     /* Model: 5 for 586, 6 for 686 and so on */
                andb    $0xf0,%al
                shrb    $4,%al
                movb    %al,x86_model
                andb    $0x0f,%cl
                movb    %cl,x86_mask
                movl    %edx,x86_capability         /* Capabilities like MMX, SSE and more */

                cmpb    $4,x86                      /* Updata AttnFlags in ExecBase */
                jbe     exec_is486
                cmpb    $5,x86
                jbe     exec_is486

exec_is486:     movl    %cr0,%eax                   /* Update CR0 register */
                andl    $0x80000011,%eax            /* Save PG,PE,ET */
                orl     $0x00050022,%eax            /* Set AM,WP,NE and MP */
                jmp     2f
                
exec_is386:     pushl   %ecx
                popfl
                movl    %cr0,%eax                   /* Update CR0 reg. i386 version */
                andl    $0x80000011,%eax            /* Save PG,PE,ET */
                orl     $2,%eax                     /* Set MP */
2:              movl    %eax,%cr0
                call    exec_GetFPU                 /* Detect whether we have FPU */
                xorl    %eax,%eax
                lldt    %ax                         /* Invalidate LDT */
                cld                                 /* Clear D flag as needed by AROS and gcc */
                popal
                ret

#/*
## Check, whether FPU is present. As we don't need to distinguish FPU type,
## we will just see if it is present.
#*/

                .type   exec_GetFPU,@function
exec_GetFPU:    movb    $0,x86_hard_math
                clts
                fninit
                fstsw   %ax
                cmpb    $0,%al
                je      1f
                movl    %cr0,%eax
                xorl    $4,%eax
                movl    %eax,%cr0
                ret
1:              movb    $1,x86_hard_math
                .byte   0xdb,0xe4
                ret

