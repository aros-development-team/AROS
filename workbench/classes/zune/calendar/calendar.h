#ifndef ZUNE_CALENDAR_H
#define ZUNE_CALENDAR_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Calendar           "Calendar.mcc"

/*** Identifier base ********************************************************/
#define MUIB_Calendar           (MUIB_AROS | 0x00000100)

/*** Attributes *************************************************************/
#define MUIA_Calendar_Date	(MUIB_Calendar | 0x00000000) /* ISG  struct ClockData * */
#define MUIA_Calendar_MonthDay 	(MUIB_Calendar | 0x00000001) /* -SG  UWORD              */
#define MUIA_Calendar_MonthDay0	(MUIB_Calendar | 0x00000002) /* -SG  UWORD              */
#define MUIA_Calendar_Month 	(MUIB_Calendar | 0x00000003) /* -SG  UWORD              */
#define MUIA_Calendar_Month0 	(MUIB_Calendar | 0x00000004) /* -SG  UWORD              */
#define MUIA_Calendar_Year  	(MUIB_Calendar | 0x00000005) /* -SG  UWORD              */
#define MUIA_Calendar_DayLabels (MUIB_Calendar | 0x00000006) /* I--  STRPTR [12]        */

/*** Macros *****************************************************************/
#define CalendarObject MUIOBJMACRO_START(MUIC_Calendar)

#endif /* ZUNE_CALENDAR_H */
