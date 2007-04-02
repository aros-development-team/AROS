#ifndef DEVICES_INPUTEVENT_H
#define DEVICES_INPUTEVENT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Input events
    Lang: english
*/

#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif
#ifndef DEVICES_TIMER_H
#   include <devices/timer.h>
#endif

struct InputEvent
{
    struct InputEvent * ie_NextEvent;

    UBYTE ie_Class;     /* see below for definitions */
    UBYTE ie_SubClass;  /* see below for definitions */
    UWORD ie_Code;      /* see below for definitions */
    UWORD ie_Qualifier; /* see below for definitions */

    union
    {
        struct
        {
            WORD ie_x;
            WORD ie_y;
        } ie_xy;

        APTR ie_addr;

        struct
        {
            UBYTE ie_prev1DownCode;
            UBYTE ie_prev1DownQual;
            UBYTE ie_prev2DownCode;
            UBYTE ie_prev2DownQual;
        } ie_dead;
    } ie_position;

    struct timeval      ie_TimeStamp;
};
#define ie_X             ie_position.ie_xy.ie_x
#define ie_Y             ie_position.ie_xy.ie_y
#define ie_EventAddress  ie_position.ie_addr
#define ie_Prev1DownCode ie_position.ie_dead.ie_prev1DownCode
#define ie_Prev1DownQual ie_position.ie_dead.ie_prev1DownQual
#define ie_Prev2DownCode ie_position.ie_dead.ie_prev2DownCode
#define ie_Prev2DownQual ie_position.ie_dead.ie_prev2DownQual

/* InputEvent Classes */
#define IECLASS_NULL           0
#define IECLASS_RAWKEY         1
#define IECLASS_RAWMOUSE       2
#define IECLASS_EVENT          3
#define IECLASS_POINTERPOS     4
#define IECLASS_TIMER          6
#define IECLASS_GADGETDOWN     7
#define IECLASS_GADGETUP       8
#define IECLASS_REQUESTER      9
#define IECLASS_MENULIST       10
#define IECLASS_CLOSEWINDOW    11
#define IECLASS_SIZEWINDOW     12
#define IECLASS_REFRESHWINDOW  13
#define IECLASS_NEWPREFS       14
#define IECLASS_DISKREMOVED    15
#define IECLASS_DISKINSERTED   16
#define IECLASS_ACTIVEWINDOW   17
#define IECLASS_INACTIVEWINDOW 18
#define IECLASS_NEWPOINTERPOS  19 /* (IEPointerPixel *) */
#define IECLASS_MENUHELP       20
#define IECLASS_CHANGEWINDOW   21

/* NewMouse standard */
#define IECLASS_NEWMOUSE       22

#define IECLASS_MAX            22

/* InputEvent SubClasses */
#define IESUBCLASS_COMPATIBLE 0
#define IESUBCLASS_PIXEL      1 /* (IEPointerPixel *) */
#define IESUBCLASS_TABLET     2
#define IESUBCLASS_NEWTABLET  3

/* InputEvent Codes */
/* Used by IECLASS_RAWKEY */
#define IECODE_UP_PREFIX       0x80
#define IECODE_KEY_CODE_FIRST  0x00
#define IECODE_KEY_CODE_LAST   0x77
#define IECODE_COMM_CODE_FIRST 0x78
#define IECODE_COMM_CODE_LAST  0x7F
/* Used by IECLASS_ANSI */
#define IECODE_CO_FIRST        0x00
#define IECODE_CO_LAST         0x1F
#define IECODE_ASCII_FIRST     0x20
#define IECODE_ASCII_LAST      0x7E
#define IECODE_ASCII_DEL       0x7F
#define IECODE_C1_FIRST        0x80
#define IECODE_C1_LAST         0x9F
#define IECODE_LATIN1_FIRST    0xA0
#define IECODE_LATIN1_LAST     0xFF
/* Used by IECODE_RAWMOUSE */
#define IECODE_LBUTTON         0x68
#define IECODE_RBUTTON         0x69
#define IECODE_MBUTTON         0x6A
#define IECODE_NOBUTTON        0xFF
/* Used by IECLASS_REQUESTER */
#define IECODE_REQCLEAR        0x00
#define IECODE_REQSET          0x01
/* Used by IECLASS_EVENT */
#define IECODE_NEWACTIVE       0x01
#define IECODE_NEWSIZE         0x02
#define IECODE_REFRESH         0x03

/* InputEvent Qualifiers */
#define IEQUALIFIERB_LSHIFT             0
#define IEQUALIFIER_LSHIFT          (1<<0)
#define IEQUALIFIERB_RSHIFT             1
#define IEQUALIFIER_RSHIFT          (1<<1)
#define IEQUALIFIERB_CAPSLOCK           2
#define IEQUALIFIER_CAPSLOCK        (1<<2)
#define IEQUALIFIERB_CONTROL            3
#define IEQUALIFIER_CONTROL         (1<<3)
#define IEQUALIFIERB_LALT               4
#define IEQUALIFIER_LALT            (1<<4)
#define IEQUALIFIERB_RALT               5
#define IEQUALIFIER_RALT            (1<<5)
#define IEQUALIFIERB_LCOMMAND           6
#define IEQUALIFIER_LCOMMAND        (1<<6)
#define IEQUALIFIERB_RCOMMAND           7
#define IEQUALIFIER_RCOMMAND        (1<<7)
#define IEQUALIFIERB_NUMERICPAD         8
#define IEQUALIFIER_NUMERICPAD      (1<<8)
#define IEQUALIFIERB_REPEAT             9
#define IEQUALIFIER_REPEAT          (1<<9)
#define IEQUALIFIERB_INTERRUPT          10
#define IEQUALIFIER_INTERRUPT       (1<<10)
#define IEQUALIFIERB_MULTIBROADCAST     11
#define IEQUALIFIER_MULTIBROADCAST  (1<<11)
#define IEQUALIFIERB_MIDBUTTON          12
#define IEQUALIFIER_MIDBUTTON       (1<<12)
#define IEQUALIFIERB_RBUTTON            13
#define IEQUALIFIER_RBUTTON         (1<<13)
#define IEQUALIFIERB_LEFTBUTTON         14
#define IEQUALIFIER_LEFTBUTTON      (1<<14)
#define IEQUALIFIERB_RELATIVEMOUSE      15
#define IEQUALIFIER_RELATIVEMOUSE   (1<<15)

/* Pointed to by IECLASS_NEWPOINTERPOS and IESUBCLASS_PIXEL */
struct IEPointerPixel
{
    struct Screen * iepp_Screen;
    struct
    {
        WORD X;
        WORD Y;
    } iepp_Position;
};

/* Used for IECLASS_NEWPOINTERPOS and IESUBCLASS_TABLET
   Pointed to by ie_EventAddress */
struct IEPointerTablet
{
    struct
    {
        UWORD X;
        UWORD Y;
    } iept_Range;
    struct
    {
        UWORD X;
        UWORD Y;
    } iept_Value;
    WORD iept_Prssure;
};

/* Used for IECLASS_NEWPOINTERPOS with IESUBCLASS_NEWTABLET
   Pointed to by ie_EventAddress */
struct IENewTablet
{
    struct Hook * ient_CallBack;

    UWORD ient_ScaledX;
    UWORD ient_ScaledY;
    UWORD ient_ScaledXFraction;
    UWORD ient_ScaledYFraction;
    ULONG ient_TabletX;
    ULONG ient_TabletY;
    ULONG ient_RangeX;
    ULONG ient_RangeY;

    struct TagItem * ient_TagList;
};

#endif /* DEVICES_INPUTEVENT_H */
