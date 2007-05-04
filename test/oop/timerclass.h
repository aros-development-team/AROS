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

#define Timer_Base (1 << NUM_METHOD_BITS)

#define M_Timer_Start 		(Timer_Base + 0)
#define M_Timer_Stop  		(Timer_Base + 1)
#define M_Timer_PrintElapsed   	(Timer_Base + 2)
#define M_Timer_TestMethod   	(Timer_Base + 3)

#define NUM_TIMER_METHODS 4

#define Timer_Start(o)			\
({					\
     ULONG methodid = M_Timer_Start;	\
     DoMethodA(o, (Msg)&methodid);	\
})

#define Timer_Stop(o)			\
({					\
     ULONG methodid = M_Timer_Stop;	\
     DoMethodA(o, (Msg)&methodid);	\
})

#define Timer_PrintElapsed(o)			\
({						\
     ULONG methodid = M_Timer_PrintElapsed;	\
     DoMethodA(o, (Msg)&methodid);		\
})

#define Timer_TestMethod(o)			\
({						\
     ULONG methodid = M_Timer_TestMethod;	\
     DoMethodA(o, (Msg)&methodid);		\
})

Class *MakeTimerClass();
VOID FreeTimerClass(Class *cl);

#endif /* TIMERCLASS_H */
