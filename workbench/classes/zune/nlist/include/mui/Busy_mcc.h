/*

		MCC_Busy (c) by kMel, Klaus Melchior

		Registered class of the Magic User Interface.

		Busy_mcc.h

*/


/*** Include stuff ***/

#ifndef BUSY_MCC_H
#define BUSY_MCC_H

#ifndef LIBRARIES_MUI_H
#include "libraries/mui.h"
#endif


/*** MUI Defines ***/

#define MUIC_Busy "Busy.mcc"
#define BusyObject MUI_NewObject(MUIC_Busy

#define BusyBar\
	BusyObject,\
		MUIA_Busy_Speed, MUIV_Busy_Speed_User,\
		End



/*** Methods ***/

#define MUIM_Busy_Move           0x80020001UL

/*** Method structs ***/

struct MUIP_Busy_Move {
	ULONG MethodID;
};


/*** Special method values ***/


/*** Special method flags ***/



/*** Attributes ***/

#define MUIA_Busy_ShowHideIH     0x800200a9UL
#define MUIA_Busy_Speed          0x80020049UL

/*** Special attribute values ***/

#define MUIV_Busy_Speed_Off              0
#define MUIV_Busy_Speed_User            -1



/*** Structures, Flags & Values ***/





#endif /* BUSY_MCC_H */

/* PrMake.rexx 0.10 (16.2.1996) Copyright 1995 kmel, Klaus Melchior */

