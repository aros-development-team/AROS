/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <devices/keymap.h>

#define DEFAULT_KEYMAP
#include "keymap_intern.h"

#undef N
#undef S
#undef A
#undef C
#undef D
#undef V
#undef ST
#undef NOP

#define N KC_NOQUAL
#define S KCF_SHIFT
#define A KCF_ALT
#define C KCF_CONTROL
#define D KCF_DEAD
#define V KC_VANILLA
#define ST KCF_STRING
#define NOP KCF_NOP

#define EUR 0xA4 /* ISO 8859-15: Euro = 164 = 0xA4) */

static CONST UBYTE lokeymaptypes[] =
{
    V,          /* 00 */
    S|A,        /* 01 */
    V,          /* 02 */
    S|A,        /* 03 */
    S|A,        /* 04 */
    S|A,        /* 05 */
    V,          /* 06 */
    S|A,        /* 07 */
    S|A,        /* 08 */
    S|A,        /* 09 */
    S|A,        /* 0A */
    V,          /* 0B */
    S,          /* 0C */
    V,          /* 0D */
    NOP,        /* 0E */
    N,          /* 0F */
    V,          /* 10 q */
    V,          /* 11 w */
    D|V,        /* 12 e */
    V,          /* 13 r */
    V,          /* 14 t */
    D|V,        /* 15 z */
    D|V,        /* 16 u */
    D|V,        /* 17 i */
    D|V,        /* 18 o */
    V,          /* 19 p */
    V,          /* 1A */
    V,          /* 1B */
    NOP,        /* 1C */
    N,          /* 1D */
    N,          /* 1E */
    N,          /* 1F */
    D|V,        /* 20 a */
    V,          /* 21 s */
    V,          /* 22 d */
    D|V,        /* 23 f */
    D|V,        /* 24 g */
    D|V,        /* 25 h */
    D|V,        /* 26 j */
    D|V,        /* 27 k */
    V,          /* 28 l */
    S,          /* 29 */
    S,          /* 2A */
    NOP,        /* 2B */
    NOP,        /* 2C */
    N,          /* 2D */
    N,          /* 2E */
    N,          /* 2F */
    S|A,        /* 30 */
    V,          /* 31 y */
    V,          /* 32 x */
    V,          /* 33 c */
    V,          /* 34 v */
    V,          /* 35 b */
    D|V,        /* 36 n */
    V,          /* 37 m */
    S,          /* 38 */
    S,          /* 39 */
    S,          /* 3A */
    NOP,        /* 3B */
    N,          /* 3C */
    N,          /* 3D */
    N,          /* 3E */
    N,          /* 3F */
    
};

static CONST UBYTE hikeymaptypes[] =
{
    D|A,        /* 40 SPACE */
    N,          /* 41 BACKSPACE */
    ST|S,       /* 42 TAB */
    N,          /* 43 ENTER */
    C,          /* 44 RETURN */
    A,          /* 45 ESCAPE */
    N,          /* 46 DEL  */
    ST|S,       /* 47 INSERT ?? */
    ST|S,       /* 48 PAGE UP ?? */
    ST|S,       /* 49 PAGE DOWN ?? */
    N,          /* 4A NUMERIC PAD - */
    ST|S,       /* 4B F11 ?? */
    ST|S,       /* 4C CURSORUP*/
    ST|S,       /* 4D CURSORDOWN */
    ST|S,       /* 4E CURSORRIGHT */
    ST|S,       /* 4F CURSORLEFT */
    ST|S,       /* 50 F1 */
    ST|S,       /* 51 F2 */
    ST|S,       /* 52 F3 */
    ST|S,       /* 53 F4 */
    ST|S,       /* 54 F5 */
    ST|S,       /* 55 F6 */
    ST|S,       /* 56 F7 */
    ST|S,       /* 57 F8 */
    ST|S,       /* 58 F9 */
    ST|S,       /* 59 F10 */
    S,          /* 5A NUMPAD ( */
    S,          /* 5B NUMPAD ) */
    N,          /* 5C NUMPAD / */
    N,          /* 5D NUMPAD * */
    N,          /* 5E NUMPAD + */
    ST,         /* 5F HELP */
    NOP,        /* 60 LEFT SHIFT*/
    NOP,        /* 61 RIGHT SHIFT */
    NOP,        /* 62 CAPS LOCK */
    NOP,        /* 63 CONTROL */
    NOP,        /* 64 LALT */
    NOP,        /* 65 RALT */
    NOP,        /* 66 LCOMMAND */
    NOP,        /* 67 RCOMMAND */
    NOP,        /* 68 LEFT MOUSE BUTTON*/
    NOP,        /* 69 RIGHT MOUSE BUTTON */
    NOP,        /* 6A MIDDLE MOUSE BUTTON */
    NOP,        /* 6B */
    NOP,        /* 6C */
    NOP,        /* 6D */
    ST|A,       /* 6E PAUSE/BREAK ??*/
    ST|S,       /* 6F F12 ?? */
    ST|C,       /* 70 HOME ?? */
    ST|C,       /* 71 END ?? */
    NOP,        /* 72 */
    NOP,        /* 73 */
    NOP,        /* 74 */
    NOP,        /* 75 */
    NOP,        /* 76 */
    NOP        /* 77 */
#if defined(NONSTANDARD_KEYMAP)
    , NOP,        /* 78 */
    NOP,        /* 79 */
    NOP,        /* 7A */
    NOP,        /* 7B */
    NOP,        /* 7C */
    NOP,        /* 7D */
    NOP,        /* 7E */
    NOP         /* 7F */
#endif
};

