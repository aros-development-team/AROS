/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <proto/oop.h>
#include <proto/exec.h>
#include <exec/libraries.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <oop/meta.h>
#include <oop/root.h>
#include <oop/method.h>

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

/* --------------------- */
/* Defines below would typically go into the includefile for the class */
#define TIMERCLASS "timerclass"

#define GUID_Timer "Timer"
#define MIDX_Timer_Start 	0
#define MIDX_Timer_Stop  	1
#define MIDX_Timer_PrintElapsed 2
#define MIDX_Timer_TestMethod	3

#define TimerBase	(__OOPI_Timer)
#define M_Timer_Start		(TimerBase + MIDX_Timer_Start)
#define M_Timer_Stop		(TimerBase + MIDX_Timer_Stop)
#define M_Timer_PrintElapsed	(TimerBase + MIDX_Timer_PrintElapsed)
#define M_Timer_TestMethod	(TimerBase + MIDX_Timer_TestMethod)



extern ULONG __OOPI_Timer;

/* -------------------------- */

struct Library *OOPBase;

Class *MakeTimerClass();
VOID FreeTimerClass(Class *cl);

ULONG __OOPI_Meta;
ULONG __OOPI_Timer;
ULONG __OOPI_Method;

int main (int argc, char **argv)
{
    SDInit();
    
    
    OOPBase = OpenLibrary(OOPNAME, 0);
    if (OOPBase)
    {
    	if ( 
	       ( __OOPI_Meta   = GetID(GUID_Meta	))
	    && ( __OOPI_Timer  = GetID(GUID_Timer	)) 
	    && ( __OOPI_Method = GetID(GUID_Method	)) 
	    
	    )
	{
            Class *timercl;
	    
	    Object *timer;
	    
	    printf("__OOPI_Meta=%ld\n", __OOPI_Meta);
	
	    timercl = MakeTimerClass();
	    printf("Timercl: %p\n", timercl);
	    if (timercl)
	    {
	    	struct TagItem timer_tags[] =
	    	{
	    	    { TAG_DONE, 0UL}
	    	};
		
		timer = NewObjectA(timercl, NULL, timer_tags);
		if (timer)
		{
		    ULONG test_mid = M_Timer_TestMethod;
		    
		    struct TagItem mobj_tags[] = 
		    {
		    	{A_Method_TargetObject,	(IPTR)timer },
			{A_Method_Message,	(IPTR)&test_mid},
			{A_Method_MethodID,	M_Timer_TestMethod},
			{TAG_DONE,	0UL}
			
		    };
		    
		    Method *test_m;
		    printf("Doing test method\n");
		    
		    printf ("Result: %ld\n", DoMethodA(timer, (Msg)&test_mid));
		    
		    test_m = (Method *)NewObjectA(NULL, METHODCLASS, mobj_tags);
		    printf("test method obj: %p\n", test_m);
		    if (test_m)
		    {
		        printf ("TestMethod result: %ld\n", CallMethod(test_m));
			
			DisposeObject((Object *)test_m);

		    }
		    
		    DisposeObject(timer);
		}
		FreeTimerClass(timercl);
	    }
        }
	
//	DebugOOP();
    }
    return (0);
}



struct TimerData
{
    struct timeval start_time;
    struct timeval elapsed_time;
};

VOID SubTime(struct timeval *dest, struct timeval *src)
{
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
    struct TimerData *data;
    EnterFunc(bug("Timer::Start(cl=%p, o=%p)\n", cl, o));
    D(bug("data=%p\n", data));
    
    data = INST_DATA(cl, o);
    D(bug("data=%p\n", data));

    gettimeofday(&(data->start_time), NULL);
    
    ReturnVoid("Timer::Start");
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
    
    printf("%ld secs and %ld micros\n"
    	,data->elapsed_time.tv_sec
    	,data->elapsed_time.tv_usec);
	
}

IPTR _Timer_TestMethod(Class *cl, Msg msg)
{
    return (12345678);
}


Class *MakeTimerClass()
{

    struct MethodDescr methods[] =
    {
	{(IPTR (*)())_Timer_Start,		MIDX_Timer_Start},
	{(IPTR (*)())_Timer_Stop,		MIDX_Timer_Stop},
	{(IPTR (*)())_Timer_PrintElapsed,	MIDX_Timer_PrintElapsed},
	{(IPTR (*)())_Timer_TestMethod,		MIDX_Timer_TestMethod}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{ methods, "Timer", 4},
	{ NULL, 0UL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {A_Class_SuperID,		(IPTR)ROOTCLASS},
	{A_Class_InterfaceDescr,	(IPTR)ifdescr},
	{A_Class_ID,			(IPTR)TIMERCLASS},
	{A_Class_InstSize,		(IPTR)sizeof (struct TimerData)},
	{TAG_DONE, 0UL}
    };

    
    Class *cl;
    
    cl = (Class *)NewObjectA(NULL, METACLASS, tags);
    if (cl)
    {
//    	AddClass(cl);
    }
    
    return (cl);
}

VOID FreeTimerClass(Class *cl)
{
    DisposeObject((Object *)cl);
    
    return;

}

