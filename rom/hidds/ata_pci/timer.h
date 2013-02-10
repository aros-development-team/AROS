/*
    Copyright © 2009-2013, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

/*
 * ata_OpenTimer
 *   create timerequest to manage timed operations
 * result
 *   timerequest to be used with any of the calls below
 * note
 *   only one task can use given timerequest
 */
struct IORequest *ata_OpenTimer(void);

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
 */
void ata_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro);
