/*
    Copyright © 2009, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * PARTIAL CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2005-03-05  T. Wiszkowski       created file; initial benchmarked nanowait and timer-based micro/sec wait
 */

/*
 * ahci_OpenTimer
 *   create timerequest to manage timed operations
 * result
 *   timerequest to be used with any of the calls below
 * note
 *   only one task can use given timerequest
 */
struct IORequest *ahci_OpenTimer();

/*
 * ahci_CloseTimer
 *   dispose timerequest; most likely never used ;)
 * params
 *   tmr - obtained via ahci_OpenTimer()
 * result
 *   none
 */
void ahci_CloseTimer(struct IORequest *tmr);

/*
 * ahci_Wait
 *   wait for a period of time or a signal
 * params
 *   tmr - obtained via ahci_OpenTimer()
 *   secs - number of seconds to wait
 *   micro - number of microseconds to wait
 *   sigs - additionally - signal to wait for
 * result
 *   ULONG signals - if caught before timeout
 */
ULONG ahci_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs);

/*
 * ahci_WaitNano
 *   waits for (pretty much) specified amount of time. benchmarked.
 * params
 *   ns - amount of nanoseconds; 
 * result
 *   none
 * note
 *   rounds up ns to nearest multiple of 100
 */
void ahci_WaitNano(ULONG ns);

