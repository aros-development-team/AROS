/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for AROS abstract keymap
    Lang: English
*/

#ifndef ABSTRACTKEYCODES_H
#define ABSTRACTKEYCODES_H

/* Various definitions */

#define  KEYUPMASK         0x80
#define  AMIGAKEYMASK      0x7f
#define  NOTAMIGAKEYMASK   0xff00

/* We only care about the qualifier for now keys as the other ones
   are taken care of by keymap.library anyway. */

#define  AKC_QUALIFIERS_FIRST  0x60
#define  AKC_QUALIFIERS_LAST   0x67
#define  AKC_SHIFT_LEFT        0x60
#define  AKC_SHIFT_RIGHT       0x61
#define  AKC_CAPS_LOCK         0x62
#define  AKC_CONTROL           0x63
#define  AKC_ALT_LEFT          0x64
#define  AKC_ALT_RIGHT         0x65
#define  AKC_COMMAND_LEFT      0x66
#define  AKC_COMMAND_RIGHT     0x67


/* Keys on the numeric pad (this table may be increased later) */

#define  AKC_NUM_0      0x0f
#define  AKC_NUM_1      0x1d
#define  AKC_NUM_2      0x1e
#define  AKC_NUM_3      0x1f
#define  AKC_NUM_4      0x2d
#define  AKC_NUM_5      0x2e
#define  AKC_NUM_6      0x2f
#define  AKC_NUM_7      0x3d
#define  AKC_NUM_8      0x3e
#define  AKC_NUM_9      0x3f
#define  AKC_NUM_POINT  0x3c  
#define  AKC_NUM_ENTER  0x43
#define  AKC_NUM_DASH   0x4a
#define  AKC_NUM_LPAREN 0x5a
#define  AKC_NUM_RPAREN 0x5b
#define  AKC_NUM_SLASH  0x5c
#define  AKC_NUM_PLUS   0x5d
#define  AKC_NUM_TIMES  0x5e


#endif /* ABSTRACTKEYCODES_H */
