/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: english.language description file.
    Lang: english
*/
/*
    Language description file for english.language.

    So what is this thing?
	Well basically its a template for you to write
	description files for your own language.

    How is it done?
	A language file is simply a shared library that
	the locale.library loads into memory. The language
	must define at least one function, which tells locale
	which other functions it defines. This function has an
	an LVO number of 5 (-30 on m68k/amiga). The function
	has to return a mask which has a bit set for each
	function that this library defines. Functions which
	this library does not define are filled in by the
	functions from the english.language definition
	(which is essentially what this file defines).

    What do I have to do?
	You have to write functions for all the locale
	functions which would be different for your
	language. Typically this would be function 3,
	GetLangString(), which returns the string
	returned by the locale.library function
	GetLocaleStr().

	There are some parts of this file which will also need to
	be altered (language names and versions), but these will
	be pointed out.

	NOTE: This file should ideally be self-contained,
	not relying upon any third party shared libraries.

	In fact it is impossible to make library function
	calls inside the locale support functions since you
	will actually have no library base to work with.
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <libraries/locale.h>

#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

/* -----------------------------------------------------------------------
  CHANGES:
    You will have to change all of these to suit your language,
    these will be used in many definitions below.
*/

#define LANGVER     41          /* Version number of language */
#define LANGREV     1           /* Revision number of language */
#define LANGTAG     "\0$VER: english.language 41.1 (20.2.1997)"

/* ----------------------------------------------------------------------- */

/* Bit masks for locale .language functions. */
#define LF_ConvToLower      (1L << 0)
#define LF_ConvToUpper      (1L << 1)
#define LF_Private2         (1L << 2)
#define LF_GetLangStr       (1L << 3)
#define LF_IsAlNum          (1L << 4)
#define LF_IsAlpha          (1L << 5)
#define LF_IsCntrl          (1L << 6)
#define LF_IsDigit          (1L << 7)
#define LF_IsGraph          (1L << 8)
#define LF_IsLower          (1L << 9)
#define LF_IsPrint          (1L << 10)
#define LF_IsPunct          (1L << 11)
#define LF_IsSpace          (1L << 12)
#define LF_IsUpper          (1L << 13)
#define LF_IsXDigit         (1L << 14)
#define LF_ToUpper          (1L << 15)
#define LF_StringComp       (1L << 16)

/* Arrays for english/Latin1 character type/conversion, defined later */
extern const UWORD __eng_ctype_array[];
extern const ULONG __eng_to_lower[];
extern const ULONG __eng_to_upper[];
extern const STRPTR __eng_strings[];
extern const ULONG __eng_collate_tab[];

/* We use these to indicate whether a character is a certain type in the
   character array.
*/
#define iAlpha  (1 << 0)
#define iCntrl  (1 << 1)
#define iDigit  (1 << 2)
#define iGraph  (1 << 3)
#define iLower  (1 << 4)
#define iPrint  (1 << 5)
#define iPunct  (1 << 6)
#define iSpace  (1 << 7)
#define iUpper  (1 << 8)
#define iXDigit (1 << 9)

/* -------------------------------------------------------------------------
   Library definition, you should not need to change any of this.
 ------------------------------------------------------------------------- */

struct Language
{
    struct Library library;
    APTR sysbase;
    BPTR seglist;
};

extern const UBYTE name[];
extern const UBYTE version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern const ULONG datatable;
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
    (STRPTR)&version[6],
    (ULONG *)inittabl
};

const UBYTE name[]=LANGSTR ".language";
const UBYTE version[]=LANGTAG;

const ULONG datatable = 0;

const APTR inittabl[4] =
{
    (APTR)sizeof(struct Language),
    (APTR)functable,
    (APTR)datatable,
    &AROS_SLIB_ENTRY(init,language)
};

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
    language->library.lib_IdString = (APTR)&version[6];

    language->seglist = segList;
    language->sysbase = SysBase;

    /*
	Although it is unlikely, you would return NULL if you for some
	unknown reason couldn't open.
    */

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
	return NULL;
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct Language *, language, 3, language)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = language->sysbase;

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

/* ULONG LanguageMask():
    This function is to inform locale.library what functions it should
    use from this library. This is done by returning a bitmask containing
    1's for functions to use, and 0's for functions to ignore.

    Unused bits MUST be 0 for future compatibility.
*/
AROS_LH0(ULONG, mask, struct Language *, language, 5, language)
{
    AROS_LIBFUNC_INIT

    /* CHANGES:
	This is where you list which functions that this language
	specifies. There are some bit masks which can be used for
	simplicity.

	Most languages will probably only need to implement
	LF_GetLangStr
    */
    return (  LF_ConvToLower | LF_ConvToUpper
	    | LF_GetLangStr
	    | LF_IsAlNum | LF_IsAlpha | LF_IsCntrl | LF_IsDigit
	    | LF_IsGraph | LF_IsLower | LF_IsPrint | LF_IsPunct
	    | LF_IsSpace | LF_IsXDigit
	    | LF_StringConv | LF_StringComp
	   );

    AROS_LIBFUNC_EXIT
}

/* ULONG ConvToLower(ULONG char): Language function 0
    This function converts the character char to the equivalent
    lower case value.
*/
AROS_LH1(ULONG, convtolower,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 6, language)
{
    AROS_LIBFUNC_INIT

    return __eng_to_lower[chr];

    AROS_LIBFUNC_EXIT
}

/* ULONG ConvToUpper(ULONG char): Language Function 1
    This function converts the character char to the equivalent
    upper case character.
*/
AROS_LH1(ULONG, convtoupper,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 7, language)
{
    AROS_LIBFUNC_INIT

    return __eng_to_upper[chr];

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

    if(id < MAXSTRMSG)
	return __eng_strings[id];
    else
	return NULL;

    AROS_LIBFUNC_EXIT
}

/* BOOL IsXXXXX(ULONG char): Language functions 4-14
    These function are the same as the ANSI C isxxxxx() functions,
    however these will pay extra attention to the current language.

    This gives the advantage of using different character sets,
    however I wouldn't recommend that, since you will have funny
    font problems.
*/