#undef N
#undef S
#undef A
#undef C
#undef D
#undef V
#undef ST
#undef NOP

#undef STRING
#undef DEAD
#undef BYTES

#define STRING(x) (IPTR)x
#define DEAD(x)   (IPTR)x

#define BYTES(b0, b1, b2, b3) \
        (((UBYTE)b0)<<24) | (((UBYTE)b1)<<16) | (((UBYTE)b2)<<8) | (((UBYTE)b3)<<0)


/* dead symbols
 
  1= ´
  2 = `
  3 = ^
  4 = ~
  5 = "
  6 = °
  
*/

STATIC CONST UBYTE a_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '\xE6',
    0, '\xC6',
    0, 0x01,
    0, 0x01,
    0, 0x81,
    0, 0x81,
    
    'a', '\xE1', '\xE0', '\xE2', '\xE3', '\xE4', '\xE5',
    
    'A', '\xC1', '\xC0', '\xC2', '\xC3', '\xC4', '\xC5'
};

STATIC CONST UBYTE e_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '\xA9',
    0, '\xA9',
    0, 0x05,
    0, 0x05,
    0, 0x85,
    0, 0x85,
    
    'e', '\xE9', '\xE8', '\xEA', 'e', '\xEB', 'e',
    
    'E', '\xC9', '\xC8', '\xCA', 'E', '\xCB', 'E'
};

STATIC CONST UBYTE u_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '\xB5',
    0, '\xB5',
    0, 0x15,
    0, 0x15,
    0, 0x95,
    0, 0x95,
    
    'u', '\xFA', '\xF9', '\xFB', 'u', '\xFC', 'u',
    
    'U', '\xDA', '\xD9', '\xDB', 'U', '\xDC', 'U'
};

STATIC CONST UBYTE i_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '\xA1',
    0, '\xA6',
    0, 0x09,
    0, 0x09,
    0, 0x89,
    0, 0x89,
    
    'i', '\xED', '\xEC', '\xEE', 'i', '\xEF', 'i',
    
    'I', '\xCD', '\xCC', '\xCE', 'I', '\xCF', 'I'
};

STATIC CONST UBYTE o_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '\xF8',
    0, '\xD8',
    0, 0x0F,
    0, 0x0F,
    0, 0x8F,
    0, 0x8F,
    
    'o', '\xF3', '\xF2', '\xF4', '\xF5', '\xF6', 'o',
    
    'O', '\xD3', '\xD2', '\xD4', '\xD5', '\xD6', 'O'
};
 
STATIC CONST UBYTE y_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, EUR,
    0, '\xA5',
    0, 0x19,
    0, 0x19,
    0, 0x99,
    0, 0x99,
    
    'y', '\xFD', 'y', 'y', 'y', '\xFF', 'y',
    
    'Y', '\xDD', 'Y', 'Y', 'Y', 'Y', 'Y'
};

