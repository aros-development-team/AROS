/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef BCM2835_H
#define BCM2835_H

#define CLID_I2C_BCM2835        "hidd.i2c.bcm2835"

#define BCM2835_PERIPHYSBASE    0x20000000                      // Peripheral physical base address
#define BCM2835_PERIPHYSSIZE    0x400000
#define BCM2835_PERIBUSBASE     0x7E000000

#define ARM_PERIIOBASE          BCM2835_PERIPHYSBASE
#define BCM_BUSBASE             BCM2835_PERIBUSBASE
#define ARM_PERIIOSIZE          BCM2835_PERIPHYSSIZE

#if (1)
// TODO: Move to a more generic ARM header..
#define ARM_STACK_DEF           4096
#define ARM_VIRTBASE            0xF0000000
#define ARM_PRIMECELLID         0xB105F00D
#define ARM_PRIMECELLPERISIZE   0x1000

#define STACK_SIZE              ARM_STACK_DEF

// TODO: Everything after this point needs to be moved to ARM/Peripheral specific headers ..
#define SYSTIMER_BASE           (ARM_PERIIOBASE + 0x003000)
#define ARMTIMER_BASE           (ARM_PERIIOBASE + 0x00b000)
#define IRQ_BASE                (ARM_PERIIOBASE + 0x00b200)
#define GPIO_PADS               (ARM_PERIIOBASE + 0x100000)
#define CLOCK_BASE              (ARM_PERIIOBASE + 0x101000)
#define GPIO_BASE               (ARM_PERIIOBASE + 0x200000)

#define SPI0_BASE               (ARM_PERIIOBASE + 0x204000)
#define BSC0_BASE               (ARM_PERIIOBASE + 0x205000)
#define GPIO_PWM                (ARM_PERIIOBASE + 0x20C000)

#define SYSTIMER_CS             (SYSTIMER_BASE + 0x00)
#define SYSTIMER_CLO            (SYSTIMER_BASE + 0x04)
#define SYSTIMER_CHI            (SYSTIMER_BASE + 0x08)
#define SYSTIMER_C0             (SYSTIMER_BASE + 0x0c)
#define SYSTIMER_C1             (SYSTIMER_BASE + 0x10)
#define SYSTIMER_C2             (SYSTIMER_BASE + 0x14)
#define SYSTIMER_C3             (SYSTIMER_BASE + 0x18)

#define SYSTIMER_M0             (1 << 0)
#define SYSTIMER_M1             (1 << 1)
#define SYSTIMER_M2             (1 << 2)
#define SYSTIMER_M3             (1 << 3)

#define ARMTIMER_LOAD           (ARMTIMER_BASE + 0x400)
#define ARMTIMER_VALUE          (ARMTIMER_BASE + 0x404)
#define ARMTIMER_CONTROL        (ARMTIMER_BASE + 0x408)
#define ARMTIMER_IRQ_ACK        (ARMTIMER_BASE + 0x40c)
#define ARMTIMER_IRQ_RAW        (ARMTIMER_BASE + 0x410)
#define ARMTIMER_IRQ_MSK        (ARMTIMER_BASE + 0x414)
#define ARMTIMER_RELOAD         (ARMTIMER_BASE + 0x418)
#define ARMTIMER_PREDIV         (ARMTIMER_BASE + 0x41c)
#define ARMTIMER_FRC            (ARMTIMER_BASE + 0x420)

#define ARMIRQ_PEND             (IRQ_BASE + 0x00)
#define GPUIRQ_PEND0            (IRQ_BASE + 0x04)               // Pending IRQs
#define GPUIRQ_PEND1            (IRQ_BASE + 0x08)
#define GPUIRQ_ENBL0            (IRQ_BASE + 0x10)               // IRQ enable bits
#define GPUIRQ_ENBL1            (IRQ_BASE + 0x14)
#define ARMIRQ_ENBL             (IRQ_BASE + 0x18)
#define GPUIRQ_DIBL0            (IRQ_BASE + 0x1C)               // IRQ disable bits
#define GPUIRQ_DIBL1            (IRQ_BASE + 0x20)
#define ARMIRQ_DIBL             (IRQ_BASE + 0x24)

#define IRQ_MASK(irq)           (1 << (irq & 0x1f))
#define IRQ_BANK(irq)           (irq >> 5)

