#ifndef _BOOTSTRAP_H
#define _BOOTSTRAP_H

/*
    Copyright (C) 2006 The AROS Development Team. All rights reserved.
    $Id:$
*/

/*
 * The target base address of 64-bit kernel
 */
#define KERNEL_TARGET_ADDRESS   0x01000000

/*
 * Multiboot stuff
 */
typedef struct {
    unsigned int   magic;
    unsigned int   flags;
    unsigned int   chksum;
} multiboot_header;

#define MB_MAGIC    0x1BADB002
#define MB_FLAGS    0x00000003

struct module {
    const char *name;
    void *address;
};

void *_binary_aros_o_start;

#if defined(__i386__) || defined(__x86_64__)
    #define LONG2BE(v)  ({ unsigned int __v32; asm volatile("bswap %0":"=a"(__v32):"0"((v))); __v32; })
#else
    #define LONG2BE(v)  (v)
#endif

#endif // _BOOTSTRAP_H

