/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Spanish PC105 Keymap
    Lang: Spanish
*/

#include "common.h"

DEFINE_KEYMAP("pc105_e")

#undef N
#undef S
#undef A
#undef C
#undef D
#undef V
#undef ST
#undef NOP

#define N   KC_NOQUAL
#define S   KCF_SHIFT
#define A   KCF_ALT
#define C   KCF_CONTROL
#define D   KCF_DEAD
#define V   KC_VANILLA
#define ST  KCF_STRING
#define NOP KCF_NOP

#define EUR 0xA4 /* ISO 8859-15: Euro = 164 = 0xA4) */

STATIC CONST UBYTE lokeymaptypes[] =
{
    S|A, 	/* 00 left of 1 */
    S|A,        /* 01 1 */
    V,          /* 02 2 */
    V,          /* 03 3 */
    S|A,        /* 04 4 */
    S|A,	/* 05 */
    S|A,       	/* 06 */
    S|A,       	/* 07 */
    S|A,        /* 08 */
    S|A,        /* 09 */
    S|A,	/* 0A */
    S|A,	/* 0B Right of 0 */
    S|A, 	/* 0C 2nd right of 0 */
    N, 		/* 0D 3rd right of 0 */
    N, 		/* 0E undef */
    S|A, 	/* 0F NUM 0 */

    V, 	    	/* 10 q */
    V,	 	/* 11 w */
    D|V, 	/* 12 e */
    V,	 	/* 13 r */
    V,	 	/* 14 t */
    V,	 	/* 15 y */
    D|V, 	/* 16 u */
    D|V, 	/* 17 i */
    D|V, 	/* 18 o */
    V,	 	/* 19 p */

    D|S|A,      	/* 1A + * */
    S|A, 	/* 1B � ` */
    N, 	        /* 1C undefined */
    N,    	/* 1D NUM 1 */
    N,  	/* 1E NUM 2 */
    N,  	/* 1F NUM 3 */

    D|V,    	/* 20 a */
    V, 		/* 21 s */
    V, 		/* 22 d */
    V, 		/* 23 f */
    V, 		/* 24 g */
    V, 		/* 25 h */
    V, 		/* 26 j */
    V, 		/* 27 k */
    V,	 	/* 28 l */
    S|A,	/* 29 � */

    D|S|A, 	/* 2A � � */
    S|A,     	/* 2B ~ ^ */
    N, 		/* 2C undefined */
    N, 		/* 2D NUM 4 */
    N, 		/* 2E NUM 5 */
    N, 		/* 2F NUM 6 */
    S|A,	/* 30 < > */
    V,	 	/* 31 z */
    V,	 	/* 32 x */
    V,	    	/* 33 c */
    V,	 	/* 34 v */
    V,	 	/* 35 b */
    V,	 	/* 36 n */

    V,	    	/* 37 m */
    S|A,	/* 38 , ; */
    S|A,	/* 39 . : */
    S, 		/* 3A - _ */
    N, 		/* 3B undefined */
    N, 		/* 3C NUM . */
    N, 		/* 3D NUM 7 */
    N, 		/* 3E NUM 8 */
    N, 		/* 3F NUM 9 */
};

