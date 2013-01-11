/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef REGISTERS_H
#define REGISTERS_H

#ifdef __PILOT_CODE__
#define WREG_L(addr)	*(Long *)(addr)
#define RREG_L(addr)	*(Long *)(addr)
#define WREG_W(addr)	*(Word *)(addr)
#define RREG_W(addr)	*(Word *)(addr)
#define WREG_B(addr)	*(Byte *)(addr)
#define RREG_B(addr)	*(Byte *)(addr)
#else
#include <exec/types.h>
#define WREG_L(addr)	*(ULONG *)(addr)
#define RREG_L(addr)	*(ULONG *)(addr)
#define WREG_W(addr)	*(UWORD *)(addr)
#define RREG_W(addr)	*(UWORD *)(addr)
#define WREG_B(addr)	*(UBYTE *)(addr)
#define RREG_B(addr)	*(UBYTE *)(addr)
#endif

#define AUX_IRQ             0x20215000  // Auxiliary Interrupt status
#define AUX_ENABLES         0x20215004  // Auxiliary enables
#define AUX_MU_IO_REG       0x20215040  // AUX_MU_IO_REG Mini Uart I/O Data
#define AUX_MU_IER_REG      0x20215044  // Mini Uart Interrupt Enable
#define AUX_MU_IIR_REG      0x20215048  // Mini Uart Interrupt Identify
#define AUX_MU_LCR_REG      0x2021504C  // Mini Uart Line Control
#define AUX_MU_MCR_REG      0x20215050  // Mini Uart Modem Control
#define AUX_MU_LSR_REG      0x20215054  // Mini Uart Line Status
#define AUX_MU_MSR_REG      0x20215058  // Mini Uart Modem Status
#define AUX_MU_SCRATCH      0x2021505C  // Mini Uart Scratch
#define AUX_MU_CNTL_REG     0x20215060  // Mini Uart Extra Control
#define AUX_MU_STAT_REG     0x20215064  // Mini Uart Extra Status
#define AUX_MU_BAUD_REG     0x20215068  // Mini Uart Baudrate
#define AUX_SPI0_CNTL0_REG  0x20215080  // SPI 1 Control register 0
#define AUX_SPI0_CNTL1_REG  0x20215084  // SPI 1 Control register 1
#define AUX_SPI0_STAT_REG   0x20215088  // SPI 1 Status
#define AUX_SPI0_IO_REG     0x20215090  // SPI 1 Data
#define AUX_SPI0_PEEK_REG   0x20215094  // SPI 1 Peek
#define AUX_SPI1_CNTL0_REG  0x202150C0  // SPI 2 Control register 0
#define AUX_SPI1_CNTL1_REG  0x202150C4  // SPI 2 Control register 1
#define AUX_SPI1_STAT_REG   0x7E2150C8  // SPI 2 Status
#define AUX_SPI1_IO_REG     0x7E2150D0  // SPI 2 Data
#define AUX_SPI1_PEEK_REG   0x7E2150D4  // SPI 2 Peek

#endif
