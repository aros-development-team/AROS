#ifndef DOS_DATETIME_H
#define DOS_DATETIME_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Date and time in dos.library
    Lang: english
*/
#ifndef DOS_DOS_H
#   include "dos/dos.h"
#endif

/* Data structure used by StrtoDate() and DatetoStr() */
struct DateTime
{
    struct DateStamp dat_Stamp;   /* DOS DateStamp */
    UBYTE	     dat_Format;  /* Controls appearance of dat_StrDate */
    UBYTE	     dat_Flags;   /* See DTF_* below */
    UBYTE	   * dat_StrDay;  /* Day of the week string */
    UBYTE	   * dat_StrDate; /* Date string */
    UBYTE	   * dat_StrTime; /* Time string */
};

/* You need this much room for each of the DateTime strings: */
#define LEN_DATSTRING	16

/* Flags for dat_Flags */
#define DTB_SUBST	0   /* Substitute Today, Tomorrow, etc. if possible. */
#define DTF_SUBST	1
#define DTB_FUTURE	1   /* Day of the week is in future. */
#define DTF_FUTURE	2

/* date format values */
#define FORMAT_DOS	0		/* dd-mmm-yy */
#define FORMAT_INT	1		/* yy-mm-dd */
#define FORMAT_USA	2		/* mm-dd-yy */
#define FORMAT_CDN	3		/* dd-mm-yy */
#define FORMAT_DEF	4		/* Current locale */
#define FORMAT_MAX	FORMAT_CDN

#endif /* DOS_DATETIME_H */