STATIC CONST UBYTE hikeymaptypes[] =
{
    D|S,    	/* 40 SPACE */
    N, 		/* 41 BACKSPACE */
    ST|S,	/* 42 TAB */
    N,		/* 43 ENTER */
    C,		/* 44 RETURN */
    A, 	 	/* 45 ESCAPE */
    N, 	 	/* 46 DEL  */
    ST|S,	/* 47 INSERT ?? */
    ST|S,	/* 48 PAGE UP ?? */
    ST|S,	/* 49 PAGE DOWN ?? */
    N,		/* 4A NUMERIC PAD - */
    ST|S,	/* 4B F11 ?? */
    ST|S, 	/* 4C CURSORUP*/
    ST|S, 	/* 4D CURSORDOWN */
    ST|S, 	/* 4E CURSORRIGHT */
    ST|S, 	/* 4F CURSORLEFT */
    ST|S, 	/* 50 F1 */
    ST|S, 	/* 51 F2 */
    ST|S, 	/* 52 F3 */
    ST|S, 	/* 53 F4 */
    ST|S, 	/* 54 F5 */
    ST|S, 	/* 55 F6 */
    ST|S, 	/* 56 F7 */
    ST|S, 	/* 57 F8 */
    ST|S, 	/* 58 F9 */
    ST|S, 	/* 59 F10 */
    NOP, 	/* 5A NUMLOCK */
    N, 		/* 5B NUMPAD ) */
    N, 		/* 5C NUMPAD / */
    N, 		/* 5D NUMPAD * */
    N, 		/* 5E NUMPAD + */
    ST,		/* 5F HELP */
    NOP,	/* 60 LEFT SHIFT*/
    NOP, 	/* 61 RIGHT SHIFT */
    NOP, 	/* 62 CAPS LOCK */
    NOP,	/* 63 CONTROL */
    NOP,	/* 64 LALT */
    NOP,	/* 65 RALT */
    NOP,	/* 66 LCOMMAND */
    NOP, 	/* 67 RCOMMAND */
    NOP, 	/* 68 LEFT MOUSE BUTTON*/
    NOP,	/* 69 RIGHT MOUSE BUTTON */
    NOP,	/* 6A MIDDLE MOUSE BUTTON */
    NOP,	/* 6B */
    NOP,	/* 6C */
    NOP,	/* 6D */
    ST|A,	/* 6E PAUSE/BREAK ??*/
    ST|S, 	/* 6F F12 ?? */
    ST|C, 	/* 70 HOME ?? */
    ST|C, 	/* 71 END ?? */
    NOP, 	/* 72 */
    NOP, 	/* 73 */
    NOP, 	/* 74 */
    NOP, 	/* 75 */
    NOP, 	/* 76 */
    NOP, 	/* 77 */
    NOP, 	/* 78 */
    NOP, 	/* 79 */
    NOP, 	/* 7A */
    NOP, 	/* 7B */
    NOP, 	/* 7C */
    NOP, 	/* 7D */
    NOP, 	/* 7E */
    NOP 	/* 7F */
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
#define DEAD(x)	  (IPTR)x
#define BYTES(b0, b1, b2, b3) \
	(((UBYTE)b0)<<24) | (((UBYTE)b1)<<16) | (((UBYTE)b2)<<8) | (((UBYTE)b3)<<0)

STATIC CONST UBYTE a_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '�', /* 0xE6 */
    0, '�', /* 0xC6 */
    0, 0x01,
    0, 0x01,
    0, 0x81,
    0, 0x81,

    'a', '�' /*0xE1*/, '�' /*0xE0*/, '�' /*0xE2*/, '�' /*0xE3*/, '�' /*0xE4*/, '�' /*0xE5*/,

    'A', '�' /*0xC1*/, '�' /*0xC0*/, '�' /*0xC2*/, '�' /*0xC3*/, '�' /*0xC4*/, '�' /*0xC5*/
};

STATIC CONST UBYTE e_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, EUR,
    0, '�',
    0, 0x05,
    0, 0x05,
    0, 0x85,
    0, 0x85,
    
    'e', '�' /*0xE9*/, '�' /*0xE8*/, '�' /*0xEA*/, 'e', '�' /*0xEB*/, 'e',
    
    'E', '�' /*0xC9*/, '�' /*0xC8*/, '�' /*0xCA*/, 'E', '�' /*0xCB*/, 'E'
};

STATIC CONST UBYTE u_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '�', /* 0xB5 */
    0, '�', /* 0xB5 */
    0, 0x15,
    0, 0x15,
    0, 0x95,
    0, 0x95,
    
    'u', '�' /*0xFA*/, '�' /*0xF9*/, '�' /*0xFB*/, 'u', '�' /*0xFC*/, 'u',
    
    'U', '�' /*0xDA*/, '�' /*0xD9*/, '�' /*0xDB*/, 'U', '�' /*0xDC*/, 'U'
};

