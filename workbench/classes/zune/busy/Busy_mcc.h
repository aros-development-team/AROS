#ifndef BUSY_MCC_H
#define BUSY_MCC_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id: clock.h 17862 2003-06-04 09:21:00Z chodorowski $
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Busy                     	"Busy.mcc"

/*** Identifier base ********************************************************/
#define MUIB_Busy                       0x80020000

/*** Attributes *************************************************************/
#define MUIA_Busy_ShowHideIH	    	(MUIB_Busy | 0xa9)
#define MUIA_Busy_Speed     	    	(MUIB_Busy | 0x49)

#define MUIV_Busy_Speed_Off 	    	0
#define MUIV_Busy_Speed_User	    	-1

/*** Methods ****************************************************************/
#define MUIM_Busy_Move    	        (MUIB_Busy | 0x01)

struct MUIP_Busy_Move
{
    ULONG MethodID;
};

/*** Macros *****************************************************************/
#define BusyObject MUIOBJMACRO_START(MUIC_Busy)
#define BusyBar    BusyObject, MUIA_Busy_Speed, MUIV_Busy_Speed_User, End

#endif /* BUSY_MCC_H */
