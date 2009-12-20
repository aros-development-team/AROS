/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: functions for serial RawIOInit/RawPutChar
    Lang: english

    Note: serial io from "PC-intern" examples
*/
#include <proto/exec.h>
#include <asm/io.h>

#undef __save_flags
#undef __restore_flags
#undef __cli
#undef __sti

#define __save_flags(x)		__asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory", "cc")
#define __cli() 		__asm__ __volatile__("cli": : :"memory")
#define __sti()			__asm__ __volatile__("sti": : :"memory")

#define SER_ERRSIGNALS 0x0300

#define SER_LSR_OVERRUNERROR 0x02
#define SER_LSR_PARITYERROR  0x04
#define SER_LSR_FRAMINGERROR 0x08
#define SER_LSR_BREAKDETECT  0x10
#define SER_LSR_ERRORMSK (SER_LSR_OVERRUNERROR|SER_LSR_PARITYERROR|\
                          SER_LSR_FRAMINGERROR|SER_LSR_BREAKDETECT)
#define SER_LSR_TSREMPTY	0x40

#define SER_LCR_8BITS      0x03
#define SER_LCR_1STOPBIT   0x00
#define SER_LCR_NOPARITY   0x00
#define SER_LCR_SETDIVISOR 0x80

#define SER_TXBUFFER       0x00
#define SER_DIVISOR_LSB    0x00
#define SER_DIVISOR_MSB    0x01
#define SER_IRQ_ENABLE     0x01
#define SER_IRQ_ID         0x02
#define SER_FIFO           0x02
#define SER_2FUNCTION      0x02
#define SER_LINE_CONTROL   0x03
#define SER_MODEM_CONTROL  0x04
#define SER_LINE_STATUS    0x05
#define SER_MODEM_STATUS   0x06
#define SER_SCRATCH        0x07

#define SER_MCR_DTR        0x01
#define SER_MCR_RTS        0x02
#define SER_MCR_LOOP       0x10

#define SER_FIFO_ENABLE        0x01
#define SER_FIFO_RESETRECEIVE  0x02
#define SER_FIFO_RESETTRANSMIT 0x04

#define SER_MAXBAUD 115200L

#if AROS_SERIAL_DEBUG == 2
UWORD __serial_rawio_port = 0x2F8;
#else
UWORD __serial_rawio_port = 0x3F8;
#endif

int ser_UARTType (short);
void ser_FIFOLevel(short, BYTE);
int ser_Init(short, LONG, BYTE);

/*****i***********************************************************************

    NAME */
	AROS_LH0(void, SerialRawIOInit,

/*  LOCATION */
	struct ExecBase *, SysBase, 84, Exec)

/*  FUNCTION
	This is a private function. It initializes raw IO. After you
	have called this function, you can use (!RawMayGetChar()) and
	RawPutChar().

    INPUTS
	None.

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawPutChar(), RawMayGetChar()

    INTERNALS

    HISTORY

*****************************************************************************/
{
   AROS_LIBFUNC_INIT
    if (__serial_rawio_port > 0)
    {
	if (ser_Init(__serial_rawio_port, SER_MAXBAUD, SER_LCR_8BITS | SER_LCR_1STOPBIT | SER_LCR_NOPARITY))
		ser_FIFOLevel(__serial_rawio_port, 0);
    }

   return;
   AROS_LIBFUNC_EXIT
} /* RawIOInit */