STATIC CONST UBYTE i_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '�', /* 0xA1 */
    0, '�', /* 0xA6 */
    0, 0x09,
    0, 0x09,
    0, 0x89,
    0, 0x89,
    
    'i', '�' /*0xED*/, '�' /*0xEC*/, '�' /*0xEE*/, 'i', '�' /*0xEF*/, 'i',
    
    'I', '�' /*0xCD*/, '�' /*0xCC*/, '�' /*0xCE*/, 'I', '�' /*0xCF*/, 'I'
};

STATIC CONST UBYTE o_descr[] =
{
    DPF_MOD, 0x10,
    DPF_MOD, 0x17,
    0, '�', /* 0xF8 */
    0, '�', /* 0xD8 */
    0, 0x0F,
    0, 0x0F,
    0, 0x8F,
    0, 0x8F,
    
    'o', '�' /*0xF3*/, '�' /*0xF2*/, '�' /*0xF4*/, '�' /*0xF5*/, '�' /*0xF6*/, 'o',
    
    'O', '�' /*0xD3*/, '�' /*0xD2*/, '�' /*0xD4*/, '�' /*0xD5*/, '�' /*0xD6*/, 'O'
};
 
STATIC CONST UBYTE space_descr[] =
{
    DPF_MOD, 0x4,
    0, 0XA0,

    ' ', '�' /*0xB4*/, '`','^','~', '�' /*0xA8*/, '�' /*0xB0*/
};

STATIC CONST UBYTE KEY1A_descr[] =
{
    DPF_DEAD, 2,
    DPF_DEAD, 3,
    0, '[',
    0, '['
};

STATIC CONST UBYTE KEY2A_descr[] =
{
    DPF_DEAD, 1,
    DPF_DEAD, 5,
    0, '{',
    0, '{'
};

STATIC CONST IPTR lokeymap[] =
{
    BYTES('\\','\\', '�','�'), 		/* 00 Left of 1 Key */
    BYTES('�', '|', '!', '1'), 		/* 01 1 */
    BYTES(0, '@', '"', '2'), 		/* 02 2 */
    BYTES('�', '#', '�', '3'), 		/* 03 3 */
    BYTES('$', '~', '$', '4'),	 	/* 04 4 */
    BYTES(0, 0, '%', '5'), 		/* 05 5 */
    BYTES(0, '�', '&', '6'), 		/* 06 6 */
    BYTES(0, '{', '/', '7'), 		/* 07 7 */
    BYTES(0, '[', '(', '8'), 		/* 08 8 */
    BYTES('�', ']', ')', '9'),		/* 09 9 */
    BYTES('�', '}', '=', '0'),		/* 0A 0 */
    BYTES('�','\\', '?','\''),	    	/* 0B Right of 0 */
    BYTES('~', '~', '�', '�'),		/* 0C 2nd right of 0 */
    BYTES('|', '|', '|', '|'),		/* 0D 3rd right of 0 */
    BYTES(0, 0, 0, 0),			/* 0E undefined */
    BYTES('0', '0', '0', '0'), 		/* 0F NUM 0 */

    BYTES(0, '@', 'Q', 'q'),    	/* 10 */
    BYTES(0, 0, 'W', 'w'),		/* 11 */
    DEAD(e_descr), 		    	/* 12 */
    BYTES('�', '�', 'R', 'r'), 		/* 13 */
    BYTES(0, 0, 'T', 't'),		/* 14 */
    BYTES('�', 0, 'Y', 'y'),		/* 15 */
    DEAD(u_descr), 		    	/* 16 */
    DEAD(i_descr), 		    	/* 17 */
    DEAD(o_descr),  	 		/* 18 */
    BYTES('�', '�', 'P', 'p'), 		/* 19 */
    DEAD(KEY1A_descr),		    	/* 1A */
    BYTES(0, ']', '*', '+'),	    	/* 1B */
    BYTES(0, 0, 0, 0),			/* 1C undefined */
    BYTES('1', '1', '1', '1'),		/* 1D NUM 1*/
    BYTES('2', '2', '2', '2'),		/* 1E NUM 2*/
    BYTES('3', '3', '3', '3'),		/* 1F NUM 3*/

    DEAD(a_descr),		    	/* 20 */
    BYTES('�', '�', 'S', 's'),		/* 21 */
    BYTES('�', '�', 'D', 'd'),		/* 22 */
    BYTES('�', 0, 'F', 'f'), 		/* 23 */
    BYTES(0, 0, 'G', 'g'), 		/* 24 */
    BYTES(0, 0, 'H', 'h'), 		/* 25 */
    BYTES('J', 'j', 'J', 'j'), 		/* 26 */
    BYTES('&', 0, 'K', 'k'), 		/* 27 */
    BYTES(0, 0, 'L', 'l'), 		/* 28 */
    BYTES('"', '~', '�', '�'), 		/* 29 */

    DEAD(KEY2A_descr),	    	    	/* 2A */
    BYTES(0, '}', '�', '�'), 	    	/* 2B */
    BYTES(0, 0, 0, 0),			/* 2C undefined */
    BYTES('4', '4', '4', '4'),		/* 2D NUM 4 */
    BYTES('5', '5', '5', '5'), 		/* 2E NUM 5 */
    BYTES('6', '6', '6', '6'), 		/* 2F NUM 6 */
    BYTES(0, '|', '>', '<'),	    	/* 30 */
    BYTES('<', '�', 'Z', 'z'),		/* 31 */
    BYTES('>', '�', 'X', 'x'),		/* 32 */
    BYTES('�', '�', 'C', 'c'),	    	/* 33 */
    BYTES('`', 0, 'V', 'v'),		/* 34 */
    BYTES('\'', 0, 'B', 'b'),		/* 35 */
    BYTES('N', 'n', 'N', 'n'),		/* 36 */

    BYTES('�', '�', 'M', 'm'),	    	/* 37 */
    BYTES('�', 0, ';', ','),		/* 38 */
    BYTES('�', '�', ':', '.'),		/* 39 */
    BYTES(0, 0, '_', '-'),		/* 3A */
    BYTES(0, 0, 0, 0),	/* 3B */
    BYTES('.', '.', '.', '.'),		/* 3C NUM . */
    BYTES('7', '7', '7', '7'),		/* 3D NUM 7 */
    BYTES('8', '8', '8', '8'),		/* 3E NUM 8 */
    BYTES('9', '9', '9', '9'),		/* 3F NUM 9 */
};

