/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 wctype linklib stubs.
*/

#include <aros/debug.h>

#include <aros/types/wctype_t.h>
#include <aros/types/wint_t.h>
#include <aros/types/mbstate_t.h>

#include <wchar.h>
#include <string.h>

#define STDC_NOINLINE_WCTYPE

/*****************************************************************************

    NAME */
#include <wctype.h>

        wctype_t wctype(

/*  SYNOPSIS */
        const char *prop)

/*  FUNCTION
        Returns a value identifying a wide character classification property,
        such as "alpha", "digit", or "upper", which can later be used with
        `iswctype()` to test characters.

    INPUTS
        prop - a null-terminated string naming a wide character class.
               Common names include: "alnum", "alpha", "blank", "cntrl",
               "digit", "graph", "lower", "print", "punct", "space",
               "upper", "xdigit".

    RESULT
        Returns a value of type wctype_t if the property name is recognized,
        or 0 if not.

    NOTES
        The returned wctype_t value is opaque and must only be used with
        `iswctype()`.

    EXAMPLE
        wctype_t type = wctype("alpha");
        if (iswctype(L'Z', type))
        {
           // is an alpha character
        }

    BUGS
        Only a fixed set of property names are recognized. No locale-specific
        names are currently supported.

    SEE ALSO
        iswctype(), towctrans(), wctrans()

    INTERNALS
        The implementation uses a small enumeration mapping known strings
        to predefined character class flags.

******************************************************************************/
{
    if (!prop) return 0;
    if (!strcmp(prop, "alnum"))   return _WCTYPE_ALNUM;
    if (!strcmp(prop, "alpha"))   return _WCTYPE_ALPHA;
    if (!strcmp(prop, "cntrl"))   return _WCTYPE_CNTRL;
    if (!strcmp(prop, "digit"))   return _WCTYPE_DIGIT;
    if (!strcmp(prop, "graph"))   return _WCTYPE_GRAPH;
    if (!strcmp(prop, "lower"))   return _WCTYPE_LOWER;
    if (!strcmp(prop, "print"))   return _WCTYPE_PRINT;
    if (!strcmp(prop, "punct"))   return _WCTYPE_PUNCT;
    if (!strcmp(prop, "space"))   return _WCTYPE_SPACE;
    if (!strcmp(prop, "upper"))   return _WCTYPE_UPPER;
    if (!strcmp(prop, "xdigit"))  return _WCTYPE_XDIGIT;
    return 0;
}

int iswalnum(wint_t wc)
{
    return iswctype(wc, _WCTYPE_ALNUM);
}

int iswalpha(wint_t wc)
{
    return iswctype(wc, _WCTYPE_ALPHA);
}

int iswcntrl(wint_t wc)
{
    return iswctype(wc, _WCTYPE_CNTRL);
}

int iswdigit(wint_t wc)
{
    return iswctype(wc, _WCTYPE_DIGIT);
}

int iswgraph(wint_t wc)
{
    return iswctype(wc, _WCTYPE_GRAPH);
}

int iswlower(wint_t wc)
{
    return iswctype(wc, _WCTYPE_LOWER);
}

int iswprint(wint_t wc)
{
    return iswctype(wc, _WCTYPE_PRINT);
}

int iswpunct(wint_t wc)
{
    return iswctype(wc, _WCTYPE_PUNCT);
}

int iswspace(wint_t wc)
{
    return iswctype(wc, _WCTYPE_SPACE);
}

int iswupper(wint_t wc)
{
    return iswctype(wc, _WCTYPE_UPPER);
}

int iswxdigit(wint_t wc)
{
    return iswctype(wc, _WCTYPE_XDIGIT);
}

int iswblank(wint_t wc)
{
    return iswctype(wc, _WCTYPE_BLANK);
}
