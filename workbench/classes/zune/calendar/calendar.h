#ifndef ZUNE_CALENDAR_H
#define ZUNE_CALENDAR_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Calendar           "Calendar.mcc"

/*** Attributes *************************************************************/
#define MUIA_Calendar_Date	(TAG_USER | (0xA303 << 16) | 0x0001)
#define MUIA_Calendar_MonthDay 	(TAG_USER | (0xA303 << 16) | 0x0002)
#define MUIA_Calendar_MonthDay0	(TAG_USER | (0xA303 << 16) | 0x0003)
#define MUIA_Calendar_Month 	(TAG_USER | (0xA303 << 16) | 0x0004)
#define MUIA_Calendar_Month0 	(TAG_USER | (0xA303 << 16) | 0x0005)
#define MUIA_Calendar_Year  	(TAG_USER | (0xA303 << 16) | 0x0006)
#define MUIA_Calendar_DayLabels (TAG_USER | (0xA303 << 16) | 0x0007)

/*** Macros *****************************************************************/
#define CalendarObject MUIOBJMACRO_START(MUIC_Calendar)

#endif /* ZUNE_CALENDAR_H */
