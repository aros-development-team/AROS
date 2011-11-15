#ifndef _BOOTSTRAP_H
#define _BOOTSTRAP_H

/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/multiboot.h>

#ifdef DEBUG
#define D(x)    x
#else
#define D(x)    /* eps */
#endif

#ifdef MULTIBOOT_64BIT

/*
 * Our kickstart is 64-bit but we are being compiled in 32-bit mode.
 * So we need a correct definition of TagItem
 */
struct TagItem64
{
    unsigned long long ti_Tag;
    unsigned long long ti_Data;
};

#else

#define TagItem64 TagItem

#endif

/* A pointer used for building boot taglist */
extern struct TagItem64 *tag;
extern struct vbe_mode VBEModeInfo;
extern struct vbe_controller VBEControllerInfo;

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

extern char _end;

#define TOP_ADDR(a1, a2) ((unsigned long)a2 > a1 ? (unsigned long)a2 : a1)
#define STR_TOP_ADDR(a1, s) TOP_ADDR(a1, s + strlen(s) + 1)

void setup_mmu(void);
void kick(void *kick_base, struct TagItem64 *km);

unsigned long AddModule(unsigned long mod_start, unsigned long mod_end, unsigned long end);
void AllocFB(void);
void Hello(void);
int ParseCmdLine(const char *cmdline);
struct mb_mmap *mmap_make(unsigned long *len, unsigned long mem_lower, unsigned long long mem_upper);
void panic(const char *str);
unsigned long mb1_parse(struct multiboot *mb, struct mb_mmap **mmap_addr, unsigned long *mmap_len);
unsigned long mb2_parse(void *mb, struct mb_mmap **mmap_addr, unsigned long *mmap_len);
void setupVESA(char *str);

#if defined(__i386__) || defined(__x86_64__)
    #define LONG2BE(v)  ({ unsigned int __v32; asm volatile("bswap %0":"=a"(__v32):"0"((v))); __v32; })
#else
    #define LONG2BE(v)  (v)
#endif

#endif // _BOOTSTRAP_H
