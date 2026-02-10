/* $Id$
 *
 *      gettimeofday.c - get time of the day
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/param.h>
#include <sys/time.h>

/****** net.lib/gettimeofday *********************************************
 
    NAME   
        gettimeofday - get date and time 
 
    SYNOPSIS
        #include <sys/time.h>
 
        error = gettimeofday(tp, tzp)
 
        int gettimeofday(struct timeval *, struct timezone *)
 
    FUNCTION
        The system's notion of the current Greenwich time and the
        current time zone is obtained with the gettimeofday() call.
        The time is expressed in seconds and microseconds since
        midnight (0 hour), January 1, 1970.  The resolution of the
        system clock is hardware dependent. If tzp is zero, the time
        zone information will not be returned. Also, if your system
        software is unable to provide time zone information, the
        structure pointed by tzp will be filled with zeroes.
   
    PORTABILITY
        UNIX
 
    INPUTS
        The structures pointed to by tp and tzp are defined in
        <sys/time.h> as:
   
             struct timeval {
                  long tv_sec;      \* seconds since Jan. 1, 1970 *\
                  long tv_usec;     \* and microseconds *\
             };
   
             struct timezone {
                  int  tz_minuteswest;   \* of Greenwich *\
                  int  tz_dsttime;  \* type of dst correction to apply *\
             };
   
        The timezone structure indicates the local time zone (meas-
        ured in minutes of time westward from Greenwich), and a flag
        that, if nonzero, indicates that Daylight Saving time
        applies locally during the appropriate part of the year.
 
    RESULT
        Returns 0 when successful and -1 with specific error code in 
        errno in case of an error. No error codes are specified,
        however.
        
    NOTES
        gettimeofday() uses GetSysTime() function of the timer.device,
        which is new to V36 of the device.
 
        Time zone information is taken from the locale.library, if it
        is available (it is included in all Amiga systems from 2.1 and
        up). Otherwise the environment variable "TZ" is consulted. If
        it fails, the time zone is initialized to the GMT.
 
        Global variable TimerBase _must_ be initialized before
        gettimeofday() is called. This is normally done automatically
        by the autoinit module (timerinit.c) included in the net.lib.
 
    SEE ALSO
        timer.device/GetSysTime()
*****************************************************************************
*
*/

/*
 * See timerinit.c for comments on these
 */
extern struct timezone __time_zone;
extern long __local_to_GMT;

int 
gettimeofday(struct timeval *tp, struct timezone *tzp)
{
  if (tp) {
    GetSysTime(tp);
    tp->tv_sec += __local_to_GMT;
  }
  if (tzp) {
    /*
     * __time_zone is set up in the timerinit.c
     */
    *tzp = __time_zone;
  }

  return 0;
}
