/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <libraries/stdc.h>

#include <ctype.h>

static const unsigned short int __ctype_b_array[256] =
{
    _ctype_cntrl, /* 0 */
    _ctype_cntrl, /* 1 */
    _ctype_cntrl, /* 2 */
    _ctype_cntrl, /* 3 */
    _ctype_cntrl, /* 4 */
    _ctype_cntrl, /* 5 */
    _ctype_cntrl, /* 6 */
    _ctype_cntrl, /* 7 */
    _ctype_cntrl, /* Backspace */
    _ctype_blank|_ctype_cntrl|_ctype_space, /* 9 */
    _ctype_cntrl|_ctype_space, /* LF */
    _ctype_cntrl|_ctype_space, /* 11 */
    _ctype_cntrl|_ctype_space, /* 12 */
    _ctype_cntrl|_ctype_space, /* CR */
    _ctype_cntrl, /* 14 */
    _ctype_cntrl, /* 15 */
    _ctype_cntrl, /* 16 */
    _ctype_cntrl, /* 17 */
    _ctype_cntrl, /* 18 */
    _ctype_cntrl, /* 19 */
    _ctype_cntrl, /* 20 */
    _ctype_cntrl, /* 21 */
    _ctype_cntrl, /* 22 */
    _ctype_cntrl, /* 23 */
    _ctype_cntrl, /* 24 */
    _ctype_cntrl, /* 25 */
    _ctype_cntrl, /* 26 */
    _ctype_cntrl, /* ESC */
    _ctype_cntrl, /* 28 */
    _ctype_cntrl, /* 29 */
    _ctype_cntrl, /* 30 */
    _ctype_cntrl, /* 31 */
    _ctype_blank|_ctype_print|_ctype_space, /* Space */
    _ctype_graph|_ctype_print|_ctype_punct, /* ! */
    _ctype_graph|_ctype_print|_ctype_punct, /* " */
    _ctype_graph|_ctype_print|_ctype_punct, /* # */
    _ctype_graph|_ctype_print|_ctype_punct, /* $ */
    _ctype_graph|_ctype_print|_ctype_punct, /* % */
    _ctype_graph|_ctype_print|_ctype_punct, /* & */
    _ctype_graph|_ctype_print|_ctype_punct, /* ' */
    _ctype_graph|_ctype_print|_ctype_punct, /* ( */
    _ctype_graph|_ctype_print|_ctype_punct, /* ) */
    _ctype_graph|_ctype_print|_ctype_punct, /* * */
    _ctype_graph|_ctype_print|_ctype_punct, /* + */
    _ctype_graph|_ctype_print|_ctype_punct, /* , */
    _ctype_graph|_ctype_print|_ctype_punct, /* - */
    _ctype_graph|_ctype_print|_ctype_punct, /* . */
    _ctype_graph|_ctype_print|_ctype_punct, /* / */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 0 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 1 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 2 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 3 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 4 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 5 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 6 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 7 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 8 */
    _ctype_digit|_ctype_graph|_ctype_print|_ctype_xdigit, /* 9 */
    _ctype_graph|_ctype_print|_ctype_punct, /* : */
    _ctype_graph|_ctype_print|_ctype_punct, /* ; */
    _ctype_graph|_ctype_print|_ctype_punct, /* < */
    _ctype_graph|_ctype_print|_ctype_punct, /* = */
    _ctype_graph|_ctype_print|_ctype_punct, /* > */
    _ctype_graph|_ctype_print|_ctype_punct, /* ? */
    _ctype_graph|_ctype_print|_ctype_punct, /* @ */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* A */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* B */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* C */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* D */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* E */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* F */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* G */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* H */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* I */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* J */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* K */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* L */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* M */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* N */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* O */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* P */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* Q */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* R */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* S */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* T */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* U */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* V */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* W */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* X */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* Y */
    _ctype_upper|_ctype_alpha|_ctype_graph|_ctype_print, /* Z */
    _ctype_graph|_ctype_print|_ctype_punct, /* [ */
    _ctype_graph|_ctype_print|_ctype_punct, /* \ */
    _ctype_graph|_ctype_print|_ctype_punct, /* ] */
    _ctype_graph|_ctype_print|_ctype_punct, /* ^ */
    _ctype_graph|_ctype_print|_ctype_punct, /* _ */
    _ctype_graph|_ctype_print|_ctype_punct, /* ` */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* a */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* b */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* c */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* d */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* e */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print|_ctype_xdigit, /* f */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* g */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* h */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* i */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* j */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* k */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* l */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* m */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* n */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* o */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* p */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* q */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* r */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* s */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* t */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* u */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* v */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* w */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* x */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* y */
    _ctype_lower|_ctype_alpha|_ctype_graph|_ctype_print, /* z */
    _ctype_graph|_ctype_print|_ctype_punct, /* { */
    _ctype_graph|_ctype_print|_ctype_punct, /* | */
    _ctype_graph|_ctype_print|_ctype_punct, /* } */
    _ctype_graph|_ctype_print|_ctype_punct, /* ~ */
    _ctype_cntrl, /* Del */
    0, /* 128 */
    0, /* 129 */
    0, /* 130 */
    0, /* 131 */
    0, /* 132 */
    0, /* 133 */
    0, /* 134 */
    0, /* 135 */
    0, /* 136 */
    0, /* 137 */
    0, /* 138 */
    0, /* 139 */
    0, /* 140 */
    0, /* 141 */
    0, /* 142 */
    0, /* 143 */
    0, /* 144 */
    0, /* 145 */
    0, /* 146 */
    0, /* 147 */
    0, /* 148 */
    0, /* 149 */
    0, /* 150 */
    0, /* 151 */
    0, /* 152 */
    0, /* 153 */
    0, /* 154 */
    0, /* CSI */
    0, /* 156 */
    0, /* 157 */
    0, /* 158 */
    0, /* 159 */
    0, /* NBSpace */
    0, /* ¡ */
    0, /* ¢ */
    0, /* £ */
    0, /* ¤ */
    0, /* ¥ */
    0, /* ¦ */
    0, /* § */
    0, /* ¨ */
    0, /* © */
    0, /* ª */
    0, /* « */
    0, /* ¬ */
    0, /* ­ */
    0, /* ® */
    0, /* ¯ */
    0, /* ° */
    0, /* ± */
    0, /* ² */
    0, /* ³ */
    0, /* ´ */
    0, /* µ */
    0, /* ¶ */
    0, /* · */
    0, /* ¸ */
    0, /* ¹ */
    0, /* º */
    0, /* » */
    0, /* ¼ */
    0, /* ½ */
    0, /* ¾ */
    0, /* ¿ */
    0, /* À */
    0, /* Á */
    0, /* Â */
    0, /* Ã */
    0, /* Ä */
    0, /* Å */
    0, /* Æ */
    0, /* Ç */
    0, /* È */
    0, /* É */
    0, /* Ê */
    0, /* Ë */
    0, /* Ì */
    0, /* Í */
    0, /* Î */
    0, /* Ï */
    0, /* Ð */
    0, /* Ñ */
    0, /* Ò */
    0, /* Ó */
    0, /* Ô */
    0, /* Õ */
    0, /* Ö */
    0, /* × */
    0, /* Ø */
    0, /* Ù */
    0, /* Ú */
    0, /* Û */
    0, /* Ü */
    0, /* Ý */
    0, /* Þ */
    0, /* ß */
    0, /* à */
    0, /* á */
    0, /* â */
    0, /* ã */
    0, /* ä */
    0, /* å */
    0, /* æ */
    0, /* ç */
    0, /* è */
    0, /* é */
    0, /* ê */
    0, /* ë */
    0, /* ì */
    0, /* í */
    0, /* î */
    0, /* ï */
    0, /* ð */
    0, /* ñ */
    0, /* ò */
    0, /* ó */
    0, /* ô */
    0, /* õ */
    0, /* ö */
    0, /* ÷ */
    0, /* ø */
    0, /* ù */
    0, /* ú */
    0, /* û */
    0, /* ü */
    0, /* ý */
    0, /* þ */
    0, /* ÿ */
};

