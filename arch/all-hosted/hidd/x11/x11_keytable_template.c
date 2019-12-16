/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "x11_keytable.h"

/* Use this template to create a keytable for your language:

   Do not touch the right values (rawkey numbers). Only change
   the XK's at the left side. To find out the XK_ names (keysym)
   start "xev" and press the key the comment describes (for
   example "Key left of S" in the xev window. In the Shell
   window you will see output like this:
   
   KeyPress event, serial 30, synthetic NO, window 0x5000001,
    root 0x47, subw 0x5000002, time 3410089115, (24,45), root:(28,69),
    state 0x0, keycode 50 (keysym 0xffe1, Shift_L), same_screen YES,
    XLookupString gives 0 characters:  ""  |
                                           |
   This is the keysym name _______________/
  
   So in this case you would have to write  "XK_Shift_L"

   Check all keys, not just the ones with "XK_????"!!!
*/

#ifndef KEYTABLE_NAME
#define KEYTABLE_NAME template_keytable
#endif

const struct _keytable KEYTABLE_NAME[] =
{    
    {XK_Control_L,	0x63 }, /* left control = control */	
    {XK_Multi_key,	0x63 }, /* right control = control */
    {XK_Super_L,	0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	0x67 },	/* right win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* left Alt = LALT */
    {XK_Mode_switch,	0x65 }, /* right Alt = RALT */
    
    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a,		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_????,		0x15 },
    {XK_????,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_????,		0x31 },
    {XK_????,		0x31 },

    
    /* Key left of 1 */
    {XK_????,		0x00 },
    
    /* Keys right of 0 */
    {XK_????,		0x0B },
    {XK_????,		0x0C },
    
    /* Keys right of P */
    {XK_????,		0x1A },
    {XK_????,		0x1B },
    
    /* Keys right of L */
    {XK_????,		0x29 },
    {XK_????,		0x2A }, 
    {XK_????,		0x2B }, /* Third key right of L might not be present */
    
    /* Key right of shift and 2nd left of X (might not be present) */       
    {XK_less,		0x30 }, 
    
    /* Keys 2nd right of N (= usually right of M) */
    {XK_comma,		0x38 }, 
    {XK_period,		0x39 }, 
    {XK_slash,		0x3A },
        
    {0, -1 }
};
