/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: serialdebug.c - AArch64 serial debug output for boot
*/

#include <aros/macros.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>
#include <hardware/pl011uart.h>

#include "serialdebug.h"
#include "bcm2708_boot.h"
#include "devicetree.h"
#include "bootconsole.h"
#include "vc_mb.h"
#include "io.h"

#undef ARM_PERIIOBASE
#define ARM_PERIIOBASE (__arm_periiobase)
extern uintptr_t __arm_periiobase;
extern unsigned int __arm_socid;
extern uintptr_t __rp1_base;
uintptr_t __uart_base = 0;

/*
 * BCM2712 has no BCM283x UART at +0x201000. The debug connector is uart10
 * (/soc/serial@7d001000, a PL011), 0x7D001000 into the peripheral window.
 */
#define RP1_UART0_BASE (__rp1_base ? __rp1_base + 0x30000 : 0x1c00030000ULL)

#define UART_BASE (__arm_socid == 0x2712 ? RP1_UART0_BASE : PL011_0_BASE)

/*
 * On a Pi 3 or Zero 2 W the firmware gives the PL011 to the Bluetooth radio
 * and makes the AUX mini-UART serial0 on GPIO 14/15. The bootstrap follows
 * the serial0 alias there, so the console never shares a UART with the radio.
 */
#define AUX_MU_LSR_TXEMPTY  (1 << 5)

static int ser_mini;

#define SER_DR (ser_mini ? AUX_MU_IO_REG : UART_BASE + PL011_DR)

#define PL011_ICR_FLAGS (PL011_ICR_RXIC|PL011_ICR_TXIC|PL011_ICR_RTIC|PL011_ICR_FEIC|PL011_ICR_PEIC|PL011_ICR_BEIC|PL011_ICR_OEIC|PL011_ICR_RIMIC|PL011_ICR_CTSMIC|PL011_ICR_DSRMIC|PL011_ICR_DCDMIC)

#define DEF_BAUD 115200

#define PL011_DIVCLOCK(baud, clock)     ((clock * 4) / baud)
#define PL011_BAUDINT(baud, clock)      ((PL011_DIVCLOCK(baud, clock) & 0xFFFFFFC0) >> 6)
#define PL011_BAUDFRAC(baud, clock)     ((PL011_DIVCLOCK(baud, clock) & 0x0000003F) >> 0)

unsigned int uartclock;
unsigned int uartdivint;
unsigned int uartdivfrac;
unsigned int uartbaud;

inline void waitSerOUT()
{
    if (ser_mini)
    {
        while ((rd32le(AUX_MU_LSR_REG) & AUX_MU_LSR_TXEMPTY) == 0) ;
        return;
    }

    while(1)
    {
       if ((rd32le(UART_BASE + PL011_FR) & PL011_FR_TXFF) == 0) break;
    }
}

inline void putByte(uint8_t chr)
{
    waitSerOUT();

    if (chr == '\n')
    {
        wr32le(SER_DR, '\r');
        waitSerOUT();
    }
    wr32le(SER_DR, chr);
}

/* Does the device tree's serial0 alias name the mini-UART (serial@7e215040)? */
static int serial0_is_mini(void)
{
    static const char mu[] = "215040";
    of_node_t *aliases = dt_find_node("/aliases");
    of_property_t *p = aliases ? dt_find_property(aliases, "serial0") : NULL;
    const char *s;
    int i, n;

    if (!p || p->op_length < sizeof(mu))
        return 0;

    /* op_length counts the terminating NUL */
    s = (const char *)p->op_value + p->op_length - sizeof(mu);
    for (i = 0, n = sizeof(mu) - 1; i < n; i++)
        if (s[i] != mu[i])
            return 0;

    return 1;
}

