// Kbdhu_Hungarian_pkf.txt
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
    Desc: unknown PC105 Keymap
    Lang: unknown
*/

#include <devices/keymap.h>

#define KMNAME "pc105_xx"

#define CONST const

extern CONST char  keymapname[];

extern CONST UBYTE lokeymaptypes[];
extern CONST IPTR  lokeymap[];
extern CONST UBYTE locapsable[];
extern CONST UBYTE lorepeatable[];

extern CONST UBYTE hikeymaptypes[];
extern CONST IPTR  hikeymap[];
extern CONST UBYTE hicapsable[];
extern CONST UBYTE hirepeatable[];

CONST struct KeyMapNode km =
{
 #if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    {
    	NULL, NULL, 0, 0, keymapname
    },
 #else
    {
    	NULL, NULL, keymapname, 0, 0
    },
 #endif
    {
    	(UBYTE *)lokeymaptypes,
    	(IPTR  *)lokeymap,
    	(UBYTE *)locapsable,
    	(UBYTE *)lorepeatable,
    	(UBYTE *)hikeymaptypes,
    	(IPTR  *)hikeymap,
    	(UBYTE *)hicapsable,
    	(UBYTE *)hirepeatable
    }
};

CONST char keymapname[] = KMNAME;

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

CONST UBYTE lokeymaptypes[] =
{
    S, 		/* 00 */
    S, 		/* 01 */
    S|A,	/* 02 */
    S|A,	/* 03 */
    S|A,	/* 04 */
    S,		/* 05 */
    S,		/* 06 */
    S,	    	/* 07 */
    S,	    	/* 08 */
    S,	    	/* 09 */
    S,	    	/* 0A */
    S,	    	/* 0B */
    S, 		/* 0C */
    N, 		/* 0D */
    N, 		/* 0E */
    S|A, 	/* 0F */
    V,	 	/* 10 q */
    V,	 	/* 11 w */
    V,	 	/* 12 e */
    V,	 	/* 13 r */
    V,	 	/* 14 t */
    V,	 	/* 15 z */
    V,	 	/* 16 u */
    V,	 	/* 17 i */
    V,	 	/* 18 o */
    V,	 	/* 19 p */
    S, 		/* 1A */
    S|A, 	/* 1B */
    S|A, 	/* 1C */
    S|A, 	/* 1D */
    S|A, 	/* 1E */
    S|A, 	/* 1F */
    V, 		/* 20 a */
    V, 		/* 21 s */
    V, 		/* 22 d */
    V, 		/* 23 f */
    V, 		/* 24 g */
    V, 		/* 25 h */
    V, 		/* 26 j */
    V, 		/* 27 k */
    V,	 	/* 28 l */
    S, 		/* 29 */
    S, 		/* 2A */
    S, 		/* 2B */
    N, 		/* 2C */
    N, 		/* 2D */
    N, 		/* 2E */
    N, 		/* 2F */
    S|A,	/* 30 */
    V,	 	/* 31 y */
    V,	 	/* 32 x */
    V,	 	/* 33 c */
    V,	 	/* 34 v */
    V,	 	/* 35 b */
    V,	 	/* 36 n */
    V,	 	/* 37 m */
    S|A,	/* 38 */
    S|A,	/* 39 */
    S, 		/* 3A */
    N, 		/* 3B */
    N, 		/* 3C */
    N, 		/* 3D */
    N, 		/* 3E */
    N, 		/* 3F */
    
};

