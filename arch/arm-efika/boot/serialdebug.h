/*
 * $Id$
 *
 *  Created on: Nov 8, 2009
 *      Author: misc
 */

#ifndef SERIALDEBUG_H_
#define SERIALDEBUG_H_

#define UART1_BASE_ADDR		0x73fbc000

#define ONEMS	(0xb0/4)
#define UBIR	(0xa4/4)
#define UBMR	(0xa8/4)
#define UCR2	(0x84/4)

void putByte(uint8_t chr);
void putBytes(const char *str);
void waitBusy();

#endif /* SERIALDEBUG_H_ */
