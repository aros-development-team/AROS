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

static UBYTE lokeymaptypes[] =
{
    N, 		/* 00 */
    S|A, 	/* 01 */
    S|A,	/* 02 */
    S|A,	/* 03 */
    S|A,	/* 04 */
    S|A,	/* 05 */
    S|A,	/* 06 */
    S|A,	/* 07 */
    S|A,	/* 08 */
    S|A,	/* 09 */
    S|A,	/* 0A */
    N, 		/* 0B */
    N, 		/* 0C */
    N, 		/* 0D */
    N, 		/* 0E */
    S|A, 	/* 0F */
    S|A, 	/* 10 */
    S|A, 	/* 11 */
    S|A, 	/* 12 */
    S|A, 	/* 13 */
    S|A, 	/* 14 */
    S|A, 	/* 15 */
    S|A, 	/* 16 */
    S|A, 	/* 17 */
    S|A, 	/* 18 */
    S|A, 	/* 19 */
    S|A, 	/* 1A */
    S|A, 	/* 1B */
    S|A, 	/* 1C */
    S|A, 	/* 1D */
    S|A, 	/* 1E */
    S|A, 	/* 1F */
    S|A, 	/* 20 */
    S|A, 	/* 21 */
    S|A, 	/* 22 */
    S|A, 	/* 23 */
    S|A, 	/* 24 */
    S|A, 	/* 25 */
    S|A, 	/* 26 */
    S|A, 	/* 27 */
    S|A, 	/* 28 */
    N, 		/* 29 */
    N, 		/* 2A */
    N, 		/* 2B */
    N, 		/* 2C */
    N, 		/* 2D */
    N, 		/* 2E */
    N, 		/* 2F */
    N, 		/* 30 */
    S|A, 	/* 31 */
    S|A, 	/* 32 */
    S|A, 	/* 33 */
    S|A, 	/* 34 */
    S|A, 	/* 35 */
    S|A, 	/* 36 */
    S|A, 	/* 37 */
    S|A,	/* 38 */
    S|A,	/* 39 */
    N, 		/* 3A */
    N, 		/* 3B */
    N, 		/* 3C */
    N, 		/* 3D */
    N, 		/* 3E */
    N, 		/* 3F */
    
};

