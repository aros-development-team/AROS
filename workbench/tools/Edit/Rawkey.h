/*** Rawkey.h : Some standard rawkey definitions. Why these ****
**** fucking codes aren't in system includes?! RKM source.  ***/

#ifndef	RAWKEY_H
#define	RAWKEY_H

#ifdef _AROS
#include <devices/rawkeycodes.h>
#endif

/* Standard qualifiers */
#define	ALTKEYS			(ALTLEFT | ALTRIGHT)
#define	SHIFTKEYS		(IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)
#define	CTRLKEYS			(IEQUALIFIER_CONTROL)

/* Raw keys that are the same on all keyboards */
#define UP_KEY      0x4C
#define DOWN_KEY    0x4D
#define RIGHT_KEY   0x4E
#define LEFT_KEY    0x4F

#define F1_KEY      0x50
#define F2_KEY      0x51
#define F3_KEY      0x52
#define F4_KEY      0x53
#define F5_KEY      0x54
#define F6_KEY      0x55
#define F7_KEY      0x56
#define F8_KEY      0x57
#define F9_KEY      0x58
#define F10_KEY     0x59

#define N0_KEY      0x0F
#define N1_KEY      0x1D
#define N2_KEY      0x1E
#define N3_KEY      0x1F
#define N4_KEY      0x2D
#define N5_KEY      0x2E
#define N6_KEY      0x2F
#define N7_KEY      0x3D
#define N8_KEY      0x3E
#define N9_KEY      0x3F

#define NPERIOD_KEY 0x3C
#define NOPAREN_KEY 0x5A
#define NCPAREN_KEY 0x5B
#define NSLASH_KEY  0x5C
#define NASTER_KEY  0x5D
#define NMINUS_KEY  0x4A
#define NPLUS_KEY   0x5E
#define NENTER_KEY  0x43

#define SPACE_KEY   0x40
#define BS_KEY      0x41
#define TAB_KEY     0x42
#define RETURN_KEY  0x44
#define ESC_KEY     0x45
#define DEL_KEY     0x46
#define HELP_KEY    0x5F

/* Keys over cursor keys on PC keyboards */

#define HOME_KEY    0x70
#define END_KEY     0x71
#define PGUP_KEY    0x48
#define PGDOWN_KEY  0x49

#endif
