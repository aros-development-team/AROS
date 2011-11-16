/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <hardware/intbits.h>

#include "amiga_hwreg.h"
#include "debug.h"

void DebugInit(void)
{
	/* Set DTR, RTS, etc */
	volatile UBYTE *ciab_pra = (APTR)0xBFD000;
	volatile UBYTE *ciab_ddra = (APTR)0xBFD200;
	*ciab_ddra = 0xc0;  /* Only DTR and RTS are driven as outputs */
	*ciab_pra = 0;      /* Turn on DTR and RTS */

	/* Set the debug UART to 115200 */
	reg_w(SERPER, SERPER_BAUD(SERPER_BASE_PAL, 115200));
}

int DebugPutChar(register int chr)
{
	if (chr == '\n')
		DebugPutChar('\r');

	while ((reg_r(SERDATR) & SERDATR_TBE) == 0);
	reg_w(INTREQ, INTF_TBE);

	/* Output a char to the debug UART */
	reg_w(SERDAT, SERDAT_STP8 | SERDAT_DB8(chr));

	return 1;
}

int DebugMayGetChar(void)
{
	int c;

	if ((reg_r(SERDATR) & SERDATR_RBF) == 0)
	    return -1;

	c = SERDATR_DB8_of(reg_r(SERDATR));

	/* Clear RBF */
	reg_w(INTREQ, INTF_RBF);

	return c;
}

#if AROS_SERIAL_DEBUG

void DebugPutStr(register const char *buff)
{
	for (; *buff != 0; buff++)
		DebugPutChar(*buff);
}

void DebugPutDec(const char *what, ULONG val)
{
	int i, num;
	DebugPutStr(what);
	DebugPutStr(": ");
	if (val == 0) {
	    DebugPutChar('0');
	    DebugPutChar('\n');
	    return;
	}

	for (i = 1000000000; i > 0; i /= 10) {
	    if (val == 0) {
	    	DebugPutChar('0');
	    	continue;
	    }

	    num = val / i;
	    if (num == 0)
	    	continue;

	    DebugPutChar("0123456789"[num]);
	    val -= num * i;
	}
	DebugPutChar('\n');
}

void DebugPutHex(const char *what, ULONG val)
{
	int i;
	DebugPutStr(what);
	DebugPutStr(": ");
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar('\n');
}

void DebugPutHexVal(ULONG val)
{
	int i;
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar(' ');
}

#endif