STATIC CONST UBYTE n_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, 0x00,
    0, '\xAF',
    0, 0x0E,
    0, 0x0E,
    0, 0x8E,
    0, 0x8E,
    
    'n', 'n', 'n', 'n', '\xF1', 'n', 'n',
    
    'N', 'N', 'N', 'N', '\xD1', 'N', 'N'
};

STATIC CONST UBYTE f_descr[] =
{
    0, 'f',
    0, 'F',
    DPF_DEAD, 1,
    DPF_DEAD, 1,
    0, 0x6,
    0, 0x6,
    0, 0x86,
    0, 0x86,
};

STATIC CONST UBYTE g_descr[] =
{
    0, 'g',
    0, 'G',
    DPF_DEAD, 2,
    DPF_DEAD, 2,
    0, 0x7,
    0, 0x7,
    0, 0x87,
    0, 0x87,
};

STATIC CONST UBYTE h_descr[] =
{
    0, 'h',
    0, 'H',
    DPF_DEAD, 3,
    DPF_DEAD, 3,
    0, 0x8,
    0, 0x8,
    0, 0x88,
    0, 0x88,
};

STATIC CONST UBYTE j_descr[] =
{
    0, 'j',
    0, 'J',
    DPF_DEAD, 4,
    DPF_DEAD, 4,
    0, 0xa,
    0, 0xa,
    0, 0x8a,
    0, 0x8a,
};

STATIC CONST UBYTE k_descr[] =
{
    0, 'k',
    0, 'K',
    DPF_DEAD, 5,
    DPF_DEAD, 5,
    0, 0xb,
    0, 0xb,
    0, 0x8b,
    0, 0x8b,
};


static CONST IPTR lokeymap[] =
{
    BYTES('~', '`', '~', '`'),          /* 00 Left of 1 Key */
    BYTES('!', '\xB9', '!', '1'),       /* 01 1 */
    BYTES('@', '\xB2', '@', '2'),       /* 02 2 */
    BYTES('#', '\xB3', '#', '3'),       /* 03 3 */
    BYTES('$', '\xA2', '$', '4'),       /* 04 4 */
    BYTES('%', '\xBC', '%', '5'),       /* 05 5 */
    BYTES('^', '\xBD', '^', '6'),       /* 06 6 */
    BYTES('&', '\xBE', '&', '7'),       /* 07 7 */
    BYTES('*', '\xB7', '*', '8'),       /* 08 8 */
    BYTES('(', '\xAB', '(', '9'),       /* 09 9 */
    BYTES(')', '\xBB', ')', '0'),       /* 0A 0 */
    BYTES('_', '-', '_', '-'),          /* 0B Right of 0 */
    BYTES('+', '=', '+', '='),          /* 0C 2nd right of 0 */
    BYTES('|', '\\', '|', '\\'),        /* 0D 3rd right of 0 */
    BYTES(0, 0, 0, 0),                  /* 0E undefined */
    BYTES('0', '0', '0', '0'),          /* 0F NUM 0 */
    BYTES('\xC5', '\xE5', 'Q', 'q'),    /* 10 */
    BYTES('\xB0', '\xB0', 'W', 'w'),    /* 11 */
    DEAD(e_descr),                      /* 12 */
    BYTES('\xAE', '\xAE', 'R', 'r'),    /* 13 */
    BYTES('\xDE', '\xFE', 'T', 't'),    /* 14 */
    DEAD(y_descr),                      /* 15 */
    DEAD(u_descr),                      /* 16 */
    DEAD(i_descr),                      /* 17 */
    DEAD(o_descr),                      /* 18 */
    BYTES('\xB6', '\xB6', 'P', 'p'),    /* 19 */
    
    BYTES('{', '[', '{', '['),          /* 1A */
    BYTES('}', ']', '}', ']'),          /* 1B */
    BYTES(0, 0, 0, 0),                  /* 1C undefined */
    BYTES('0', '0', '0', '1'),          /* 1D NUM 1*/
    BYTES('0', '0', '0', '2'),          /* 1E NUM 2*/
    BYTES('0', '0', '0', '3'),          /* 1F NUM 3*/
    
    DEAD(a_descr),                      /* 20 */
    BYTES('\xA7', '\xDF', 'S', 's'),    /* 21 */
    BYTES('\xD0', '\xF0', 'D', 'd'),    /* 22 */
    DEAD(f_descr),                      /* 23 */
    DEAD(g_descr),                      /* 24 */
    DEAD(h_descr),                      /* 25 */
    DEAD(j_descr),                      /* 26 */
    DEAD(k_descr),                      /* 27 */
    BYTES('\xA3', '\xA3', 'L', 'l'),    /* 28 */
    
    BYTES(':', ';', ':', ';'),          /* 29 */
    BYTES('"', '\'', '"', '\''),        /* 2A */
    BYTES(0, 0, 0, 0),                  /* 2B undefined */
    BYTES(0, 0, 0, 0),                  /* 2C undefined */
    BYTES('0', '0', '0', '4'),          /* 2D NUM 4 */
    BYTES('0', '0', '0', '5'),          /* 2E NUM 5 */
    BYTES('0', '0', '0', '6'),          /* 2F NUM 6 */
    BYTES('\xBB', '\xAB', '>', '<'),    /* 30 */
    
    BYTES('\xAC', '\xB1', 'Z', 'z'),    /* 31 */
    BYTES('\xF7', '\xD7', 'X', 'x'),    /* 32 */
    BYTES('\xC7', '\xE7', 'C', 'c'),    /* 33 */
    BYTES('\xAA', '\xAA', 'V', 'v'),    /* 34 */
    BYTES('\xBA', '\xBA', 'B', 'b'),    /* 35 */
    DEAD(n_descr),                      /* 36 */
    BYTES('\xBF', '\xB8', 'M', 'm'),    /* 37 */
    
    BYTES('<', ',', '<', ','),          /* 38 */
    BYTES('>', '.', '>', '.'),          /* 39 */
    BYTES('?', '/', '?', '/'),          /* 3A */
    BYTES(0, 0, 0, 0),  /* 3B */
    BYTES('0', '0', '0', '.'),          /* 3C NUM . */
    BYTES('0', '0', '0', '7'),          /* 3D NUM 7 */
    BYTES('0', '0', '0', '8'),          /* 3E NUM 8 */
    BYTES('0', '0', '0', '9'),          /* 3F NUM 9 */
};

