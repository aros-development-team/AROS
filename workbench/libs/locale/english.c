/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: english.language description file.
    Lang: english
*/

#include <exec/types.h>
#include <libraries/locale.h>

#include <proto/exec.h>
#include <aros/libcall.h>

/* Arrays for english/Latin1 character type/conversion, defined later */
extern const UWORD __eng_ctype_array[];
extern const ULONG __eng_to_lower[];
extern const ULONG __eng_to_upper[];
extern const STRPTR __eng_strings[];
extern const ULONG __eng_collate_tab[];
extern ULONG AROS_SLIB_ENTRY(null,Locale)();

/* We use these to indicate whether a character is a certain type in the
   character array. You don't actually have to use these really, you
   can do whatever you like.
*/
#define iAlpha  (1 << 0)    /* Alphabetical characters */
#define iCntrl  (1 << 1)    /* A control character */
#define iDigit  (1 << 2)    /* Digit */
#define iGraph  (1 << 3)    /* Graphical */
#define iLower  (1 << 4)    /* Lower case alphabetical */
#define iPrint  (1 << 5)    /* Printable */
#define iPunct  (1 << 6)    /* Punctuation */
#define iSpace  (1 << 7)    /* Whitespace */
#define iUpper  (1 << 8)    /* Upper case */
#define iXDigit (1 << 9)    /* Hexadecimal digit */

/* ------------------------------------------------------------------ 
    Language Functions
   ------------------------------------------------------------------ */

/* ULONG ConvToLower(ULONG char): Language function 0
    This function converts the character char to the equivalent
    lower case value.
*/
AROS_LD1(ULONG, convtolower,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 6, english);
AROS_LH1(ULONG, convtolower,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 6, english)
{
    AROS_LIBFUNC_INIT

    return __eng_to_lower[chr];

    AROS_LIBFUNC_EXIT
}

/* ULONG ConvToUpper(ULONG char): Language Function 1
    This function converts the character char to the equivalent
    upper case character.
*/
AROS_LD1(ULONG, convtoupper,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 7, english);
AROS_LH1(ULONG, convtoupper,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 7, english)
{
    AROS_LIBFUNC_INIT

    return __eng_to_upper[chr];

    AROS_LIBFUNC_EXIT
}

/* STRPTR GetLangString(ULONG num): Language function 3
    This function is called by GetLocaleStr() and should return
    the string matching the string id passed in as num.
*/
AROS_LD1(STRPTR, getlangstring,
    AROS_LDA(ULONG, id, D0),
    struct LocaleBase *, LocaleBase, 9, english);
AROS_LH1(STRPTR, getlangstring,
    AROS_LHA(ULONG, id, D0),
    struct LocaleBase *, LocaleBase, 9, english)
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

AROS_LD1(BOOL, isalnum,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 10, english);
AROS_LH1(BOOL, isalnum,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 10, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(
      (__eng_ctype_array[chr] & iAlpha) || (__eng_ctype_array[chr] & iDigit)
      );

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, isalpha,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 11, english);
AROS_LH1(BOOL, isalpha,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 11, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iAlpha);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, iscntrl,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 12, english);
AROS_LH1(BOOL, iscntrl,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 12, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iCntrl);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, isdigit,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 13, english);
AROS_LH1(BOOL, isdigit,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 13, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iDigit);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, isgraph,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 14, english);
AROS_LH1(BOOL, isgraph,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 14, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iGraph);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, islower,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 15, english);
AROS_LH1(BOOL, islower,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 15, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iLower);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, isprint,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 16, english);
AROS_LH1(BOOL, isprint,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 16, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iPrint);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, ispunct,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 17, english);
AROS_LH1(BOOL, ispunct,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 17, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iPunct);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, isspace,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 18, english);
AROS_LH1(BOOL, isspace,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 18, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iSpace);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, isupper,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 19, english);
AROS_LH1(BOOL, isupper,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 19, english)
{
    AROS_LIBFUNC_INIT

    return (BOOL)(__eng_ctype_array[chr] & iUpper);

    AROS_LIBFUNC_EXIT
}

AROS_LD1(BOOL, isxdigit,
    AROS_LDA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 20, english);
AROS_LH1(BOOL, isxdigit,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 20, english)
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
AROS_LD4(ULONG, strconvert,
    AROS_LDA(STRPTR,    string1, A1),
    AROS_LDA(STRPTR,    string2, A2),
    AROS_LDA(LONG,      length,  D0),
    AROS_LDA(ULONG,     type,    D1),
    struct LocaleBase *, LocaleBase, 21, english);
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
	STRPTR origS1 = string1;

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
	const ULONG *collTab;

	if(type == SC_ASCII)
	    collTab = __eng_to_upper;
	else
	    collTab = __eng_collate_tab;

	while(--length && *string1)
	{
	    (ULONG)(*string2++) = collTab[ (UBYTE)*string1 ];
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
AROS_LD4(LONG, strcompare,
    AROS_LDA(STRPTR,    string1, A1),
    AROS_LDA(STRPTR,    string2, A2),
    AROS_LDA(LONG,      length,  D0),
    AROS_LDA(ULONG,     type,    D1),
    struct LocaleBase *, LocaleBase, 22, english);
AROS_LH4(LONG, strcompare,
    AROS_LHA(STRPTR,    string1, A1),
    AROS_LHA(STRPTR,    string2, A2),
    AROS_LHA(LONG,      length,  D0),
    AROS_LHA(ULONG,     type,    D1),
    struct LocaleBase *, LocaleBase, 22, english)
{
    AROS_LIBFUNC_INIT
    ULONG a, b;

    const ULONG *colltab;

    if (!string1 || !string2)
      return 0;

    /* Determine which collation table to use... */
    if(type == SC_ASCII)
	colltab = __eng_to_upper;
    else
	colltab = __eng_collate_tab;

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
    else if((type == SC_COLLATE1) || (type == SC_ASCII))
    {
	do
	{
	    a = colltab[(UBYTE)*string1];
	    b = colltab[(UBYTE)*string2];
	    string1++;
	    string2++;
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

void *const __eng_functable[] =
{
    /* 0 - 3 */
    &AROS_SLIB_ENTRY(convtolower, english),
    &AROS_SLIB_ENTRY(convtoupper, english),
    &AROS_SLIB_ENTRY(null, Locale),
    &AROS_SLIB_ENTRY(getlangstring, english),

    /* 4 - 7 */
    &AROS_SLIB_ENTRY(isalnum, english),
    &AROS_SLIB_ENTRY(isalpha, english),
    &AROS_SLIB_ENTRY(iscntrl, english),
    &AROS_SLIB_ENTRY(isdigit, english),

    /* 8 - 11 */
    &AROS_SLIB_ENTRY(isgraph, english),
    &AROS_SLIB_ENTRY(islower, english),
    &AROS_SLIB_ENTRY(isprint, english),
    &AROS_SLIB_ENTRY(isspace, english),

    /* 12 - 15 */
    &AROS_SLIB_ENTRY(ispunct, english),
    &AROS_SLIB_ENTRY(isupper, english),
    &AROS_SLIB_ENTRY(isxdigit, english),
    &AROS_SLIB_ENTRY(strconvert, english),

    /* 16 */
    &AROS_SLIB_ENTRY(strcompare, english),

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
    list so that "f€ll" would come before full not after it, as would
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
	'{',            /* 'Ü' */       '\"',           /* '™' */
	'|',		/* '∫' */	 '}',            /* 'á' */
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
