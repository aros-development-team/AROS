/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS generic HPET definitions.
    Lang: english
*/

#ifndef RESOURCES_HPET_H
#define RESOURCES_HPET_H

#include <utility/tagitem.h>

#define HPET_TAG_BASE	(TAG_USER + 0x0ABC0000)

#define HPET_BASE_ADDR	(HPET_TAG_BASE + 0)	/* (IPTR)	  Base address of the timers block	*/
#define HPET_UNIT_ADDR	(HPET_TAG_BASE + 1)	/* (IPTR)	  Address of the particular unit	*/
#define HPET_UNIT_OWNER	(HPET_TAG_BASE + 2)	/* (const char *) Current owner	of the unit		*/

#endif