/* Strings for the F1 key. In a real AmigaOS keymap, these would have come after
** the HiKeyMap, but we do it this way to avoid prototyping
**
** String descriptors are byte arrays and work like this:
**
** sizeofstring,offset_from_start_array_to_start_of_string
** sizeofstring,offset_from_start_array_to_start_of_string
** ..
** ..
** string1
** string2
** ..
** ..
**
** The number of strings depends on the qualifier flags
** set in the keymap type.
*/

CONST UBYTE f1_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'0','~',
    0x9B,'1','0','~'
};

CONST UBYTE f2_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'1','~',
    0x9B,'1','1','~'
};

CONST UBYTE f3_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'2','~',
    0x9B,'1','2','~'
};

CONST UBYTE f4_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'3','~',
    0x9B,'1','3','~'
};

CONST UBYTE f5_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'4','~',
    0x9B,'1','4','~'
};

CONST UBYTE f6_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'5','~',
    0x9B,'1','5','~'
};

CONST UBYTE f7_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'6','~',
    0x9B,'1','6','~'
};

CONST UBYTE f8_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'7','~',
    0x9B,'1','7','~'
};

CONST UBYTE f9_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'8','~',
    0x9B,'1','8','~'
};

CONST UBYTE f10_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'9','~',
    0x9B,'1','9','~'
};

CONST UBYTE f11_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'2','0','~',
    0x9B,'3','0','~'
};

CONST UBYTE f12_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'2','1','~',
    0x9B,'3','1','~'
};

CONST UBYTE insert_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','0','~',
    0x9B,'5','0','~'
};

CONST UBYTE pageup_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','1','~',
    0x9B,'5','1','~'
};

CONST UBYTE pagedown_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','2','~',
    0x9B,'5','2','~'
};

CONST UBYTE pausebreak_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','3','~',
    0x9B,'5','3','~'
};

CONST UBYTE home_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','4','~',
    0x9B,'5','4','~'
};

CONST UBYTE end_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','5','~',
    0x9B,'5','5','~'
};

CONST UBYTE up_descr[] =
{
    2,4,
    2,6,
    
    0x9B,'A',
    0x9B,'T'
};

CONST UBYTE down_descr[] =
{
    2,4,
    2,6,
    
    0x9B,'B',
    0x9B,'S'
};