AROS_LH1(BOOL, isalnum,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 10, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(
      (__eng_ctype_array[chr] & iAlpha) || (__eng_ctype_array[chr] & iDigit)
      );

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, isalpha,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 11, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, iscntrl,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 12, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iCntrl);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, isdigit,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 13, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iDigit);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, isgraph,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 14, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iGraph);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, islower,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 15, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iLower);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, isprint,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 16, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iPrint);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, ispunct,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 17, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iPunct);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, isspace,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 18, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iSpace);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, isupper,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 19, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iUpper);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BOOL, isxdigit,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 20, language)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iXDigit);

    AROS_LIBFUNC_EXIT
}

/* ULONG strconvert(STRPTR s1, STRPTR s2, LONG len, ULONG typ): LF 15
    This function will convert a string to automatically use the
    collation table. This is a bit dodgy in my opinion, however the ANSI
    people didn't think so...

    For SC_ASCII and SC_COLLATE1 this is just convert the string as is.
    If you use SC_COLLATE2 this does SC_COLLATE1 encoding, the repeats
    the string as is...
*/
AROS_LH4(ULONG, strconvert,
    AROS_LHA(STRPTR,    string1, A1),
    AROS_LHA(STRPTR,    string2, A2),
    AROS_LHA(LONG,      length,  D0),
    AROS_LHA(ULONG,     type,    D1),
    struct LocaleBase *, LocaleBase, 21, english)
{
    AROS_LIBFUNC_INIT

    ULONG count = 0;

    if(type == SC_COLLATE2)
    {
	STRPTR origS1;

	while(--length && *string1)
	{
	    *string2++ = __eng_collate_tab[(UBYTE)*string1];
	    count++;
	}
	while(--length && *origS1)
	{
	    *string2++ = *origS1++;
	    count++;
	}
	*string2 = '\0';
    }
    else if((type == SC_COLLATE1) || (type == SC_ASCII))
    {
	STRPTR collTab;

	if(type == SC_ASCII)
	    collTab = __eng_to_upper;
	else
	    collTab = __eng_collate_tab;

	while(--length && *string1)
	{
	    *string2++ = collTab[ (UBYTE)*string1 ];
	    count++;
	}
        *string2 = '\0';
    }
    return count;

    AROS_LIBFUNC_EXIT
}

/* LONG strcompare(STRPTR s1, STRPTR s2, LONG len, ULONG typ): LF 16
    This function will do the comparison using either plain ASCII
    or the collation information. This is explained more in
    the data file, or in the autodoc...
*/
AROS_LH4(LONG, strcompare,
    AROS_LHA(STRPTR,    string1, A1),
    AROS_LHA(STRPTR,    string2, A2),
    AROS_LHA(LONG,      length,  D0),
    AROS_LHA(ULONG,     type,    D1),
    struct LocaleBase *, LocaleBase, 22, english)
{
    AROS_LIBFUNC_INIT
    ULONG a, b;

    if(type == SC_COLLATE2)
    {
	/* Collate 2, a bit more difficult */
	STRPTR origS1, origS2;

	origS1 = string1;
	origS2 = string2;
	do
	{
	    a = colltab[(UBYTE)*string1++];
	    b = colltab[(UBYTE)*string2++];
	} while( (a == b) && --length);

	/* If we reached the end, and everything is the same */
	if((a == 0) && (a == b))
	{
	    /* Compare again using strings as is... */
	    do
	    {
		a = *origS1++;
		b = *origS2++;
	    } while( (a == b) && --length);
	}
	return a - b;
    }
    else if((type == SC_COLLATE1) || (SC_ASCII))
    {
	ULONG *colltab;

	/* Determine which collation table to use... */
	if(type == SC_ASCII)
	    colltab = __eng_to_upper;
	else
	    colltab = __eng_collate_tab;

	do
	{
	    a = colltab[(UBYTE)*string1++];
	    b = colltab[(UBYTE)*string2++];
	} while( (a == b) && --length);
	return a - b;
    }

    /* Ha: Wrong arguments... */
    return 0;
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

    /*
	CHANGES:
	 This is where language specific functions must go.
	 If this language doesn't require a specific function
	 you must still define the entry. You should make this a
	 function which just returns 0. For the moment, using
	 &AROS_SLIB_ENTRY(null, language) will suffice, however
	 watch out if that function is ever given a purpose.

	 You need only include enough vectors to correspond to
	 all of your entries.
    */

    /* 0 - 3 */
    &AROS_SLIB_ENTRY(convtolower, language),
    &AROS_SLIB_ENTRY(convtoupper, language),
    &AROS_SLIB_ENTRY(null, language),
    &AROS_SLIB_ENTRY(getlangstring, language),

    /* 4 - 7 */
    &AROS_SLIB_ENTRY(isalnum, language),
    &AROS_SLIB_ENTRY(isalpha, language),
    &AROS_SLIB_ENTRY(iscntrl, language),
    &AROS_SLIB_ENTRY(isdigit, language),

    /* 8 - 11 */
    &AROS_SLIB_ENTRY(isgraph, language),
    &AROS_SLIB_ENTRY(islower, language),
    &AROS_SLIB_ENTRY(isprint, language),
    &AROS_SLIB_ENTRY(isspace, language),

    /* 12 - 15 */
    &AROS_SLIB_ENTRY(ispunct, language),
    &AROS_SLIB_ENTRY(isupper, language),
    &AROS_SLIB_ENTRY(isxdigit, language),
    &AROS_SLIB_ENTRY(stringconv, language),

    /* 16 */
    &AROS_SLIB_ENTRY(stringcomp, language),

    (void *)-1
};

/*
    Most languages do not need most of this data. If your languages
    uses the same codeset as english (ISO-8859-1/ECMA Latin 1), then
    my allowing the english code to do all the work (see Mask() function)
    you can reduce this file to just the strings.

    ----------------------------------------------------------------------

    This is the list of strings. It is an array of pointers to strings,
    although how it is laid out is implementation dependant.
*/
const STRPTR __eng_strings[] =
{
    /* A blank string */
    "",

    /*  The days of the week. Starts with the first day of the week.
	In English this would be Sunday, this depends upon the settings
	of Locale->CalendarType.
    */
    "Sunday",   "Monday",   "Tuesday",  "Wednesday",
    "Thursday", "Friday",   "Saturday",

    /* Abbreviated days of the week */
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",

    /* Months of the year */
    "January",  "February", "March",
    "April",    "May",      "June",
    "July",     "August",   "September",
    "October",  "November", "December",

    /* Abbreviated months of the year */
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",

    "Yes", /* Yes, affirmative response */
    "No", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "am", "pm",

    /* Soft and hard hyphens */
    "-", "-",

    /* Open and close quotes */
    "\"", "\"",

    /* Days: But not actual day names
       Yesterday - the day before the current
       Today - the current day
       Tomorrow - the next day
       Future.
    */
    "Yesterday", "Today", "Tomorrow", "Future"
};

