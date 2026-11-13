#ifndef LIBRARIES_LOCALE_H
#define LIBRARIES_LOCALE_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for locale.library
    Lang: english
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

struct LocaleBase
{
    struct Library lb_LibNode;
    BOOL           lb_SysPatches;
};

struct Locale
{
    STRPTR loc_LocaleName;
    STRPTR loc_LanguageName;
    STRPTR loc_PrefLanguages[10];
    ULONG  loc_Flags;
    ULONG  loc_CodeSet;

    ULONG  loc_CountryCode;
    ULONG  loc_TelephoneCode;
    LONG   loc_GMTOffset;       /* offset from local to GMT, positive for zones west of Greenwich */
    UBYTE  loc_MeasuringSystem; /* see below */
    UBYTE  loc_CalendarType;    /* see below */
    UBYTE  loc_Reserved0[2];

    STRPTR loc_DateTimeFormat;
    STRPTR loc_DateFormat;
    STRPTR loc_TimeFormat;
    STRPTR loc_ShortDateTimeFormat;
    STRPTR loc_ShortDateFormat;
    STRPTR loc_ShortTimeFormat;

    STRPTR  loc_DecimalPoint;
    STRPTR  loc_GroupSeparator;
    STRPTR  loc_FracGroupSeparator;
    UBYTE * loc_Grouping;
    UBYTE * loc_FracGrouping;
    STRPTR  loc_MonDecimalPoint;
    STRPTR  loc_MonGroupSeparator;
    STRPTR  loc_MonFracGroupSeparator;
    UBYTE * loc_MonGrouping;
    UBYTE * loc_MonFracGrouping;

    UBYTE loc_MonFracDigits;
    UBYTE loc_MonIntFracDigits;
    UBYTE loc_Reserved1[2];

    STRPTR loc_MonCS;
    STRPTR loc_MonSmallCS;
    STRPTR loc_MonIntCS;

    STRPTR loc_MonPositiveSign;
    UBYTE  loc_MonPositiveSpaceSep; /* see below */
    UBYTE  loc_MonPositiveSignPos;  /* see below */
    UBYTE  loc_MonPositiveCSPos;    /* see below */
    UBYTE  loc_Reserved2;
    STRPTR loc_MonNegativeSign;
    UBYTE  loc_MonNegativeSpaceSep; /* see below */
    UBYTE  loc_MonNegativeSignPos;  /* see below */
    UBYTE  loc_MonNegativeCSPos;    /* see below */
    UBYTE  loc_Reserved3;
};

/* loc_Flags, AROS-specific */
#define LOCF_GMT_CLOCK (1UL << 16)	/* Hardware clock stores GMT */

/* loc_MeasuringSystem */
#define MS_ISO      0
#define MS_AMERICAN 1
#define MS_IMPERIAL 2
#define MS_BRITISH  3

/* loc_CalendarType */
#define CT_7SUN 0
#define CT_7MON 1
#define CT_7TUE 2
#define CT_7WED 3
#define CT_7THU 4
#define CT_7FRI 5
#define CT_7SAT 6

/* loc_MonPositiveSpaceSep and loc_MonNegativeSpaceSep */
#define SS_NOSPACE 0
#define SS_SPACE   1

/* loc_MonPositiveSignPos and loc_MonNegativeSignPos */
#define SP_PARENS    0
#define SP_PREC_ALL  1
#define SP_SUCC_ALL  2
#define SP_PREC_CURR 3
#define SP_SUCC_CURR 4

/* loc_MonPositiveCSPos and loc_MonNegativeCSPos */
#define CSP_PRECEDES 0
#define CSP_SUCCEEDS 1

                       /* OpenCatalog() */

#define OC_TagBase         (TAG_USER + 0x90000)
#define OC_BuiltInLanguage (OC_TagBase + 1)
#define OC_BuiltInCodeSet  (OC_TagBase + 2)
#define OC_Version         (OC_TagBase + 3)
#define OC_Language        (OC_TagBase + 4)

                        /* StrnCmp() */

#define SC_ASCII    0
#define SC_COLLATE1 1
#define SC_COLLATE2 2

                 /* Internal String-Numbers */

// Include the common definitions
#include <libraries/localestd.h>

#define SOFTHYPHEN 43
#define HARDHYPHEN 44

#define OPENQUOTE  45
#define CLOSEQUOTE 46

#define YESTERDAYSTR 47
#define TODAYSTR     48
#define TOMORROWSTR  49
#define FUTURESTR    50

#define LANG_NAME   51  /* V50 */

#define MAXSTRMSG 52

struct Catalog
{
    struct Node cat_Link;

    UWORD  cat_Pad;
    STRPTR cat_Language;
    ULONG  cat_CodeSet;
    UWORD  cat_Version;
    UWORD  cat_Revision;
};

#endif /* LIBRARIES_LOCALE_H */
