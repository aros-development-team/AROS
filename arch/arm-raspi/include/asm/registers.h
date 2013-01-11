/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    These need refining if IO-base is moved to virtual memory, now they reflect the state of Arm physical address space

*/

#ifndef REGISTERS_RASPI_H
#define REGISTERS_RASPI_H

#define GPFSEL0             0x20200000  // GPIO Function Select 0
#define GPFSEL1             0x20200004  // GPIO Function Select 1
#define GPFSEL2             0x20200008  // GPIO Function Select 2
#define GPFSEL3             0x2020000C  // GPIO Function Select 3
#define GPFSEL4             0x20200010  // GPIO Function Select 4
#define GPFSEL5             0x20200014  // GPIO Function Select 5
#define GPSET0              0x2020001C  // GPIO Pin Output Set 0
#define GPSET1              0x20200020  // GPIO Pin Output Set 1
#define GPCLR0              0x20200028  // GPIO Pin Output Clear 0
#define GPCLR1              0x2020002C  // GPIO Pin Output Clear 1
#define GPLEV0              0x20200034  // GPIO Pin Level 0
#define GPLEV1              0x20200038  // GPIO Pin Level 1
#define GPEDS0              0x20200040  // GPIO Pin Event Detect Status 0
#define GPEDS1              0x20200044  // GPIO Pin Event Detect Status 1
#define GPREN0              0x2020004C  // GPIO Pin Rising Edge Detect Enable 0
#define GPREN1              0x20200050  // GPIO Pin Rising Edge Detect Enable 1
#define GPFEN0              0x20200058  // GPIO Pin Falling Edge Detect Enable 0
#define GPFEN1              0x2020005C  // GPIO Pin Falling Edge Detect Enable 1

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
#define AUX_SPI1_STAT_REG   0x202150C8  // SPI 2 Status
#define AUX_SPI1_IO_REG     0x202150D0  // SPI 2 Data
#define AUX_SPI1_PEEK_REG   0x202150D4  // SPI 2 Peek

#endif
