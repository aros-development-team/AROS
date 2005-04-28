/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: english.language description file.
    Lang: english
*/

#include <exec/types.h>
#include <libraries/locale.h>

#include <proto/exec.h>
#include <aros/asmcall.h>
#include <aros/libcall.h>


/*
    This is the language implementation for the default .language, the
    English language based on the ISO-8859-1 (ECMA Latin 1) code table.

    This code can also be used to create other .languages.

    Languages using the same code table could be implemented by reusing
    the code for GetLangString, StrCompare, and StrConvert, and redefining
    the language character order tables and the language strings. For all
    other functions the english code could be allowed to do all the work
    (see Mask() function).

    Languages even using the same character order could be implemented by
    reusing the code for GetLangString and only redefining the language
    strings.
    ("Character order" refers to the alphabet. However, it also concerns the
     the way characters outside the alphabet should be ordered, and how
     characters with accents should mix with "regular"characters. If you're
     defining a custom .languages to change the sorting behaviour of the OS,
     then you're changing its character order.)

    Languages using a different code table of one-byte characters, like
    other ISO-8859 pages, could be implemented by reusing the code for all
    functions, and just redefining the tables and strings according to
    code table and language to implement.

    (There's no need to actually change anything in the functions in any of
     these cases; it's just necessary to reuse them as access-functions for
     the tables since locale.library currently has no mechanism for just
     passing code tables or language tables. That's why you also don't have
     to include the code table arrays if none of the code-table functions
     are reused. Likewise, for a language using the same character order the
     only required table is the one including the language strings.)

    Most functions should be adaptable to multi-byte characters with very
    few alterations.
*/


/* Code table arrays for character type/conversion.
   The arrays defined later are those valid for ISO 8859/1.
*/
extern const UWORD __code_table_ctype[];
extern const ULONG __code_table_to_lower[];
extern const ULONG __code_table_to_upper[];
#define __CODE_TABLE_SIZE 256


/* Language arrays.
   The arrays defined later are valid for "english.language" only.
*/
extern const STRPTR __language_strings[];
extern const ULONG  __language_short_order_tab[];
extern const ULONG  __language_long_order_tab[];
extern ULONG AROS_SLIB_ENTRY( null, Locale)();


/* We use these in the code table ctype array to indicate the types that a
   character belongs to, and test against them in the IsXXXXX functions.
   They're not a requirement, though. If you implement your own IsXXXX
   functions you can do as you see fit.
   Note, however, that the code table functions in this file ought to be
   valid for any code table with single-byte characters. Replacing the
   values in the code table arrays ought to be enough to provide a language
   with support for a different code table.
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

/* This is not in the array. It's only used for testing against:
*/
#define iAlNum  (iAlpha | iDigit)  /* Alphanumerical character. */


/* ------------------------------------------------------------------
    Language Functions
   ------------------------------------------------------------------ */

/* ULONG ConvToLower(ULONG char): Language function 0
    This function converts the character char to the equivalent
    lower case value.
*/
AROS_LH1(ULONG, convtolower,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 6, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + __code_table_to_lower[(UBYTE)chr]);

    AROS_LIBFUNC_EXIT
}

/* ULONG ConvToUpper(ULONG char): Language Function 1
    This function converts the character char to the equivalent
    upper case character.
*/
AROS_LH1(ULONG, convtoupper,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 7, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + __code_table_to_upper[(UBYTE)chr]);

    AROS_LIBFUNC_EXIT
}

/* STRPTR GetLangString(ULONG num): Language function 3
    This function is called by GetLocaleStr() and should return
    the string matching the string id passed in as num.
*/
AROS_LH1(STRPTR, getlangstring,
    AROS_LHA(ULONG, id, D0),
    struct LocaleBase *, LocaleBase, 9, english)
{
    AROS_LIBFUNC_INIT

    if(id < MAXSTRMSG)
        return __language_strings[id];
    else
        return NULL;

    AROS_LIBFUNC_EXIT
}

/* BOOL IsXXXXX(ULONG char): Language functions 4-14
    These function are the same as the ANSI C isxxxxx() functions,
    however these will pay extra attention to the current language.

    This gives the advantage of allowing different character sets.
    Of course, this would also require using the relevant fonts, since
    without those the effects would be quite peculiar.
*/

AROS_LH1(ULONG, isalnum,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 10, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iAlNum));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, isalpha,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 11, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iAlpha));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, iscntrl,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 12, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iCntrl));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, isdigit,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 13, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iDigit));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, isgraph,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 14, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iGraph));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, islower,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 15, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iLower));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, isprint,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 16, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iPrint));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, ispunct,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 17, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iPunct));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, isspace,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 18, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iSpace));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, isupper,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 19, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iUpper));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, isxdigit,
    AROS_LHA(ULONG, chr, D0),
    struct LocaleBase *, LocaleBase, 20, english)
{
    AROS_LIBFUNC_INIT

    /* For compatability we have to use only lower byte, and preserve upper word! */
    return ((chr & ~0xFFFF) + (__code_table_ctype[(UBYTE)chr] & iXDigit));

