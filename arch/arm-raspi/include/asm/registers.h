/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    These need refining if IO-base is moved to virtual memory, now they reflect the state of Arm physical address space

*/

#ifndef REGISTERS_RASPI_H
#define REGISTERS_RASPI_H

/* This stuff should really be in arm/bcm2835.h */
#define VIRTIO_BASE         0x20000000                  // Peripheral base address        
#define GPIO_PADS           (VIRTIO_BASE + 0x100000)
#define CLOCK_BASE          (VIRTIO_BASE + 0x101000)
#define GPIO_BASE           (VIRTIO_BASE + 0x200000)
#define UART0_BASE          (VIRTIO_BASE + 0x201000)
#define SPI0_BASE           (VIRTIO_BASE + 0x204000)
#define BSC0_BASE           (VIRTIO_BASE + 0x205000)
#define GPIO_PWM            (VIRTIO_BASE + 0x20C000)

#define GPIO_PADS_0_27      0x002c
#define GPIO_PADS_28_45     0x0030
#define GPIO_PADS_46_53     0x0034 

#define GPFSEL0             (GPIO_BASE + 0x0)           // GPIO Function Select 0
#define GPFSEL1             (GPIO_BASE + 0x4)           // GPIO Function Select 1
#define GPFSEL2             (GPIO_BASE + 0x8)           // GPIO Function Select 2
#define GPFSEL3             (GPIO_BASE + 0xC)           // GPIO Function Select 3
#define GPFSEL4             (GPIO_BASE + 0x10)          // GPIO Function Select 4
#define GPFSEL5             (GPIO_BASE + 0x14)          // GPIO Function Select 5
#define GPSET0              (GPIO_BASE + 0x1C)          // GPIO Pin Output Set 0
#define GPSET1              (GPIO_BASE + 0x20)          // GPIO Pin Output Set 1
#define GPCLR0              (GPIO_BASE + 0x28)          // GPIO Pin Output Clear 0
#define GPCLR1              (GPIO_BASE + 0x2C)          // GPIO Pin Output Clear 1
#define GPLEV0              (GPIO_BASE + 0x34)          // GPIO Pin Level 0
#define GPLEV1              (GPIO_BASE + 0x38)          // GPIO Pin Level 1
#define GPEDS0              (GPIO_BASE + 0x40)          // GPIO Pin Event Detect Status 0
#define GPEDS1              (GPIO_BASE + 0x44)          // GPIO Pin Event Detect Status 1
#define GPREN0              (GPIO_BASE + 0x4C)          // GPIO Pin Rising Edge Detect Enable 0
#define GPREN1              (GPIO_BASE + 0x50)          // GPIO Pin Rising Edge Detect Enable 1
#define GPFEN0              (GPIO_BASE + 0x58)          // GPIO Pin Falling Edge Detect Enable 0
#define GPFEN1              (GPIO_BASE + 0x5C)          // GPIO Pin Falling Edge Detect Enable 1
#define GPHEN0              (GPIO_BASE + 0x64)
#define GPHEN1              (GPIO_BASE + 0x68)
#define GPLEN0              (GPIO_BASE + 0x70)
#define GPLEN1              (GPIO_BASE + 0x74)
#define GPAREN0             (GPIO_BASE + 0x7c)
#define GPAREN1             (GPIO_BASE + 0x80)
#define GPAFEN0             (GPIO_BASE + 0x88)
#define GPAFEN1             (GPIO_BASE + 0x8c)
#define GPPUD               (GPIO_BASE + 0x94)
#define GPPUDCLK0           (GPIO_BASE + 0x98)
#define GPPUDCLK1           (GPIO_BASE + 0x9c)

#define UART_DR                 (0x00)
#define UART_RSRECR             (0x04)
#define UART_FR                 (0x18)
#define UART_ILPR               (0x20)
#define UART_IBRD               (0x24)
#define UART_FBRD               (0x28)
#define UART_LCRH               (0x2C)
#define UART_CR                 (0x30)
#define UART_IFLS               (0x34)
#define UART_IMSC               (0x38)
#define UART_RIS                (0x3C)
#define UART_MIS                (0x40)
#define UART_ICR                (0x44)
#define UART_DMACR              (0x48)
#define UART_ITCR               (0x80)
#define UART_ITIP               (0x84)
#define UART_ITOP               (0x88)
#define UART_TDR                (0x8C)

#if (0)
#define ONEMS	              (0xb0/4)
#define UBIR	              (0xa4/4)
#define UBMR	              (0xa8/4)
#define UCR2	              (0x84/4)
#endif

#define SPI0_CS             (0x00)
#define SPI0_FIFO           (0x04)
#define SPI0_CLK            (0x08)
#define SPI0_DLEN           (0x0c)
#define SPI0_LTOH           (0x10)
#define SPI0_DC             (0x14)

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
