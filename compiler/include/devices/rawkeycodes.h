#ifndef DEVICES_RAWKEYCODES_H
#define DEVICES_RAWKEYCODES_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Standard Amiga (in case of AROS: virtual) rawkey codes
    Lang: english
*/

#define RAWKEY_ESCAPE  	    0x45

#define RAWKEY_F1   	    0x50
#define RAWKEY_F2   	    0x51
#define RAWKEY_F3   	    0x52
#define RAWKEY_F4   	    0x53
#define RAWKEY_F5   	    0x54
#define RAWKEY_F6   	    0x55
#define RAWKEY_F7   	    0x56
#define RAWKEY_F8   	    0x57
#define RAWKEY_F9   	    0x58
#define RAWKEY_F10  	    0x59
#define RAWKEY_F11  	    0x4B
#define RAWKEY_F12  	    0x6F

#define RAWKEY_PAUSE	    0x6E

#define RAWKEY_TILDE	    0x00
#define RAWKEY_1    	    0x01
#define RAWKEY_2    	    0x02
#define RAWKEY_3    	    0x03
#define RAWKEY_4    	    0x04
#define RAWKEY_5    	    0x05
#define RAWKEY_6    	    0x06
#define RAWKEY_7    	    0x07
#define RAWKEY_8    	    0x08
#define RAWKEY_9    	    0x09
#define RAWKEY_0    	    0x0A
#define RAWKEY_MINUS	    0x0B
#define RAWKEY_EQUAL	    0x0C
#define RAWKEY_BACKSLASH    0x0D
#define RAWKEY_BACKSPACE    0x41

#define RAWKEY_TAB  	    0x42
#define RAWKEY_Q    	    0x10
#define RAWKEY_W    	    0x11
#define RAWKEY_E    	    0x12
#define RAWKEY_R    	    0x13
#define RAWKEY_T    	    0x14
#define RAWKEY_Y    	    0x15
#define RAWKEY_U    	    0x16
#define RAWKEY_I    	    0x17
#define RAWKEY_O    	    0x18
#define RAWKEY_P    	    0x19
#define RAWKEY_LBRACKET     0x1A
#define RAWKEY_RBRACKET     0x1B
#define RAWKEY_RETURN	    0x44
#define RAWKEY_CAPSLOCK     0x62
#define RAWKEY_A    	    0x20
#define RAWKEY_S    	    0x21
#define RAWKEY_D    	    0x22
#define RAWKEY_F    	    0x23
#define RAWKEY_G    	    0x24
#define RAWKEY_H    	    0x25
#define RAWKEY_J    	    0x26
#define RAWKEY_K    	    0x27
#define RAWKEY_L    	    0x28
#define RAWKEY_SEMICOLON    0x29
#define RAWKEY_QUOTE	    0x2A
#define RAWKEY_2B   	    0x2B

#define RAWKEY_LSHIFT	    0x60
#define RAWKEY_LESSGREATER  0x30
#define RAWKEY_Z    	    0x31
#define RAWKEY_X    	    0x32
#define RAWKEY_C    	    0x33
#define RAWKEY_V    	    0x34
#define RAWKEY_B    	    0x35
#define RAWKEY_N    	    0x36
#define RAWKEY_M    	    0x37
#define RAWKEY_COMMA	    0x38
#define RAWKEY_PERIOD	    0x39
#define RAWKEY_SLASH	    0x3A
#define RAWKEY_RSHIFT	    0x61

#define RAWKEY_CONTROL	    0x63
#define RAWKEY_LCONTROL     0x63
#define RAWKEY_LAMIGA	    0x66
#define RAWKEY_LALT 	    0x64
#define RAWKEY_SPACE	    0x40
#define RAWKEY_RALT 	    0x65
#define RAWKEY_RAMIGA	    0x67

#define RAWKEY_INSERT	    0x47
#define RAWKEY_DELETE	    0x46
#define RAWKEY_HOME 	    0x70
#define RAWKEY_END  	    0x71
#define RAWKEY_PAGEUP	    0x48
#define RAWKEY_PAGEDOWN     0x49
#define RAWKEY_HELP 	    0x5F

#define RAWKEY_UP   	    0x4C
#define RAWKEY_LEFT 	    0x4F
#define RAWKEY_DOWN 	    0x4D
#define RAWKEY_RIGHT	    0x4E

#define RAWKEY_KP_7 	    0x3D
#define RAWKEY_KP_8 	    0x3E
#define RAWKEY_KP_9 	    0x3F
#define RAWKEY_KP_4 	    0x2D
#define RAWKEY_KP_5 	    0x2E
#define RAWKEY_KP_6 	    0x2F
#define RAWKEY_KP_1 	    0x1D
#define RAWKEY_KP_2 	    0x1E
#define RAWKEY_KP_3 	    0x1F
#define RAWKEY_KP_0 	    0x0F
#define RAWKEY_KP_DECIMAL   0x3C
#define RAWKEY_KP_PLUS	    0x5E
#define RAWKEY_KP_MINUS     0x4A
#define RAWKEY_KP_ENTER     0x43

/* Extra PC and multimedia keys (MorphOS-compatible) */
#define RAWKEY_KP_DIVIDE    0x5C
#define RAWKEY_KP_MULTIPLY  0x5D
#define RAWKEY_SCRLOCK      0x6B
#define RAWKEY_PRTSCREEN    0x6C
#define RAWKEY_NUMLOCK      0x6D

#define RAWKEY_MEDIA1       0x72
#define RAWKEY_MEDIA2       0x73
#define RAWKEY_MEDIA3       0x74
#define RAWKEY_MEDIA4       0x75
#define RAWKEY_MEDIA5       0x76
#define RAWKEY_MEDIA6       0x77
#define RAWKEY_CDTV_STOP    RAWKEY_MEDIA1
#define RAWKEY_CDTV_PLAY    RAWKEY_MEDIA2
#define RAWKEY_CDTV_PREV    RAWKEY_MEDIA3
#define RAWKEY_CDTV_NEXT    RAWKEY_MEDIA4
#define RAWKEY_CDTV_REW     RAWKEY_MEDIA5
#define RAWKEY_CDTV_FF      RAWKEY_MEDIA6

/*
   NewMouse standard:

   Copyright (c) 1999 by Alessandro Zummo <azummo@ita.flashnet.it> .
   All Rights Reserved
*/

#define RAWKEY_NM_WHEEL_UP      0x7A
#define RAWKEY_NM_WHEEL_DOWN    0x7B
#define RAWKEY_NM_WHEEL_LEFT    0x7C
#define RAWKEY_NM_WHEEL_RIGHT   0x7D
#define RAWKEY_NM_BUTTON_FOURTH 0x7E

#endif /* DEVICES_RAWKEYCODES_H */
