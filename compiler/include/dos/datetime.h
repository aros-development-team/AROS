#ifndef DOS_DATETIME_H
#define DOS_DATETIME_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Structures to handle date and time
    Lang: english
*/

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

/* DateStamp structure as defined in <dos/dos.h>. This structure
   sufficiently describes a system-date and -time (i.e. any date since
   1.1.1978).

   struct DateStamp
   {
      LONG ds_Days;   ** Number of days since 1.1.1978 **
      LONG ds_Minute; ** Number of minutes since midnight **
      LONG ds_Tick;   ** Number of ticks (1/50 second) in the current minute
                         Note that this may not be exact **
   };
*/


/* DateTime is used by StrToDate() and DateToStr(). It descibes not only
   a date, but supplies also pointers to strings describing the date in
   human-readable form.
*/
struct DateTime
{
    struct DateStamp dat_Stamp;   /* see above */
    UBYTE	     dat_Format;  /* Describes, which format the strings
                                     should have (see below) */
    UBYTE	     dat_Flags;   /* see below */
    /* The following pointers may be NULL under certain circumstances. */
    UBYTE	   * dat_StrDay;  /* Day of the week string */
    UBYTE	   * dat_StrDate; /* Date string */
    UBYTE	   * dat_StrTime; /* Time string */
};

/* You need this much room for each of the DateTime strings. */
#define LEN_DATSTRING 16

/* dat_Format */
#define FORMAT_DOS 0          /* DOS internal format, e.g. 21-Jan-78 */
#define FORMAT_INT 1          /* International format, e.g. 78-01-21 */
#define FORMAT_USA 2          /* US-American format, e.g. 01-21-78 */
#define FORMAT_CDN 3          /* Canadian format, e.g. 21-01-78 */
#define FORMAT_DEF 4          /* Format of current locale */
#define FORMAT_MAX FORMAT_CDN

/* dat_Flags */
#define DTB_SUBST  0 /* Substitute Today, Tomorrow, etc. if possible. */
#define DTB_FUTURE 1 /* Day of the week is in future. */
#define DTF_SUBST  (1<<DTB_SUBST)
#define DTF_FUTURE (1<<DTB_FUTURE)

#endif /* DOS_DATETIME_H */