static const unsigned char __ctype_toupper_array[256] =
{
      0,  1,  2,  3,   4,  5,  6,  7,
      8,  9, 10, 11,  12, 13, 14, 15,
     16, 17, 18, 19,  20, 21, 22, 23,
     24, 25, 26, 27,  28, 29, 30, 31,
    ' ','!','"','#', '$','%','&','\'',
    '(',')','*','+', ',','-','.','/',
    '0','1','2','3', '4','5','6','7',
    '8','9',':',';', '<','=','>','?',
    '@','A','B','C', 'D','E','F','G',
    'H','I','J','K', 'L','M','N','O',
    'P','Q','R','S', 'T','U','V','W',
    'X','Y','Z','[', '\\',']','^','_',
    '`','A','B','C', 'D','E','F','G',
    'H','I','J','K', 'L','M','N','O',
    'P','Q','R','S', 'T','U','V','W',
    'X','Y','Z','{', '|','}','~',127,

    128,129,130,131, 132,133,134,135,
    136,137,138,139, 140,141,142,143,
    144,145,146,147, 148,149,150,151,
    152,153,154,155, 156,157,158,159,
    160,161,162,163, 164,165,166,167,
    168,169,170,171, 172,173,174,175,
    176,177,178,179, 180,181,182,183,
    184,185,186,187, 188,189,190,191,
    192,193,194,195, 196,197,198,199,
    200,201,202,203, 204,205,206,207,
    208,209,210,211, 212,213,214,215,
    216,217,218,219, 220,221,222,223,
    224,225,226,227, 228,229,230,231,
    232,233,234,235, 236,237,238,239,
    240,241,242,243, 244,245,246,247,
    248,249,250,251, 252,253,254,255,
};