    AROS_LIBFUNC_EXIT
}



/* ULONG strconvert(STRPTR s1, STRPTR s2, ULONG len, ULONG typ): LF 15
    This function will convert a string to automatically use the
    character order table. This is a bit dodgy in the opinion of some
    developers. However, the ANSI people saw this differently ...

    Since comparing pre-converted strings is not faster than comparing
    the strings themselves there should be very little reason to ever
    do this.
*/
AROS_LH4(ULONG, strconvert,
    AROS_LHA(STRPTR,    string1, A1),
    AROS_LHA(STRPTR,    string2, A2),
    AROS_LHA(ULONG,     length,  D0),
    AROS_LHA(ULONG,     type,    D1),
    struct LocaleBase *, LocaleBase, 21, english)
{
    AROS_LIBFUNC_INIT

    ULONG count = 0;
    const ULONG *collTab;

    if      (type == SC_ASCII)
        collTab = __code_table_to_upper;
    else if (type == SC_COLLATE1)
        collTab = __language_short_order_tab;
    else if (type == SC_COLLATE2)
        collTab = __language_long_order_tab;
    else /* Wrong argument: */
        return 0;

    if (length && string2)
    {
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
    or the collation (character order) information. This is explained in
    more detail in the data file, or in the autodoc...
*/
AROS_LH4(LONG, strcompare,
    AROS_LHA(STRPTR,    string1, A1),
    AROS_LHA(STRPTR,    string2, A2),
    AROS_LHA(LONG,      length,  D0),
    AROS_LHA(ULONG,     type,    D1),
    struct LocaleBase *, LocaleBase, 22, english)
{
    AROS_LIBFUNC_INIT

    if (string1 && string2)
    {   const ULONG *collTab;
        ULONG a= 0;

        /* Determine which collation table to use... */
        if      (type == SC_ASCII)
            collTab = __code_table_to_upper;
        else if (type == SC_COLLATE1)
            collTab = __language_short_order_tab;
        else if (type == SC_COLLATE2)
            collTab = __language_long_order_tab;
        else /* Wrong argument: */
            return 0;

        while ( length--
        &&      ! (a= ( collTab[(UBYTE)*string1]
                      - collTab[(UBYTE)*(string2++)]))
        &&      *string1++ );

        return a;
    }
    else if (length)
    {
        if (string1 && *string1)
            return  1; /* String1 exists therefore is bigger. */
        if (string2 && *string2)
            return -1; /* String2 exists therefore is bigger. */
    }

    return 0; /* Equal for the 0 characters we compared. */

    AROS_LIBFUNC_EXIT
}

/* -----------------------------------------------------------------------
   Library function table - you will need to alter this to indicate a
   different language.
   (It's right here at the end of the library so that there's no need for
   function-prototypes. Using prototypes would also be OK, though.)
   ----------------------------------------------------------------------- */

#ifdef __MORPHOS__
#define Xj(a,b) a##b
#define TRAPIT(n,s)  \
struct EmulLibEntry Xj(LIB_##n,_Gate) = { TRAP_LIB, 0, (void (*)(void)) LIB_##n }
#define AROS_SLIB_ENTRY_GATED(n,s) Xj(LIB_##n,_Gate)

    /* 0 - 3 */
    TRAPIT(convtolower, english);
    TRAPIT(convtoupper, english);
    TRAPIT(null, Locale);
    TRAPIT(getlangstring, english);

    /* 4 - 7 */
    TRAPIT(isalnum, english);
    TRAPIT(isalpha, english);
    TRAPIT(iscntrl, english);
    TRAPIT(isdigit, english);

    /* 8 - 11 */
    TRAPIT(isgraph, english);
    TRAPIT(islower, english);
    TRAPIT(isprint, english);
    TRAPIT(ispunct, english);

    /* 12 - 15 */
    TRAPIT(isspace, english);
    TRAPIT(isupper, english);
    TRAPIT(isxdigit, english);
    TRAPIT(strconvert, english);

    /* 16 */
    TRAPIT(strcompare, english);
#else
#define AROS_SLIB_ENTRY_GATED AROS_SLIB_ENTRY
#endif /*Morphos*/


void *const __eng_functable[] =
{
    /* 0 - 3 */
    &AROS_SLIB_ENTRY_GATED(convtolower, english),
    &AROS_SLIB_ENTRY_GATED(convtoupper, english),
    NULL,
    &AROS_SLIB_ENTRY_GATED(getlangstring, english),

    /* 4 - 7 */
    &AROS_SLIB_ENTRY_GATED(isalnum, english),
    &AROS_SLIB_ENTRY_GATED(isalpha, english),
    &AROS_SLIB_ENTRY_GATED(iscntrl, english),
    &AROS_SLIB_ENTRY_GATED(isdigit, english),

    /* 8 - 11 */
    &AROS_SLIB_ENTRY_GATED(isgraph, english),
    &AROS_SLIB_ENTRY_GATED(islower, english),
    &AROS_SLIB_ENTRY_GATED(isprint, english),
    &AROS_SLIB_ENTRY_GATED(ispunct, english),

    /* 12 - 15 */
    &AROS_SLIB_ENTRY_GATED(isspace, english),
    &AROS_SLIB_ENTRY_GATED(isupper, english),
    &AROS_SLIB_ENTRY_GATED(isxdigit, english),
    &AROS_SLIB_ENTRY_GATED(strconvert, english),

    /* 16 */
    &AROS_SLIB_ENTRY_GATED(strcompare, english),

    (void *)-1
};



/*  --------------------------------------------------------------------
    Language data:

    Whether your .language needs any of these tables depends on how it
    differs from english.
    - If it uses a different code table you'll need the code table
      defining tables.
    - If it uses a different character order (which will most likely
      also be true for languages using a different code table) you'll
      need the language character order tables.
    - If your .languages just different words and names, which is
      usually true, you'll need the language strings.

    If you're creating a custom .language:
    - If you're just changing the sorting behaviour you'll only need
      the language character order tables.
    - If you just want to change the names of things, eg. for a dialect,
      you only need the language strings (provided the dialect uses the
      regular English alphabet).

    -------------------------------------------------------------------- */


/* Code table defining tables:  Ctype, to-lower, to-upper.
   (The descriptions for non-ASCII characters is given in the comments for
    ctype.)
*/


/* Array for the IsXXXXX() functions:
   Each entry gives the basic types a character belongs to.
*/

const UWORD __code_table_ctype[ __CODE_TABLE_SIZE] =
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

        /* 160 Non-breaking space   */  iPrint | iPunct,
        /* 161 Inverted exclamation */  iGraph | iPrint | iPunct,
        /* 162 Cent                 */  iGraph | iPrint | iPunct,
        /* 163 Pound sterling       */  iGraph | iPrint | iPunct,
        /* 164 Currency             */  iGraph | iPrint | iPunct,
        /* 165 Yen                  */  iGraph | iPrint | iPunct,
        /* 166 Broken bar           */  iGraph | iPrint | iPunct,
        /* 167 Section              */  iGraph | iPrint | iPunct,
        /* 168 Dieresis/Umlaut      */  iGraph | iPrint | iPunct,
        /* 169 Copyright            */  iGraph | iPrint | iPunct,
        /* 170 Feminine ordinal     */  iGraph | iPrint | iPunct,
        /* 171 Left guillemot       */  iGraph | iPrint | iPunct,
        /* 172 Not                  */  iGraph | iPrint | iPunct,
        /* 173 Soft hyphen          */  iGraph | iPrint | iPunct,
        /* 174 Registered trademark */  iGraph | iPrint | iPunct,
        /* 175 Macron accent        */  iGraph | iPrint | iPunct,
        /* 176 Degree               */  iGraph | iPrint | iPunct,
        /* 177 Plus Minus           */  iGraph | iPrint | iPunct,
        /* 178 Superscript two      */  iGraph | iPrint | iPunct,
        /* 179 Superscript three    */  iGraph | iPrint | iPunct,
        /* 180 Acute accent         */  iGraph | iPrint | iPunct,
        /* 181 Mu                   */  iGraph | iPrint | iPunct,
        /* 182 Paragraph            */  iGraph | iPrint | iPunct,
        /* 183 Middle dot           */  iGraph | iPrint | iPunct,
        /* 184 Cedilla              */  iGraph | iPrint | iPunct,
        /* 185 Superscript one      */  iGraph | iPrint | iPunct,
        /* 186 Masculine ordinal    */  iGraph | iPrint | iPunct,
        /* 187 Right guillemot      */  iGraph | iPrint | iPunct,
        /* 188 One quarter          */  iGraph | iPrint | iPunct,
        /* 189 One half             */  iGraph | iPrint | iPunct,
        /* 190 Three quarters       */  iGraph | iPrint | iPunct,
        /* 191 Inverted question    */  iGraph | iPrint | iPunct,

        /* 192 Capital A grave      */  iAlpha | iGraph | iPrint | iUpper,
        /* 193 Capital A acute      */  iAlpha | iGraph | iPrint | iUpper,
        /* 194 Capital A circumflex */  iAlpha | iGraph | iPrint | iUpper,
        /* 195 Capital A tilde      */  iAlpha | iGraph | iPrint | iUpper,
        /* 196 Capital A dieresis   */  iAlpha | iGraph | iPrint | iUpper,
        /* 197 Capital A ring       */  iAlpha | iGraph | iPrint | iUpper,
        /* 198 Capital AE           */  iAlpha | iGraph | iPrint | iUpper,
        /* 199 Capital C cedilla    */  iAlpha | iGraph | iPrint | iUpper,
        /* 200 Capital E grave      */  iAlpha | iGraph | iPrint | iUpper,
        /* 201 Capital E acute      */  iAlpha | iGraph | iPrint | iUpper,
        /* 202 Capital E circumflex */  iAlpha | iGraph | iPrint | iUpper,
        /* 203 Capital E dieresis   */  iAlpha | iGraph | iPrint | iUpper,
        /* 204 Capital I grave      */  iAlpha | iGraph | iPrint | iUpper,
        /* 205 Capital I acute      */  iAlpha | iGraph | iPrint | iUpper,
        /* 206 Capital I circumflex */  iAlpha | iGraph | iPrint | iUpper,
        /* 207 Capital I dieresis   */  iAlpha | iGraph | iPrint | iUpper,
        /* 208 Capital Eth          */  iAlpha | iGraph | iPrint | iUpper,
        /* 209 Capital N tilde      */  iAlpha | iGraph | iPrint | iUpper,
        /* 210 Capital O grave      */  iAlpha | iGraph | iPrint | iUpper,
        /* 211 Capital O acute      */  iAlpha | iGraph | iPrint | iUpper,
        /* 212 Capital O circumflex */  iAlpha | iGraph | iPrint | iUpper,
        /* 213 Capital O tilde      */  iAlpha | iGraph | iPrint | iUpper,
        /* 214 Capital O dieresis   */  iAlpha | iGraph | iPrint | iUpper,
        /* 215 Multiply             */  iGraph | iPrint | iPunct,
        /* 216 Capital O slash      */  iAlpha | iGraph | iPrint | iUpper,
        /* 217 Capital U grave      */  iAlpha | iGraph | iPrint | iUpper,
        /* 218 Capital U acute      */  iAlpha | iGraph | iPrint | iUpper,
        /* 219 Capital U circumflex */  iAlpha | iGraph | iPrint | iUpper,
        /* 220 Capital U dieresis   */  iAlpha | iGraph | iPrint | iUpper,
        /* 221 Capital Y acute      */  iAlpha | iGraph | iPrint | iUpper,
        /* 222 Capital Thorn        */  iAlpha | iGraph | iPrint | iUpper,
        /* 223 Ringel-s             */  iAlpha | iGraph | iLower | iPrint | iUpper,

        /* 224 Small a grave        */  iAlpha | iGraph | iLower | iPrint,
        /* 225 Small a acute        */  iAlpha | iGraph | iLower | iPrint,
        /* 226 Small a circumflex   */  iAlpha | iGraph | iLower | iPrint,
        /* 227 Small a tilde        */  iAlpha | iGraph | iLower | iPrint,
        /* 228 Small a dieresis     */  iAlpha | iGraph | iLower | iPrint,
        /* 229 Small a ring         */  iAlpha | iGraph | iLower | iPrint,
        /* 230 Small ae             */  iAlpha | iGraph | iLower | iPrint,
        /* 231 Small c cedilla      */  iAlpha | iGraph | iLower | iPrint,
        /* 232 Small e grave        */  iAlpha | iGraph | iLower | iPrint,
        /* 233 Small e acute        */  iAlpha | iGraph | iLower | iPrint,
        /* 234 Small e circumflex   */  iAlpha | iGraph | iLower | iPrint,
        /* 235 Small e dieresis     */  iAlpha | iGraph | iLower | iPrint,
        /* 236 Small i grave        */  iAlpha | iGraph | iLower | iPrint,
        /* 237 Small i acute        */  iAlpha | iGraph | iLower | iPrint,
        /* 238 Small i circumflex   */  iAlpha | iGraph | iLower | iPrint,
        /* 239 Small i dieresis     */  iAlpha | iGraph | iLower | iPrint,
        /* 240 Small eth            */  iAlpha | iGraph | iLower | iPrint,
        /* 241 Small n tilde        */  iAlpha | iGraph | iLower | iPrint,
        /* 242 Small o grave        */  iAlpha | iGraph | iLower | iPrint,
        /* 243 Small o acute        */  iAlpha | iGraph | iLower | iPrint,
        /* 244 Small o circumflex   */  iAlpha | iGraph | iLower | iPrint,
        /* 245 Small o tilde        */  iAlpha | iGraph | iLower | iPrint,
        /* 246 Small o dieresis     */  iAlpha | iGraph | iLower | iPrint,
        /* 247 Division             */  iGraph | iPrint | iPunct,
        /* 248 Small o slash        */  iAlpha | iGraph | iLower | iPrint,
        /* 249 Small u grave        */  iAlpha | iGraph | iLower | iPrint,
        /* 250 Small u acute        */  iAlpha | iGraph | iLower | iPrint,
        /* 251 Small u circumflex   */  iAlpha | iGraph | iLower | iPrint,
        /* 252 Small u dieresis     */  iAlpha | iGraph | iLower | iPrint,
        /* 253 Small y acute        */  iAlpha | iGraph | iLower | iPrint,
        /* 254 Small thorn          */  iAlpha | iGraph | iLower | iPrint,
        /* 255 Small y dieresis     */  iAlpha | iGraph | iLower | iPrint,
};



/* Array for the ConvToLower function:
   Each entry gives the lower case version of a character.
   If the character is already in lower case the entry holds the
   character itself.
*/

const ULONG __code_table_to_lower[ __CODE_TABLE_SIZE] =
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

        ' ',            /* ' ' */        '!',            /* '!' */
        '\"',           /* '"' */        '#',            /* '#' */
        '$',            /* '$' */        '%',            /* '%' */
        '&',            /* '&' */        '\'',           /* ''' */
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
        '>',            /* '>' */        '\?',           /* '?' */

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
        '\\',           /* '\' */        ']',            /* ']' */
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

        160,            /* 160 */        161,            /* 161 */
        162,            /* 162 */        163,            /* 163 */
        164,            /* 164 */        165,            /* 165 */
        166,            /* 166 */        167,            /* 167 */
        168,            /* 168 */        169,            /* 169 */
        170,            /* 170 */        171,            /* 171 */
        172,            /* 172 */        173,            /* 173 */
        174,            /* 174 */        175,            /* 175 */
        176,            /* 176 */        177,            /* 177 */
        178,            /* 178 */        179,            /* 179 */
        180,            /* 180 */        181,            /* 181 */
        182,            /* 182 */        183,            /* 183 */
        184,            /* 184 */        185,            /* 185 */
        186,            /* 186 */        187,            /* 187 */
        188,            /* 188 */        189,            /* 189 */
        190,            /* 190 */        191,            /* 191 */

        224,            /* 192 */        225,            /* 193 */
        226,            /* 194 */        227,            /* 195 */
        228,            /* 196 */        229,            /* 197 */
        230,            /* 198 */        231,            /* 199 */
        232,            /* 200 */        233,            /* 201 */
        234,            /* 202 */        235,            /* 203 */
        236,            /* 204 */        237,            /* 205 */
        238,            /* 206 */        239,            /* 207 */
        240,            /* 208 */        241,            /* 209 */
        242,            /* 210 */        243,            /* 211 */
        244,            /* 212 */        245,            /* 213 */
        246,            /* 214 */        215,            /* 215 Multiply */
        248,            /* 216 */        249,            /* 217 */
        250,            /* 218 */        251,            /* 219 */
        252,            /* 220 */        253,            /* 221 */
        254,            /* 222 */        223,            /* 223 Ringel-S */

        224,            /* 224 */        225,            /* 225 */
        226,            /* 226 */        227,            /* 227 */
        228,            /* 228 */        229,            /* 229 */
        230,            /* 230 */        231,            /* 231 */
        232,            /* 232 */        233,            /* 233 */
        234,            /* 234 */        235,            /* 235 */
        236,            /* 236 */        237,            /* 237 */
        238,            /* 238 */        239,            /* 239 */
        240,            /* 240 */        241,            /* 241 */
        242,            /* 242 */        243,            /* 243 */
        244,            /* 244 */        245,            /* 245 */
        246,            /* 246 */        247,            /* 247 */
        248,            /* 248 */        249,            /* 249 */
        250,            /* 250 */        251,            /* 251 */
        252,            /* 252 */        253,            /* 253 */
        254,            /* 254 */        255,            /* 255 */
};


/* Array for the ConvToUpper function:
   Each entry gives the upper case version of a character.
   If the character is already in upper case the entry holds the
   character itself.
*/

const ULONG __code_table_to_upper[ __CODE_TABLE_SIZE] =
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

