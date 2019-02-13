/* **********************************************************
 * Copyright (C) 1998-2001 VMware, Inc.
 * All Rights Reserved
 * Id: svga_limits.h,v 1.8 2001/01/26 23:32:15 yoel Exp $
 * **********************************************************/

/*
 * svga_reg.h --
 *
 * SVGA limits
 */

#ifndef _SVGA_LIMITS_H_
#define _SVGA_LIMITS_H_

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_MONITOR
//#include "includeCheck.h"

/*
 * Location and size of SVGA frame buffer.
 */
#define SVGA_FB_MAX_SIZE       (64*1024*1024)
#define SVGA_MEM_SIZE          (256*1024)

/*
 * SVGA_FB_START is the default starting address of the SVGA frame
 * buffer in the guest's physical address space.
 * SVGA_FB_START_BIGMEM is the starting address of the SVGA frame
 * buffer for VMs that have a large amount of physical memory.
 *
 * The address of SVGA_FB_START is set to 2GB - (SVGA_FB_MAX_SIZE + SVGA_MEM_SIZE), 
 * thus the SVGA frame buffer sits at [SVGA_FB_START .. 2GB-1] in the
 * physical address space.  Our older SVGA drivers for NT treat the
 * address of the frame buffer as a signed integer.  For backwards
 * compatibility, we keep the default location of the frame buffer
 * at under 2GB in the address space.  This restricts VMs to have "only"
 * up to ~2031MB (i.e., up to SVGA_FB_START) of physical memory.
 *
 * For VMs that want more memory than the ~2031MB, we place the SVGA
 * frame buffer at SVGA_FB_START_BIGMEM.  This allows VMs to have up
 * to 3584MB, at least as far as the SVGA frame buffer is concerned
 * (note that there may be other issues that limit the VM memory
 * size).  PCI devices use high memory addresses, so we have to put
 * SVGA_FB_START_BIGMEM low enough so that it doesn't overlap with any
 * of these devices.  Placing SVGA_FB_START_BIGMEM at 0xE0000000
 * should leave plenty of room for the PCI devices.
 *
 * NOTE: All of that is only true for the 0710 chipset.  As of the 0405
 * chipset, the framebuffer start is determined solely based on the value
 * the guest BIOS or OS programs into the PCI base address registers.
 */
#define SVGA_FB_LEGACY_START		0x7EFC0000
#define SVGA_FB_LEGACY_START_BIGMEM	0xE0000000

#endif
