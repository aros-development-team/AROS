#ifndef _STDC_LOCALE_H_
#define _STDC_LOCALE_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file locale.h
*/

#include <aros/system.h>
#include <aros/types/locale_t.h>

/*
    struct lconv contains members relating to the formatting of numerical
    values. The comments show the default values, ie those used by the "C"
    locale.
*/
struct lconv
{
    char *decimal_point;		/* "." */
    char *thousands_sep;		/* "" */
    char *grouping;			/* "" */
    char *mon_decimal_point;		/* "" */
    char *mon_thousands_sep;		/* "" */
    char *mon_grouping;			/* "" */
    char *positive_sign;		/* "" */
    char *negative_sign;		/* "" */
    char *currency_symbol;		/* "" */
    char frac_digits;			/* CHAR_MAX */
    char p_cs_precedes;			/* CHAR_MAX */
    char n_cs_precedes;			/* CHAR_MAX */
    char p_sep_by_space;		/* CHAR_MAX */
    char n_sep_by_space;		/* CHAR_MAX */
    char p_sign_posn;			/* CHAR_MAX */
    char n_sign_posn;			/* CHAR_MAX */
    char *int_curr_symbol;		/* "" */
    char int_frac_digits;		/* CHAR_MAX */
    char int_p_cs_precedes;		/* CHAR_MAX */
    char int_n_cs_precedes;		/* CHAR_MAX */
    char int_p_sep_by_space;		/* CHAR_MAX */
    char int_n_sep_by_space;		/* CHAR_MAX */
    char int_p_sign_posn;		/* CHAR_MAX */
    char int_n_sign_posn;		/* CHAR_MAX */
};

#include <aros/types/null.h>

#define LC_ALL		    0
/* Affects string collation (e.g., strcoll(), wcscoll()) */
#define LC_COLLATE	    1
/* Character classification/conversion (isalpha(), toupper(), wctype(), etc.) */
#define LC_CTYPE	    2
/* Affects monetary formatting (localeconv()) */
#define LC_MONETARY	    3
/* Affects decimal point and thousands separator (localeconv(), numeric I/O) */
#define LC_NUMERIC	    4
/* Affects date/time formatting (strftime(), wcsftime()) */
#define LC_TIME		    5

typedef struct __locale *locale_t;

__BEGIN_DECLS

char *setlocale(int category, const char *locale);
struct lconv *localeconv(void);

__END_DECLS

#endif /* _STDC_LOCALE_H_ */
