/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#if 0
static const __attribute__((section(".text"))) struct _keytable
{
    ULONG  keysym;
    ULONG  hiddcode;
}
keytable[] =
{
    {K_Escape,      0x45 },

    {K_F1,          0x50 },
    {K_F2,          0x51 },
    {K_F3,          0x52 },
    {K_F4,          0x53 },
    
    {K_F5,          0x54 },
    {K_F6,          0x55 },
    {K_F7,          0x56 },
    {K_F8,          0x57 },
    
    {K_F9,          0x58 },
    {K_F10,         0x59 },
    {K_F11,         0x66 },     /* LAMIGA. F11 rawkey code would be 4B */
    {K_F12,         0x67 },     /* RAMIGA. F12 rawkey code would be 6F*/

/*  {K_Sys_Req, ??? 	 }, */
/*  {K_Scroll_Lock, ???? }, */
    {K_Pause,	    0x6e },


    {K_BackQuote,   0x00 },
    {K_1,           0x01 },
    {K_2,           0x02 },
    {K_3,           0x03 },
    {K_4,           0x04 },    
    {K_5,           0x05 },
    {K_6,           0x06 },
    {K_7,           0x07 },
    {K_8,           0x08 },
    {K_9,           0x09 },
    {K_0,           0x0A },
    {K_Minus,       0x0b },
    {K_Equal,       0x0c },                            
    {K_Backspace,   0x41 },

    {K_Insert,	    0x47 },
    {K_Home,        0x70 },
    {K_PgUp,	    0x48 },
    {K_Del,         0x46 },
    {K_End, 	    0x71 },
    {K_PgDn,	    0x49 },
        
    {K_Right,       0x4e },
    {K_Up,          0x4c },
    {K_Left,        0x4f },
    {K_Down,        0x4d },
    
    {K_KP_Numl,     0x5a },
    {K_KP_Divide,   0x5b },
    {K_KP_Multiply, 0x5c },
    {K_KP_Sub,      0x5d },

    {K_KP_7,        0x3d },
    {K_KP_8,        0x3e },
    {K_KP_9,        0x3f },
    {K_KP_Add,      0x5e },

    {K_KP_4,        0x2d },
    {K_KP_5,        0x2e },
    {K_KP_6,        0x2f },

    {K_KP_1,        0x1d },
    {K_KP_2,        0x1e },
    {K_KP_3,        0x1f },
    {K_KP_Enter,    0x43 },

    {K_KP_0,        0x0f },
    {K_KP_Decimal,  0x3c },
        
    {K_Q,           0x10 },
    {K_W,           0x11 },
    {K_E,           0x12 },
    {K_R,           0x13 },
    {K_T,           0x14 },
    {K_Y,           0x15 },
    {K_U,           0x16 },
    {K_I,           0x17 },
    {K_O,           0x18 },
    {K_P,           0x19 },
    {K_LBracket,    0x1a },
    {K_RBracket,    0x1b },
    {K_Enter,       0x44 },

    {K_A,           0x20 },
    {K_S,           0x21 },
    {K_D,           0x22 },
    {K_F,           0x23 },
    {K_G,           0x24 },
    {K_H,           0x25 },
    {K_J,           0x26 },
    {K_K,           0x27 },
    {K_L,           0x28 },
    {K_Semicolon,   0x29 },
    {K_Quote,       0x2a },
    {K_BackSlash,   0x2b },

    {K_LessGreater, 0x30 },
    {K_Z,           0x31 },
    {K_X,           0x32 },
    {K_C,           0x33 },
    {K_V,           0x34 },
    {K_B,           0x35 },
    {K_N,           0x36 },
    {K_M,           0x37 },
    {K_Comma,       0x38 },
    {K_Period,      0x39 },
    {K_Slash,       0x3a },
        
    {K_Space,       0x40 },

    {K_LShift,      0x60 },
    {K_RShift,      0x61 },

    {K_LAlt,        0x64 },
    {K_RAlt,        0x65 },

    {K_LCtrl,       0x63 },
    {K_RCtrl,       0x63 },

    {K_LMeta,       0x66 },	/* Left Win key = LAmi */
    {K_RMeta,       0x67 },	/* Right Win key = RAmi */
/*  [K_Menu,	    ???? }, */
    {K_Tab,         0x42 },

    {K_CapsLock,    0x62 },

    {K_ResetRequest,0x78 },
    {0, -1 }
};

#endif