#include "standard.h"

STATIC CONST IPTR hikeymap[] =
{
    DEAD(space_descr),	    	/* 40 SPACE */
    BYTES(8, 8, 8, 8),		/* 41 BACKSPACE*/
    STRING(tab_descr),		/* 42 TAB */
    BYTES(13, 13, 13, 13),	/* 43 ENTER */
    BYTES(0, 0, 10, 13),	/* 44 RETURN */
    BYTES(0, 0, 0x9B, 27),	/* 45 ESCAPE */
    BYTES(127, 127, 127, 127),	/* 46 DEL */
    STRING(insert_descr),	/* 47 INSERT ?? */
    STRING(pageup_descr),	/* 48 PAGEUP ?? */
    STRING(pagedown_descr),	/* 49 PAGEDOWN ?? */
    BYTES('-', '-', '-', '-'),	/* 4A NUMPAD - */
    STRING(f11_descr), 		/* 4B F11 ?? */
    STRING(up_descr),		/* 4C CURSOR UP*/
    STRING(down_descr),		/* 4D CURSOR DOWN*/
    STRING(right_descr),	/* 4E CURSOR RIGHT */
    STRING(left_descr),		/* 4F CURSOR LEFT */
    STRING(f1_descr),		/* 50 F1 */
    STRING(f2_descr),		/* 51 */
    STRING(f3_descr),		/* 52 */
    STRING(f4_descr),		/* 53 */
    STRING(f5_descr),		/* 54 */
    STRING(f6_descr),		/* 55 */
    STRING(f7_descr),		/* 56 */
    STRING(f8_descr),		/* 57 */
    STRING(f9_descr),		/* 58 */
    STRING(f10_descr),		/* 59 */
    BYTES(0, 0, 0, 0),		/* 5A */
    BYTES('/', '/', '/', '/'),	/* 5B */
    BYTES('*', '*', '*', '*'),	/* 5C */
    BYTES('-', '-', '-', '-'),	/* 5D */
    BYTES('+', '+', '+', '+'),	/* 5E */
    STRING(help_descr),		/* 5F HELP */
    BYTES(0, 0, 0, 0),		/* 60 */
    BYTES(0, 0, 0, 0),		/* 61 */
    BYTES(0, 0, 0, 0),		/* 62 */
    BYTES(0, 0, 0, 0),		/* 63 */
    BYTES(0, 0, 0, 0),		/* 64 */
    BYTES(0, 0, 0, 0),		/* 65 */
    BYTES(0, 0, 0, 0),		/* 66 */
    BYTES(0, 0, 0, 0),		/* 67 */
    BYTES(0, 0, 0, 0),		/* 68 */
    BYTES(0, 0, 0, 0),		/* 69 */
    BYTES(0, 0, 0, 0),		/* 6A */
    BYTES(0, 0, 0, 0),		/* 6B */
    BYTES(0, 0, 0, 0),		/* 6C */
    BYTES(0, 0, 0, 0),		/* 6D */
    STRING(pausebreak_descr),	/* 6E PAUSE/BREAK ?? */
    STRING(f12_descr),		/* 6F F12 ?? */
    STRING(home_descr),		/* 70 HOME ?? */
    STRING(end_descr),		/* 71 END ?? */
    BYTES(0, 0, 0, 0),		/* 72 */
    BYTES(0, 0, 0, 0),		/* 73 */
    BYTES(0, 0, 0, 0),		/* 74 */
    BYTES(0, 0, 0, 0),		/* 75 */
    BYTES(0, 0, 0, 0),		/* 76 */
    BYTES(0, 0, 0, 0),		/* 77 */
    BYTES(0, 0, 0, 0),		/* 78 */
    BYTES(0, 0, 0, 0),		/* 79 */
    BYTES(0, 0, 0, 0),		/* 7A */
    BYTES(0, 0, 0, 0),		/* 7B */
    BYTES(0, 0, 0, 0),		/* 7C */
    BYTES(0, 0, 0, 0),		/* 7D */
    BYTES(0, 0, 0, 0),		/* 7E */
    BYTES(0, 0, 0, 0),		/* 7F */
};