CONST UBYTE left_descr[] =
{
    2,4,
    3,6,
    
    0x9B,'D',
    0x9B,' ','A'
};

CONST UBYTE right_descr[] =
{
    2,4,
    3,6,
    
    0x9B,'C',
    0x9B,' ','@'
};

CONST UBYTE tab_descr[] =
{
    1,4,
    2,5,
    
    0x9,
    0x9B,'Z'
};

CONST UBYTE help_descr[] =
{
    3,2,
    
    0x9B,'?','~'
};

STATIC CONST UBYTE space_descr[] =
{
    DPF_MOD, 0x4,
    0, 0XA0,

    ' ', '\xB4', '`','^','~', '\xA8', '\xB0'
};

/* NB: under AmigaOS, hikeymap covers the range 0x40 -> 0x77 */
static CONST IPTR hikeymap[] =
{
    DEAD(space_descr),          /* 40 SPACE */
    BYTES(8, 8, 8, 8),          /* 41 BACKSPACE*/
    STRING(tab_descr),          /* 42 TAB */
    BYTES(13, 13, 13, 13),      /* 43 ENTER */
    BYTES(0, 0, 10, 13),        /* 44 RETURN */
    BYTES(0, 0, 0x9B, 27),      /* 45 ESCAPE */
    BYTES(127, 127, 127, 127),  /* 46 DEL */
    STRING(insert_descr),       /* 47 INSERT ?? */
    STRING(pageup_descr),       /* 48 PAGEUP ?? */
    STRING(pagedown_descr),     /* 49 PAGEDOWN ?? */
    BYTES('-', '-', '-', '-'),  /* 4A NUMPAD - */
    STRING(f11_descr),          /* 4B F11 ?? */
    STRING(up_descr),           /* 4C CURSOR UP*/
    STRING(down_descr),         /* 4D CURSOR DOWN*/
    STRING(right_descr),        /* 4E CURSOR RIGHT */
    STRING(left_descr),         /* 4F CURSOR LEFT */
    STRING(f1_descr),           /* 50 F1 */
    STRING(f2_descr),           /* 51 */
    STRING(f3_descr),           /* 52 */
    STRING(f4_descr),           /* 53 */
    STRING(f5_descr),           /* 54 */
    STRING(f6_descr),           /* 55 */
    STRING(f7_descr),           /* 56 */
    STRING(f8_descr),           /* 57 */
    STRING(f9_descr),           /* 58 */
    STRING(f10_descr),          /* 59 */
    BYTES('[', '(', '[', '('),  /* 5A */
    BYTES(']', ')', ']', ')'),  /* 5B */
    BYTES('/', '/', '/', '/'),  /* 5C */
    BYTES('*', '*', '*', '*'),  /* 5D */
    BYTES('+', '+', '+', '+'),  /* 5E */
    STRING(help_descr),         /* 5F HELP */
    BYTES(0, 0, 0, 0),          /* 60 */
    BYTES(0, 0, 0, 0),          /* 61 */
    BYTES(0, 0, 0, 0),          /* 62 */
    BYTES(0, 0, 0, 0),          /* 63 */
    BYTES(0, 0, 0, 0),          /* 64 */
    BYTES(0, 0, 0, 0),          /* 65 */
    BYTES(0, 0, 0, 0),          /* 66 */
    BYTES(0, 0, 0, 0),          /* 67 */
    BYTES(0, 0, 0, 0),          /* 68 */
    BYTES(0, 0, 0, 0),          /* 69 */
    BYTES(0, 0, 0, 0),          /* 6A */
    BYTES(0, 0, 0, 0),          /* 6B */
    BYTES(0, 0, 0, 0),          /* 6C */
    BYTES(0, 0, 0, 0),          /* 6D */
    STRING(pausebreak_descr),   /* 6E PAUSE/BREAK ?? */
    STRING(f12_descr),          /* 6F F12 ?? */
    STRING(home_descr),         /* 70 HOME ?? */
    STRING(end_descr),          /* 71 END ?? */
    BYTES(0, 0, 0, 0),          /* 72 */
    BYTES(0, 0, 0, 0),          /* 73 */
    BYTES(0, 0, 0, 0),          /* 74 */
    BYTES(0, 0, 0, 0),          /* 75 */
    BYTES(0, 0, 0, 0),          /* 76 */
    BYTES(0, 0, 0, 0)          /* 77 */
#if defined(NONSTANDARD_KEYMAP)
    , BYTES(0, 0, 0, 0),          /* 78 */
    BYTES(0, 0, 0, 0),          /* 79 */
    BYTES(0, 0, 0, 0),          /* 7A */
    BYTES(0, 0, 0, 0),          /* 7B */
    BYTES(0, 0, 0, 0),          /* 7C */
    BYTES(0, 0, 0, 0),          /* 7D */
    BYTES(0, 0, 0, 0),          /* 7E */
    BYTES(0, 0, 0, 0),          /* 7F */
#endif
};

