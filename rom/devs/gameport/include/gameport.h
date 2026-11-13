#ifndef DEVICES_GAMEPORT_H
#define DEVICES_GAMEPORT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Important defines and structures for gameport.device
    Lang: english
*/

#include <exec/io.h>

/**********************************************************************
 ********************** Gameport Device Commands **********************
 **********************************************************************/

#define GPD_READEVENT        (CMD_NONSTD + 0)
#define GPD_ASKCTYPE         (CMD_NONSTD + 1)
#define GPD_SETCTYPE         (CMD_NONSTD + 2)
#define GPD_ASKTRIGGER       (CMD_NONSTD + 3)
#define GPD_SETTRIGGER       (CMD_NONSTD + 4)

/********************************************************
 ********************** Structures **********************
 ********************************************************/
 
#define GPTB_DOWNKEYS	0
#define GPTB_UPKEYS	1
 
#define GPTF_DOWNKEYS	(1 << GPTB_DOWNKEYS)
#define GPTF_UPKEYS	(1 << GPTB_UPKEYS)
 
struct GamePortTrigger
{
    UWORD gpt_Keys;
    UWORD gpt_Timeout;
    UWORD gpt_XDelta;
    UWORD gpt_YDelta;
};


/**************************************************************
 ********************** Controller Types **********************
 **************************************************************/

#define GPCT_ALLOCATED		-1
#define GPCT_NOCONTROLLER	0
#define GPCT_MOUSE		1
#define GPCT_RELJOYSTICK	2
#define GPCT_ABSJOYSTICK	3

#endif /* DEVICES_GAMEPORT_H */
