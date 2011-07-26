#ifndef _BOOTSTRAP_H
#define _BOOTSTRAP_H

/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 
struct KernelMessage {
    struct { void *low,*high; } GRUBData;
    
    struct { void *low,*high; } kernelBase;
    struct { void *low,*high; } kernelLowest;
    struct { void *low,*high; } kernelHighest;  
    struct { void *low,*high; } kernelBSS;     
    struct { void *low,*high; } GDT;
    struct { void *low,*high; } IDT;
    struct { void *low,*high; } PL4;  
    
    struct { void *low,*high; } vbeModeInfo;
    struct { void *low,*high; } vbeControllerInfo;
    
};
*/

#ifdef DEBUG
#define D(x)    x
#else
#define D(x)    /* eps */
#endif

/*
 * Our kickstart is 64-bit but we are being compiled in 32-bit mode.
 * So we need a correct definition of TagItem
 */
struct TagItem64
{
    unsigned long long ti_Tag;
    unsigned long long ti_Data;
};

/* A pointer used for building boot taglist */
extern struct TagItem64 *tag;

/* The target base address of 64-bit kernel */
#define KERNEL_TARGET_ADDRESS   0x01000000

/* Our kickstart will operate with page size = 4KB */
#define PAGE_SIZE 4096

//#define KERNEL_HIGH_OFFSET      0x1ffULL
//#define KERNEL_HIGH_OFFSET      0x1ffULL
//#define KERNEL_HIGH_OFFSET      31ULL
//#define KERNEL_OFFSET           0x01000000
#define KERNEL_OFFSET           0
//#define KERNEL_OFFSET           0xfffffffff8000000ULL
//#define KERNEL_OFFSET           0x0000000000000000ULL

//extern void *_binary_aros_o_start;
extern void *_binary_vesa_start;
extern unsigned long _binary_vesa_size;

void setup_mmu(void);
void kick(void *kick_base, struct TagItem64 *km);

#if defined(__i386__) || defined(__x86_64__)
    #define LONG2BE(v)  ({ unsigned int __v32; asm volatile("bswap %0":"=a"(__v32):"0"((v))); __v32; })
#else
    #define LONG2BE(v)  (v)
#endif

#endif // _BOOTSTRAP_H

