#include <exec/types.h>

#ifdef __MORPHOS__
#define CVa2i(__p0) \
	LP1(300, LONG , CVa2i, \
		STRPTR , __p0, a0, \
		, RexxSysBase, 0, 0, 0, 0, 0, 0)

#define CVi2az(__p0, __p1, __p2) \
	LP3(318, LONG , CVi2az, \
		STRPTR , __p0, a0, \
		LONG , __p1, d0, \
		LONG , __p2, d1, \
		, RexxSysBase, 0, 0, 0, 0, 0, 0)
#endif

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/errors.h>
#include <aros/asmcall.h>
#include "locale_intern.h"
#include <proto/locale.h>

#include <string.h>
#include <stdlib.h>


static LONG rxGetCatalogStr(struct LocaleBase *LocaleBase,
    struct Locale *locale, struct RexxMsg *rxmsg, STRPTR * resstr)
{
    *resstr =
        GetCatalogStr((struct Catalog *)CVa2i(rxmsg->rm_Args[1]),
        atol(rxmsg->rm_Args[2]), rxmsg->rm_Args[3]);
    return strlen(*resstr);
}

static LONG rxGetLocaleStr(struct LocaleBase *LocaleBase,
    struct Locale *locale, struct RexxMsg *rxmsg, STRPTR * resstr)
{
    *resstr = GetLocaleStr(locale, atol(rxmsg->rm_Args[1]));
    return strlen(*resstr);
}

static LONG rxOpenCatalog(struct LocaleBase *LocaleBase,
    struct Locale *locale, struct RexxMsg *rxmsg, STRPTR * resstr)
{
    APTR cat =
        OpenCatalog(locale, rxmsg->rm_Args[1], OC_BuiltInLanguage,
        (ULONG) rxmsg->rm_Args[2], OC_Version, atol(rxmsg->rm_Args[3]),
        TAG_DONE);
    return CVi2az(*resstr, (LONG) cat, 12);
}

static LONG rxCloseCatalog(struct LocaleBase *LocaleBase,
    struct Locale *locale, struct RexxMsg *rxmsg, STRPTR * resstr)
{
    CloseCatalog((struct Catalog *)CVa2i(rxmsg->rm_Args[1]));
    return 0;
}

static LONG rxStrnCmp(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    LONG reslen =
        StrnCmp(locale, rxmsg->rm_Args[1], rxmsg->rm_Args[2], -1,
        atol(rxmsg->rm_Args[3]));

    if (reslen < 0)
    {
        *resstr[0] = '-';
        *resstr[1] = '1';
        reslen = 2;
    }
    else if (reslen > 0)
    {
        *resstr[0] = '1';
        reslen = 1;
    }
    else
    {
        *resstr[0] = '0';
        reslen = 1;
    }

    return reslen;
}

static LONG rxConvToLower(struct LocaleBase *LocaleBase,
    struct Locale *locale, struct RexxMsg *rxmsg, STRPTR * resstr)
{
    *resstr[0] = (UBYTE) ConvToLower(locale, *rxmsg->rm_Args[1]);
    return 1;
}

static LONG rxConvToUpper(struct LocaleBase *LocaleBase,
    struct Locale *locale, struct RexxMsg *rxmsg, STRPTR * resstr)
{
    *resstr[0] = (UBYTE) ConvToUpper(locale, *rxmsg->rm_Args[1]);
    return 1;
}

static LONG rxIsAlpha(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsAlpha(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsSpace(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsSpace(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsDigit(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsDigit(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsGraph(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsGraph(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsAlNum(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsAlNum(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsCntrl(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsCntrl(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsLower(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsLower(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsPunct(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsPunct(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsUpper(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsUpper(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsPrint(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsPrint(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

static LONG rxIsXDigit(struct LocaleBase *LocaleBase, struct Locale *locale,
    struct RexxMsg *rxmsg, STRPTR * resstr)
{
    if (IsXDigit(locale, *rxmsg->rm_Args[1]))
        *resstr[0] = '1';
    else
        *resstr[0] = '0';
    return 1;
}

struct dispentry
{
    CONST_STRPTR FuncName;
      LONG(*Function) (struct LocaleBase *, struct Locale *,
        struct RexxMsg *, STRPTR *);
    ULONG NumArgs;
};

/* MUST be alphabetically sorted! */
struct dispentry disptable[] = {
    {"CLOSECATALOG", rxCloseCatalog, 1},
    {"CONVTOLOWER", rxConvToLower, 1},
    {"CONVTOUPPER", rxConvToUpper, 1},
    {"GETCATALOGSTR", rxGetCatalogStr, 3},
    {"GETLOCALESTR", rxGetLocaleStr, 1},        /*** NEW ***/
    {"ISALNUM", rxIsAlNum, 1},
    {"ISALPHA", rxIsAlpha, 1},
    {"ISCNTRL", rxIsCntrl, 1},
    {"ISDIGIT", rxIsDigit, 1},
    {"ISGRAPH", rxIsGraph, 1},
    {"ISLOWER", rxIsLower, 1},
    {"ISPRINT", rxIsPrint, 1},
    {"ISPUNCT", rxIsPunct, 1},
    {"ISSPACE", rxIsSpace, 1},
    {"ISUPPER", rxIsUpper, 1},
    {"ISXDIGIT", rxIsXDigit, 1},
    {"OPENCATALOG", rxOpenCatalog, 3},
    {"STRNCMP", rxStrnCmp, 3}
};

int dispcomp(const void *name, const void *dispentry)
{
    return stricmp(name, ((const struct dispentry *)dispentry)->FuncName);
}


/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, RexxHost,

/*  SYNOPSIS */
	AROS_LHA(struct RexxMsg *, rxmsg, A0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 5, Locale)

/*  FUNCTION
	locale.library rexxhost interface

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Locale *locale =
        (struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale;
    struct dispentry *dispentry;
    LONG reslen;
    UBYTE resbuf[12];
    STRPTR argstr = resbuf;

    if (!RexxSysBase)
        return (ERR10_014);
    if (!rxmsg || !IsRexxMsg(rxmsg) || !(rxmsg->rm_Action & RXFUNC)
        || !rxmsg->rm_Args[0])
        return (ERR10_010);

    if (!(dispentry =
            bsearch(rxmsg->rm_Args[0], disptable,
                sizeof(disptable) / sizeof(struct dispentry),
                sizeof(struct dispentry), dispcomp)))
        return (ERR10_001);
    if ((rxmsg->rm_Action & RXARGMASK) != dispentry->NumArgs)
        return (ERR10_017);

    reslen = dispentry->Function(LocaleBase, locale, rxmsg, &argstr);

    if (!(argstr = CreateArgstring(argstr, reslen)))
        return (ERR10_003);

#ifdef __MORPHOS__
    REG_A0 = (ULONG) argstr;
#else
#error register a0 must be set to argstr before returning...
#endif

    return 0;

    AROS_LIBFUNC_EXIT
}
