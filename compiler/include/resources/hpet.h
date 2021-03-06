/*
    Copyright (C) 2011-2020, The AROS Development Team. All rights reserved.

    Desc: AROS HPET ClockSource definitions.
*/

#ifndef RESOURCES_HPET_H
#define RESOURCES_HPET_H

#include <utility/tagitem.h>
#include <resources/clocksource.h>

#define HPET_TAG_BASE	(TAG_USER + 0x0ABC0000)

#define HPET_BASE_ADDR	(HPET_TAG_BASE + 0)	/* (IPTR)	  Base address of the timers block	*/
#define HPET_UNIT_ADDR	(HPET_TAG_BASE + 1)	/* (IPTR)	  Address of the particular unit	*/

#endif /* !RESOURCES_HPET_H */