void serInit(void)
{
    unsigned int        uartvar;

    volatile unsigned int *uart_msg = (unsigned int *) BOOTMEMADDR(bm_mboxmsg);

    uartbaud = DEF_BAUD;

    __uart_base = UART_BASE;

    /* BCM2712 has no BCM283x GPIO block, and uart10's pins are dedicated.
     * Firmware has already set this up.
     */
    if (__arm_socid == 0x2712)
    {
        /*
         * RP1 UART0 has already been initialised by firmware:
         *
         * config.txt:
         *   enable_uart=1
         *   enable_rp1_uart=1
         *   pciex4_reset=0
         *
         * GPIO14 = TX
         * GPIO15 = RX
         * 115200 8N1
         */
        uartbaud = 115200;

        return;
    }

    /* BCM2711 is left on the PL011: pl011bt does not drive its radio. */
    if (__arm_socid == 0xc43 && serial0_is_mini())
    {
        ser_mini = 1;
        __uart_base = AUX_MU_IO_REG;
    }

    uart_msg[0] = AROS_LONG2LE(8 * 4);
    uart_msg[1] = AROS_LONG2LE(VCTAG_REQ);
    uart_msg[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    uart_msg[3] = AROS_LONG2LE(8);
    uart_msg[4] = AROS_LONG2LE(4);
    uart_msg[5] = AROS_LONG2LE(ser_mini ? VCCLOCK_CORE : VCCLOCK_UART);  // the mini-UART divides the core clock
    uart_msg[6] = 0;
    uart_msg[7] = 0;                            // terminate tag

    vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void*)uart_msg);
    uart_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);

    if (uart_msg)
        uartclock = AROS_LE2LONG(uart_msg[6]);
    else
        uartclock = ser_mini ? 250000000 : 48000000;   /* firmware defaults */

    if (ser_mini)
    {
        wr32le(AUX_ENABLES, rd32le(AUX_ENABLES) | 1);
        wr32le(AUX_MU_CNTL_REG, 0);
        wr32le(AUX_MU_IER_REG, 0);
    }
    else
        wr32le(UART_BASE + PL011_CR, 0);

    uartvar = rd32le(GPFSEL1);
    uartvar &= ~(7<<12);                        // TX on GPIO14
    uartvar |= (ser_mini ? 2 : 4)<<12;          // alt5 (mini-UART) or alt0 (PL011)
    uartvar &= ~(7<<15);                        // RX on GPIO15
    uartvar |= (ser_mini ? 2 : 4)<<15;
    wr32le(GPFSEL1, uartvar);

    /* Disable pull-ups and pull-downs on rs232 lines */
    wr32le(GPPUD, 0);

    for (uartvar = 0; uartvar < 150; uartvar++) asm volatile ("nop\n");

    wr32le(GPPUDCLK0, (1 << 14)|(1 << 15));

    for (uartvar = 0; uartvar < 150; uartvar++) asm volatile ("nop\n");

    wr32le(GPPUDCLK0, 0);

    if (ser_mini)
    {
        wr32le(AUX_MU_LCR_REG, 3);              // 8 bit (bit 1 is needed too)
        wr32le(AUX_MU_MCR_REG, 0);
        wr32le(AUX_MU_IIR_REG, 0xC6);           // clear both FIFOs
        wr32le(AUX_MU_BAUD_REG, uartclock / (8 * uartbaud) - 1);
        wr32le(AUX_MU_CNTL_REG, 3);             // enable tx and rx
        return;
    }

    wr32le(UART_BASE + PL011_ICR, PL011_ICR_FLAGS);
    uartdivint = PL011_BAUDINT(uartbaud, uartclock);
    wr32le(UART_BASE + PL011_IBRD, uartdivint);
    uartdivfrac = PL011_BAUDFRAC(uartbaud, uartclock);
    wr32le(UART_BASE + PL011_FBRD, uartdivfrac);
    wr32le(UART_BASE + PL011_LCRH, PL011_LCRH_WLEN8|PL011_LCRH_FEN);           // 8N1, Fifo enabled
    wr32le(UART_BASE + PL011_CR, PL011_CR_UARTEN|PL011_CR_TXE|PL011_CR_RXE);   // enable the uart, tx and rx

    for (uartvar = 0; uartvar < 150; uartvar++) asm volatile ("nop\n");
}