#undef SETBITS

#define SETBITS(b0, b1, b2, b3, b4, b5, b6, b7) \
        (b0<<0)|(b1<<1)|(b2<<2)|(b3<<3)|(b4<<4)|(b5<<5)|(b6<<6)|(b7<<7)
        
static CONST UBYTE locapsable[] =
{
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 00 - 07 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 08 - 0F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),    /* 10 - 17 */
    SETBITS(1, 1, 0, 0, 0, 0, 0, 0),    /* 18 - 1F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),    /* 20 - 27 */
    SETBITS(1, 0, 0, 0, 0, 0, 0, 0),    /* 28 - 2F */
    
    SETBITS(0, 1, 1, 1, 1, 1, 1, 1),    /* 30 - 37 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)     /* 38 - 3F */
};

static CONST UBYTE hicapsable[] =
{
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 40 - 47 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 48 - 4F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 50 - 57 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 58 - 5F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 60 - 67 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 68 - 6F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)     /* 70 - 77 */
#if defined(NONSTANDARD_KEYMAP)
    , SETBITS(0, 0, 0, 0, 0, 0, 0, 0)     /* 78 - 7F */
#endif
};

static CONST UBYTE lorepeatable[] =
{
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),    /* 00 - 07 */
    SETBITS(1, 1, 1, 1, 1, 1, 0, 1),    /* 08 - 0F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),    /* 10 - 17 */
    SETBITS(1, 1, 1, 1, 0, 1, 1, 1),    /* 18 - 1F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),    /* 20 - 27 */
    SETBITS(1, 1, 1, 1, 0, 1, 1, 1),    /* 28 - 2F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),    /* 30 - 37 */
    SETBITS(1, 1, 1, 0, 1, 1, 1, 1)     /* 38 - 3F */
};

static CONST UBYTE hirepeatable[] =
{
    SETBITS(1, 1, 1, 0, 0, 0, 1, 0),    /* 40 - 47 */
    SETBITS(1, 1, 0, 0, 1, 1, 1, 1),    /* 48 - 4F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 50 - 57 */
    SETBITS(0, 0, 1, 1, 1, 1, 1, 0),    /* 58 - 5F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 60 - 67 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 68 - 6F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)     /* 70 - 77 */
#if defined(NONSTANDARD_KEYMAP)
    , SETBITS(0, 0, 0, 0, 0, 0, 0, 0)     /* 78 - 7F */
#endif
};

CONST struct KeyMap def_km =
{
    lokeymaptypes,
    lokeymap,
    locapsable,
    lorepeatable,
    
    hikeymaptypes,
    hikeymap,
    hicapsable,
    hirepeatable
};

/* index vertically: (keytype & KC_VANILLA)
** index horizontally: KCF_xxx qualifier combination for keypress.
** Which is used to get the index of the byte describing the keypress.
** X means invalid key.
** S means Ctrl-c alike combination (clear bits 5 and 6)
*/
#undef X /* undefined */
#undef S /* Ctrl-c like combo */
#define X (-1)
#define S (-2)

#if 1

