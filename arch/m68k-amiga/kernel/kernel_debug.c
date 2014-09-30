/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/** Screen/Serial Debug **/
#define SERDATR			0x18
#define   SERDATR_OVRUN		(1 << 15)	/* Overrun */
#define   SERDATR_RBF		(1 << 14)	/* Rx Buffer Full */
#define   SERDATR_TBE		(1 << 13)	/* Tx Buffer Empty */
#define   SERDATR_TSRE		(1 << 12)	/* Tx Shift Empty */
#define   SERDATR_RXD		(1 << 11)	/* Rx Pin */
#define   SERDATR_STP9		(1 <<  9)	/* Stop bit (if 9 data) */
#define   SERDATR_STP8		(1 <<  8)	/* Stop bit (if 8 data) */
#define   SERDATR_DB9_of(x)	((x) & 0x1ff)	/* 9-bit data */
#define   SERDATR_DB8_of(x)	((x) & 0xff)	/* 8-bit data */
#define SERDAT			0x30
#define   SERDAT_STP9		(1 << 9)
#define   SERDAT_STP8		(1 << 8)
#define   SERDAT_DB9(x)		((x) & 0x1ff)
#define   SERDAT_DB8(x)		((x) & 0xff)

static inline void reg_w(ULONG reg, UWORD val)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	*r = val;
}

static inline UWORD reg_r(ULONG reg)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	return *r;
}

/*
 * KernelBase is an optional parameter here. During
 * very early startup it can be NULL.
 */
int krnPutC(int chr, struct KernelBase *KernelBase)
{
	if (chr == '\n')
		krnPutC('\r', KernelBase);

	while ((reg_r(SERDATR) & SERDATR_TBE) == 0);

	/* Output a char to the debug UART */
	reg_w(SERDAT, SERDAT_STP8 | SERDAT_DB8(chr));

	return 1;
}
