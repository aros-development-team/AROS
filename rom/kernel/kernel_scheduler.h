/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

BOOL core_Schedule(void);			/* Reschedule the current task if needed */
void core_Switch(void);				/* Switch away from the current task     */
struct Task *core_Dispatch(void);		/* Select the new task for execution     */
