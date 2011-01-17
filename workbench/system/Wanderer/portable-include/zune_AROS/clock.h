#ifndef ZUNE_CLOCK_H
#define ZUNE_CLOCK_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Clock                      "Clock.mcc"

/*** Identifier base ********************************************************/
#define MUIB_Clock                      (MUIB_AROS | 0x00000000)

/*** Attributes *************************************************************/
#define MUIA_Clock_Hour 	        (MUIB_Clock | 0x00000000) /* -SG  UWORD              */
#define MUIA_Clock_Min 	    	        (MUIB_Clock | 0x00000001) /* -SG  UWORD              */
#define MUIA_Clock_Sec 	    	        (MUIB_Clock | 0x00000002) /* -SG  UWORD              */
#define MUIA_Clock_Time	    	        (MUIB_Clock | 0x00000003) /* ISG  struct ClockData * */
#define MUIA_Clock_Ticked   	        (MUIB_Clock | 0x00000004) /* ---  BOOL               */
#define MUIA_Clock_Frozen  	        (MUIB_Clock | 0x00000005) /* ISG  BOOL               */
#define MUIA_Clock_EditHand	        (MUIB_Clock | 0x00000006) /* ISG  WORD               */

#define MUIV_Clock_EditHand_Hour        (0)
#define MUIV_Clock_EditHand_Minute      (1)
#define MUIV_Clock_EditHand_Second      (2)

/*** Methods ****************************************************************/
#define MUIM_Clock_Timer    	        (MUIB_Clock | 0x00000000)

/*** Macros *****************************************************************/
#define ClockObject MUIOBJMACRO_START(MUIC_Clock)

#endif /* ZUNE_CLOCK_H */
