/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

uint32_t count_bits_set(uint32_t x) {
    uint32_t c, y = x;

    c = 0x55555555;
    y = ((y>>1) & c) + (y & c);
    c = 0x33333333;
    y = ((y>>2) & c) + (y & c);
    y = (y>>4) + y;
    c = 0x0f0f0f0f;
    y &= c;
    y = (y>>8) + y;
    y = (y>>16) + y;
    return y & 0x1f;
}

void delay_ms(struct ahci_hba_chip *hba_chip, uint32_t msec) {

	/* Allocate a signal within this task context */
	hba_chip->tr.tr_node.io_Message.mn_ReplyPort->mp_SigBit = SIGB_SINGLE;
	hba_chip->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

	/* Specify the request */
	hba_chip->tr.tr_node.io_Command = TR_ADDREQUEST;
	hba_chip->tr.tr_time.tv_secs = msec / 1000;
	hba_chip->tr.tr_time.tv_micro = 1000 * (msec % 1000);

	/* Wait */
	DoIO((struct IORequest *)&hba_chip->tr);

	hba_chip->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}

void delay_us(struct ahci_hba_chip *hba_chip, uint32_t usec) {

	/* Allocate a signal within this task context */
	hba_chip->tr.tr_node.io_Message.mn_ReplyPort->mp_SigBit = SIGB_SINGLE;
	hba_chip->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

	/* Specify the request */
	hba_chip->tr.tr_node.io_Command = TR_ADDREQUEST;
	hba_chip->tr.tr_time.tv_secs = usec / 1000000;
	hba_chip->tr.tr_time.tv_micro = (usec % 1000000);

	/* Wait */
	DoIO((struct IORequest *)&hba_chip->tr);

	hba_chip->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}

BOOL wait_until_set(struct ahci_hba_chip *hba_chip, volatile uint32_t *reg, uint32_t bits, uint32_t timeout) {
	int trys = (timeout + 9999) / 10000;
	while (trys--) {
		if (((*reg) & bits) == bits)
			return TRUE;
        delay_us(hba_chip, 10000);
	}
	return FALSE;
}

BOOL wait_until_clr(struct ahci_hba_chip *hba_chip, volatile uint32_t *reg, uint32_t bits, uint32_t timeout) {
	int trys = (timeout + 9999) / 10000;
	while (trys--) {
		if (((*reg) & bits) == 0)
			return TRUE;
        delay_us(hba_chip, 10000);
	}
	return FALSE;
}

