/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <devices/keymap.h>


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

static CONST UBYTE lokeymaptypes[] =
{
    S,          /* 00 */
    S|A,        /* 01 */
    S|A,        /* 02 */
    S|A,        /* 03 */
    S|A,        /* 04 */
    S|A,        /* 05 */
    S|A,        /* 06 */
    S|A,        /* 07 */
    S|A,        /* 08 */
    S|A,        /* 09 */
    S|A,        /* 0A */
    S,          /* 0B */
    S,          /* 0C */
    S,          /* 0D */
    N,          /* 0E */
    S|A,        /* 0F */
    V,          /* 10 q */
    V,          /* 11 w */
    V,          /* 12 e */
    V,          /* 13 r */
    V,          /* 14 t */
    V,          /* 15 z */
    V,          /* 16 u */
    V,          /* 17 i */
    V,          /* 18 o */
    V,          /* 19 p */
    S|A,        /* 1A */
    S|A,        /* 1B */
    S|A,        /* 1C */
    S|A,        /* 1D */
    S|A,        /* 1E */
    S|A,        /* 1F */
    V,          /* 20 a */
    V,          /* 21 s */
    V,          /* 22 d */
    V,          /* 23 f */
    V,          /* 24 g */
    V,          /* 25 h */
    V,          /* 26 j */
    V,          /* 27 k */
    V,          /* 28 l */
    S,          /* 29 */
    S,          /* 2A */
    V,          /* 2B */
    N,          /* 2C */
    N,          /* 2D */
    N,          /* 2E */
    N,          /* 2F */
    S,          /* 30 */
    V,          /* 31 y */
    V,          /* 32 x */
    V,          /* 33 c */
    V,          /* 34 v */
    V,          /* 35 b */
    V,          /* 36 n */
    V,          /* 37 m */
    S|A,        /* 38 */
    S|A,        /* 39 */
    S,          /* 3A */
    N,          /* 3B */
    N,          /* 3C */
    N,          /* 3D */
    N,          /* 3E */
    N,          /* 3F */
    
};

