/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS generic ClockSource resource definitions.
    Lang: english
*/

#ifndef RESOURCES_CLOCKSOURCE_H
#define RESOURCES_CLOCKSOURCE_H

#include <utility/tagitem.h>

#define CLOCKSOURCE_TAG_BASE	        (TAG_USER + 0x0AAA0000)

#define CLOCKSOURCE_COUNT	        (CLOCKSOURCE_TAG_BASE + 0)	        /* Number of units exposed by the resource              */
#define CLOCKSOURCE_ID                   (CLOCKSOURCE_TAG_BASE + 1)	        /* ID (e.g. "PIT", "HPET")                              */
#define CLOCKSOURCE_FRIENDLY             (CLOCKSOURCE_TAG_BASE + 2)	        /* Friendly Name                                        */
#define CLOCKSOURCE_FREQUENCY            (CLOCKSOURCE_TAG_BASE + 3)	        /* Frequency the timesource operates at                 */
#define CLOCKSOURCE_PERIODIC	        (CLOCKSOURCE_TAG_BASE + 4)	        /* Timesource supports periodic mode                    */
#define CLOCKSOURCE_ONESHOT	        (CLOCKSOURCE_TAG_BASE + 5)	        /* Timesource supports oneshot mode                     */

#define CLOCKSOURCE_UNIT_ID              (CLOCKSOURCE_TAG_BASE + 100)	        /* ID                                                   */
#define CLOCKSOURCE_UNIT_OWNER	        (CLOCKSOURCE_TAG_BASE + 101)	        /* (const strcut Node *) Current owner of a unit        */

#endif /* !RESOURCES_CLOCKSOURCE_H */