        ' ',            /* 32  */        '!',            /* '!' */
        '\"',           /* '"' */        '#',            /* '#' */
        '$',            /* '$' */        '%',            /* '%' */
        '&',            /* '&' */        '\'',           /* ''' */
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
        '>',            /* '>' */        '\?',           /* '?' */

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
        '\\',           /* '\' */        ']',            /* ']' */
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

        160,            /* 160 */        161,            /* 161 */
        162,            /* 162 */        163,            /* 163 */
        164,            /* 164 */        165,            /* 165 */
        166,            /* 166 */        167,            /* 167 */
        168,            /* 168 */        169,            /* 169 */
        170,            /* 170 */        171,            /* 171 */
        172,            /* 172 */        173,            /* 173 */
        174,            /* 174 */        175,            /* 175 */
        176,            /* 176 */        177,            /* 177 */
        178,            /* 178 */        179,            /* 179 */
        180,            /* 180 */        181,            /* 181 */
        182,            /* 182 */        183,            /* 183 */
        184,            /* 184 */        185,            /* 185 */
        186,            /* 186 */        187,            /* 187 */
        188,            /* 188 */        189,            /* 189 */
        190,            /* 190 */        191,            /* 191 */

        192,            /* 192 */        193,            /* 193 */
        194,            /* 194 */        195,            /* 195 */
        196,            /* 196 */        197,            /* 197 */
        198,            /* 198 */        199,            /* 199 */
        200,            /* 200 */        201,            /* 201 */
        202,            /* 202 */        203,            /* 203 */
        204,            /* 204 */        205,            /* 205 */
        206,            /* 206 */        207,            /* 207 */
        208,            /* 208 */        209,            /* 209 */
        210,            /* 210 */        211,            /* 211 */
        212,            /* 212 */        213,            /* 213 */
        214,            /* 214 */        215,            /* 215 */
        216,            /* 216 */        217,            /* 217 */
        218,            /* 218 */        219,            /* 219 */
        220,            /* 220 */        221,            /* 221 */
        222,            /* 222 */        223,            /* 223 */

