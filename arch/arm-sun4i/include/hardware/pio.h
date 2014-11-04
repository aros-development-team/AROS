/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i port io module
    Lang: english
*/

#ifndef HARDWARE_SUN4I_PIOH
#define HARDWARE_SUN4I_PIOH

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4i_PIO_BASE          0x01C20800

enum { PA = 0, PB, PC, PD, PE, PF, PG, PH, PI };

#define PIO_CFG0_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+0))))
#define PIO_CFG1_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+4))))
#define PIO_CFG2_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+8))))

#define PIO_CFG3_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+12))))
#define PIO_DATA_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+16))))
#define PIO_DRV0_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+20))))
#define PIO_DRV1_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+24))))
#define PIO_PUL0_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+28))))
#define PIO_PUL1_REG(n) *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(n*36+32))))

#define PIO_INT_CFG0_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x200))))
#define PIO_INT_CFG1_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x204))))
#define PIO_INT_CFG2_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x208))))
#define PIO_INT_CFG3_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x20c))))
#define PIO_INT_CTL_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x210))))
#define PIO_INT_STA_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x214))))
#define PIO_INT_DEB_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x218))))

#define SDR_PAD_DRV_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x220))))
#define SDR_PAD_PIL_REG *((volatile uint32_t *) ((uint32_t *) (SUN4i_PIO_BASE+(0x224))))

#endif
