#ifndef KEYS_H
#define KEYS_H

/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PC keymap.
    Lang: English.
*/

#define	K_Escape	0x0001
#define K_F1		0x003b
#define K_F2		0x003c
#define K_F3		0x003d
#define K_F4		0x003e
#define K_F5		0x003f
#define K_F6		0x0040
#define K_F7		0x0041
#define K_F8		0x0042
#define K_F9		0x0043
#define K_F10		0x0044
#define K_F11		0x0057
#define K_F12		0x0058
#define K_Sys_Req	0x4037	/* PrintScreen key */
#define K_Scroll_Lock	0x4046  /* It's not a special key but treat it as such */
#define K_Pause		0x1f01 	/* Pause/Break key */
#define K_BackQuote	0x0029
#define K_0		0x000b
#define K_1		0x0002
#define K_2		0x0003
#define K_3		0x0004
#define K_4		0x0005
#define K_5		0x0006
#define K_6		0x0007
#define K_7		0x0008
#define K_8		0x0009
#define K_9		0x000a
#define K_Minus		0x000c
#define K_Equal		0x000d
#define K_Backspace	0x000e
#define K_Tab		0x000f
#define K_Q		0x0010
#define K_W		0x0011
#define K_E		0x0012
#define K_R		0x0013
#define K_T		0x0014
#define K_Y		0x0015
#define K_U		0x0016
#define K_I		0x0017
#define K_O		0x0018
#define K_P		0x0019
#define K_LBracket	0x001a
#define K_RBracket	0x001b
#define K_A		0x001e
#define K_S		0x001f
#define K_D		0x0020
#define K_F		0x0021
#define K_G		0x0022
#define K_H		0x0023
#define K_J		0x0024
#define K_K		0x0025
#define K_L		0x0026
#define K_Semicolon	0x0027
#define K_Quote		0x0028
#define K_BackSlash	0x002b /* small key left of lower part of RETURN */

#define K_LessGreater   0x0056 /* stegerg: added. right of small(!) lshift */
#define K_Z		0x002c
#define K_X		0x002d
#define K_C		0x002e
#define K_V		0x002f
#define K_B		0x0030
#define K_N		0x0031
#define K_M		0x0032
#define K_Comma		0x0033
#define K_Period	0x0034
#define K_Slash		0x0035

#define K_Enter		0x001c

#define K_CapsLock	0x003a

#define K_LShift	0x002a
#define K_RShift	0x0036

#define K_LCtrl		0x001d
#define K_RCtrl		0x401d

#define K_LMeta		0x405b	/* Subst for LAmiga key */
#define K_RMeta		0x405c	/* Subst for RAmiga key */

#define K_LAlt		0x0038
#define K_RAlt		0x4038

#define K_Space		0x0039

#define K_Menu		0x405d

#define K_Insert	0x4052
#define K_Home		0x4047
#define K_PgUp		0x4049
#define K_Del		0x4053
#define K_End		0x404f
#define K_PgDn		0x4051

#define K_Up		0x4048
#define K_Down		0x4050
#define K_Left		0x404b
#define K_Right		0x404d

/* Numeric keypad */
#define K_KP_Numl	0x0045
#define K_KP_Divide	0x4035
#define K_KP_Multiply	0x0037
#define K_KP_Sub	0x004a

#define K_KP_Add	0x004e
#define K_KP_Enter	0x401c

#define K_KP_7		0x0047
#define K_KP_8		0x0048
#define K_KP_9		0x0049

#define K_KP_4		0x004b
#define K_KP_5		0x004c
#define K_KP_6		0x004d

#define K_KP_1		0x004f
#define K_KP_2		0x0050
#define K_KP_3		0x0051

#define K_KP_0		0x0052
#define K_KP_Decimal	0x0053
/* Reset Key! */
#define K_ResetRequest	0x7f7f

#endif /* KEYS_H */