        192,            /* 224 */        193,            /* 225 */
        194,            /* 226 */        195,            /* 227 */
        196,            /* 228 */        197,            /* 229 */
        198,            /* 230 */        199,            /* 231 */
        200,            /* 232 */        201,            /* 233 */
        202,            /* 234 */        203,            /* 235 */
        204,            /* 236 */        205,            /* 237 */
        206,            /* 238 */        207,            /* 239 */
        208,            /* 240 */        209,            /* 241 */
        210,            /* 242 */        211,            /* 243 */
        212,            /* 244 */        213,            /* 245 */
        214,            /* 246 */        247,            /* 247 Division */
        216,            /* 248 */        217,            /* 249 */
        218,            /* 250 */        219,            /* 251 */
        220,            /* 252 */        221,            /* 253 */
        222,            /* 254 */        255,            /* 255 Small y dieresis */
};



/* Language character order tables:   Short order, long order.
   (These values are not really characters. Rather, they should be
    considered indices for implied sorting tables.)
*/


/*
    This is the short character order table (COLLATE1).

    This defines a sorting order for the regular characters of the language
    only. Sorting using this order will place others characters together
    with the regular characters they are related to.

    This is done by giving for each character the regular character to be
    used for sorting instead.

    Eg. the entry for 'a' is 'A', so while sorting 'a' will be considered
    the same as 'A'. Thus sorting will place "amiga" and "Amiga" immediately
    together under "A" (in no particular order), rather on different sides
    of "Z-80", as would happen by sorting just by character values. Though
    this simple example could also be done using ToUpper, the short
    character order also groups all accented characters with their base
    character.

    (Though for regular English characters the ordinals have here been
     chosen equal to the character values, there's no need to do so. It's
     also not true for other characters. Eg. the value of '~' is 126, but
     it's ordered here as the 100-th character.)
*/

