/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <ctype.h>

const unsigned short int __ctype_b_array[384] =
{
    _IScntrl, /* -128 */
    _IScntrl, /* -127 */
    _IScntrl, /* -126 */
    _IScntrl, /* -125 */
    _IScntrl, /* -124 */
    _IScntrl, /* -123 */
    _IScntrl, /* -122 */
    _IScntrl, /* -121 */
    _IScntrl, /* -120 */
    _IScntrl, /* -119 */
    _IScntrl, /* -118 */
    _IScntrl, /* -117 */
    _IScntrl, /* -116 */
    _IScntrl, /* -115 */
    _IScntrl, /* -114 */
    _IScntrl, /* -113 */
    _IScntrl, /* -112 */
    _IScntrl, /* -111 */
    _IScntrl, /* -110 */
    _IScntrl, /* -109 */
    _IScntrl, /* -108 */
    _IScntrl, /* -107 */
    _IScntrl, /* -106 */
    _IScntrl, /* -105 */
    _IScntrl, /* -104 */
    _IScntrl, /* -103 */
    _IScntrl, /* -102 */
    _IScntrl, /* CSI */
    _IScntrl, /* -100 */
    _IScntrl, /* -99 */
    _IScntrl, /* -98 */
    _IScntrl, /* -97 */
    _IScntrl, /* NBSpace */
    _IScntrl, /* ¡ */
    _IScntrl, /* ¢ */
    _IScntrl, /* £ */
    _IScntrl, /* ¤ */
    _IScntrl, /* ¥ */
    _IScntrl, /* ¦ */
    _IScntrl, /* § */
    _IScntrl, /* ¨ */
    _IScntrl, /* © */
    _IScntrl, /* ª */
    _IScntrl, /* « */
    _IScntrl, /* ¬ */
    _IScntrl, /* ­ */
    _IScntrl, /* ® */
    _IScntrl, /* ¯ */
    _IScntrl, /* ° */
    _IScntrl, /* ± */
    _IScntrl, /* ² */
    _IScntrl, /* ³ */
    _IScntrl, /* ´ */
    _IScntrl, /* µ */
    _IScntrl, /* ¶ */
    _IScntrl, /* · */
    _IScntrl, /* ¸ */
    _IScntrl, /* ¹ */
    _IScntrl, /* º */
    _IScntrl, /* » */
    _IScntrl, /* ¼ */
    _IScntrl, /* ½ */
    _IScntrl, /* ¾ */
    _IScntrl, /* ¿ */
    _IScntrl, /* À */
    _IScntrl, /* Á */
    _IScntrl, /* Â */
    _IScntrl, /* Ã */
    _IScntrl, /* Ä */
    _IScntrl, /* Å */
    _IScntrl, /* Æ */
    _IScntrl, /* Ç */
    _IScntrl, /* È */
    _IScntrl, /* É */
    _IScntrl, /* Ê */
    _IScntrl, /* Ë */
    _IScntrl, /* Ì */
    _IScntrl, /* Í */
    _IScntrl, /* Î */
    _IScntrl, /* Ï */
    _IScntrl, /* Ð */
    _IScntrl, /* Ñ */
    _IScntrl, /* Ò */
    _IScntrl, /* Ó */
    _IScntrl, /* Ô */
    _IScntrl, /* Õ */
    _IScntrl, /* Ö */
    _IScntrl, /* × */
    _IScntrl, /* Ø */
    _IScntrl, /* Ù */
    _IScntrl, /* Ú */
    _IScntrl, /* Û */
    _IScntrl, /* Ü */
    _IScntrl, /* Ý */
    _IScntrl, /* Þ */
    _IScntrl, /* ß */
    _IScntrl, /* à */
    _IScntrl, /* á */
    _IScntrl, /* â */
    _IScntrl, /* ã */
    _IScntrl, /* ä */
    _IScntrl, /* å */
    _IScntrl, /* æ */
    _IScntrl, /* ç */
    _IScntrl, /* è */
    _IScntrl, /* é */
    _IScntrl, /* ê */
    _IScntrl, /* ë */
    _IScntrl, /* ì */
    _IScntrl, /* í */
    _IScntrl, /* î */
    _IScntrl, /* ï */
    _IScntrl, /* ð */
    _IScntrl, /* ñ */
    _IScntrl, /* ò */
    _IScntrl, /* ó */
    _IScntrl, /* ô */
    _IScntrl, /* õ */
    _IScntrl, /* ö */
    _IScntrl, /* ÷ */
    _IScntrl, /* ø */
    _IScntrl, /* ù */
    _IScntrl, /* ú */
    _IScntrl, /* û */
    _IScntrl, /* ü */
    _IScntrl, /* ý */
    _IScntrl, /* þ */
    _IScntrl, /* ÿ */
    _IScntrl, /* 0 */
    _IScntrl, /* 1 */
    _IScntrl, /* 2 */
    _IScntrl, /* 3 */
    _IScntrl, /* 4 */
    _IScntrl, /* 5 */
    _IScntrl, /* 6 */
    _IScntrl, /* 7 */
    _IScntrl, /* Backspace */
    _ISblank|_IScntrl|_ISspace, /* 9 */
    _IScntrl|_ISspace, /* LF */
    _IScntrl|_ISspace, /* 11 */
    _IScntrl|_ISspace, /* 12 */
    _IScntrl|_ISspace, /* CR */
    _IScntrl, /* 14 */
    _IScntrl, /* 15 */
    _IScntrl, /* 16 */
    _IScntrl, /* 17 */
    _IScntrl, /* 18 */
    _IScntrl, /* 19 */
    _IScntrl, /* 20 */
    _IScntrl, /* 21 */
    _IScntrl, /* 22 */
    _IScntrl, /* 23 */
    _IScntrl, /* 24 */
    _IScntrl, /* 25 */
    _IScntrl, /* 26 */
    _IScntrl, /* ESC */
    _IScntrl, /* 28 */
    _IScntrl, /* 29 */
    _IScntrl, /* 30 */
    _IScntrl, /* 31 */
    _ISblank|_ISprint|_ISspace, /* Space */
    _ISgraph|_ISprint|_ISpunct, /* ! */
    _ISgraph|_ISprint|_ISpunct, /* " */
    _ISgraph|_ISprint|_ISpunct, /* # */
    _ISgraph|_ISprint|_ISpunct, /* $ */
    _ISgraph|_ISprint|_ISpunct, /* % */
    _ISgraph|_ISprint|_ISpunct, /* & */
    _ISgraph|_ISprint|_ISpunct, /* ' */
    _ISgraph|_ISprint|_ISpunct, /* ( */
    _ISgraph|_ISprint|_ISpunct, /* ) */
    _ISgraph|_ISprint|_ISpunct, /* * */
    _ISgraph|_ISprint|_ISpunct, /* + */
    _ISgraph|_ISprint|_ISpunct, /* , */
    _ISgraph|_ISprint|_ISpunct, /* - */
    _ISgraph|_ISprint|_ISpunct, /* . */
    _ISgraph|_ISprint|_ISpunct, /* / */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 0 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 1 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 2 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 3 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 4 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 5 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 6 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 7 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 8 */
    _ISdigit|_ISgraph|_ISprint|_ISxdigit, /* 9 */
    _ISgraph|_ISprint|_ISpunct, /* : */
    _ISgraph|_ISprint|_ISpunct, /* ; */
    _ISgraph|_ISprint|_ISpunct, /* < */
    _ISgraph|_ISprint|_ISpunct, /* = */
    _ISgraph|_ISprint|_ISpunct, /* > */
    _ISgraph|_ISprint|_ISpunct, /* ? */
    _ISgraph|_ISprint|_ISpunct, /* @ */
    _ISupper|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* A */
    _ISupper|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* B */
    _ISupper|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* C */
    _ISupper|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* D */
    _ISupper|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* E */
    _ISupper|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* F */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* G */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* H */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* I */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* J */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* K */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* L */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* M */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* N */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* O */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* P */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* Q */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* R */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* S */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* T */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* U */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* V */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* W */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* X */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* Y */
    _ISupper|_ISalpha|_ISgraph|_ISprint, /* Z */
    _ISgraph|_ISprint|_ISpunct, /* [ */
    _ISgraph|_ISprint|_ISpunct, /* \ */
    _ISgraph|_ISprint|_ISpunct, /* ] */
    _ISgraph|_ISprint|_ISpunct, /* ^ */
    _ISgraph|_ISprint|_ISpunct, /* _ */
    _ISgraph|_ISprint|_ISpunct, /* ` */
    _ISlower|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* a */
    _ISlower|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* b */
    _ISlower|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* c */
    _ISlower|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* d */
    _ISlower|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* e */
    _ISlower|_ISalpha|_ISgraph|_ISprint|_ISxdigit, /* f */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* g */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* h */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* i */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* j */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* k */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* l */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* m */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* n */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* o */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* p */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* q */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* r */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* s */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* t */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* u */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* v */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* w */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* x */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* y */
    _ISlower|_ISalpha|_ISgraph|_ISprint, /* z */
    _ISgraph|_ISprint|_ISpunct, /* { */
    _ISgraph|_ISprint|_ISpunct, /* | */
    _ISgraph|_ISprint|_ISpunct, /* } */
    _ISgraph|_ISprint|_ISpunct, /* ~ */
    _IScntrl, /* Del */
    _IScntrl, /* 128 */
    _IScntrl, /* 129 */
    _IScntrl, /* 130 */
    _IScntrl, /* 131 */
    _IScntrl, /* 132 */
    _IScntrl, /* 133 */
    _IScntrl, /* 134 */
    _IScntrl, /* 135 */
    _IScntrl, /* 136 */
    _IScntrl, /* 137 */
    _IScntrl, /* 138 */
    _IScntrl, /* 139 */
    _IScntrl, /* 140 */
    _IScntrl, /* 141 */
    _IScntrl, /* 142 */
    _IScntrl, /* 143 */
    _IScntrl, /* 144 */
    _IScntrl, /* 145 */
    _IScntrl, /* 146 */
    _IScntrl, /* 147 */
    _IScntrl, /* 148 */
    _IScntrl, /* 149 */
    _IScntrl, /* 150 */
    _IScntrl, /* 151 */
    _IScntrl, /* 152 */
    _IScntrl, /* 153 */
    _IScntrl, /* 154 */
    _IScntrl, /* CSI */
    _IScntrl, /* 156 */
    _IScntrl, /* 157 */
    _IScntrl, /* 158 */
    _IScntrl, /* 159 */
    _IScntrl, /* NBSpace */
    _IScntrl, /* ¡ */
    _IScntrl, /* ¢ */
    _IScntrl, /* £ */
    _IScntrl, /* ¤ */
    _IScntrl, /* ¥ */
    _IScntrl, /* ¦ */
    _IScntrl, /* § */
    _IScntrl, /* ¨ */
    _IScntrl, /* © */
    _IScntrl, /* ª */
    _IScntrl, /* « */
    _IScntrl, /* ¬ */
    _IScntrl, /* ­ */
    _IScntrl, /* ® */
    _IScntrl, /* ¯ */
    _IScntrl, /* ° */
    _IScntrl, /* ± */
    _IScntrl, /* ² */
    _IScntrl, /* ³ */
    _IScntrl, /* ´ */
    _IScntrl, /* µ */
    _IScntrl, /* ¶ */
    _IScntrl, /* · */
    _IScntrl, /* ¸ */
    _IScntrl, /* ¹ */
    _IScntrl, /* º */
    _IScntrl, /* » */
    _IScntrl, /* ¼ */
    _IScntrl, /* ½ */
    _IScntrl, /* ¾ */
    _IScntrl, /* ¿ */
    _IScntrl, /* À */
    _IScntrl, /* Á */
    _IScntrl, /* Â */
    _IScntrl, /* Ã */
    _IScntrl, /* Ä */
    _IScntrl, /* Å */
    _IScntrl, /* Æ */
    _IScntrl, /* Ç */
    _IScntrl, /* È */
    _IScntrl, /* É */
    _IScntrl, /* Ê */
    _IScntrl, /* Ë */
    _IScntrl, /* Ì */
    _IScntrl, /* Í */
    _IScntrl, /* Î */
    _IScntrl, /* Ï */
    _IScntrl, /* Ð */
    _IScntrl, /* Ñ */
    _IScntrl, /* Ò */
    _IScntrl, /* Ó */
    _IScntrl, /* Ô */
    _IScntrl, /* Õ */
    _IScntrl, /* Ö */
    _IScntrl, /* × */
    _IScntrl, /* Ø */
    _IScntrl, /* Ù */
    _IScntrl, /* Ú */
    _IScntrl, /* Û */
    _IScntrl, /* Ü */
    _IScntrl, /* Ý */
    _IScntrl, /* Þ */
    _IScntrl, /* ß */
    _IScntrl, /* à */
    _IScntrl, /* á */
    _IScntrl, /* â */
    _IScntrl, /* ã */
    _IScntrl, /* ä */
    _IScntrl, /* å */
    _IScntrl, /* æ */
    _IScntrl, /* ç */
    _IScntrl, /* è */
    _IScntrl, /* é */
    _IScntrl, /* ê */
    _IScntrl, /* ë */
    _IScntrl, /* ì */
    _IScntrl, /* í */
    _IScntrl, /* î */
    _IScntrl, /* ï */
    _IScntrl, /* ð */
    _IScntrl, /* ñ */
    _IScntrl, /* ò */
    _IScntrl, /* ó */
    _IScntrl, /* ô */
    _IScntrl, /* õ */
    _IScntrl, /* ö */
    _IScntrl, /* ÷ */
    _IScntrl, /* ø */
    _IScntrl, /* ù */
    _IScntrl, /* ú */
    _IScntrl, /* û */
    _IScntrl, /* ü */
    _IScntrl, /* ý */
    _IScntrl, /* þ */
    _IScntrl, /* ÿ */
};

const int __ctype_toupper_array[384] =
{
    /* -128 */
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

const int __ctype_tolower_array[384] =
{
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

#ifdef AROSC_SHARED

static int __ctype_init(void)
{
    __ctype_b       = &__ctype_b_array[128];
    __ctype_toupper = &__ctype_toupper_array[128];
    __ctype_tolower = &__ctype_tolower_array[128];

    return 1;
}

ADD2INIT(__ctype_init, 20);

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

