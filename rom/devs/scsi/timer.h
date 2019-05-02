/*
    Copyright © 2009-2013, The AROS Development Team. All rights reserved
    $Id: timer.h 46720 2013-02-28 18:48:18Z sonic $

    Desc:
    Lang: English
*/

struct scsiBase;

/*
 * scsi_OpenTimer
 *   create timerequest to manage timed operations
 * result
 *   timerequest to be used with any of the calls below
 * note
 *   only one task can use given timerequest
 */
struct IORequest *scsi_OpenTimer(struct scsiBase *base);

/*
 * scsi_CloseTimer
 *   dispose timerequest; most likely never used ;)
 * params
 *   tmr - obtained via scsi_OpenTimer()
 * result
 *   none
 */
void scsi_CloseTimer(struct IORequest *tmr);

/*
 * scsi_Wait
 *   wait for a period of time or a signal
 * params
 *   tmr - obtained via scsi_OpenTimer()
 *   secs - number of seconds to wait
 *   micro - number of microseconds to wait
 *   sigs - additionally - signal to wait for
 * result
 *   ULONG signals - if caught before timeout
 */
ULONG scsi_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs);

BOOL scsi_Calibrate(struct IORequest* tmr, struct scsiBase *base);

/*
 * scsi_WaitNano
 *   waits for (pretty much) specified amount of time. benchmarked.
 * params
 *   ns - amount of nanoseconds; 
 * result
 *   none
 * note
 *   rounds up ns to nearest multiple of 100
 */
void scsi_WaitNano(ULONG ns, struct scsiBase *base);