#define GPUIRQ0_BASE            (0 << 5)
#define IRQ_TIMER0              (GPUIRQ0_BASE + 0)
#define IRQ_TIMER1              (GPUIRQ0_BASE + 1)
#define IRQ_TIMER2              (GPUIRQ0_BASE + 2)
#define IRQ_TIMER3              (GPUIRQ0_BASE + 3)
#define IRQ_CODEC0              (GPUIRQ0_BASE + 4)
#define IRQ_CODEC1              (GPUIRQ0_BASE + 5)
#define IRQ_CODEC2              (GPUIRQ0_BASE + 6)
#define IRQ_VC_JPEG             (GPUIRQ0_BASE + 7)
#define IRQ_ISP                 (GPUIRQ0_BASE + 8)
#define IRQ_VC_USB              (GPUIRQ0_BASE + 9)
#define IRQ_VC_3D               (GPUIRQ0_BASE + 10)
#define IRQ_TRANSPOSER          (GPUIRQ0_BASE + 11)
#define IRQ_MULTICORESYNC0      (GPUIRQ0_BASE + 12)
#define IRQ_MULTICORESYNC1      (GPUIRQ0_BASE + 13)
#define IRQ_MULTICORESYNC2      (GPUIRQ0_BASE + 14)
#define IRQ_MULTICORESYNC3      (GPUIRQ0_BASE + 15)
#define IRQ_DMA0                (GPUIRQ0_BASE + 16)
#define IRQ_DMA1                (GPUIRQ0_BASE + 17)
#define IRQ_VC_DMA2             (GPUIRQ0_BASE + 18)
#define IRQ_VC_DMA3             (GPUIRQ0_BASE + 19)
#define IRQ_DMA4                (GPUIRQ0_BASE + 20)
#define IRQ_DMA5                (GPUIRQ0_BASE + 21)
#define IRQ_DMA6                (GPUIRQ0_BASE + 22)
#define IRQ_DMA7                (GPUIRQ0_BASE + 23)
#define IRQ_DMA8                (GPUIRQ0_BASE + 24)
#define IRQ_DMA9                (GPUIRQ0_BASE + 25)
#define IRQ_DMA10               (GPUIRQ0_BASE + 26)
#define IRQ_DMA11               (GPUIRQ0_BASE + 27)
#define IRQ_DMA12               (GPUIRQ0_BASE + 28)
#define IRQ_AUX                 (GPUIRQ0_BASE + 29)
#define IRQ_ARM                 (GPUIRQ0_BASE + 30)
#define IRQ_VPUDMA              (GPUIRQ0_BASE + 31)

#define GPUIRQ1_BASE            (1 << 5)
#define IRQ_HOSTPORT            (GPUIRQ1_BASE + 0)
#define IRQ_VIDEOSCALER         (GPUIRQ1_BASE + 1)
#define IRQ_CCP2TX              (GPUIRQ1_BASE + 2)
#define IRQ_SDC                 (GPUIRQ1_BASE + 3)
#define IRQ_DSI0                (GPUIRQ1_BASE + 4)
#define IRQ_AVE                 (GPUIRQ1_BASE + 5)
#define IRQ_CAM0                (GPUIRQ1_BASE + 6)
#define IRQ_CAM1                (GPUIRQ1_BASE + 7)
#define IRQ_HDMI0               (GPUIRQ1_BASE + 8)
#define IRQ_HDMI1               (GPUIRQ1_BASE + 9)
#define IRQ_PIXELVALVE1         (GPUIRQ1_BASE + 10)
#define IRQ_I2CSPISLV           (GPUIRQ1_BASE + 11)
#define IRQ_DSI1                (GPUIRQ1_BASE + 12)
#define IRQ_PWA0                (GPUIRQ1_BASE + 13)
#define IRQ_PWA1                (GPUIRQ1_BASE + 14)
#define IRQ_CPR                 (GPUIRQ1_BASE + 15)
#define IRQ_SMI                 (GPUIRQ1_BASE + 16)
#define IRQ_GPIO0               (GPUIRQ1_BASE + 17)
#define IRQ_GPIO1               (GPUIRQ1_BASE + 18)
#define IRQ_GPIO2               (GPUIRQ1_BASE + 19)
#define IRQ_GPIO3               (GPUIRQ1_BASE + 20)
#define IRQ_VC_I2C              (GPUIRQ1_BASE + 21)
#define IRQ_VC_SPI              (GPUIRQ1_BASE + 22)
#define IRQ_VC_I2SPCM           (GPUIRQ1_BASE + 23)
#define IRQ_VC_SDIO             (GPUIRQ1_BASE + 24)
#define IRQ_VC_UART             (GPUIRQ1_BASE + 25)
#define IRQ_SLIMBUS             (GPUIRQ1_BASE + 26)
#define IRQ_VEC                 (GPUIRQ1_BASE + 27)
#define IRQ_CPG                 (GPUIRQ1_BASE + 28)
#define IRQ_RNG                 (GPUIRQ1_BASE + 29)
#define IRQ_VC_ARASANSDIO       (GPUIRQ1_BASE + 30)
#define IRQ_AVSPMON             (GPUIRQ1_BASE + 31)

