/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RASPI_MINIUART_H
#define RASPI_MINIUART_H

#include <exec/types.h>
#include <stdint.h>

void ser_InitMINIUART(void);
void ser_PutCMINIUART(uint32_t c);

#endif /* RASPI_MINIUART */
