/*      $NetBSD: empbreg.h,v 1.1 2012/05/30 18:01:51 rkujawa Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Elbox Mediator registers. This information was obtained using reverse 
 * engineering methods by Frank Wille and Radoslaw Kujawa (without access to
 * official documentation). 
 */

#ifndef _AMIGA_EMPBREG_H_

/* Zorro IDs */
#define ZORRO_MANID_ELBOX       2206    
#define ZORRO_PRODID_MEDZIII    30      /* Z-III        DMA    4 IRQ */
#define ZORRO_PRODID_MEDZIV     31      /* Z-IV         Bounce 4 IRQ */
#define ZORRO_PRODID_MED1K2     32      /* 1200         Bounce */
#define ZORRO_PRODID_MED4K      33      /* 3000D/4000D  DMA    4 IRQ */
#define ZORRO_PRODID_MED1K2SX   40      /* 1200SX       DMA    4 IRQ */
#define ZORRO_PRODID_MED1K2LT2  48      /* 1200LTx2     DMA    4 IRQ */
#define ZORRO_PRODID_MED1K2LT4  49      /* 1200LTx4     DMA    4 IRQ */
#define ZORRO_PRODID_MED1K2TX   60      /* 1200TX       DMA    4 IRQ */
#define ZORRO_PRODID_MED4KMKII  63      /* 4000 MKII    DMA    4 IRQ */

#define ZORRO_PRODID_MEDZIV_MEM         ZORRO_PRODID_MEDZIV+0x80
#define ZORRO_PRODID_MED1K2_MEM         ZORRO_PRODID_MED1K2+0x80
#define ZORRO_PRODID_MED4K_MEM          ZORRO_PRODID_MED4K+0x80
#define ZORRO_PRODID_MED1K2SX_MEM       ZORRO_PRODID_MED1K2SX+0x80
#define ZORRO_PRODID_MED1K2LT2_MEM      ZORRO_PRODID_MED1K2LT2+0x80
#define ZORRO_PRODID_MED1K2LT4_MEM      ZORRO_PRODID_MED1K2LT4+0x80
#define ZORRO_PRODID_MED1K2TX_MEM       ZORRO_PRODID_MED1K2TX+0x80
#define ZORRO_PRODID_MED4KMKII_MEM      ZORRO_PRODID_MED4KMKII+0x80

/*
 * Mediator 1200 consists of two boards. First board lives in Z2 I/O
 * space and is internally divided into two 64kB spaces. Second board, used
 * as a window into PCI memory space is configured somewhere within 24-bit Fast
 * RAM space (its size depends on a WINDOW jumper setting).
 */
#define EMPB_SETUP_OFF          0x00000000
#define EMPB_SETUP_SIZE         0xFFFF

#define EMPB_SETUP_CONFIG_OFF   0x0     /* Config bits */
#define EMPB_SETUP_STATUS_OFF   0x4     /* Status bits */
#define EMPB_SETUP_WINDOW_OFF   0x2     /* set memory window position */
#define EMPB_SETUP_BRIDGE_OFF   0x7     /* select between conf or I/O */

#define EMPB_BRIDGE_CONF        0xA0    /* switch into configuration space */
#define EMPB_BRIDGE_IO          0x20    /* switch into I/O space */

#define EMPB_BRIDGE_OFF         0x00010000
#define EMPB_BRIDGE_SIZE        0xFFFF

#define EMPB_CONF_DEV_STRIDE    0x800   /* offset between PCI devices */
#define EMPB_CONF_FUNC_STRIDE   0x100   /* XXX: offset between PCI funcs */ 

/* All PCI interrupt lines are wired to INT2? */
#define EMPB_INT                2       // XXX: wild guess

/*
 * Mediator 4000 consists of two boards. First board lives in Z3 I/O
 * space and is internally divided into four 4M spaces. Second board, used
 * as a window into PCI memory space is configured somewhere within 24-bit Fast
 * RAM space (its size depends on a WINDOW jumper setting).
 */
#define EMZ4_SETUP_OFF          0x00000000
#define EMZ4_SETUP_SIZE         0xFFFF

#define EMZ4_SETUP_CONFIG_OFF   0x0     /* Config bits */
#define EMZ4_SETUP_STATUS_OFF   0x4     /* Status bits */

#define EMZ4_IOPORT_OFF         0x00c06000
#define EMZ4_IOPORT_SIZE        0x00000FFF

#define EMZ4_CONFIG_OFF         0x00800000
#define EMZ4_CONFIG_SIZE        0x003FFFFF

#define EMZ4_CONF_BUS_STRIDE    0x10000 /* offset between PCI busses */
#define EMZ4_CONF_DEV_STRIDE    0x800   /* offset between PCI devices */
#define EMZ4_CONF_FUNC_STRIDE   0x100   /* XXX: offset between PCI funcs */ 

#define EMZ4_BUS_MAX    64

/* All PCI interrupt lines are wired to INT2? */
#define EMZ4_INT        2       // XXX: wild guess


#endif /* _AMIGA_EMPBREG_H_ */
