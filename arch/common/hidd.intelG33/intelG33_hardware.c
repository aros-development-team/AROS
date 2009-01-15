/*
    Copyright � 2009-2010, The AROS Development Team. All rights reserved.
    $Id: intelG33_hardware.c 29403 2008-09-05 19:13:26Z j.koivisto $

    Desc: intelG33_hardware.c
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <inttypes.h>

#include "intelG33_intern.h"
#include "intelG33_regs.h"

#define readb(addr) (*(volatile uint8_t *) (addr))
#define readw(addr) (*(volatile uint16_t *) (addr))
#define readl(addr) (*(volatile uint32_t *) (addr))

#define writeb(b,addr) ((*(volatile uint8_t *) (addr)) = (b))
#define writew(b,addr) ((*(volatile uint16_t *) (addr)) = (b))
#define writel(b,addr) ((*(volatile uint32_t *) (addr)) = (b))

#define G33_RD_REGL(a, reg) (readl((a) + reg ))
#define G33_RD_REGW(a, reg) (readw((a) + reg))
#define G33_RD_REGB(a, reg) (readb((a) + reg))
#define G33_RD_REG_ARRAY(a, reg, offset) (readl(((a) + reg) + ((offset) << 2)))

#define G33_WR_REGL(a, reg, value) (writel((value), ((a) + reg)))
#define G33_WR_REGW(a, reg, value) (writew((value), ((a) + reg)))
#define G33_WR_REGB(a, reg, value) (writeb((value), ((a) + reg)))
#define G33_WR_REG_ARRAY(a, reg, offset, value) (writel((value), (((a) + reg) + ((offset) << 2))))