const ULONG __language_short_order_tab[ __CODE_TABLE_SIZE] =
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

        ' ',            /* ' ' */        '!',            /* '!' */
        '\"',           /* '"' */        '#',            /* '#' */
        '$',            /* '$' */        '%',            /* '%' */
        '&',            /* '&' */        '\'',           /* ''' */
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
        '>',            /* '>' */        '\?',           /* '?' */

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
        '\\',           /* '\' */        ']',            /* ']' */
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
        'Z',            /* 'z' */        97 ,            /* '{' */
        98 ,            /* '|' */        99 ,            /* '}' */
        100,            /* '~' */        101,            /* 127 */

        224,            /* 128 */        225,            /* 129 */
        226,            /* 130 */        227,            /* 131 */
        228,            /* 132 */        229,            /* 133 */
        230,            /* 134 */        231,            /* 135 */
        232,            /* 136 */        233,            /* 137 */
        234,            /* 138 */        235,            /* 139 */
        236,            /* 140 */        237,            /* 141 */
        238,            /* 142 */        239,            /* 143 */
        240,            /* 144 */        241,            /* 145 */
        242,            /* 146 */        243,            /* 147 */
        244,            /* 148 */        245,            /* 149 */
        246,            /* 150 */        247,            /* 151 */
        248,            /* 152 */        249,            /* 153 */
        250,            /* 154 */        251,            /* 155 */
        252,            /* 156 */        253,            /* 157 */
        254,            /* 158 */        255,            /* 159 */

        ' ',            /* 160 */        '!',            /* 161 */
        '$',            /* 162 */        '$',            /* 163 */
        103,            /* 164 */        104,            /* 165 */
        105,            /* 166 */        'S',            /* 167 */
        106,            /* 168 */        107,            /* 169 */
        108,            /* 170 */        '\"',           /* 171 */
        109,            /* 172 */        110,            /* 173 */
        111,            /* 174 */        112,            /* 175 */
        112,            /* 176 */        114,            /* 177 */
        115,            /* 178 */        116,            /* 179 */
        117,            /* 180 */        118,            /* 181 */
        119,            /* 182 */        120,            /* 183 */
        121,            /* 184 */        122,            /* 185 */
        123,            /* 186 */        '\"',           /* 187 */
        124,            /* 188 */        125,            /* 189 */
        126,            /* 190 */        '\?',           /* 191 */

        'A',            /* 192 */        'A',            /* 193 */
        'A',            /* 194 */        'A',            /* 195 */
        'A',            /* 196 */        'A',            /* 197 */
        'A',            /* 198 */        'C',            /* 199 */
        'E',            /* 200 */        'E',            /* 201 */
        'E',            /* 202 */        'E',            /* 203 */
        'I',            /* 204 */        'I',            /* 205 */
        'I',            /* 206 */        'I',            /* 207 */
        'D',            /* 208 */        'N',            /* 209 */
        'O',            /* 210 */        'O',            /* 211 */
        'O',            /* 212 */        'O',            /* 213 */
        'O',            /* 214 */        '*',            /* 215 */
        'O',            /* 216 */        'U',            /* 217 */
        'U',            /* 218 */        'U',            /* 219 */
        'U',            /* 220 */        'Y',            /* 221 */
        'T',            /* 222 */        'S',            /* 223 */

        'A',            /* 224 */        'A',            /* 225 */
        'A',            /* 226 */        'A',            /* 227 */
        'A',            /* 228 */        'A',            /* 229 */
        'A',            /* 230 */        'C',            /* 231 */
        'E',            /* 232 */        'E',            /* 233 */
        'E',            /* 234 */        'E',            /* 235 */
        'I',            /* 236 */        'I',            /* 237 */
        'I',            /* 238 */        'I',            /* 239 */
        'D',            /* 240 */        'N',            /* 241 */
        'O',            /* 242 */        'O',            /* 243 */
        'O',            /* 244 */        'O',            /* 245 */
        'O',            /* 246 */        '/',            /* 247 */
        'O',            /* 248 */        'U',            /* 249 */
        'U',            /* 250 */        'U',            /* 251 */
        'U',            /* 252 */        'Y',            /* 253 */
        'T',            /* 254 */        'Y',            /* 255 */
};



