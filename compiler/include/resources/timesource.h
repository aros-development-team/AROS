/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS generic TimeSource resource definitions.
    Lang: english
*/

#ifndef RESOURCES_TIMESOURCE_H
#define RESOURCES_TIMESOURCE_H

#include <utility/tagitem.h>

#define TIMESOURCE_TAG_BASE	(TAG_USER + 0x0AAA0000)

#define TIMESOURCE_COUNT	        (TIMESOURCE_TAG_BASE + 0)	        /* Number of units exposed by the resource              */

#define TIMESOURCE_UNIT_OWNER	(TIMESOURCE_TAG_BASE + 100)	/* (const strcut Node *) Current owner of the unit      */

#endif /* !RESOURCES_TIMESOURCE_H */
