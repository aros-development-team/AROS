/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include "amiga_hwreg.h"
#include "debug.h"

void DebugInit()
{
	/* Set the debug UART to 115200 */
	reg_w(SERPER, SERPER_BAUD(SERPER_BASE_PAL, 115200));
}

int DebugPutChar(register int chr)
{
	if (chr == '\n')
		DebugPutChar('\r');
	while ((reg_r(SERDATR) & SERDATR_TBE) == 0);

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
	reg_w(INTREQ, (1 << 11));

	return c;
}

#ifdef AROS_SERIAL_DEBUG

void DebugPuts(register const char *buff)
{
	for (; *buff != 0; buff++)
		DebugPutChar(*buff);
}

void DebugPutHex(const char *what, ULONG val)
{
	int i;
	DebugPuts(what);
	DebugPuts(": ");
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar('\n');
}
#endif