static const unsigned char __ctype_tolower_array[256] =
{
      0,  1,  2,  3,   4,  5,  6,  7,
      8,  9, 10, 11,  12, 13, 14, 15,
     16, 17, 18, 19,  20, 21, 22, 23,
     24, 25, 26, 27,  28, 29, 30, 31,
    ' ','!','"','#', '$','%','&','\'',
    '(',')','*','+', ',','-','.','/',
    '0','1','2','3', '4','5','6','7',
    '8','9',':',';', '<','=','>','?',
    '@','a','b','c', 'd','e','f','g',
    'h','i','j','k', 'l','m','n','o',
    'p','q','r','s', 't','u','v','w',
    'x','y','z','[', '\\',']','^','_',
    '`','a','b','c', 'd','e','f','g',
    'h','i','j','k', 'l','m','n','o',
    'p','q','r','s', 't','u','v','w',
    'x','y','z','{', '|','}','~',127,

    128,129,130,131, 132,133,134,135,
    136,137,138,139, 140,141,142,143,
    144,145,146,147, 148,149,150,151,
    152,153,154,155, 156,157,158,159,
    160,161,162,163, 164,165,166,167,
    168,169,170,171, 172,173,174,175,
    176,177,178,179, 180,181,182,183,
    184,185,186,187, 188,189,190,191,
    192,193,194,195, 196,197,198,199,
    200,201,202,203, 204,205,206,207,
    208,209,210,211, 212,213,214,215,
    216,217,218,219, 220,221,222,223,
    224,225,226,227, 228,229,230,231,
    232,233,234,235, 236,237,238,239,
    240,241,242,243, 244,245,246,247,
    248,249,250,251, 252,253,254,255,
};

/* Pointers have to be available both when in static linklib and
   internally to stdc.library also
*/
const unsigned short int * const __ctype_b = &__ctype_b_array[0];
const unsigned char * const __ctype_toupper = &__ctype_toupper_array[0];
const unsigned char * const __ctype_tolower = &__ctype_tolower_array[0];

const unsigned short int * const * const __ctype_b_ptr = &__ctype_b;
const unsigned char * const * const __ctype_toupper_ptr = &__ctype_toupper;
const unsigned char * const * const __ctype_tolower_ptr = &__ctype_tolower;

#ifndef STDC_STATIC
static int __ctype_init(struct StdCBase *StdCBase)
{
    /* Currently these values are the same for all libbases
       but could in theory be changed in the future to make
       it locale dependent.
    */
    StdCBase->__ctype_b = &__ctype_b_array[0];
    StdCBase->__ctype_toupper = &__ctype_toupper_array[0];
    StdCBase->__ctype_tolower = &__ctype_tolower_array[0];

    return 1;
}

ADD2INITLIB(__ctype_init, 20);
#endif

