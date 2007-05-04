/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include "oop.h"
#include "protos.h"
#include "timerclass.h"

struct TimerData
{
    struct timeval start_time;
    struct timeval elapsed_time;
};

/* Does dest - src => dest */
VOID SubTime(struct timeval *dest, struct timeval *src)
{
    /* Normalize the terms */
    while(src->tv_usec > 999999)
    {
	src->tv_sec++;
	src->tv_usec -= 1000000;
    }
    while(dest->tv_usec > 999999)
    {
	dest->tv_sec++;
	dest->tv_usec -= 1000000;
    }

    dest->tv_usec -= src->tv_usec;
    dest->tv_sec -= src->tv_sec;

    if(dest->tv_usec < 0)
    {
	dest->tv_usec += 1000000;
	dest->tv_sec--;
    }
    
    return;
}

VOID _Timer_Start(Class *cl, Object *o, Msg msg)
{
    struct TimerData *data = INST_DATA(cl, o);
    gettimeofday(&(data->start_time), NULL);
    
    return;
}

VOID _Timer_Stop(Class *cl, Object *o, Msg msg)
{
    struct TimerData *data = INST_DATA(cl, o);
    gettimeofday(&(data->elapsed_time), NULL);
    
    SubTime(&(data->elapsed_time), &(data->start_time));
    
    return;
}

VOID _Timer_PrintElapsed(Class *cl, Object *o, Msg msg)
{
    struct TimerData *data = INST_DATA(cl, o);
    
    printf("%d secs and %d micros\n"
    	,data->elapsed_time.tv_sec
    	,data->elapsed_time.tv_usec);
	
}

IPTR _Timer_TestMethod(Class *cl, Msg msg)
{
    return (12345678);
}


Class *MakeTimerClass()
{
    struct MethodDescr timerdescr[] =
    {
    	{_Timer_Start, 		M_Timer_Start},
	{_Timer_Stop,  		M_Timer_Stop},
	{_Timer_PrintElapsed,	M_Timer_PrintElapsed},
	{_Timer_TestMethod,	M_Timer_TestMethod},
	{NULL, 0UL}
    };
    
    Class *cl;
    
    cl = MakeClass(TIMERCLASS, ROOTCLASS, timerdescr, sizeof (struct TimerData), NUM_TIMER_METHODS);
    if (cl)
    {
    	AddClass(cl);
    }
    
    return (cl);
}

VOID FreeTimerClass(Class *cl)
{
    RemoveClass(cl);
    FreeClass(cl);
}
