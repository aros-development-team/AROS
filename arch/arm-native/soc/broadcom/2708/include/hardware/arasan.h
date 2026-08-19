/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
*/

#ifndef ARASAN_H
#define ARASAN_H

#define ARASAN_BASE                     (ARM_PERIIOBASE + 0x300000)

/*
 * BCM2711 routes the card slot to a second controller with the same standard
 * SDHCI register layout, in its own window and with its own clock and IRQ.
 */
#define EMMC2_BASE                      (ARM_PERIIOBASE + 0x340000)

/* BCM2712 (Raspberry Pi 5) eMMC2 controller */
#define BCM2712_EMMC2_BASE              (ARM_PERIIOBASE + 0x100000)

/* Both windows share one interrupt line on the BCM2711 (GIC SPI 126). */
#define IRQ_BCM2711_SDHCI               158
#define IRQ_BCM2712_SDHCI               300

/* Identification clock, used until the card reports what it can take. */
#define BCM2708SDCLOCK_MIN              400000

#define ARASAN_CMD                      (ARASAN_BASE + 0x00)
#define ARASAN_ARG	                (ARASAN_BASE + 0x04)
#define ARASAN_TIMEOUT	                (ARASAN_BASE + 0x08)
#define ARASAN_CLKDIV	                (ARASAN_BASE + 0x0C)
#define ARASAN_RESPONSE_0	        (ARASAN_BASE + 0x10)
#define ARASAN_RESPONSE_1	        (ARASAN_BASE + 0x14)
#define ARASAN_RESPONSE_2	        (ARASAN_BASE + 0x18)
#define ARASAN_RESPONSE_3	        (ARASAN_BASE + 0x1C)
#define ARASAN_STATUS	                (ARASAN_BASE + 0x20)
#define ARASAN_VDD	                (ARASAN_BASE + 0x30)
#define ARASAN_EDM	                (ARASAN_BASE + 0x34)
#define ARASAN_HOSTCONFIG	        (ARASAN_BASE + 0x38)
#define ARASAN_HBCT	                (ARASAN_BASE + 0x3c)
#define ARASAN_DATA	                (ARASAN_BASE + 0x40)
#define ARASAN_HBLC	                (ARASAN_BASE + 0x50)

#define ARASAN_CMD_READ     	        (1 << 6)
#define ARASAN_CMD_WRITE    	        (1 << 7)
#define ARASAN_CMD_LONGRSP 	        (1 << 9)
#define ARASAN_CMD_NORSP   	        (1 << 10)
#define ARASAN_CMD_BUSY     	        (1 << 11)
#define ARASAN_CMD_FAIL	                (1 << 14)
#define ARASAN_CMD_ENABLE   	        (1 << 15)

#define ARASAN_VDD_ENABLE	        (1 << 0)

#define ARASAN_HOSTCONFIG_WIDE_INT_BUS  0x2
#define ARASAN_HOSTCONFIG_WIDEEXT_4BIT  0x4
#define ARASAN_HOSTCONFIG_SLOW_CARD     0x8
#define ARASAN_HOSTCONFIG_BLOCK_IRPT_EN (1<<8)
#define ARASAN_HOSTCONFIG_BUSY_IRPT_EN  (1<<10)
#define ARASAN_HOSTCONFIG_WIDEEXT_CLR   0xFFFFFFFB

#define ARASAN_DATAFLAG	                (1 << 0)
#define ARASAN_CMDTIMEOUT	        (1 << 6)
#define ARASAN_HSTS_BLOCK	        (1 << 9)	/**< block flag in status reg */
#define ARASAN_HSTS_BUSY	        (1 << 10)	/**< Busy flag in status reg */

#define ARASAN_RW_THRESHOLD             3

#endif /* ARASAN_H */