/* Array for the IsXXXXX() functions*/
const UWORD __eng_ctype_array[256] = 
{
	/*   0 */  iCntrl,
	/*   1 */  iCntrl,
	/*   2 */  iCntrl,
	/*   3 */  iCntrl,
	/*   4 */  iCntrl,
	/*   5 */  iCntrl,
	/*   6 */  iCntrl,
	/*   7 */  iCntrl,
	/*   8 */  iCntrl,
	/*   9 */  iCntrl | iPrint | iSpace,
	/*  10 */  iCntrl | iPrint | iSpace,
	/*  11 */  iCntrl | iPrint | iSpace,
	/*  12 */  iCntrl | iPrint | iSpace,
	/*  13 */  iCntrl | iPrint | iSpace,
	/*  14 */  iCntrl,
	/*  15 */  iCntrl,
	/*  16 */  iCntrl,
	/*  17 */  iCntrl,
	/*  18 */  iCntrl,
	/*  19 */  iCntrl,
	/*  20 */  iCntrl,
	/*  21 */  iCntrl,
	/*  22 */  iCntrl,
	/*  23 */  iCntrl,
	/*  24 */  iCntrl,
	/*  25 */  iCntrl,
	/*  26 */  iCntrl,
	/*  27 */  iCntrl,
	/*  28 */  iCntrl,
	/*  29 */  iCntrl,
	/*  30 */  iCntrl,
	/*  31 */  iCntrl,
	/*  32 */  iPrint | iSpace,
	/* '!' */  iGraph | iPrint | iPunct,
	/* '"' */  iGraph | iPrint | iPunct,
	/* '#' */  iGraph | iPrint | iPunct,
	/* '$' */  iGraph | iPrint | iPunct,
	/* '%' */  iGraph | iPrint | iPunct,
	/* '&' */  iGraph | iPrint | iPunct,
	/* ''' */  iGraph | iPrint | iPunct,
	/* '(' */  iGraph | iPrint | iPunct,
	/* ')' */  iGraph | iPrint | iPunct,
	/* '*' */  iGraph | iPrint | iPunct,
	/* '+' */  iGraph | iPrint | iPunct,
	/* ',' */  iGraph | iPrint | iPunct,
	/* '-' */  iGraph | iPrint | iPunct,
	/* '.' */  iGraph | iPrint | iPunct,
	/* '/' */  iGraph | iPrint | iPunct,
	/* '0' */  iDigit | iGraph | iPrint | iXDigit,
	/* '1' */  iDigit | iGraph | iPrint | iXDigit,
	/* '2' */  iDigit | iGraph | iPrint | iXDigit,
	/* '3' */  iDigit | iGraph | iPrint | iXDigit,
	/* '4' */  iDigit | iGraph | iPrint | iXDigit,
	/* '5' */  iDigit | iGraph | iPrint | iXDigit,
	/* '6' */  iDigit | iGraph | iPrint | iXDigit,
	/* '7' */  iDigit | iGraph | iPrint | iXDigit,
	/* '8' */  iDigit | iGraph | iPrint | iXDigit,
	/* '9' */  iDigit | iGraph | iPrint | iXDigit,
	/* ':' */  iGraph | iPrint | iPunct,
	/* ';' */  iGraph | iPrint | iPunct,
	/* '<' */  iGraph | iPrint | iPunct,
	/* '=' */  iGraph | iPrint | iPunct,
	/* '>' */  iGraph | iPrint | iPunct,
	/* '?' */  iGraph | iPrint | iPunct,
	/* '@' */  iGraph | iPrint | iPunct,
	/* 'A' */  iAlpha | iGraph | iPrint | iUpper | iXDigit,
	/* 'B' */  iAlpha | iGraph | iPrint | iUpper | iXDigit,
	/* 'C' */  iAlpha | iGraph | iPrint | iUpper | iXDigit,
	/* 'D' */  iAlpha | iGraph | iPrint | iUpper | iXDigit,
	/* 'E' */  iAlpha | iGraph | iPrint | iUpper | iXDigit,
	/* 'F' */  iAlpha | iGraph | iPrint | iUpper | iXDigit,
	/* 'G' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'H' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'I' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'J' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'K' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'L' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'M' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'N' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'O' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'P' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'Q' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'R' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'S' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'T' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'U' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'V' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'W' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'X' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'Y' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'Z' */  iAlpha | iGraph | iPrint | iUpper,
	/* '[' */  iGraph | iPrint | iPunct,
	/* '\' */  iGraph | iPrint | iPunct,
	/* ']' */  iGraph | iPrint | iPunct,
	/* '^' */  iGraph | iPrint | iPunct,
	/* '_' */  iGraph | iPrint | iPunct,
	/* '`' */  iGraph | iPrint | iPunct,
	/* 'a' */  iAlpha | iGraph | iLower | iPrint | iXDigit,
	/* 'b' */  iAlpha | iGraph | iLower | iPrint | iXDigit,
	/* 'c' */  iAlpha | iGraph | iLower | iPrint | iXDigit,
	/* 'd' */  iAlpha | iGraph | iLower | iPrint | iXDigit,
	/* 'e' */  iAlpha | iGraph | iLower | iPrint | iXDigit,
	/* 'f' */  iAlpha | iGraph | iLower | iPrint | iXDigit,
	/* 'g' */  iAlpha | iGraph | iLower | iPrint,
	/* 'h' */  iAlpha | iGraph | iLower | iPrint,
	/* 'i' */  iAlpha | iGraph | iLower | iPrint,
	/* 'j' */  iAlpha | iGraph | iLower | iPrint,
	/* 'k' */  iAlpha | iGraph | iLower | iPrint,
	/* 'l' */  iAlpha | iGraph | iLower | iPrint,
	/* 'm' */  iAlpha | iGraph | iLower | iPrint,
	/* 'n' */  iAlpha | iGraph | iLower | iPrint,
	/* 'o' */  iAlpha | iGraph | iLower | iPrint,
	/* 'p' */  iAlpha | iGraph | iLower | iPrint,
	/* 'q' */  iAlpha | iGraph | iLower | iPrint,
	/* 'r' */  iAlpha | iGraph | iLower | iPrint,
	/* 's' */  iAlpha | iGraph | iLower | iPrint,
	/* 't' */  iAlpha | iGraph | iLower | iPrint,
	/* 'u' */  iAlpha | iGraph | iLower | iPrint,
	/* 'v' */  iAlpha | iGraph | iLower | iPrint,
	/* 'w' */  iAlpha | iGraph | iLower | iPrint,
	/* 'x' */  iAlpha | iGraph | iLower | iPrint,
	/* 'y' */  iAlpha | iGraph | iLower | iPrint,
	/* 'z' */  iAlpha | iGraph | iLower | iPrint,
	/* '{' */  iGraph | iPrint | iPunct,
	/* '|' */  iGraph | iPrint | iPunct,
	/* '}' */  iGraph | iPrint | iPunct,
	/* '~' */  iGraph | iPrint | iPunct,
	/* 127 */  iCntrl,
	/* 128 */  iCntrl,
	/* 129 */  iCntrl,
	/* 130 */  iCntrl,
	/* 131 */  iCntrl,
	/* 132 */  iCntrl,
	/* 133 */  iCntrl,
	/* 134 */  iCntrl,
	/* 135 */  iCntrl,
	/* 136 */  iCntrl,
	/* 137 */  iCntrl,
	/* 138 */  iCntrl,
	/* 139 */  iCntrl,
	/* 140 */  iCntrl,
	/* 141 */  iCntrl,
	/* 142 */  iCntrl,
	/* 143 */  iCntrl,
	/* 144 */  iCntrl,
	/* 145 */  iCntrl,
	/* 146 */  iCntrl,
	/* 147 */  iCntrl,
	/* 148 */  iCntrl,
	/* 149 */  iCntrl,
	/* 150 */  iCntrl,
	/* 151 */  iCntrl,
	/* 152 */  iCntrl,
	/* 153 */  iCntrl,
	/* 154 */  iCntrl,
	/* 155 */  iCntrl,
	/* 156 */  iCntrl,
	/* 157 */  iCntrl,
	/* 158 */  iCntrl,
	/* 159 */  iCntrl,
	/* 160 */  iPrint | iSpace,
	/* '∞' */  iGraph | iPrint | iPunct,
	/* '¢' */  iGraph | iPrint | iPunct,
	/* '£' */  iGraph | iPrint | iPunct,
	/* 'ß' */  iGraph | iPrint | iPunct,
	/* 'Ä' */  iGraph | iPrint | iPunct,
	/* '∂' */  iGraph | iPrint | iPunct,
	/* 'ﬂ' */  iGraph | iPrint | iPunct,
	/* 'Æ' */  iGraph | iPrint | iPunct,
	/* '©' */  iGraph | iPrint | iPunct,
	/* 'Å' */  iGraph | iPrint | iPunct,
	/* '¥' */  iGraph | iPrint | iPunct,
	/* '®' */  iGraph | iPrint | iPunct,
	/* 'Ç' */  iGraph | iPrint | iPunct,
	/* '∆' */  iGraph | iPrint | iPunct,
	/* 'ÿ' */  iGraph | iPrint | iPunct,
	/* 'É' */  iGraph | iPrint | iPunct,
	/* '±' */  iGraph | iPrint | iPunct,
	/* 'æ' */  iGraph | iPrint | iPunct,
	/* 'Ñ' */  iGraph | iPrint | iPunct,
	/* '•' */  iGraph | iPrint | iPunct,
	/* 'µ' */  iGraph | iPrint | iPunct,
	/* 'è' */  iGraph | iPrint | iPunct,
	/* 'Ö' */  iGraph | iPrint | iPunct,
	/* 'Ω' */  iGraph | iPrint | iPunct,
	/* 'º' */  iGraph | iPrint | iPunct,
	/* 'Ü' */  iGraph | iPrint | iPunct,
	/* '™' */  iGraph | iPrint | iPunct,
	/* '∫' */  iGraph | iPrint | iPunct,
	/* 'á' */  iGraph | iPrint | iPunct,
	/* 'Ê' */  iGraph | iPrint | iPunct,
	/* '¯' */  iGraph | iPrint | iPunct,
	/* 'ø' */  iAlpha | iGraph | iPrint | iUpper,
	/* '°' */  iAlpha | iGraph | iPrint | iUpper,
	/* '¨' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'à' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ü' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'â' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ê' */  iAlpha | iGraph | iPrint | iUpper,
	/* '´' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ª' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ä' */  iAlpha | iGraph | iPrint | iUpper,
	/* '†' */  iAlpha | iGraph | iPrint | iUpper,
	/* '¿' */  iAlpha | iGraph | iPrint | iUpper,
	/* '√' */  iAlpha | iGraph | iPrint | iUpper,
	/* '’' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ë' */  iAlpha | iGraph | iPrint | iUpper,
	/* '¶' */  iAlpha | iGraph | iPrint | iUpper,
	/* '≠' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ã' */  iAlpha | iGraph | iPrint | iUpper,
	/* '≥' */  iAlpha | iGraph | iPrint | iUpper,
	/* '≤' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'å' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'π' */  iAlpha | iGraph | iPrint | iUpper,
	/* '˜' */  iAlpha | iGraph | iPrint | iUpper,
	/* '◊' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ˇ' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ç' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'é' */  iAlpha | iGraph | iPrint | iUpper,
	/* '§' */  iAlpha | iGraph | iPrint | iUpper,
	/* '–' */  iAlpha | iGraph | iPrint | iUpper,
	/* '' */  iAlpha | iGraph | iPrint | iUpper,
	/* 'ﬁ' */  iAlpha | iGraph | iPrint | iUpper,
	/* '˛' */  iAlpha | iGraph | iLower | iPrint | iUpper,
	/* '˝' */  iAlpha | iGraph | iLower | iPrint,
	/* '∑' */  iAlpha | iGraph | iLower | iPrint,
	/* 'í' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ì' */  iAlpha | iGraph | iLower | iPrint,
	/* 'î' */  iAlpha | iGraph | iLower | iPrint,
	/* '¬' */  iAlpha | iGraph | iLower | iPrint,
	/* ' ' */  iAlpha | iGraph | iLower | iPrint,
	/* '¡' */  iAlpha | iGraph | iLower | iPrint,
	/* 'À' */  iAlpha | iGraph | iLower | iPrint,
	/* '»' */  iAlpha | iGraph | iLower | iPrint,
	/* 'Õ' */  iAlpha | iGraph | iLower | iPrint,
	/* 'Œ' */  iAlpha | iGraph | iLower | iPrint,
	/* 'œ' */  iAlpha | iGraph | iLower | iPrint,
	/* 'Ã' */  iAlpha | iGraph | iLower | iPrint,
	/* '”' */  iAlpha | iGraph | iLower | iPrint,
	/* '‘' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ï' */  iAlpha | iGraph | iLower | iPrint,
	/* '“' */  iAlpha | iGraph | iLower | iPrint,
	/* '⁄' */  iAlpha | iGraph | iLower | iPrint,
	/* '€' */  iAlpha | iGraph | iLower | iPrint,
	/* 'Ÿ' */  iAlpha | iGraph | iLower | iPrint,
	/* 'û' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ñ' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ó' */  iGraph | iPrint | iPunct,
	/* 'Ø' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ò' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ô' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ö' */  iAlpha | iGraph | iLower | iPrint,
	/* '∏' */  iAlpha | iGraph | iLower | iPrint,
	/* 'õ' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ú' */  iAlpha | iGraph | iLower | iPrint,
	/* 'ù' */  iAlpha | iGraph | iLower | iPrint
};

