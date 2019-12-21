/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "x11_keytable.h"

#ifndef KEYTABLE_NAME
#define KEYTABLE_NAME englishgb_keytable
#endif

const struct _keytable KEYTABLE_NAME[] =
{    
    {XK_Control_L,	        0x00, 0x63 }, /* left control = control */	
    {XK_Multi_key,	        0x00, 0x63 }, /* right control = control */
    {XK_Super_L,	        0x00, 0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	        0x00, 0x67 },	/* right win = RAMIGA */
    {XK_Menu,		        0x00, 0x67 }, /* menu key = RAMIGA */
    {XK_Meta_L,		        0x00, 0x64 }, /* left Alt = LALT */
    {XK_Mode_switch,	        0x00, 0x65 }, /* right Alt = RALT */

    /* Key left of S */
    {XK_A,		        0x00, 0x20 },
    {XK_a,		        0x00, 0x20 },

    /* Key right of N */
    {XK_M,		        0x00, 0x37 },
    {XK_m,		        0x00, 0x37 },

    /* Key right of TAB */
    {XK_Q,		        0x00, 0x10 },
    {XK_q,		        0x00, 0x10 },

    /* Key between T and U */
    {XK_y,		        0x00, 0x15 },
    {XK_Y,		        0x00, 0x15 },

    /* Key left of E */
    {XK_W,		        0x00, 0x11 },
    {XK_w,		        0x00, 0x11 },

    /* Key left of X */
    {XK_z,		        0x00, 0x31 },
    {XK_Z,		        0x00, 0x31 },

    /* Key left of 1 */
    {XK_grave,		        0x00, 0x00 },

    /* Keys right of 0 */
    {XK_minus,		        0x00, 0x0B },
    {XK_equal,		        0x00, 0x0C },

    /* Keys right of P */
    {XK_bracketleft,	        0x00, 0x1A },
    {XK_bracketright,	        0x00, 0x1B },

    /* Keys right of L */
    {XK_semicolon,	        0x00, 0x29 },
    {XK_apostrophe,	        0x00, 0x2A }, 
    {XK_numbersign,	        0x60, 0x03 }, 

    /* Key right of shift and 2nd left of X (might not be present) */       
    {XK_backslash,              0x00, 0x0D }, 

    /* Keys 2nd right of N (= usually right of M) */    
    {XK_comma,		        0x00, 0x38 }, 
    {XK_period,		        0x00, 0x39 }, 
    {XK_slash,		        0x00, 0x3A },

    {0, 0, -1 }
};