/* stegerg: on the Amiga you get a key event if you
            press for example CONTROL + A, even if
            the keymaptype for that key has not set
            KCF_CONTROL. So it looks like if for a
            certain keypress some qualifiers are set
            which are not set in the keymap-type then
            this qualifiers are simply to be ignored
*/
            
    const BYTE keymaptype_table[8][8] =
    {
        {3, 3, 3, 3, 3, 3, 3, 3},       /* KCF_NOQUAL                   == 0 */
        {3, 2, 3, 2, 3, 2, 3, 2},       /* KCF_SHIFT                    == 1 */
        {3, 3, 2, 2, 3, 3, 2, 2},       /* KCF_ALT                      == 2 */
        {3, 2, 1, 0, 3, 2, 1, 0},       /* KCF_SHIFT|KCF_ALT            == 3 */
        {3, 3, 3, 3, 2, 2, 2, 2},       /* KCF_CONTROL                  == 4 */
        {3, 2, 3, 2, 1, 0, 1, 0},       /* KCF_SHIFT|KCF_CONTROL        == 5 */
        {3, 3, 2, 2, 1, 1, 0, 0},       /* KCF_ALT|KCF_CONTROL          == 6 */
        {3, 2, 1, 0, S, X, X, X}        /* KCF_SHIFT|KCF_ALT|KCF_CONTROL == KC__VANILLA == 7 */

    };

#else

    const BYTE keymaptype_table[8][8] =
    {
        {3, X, X, X, X, X, X, X},       /* KCF_NOQUAL                   == 0 */
        {3, 2, X, X, X, X, X, X},       /* KCF_SHIFT                    == 1 */
        {3, X, 2, X, X, X, X, X},       /* KCF_ALT                      == 2 */
        {3, 2, 1, 0, X, X, X, X},       /* KCF_SHIFT|KCF_ALT            == 3 */
        {3, X, X, X, 2, X, X, X},       /* KCF_CONTROL                  == 4 */
        {3, 2, X, X, 1, 0, X, X},       /* KCF_SHIFT|KCF_CONTROL        == 5 */
        {3, X, 2, X, 1, X, 0, X},       /* KCF_ALT|KCF_CONTROL          == 6 */
        {3, 2, 1, 0, S, X, X, X}        /* KCF_SHIFT|KCF_ALT|KCF_CONTROL == KC__VANILLA == 7 */

    };
#endif

#undef S

/* index vertically: (keytype & KC_VANILLA)
** index horizontally: KCF_xxx qualifier combination for keypress.
** Used to get the number of the string descriptor,
** depending on the KCF_SHIFT, KCF_ALT and KCF_CONTROL qualifiers
** of the key pressed
*/

#if 1

    /* stegerg: see comment before keymaptype_table */
    
    const BYTE keymapstr_table[8][8] =
    {
        {0, 0, 0, 0, 0, 0, 0, 0},       /* KCF_NOQUAL                   == 0 */
        {0, 1, 0, 1, 0, 1, 0, 1},       /* KCF_SHIFT                    == 1 */
        {0, 0, 1, 1, 0, 0, 1, 1},       /* KCF_ALT                      == 2 */
        {0, 1, 2, 3, 0, 1, 2, 3},       /* KCF_SHIFT|KCF_ALT            == 3 */
        {0, 0, 0, 0, 1, 1, 1, 1},       /* KCF_CONTROL                  == 4 */
        {0, 1, 0, 1, 2, 3, 2, 3},       /* KCF_SHIFT|KCF_CONTROL        == 5 */
        {0, 0, 1, 1, 2, 2, 3, 3},       /* KCF_ALT|KCF_CONTROL          == 6 */
        {0, 1, 2, 3, 4, 5, 6, 7}        /* KCF_SHIFT|KCF_ALT|KCF_CONTROL == KC__VANILLA == 7 */

    };
#else
    const BYTE keymapstr_table[8][8] =
    {
        {0, X, X, X, X, X, X, X},       /* KCF_NOQUAL                   == 0 */
        {0, 1, X, X, X, X, X, X},       /* KCF_SHIFT                    == 1 */
        {0, X, 1, X, X, X, X, X},       /* KCF_ALT                      == 2 */
        {0, 1, 2, 3, X, X, X, X},       /* KCF_SHIFT|KCF_ALT            == 3 */
        {0, X, X, X, 1, X, X, X},       /* KCF_CONTROL                  == 4 */
        {0, 1, X, X, 2, 3, X, X},       /* KCF_SHIFT|KCF_CONTROL        == 5 */
        {0, X, 1, X, 2, X, 3, X},       /* KCF_ALT|KCF_CONTROL          == 6 */
        {0, 1, 2, 3, 4, 5, 6, 7}        /* KCF_SHIFT|KCF_ALT|KCF_CONTROL == KC__VANILLA == 7 */

    };
#endif


#undef X
