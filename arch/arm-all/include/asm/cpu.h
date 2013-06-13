/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: cpu.h
    Lang: english
*/

#ifndef ASM_ARM_CPU_H
#define ASM_ARM_CPU_H

#define vfpreg(_vfp_) #_vfp_

#define fmrx(_vfp_) ({                  \
        unsigned long __v;                        \
        asm("mrc p10, 7, %0, " vfpreg(_vfp_) ", cr0, 0 @ fmrx   %0, " #_vfp_    \
            : "=r" (__v) : : "cc");     \
        __v;                            \
 })

#define fmxr(_vfp_,_var_)               \
        asm("mcr p10, 7, %0, " vfpreg(_vfp_) ", cr0, 0 @ fmxr   " #_vfp_ ", %0" \
           : : "r" (_var_) : "cc")

#endif /* ASM_ARM_CPU_H */