int ser_UARTType (short port) {

	outb_p(0xAA, port+SER_LINE_CONTROL); /* set Divisor-Latch */
	if (inb_p(port+SER_LINE_CONTROL) != 0xAA)
		return 1;
	outb_p(0x55, port+SER_DIVISOR_MSB); /* write Divisor */
	if (inb_p(port+SER_DIVISOR_MSB) != 0x55)
		return 1;
	outb_p(0x55, port+SER_LINE_CONTROL); /* clear Divisor-Latch */
	if (inb_p(port+SER_LINE_CONTROL) != 0x55)
		return 1;
	outb_p(0x55, port+SER_IRQ_ENABLE);
	if (inb_p(port+SER_IRQ_ENABLE) != 0x05)
		return 1;
	outb_p(0, port+SER_FIFO); /* clear FIFO and IRQ */
	outb_p(0, port+SER_IRQ_ENABLE);
	if (inb_p(port+SER_IRQ_ID) != 1)
		return 1;
	outb_p(0xF5, port+SER_MODEM_CONTROL);
	if (inb_p(port+SER_MODEM_CONTROL) != 0x15)
		return 1;
	outb_p(SER_MCR_LOOP, port+SER_MODEM_CONTROL); /* Looping */
	inb_p(port+SER_MODEM_STATUS);
	if ((inb_p(port+SER_MODEM_STATUS) & 0xF0) != 0)
		return 1;
	outb_p(0x1F, port+SER_MODEM_CONTROL);
	if ((inb_p(port+SER_MODEM_STATUS) & 0xF0) != 0xF0)
		return 1;
	outb_p(SER_MCR_DTR | SER_MCR_RTS, port + SER_MODEM_CONTROL);

	outb_p(0x55, port+SER_SCRATCH); /* Scratch-Register ?*/
	if (inb_p(port+SER_SCRATCH) != 0x55)
		return 2;	//INS8250;
	outb_p(0, port+SER_SCRATCH);

	outb_p(0xCF, port+SER_FIFO);              /* FIFO ? */
	if ((inb_p(port+SER_IRQ_ID) & 0xC0) != 0xC0)
		return 3;	//NS16450;
	outb_p(0, port+SER_FIFO);
                            /* Alternate-Function Register ? */
	outb_p(SER_LCR_SETDIVISOR, port+SER_LINE_CONTROL);
	outb_p(0x07, port+SER_2FUNCTION);
	if (inb_p(port+SER_2FUNCTION ) != 0x07)
	{
		outb_p(0, port+SER_LINE_CONTROL);
		return 4;	//NS16550A;
	}
	outb_p(0, port+SER_LINE_CONTROL);               /* reset registers */
	outb_p(0, port+SER_2FUNCTION);
	return 5;	//NS16C552;
}

void ser_FIFOLevel(short port, BYTE level) {

	if (level)
		outb_p(level | SER_FIFO_ENABLE, port+SER_FIFO);
	else
		outb_p(SER_FIFO_RESETRECEIVE | SER_FIFO_RESETTRANSMIT, port+SER_FIFO);
}

int ser_Init(short port, LONG baudRate, BYTE params) {
WORD uDivisor;

	if (ser_UARTType(port)!=1)
	{
		uDivisor=(WORD)(SER_MAXBAUD / baudRate);
		outb_p(inb_p(port+SER_LINE_CONTROL) | SER_LCR_SETDIVISOR, port+SER_LINE_CONTROL);
		outb_p(uDivisor & 0xFF, port+SER_DIVISOR_LSB);
		outb_p(uDivisor>>8, port+SER_DIVISOR_MSB);
		outb_p(inb_p(port+SER_LINE_CONTROL) & ~SER_LCR_SETDIVISOR, port+SER_LINE_CONTROL);

		outb_p(params, port+SER_LINE_CONTROL);
		inb_p(port+SER_TXBUFFER);
		return 1;
	}
	return 0;
}

int ser_WriteByte(short, UBYTE , ULONG , BYTE , BYTE );
int ser_IsWritingPossible(short);


	AROS_LH1(void, SerialRawPutChar,

/*  SYNOPSIS */
	AROS_LHA(UBYTE, chr, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 86, Exec)

/*  FUNCTION
	Emits a single character.

    INPUTS
	chr - The character to emit

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawIOInit(), RawPutChar(), RawMayGetChar()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    unsigned long flags;

    if ((__serial_rawio_port > 0) && (chr))
    {
	__save_flags(flags);

        /* stegerg: Don't use Disable/Enable, because we want
           interrupt enabled flag to stay the same as
           it was before the Disable() call */
	__cli();
	
	if (chr==0x0A)
        {
            // Send <CR> before <LF>
    	    ser_WriteByte(__serial_rawio_port, 0x0D, 0, 0, 0);
	}
	ser_WriteByte(__serial_rawio_port, chr, 0, 0, 0);

        /* Interrupt flag is stored in flags - if it was
           enabled before, it will be renabled when the flags
           are restored */
    	__restore_flags(flags);
    }
    
	AROS_LIBFUNC_EXIT
} /* RawPutChar */

int ser_IsWritingPossible(short port) {

	return inb_p(port+SER_LINE_STATUS) & SER_LSR_TSREMPTY;
}

int ser_WriteByte(short port, UBYTE data, ULONG timeout, BYTE sigmask, BYTE sigvals) {

	if (timeout)
	{
		while (!ser_IsWritingPossible(port) && timeout)
			timeout--;
		if (!timeout)
			return 1;
	}
	else
		while (!ser_IsWritingPossible(port));

	if ((inb_p(port+SER_MODEM_STATUS) & sigmask) == sigvals)
	{
		outb_p(data, port+SER_TXBUFFER);
		return inb_p(port+SER_LINE_STATUS) & SER_LSR_ERRORMSK;
	}
	else
		return SER_ERRSIGNALS;
}