#define ARMIRQ_BASE             (2 << 5)
#define IRQ_ARM_TIMER           (ARMIRQ_BASE + 0)
#define IRQ_ARM_MAILBOX         (ARMIRQ_BASE + 1)
#define IRQ_ARM_DOORBELL_0      (ARMIRQ_BASE + 2)
#define IRQ_ARM_DOORBELL_1      (ARMIRQ_BASE + 3)
#define IRQ_VPU0_HALTED         (ARMIRQ_BASE + 4)
#define IRQ_VPU1_HALTED         (ARMIRQ_BASE + 5)
#define IRQ_ILLEGAL_TYPE0       (ARMIRQ_BASE + 6)
#define IRQ_ILLEGAL_TYPE1       (ARMIRQ_BASE + 7)
#define IRQ_PENDING1            (ARMIRQ_BASE + 8)
#define IRQ_PENDING2            (ARMIRQ_BASE + 9)
#define IRQ_JPEG                (ARMIRQ_BASE + 10)
#define IRQ_USB                 (ARMIRQ_BASE + 11)
#define IRQ_3D                  (ARMIRQ_BASE + 12)
#define IRQ_DMA2                (ARMIRQ_BASE + 13)
#define IRQ_DMA3                (ARMIRQ_BASE + 14)
#define IRQ_I2C                 (ARMIRQ_BASE + 15)
#define IRQ_SPI                 (ARMIRQ_BASE + 16)
#define IRQ_I2SPCM              (ARMIRQ_BASE + 17)
#define IRQ_SDIO                (ARMIRQ_BASE + 18)
#define IRQ_UART0               (ARMIRQ_BASE + 19)
#define IRQ_ARASANSDIO          (ARMIRQ_BASE + 20)

#define GPIO_PADS_0_27          0x002c
#define GPIO_PADS_28_45         0x0030
#define GPIO_PADS_46_53         0x0034 

#define GPFSEL0                 (GPIO_BASE + 0x0)               // GPIO Function Selectors..
#define GPFSEL1                 (GPIO_BASE + 0x4)
#define GPFSEL2                 (GPIO_BASE + 0x8)
#define GPFSEL3                 (GPIO_BASE + 0xC)
#define GPFSEL4                 (GPIO_BASE + 0x10
#define GPFSEL5                 (GPIO_BASE + 0x14)
#define GPSET0                  (GPIO_BASE + 0x1C)              // GPIO Pin Output control..
#define GPSET1                  (GPIO_BASE + 0x20)
#define GPCLR0                  (GPIO_BASE + 0x28)
#define GPCLR1                  (GPIO_BASE + 0x2C)
#define GPLEV0                  (GPIO_BASE + 0x34)              // GPIO Pin Levels..
#define GPLEV1                  (GPIO_BASE + 0x38)
#define GPEDS0                  (GPIO_BASE + 0x40)              // GPIO Pin Event Detect Status ..
#define GPEDS1                  (GPIO_BASE + 0x44)
#define GPREN0                  (GPIO_BASE + 0x4C)              // GPIO Pin Rising Edge Detect Enables..
#define GPREN1                  (GPIO_BASE + 0x50)
#define GPFEN0                  (GPIO_BASE + 0x58)              // GPIO Pin Falling Edge Detect Enables..
#define GPFEN1                  (GPIO_BASE + 0x5C)
#define GPHEN0                  (GPIO_BASE + 0x64)
#define GPHEN1                  (GPIO_BASE + 0x68)
#define GPLEN0                  (GPIO_BASE + 0x70)
#define GPLEN1                  (GPIO_BASE + 0x74)
#define GPAREN0                 (GPIO_BASE + 0x7c)
#define GPAREN1                 (GPIO_BASE + 0x80)
#define GPAFEN0                 (GPIO_BASE + 0x88)
#define GPAFEN1                 (GPIO_BASE + 0x8c)
#define GPPUD                   (GPIO_BASE + 0x94)
#define GPPUDCLK0               (GPIO_BASE + 0x98)
#define GPPUDCLK1               (GPIO_BASE + 0x9c)

#define SPI0_CS                 (0x00)
#define SPI0_FIFO               (0x04)
#define SPI0_CLK                (0x08)
#define SPI0_DLEN               (0x0c)
#define SPI0_LTOH               (0x10)
#define SPI0_DC                 (0x14)