/*****************************************************************************

    NAME
        #include <ctype.h>

        int isupper (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is uppercase. Works for all characters between
        -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is uppercase, 0 otherwise.

    NOTES

    EXAMPLE
        isupper ('A')    -> true
        isupper ('a')    -> false
        isupper ('0')    -> false
        isupper ('.')    -> false
        isupper ('\n')   -> false
        isupper ('\001') -> false
        isupper (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int islower (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is lowercase. Works for all characters between
        -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is lowercase, 0 otherwise.

    NOTES

    EXAMPLE
        islower ('A')    -> false
        islower ('a')    -> true
        islower ('0')    -> false
        islower ('.')    -> false
        islower ('\n')   -> false
        islower ('\001') -> false
        islower (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/******************************************************************************

    NAME
        #include <ctype.h>

        int isalpha (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is an alphabetic character. Works for all
        characters between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is an alphabetic character, 0 otherwise.

    NOTES

    EXAMPLE
        isalpha ('A')    -> true
        isalpha ('a')    -> true
        isalpha ('0')    -> false
        isalpha ('.')    -> false
        isalpha ('\n')   -> false
        isalpha ('\001') -> false
        isalpha (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isalnum (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is an alphabetic character or a digit. Works
        for all characters between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is alphabetic character or a digit, 0 otherwise.

    NOTES

    EXAMPLE
        isalnum ('A')    -> true
        isalnum ('a')    -> true
        isalnum ('0')    -> true
        isalnum ('.')    -> false
        isalnum ('\n')   -> false
        isalnum ('\001') -> false
        isalnum (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isascii (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is an ascii character. Works for all characters
        between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is an ascii character, 0 otherwise.

    NOTES

    EXAMPLE
        isascii ('A')    -> true
        isascii ('a')    -> true
        isascii ('0')    -> true
        isascii ('.')    -> true
        isascii ('\n')   -> true
        isascii ('\001') -> true
        isascii (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isblank (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is a space or a tab. Works for all characters
        between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is a space or tab, 0 otherwise.

    NOTES

    EXAMPLE
        isblank ('A')    -> false
        isblank ('a')    -> false
        isblank ('0')    -> false
        isblank ('.')    -> false
        isblank (' ')    -> true
        isblank ('\n')   -> false
        isblank ('\001') -> false
        isblank (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int iscntrl (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is a control character. Works for all
        characters between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is a control character, 0 otherwise.

    NOTES

    EXAMPLE
        iscntrl ('A')    -> false
        iscntrl ('a')    -> false
        iscntrl ('0')    -> false
        iscntrl ('.')    -> false
        iscntrl ('\n')   -> true
        iscntrl ('\001') -> true
        iscntrl (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isdigit (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is a digit. Works for all characters between
        -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is a digit, 0 otherwise.

    NOTES

    EXAMPLE
        isdigit ('A')    -> false
        isdigit ('a')    -> false
        isdigit ('0')    -> true
        isdigit ('.')    -> false
        isdigit ('\n')   -> false
        isdigit ('\001') -> false
        isdigit (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isgraph (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is a printable character but no whitespace.
        Works for all characters between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is a printable character but no whitespace, 0
        otherwise.

    NOTES

    EXAMPLE
        isgraph ('A')    -> true
        isgraph ('a')    -> true
        isgraph ('0')    -> true
        isgraph ('.')    -> true
        isgraph ('\n')   -> false
        isgraph ('\001') -> false
        isgraph (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isprint (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is a printable character. Works for all
        characters between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is a printable character, 0 otherwise.

    NOTES

    EXAMPLE
        isprint ('A')    -> true
        isprint ('a')    -> true
        isprint ('0')    -> true
        isprint ('.')    -> true
        isprint ('\n')   -> true
        isprint ('\001') -> false
        isprint (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isspace (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is whitespace. Works for all characters between
        -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is whitespace, 0 otherwise.

    NOTES

    EXAMPLE
        isspace ('A')    -> false
        isspace ('a')    -> false
        isspace ('0')    -> false
        isspace ('.')    -> false
        isspace ('\n')   -> true
        isspace ('\001') -> false
        isspace (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int ispunct (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is printable but not alphanumeric. Works for
        all characters between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is printable but not alphanumeric, 0
        otherwise.

    NOTES

    EXAMPLE
        ispunct ('A')    -> false
        ispunct ('a')    -> false
        ispunct ('0')    -> false
        ispunct ('.')    -> true
        ispunct ('\n')   -> false
        ispunct ('\001') -> false
        ispunct (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
/*****************************************************************************

    NAME
        #include <ctype.h>

        int isxdigit (

    SYNOPSIS
        int c)

    FUNCTION
        Test if a character is a hexadecimal digit. Works for all
        characters between -128 and 255 inclusive both.

    INPUTS
        c - The character to test.

    RESULT
        != 0 if the character is a hexadecimal digit, 0 otherwise.

    NOTES

    EXAMPLE
        isxdigit ('A')    -> true
        isxdigit ('a')    -> true
        isxdigit ('x')    -> false
        isxdigit ('0')    -> true
        isxdigit ('.')    -> false
        isxdigit ('\n')   -> false
        isxdigit ('\001') -> false
        isxdigit (EOF)    -> false

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

