/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: shqipja.language description file.
    Lang: english
*/

/*  Language file for the Albanian language. */

#include <exec/types.h>
#include <aros/system.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <libraries/locale.h>

#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <aros/debug.h>

#define LANGSTR     "shqipja"   /* String version of above */
#define LANGVER     41          /* Version number of language */
#define LANGREV     0           /* Revision number of language */
#define LANGTAG     "\0$VER: "LANGSTR".language 41.0 (14.08.2010)"

STRPTR AROS_SLIB_ENTRY(getlangstring,language)();

/* ----------------------------------------------------------------------- */

/* Bit masks for locale .language functions. Only implement GetString() */
#define LF_GetLangStr       (1L << 3)

/* Arrays for Italian character type/conversion */
extern const STRPTR __shqipja_strings[];

/* -------------------------------------------------------------------------
   Library definition, you should not need to change any of this.
 ------------------------------------------------------------------------- */

struct Language
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR seglist;
};

extern const UBYTE name[];
extern const UBYTE version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern struct Language *AROS_SLIB_ENTRY(init,language)();
extern struct Language *AROS_SLIB_ENTRY(open,language)();
extern BPTR AROS_SLIB_ENTRY(close,language)();
extern BPTR AROS_SLIB_ENTRY(expunge,language)();
extern int AROS_SLIB_ENTRY(null,language)();
extern ULONG AROS_SLIB_ENTRY(mask,language)();
extern const char end;

int entry(void)
{
    return -1;
}

const struct Resident languageTag =
{
    RTC_MATCHWORD,
    (struct Resident *)&languageTag,
    (APTR)&end,
    RTF_AUTOINIT,
    LANGVER,
    NT_LIBRARY,
    -120,
    (STRPTR)name,
    (STRPTR)&version[7],
    (ULONG *)inittabl
};

const UBYTE name[]=LANGSTR ".language";
const UBYTE version[]=LANGTAG;

const ULONG datatable = 0;

const APTR inittabl[4] =
{
    (APTR)sizeof(struct Language),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,language)
};

struct ExecBase *mySysBase;