#undef SETBITS

#define SETBITS(b0, b1, b2, b3, b4, b5, b6, b7) \
	(b0<<0)|(b1<<1)|(b2<<2)|(b3<<3)|(b4<<4)|(b5<<5)|(b6<<6)|(b7<<7)
	
STATIC CONST UBYTE locapsable[] =
{
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 00 - 07 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 08 - 0F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 10 - 17 */
    SETBITS(1, 1, 0, 0, 0, 0, 0, 0),	/* 18 - 1F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 20 - 27 */
    SETBITS(1, 1, 0, 1, 0, 0, 0, 0),	/* 28 - 2F */
    
    SETBITS(0, 1, 1, 1, 1, 1, 1, 1),	/* 30 - 37 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)	/* 38 - 3F */
};

STATIC CONST UBYTE hicapsable[] =
{
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 40 - 47 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 48 - 4F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 50 - 57 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 58 - 5F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 60 - 67 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 68 - 6F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 70 - 77 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)	/* 78 - 7F */
};

STATIC CONST UBYTE lorepeatable[] =
{
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 00 - 07 */
    SETBITS(1, 1, 1, 1, 1, 1, 0, 1),	/* 08 - 0F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 10 - 17 */
    SETBITS(1, 1, 1, 1, 0, 1, 1, 1),	/* 18 - 1F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 20 - 27 */
    SETBITS(1, 1, 1, 1, 0, 1, 1, 1),	/* 28 - 2F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 30 - 37 */
    SETBITS(1, 1, 1, 0, 1, 1, 1, 1)	/* 38 - 3F */
};

STATIC CONST UBYTE hirepeatable[] =
{
    SETBITS(1, 1, 1, 0, 0, 0, 1, 0),	/* 40 - 47 */
    SETBITS(1, 1, 0, 0, 1, 1, 1, 1),	/* 48 - 4F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 50 - 57 */
    SETBITS(0, 0, 1, 1, 1, 1, 1, 0),	/* 58 - 5F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 60 - 67 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 68 - 6F */
    
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 70 - 77 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)	/* 78 - 7F */
};