const ULONG __eng_to_lower[256] = 
{
	0  ,            /* 0   */        1  ,            /* 1   */
	2  ,            /* 2   */        3  ,            /* 3   */
	4  ,            /* 4   */        5  ,            /* 5   */
	6  ,            /* 6   */        7  ,            /* 7   */
	8  ,            /* 8   */        9  ,            /* 9   */
	10 ,            /* 10  */        11 ,            /* 11  */
	12 ,            /* 12  */        13 ,            /* 13  */
	14 ,            /* 14  */        15 ,            /* 15  */
	16 ,            /* 16  */        17 ,            /* 17  */
	18 ,            /* 18  */        19 ,            /* 19  */
	20 ,            /* 20  */        21 ,            /* 21  */
	22 ,            /* 22  */        23 ,            /* 23  */
	24 ,            /* 24  */        25 ,            /* 25  */
	26 ,            /* 26  */        27 ,            /* 27  */
	28 ,            /* 28  */        29 ,            /* 29  */
	30 ,            /* 30  */        31 ,            /* 31  */
	32 ,            /* 32  */        '!',            /* '!' */
	'"',            /* '"' */        '#',            /* '#' */
	'$',            /* '$' */        '%',            /* '%' */
	'&',            /* '&' */        39 ,            /* ''' */
	'(',            /* '(' */        ')',            /* ')' */
	'*',            /* '*' */        '+',            /* '+' */
	',',            /* ',' */        '-',            /* '-' */
	'.',            /* '.' */        '/',            /* '/' */
	'0',            /* '0' */        '1',            /* '1' */
	'2',            /* '2' */        '3',            /* '3' */
	'4',            /* '4' */        '5',            /* '5' */
	'6',            /* '6' */        '7',            /* '7' */
	'8',            /* '8' */        '9',            /* '9' */
	':',            /* ':' */        ';',            /* ';' */
	'<',            /* '<' */        '=',            /* '=' */
	'>',            /* '>' */        '?',            /* '?' */
	'@',            /* '@' */        'a',            /* 'A' */
	'b',            /* 'B' */        'c',            /* 'C' */
	'd',            /* 'D' */        'e',            /* 'E' */
	'f',            /* 'F' */        'g',            /* 'G' */
	'h',            /* 'H' */        'i',            /* 'I' */
	'j',            /* 'J' */        'k',            /* 'K' */
	'l',            /* 'L' */        'm',            /* 'M' */
	'n',            /* 'N' */        'o',            /* 'O' */
	'p',            /* 'P' */        'q',            /* 'Q' */
	'r',            /* 'R' */        's',            /* 'S' */
	't',            /* 'T' */        'u',            /* 'U' */
	'v',            /* 'V' */        'w',            /* 'W' */
	'x',            /* 'X' */        'y',            /* 'Y' */
	'z',            /* 'Z' */        '[',            /* '[' */
	92 ,            /* '\' */        ']',            /* ']' */
	'^',            /* '^' */        '_',            /* '_' */
	'`',            /* '`' */        'a',            /* 'a' */
	'b',            /* 'b' */        'c',            /* 'c' */
	'd',            /* 'd' */        'e',            /* 'e' */
	'f',            /* 'f' */        'g',            /* 'g' */
	'h',            /* 'h' */        'i',            /* 'i' */
	'j',            /* 'j' */        'k',            /* 'k' */
	'l',            /* 'l' */        'm',            /* 'm' */
	'n',            /* 'n' */        'o',            /* 'o' */
	'p',            /* 'p' */        'q',            /* 'q' */
	'r',            /* 'r' */        's',            /* 's' */
	't',            /* 't' */        'u',            /* 'u' */
	'v',            /* 'v' */        'w',            /* 'w' */
	'x',            /* 'x' */        'y',            /* 'y' */
	'z',            /* 'z' */        '{',            /* '{' */
	'|',            /* '|' */        '}',            /* '}' */
	'~',            /* '~' */        127,            /* 127 */
	128,            /* 128 */        129,            /* 129 */
	130,            /* 130 */        131,            /* 131 */
	132,            /* 132 */        133,            /* 133 */
	134,            /* 134 */        135,            /* 135 */
	136,            /* 136 */        137,            /* 137 */
	138,            /* 138 */        139,            /* 139 */
	140,            /* 140 */        141,            /* 141 */
	142,            /* 142 */        143,            /* 143 */
	144,            /* 144 */        145,            /* 145 */
	146,            /* 146 */        147,            /* 147 */
	148,            /* 148 */        149,            /* 149 */
	150,            /* 150 */        151,            /* 151 */
	152,            /* 152 */        153,            /* 153 */
	154,            /* 154 */        155,            /* 155 */
	156,            /* 156 */        157,            /* 157 */
	158,            /* 158 */        159,            /* 159 */
	160,            /* 160 */        '∞',            /* '∞' */
	'¢',            /* '¢' */        '£',            /* '£' */
	'ß',            /* 'ß' */        'Ä',            /* 'Ä' */
	'∂',            /* '∂' */        'ﬂ',            /* 'ﬂ' */
	'Æ',            /* 'Æ' */        '©',            /* '©' */
	'Å',            /* 'Å' */        '¥',            /* '¥' */
	'®',            /* '®' */        'Ç',            /* 'Ç' */
	'∆',            /* '∆' */        'ÿ',            /* 'ÿ' */
	'É',            /* 'É' */        '±',            /* '±' */
	'æ',            /* 'æ' */        'Ñ',            /* 'Ñ' */
	'•',            /* '•' */        'µ',            /* 'µ' */
	'è',            /* 'è' */        'Ö',            /* 'Ö' */
	'Ω',            /* 'Ω' */        'º',            /* 'º' */
	'Ü',            /* 'Ü' */        '™',            /* '™' */
	'∫',            /* '∫' */        'á',            /* 'á' */
	'Ê',            /* 'Ê' */        '¯',            /* '¯' */
	'˝',            /* 'ø' */        '∑',            /* '°' */
	'í',            /* '¨' */        'ì',            /* 'à' */
	'î',            /* 'ü' */        '¬',            /* 'â' */
	' ',            /* 'ê' */        '¡',            /* '´' */
	'À',            /* 'ª' */        '»',            /* 'ä' */
	'Õ',            /* '†' */        'Œ',            /* '¿' */
	'œ',            /* '√' */        'Ã',            /* '’' */
	'”',            /* 'ë' */        '‘',            /* '¶' */
	'ï',            /* '≠' */        '“',            /* 'ã' */
	'⁄',            /* '≥' */        '€',            /* '≤' */
	'Ÿ',            /* 'å' */        'û',            /* 'π' */
	'ñ',            /* '˜' */        'ó',            /* '◊' */
	'Ø',            /* 'ˇ' */        'ò',            /* 'ç' */
	'ô',            /* 'é' */        'ö',            /* '§' */
	'∏',            /* '–' */        'õ',            /* '' */
	'ú',            /* 'ﬁ' */        '˛',            /* '˛' */
	'˝',            /* '˝' */        '∑',            /* '∑' */
	'í',            /* 'í' */        'ì',            /* 'ì' */
	'î',            /* 'î' */        '¬',            /* '¬' */
	' ',            /* ' ' */        '¡',            /* '¡' */
	'À',            /* 'À' */        '»',            /* '»' */
	'Õ',            /* 'Õ' */        'Œ',            /* 'Œ' */
	'œ',            /* 'œ' */        'Ã',            /* 'Ã' */
	'”',            /* '”' */        '‘',            /* '‘' */
	'ï',            /* 'ï' */        '“',            /* '“' */
	'⁄',            /* '⁄' */        '€',            /* '€' */
	'Ÿ',            /* 'Ÿ' */        'û',            /* 'û' */
	'ñ',            /* 'ñ' */        'ó',            /* 'ó' */
	'Ø',            /* 'Ø' */        'ò',            /* 'ò' */
	'ô',            /* 'ô' */        'ö',            /* 'ö' */
	'∏',            /* '∏' */        'õ',            /* 'õ' */
	'ú',            /* 'ú' */        'ù',            /* 'ù' */
};