#define BSC0_CONTROL            (BSC0_BASE + 0x00)
#define BSC0_STATUS             (BSC0_BASE + 0x01)
#define BSC0_DATALEN            (BSC0_BASE + 0x02)
#define BSC0_FIFO               (BSC0_BASE + 0x04)

#define BSC_CONTROL_READ        (1 << 0)
#define BSC_CONTROL_CLEAR       (1 << 4)
#define BSC_CONTROL_ST          (1 << 7)
#define BSC_CONTROL_INTD        (1 << 8)
#define BSC_CONTROL_INTT        (1 << 9)
#define BSC_CONTROL_INTR        (1 << 10)
#define BSC_CONTROL_I2CEN       (1 << 15)

#define BSC_STATUS_TA           (1 << 0)
#define BSC_STATUS_DONE         (1 << 1)
#define BSC_STATUS_TXW          (1 << 2)
#define BSC_STATUS_RXR          (1 << 3)
#define BSC_STATUS_TXD          (1 << 4)
#define BSC_STATUS_RXD          (1 << 5)
#define BSC_STATUS_TXE          (1 << 6)
#define BSC_STATUS_RXF          (1 << 7)
#define BSC_STATUS_ERR          (1 << 8)
#define BSC_STATUS_CLKT         (1 << 9)

#define BSC_READ                BSC_CONTROL_I2CEN|BSC_CONTROL_ST|BSC_CONTROL_CLEAR|BSC_CONTROL_READ
#define BSC_WRITE               BSC_CONTROL_I2CEN|BSC_CONTROL_ST
#define BSC_CLEAR               BSC_STATUS_CLKT|BSC_STATUS_ERR|BSC_STATUS_DONE

#define AUX_IRQ                 (ARM_PERIIOBASE + 0x215000)       // Auxiliary Interrupt status
#define AUX_ENABLES             (ARM_PERIIOBASE + 0x215004)       // Auxiliary enables
#define AUX_MU_IO_REG           (ARM_PERIIOBASE + 0x215040)       // AUX_MU_IO_REG Mini Uart I/O Data
#define AUX_MU_IER_REG          (ARM_PERIIOBASE + 0x215044)       // Mini Uart Interrupt Enable
#define AUX_MU_IIR_REG          (ARM_PERIIOBASE + 0x215048)       // Mini Uart Interrupt Identify
#define AUX_MU_LCR_REG          (ARM_PERIIOBASE + 0x21504C)       // Mini Uart Line Control
#define AUX_MU_MCR_REG          (ARM_PERIIOBASE + 0x215050)       // Mini Uart Modem Control
#define AUX_MU_LSR_REG          (ARM_PERIIOBASE + 0x215054)       // Mini Uart Line Status
#define AUX_MU_MSR_REG          (ARM_PERIIOBASE + 0x215058)       // Mini Uart Modem Status
#define AUX_MU_SCRATCH          (ARM_PERIIOBASE + 0x21505C)       // Mini Uart Scratch
#define AUX_MU_CNTL_REG         (ARM_PERIIOBASE + 0x215060)       // Mini Uart Extra Control
#define AUX_MU_STAT_REG         (ARM_PERIIOBASE + 0x215064)       // Mini Uart Extra Status
#define AUX_MU_BAUD_REG         (ARM_PERIIOBASE + 0x215068)       // Mini Uart Baudrate
#define AUX_SPI0_CNTL0_REG      (ARM_PERIIOBASE + 0x215080)       // SPI 1 Control register 0
#define AUX_SPI0_CNTL1_REG      (ARM_PERIIOBASE + 0x215084)       // SPI 1 Control register 1
#define AUX_SPI0_STAT_REG       (ARM_PERIIOBASE + 0x215088)       // SPI 1 Status
#define AUX_SPI0_IO_REG         (ARM_PERIIOBASE + 0x215090)       // SPI 1 Data
#define AUX_SPI0_PEEK_REG       (ARM_PERIIOBASE + 0x215094)       // SPI 1 Peek
#define AUX_SPI1_CNTL0_REG      (ARM_PERIIOBASE + 0x2150C0)       // SPI 2 Control register 0
#define AUX_SPI1_CNTL1_REG      (ARM_PERIIOBASE + 0x2150C4)       // SPI 2 Control register 1
#define AUX_SPI1_STAT_REG       (ARM_PERIIOBASE + 0x2150C8)       // SPI 2 Status
#define AUX_SPI1_IO_REG         (ARM_PERIIOBASE + 0x2150D0)       // SPI 2 Data
#define AUX_SPI1_PEEK_REG       (ARM_PERIIOBASE + 0x2150D4)       // SPI 2 Peek
#endif
#endif /* BCM2835_H */
