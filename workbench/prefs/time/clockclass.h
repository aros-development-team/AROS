/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIA_Clock_Hour 	(TAG_USER | (0xA303 << 16) | 0x0011)
#define MUIA_Clock_Min 	    	(TAG_USER | (0xA303 << 16) | 0x0012)
#define MUIA_Clock_Sec 	    	(TAG_USER | (0xA303 << 16) | 0x0013)
#define MUIA_Clock_Time	    	(TAG_USER | (0xA303 << 16) | 0x0014)
#define MUIA_Clock_Ticked   	(TAG_USER | (0xA303 << 16) | 0x0015)
#define MUIA_Clock_Frozen  	(TAG_USER | (0xA303 << 16) | 0x0016)
#define MUIA_Clock_EditHand	(TAG_USER | (0xA303 << 16) | 0x0017)

#define EDITHAND_HOUR 0
#define EDITHAND_MIN  1
#define EDITHAND_SEC  2

#define MUIM_Clock_Timer    	0x785A09

/*********************************************************************************************/

BOOL MakeClockClass(void);
void KillClockClass(void);

/*********************************************************************************************/
