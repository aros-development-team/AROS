/*
 * time.c --- GMT time functions
 *
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <proto/timer.h>

#include "config.h"
#include "base.h"

#include <proto/usergroup.h>

/*
 * Time zone support for the gettimeofday. Zeroes default to the GMT
 * without daylight saving.
 */
static struct timezone __time_zone = {0,0};

/*
 * Seconds to to the system time (seconds from 00:00 1.1.1978)
 * to the GMT (seconds from 00:00 1.1.1970).
 * _STIopenTimer() adds the local time seconds west from GMT to this
 * value, so the local time gets converted to the GMT.
 */
#define AMIGA_OFFSET ((8L*365 + 8/4)*24*60*60)
static long __local_to_GMT = AMIGA_OFFSET;

static struct timerequest timereq[1] = { 0 };

#if !defined(__AROS__)
struct Library *TimerBase;
/* This structure must only be allocated by locale.library and is READ-ONLY! */
struct Locale {
    STRPTR	loc_LocaleName;	  /* locale's name		 */
    STRPTR	loc_LanguageName;	  /* language of this locale	 */
    STRPTR	loc_PrefLanguages[10];	  /* preferred languages	 */
    ULONG	loc_Flags;		  /* always 0 for now		 */

    ULONG	loc_CodeSet;		  /* always 0 for now		 */
    ULONG	loc_CountryCode;	  /* user's country code	 */
    ULONG	loc_TelephoneCode;	  /* country's telephone code	 */
    LONG	loc_GMTOffset;		  /* minutes from GMT		 */

    /* deleted the rest to save space */
};
void CloseLocale( struct Locale *locale );
struct Locale *OpenLocale( STRPTR name );
#pragma libcall LocaleBase CloseLocale 2A 801
#pragma libcall LocaleBase OpenLocale 9C 801
#else
struct Device *TimerBase;
#endif

int TimeInit(struct Library *ugBase)
{
    D(bug("[UserGroup] %s()\n", __func__));

    if (OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)timereq, 0)) {
        TimerBase = NULL;
        return -1;
    } else {
#if !defined(__AROS__)
        TimerBase = (struct Library*)timereq->tr_node.io_Device;
        if (TimerBase->lib_Version >= 36)
#else
        TimerBase = timereq->tr_node.io_Device;
        if (TimerBase->dd_Library.lib_Version >= 36)
#endif
        {
            /*
             * Initialize time zone information for the gettimeofday()
             * First try to open locale (2.1 and up), and if that fails,
             * try to read environment variable TZ.
             */
            void *LocaleBase;
            struct Locale *thisLocale = NULL;

            if ((LocaleBase = OpenLibrary("locale.library", 38)) != NULL) {
                if ((thisLocale = OpenLocale(NULL)) != NULL) {
                    /*
                     * Update time zone minutes west from GMT.
                     */
                    __time_zone.tz_minuteswest = thisLocale->loc_GMTOffset;
                    CloseLocale(thisLocale);
                }
                CloseLibrary(LocaleBase);
            }
            if (!thisLocale) { /* if locale information was not available */
                short len;
                LONG value;
                char zone[10];

                BPTR file = Open("ENV:TZ", MODE_OLDFILE);
                if (file) {
                    len = Read(file, zone, sizeof(zone));
                    if (len > 3) {
                        zone[len] = '\000';
                        /* should interpret floats as well! */
                        if (StrToLong(zone+3, &value) > 0) {
                            /*
                             * Update time zone minutes west from GMT.
                             */
                            __time_zone.tz_minuteswest = (short)value * (short)60;
                        }
                    }
                    Close(file);
                }
            }

            /*
             * Update local time seconds to GMT translation
             */
            __local_to_GMT += (short)__time_zone.tz_minuteswest * (short)60;

            return 0;
        }

        return -1;
    }
}

void TimeCleanup(struct Library *ugBase)
{
    D(bug("[UserGroup] %s()\n", __func__));

    if (TimerBase) {
        CloseDevice((struct IORequest *)timereq);
        TimerBase = NULL;
    }
}

/****i* usergroup.library/gettimeofday *************************************

    NAME
        gettimeofday - get date and time

    SYNOPSIS
        #include <sys/time.h>

        success = gettimeofday(tvp, tzp)
          D0                   A0   A1

        int gettimeofday(struct timeval *, struct timezone *)

    DESCRIPTION
        The system's notion of the current Greenwich time and the current
        time zone is obtained with the gettimeofday() call.  The time is
        expressed in seconds and microseconds since midnight (0 hour),
        January 1, 1970.  The resolution of the system clock is hardware
        dependent, and the time may be updated continuously or in `ticks.'
        If tp or tzp is NULL, the associated time information will not be
        returned or set.

        The structures pointed to by tp and tzp are defined in <sys/time.h>
        as:

        struct timeval {
                long    tv_sec;         \* seconds since Jan. 1, 1970 *\
                long    tv_usec;        \* and microseconds *\
        };

        struct timezone {
                int     tz_minuteswest; \* of Greenwich *\
                int     tz_dsttime;     \* type of dst correction to apply *\
        };

        The timezone structure indicates the local time zone (measured in
        minutes of time westward from Greenwich), and a flag that, if
        nonzero, indicates that Daylight Saving time applies locally during
        the appropriate part of the year.

    RETURN
        A 0 return value indicates that the call succeeded.  A -1 return
        value indicates an error occurred, and in this case an error code is
        available by ug_GetErr() call and it may be stored into the global
        variable errno.

    ERRORS
        The following error codes may be available by ug_GetErr() or stored
        in errno:

        [EFAULT]  An argument address referenced invalid memory.

****************************************************************************
*/

AROS_LH2I(int, gettimeofday,
          AROS_LHA(struct timeval *, tvp, A0),
          AROS_LHA(struct timezone *, tzp, A1),
          struct Library *, UserGroupBase, 45, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    if (tvp) {
        GetSysTime(tvp);
        tvp->tv_sec += __local_to_GMT;
    }
    if (tzp)
        *tzp = __time_zone;

    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH2I(int, settimeofday,
          AROS_LHA(struct timeval *, tvp, A0),
          AROS_LHA(struct timezone *, tzp, A1),
          struct Library *, UserGroupBase, 46, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    return -1;

    AROS_LIBFUNC_EXIT
}
