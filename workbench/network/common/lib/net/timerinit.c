/* $Id$
 *
 *      timerinit.c - SAS C auto initialization functions for timer device
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

/****** net.lib/autoinit_timer.device *****************************************

    NAME
        timerinit - SAS C Autoinitialization Functions for timer.device

    SYNOPSIS
        #include <time.h>
    
        int daylight;
        long timezone;
        char *tzname[2];

        void tzset(void);

        #include <sys/time.h>

        struct Device *TimerBase;

        LONG _STI_200_openTimer(void);
        void _STD_200_closeTimer(void);
        
    FUNCTION
        These functions open and close the timer.device at the startup and
        exit of the program, respectively. For a program to use these
        functions, it must be linked with netlib:net.lib.

        The opened device base is stored in the TimerBase global variable.

        If the device can be opened, the _STIopenTimer() sets up the time zone
        information, which is used by the gettimeofday() function and the time
        conversion routines of the C-library.

    NOTES
        The time zone information is got from the environment variable named
        TZ. The format for this variable is:

            zzznnnddd

        where zzz is three letter identifier for the time zone (for example
        GMT), and the nnn is hours west from Greenwich on range [-23,24]
        (negative values are to east).  The last field is the abbreviation for
        the local daylight saving time zone (which is not interpreted by this
        version).

        If the TZ environment variable cannot be found, Greenwich Mean Time
        (GMT) is used instead.

        The autoinitialization and autotermination functions are features
        specific to the SAS C6.  However, these functions can be used with
        other (ANSI) C compilers, too.  Example follows:

        \* at start of main() *\

        atexit(_STD_200_closeTimer);
        _STI_200_openTimer();

	The tzset() does nothing. All the necessary initialization is done at
        the autoinit function.

    BUGS
        TZ "hours west from GMT" should be interpreted as float.

        The same autoinitialization won't work for both SAS C 6.3 and SAS C
        6.50 or latter.  Only way to terminate an initialization function is
        by exit() call with SAS C 6.3 binary.  If an autoinitialization
        function is terminated by exit() call with SAS C 6.50 binary, the
        autotermination functions won't be called.  Due this braindamage
        these compilers require separate net.lib libraries.

    SEE ALSO
        net.lib/gettimeofday(),
        SAS/C 6 User's Guide p. 145 for details of autoinitialization and
        autotermination functions.
*****************************************************************************
*
*/

#include <exec/types.h>
#include <exec/devices.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdlib.h>
#include <sys/time.h>

#include <time.h>

/* SAS C 6.50 kludge */
#if __VERSION__ > 6 || __REVISION__ >= 50
#define exit(x) return(x)
#endif

struct Device *TimerBase = 0L;
static void *unit;

/*
 * Time zone support for the gettimeofday. Zeroes default to the GMT
 * without daylight saving.
 */
struct timezone __time_zone = {0,0};

/*
 * Seconds to to the system time (seconds from 00:00 1.1.1978) 
 * to the GMT (seconds from 00:00 1.1.1970).
 * _STIopenTimer() adds the local time seconds west from GMT to this
 * value, so the local time gets converted to the GMT.
 */
long __local_to_GMT = ((8L*365 + 8/4)*24*60*60);

int  __daylight = 0;
long __timezone = 0;
char *__tzname[2] = { 0 };
char *_TZ = NULL;

char __zone_string[12] = "GMT0";

/* 
 * Locale information is included here, since the 2.1 includes may be hard 
 * to find.
 */

/* This structure must only be allocated by locale.library and is READ-ONLY! */
struct Locale
{
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

LONG __stdargs
_STI_200_openTimer(void)
{
  struct timerequest dummyTimer = { 0 };

  if (!TimerBase && !OpenDevice("timer.device", UNIT_VBLANK, 
				(struct IORequest *)&dummyTimer, 0L)) {
    TimerBase = dummyTimer.tr_node.io_Device;
    unit = dummyTimer.tr_node.io_Unit;
    if (TimerBase->dd_Library.lib_Version >= 36) {
      /*
       * Initialize time zone information for the gettimeofday()
       * First try to open locale (2.1 and up), and if that fails,
       * try to read environment variable TZ.
       */
      void *LocaleBase;
      struct Locale *thisLocale = NULL;
      short dstoff = 0;

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
	short len, i;
	long value;
	BPTR file = Open("ENV:TZ", MODE_OLDFILE);
	if (file) {
	  len = Read(file, __zone_string, sizeof(__zone_string) - 1);
	  if (len > 3) {
	    /*
	     * make sure the string is 0 terminated and does not have the
	     * newline at the end
	     */
	    for (i = 0; i < len; i++)
	      if (__zone_string[i] < ' ')
		break;
	    __zone_string[i] = '\0';

	    /* should interpret floats as well! */
	    if ((dstoff = StrToLong(__zone_string+3, &value)) > 0) {
	      /*
	       * Update time zone minutes west from GMT.
	       */
	      __time_zone.tz_minuteswest = (short)value * (short)60;
	      /*
	       * Set the offset to the possible DST zone name
	       */
	      dstoff += 3;
	    }
	  }
	  Close(file);
	}
      }

      /*
       * Update local time seconds to GMT translation
       */
      __timezone = (short)__time_zone.tz_minuteswest * (short)60;
      __local_to_GMT += __timezone;

      /*
       * tzset() stuff
       */
      if (dstoff > 3) {
	__daylight = 1;
      }
      else
	dstoff = 3;

      __zone_string[3] = '\0'; /* terminate time zone name */
      __tzname[0] = __zone_string;
      __tzname[1] = __zone_string + dstoff;

      return 0;
    }
  }
  exit(20);
}

void __stdargs
_STD_200_closeTimer(void)
{
  struct timerequest dummyTimer = { 0 };
  if (!TimerBase)
    return;

  dummyTimer.tr_node.io_Device = TimerBase;
  dummyTimer.tr_node.io_Unit = unit;
  CloseDevice((struct IORequest*)&dummyTimer);
}

void
tzset(void)
{
}
