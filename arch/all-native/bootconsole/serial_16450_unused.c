/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This code was rescued from arch/x86_64/exec/serialrawio.c before x86-64 port update.
 * Perhaps it can be useful in future, however it's questionable whether it is really
 * needed for debugging console.
 */

#include <asm/io.h>
#include <hardware/uart.h>

/** UART Types **/
#define SER_UART_INS8250       1
#define SER_UART_NS16450       2
#define SER_UART_NS16550A      3
#define SER_UART_NS16C552      4

unsigned char __ser_UARTType(short port)
{
    outb_p(0xAA, port + UART_LCR);                  /* set Divisor-Latch */
    if (inb_p(port + UART_LCR) != 0xAA)
        return 0;
    outb_p(0x55, port + UART_DLM);                   /* write Divisor */
    if (inb_p(port + UART_DLM) != 0x55)
        return 0;
    outb_p(0x55, port + UART_LCR);                  /* clear Divisor-Latch */
    if (inb_p(port + UART_LCR) != 0x55)
        return 0;
    outb_p(0x55, port + UART_IER);
    if (inb_p(port + UART_IER) != 0x05)
        return 0;
    outb_p(0, port + UART_FCR);                     /* clear FIFO and IRQ */
    outb_p(0, port + UART_IER);
    if (inb_p(port + UART_IIR) != 1)
        return 0;
    outb_p(0xF5, port + UART_MCR);
    if (inb_p(port + UART_MCR) != 0x15)
        return 0;
    outb_p(UART_MCR_LOOP, port + UART_MCR);         /* Looping */
    inb_p(port + UART_MSR);
    if ((inb_p(port + UART_MSR) & 0xF0) != 0)
        return 0;
    outb_p(0x1F, port + UART_MCR);
    if ((inb_p(port + UART_MSR) & 0xF0) != 0xF0)
        return 0;
    outb_p(SER_MCR_DTR | SER_MCR_RTS, port + UART_MCR);

    outb_p(0x55, port + UART_SCR);                  /* Scratch-Register ?*/
    if (inb_p(port + UART_SCR) != 0x55)
        return SER_UART_INS8250;
    outb_p(0, port + UART_SCR);

    outb_p(0xCF, port + UART_FCR);                  /* FIFO ? */
    if ((inb_p(port + UART_IIR) & 0xC0) != 0xC0)
        return SER_UART_NS16450;
    outb_p(0, port + UART_FCR);

    outb_p(UART_LCR_DLAB, port + UART_LCR);    /* Alternate-Function Register ? */
    outb_p(0x07, port + UART_EFR);
    if (inb_p(port + UART_EFR ) != 0x07)
    {
        outb_p(0, port + UART_LCR);
        return SER_UART_NS16550A;
    }
    outb_p(0, port + UART_LCR);                     /* reset registers */
    outb_p(0, port + UART_EFR);
    return SER_UART_NS16C552;
}

static void __ser_FIFOLevel(short port, BYTE level)
{
    if (level)
        outb_p(level | UART_FCR_ENABLE_FIFO, port + UART_FCR);
    else
        outb_p(UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, port + UART_FCR);
}
