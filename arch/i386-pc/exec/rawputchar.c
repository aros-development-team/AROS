/*
    (C) Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Emit one character via raw IO
    Lang: english

    Note: serial IO from "PC-intern" examples
*/

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>
#include <asm/io.h>

//void putc(char);
int ser_WriteByte(int, BYTE , ULONG , BYTE , BYTE );
int ser_IsWritingPossible(int);


	AROS_LH1(void, RawPutChar,

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
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    Disable();
    
    /* Don't write 0 bytes */
    if (chr)
    {
	if (chr==0x0A)
		ser_WriteByte(0x2F8, 0x0D, 1, 0, 0);
	ser_WriteByte(0x2F8, chr, 1, 0, 0);
//	putc(chr);
    }

    Enable();
    
    AROS_LIBFUNC_EXIT
} /* RawPutChar */

#define SER_ERRSIGNALS 0x0300

#define SER_TXBUFFER       0x00
#define SER_LINE_STATUS		0x05
#define SER_MODEM_STATUS   0x06

#define SER_LSR_OVERRUNERROR 0x02
#define SER_LSR_PARITYERROR  0x04
#define SER_LSR_FRAMINGERROR 0x08
#define SER_LSR_BREAKDETECT  0x10
#define SER_LSR_ERRORMSK (SER_LSR_OVERRUNERROR|SER_LSR_PARITYERROR|\
                          SER_LSR_FRAMINGERROR|SER_LSR_BREAKDETECT)
#define SER_LSR_TSREMPTY	0x40

int ser_IsWritingPossible(int port) {

	return inb_p(port+SER_LINE_STATUS) & SER_LSR_TSREMPTY;
}

int ser_WriteByte(int port, BYTE data, ULONG timeout, BYTE sigmask, BYTE sigvals) {

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

