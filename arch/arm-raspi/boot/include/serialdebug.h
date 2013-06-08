/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SERIALDEBUG_H_
#define SERIALDEBUG_H_

void serInit();
//void waitBusy();
void waitSerIN();
void waitSerOUT();
void putByte(uint8_t chr);

#endif /* SERIALDEBUG_H_ */