AROS_UFH3(struct Language *, AROS_SLIB_ENTRY(init,language),
    AROS_UFHA(struct Language *, language, D0),
    AROS_UFHA(BPTR,             segList, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    /*
	You could just as easily do this bit as the InitResident()
	datatable, however this works just as well.
    */
    language->library.lib_Node.ln_Type = NT_LIBRARY;
    language->library.lib_Node.ln_Pri = -120;
    language->library.lib_Node.ln_Name = (char *)name;
    language->library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    language->library.lib_Version = LANGVER;
    language->library.lib_Revision = LANGREV;
    language->library.lib_IdString = (APTR)&version[7];

    language->seglist = segList;
    language->sysbase = SysBase;
    mySysBase = SysBase;

    /*
	Although it is unlikely, you would return NULL if you for some
	unknown reason couldn't open.
    */
    bug("GetLangStr: Loaded at address %p\n", &AROS_SLIB_ENTRY(getlangstring,language));
    return language;

    AROS_USERFUNC_EXIT
}

#define SysBase language->sysbase

AROS_LH1(struct Language *, open,
    AROS_LHA(ULONG, version, D0),
    struct Language *, language, 1, language)
{
    AROS_LIBFUNC_INIT
    language->library.lib_OpenCnt++;
    language->library.lib_Flags &= ~LIBF_DELEXP;

    /* Again return NULL if you could not open */
    return language;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct Language *, language, 2, language)
{
    AROS_LIBFUNC_INIT

    if(! --language->library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(language->library.lib_Flags & LIBF_DELEXP)
	{
	    /* Yes, expunge the library */
	    return AROS_LC0(BPTR, expunge, struct Language *, language, 3, language);
	}
    }
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct Language *, language, 3, language)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    if(language->library.lib_OpenCnt)
    {
	/* Can't expunge, we are still open */
	language->library.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    Remove(&language->library.lib_Node);
    ret = language->seglist;

    FreeMem((UBYTE *)language - language->library.lib_NegSize,
	    language->library.lib_PosSize + language->library.lib_NegSize);

    return ret;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct Language *, language, 4, language)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------------
   Language specific functions
 ------------------------------------------------------------------------ */

#undef SysBase
#define SysBase	mySysBase

/* ULONG LanguageMask():
    This function is to inform locale.library what functions it should
    use from this library. This is done by returning a bitmask containing
    1's for functions to use, and 0's for functions to ignore.

    Unused bits MUST be 0 for future compatibility.
*/
AROS_LH0(ULONG, mask, struct Language *, language, 5, language)
{
    AROS_LIBFUNC_INIT

    return ( LF_GetLangStr );

    AROS_LIBFUNC_EXIT
}

/* STRPTR GetLangString(ULONG num): Language function 3
    This function is called by GetLocaleStr() and should return
    the string matching the string id passed in as num.
*/
AROS_LH1(STRPTR, getlangstring,
    AROS_LHA(ULONG, id, D0),
    struct LocaleBase *, LocaleBase, 9, language)
{
    AROS_LIBFUNC_INIT

    //kprintf("\nWe have got to getlangstring\n");

    if(id < MAXSTRMSG)
	return __shqipja_strings[id];
    else
	return NULL;

    AROS_LIBFUNC_EXIT
}

/* -----------------------------------------------------------------------
   Library function table - you will need to alter this
   I have this right here at the end of the library so that I do not
   have to have prototypes for the functions. Although you could do that.
   ----------------------------------------------------------------------- */

void *const functable[] =
{
    &AROS_SLIB_ENTRY(open,language),
    &AROS_SLIB_ENTRY(close,language),
    &AROS_SLIB_ENTRY(expunge,language),
    &AROS_SLIB_ENTRY(null,language),
    &AROS_SLIB_ENTRY(mask,language),

    /* Note, shorter function table, as only getlangstring is used */

    /* 0 - 3 */
    &AROS_SLIB_ENTRY(null, language),
    &AROS_SLIB_ENTRY(null, language),
    &AROS_SLIB_ENTRY(null, language),    
    &AROS_SLIB_ENTRY(getlangstring, language),
    (void *)-1
};

/*
    Note how only the required data structures are kept...

    This is the list of strings. It is an array of pointers to strings,
    although how it is laid out is implementation dependant.
*/
const STRPTR __shqipja_strings[] =
{
    /* A blank string */
    "",

    /*  The days of the week. Starts with the first day of the week.
	In English this would be Sunday, this depends upon the settings
	of Locale->CalendarType.
    */

#warning: stegerg: I think this must always start with Sunday and not what comment above says

    "e diel", "e h�n�", "e mart�", "e m�rkur�", "e enjte",
    "e premte", "e shtun�",

    /* Abbreviated days of the week */
    "Die", "H�n", "Mar", "M�r", "Enj", "Pre", "Sht",

    /* Months of the year */
    "janar", "shkurt", "mars",
    "prill", "maj", "qershor",
    "korrik", "gusht", "shtator",
    "tetor", "n�ntor", "dhjetor",

    /* Abbreviated months of the year */
    "Jan", "Shk", "Mar", "Pri", "Maj", "Qer",
    "Kor", "Gus", "Sht", "Tet", "N�n", "Dhj",

    "Po", /* Yes, affirmative response */
    "Jo", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "PD", "MD",

    /* Soft and hard hyphens */
    "\xAD", "-",

    /* Open and close quotes */
    "\"", "\"",

    /* Days: But not actual day names
       Yesterday - the day before the current
       Today - the current day
       Tomorrow - the next day
       Future.
    */
    "Dje", "Sot", "Nes�r", "Ardhmen"
};

/* This is the end of ROMtag marker. */
const char end=0;
