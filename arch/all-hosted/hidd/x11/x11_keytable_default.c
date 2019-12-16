/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "x11_keytable.h"

#ifndef KEYTABLE_NAME
#define KEYTABLE_NAME keytable
#endif

const struct _keytable KEYTABLE_NAME[] =
{
    {XK_Return, 	0x44 },
    {XK_Right,		0x4e },
    {XK_Up,		0x4c },
    {XK_Left,		0x4f },
    {XK_Down,		0x4d },
    {XK_Help,		0x5f },

    {XK_BackSpace,	0x41 },
    {XK_Delete,		0x46 },
    {XK_space,		0x40 },
    {XK_Shift_L,	0x60 },
    {XK_Shift_R,	0x61 },
    {XK_Caps_Lock,	0x62 },
    {XK_Alt_L,		0x64 },
    {XK_Alt_R,		0x65 },
    {XK_Escape,		0x45 },
    {XK_Tab,		0x42 },

    {XK_F1,		0x50 },
    {XK_F2,		0x51 },
    {XK_F3,		0x52 },
    {XK_F4,		0x53 },
    {XK_F5,		0x54 },
    {XK_F6,		0x55 },
    {XK_F7,		0x56 },
    {XK_F8,		0x57 },
    {XK_F9,		0x58 },
    {XK_F10,		0x59 },

    {XK_F11,		0x4B },	
    {XK_F12,		0x5f },	/* HELP, F12 would be 0x6F */
    {XK_Home,		0x70 },	
    {XK_End,		0x71 },
    {XK_Insert,		0x47 },
    {XK_Prior,		0x48 }, /* PageUP */
    {XK_Next,		0x49 }, /* PageDown */
    {XK_Print,		0x6c },
    {XK_Scroll_Lock,	0x6b },
    {XK_Pause,		0x6e },
    
    {XK_KP_Enter,	0x43 },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Decimal,	0x3c },
    {XK_KP_Separator,	0x3c },
    {XK_KP_Delete,	0x3c },
    {XK_KP_Add,		0x5e },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Multiply,	0x5d },
    {XK_KP_Divide,	0x5c },

    {XK_KP_0,		0x0f },
    {XK_KP_Insert,	0x0f },    
    {XK_KP_1,		0x1d },
    {XK_KP_End,		0x1d },   
    {XK_KP_2,		0x1e },
    {XK_KP_Down,	0x1e },
    {XK_KP_3,		0x1f },
    {XK_KP_Page_Down,	0x1f },
    {XK_KP_4,		0x2d },
    {XK_KP_Left,	0x2d },
    {XK_KP_5,		0x2e },
    {XK_KP_Begin,	0x2e },
    {XK_KP_6,		0x2f },
    {XK_KP_Right,	0x2f },
    {XK_KP_7,		0x3d },
    {XK_KP_Home,	0x3d },
    {XK_KP_8,		0x3e },
    {XK_KP_Up,		0x3e },
    {XK_KP_9,		0x3f },
    {XK_KP_Page_Up,	0x3f },
       
    {XK_E,		0x12 },
    {XK_e,		0x12 },
    {XK_R,		0x13 },
    {XK_r,		0x13 },
    {XK_T,		0x14 },
    {XK_t,		0x14 },
    {XK_U,		0x16 },
    {XK_u,		0x16 },
    {XK_I,		0x17 },
    {XK_i,		0x17 },
    {XK_O,		0x18 },
    {XK_o,		0x18 },
    {XK_P,		0x19 },
    {XK_p,		0x19 },

    {XK_S,		0x21 },
    {XK_s,		0x21 },
    {XK_D,		0x22 },
    {XK_d,		0x22 },
    {XK_F,		0x23 },
    {XK_f,		0x23 },
    {XK_G,		0x24 },
    {XK_g,		0x24 },
    {XK_H,		0x25 },
    {XK_h,		0x25 },
    {XK_J,		0x26 },
    {XK_j,		0x26 },
    {XK_K,		0x27 },
    {XK_k,		0x27 },
    {XK_L,		0x28 },
    {XK_l,		0x28 },

    {XK_X,		0x32 },
    {XK_x,		0x32 },
    {XK_c,		0x33 },
    {XK_C,		0x33 },
    {XK_V,		0x34 },
    {XK_v,		0x34 },
    {XK_B,		0x35 },
    {XK_b,		0x35 },
    {XK_N,		0x36 },    
    {XK_n,		0x36 },
    
    {XK_1,		0x01 },
    {XK_2,		0x02 },
    {XK_3,		0x03 },
    {XK_4,		0x04 },    
    {XK_5,		0x05 },
    {XK_6,		0x06 },
    {XK_7,		0x07 },
    {XK_8,		0x08 },
    {XK_9,		0x09 },
    {XK_0,		0x0A },
    {0, - 1 }
};