CONST UBYTE hikeymaptypes[] =
{
    N, 		/* 40 SPACE */
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





CONST IPTR lokeymap[] ={
/*---- 1st QWERTY row - 0x00 to 0x0F */
 BYTES('.' ,'.' ,0xA7,'0' ),
 BYTES(0x7E,0x7E,0x27,'1' ),
 BYTES(0xA1,0xA1,0x22,'2' ),
 BYTES(0x5E,0x5E,'+' ,'3' ),
 BYTES(0xA2,0xA2,'!' ,'4' ),
 BYTES(0xB0,0xB0,'%' ,'5' ),
 BYTES(0xB2,0xB2,0x2F,'6' ),
 BYTES(0x60,0x60,'=' ,'7' ),
 BYTES(0xFF,0xFF,'(' ,'8' ),
 BYTES(0xB4,0xB4,')' ,'9' ),
 BYTES(0xBD,0xBD,0xD6,0xF6),
 BYTES(0xA8,0xA8,0xDC,0xFC),
 BYTES(0xB8,0xB8,0xD3,0xF3),
 BYTES('.' ,'.' ,0xA7,'0' ),
BYTES(0,   0,   0,   0  ),
BYTES('0', '0', '0', '0'),
/*---- 2nd QWERTY row - 0x10 to 0x1F */
 BYTES(0x5C,0x5C,'Q' ,'q' ),
 BYTES(0x7C,0x7C,'W' ,'w' ),
 BYTES(0xC4,0xC4,'E' ,'e' ),
 BYTES('.' ,'.' ,'R' ,'r' ),
 BYTES('.' ,'.' ,'T' ,'t' ),
 BYTES('.' ,'.' ,'Z' ,'z' ),
 BYTES(0x80,0x80,'U' ,'u' ),
 BYTES(0xCD,0xCD,'I' ,'i' ),
 BYTES('.' ,'.' ,'O' ,'o' ),
 BYTES('.' ,'.' ,'P' ,'p' ),
 BYTES(0xF7,0xF7,0xD5,0xF5),
 BYTES(0xD7,0xD7,0xDA,0xFA),
BYTES(0,   0,   0,   0  ),
BYTES('1', '1', '1', '1'),
BYTES('2', '2', '2', '2'),
BYTES('3', '3', '3', '3'),
/*---- 3rd QWERTY row - 0x20 to 0x2F */
 BYTES(0xE4,0xE4,'A' ,'a' ),
 BYTES(0xF0,0xF0,'S' ,'s' ),
 BYTES(0xD0,0xD0,'D' ,'d' ),
 BYTES('[' ,'[' ,'F' ,'f' ),
 BYTES(']' ,']' ,'G' ,'g' ),
 BYTES('.' ,'.' ,'H' ,'h' ),
 BYTES(0xED,0xED,'J' ,'j' ),
 BYTES(0xB3,0xB3,'K' ,'k' ),
 BYTES(0xA3,0xA3,'L' ,'l' ),
 BYTES('$' ,'$' ,0xC9,0xE9),
 BYTES(0xDF,0xDF,0xC1,0xE1),
 BYTES(0xA4,0xA4,0xDB,0xFB),
BYTES(0,   0,   0,   0  ),
BYTES('4', '4', '4', '4'),
BYTES('5', '5', '5', '5'),
BYTES('6', '6', '6', '6'),
/*---- 4th QWERTY row - 0x30 to 0x3F */
 BYTES('<' ,'<' ,0xCD,0xED),
 BYTES('>' ,'>' ,'Y' ,'y' ),
 BYTES('#' ,'#' ,'X' ,'x' ),
 BYTES('&' ,'&' ,'C' ,'c' ),
 BYTES('@' ,'@' ,'V' ,'v' ),
 BYTES('{' ,'{' ,'B' ,'b' ),
 BYTES('}' ,'}' ,'N' ,'n' ),
 BYTES('<' ,'<' ,'M' ,'m' ),
 BYTES(';' ,';' ,'?' ,',' ),
 BYTES('>' ,'>' ,':' ,'.' ),
 BYTES('*' ,'*' ,'_' ,'-' ),
BYTES(0,   0,   0,   0  ),
BYTES(',', '.', ',', '.'),
BYTES('7', '7', '7', '7'),
BYTES('8', '8', '8', '8'),
BYTES('9', '9', '9', '9'),};



#include "standard.h"

CONST IPTR hikeymap[] =
{
    BYTES(' ', ' ', ' ', ' '),	/* 40 */
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
	
CONST UBYTE locapsable[] =
{
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 00 - 07 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0),	/* 08 - 0F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 10 - 17 */
    SETBITS(1, 1, 0, 0, 0, 0, 0, 0),	/* 18 - 1F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 20 - 27 */
    SETBITS(1, 0, 0, 0, 0, 0, 0, 0),	/* 28 - 2F */
    
    SETBITS(0, 1, 1, 1, 1, 1, 1, 1),	/* 30 - 37 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)	/* 38 - 3F */
};

CONST UBYTE hicapsable[] =
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

CONST UBYTE lorepeatable[] =
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

CONST UBYTE hirepeatable[] =
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

