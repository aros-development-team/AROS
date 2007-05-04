#ifndef TIMERCLASS_H
#define TIMERCLASS_H

/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "oop.h"

#define TIMERCLASS "timerclass"

#define MIDX_Timer_Start 	0
#define MIDX_Timer_Stop  	1
#define MIDX_Timer_PrintElapsed 2
#define MIDX_Timer_TestMethod	3

#ifdef HASHED_METHODS
#   define I_Timer (1)
#   define Timer_Base (I_Timer << NUM_METHOD_BITS)

#   define M_Timer_Start 		(Timer_Base + 0)
#   define M_Timer_Stop  		(Timer_Base + 1)
#   define M_Timer_PrintElapsed   	(Timer_Base + 2)
#   define M_Timer_TestMethod   	(Timer_Base + 3)

#   define METHODID IPTR
#endif

#ifdef HASHED_IFS
#   define I_Timer	(1)
#   define Timer_Base   (I_Timer << NUM_METHOD_BITS)

#   define M_Timer_Start 		(Timer_Base + MIDX_Timer_Start)
#   define M_Timer_Stop  		(Timer_Base + MIDX_Timer_Stop)
#   define M_Timer_PrintElapsed   	(Timer_Base + MIDX_Timer_PrintElapsed)
#   define M_Timer_TestMethod   	(Timer_Base + MIDX_Timer_TestMethod)

#   define METHODID IPTR
#endif

#ifdef HASHED_STRINGS
#   define M_Timer_Start		"Start"
#   define M_Timer_Stop 		"Stop"
#   define M_Timer_PrintElapsed 	"PrintElapsed"
#   define M_Timer_TestMethod		"TestMethod"

#   define METHODID STRPTR
#endif

#define Timer_Start(o)			\
({					\
     METHODID methodid = M_Timer_Start;	\
     DoMethodA(o, (Msg)&methodid);	\
})

#define Timer_Stop(o)			\
({					\
     METHODID methodid = M_Timer_Stop;	\
     DoMethodA(o, (Msg)&methodid);	\
})

#define Timer_PrintElapsed(o)			\
({						\
     METHODID methodid = M_Timer_PrintElapsed;	\
     DoMethodA(o, (Msg)&methodid);		\
})

#define Timer_TestMethod(o)			\
({						\
     METHODID methodid = M_Timer_TestMethod;	\
     DoMethodA(o, (Msg)&methodid);		\
})

Class *MakeTimerClass();
VOID FreeTimerClass(Class *cl);

#endif /* TIMERCLASS_H */