static UBYTE hikeymaptypes[] =
{
    N, 		/* 40 */
    V, 	/* 41 */
    V,	/* 42 */
    V,	/* 43 */
    V,	/* 44 */
    V,  /* 45 ESCAPE */
    V,  /* 46 DEL  */
    V,	/* 47 */
    V,	/* 48 */
    V,	/* 49 */
    V,	/* 4A */
    V, 	/* 4B */
    S, 		/* 4C */
    S, 		/* 4D */
    S, 		/* 4E */
    S, 	/* 4F */
    V|ST, 		/* 50 */
    S, 		/* 51 */
    S, 		/* 52 */
    S, 		/* 53 */
    S, 		/* 54 */
    S, 		/* 55 */
    S, 		/* 56 */
    S, 		/* 57 */
    S, 		/* 58 */
    S, 		/* 59 */
    S, 		/* 5A */
    S, 		/* 5B */
    S, 		/* 5C */
    S, 		/* 5D */
    S, 		/* 5E */
    NOP,	/* 5F HELP */
    S, 		/* 60 */
    S, 		/* 61 */
    S, 		/* 62 */
    NOP,	/* 63 CONTROL */
    NOP,	/* 64 LALT */
    NOP,	/* 65 RALT */
    NOP,	/* 66 LCOMMAND */
    NOP, 	/* 67 RCOMMAND */
    S, 		/* 68 */
    S, 		/* 69 */
    S, 		/* 6A */
    S, 		/* 6B */
    S, 		/* 6C */
    S, 		/* 6D */
    S, 		/* 6E */
    S, 		/* 6F */
    S, 		/* 70 */
    S, 		/* 71 */
    S, 		/* 72 */
    S, 		/* 73 */
    S, 		/* 74 */
    S, 		/* 75 */
    S, 		/* 76 */
    S, 		/* 77 */
    S, 		/* 78 */
    S, 		/* 79 */
    S, 		/* 7A */
    S, 		/* 7B */
    S, 		/* 7C */
    S, 		/* 7D */
    S, 		/* 7E */
    S, 		/* 7F */
    
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
	(b3<<24)|(b2<<16)|(b1<<8)|(b0<<0)

static IPTR lokeymap[] =
{
    BYTES(0, 0, 0, 0x7C), 		/* 00 */
    BYTES(0xB9, 0xA1, '!', '1'), 	/* 01 */
    BYTES(0xB2, '@', '"', '2'), 	/* 02 */
    BYTES(0xB3, 0xA3, '#', '3'), 	/* 03 */
    BYTES(0xBC, 0xA4, '$', '4'), 	/* 04 */
    BYTES(0xBD, 0xBD, '%', '5'), 	/* 05 */
    BYTES(0xBE, 0xBE, '&', '6'), 	/* 06 */
    BYTES(0xF7, '{', '/', '7'), 	/* 07 */
    BYTES('[', '[', '(', '8'), 		/* 08 */
    BYTES(']', ']', ')', '9'),		/* 09 */
    BYTES('}', '}', '=', '0'),		/* 0A */
    BYTES(0, 0, 0, 0),	/* 0B */
    BYTES(0, 0, 0, 0),	/* 0C */
    BYTES(0, 0, 0, 0),	/* 0D */
    BYTES(0, 0, 0, 0),	/* 0E */
    BYTES('0', '0', '0', '0'), 		/* 0F */
    BYTES('Q', 'q', 'Q', 'q'), 		/* 10 */
    BYTES('W', 'w', 'W', 'w'),		/* 11 */
    BYTES('E', 'e', 'E', 'e'), 		/* 12 */
    BYTES('R', 'r', 'R', 'r'), 		/* 13 */
    BYTES('T', 't', 'T', 't'),		/* 14 */
    BYTES('Y', 'y', 'Y', 'y'),		/* 15 */
    BYTES('U', 'u', 'U', 'u'), 		/* 16 */
    BYTES('I', 'i', 'I', 'i'), 		/* 17 */
    BYTES('O', 'o', 'O', 'o'), 		/* 18 */
    BYTES('P', 'p', 'P', 'p'), 		/* 19 */
    
    BYTES(0, 0, 0, 0),	/* 1A */
    BYTES(0, 0, 0, 0),	/* 1B */
    BYTES(0, 0, 0, 0),	/* 1C */
    BYTES('1', '1', '1', '1'),		/* 1D */
    BYTES('2', '2', '2', '2'),		/* 1E */
    BYTES('3', '3', '3', '3'),		/* 1F */
    
    BYTES('A', 'a', 'A', 'a'),		/* 20 */
    BYTES('S', 's', 'S', 's'),		/* 21 */
    BYTES('D', 'd', 'D', 'd'),		/* 22 */
    BYTES('F', 'f', 'F', 'f'), 		/* 23 */
    BYTES('G', 'g', 'G', 'g'), 		/* 24 */
    BYTES('H', 'h', 'H', 'h'), 		/* 25 */
    BYTES('J', 'j', 'J', 'j'), 		/* 26 */
    BYTES('K', 'k', 'K', 'k'), 		/* 27 */
    BYTES('L', 'l', 'L', 'l'), 		/* 28 */
    
    BYTES(0, 0, 0, 0), /* 29 */
    BYTES(0, 0, 0, 0), /* 2A */
    BYTES(0, 0, 0, 0), /* 2B */
    BYTES(0, 0, 0, 0), /* 2C */
    BYTES('4', '4', '4', '4'),		/* 2D */
    BYTES('5', '5', '5', '5'), 		/* 2E */
    BYTES('6', '6', '6', '6'), 		/* 2F */
    BYTES(0, 0, 0, 0), /* 30 */
    
    BYTES('Z', 'z', 'Z', 'z'),		/* 31 */
    BYTES('X', 'x', 'X', 'x'),		/* 32 */
    BYTES('C', 'c', 'C', 'c'),		/* 33 */
    BYTES('V', 'v', 'V', 'v'),		/* 34 */
    BYTES('B', 'b', 'B', 'b'),		/* 35 */
    BYTES('N', 'n', 'N', 'n'),		/* 36 */
    BYTES('M', 'm', 'M', 'm'),		/* 37 */
    
    BYTES(';', ',', ';', ','),		/* 38 */
    BYTES(':', '.', ':', '.'),		/* 39 */
    BYTES(0, 0, 0, 0),	/* 3A */
    BYTES(0, 0, 0, 0),	/* 3B */
    BYTES('.', '.', '.', '.'),		/* 3C */
    BYTES('7', '7', '7', '7'),		/* 3D */
    BYTES('8', '8', '8', '8'),		/* 3E */
    BYTES('9', '9', '9', '9'),		/* 3F */
};

/* Strings for the F1 key. In a real AmigaOS keymap, these would have come after
** the HiKeyMap, but we do it this way to avoid prototyping
*/
#define N_F1 	"F1"
#define S_F1 	"S-F1"
#define A_F1	"A-F1"
#define SA_F1	"S-A-F1"
#define C_F1	"C-F1"
#define SC_F1	"S-C-F1"
#define AC_F1	"A-C-F1"
#define SAC_F1	"S-A-C-F1"

#define F1_DS 16 /* descriptor array size */
#define s(x) (sizeof(x) - 1) /* substract 0 terminator */

UBYTE f1_descr[] =
{
    s(N_F1),
    F1_DS + 0,
    
    s(S_F1),
    F1_DS + s(N_F1),
    
    s(A_F1),
    F1_DS + s(N_F1) + s(S_F1),
    
    s(SA_F1),
    F1_DS + s(N_F1) + s(S_F1) + s(A_F1),
    
    s(C_F1),
    F1_DS + s(N_F1) + s(S_F1) + s(A_F1) + s(SA_F1),
    
    s(SC_F1),
    F1_DS + s(N_F1) + s(S_F1) + s(A_F1) + s(SA_F1) + s(C_F1),
    
    s(AC_F1),
    F1_DS + s(N_F1) + s(S_F1) + s(A_F1) + s(SA_F1) + s(C_F1) + s(SC_F1),
    
    s(SAC_F1),
    F1_DS + s(N_F1) + s(S_F1) + s(A_F1) + s(SA_F1) + s(C_F1) + s(SC_F1) + s(AC_F1),
    
    				'F','1',
    'S','-',			'F','1',
    'A','-',			'F','1',
    'S','-','A','-',		'F','1',
    'C','-',			'F','1',
    'S','-','C','-',		'F','1',
    'A','-','C','-',		'F','1',
    'S','-','A','-','C','-',	'F','1'
    
};


static IPTR hikeymap[] =
{
    BYTES(' ', ' ', ' ', ' '),	/* 40 */
    BYTES(8, 8, 8, 8),		/* 41 BACKSPACE*/
    BYTES(0, 0, 0, 0),	/* 42 */
    BYTES(13, 13, 13, 13),	/* 43 ENTER */
    BYTES(13, 13, 13, 13),	/* 44 RETURN */
    BYTES(27, 27, 27, 27),	/* 45 ESCAPE */
    BYTES(127, 127, 127, 127),	/* 46 DEL */
    BYTES(0, 0, 0, 0),	/* 47 */
    BYTES(0, 0, 0, 0),	/* 48 */
    BYTES(0, 0, 0, 0),	/* 49 */
    BYTES('-', '-', '-', '-'),		/* 4A */
    BYTES(0, 0, 0, 0), 	/* 4B */
    BYTES(0, 0, 0, 0),	/* 4C */
    BYTES(0, 0, 0, 0),	/* 4D */
    BYTES(0, 0, 0, 0),	/* 4E */
    BYTES(0, 0, 0, 0),	/* 4F */
    STRING(f1_descr),	/* 50 */
    BYTES(0, 0, 0, 0),	/* 51 */
    BYTES(0, 0, 0, 0),	/* 52 */
    BYTES(0, 0, 0, 0),	/* 53 */
    BYTES(0, 0, 0, 0),	/* 54 */
    BYTES(0, 0, 0, 0),	/* 55 */
    BYTES(0, 0, 0, 0),	/* 56 */
    BYTES(0, 0, 0, 0),	/* 57 */
    BYTES(0, 0, 0, 0),	/* 58 */
    BYTES(0, 0, 0, 0),	/* 59 */
    BYTES(0, 0, 0, 0),	/* 5A */
    BYTES(0, 0, 0, 0),	/* 5B */
    BYTES('/', '/', '/', '/'),		/* 5C */
    BYTES('*', '*', '*', '*'),		/* 5D */
    BYTES('+', '+', '+', '+'),		/* 5E */
    BYTES(0, 0, 0, 0),	/* 5F */
    BYTES(0, 0, 0, 0),	/* 60 */
    BYTES(0, 0, 0, 0),	/* 61 */
    BYTES(0, 0, 0, 0),	/* 62 */
    BYTES(0, 0, 0, 0),	/* 63 */
    BYTES(0, 0, 0, 0),	/* 64 */
    BYTES(0, 0, 0, 0),	/* 65 */
    BYTES(0, 0, 0, 0),	/* 66 */
    BYTES(0, 0, 0, 0),	/* 67 */
    BYTES(0, 0, 0, 0),	/* 68 */
    BYTES(0, 0, 0, 0),	/* 69 */
    BYTES(0, 0, 0, 0),	/* 6A */
    BYTES(0, 0, 0, 0),	/* 6B */
    BYTES(0, 0, 0, 0),	/* 6C */
    BYTES(0, 0, 0, 0),	/* 6D */
    BYTES(0, 0, 0, 0),	/* 6E */
    BYTES(0, 0, 0, 0),	/* 6F */
    BYTES(0, 0, 0, 0),	/* 70 */
    BYTES(0, 0, 0, 0),	/* 71 */
    BYTES(0, 0, 0, 0),	/* 72 */
    BYTES(0, 0, 0, 0),	/* 73 */
    BYTES(0, 0, 0, 0),	/* 74 */
    BYTES(0, 0, 0, 0),	/* 75 */
    BYTES(0, 0, 0, 0),	/* 76 */
    BYTES(0, 0, 0, 0),	/* 77 */
    BYTES(0, 0, 0, 0),	/* 78 */
    BYTES(0, 0, 0, 0),	/* 79 */
    BYTES(0, 0, 0, 0),	/* 7A */
    BYTES(0, 0, 0, 0),	/* 7B */
    BYTES(0, 0, 0, 0),	/* 7C */
    BYTES(0, 0, 0, 0),	/* 7D */
    BYTES(0, 0, 0, 0),	/* 7E */
    BYTES(0, 0, 0, 0),	/* 7F */
};

#undef SETBITS

#define SETBITS(b0, b1, b2, b3, b4, b5, b6, b7) \
	(b0<<0)|(b1<<1)|(b2<<2)|(b3<<3)|(b4<<4)|(b5<<5)|(b6<<6)|(b7<<7)
	
static UBYTE locapsable[] =
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

static UBYTE hicapsable[] =
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

static UBYTE lorepeatable[] =
{
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 00 - 07 */
    SETBITS(1, 1, 1, 0, 0, 0, 0, 0),	/* 08 - 0F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 10 - 17 */
    SETBITS(1, 1, 0, 0, 0, 0, 0, 0),	/* 18 - 1F */
    
    SETBITS(1, 1, 1, 1, 1, 1, 1, 1),	/* 20 - 27 */
    SETBITS(1, 0, 0, 0, 0, 0, 0, 0),	/* 28 - 2F */
    
    SETBITS(0, 1, 1, 1, 1, 1, 1, 1),	/* 30 - 37 */
    SETBITS(0, 0, 0, 0, 0, 0, 0, 0)	/* 38 - 3F */
};

static UBYTE hirepeatable[] =
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

struct KeyMap def_km =
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
const BYTE keymaptype_table[8][8] =
{
    {3, X, X, X, X, X, X, X},	/* KCF_NOQUAL 			== 0 */
    {3, 2, X, X, X, X, X, X}, 	/* KCF_SHIFT  			== 1 */
    {3, X, X, X, X, X, X, X}, 	/* KCF_ALT    			== 2 */
    {3, 2, 1, 0, X, X, X, X}, 	/* KCF_SHIFT|KCF_ALT 		== 3 */
    {3, X, X, X, 2, X, X, X}, 	/* KCF_CONTROL			== 4 */
    {3, 2, X, X, 1, 0, X, X}, 	/* KCF_SHIFT|KCF_CONTROL	== 5 */
    {3, X, 2, X, 1, X, 0, X}, 	/* KCF_ALT|KCF_CONTROL		== 6 */
    {3, 2, 1, 0, S, X, X, X} 	/* KCF_SHIFT|KCF_ALT|KCF_CONTROL == KC__VANILLA == 7 */

};

#undef S

/* index vertically: (keytype & KC_VANILLA)
** index horizontally: KCF_xxx qualifier combination for keypress.
** Used to get the number of the string descriptor,
** depending on the KCF_SHIFT, KCF_ALT and KCF_CONTROL qualifiers
** of the key pressed
*/
const BYTE keymapstr_table[8][8] =
{
    {0, X, X, X, X, X, X, X},	/* KCF_NOQUAL 			== 0 */
    {0, 1, X, X, X, X, X, X}, 	/* KCF_SHIFT  			== 1 */
    {0, X, 1, X, X, X, X, X}, 	/* KCF_ALT    			== 2 */
    {0, 1, 2, 3, X, X, X, X}, 	/* KCF_SHIFT|KCF_ALT 		== 3 */
    {0, X, X, X, 1, X, X, X}, 	/* KCF_CONTROL			== 4 */
    {0, 1, X, X, 2, 3, X, X}, 	/* KCF_SHIFT|KCF_CONTROL	== 5 */
    {0, X, 1, X, 2, X, 3, X}, 	/* KCF_ALT|KCF_CONTROL		== 6 */
    {0, 1, 2, 3, 4, 5, 6, 7} 	/* KCF_SHIFT|KCF_ALT|KCF_CONTROL == KC__VANILLA == 7 */

};



#undef X
