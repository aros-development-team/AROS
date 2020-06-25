/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS generic TimeSource resource definitions.
    Lang: english
*/

#ifndef RESOURCES_TIMESOURCE_H
#define RESOURCES_TIMESOURCE_H

#include <utility/tagitem.h>

#define TIMESOURCE_TAG_BASE	        (TAG_USER + 0x0AAA0000)

#define TIMESOURCE_COUNT	        (TIMESOURCE_TAG_BASE + 0)	        /* Number of units exposed by the resource              */
#define TIMESOURCE_ID                   (TIMESOURCE_TAG_BASE + 1)	        /* ID (e.g. "PIT", "HPET")                              */
#define TIMESOURCE_FRIENDLY             (TIMESOURCE_TAG_BASE + 2)	        /* Friendly Name                                        */
#define TIMESOURCE_FREQUENCY            (TIMESOURCE_TAG_BASE + 3)	        /* Frequency the timesource operates at                 */
#define TIMESOURCE_PERIODIC	        (TIMESOURCE_TAG_BASE + 4)	        /* Timesource supports periodic mode                    */
#define TIMESOURCE_ONESHOT	        (TIMESOURCE_TAG_BASE + 5)	        /* Timesource supports oneshot mode                     */

#define TIMESOURCE_UNIT_ID              (TIMESOURCE_TAG_BASE + 100)	        /* ID                                                   */
#define TIMESOURCE_UNIT_OWNER	        (TIMESOURCE_TAG_BASE + 101)	        /* (const strcut Node *) Current owner of a unit        */

#endif /* !RESOURCES_TIMESOURCE_H */