/*
    This is the long character order table (COLLATE2).

    This defines a sorting order for all the characters of the code set.
    Sorting using this order will group related characters together, but
    will nevertheless sort out the different characters.

    This is done by giving for each character a different ordinal, where the
    ordinals for related characters are "adjacent".

    Eg. the ordinal for 'A' is 74, and ordinal for 'a' is 75. Thus sorting
    will place "amiga" shortly after "Amiga", but "Amy" will be sorted
    closer since it too starts with ordinal 74. Though this is a simple
    example, for all accented characters the long character order also
    defines ordinals adjacent to that of their base characters.

    (Characters will usually have an ordinal different from their value.)
*/
const ULONG __language_long_order_tab[ __CODE_TABLE_SIZE] =
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

        32 ,            /* ' ' */        34 ,            /* '!' */
        36 ,            /* '"' */        39 ,            /* '#' */
        40 ,            /* '$' */        43 ,            /* '%' */
        44 ,            /* '&' */        45 ,            /* ''' */
        46 ,            /* '(' */        47 ,            /* ')' */
        48 ,            /* '*' */        50 ,            /* '+' */
        51 ,            /* ',' */        52 ,            /* '-' */
        53 ,            /* '.' */        54 ,            /* '/' */
        56 ,            /* '0' */        57 ,            /* '1' */
        58 ,            /* '2' */        59 ,            /* '3' */
        60 ,            /* '4' */        61 ,            /* '5' */
        62 ,            /* '6' */        63 ,            /* '7' */
        64 ,            /* '8' */        65 ,            /* '9' */
        66 ,            /* ':' */        67 ,            /* ';' */
        68 ,            /* '<' */        69 ,            /* '=' */
        70 ,            /* '>' */        71 ,            /* '?' */

        73 ,            /* '@' */        74 ,            /* 'A' */
        90 ,            /* 'B' */        92 ,            /* 'C' */
        96 ,            /* 'D' */        100,            /* 'E' */
        110,            /* 'F' */        112,            /* 'G' */
        114,            /* 'H' */        116,            /* 'I' */
        126,            /* 'J' */        128,            /* 'K' */
        130,            /* 'L' */        132,            /* 'M' */
        134,            /* 'N' */        138,            /* 'O' */
        152,            /* 'P' */        154,            /* 'Q' */
        156,            /* 'R' */        158,            /* 'S' */
        162,            /* 'T' */        166,            /* 'U' */
        176,            /* 'V' */        178,            /* 'W' */
        180,            /* 'X' */        182,            /* 'Y' */
        187,            /* 'Z' */        189,            /* '[' */
        190,            /* '\' */        191,            /* ']' */
        192,            /* '^' */        193,            /* '_' */

        194,            /* '`' */        75 ,            /* 'a' */
        91 ,            /* 'b' */        93 ,            /* 'c' */
        97 ,            /* 'd' */        101,            /* 'e' */
        111,            /* 'f' */        113,            /* 'g' */
        115,            /* 'h' */        117,            /* 'i' */
        127,            /* 'j' */        129,            /* 'k' */
        131,            /* 'l' */        133,            /* 'm' */
        135,            /* 'n' */        139,            /* 'o' */
        153,            /* 'p' */        155,            /* 'q' */
        157,            /* 'r' */        159,            /* 's' */
        163,            /* 't' */        167,            /* 'u' */
        177,            /* 'v' */        179,            /* 'w' */
        181,            /* 'x' */        183,            /* 'y' */
        188,            /* 'z' */        195,            /* '{' */
        196,            /* '|' */        197,            /* '}' */
        198,            /* '~' */        199,            /* 127 */

        224,            /* 128 */        225,            /* 129 */
        226,            /* 130 */        227,            /* 131 */
        228,            /* 132 */        229,            /* 133 */
        230,            /* 134 */        231,            /* 135 */
        232,            /* 136 */        233,            /* 137 */
        234,            /* 138 */        235,            /* 139 */
        236,            /* 140 */        237,            /* 141 */
        238,            /* 142 */        239,            /* 143 */
        240,            /* 144 */        241,            /* 145 */
        242,            /* 146 */        243,            /* 147 */
        244,            /* 148 */        245,            /* 149 */
        246,            /* 150 */        247,            /* 151 */
        248,            /* 152 */        249,            /* 153 */
        250,            /* 154 */        251,            /* 155 */
        252,            /* 156 */        253,            /* 157 */
        254,            /* 158 */        255,            /* 159 */

        33 ,            /* 160 */        35 ,            /* 161 */
        41 ,            /* 162 */        42 ,            /* 163 */
        200,            /* 164 */        201,            /* 165 */
        202,            /* 166 */        160,            /* 167 */
        203,            /* 168 */        204,            /* 169 */
        205,            /* 170 */        37 ,            /* 171 */
        206,            /* 172 */        207,            /* 173 */
        208,            /* 174 */        209,            /* 175 */
        210,            /* 176 */        211,            /* 177 */
        212,            /* 178 */        213,            /* 179 */
        214,            /* 180 */        215,            /* 181 */
        216,            /* 182 */        217,            /* 183 */
        218,            /* 184 */        219,            /* 185 */
        220,            /* 186 */        38 ,            /* 187 */
        221,            /* 188 */        222,            /* 189 */
        223,            /* 190 */        72 ,            /* 191 */

        76 ,            /* 192 */        77 ,            /* 193 */
        78 ,            /* 194 */        79 ,            /* 195 */
        80 ,            /* 196 */        81 ,            /* 197 */
        82 ,            /* 198 */        94 ,            /* 199 */
        102 ,           /* 200 */        103,            /* 201 */
        104,            /* 202 */        105,            /* 203 */
        118,            /* 204 */        119,            /* 205 */
        120,            /* 206 */        121,            /* 207 */
        98 ,            /* 208 */        136,            /* 209 */
        140,            /* 210 */        141,            /* 211 */
        142,            /* 212 */        143,            /* 213 */
        144,            /* 214 */        49 ,            /* 215 */
        145,            /* 216 */        168,            /* 217 */
        169,            /* 218 */        170,            /* 219 */
        171,            /* 220 */        184,            /* 221 */
        164,            /* 222 */        161,            /* 223 */

        83 ,            /* 224 */        84 ,            /* 225 */
        85 ,            /* 226 */        86 ,            /* 227 */
        87 ,            /* 228 */        88 ,            /* 229 */
        89 ,            /* 230 */        95 ,            /* 231 */
        106,            /* 232 */        107,            /* 233 */
        108,            /* 234 */        109,            /* 235 */
        122,            /* 236 */        123,            /* 237 */
        124,            /* 238 */        125,            /* 239 */
        99 ,            /* 240 */        137,            /* 241 */
        146,            /* 242 */        147,            /* 243 */
        148,            /* 244 */        149,            /* 245 */
        150,            /* 246 */        55 ,            /* 247 */
        151,            /* 248 */        172,            /* 249 */
        173,            /* 250 */        174,            /* 251 */
        175,            /* 252 */        185,            /* 253 */
        165,            /* 254 */        186,            /* 255 */
};