const ULONG __eng_to_upper[256] = 
{
	0  ,            /* 0   */        1  ,            /* 1   */
	2  ,            /* 2   */        3  ,            /* 3   */
	4  ,            /* 4   */        5  ,            /* 5   */
	6  ,            /* 6   */        7  ,            /* 7   */
	8  ,            /* 8   */        9  ,            /* 9   */
	10 ,            /* 10  */        11 ,            /* 11  */
	12 ,            /* 12  */        13 ,            /* 13  */
	14 ,            /* 14  */        15 ,            /* 15  */
	16 ,            /* 16  */        17 ,            /* 17  */
	18 ,            /* 18  */        19 ,            /* 19  */
	20 ,            /* 20  */        21 ,            /* 21  */
	22 ,            /* 22  */        23 ,            /* 23  */
	24 ,            /* 24  */        25 ,            /* 25  */
	26 ,            /* 26  */        27 ,            /* 27  */
	28 ,            /* 28  */        29 ,            /* 29  */
	30 ,            /* 30  */        31 ,            /* 31  */
	32 ,            /* 32  */        '!',            /* '!' */
	'"',            /* '"' */        '#',            /* '#' */
	'$',            /* '$' */        '%',            /* '%' */
	'&',            /* '&' */        39 ,            /* ''' */
	'(',            /* '(' */        ')',            /* ')' */
	'*',            /* '*' */        '+',            /* '+' */
	',',            /* ',' */        '-',            /* '-' */
	'.',            /* '.' */        '/',            /* '/' */
	'0',            /* '0' */        '1',            /* '1' */
	'2',            /* '2' */        '3',            /* '3' */
	'4',            /* '4' */        '5',            /* '5' */
	'6',            /* '6' */        '7',            /* '7' */
	'8',            /* '8' */        '9',            /* '9' */
	':',            /* ':' */        ';',            /* ';' */
	'<',            /* '<' */        '=',            /* '=' */
	'>',            /* '>' */        '?',            /* '?' */
	'@',            /* '@' */        'A',            /* 'A' */
	'B',            /* 'B' */        'C',            /* 'C' */
	'D',            /* 'D' */        'E',            /* 'E' */
	'F',            /* 'F' */        'G',            /* 'G' */
	'H',            /* 'H' */        'I',            /* 'I' */
	'J',            /* 'J' */        'K',            /* 'K' */
	'L',            /* 'L' */        'M',            /* 'M' */
	'N',            /* 'N' */        'O',            /* 'O' */
	'P',            /* 'P' */        'Q',            /* 'Q' */
	'R',            /* 'R' */        'S',            /* 'S' */
	'T',            /* 'T' */        'U',            /* 'U' */
	'V',            /* 'V' */        'W',            /* 'W' */
	'X',            /* 'X' */        'Y',            /* 'Y' */
	'Z',            /* 'Z' */        '[',            /* '[' */
	92 ,            /* '\' */        ']',            /* ']' */
	'^',            /* '^' */        '_',            /* '_' */
	'`',            /* '`' */        'A',            /* 'a' */
	'B',            /* 'b' */        'C',            /* 'c' */
	'D',            /* 'd' */        'E',            /* 'e' */
	'F',            /* 'f' */        'G',            /* 'g' */
	'H',            /* 'h' */        'I',            /* 'i' */
	'J',            /* 'j' */        'K',            /* 'k' */
	'L',            /* 'l' */        'M',            /* 'm' */
	'N',            /* 'n' */        'O',            /* 'o' */
	'P',            /* 'p' */        'Q',            /* 'q' */
	'R',            /* 'r' */        'S',            /* 's' */
	'T',            /* 't' */        'U',            /* 'u' */
	'V',            /* 'v' */        'W',            /* 'w' */
	'X',            /* 'x' */        'Y',            /* 'y' */
	'Z',            /* 'z' */        '{',            /* '{' */
	'|',            /* '|' */        '}',            /* '}' */
	'~',            /* '~' */        127,            /* 127 */
	128,            /* 128 */        129,            /* 129 */
	130,            /* 130 */        131,            /* 131 */
	132,            /* 132 */        133,            /* 133 */
	134,            /* 134 */        135,            /* 135 */
	136,            /* 136 */        137,            /* 137 */
	138,            /* 138 */        139,            /* 139 */
	140,            /* 140 */        141,            /* 141 */
	142,            /* 142 */        143,            /* 143 */
	144,            /* 144 */        145,            /* 145 */
	146,            /* 146 */        147,            /* 147 */
	148,            /* 148 */        149,            /* 149 */
	150,            /* 150 */        151,            /* 151 */
	152,            /* 152 */        153,            /* 153 */
	154,            /* 154 */        155,            /* 155 */
	156,            /* 156 */        157,            /* 157 */
	158,            /* 158 */        159,            /* 159 */
	160,            /* 160 */        '∞',            /* '∞' */
	'¢',            /* '¢' */        '£',            /* '£' */
	'ß',            /* 'ß' */        'Ä',            /* 'Ä' */
	'∂',            /* '∂' */        'ﬂ',            /* 'ﬂ' */
	'Æ',            /* 'Æ' */        '©',            /* '©' */
	'Å',            /* 'Å' */        '¥',            /* '¥' */
	'®',            /* '®' */        'Ç',            /* 'Ç' */
	'∆',            /* '∆' */        'ÿ',            /* 'ÿ' */
	'É',            /* 'É' */        '±',            /* '±' */
	'æ',            /* 'æ' */        'Ñ',            /* 'Ñ' */
	'•',            /* '•' */        'µ',            /* 'µ' */
	'è',            /* 'è' */        'Ö',            /* 'Ö' */
	'Ω',            /* 'Ω' */        'º',            /* 'º' */
	'Ü',            /* 'Ü' */        '™',            /* '™' */
	'∫',            /* '∫' */        'á',            /* 'á' */
	'Ê',            /* 'Ê' */        '¯',            /* '¯' */
	'ø',            /* 'ø' */        '°',            /* '°' */
	'¨',            /* '¨' */        'à',            /* 'à' */
	'ü',            /* 'ü' */        'â',            /* 'â' */
	'ê',            /* 'ê' */        '´',            /* '´' */
	'ª',            /* 'ª' */        'ä',            /* 'ä' */
	'†',            /* '†' */        '¿',            /* '¿' */
	'√',            /* '√' */        '’',            /* '’' */
	'ë',            /* 'ë' */        '¶',            /* '¶' */
	'≠',            /* '≠' */        'ã',            /* 'ã' */
	'≥',            /* '≥' */        '≤',            /* '≤' */
	'å',            /* 'å' */        'π',            /* 'π' */
	'˜',            /* '˜' */        '◊',            /* '◊' */
	'ˇ',            /* 'ˇ' */        'ç',            /* 'ç' */
	'é',            /* 'é' */        '§',            /* '§' */
	'–',            /* '–' */        '',            /* '' */
	'ﬁ',            /* 'ﬁ' */        '˛',            /* '˛' */
	'ø',            /* '˝' */        '°',            /* '∑' */
	'¨',            /* 'í' */        'à',            /* 'ì' */
	'ü',            /* 'î' */        'â',            /* '¬' */
	'ê',            /* ' ' */        '´',            /* '¡' */
	'ª',            /* 'À' */        'ä',            /* '»' */
	'†',            /* 'Õ' */        '¿',            /* 'Œ' */
	'√',            /* 'œ' */        '’',            /* 'Ã' */
	'ë',            /* '”' */        '¶',            /* '‘' */
	'≠',            /* 'ï' */        'ã',            /* '“' */
	'≥',            /* '⁄' */        '≤',            /* '€' */
	'å',            /* 'Ÿ' */        'π',            /* 'û' */
	'˜',            /* 'ñ' */        '◊',            /* 'ó' */
	'ˇ',            /* 'Ø' */        'ç',            /* 'ò' */
	'é',            /* 'ô' */        '§',            /* 'ö' */
	'–',            /* '∏' */        '',            /* 'õ' */
	'ﬁ',            /* 'ú' */        'ù',            /* 'ù' */
};