static CONST UBYTE hikeymaptypes[] =
{
    N,          /* 40 SPACE */
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
    NOP,        /* 77 */
    NOP,        /* 78 */
    NOP,        /* 79 */
    NOP,        /* 7A */
    NOP,        /* 7B */
    NOP,        /* 7C */
    NOP,        /* 7D */
    NOP,        /* 7E */
    NOP         /* 7F */
    
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

static CONST IPTR lokeymap[] =
{
    BYTES('~', '`', '~', '`'),          /* 00 Left of 1 Key */
    BYTES(0xB9, 0xA1, '!', '1'),        /* 01 1 */
    BYTES(0xB2, '@', '@', '2'),         /* 02 2 */
    BYTES(0xB3, 0xA3, '#', '3'),        /* 03 3 */
    BYTES(0xBC, 0xA4, '$', '4'),        /* 04 4 */
    BYTES(0xBD, 0xBD, '%', '5'),        /* 05 5 */
    BYTES(0xBE, 0xBE, '^', '6'),        /* 06 6 */
    BYTES(0xF7, '{', '&', '7'),         /* 07 7 */
    BYTES('[', '[', '*', '8'),          /* 08 8 */
    BYTES(']', ']', '(', '9'),          /* 09 9 */
    BYTES('}', '}', ')', '0'),          /* 0A 0 */
    BYTES('_', '-', '_', '-'),          /* 0B Right of 0 */
    BYTES('+', '=', '+', '='),          /* 0C 2nd right of 0 */
    BYTES('|', '\\', '|', '\\'),        /* 0D 3rd right of 0 */
    BYTES(0, 0, 0, 0),                  /* 0E undefined */
    BYTES('0', '0', '0', '0'),          /* 0F NUM 0 */
    BYTES('Q', 'q', 'Q', 'q'),          /* 10 */
    BYTES('W', 'w', 'W', 'w'),          /* 11 */
    BYTES('E', 'e', 'E', 'e'),          /* 12 */
    BYTES('R', 'r', 'R', 'r'),          /* 13 */
    BYTES('T', 't', 'T', 't'),          /* 14 */
    BYTES('Y', 'y', 'Y', 'y'),          /* 15 */
    BYTES('U', 'u', 'U', 'u'),          /* 16 */
    BYTES('I', 'i', 'I', 'i'),          /* 17 */
    BYTES('O', 'o', 'O', 'o'),          /* 18 */
    BYTES('P', 'p', 'P', 'p'),          /* 19 */
    
    BYTES('{', '[', '{', '['),          /* 1A */
    BYTES('}', ']', '}', ']'),          /* 1B */
    BYTES(0, 0, 0, 0),                  /* 1C undefined */
    BYTES('1', '1', '1', '1'),          /* 1D NUM 1*/
    BYTES('2', '2', '2', '2'),          /* 1E NUM 2*/
    BYTES('3', '3', '3', '3'),          /* 1F NUM 3*/
    
    BYTES('A', 'a', 'A', 'a'),          /* 20 */
    BYTES('S', 's', 'S', 's'),          /* 21 */
    BYTES('D', 'd', 'D', 'd'),          /* 22 */
    BYTES('F', 'f', 'F', 'f'),          /* 23 */
    BYTES('G', 'g', 'G', 'g'),          /* 24 */
    BYTES('H', 'h', 'H', 'h'),          /* 25 */
    BYTES('J', 'j', 'J', 'j'),          /* 26 */
    BYTES('K', 'k', 'K', 'k'),          /* 27 */
    BYTES('L', 'l', 'L', 'l'),          /* 28 */
    
    BYTES(':', ';', ':', ';'),          /* 29 */
    BYTES('"', 0x27, '"', 0x27),        /* 2A */
    BYTES('|', '\\', '|', '\\'),        /* 2B */
    BYTES(0, 0, 0, 0),                  /* 2C undefined */
    BYTES('4', '4', '4', '4'),          /* 2D NUM 4 */
    BYTES('5', '5', '5', '5'),          /* 2E NUM 5 */
    BYTES('6', '6', '6', '6'),          /* 2F NUM 6 */
    BYTES('>', '<', '>', '<'),           /* 30 */
    
    BYTES('Z', 'z', 'Z', 'z'),          /* 31 */
    BYTES('X', 'x', 'X', 'x'),          /* 32 */
    BYTES('C', 'c', 'C', 'c'),          /* 33 */
    BYTES('V', 'v', 'V', 'v'),          /* 34 */
    BYTES('B', 'b', 'B', 'b'),          /* 35 */
    BYTES('N', 'n', 'N', 'n'),          /* 36 */
    BYTES('M', 'm', 'M', 'm'),          /* 37 */
    
    BYTES('<', ',', '<', ','),          /* 38 */
    BYTES('>', '.', '>', '.'),          /* 39 */
    BYTES('?', '/', '?', '/'),          /* 3A */
    BYTES(0, 0, 0, 0),  /* 3B */
    BYTES('.', '.', '.', '.'),          /* 3C NUM . */
    BYTES('7', '7', '7', '7'),          /* 3D NUM 7 */
    BYTES('8', '8', '8', '8'),          /* 3E NUM 8 */
    BYTES('9', '9', '9', '9'),          /* 3F NUM 9 */
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

static CONST IPTR hikeymap[] =
{
    BYTES(' ', ' ', ' ', ' '),  /* 40 */
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
    BYTES(0, 0, 0, 0),          /* 77 */
    BYTES(0, 0, 0, 0),          /* 78 */
    BYTES(0, 0, 0, 0),          /* 79 */
    BYTES(0, 0, 0, 0),          /* 7A */
    BYTES(0, 0, 0, 0),          /* 7B */
    BYTES(0, 0, 0, 0),          /* 7C */
    BYTES(0, 0, 0, 0),          /* 7D */
    BYTES(0, 0, 0, 0),          /* 7E */
    BYTES(0, 0, 0, 0),          /* 7F */
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
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 70 - 77 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)     /* 78 - 7F */
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
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),    /* 70 - 77 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)     /* 78 - 7F */
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
