/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

struct ataBase;

/*
 * ata_OpenTimer
 *   create timerequest to manage timed operations
 * result
 *   timerequest to be used with any of the calls below
 * note
 *   only one task can use given timerequest
 */
struct IORequest *ata_OpenTimer(struct ataBase *base);

/*
 * ata_CloseTimer
 *   dispose timerequest; most likely never used ;)
 * params
 *   tmr - obtained via ata_OpenTimer()
 * result
 *   none
 */
void ata_CloseTimer(struct IORequest *tmr);

/*
 * ata_Wait
 *   wait for a period of time or a signal
 * params
 *   tmr - obtained via ata_OpenTimer()
 *   secs - number of seconds to wait
 *   micro - number of microseconds to wait
 *   sigs - additionally - signal to wait for
 * result
 *   ULONG signals - if caught before timeout
 */
ULONG ata_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs);

/*
 * ata_WaitNano
 *   waits for (pretty much) specified amount of time. benchmarked.
 * params
 *   ns - amount of nanoseconds; 
 * result
 *   none
 * note
 *   rounds up ns to nearest multiple of 100
 */
void ata_WaitNano(ULONG ns, struct ataBase *base);

