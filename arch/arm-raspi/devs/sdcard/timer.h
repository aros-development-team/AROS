/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

struct SDCardBase;

/*
 * sdcard_OpenTimer
 *   create timerequest to manage timed operations
 * result
 *   timerequest to be used with any of the calls below
 * note
 *   only one task can use given timerequest
 */
struct IORequest *sdcard_OpenTimer(struct SDCardBase *SDCardBase);

/*
 * sdcard_CloseTimer
 *   dispose timerequest; most likely never used ;)
 * params
 *   tmr - obtained via sdcard_OpenTimer()
 * result
 *   none
 */
void sdcard_CloseTimer(struct IORequest *tmr);

/*
 * sdcard_Wait
 *   wait for a period of time or a signal
 * params
 *   tmr - obtained via sdcard_OpenTimer()
 *   secs - number of seconds to wait
 *   micro - number of microseconds to wait
 *   sigs - additionally - signal to wait for
 * result
 *   ULONG signals - if caught before timeout
 */
ULONG sdcard_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs);

/*
 * sdcard_WaitNano
 *   waits for (pretty much) specified amount of time. benchmarked.
 * params
 *   ns - amount of nanoseconds; 
 * result
 *   none
 * note
 *   rounds up ns to nearest multiple of 100
 */
void sdcard_WaitNano(ULONG ns, struct SDCardBase *SDCardBase);