/*
    This is the string collation table.

    The basic idea is to have the character which is found in the normal
    ENGLISH alphabet used instead, this will allow for sorting in a
    list so that "f€ol" would come before full not after it, as would
    happen by sorting just by ASCII characters.
*/
const ULONG __eng_collate_tab[256] =
{
	0  ,            /* 0   */        1  ,            /* 1   */
	2  ,            /* 2   */        3  ,            /* 3   */
	4  ,            /* 4   */        5  ,            /* 5   */
	6  ,            /* 6   */        7  ,            /* 7   */
	8  ,            /* 8   */        9  ,            /* 9   */
	10 ,            /* 10  */        11 ,            /* 11  */
	12 ,            /* 12  */        13 ,            /* 13  */
	14 ,            /* 14  */        15 ,            /* 15  */
	16 ,            /* 16  */        17 ,            /* 17  */
	18 ,            /* 18  */        19 ,            /* 19  */
	20 ,            /* 20  */        21 ,            /* 21  */
	22 ,            /* 22  */        23 ,            /* 23  */
	24 ,            /* 24  */        25 ,            /* 25  */
	26 ,            /* 26  */        27 ,            /* 27  */
	28 ,            /* 28  */        29 ,            /* 29  */
	30 ,            /* 30  */        31 ,            /* 31  */
	32 ,            /* 32  */        '!',            /* '!' */
	'"',            /* '"' */        '#',            /* '#' */
	'$',            /* '$' */        '%',            /* '%' */
	'&',            /* '&' */        39 ,            /* ''' */
	'(',            /* '(' */        ')',            /* ')' */
	'*',            /* '*' */        '+',            /* '+' */
	',',            /* ',' */        '-',            /* '-' */
	'.',            /* '.' */        '/',            /* '/' */
	'0',            /* '0' */        '1',            /* '1' */
	'2',            /* '2' */        '3',            /* '3' */
	'4',            /* '4' */        '5',            /* '5' */
	'6',            /* '6' */        '7',            /* '7' */
	'8',            /* '8' */        '9',            /* '9' */
	':',            /* ':' */        ';',            /* ';' */
	'<',            /* '<' */        '=',            /* '=' */
	'>',            /* '>' */        '?',            /* '?' */
	'@',            /* '@' */        'A',            /* 'A' */
	'B',            /* 'B' */        'C',            /* 'C' */
	'D',            /* 'D' */        'E',            /* 'E' */
	'F',            /* 'F' */        'G',            /* 'G' */
	'H',            /* 'H' */        'I',            /* 'I' */
	'J',            /* 'J' */        'K',            /* 'K' */
	'L',            /* 'L' */        'M',            /* 'M' */
	'N',            /* 'N' */        'O',            /* 'O' */
	'P',            /* 'P' */        'Q',            /* 'Q' */
	'R',            /* 'R' */        'S',            /* 'S' */
	'T',            /* 'T' */        'U',            /* 'U' */
	'V',            /* 'V' */        'W',            /* 'W' */
	'X',            /* 'X' */        'Y',            /* 'Y' */
	'Z',            /* 'Z' */        '[',            /* '[' */
	92 ,            /* '\' */        ']',            /* ']' */
	'^',            /* '^' */        '_',            /* '_' */
	'`',            /* '`' */        'A',            /* 'a' */
	'B',            /* 'b' */        'C',            /* 'c' */
	'D',            /* 'd' */        'E',            /* 'e' */
	'F',            /* 'f' */        'G',            /* 'g' */
	'H',            /* 'h' */        'I',            /* 'i' */
	'J',            /* 'j' */        'K',            /* 'k' */
	'L',            /* 'l' */        'M',            /* 'm' */
	'N',            /* 'n' */        'O',            /* 'o' */
	'P',            /* 'p' */        'Q',            /* 'q' */
	'R',            /* 'r' */        'S',            /* 's' */
	'T',            /* 't' */        'U',            /* 'u' */
	'V',            /* 'v' */        'W',            /* 'w' */
	'X',            /* 'x' */        'Y',            /* 'y' */
	'Z',            /* 'z' */        '{',            /* '{' */
	'|',            /* '|' */        '}',            /* '}' */
	'~',            /* '~' */        127,            /* 127 */
	128,            /* 128 */        129,            /* 129 */
	130,            /* 130 */        131,            /* 131 */
	132,            /* 132 */        133,            /* 133 */
	134,            /* 134 */        135,            /* 135 */
	136,            /* 136 */        137,            /* 137 */
	138,            /* 138 */        139,            /* 139 */
	140,            /* 140 */        141,            /* 141 */
	142,            /* 142 */        143,            /* 143 */
	144,            /* 144 */        145,            /* 145 */
	146,            /* 146 */        147,            /* 147 */
	148,            /* 148 */        149,            /* 149 */
	150,            /* 150 */        151,            /* 151 */
	152,            /* 152 */        153,            /* 153 */
	154,            /* 154 */        155,            /* 155 */
	156,            /* 156 */        157,            /* 157 */
	158,            /* 158 */        159,            /* 159 */
	20,             /* 160 */        '!',            /* '∞' */
	'$',            /* '¢' */        '$',            /* '£' */
	'g',            /* 'ß' */        'h',            /* 'Ä' */
	'i',            /* '∂' */        'S',            /* 'ﬂ' */
	'j',            /* 'Æ' */        'k',            /* '©' */
	'l',            /* 'Å' */        '\"',           /* '¥' */
	'm',            /* '®' */        'n',            /* 'Ç' */
	'o',            /* '∆' */        'p',            /* 'ÿ' */
	'q',            /* 'É' */        'r',            /* '±' */
	's',            /* 'æ' */        't',            /* 'Ñ' */
	'u',            /* '•' */        'v',            /* 'µ' */
	'w',            /* 'è' */        'x',            /* 'Ö' */
	'y',            /* 'Ω' */        'z',            /* 'º' */
	'{',            /* 'Ü' */        '\"'            /* '™' */
	'|',            /* '∫' */        '}',            /* 'á' */
	'~',            /* 'Ê' */        '?',            /* '¯' */
	'A',            /* 'ø' */        'A',            /* '°' */
	'A',            /* '¨' */        'A',            /* 'à' */
	'A',            /* 'ü' */        'A',            /* 'â' */
	'A',            /* 'ê' */        'C',            /* '´' */
	'E',            /* 'ª' */        'E',            /* 'ä' */
	'E',            /* '†' */        'E',            /* '¿' */
	'I',            /* '√' */        'I',            /* '’' */
	'I',            /* 'ë' */        'I',            /* '¶' */
	'D',            /* '≠' */        'N',            /* 'ã' */
	'O',            /* '≥' */        'O',            /* '≤' */
	'O',            /* 'å' */        'O',            /* 'π' */
	'O',            /* '˜' */        '*',            /* '◊' */
	'O',            /* 'ˇ' */        'U',            /* 'ç' */
	'U',            /* 'é' */        'U',            /* '§' */
	'U',            /* '–' */        'Y',            /* '' */
	'P',            /* 'ﬁ' */        'Y',            /* '˛' */
	'A',            /* '˝' */        'A',            /* '∑' */
	'A',            /* 'í' */        'A',            /* 'ì' */
	'A',            /* 'î' */        'A',            /* '¬' */
	'A',            /* ' ' */        'C',            /* '¡' */
	'E',            /* 'À' */        'E',            /* '»' */
	'E',            /* 'Õ' */        'E',            /* 'Œ' */
	'I',            /* 'œ' */        'I',            /* 'Ã' */
	'I',            /* '”' */        'I',            /* '‘' */
	'D',            /* 'ï' */        'N',            /* '“' */
	'O',            /* '⁄' */        'O',            /* '€' */
	'O',            /* 'Ÿ' */        'O',            /* 'û' */
	'O',            /* 'ñ' */        '/',            /* 'ó' */
	'O',            /* 'Ø' */        'U',            /* 'ò' */
	'U',            /* 'ô' */        'U',            /* 'ö' */
	'U',            /* '∏' */        'Y',            /* 'õ' */
	'P',            /* 'ú' */        'Y',            /* 'ù' */
};



/* This is the end of ROMtag marker. */
const char end=0;
