/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIA_Calendar_Date	(TAG_USER | (0xA303 << 16) | 0x0001)
#define MUIA_Calendar_MonthDay 	(TAG_USER | (0xA303 << 16) | 0x0002)
#define MUIA_Calendar_MonthDay0	(TAG_USER | (0xA303 << 16) | 0x0003)
#define MUIA_Calendar_Month 	(TAG_USER | (0xA303 << 16) | 0x0004)
#define MUIA_Calendar_Month0 	(TAG_USER | (0xA303 << 16) | 0x0005)
#define MUIA_Calendar_Year  	(TAG_USER | (0xA303 << 16) | 0x0006)
#define MUIA_Calendar_DayLabels (TAG_USER | (0xA303 << 16) | 0x0007)

/*********************************************************************************************/

BOOL MakeCalendarClass(void);
void KillCalendarClass(void);

/*********************************************************************************************/
