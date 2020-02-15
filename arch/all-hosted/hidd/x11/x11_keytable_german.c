/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "x11_keytable.h"

#ifndef KEYTABLE_NAME
#define KEYTABLE_NAME german_keytable
#endif

const struct _keytable KEYTABLE_NAME[] =
{
    {XK_Control_L,	0x00, 0x63 }, /* linke STRG = control */	
    {XK_Multi_key,	0x00, 0x63 }, /* rechte STRG = control */
    {XK_Super_L,	0x00, 0x66 },	/* Linke Win = LAMIGA */
    {XK_Super_R,	0x00, 0x67 },	/* Rechte Win = RAMIGA */
    {XK_Meta_L,		0x00, 0x64 }, /* Linke Alt = LALT */
    {XK_Mode_switch,	0x00, 0x65 }, /* Alt Gr = RALT */

    /* Key left of S */
    {XK_A,		0x00, 0x20 },
    {XK_a,		0x00, 0x20 },

    /* Key right of N */
    {XK_M,		0x00, 0x37 },
    {XK_m,		0x00, 0x37 },

    /* Key right of TAB */
    {XK_Q,		0x00, 0x10 },
    {XK_q,		0x00, 0x10 },

    /* Key between T and U */
    {XK_Z,		0x00, 0x15 },
    {XK_z,		0x00, 0x15 },

    /* Key left of E */
    {XK_W,		0x00, 0x11 },
    {XK_w,		0x00, 0x11 },

    /* Key left of X */
    {XK_y,		0x00, 0x31 },
    {XK_Y,		0x00, 0x31 },

    /* Key left of 1 */
    {XK_asciicircum,	0x00, 0x00 }, /* Akzent links neben 1 Taste */

    /* Keys right of 0 */
    {XK_equal,		0x00, 0x0A }, /* = */
    {XK_ssharp,		0x00, 0x0B }, /* scharfes s */
    {XK_acute,		0x00, 0x0C }, /* Akzent rechts von scharfem s */

    /* Keys right of P */
    {XK_udiaeresis,	0x00, 0x1A }, /* Umlaut u */
    {XK_Udiaeresis,	0x00, 0x1A },
    {XK_plus,		0x00, 0x1B }, /* + */

    /* Keys right of L */    
    {XK_odiaeresis,	0x00, 0x29 }, /* Umlaut o */
    {XK_Odiaeresis,	0x00, 0x29 },
    {XK_adiaeresis,	0x00, 0x2A }, /* Umlaut a */
    {XK_Adiaeresis,	0x00, 0x2A },
    {XK_numbersign,	0x00, 0x2B }, /* # */

    /* Key right of shift and 2nd left of X (might not be present) */        
    {XK_less,		0x00, 0x30 }, /* < */

    /* Keys 2nd right of N (= usually right of M) */       
    {XK_comma,		0x00, 0x38 }, /* Komma */
    {XK_period,		0x00, 0x39 }, /* Punkt */
    {XK_minus,		0x00, 0x3A }, /* Minus */

    {0, 0, -1 }
};