/* --------------------------------------------------------------------

    This is the list of strings. It is an array of pointers to strings,
    although how it is laid out is implementation dependant.
*/
const STRPTR __language_strings[] =
{
    /* A blank string: */
    "",

    /*  The days of the week. Starts with the day which is called "Sunday"
        in English. Always used this order: What day the calender actually
        starts a week with is set using Locale->CalendarType.
    */
    "Sunday"   ,
    "Monday"   ,
    "Tuesday"  ,
    "Wednesday",
    "Thursday" ,
    "Friday"   ,
    "Saturday" ,

    /* Abbreviated days of the week: */
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",

    /* Months of the year: */
    "January"  ,
    "February" ,
    "March"    ,
    "April"    ,
    "May"      ,
    "June"     ,
    "July"     ,
    "August"   ,
    "September",
    "October"  ,
    "November" ,
    "December" ,

    /* Abbreviated months of the year: */
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",

    /* Yes and No: */
    "Yes",  /* Yes - affirmative response; */
    "No" ,  /* No  - negative response;    */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359: */
    "am",
    "pm",

    /* Soft and hard hyphens: */
    "-",  /* Soft; */
    "-",  /* Hard; */

    /* Open and close quotes */
    "\"",
    "\"",

    /* Days: Relative day names: */
    "Yesterday",  /* Yesterday - the day before the current;  */
    "Today"    ,  /* Today     - the current day;             */
    "Tomorrow" ,  /* Tomorrow  - the next day;                */
    "Future"      /* Future    - all days beyond the current. */
};


/* This is the end-of-ROMtag marker. */
const char end=0;

