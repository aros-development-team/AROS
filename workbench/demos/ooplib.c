/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/oop.h>
#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <oop/meta.h>
#include <oop/root.h>
#include <oop/method.h>
#include <oop/interface.h>
#include <oop/server.h>

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>


#define SDEBUG 0
#define DEBUG 1
#include <aros/debug.h>

#define MYSERVERID "demoserver"
#define MYTIMERID "timer"

#define SERVERTASK_STACKSIZE 20000

struct ServerParam
{
    struct Task *Caller;
    ULONG SigBit;
};
struct Task *CreateServerTask(APTR taskparams);


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


// #define GLOBAL_CLASS 

extern ULONG __OOPI_Timer;

/* -------------------------- */

struct Library *OOPBase;

Class *MakeTimerClass();
VOID FreeTimerClass(Class *cl);

ULONG __OOPI_Meta;
ULONG __OOPI_Timer;
ULONG __OOPI_Method;
ULONG __OOPI_Server;
ULONG __OOPI_Interface;

Class *timercl;

struct ServerParam sp;

#define NUM_INVOCATIONS 10000L
#define NUM_IF_INVOCATIONS 10000000L

int main (int argc, char **argv)
{
    SDInit();
    
    
    OOPBase = OpenLibrary(OOPNAME, 0);
    if (OOPBase)
    {
    	if ( 
	       ( __OOPI_Meta   	  = GetID( GUID_Meta		))
	    && ( __OOPI_Timer  	  = GetID( GUID_Timer		)) 
	    && ( __OOPI_Method 	  = GetID( GUID_Method		)) 
	    && ( __OOPI_Server 	  = GetID( GUID_Server		)) 
	    && ( __OOPI_Interface = GetID( GUID_Interface	)) 
	    
	    )
	{
	    
	    
	
	    timercl = MakeTimerClass();
	    printf("Timercl: %p\n", timercl);
	    if (timercl)
	    {
	        /* Create the server task */
		struct Task *servertask;
		Object *timer;
		struct TagItem tags[] = {{TAG_DONE, 0UL}};
		
		timer = NewObjectA(timercl, NULL, tags);
		if (timer)
		{
		    register Interface *iftimer;
		    struct TagItem iftags[] =
		    {
		    	{ A_Interface_TargetObject,	(IPTR)timer},
			{ A_Interface_InterfaceID,	(IPTR)TimerBase},
			{ TAG_DONE, 0UL }
		    };
		    
		    D(bug("Local timer obj created\n"));
		    
		    iftimer = (Interface *)NewObjectA(NULL, INTERFACECLASS, iftags);
		    if (iftimer)
		    {
			ULONG test_mid;
			
			register Msg msg = (Msg)&test_mid;
			    
			ULONG i;
			
			D(bug("iftimer objects created\n"));
			    
			printf("Doing %ld invocations using interface objects\n",
			    		NUM_IF_INVOCATIONS);
			    
			test_mid = M_Timer_Start;
			iftimer->Call(iftimer, (Msg)&test_mid);
			    
			test_mid = M_Timer_TestMethod;
			    
			for (i = 0; i < NUM_IF_INVOCATIONS; i ++)
			{
			    iftimer->Call(iftimer, msg);
			}
			    
			test_mid = M_Timer_Stop;
			iftimer->Call(iftimer, (Msg)&test_mid);
			    
			printf("Time elapsed: ");
			
			test_mid = M_Timer_PrintElapsed;
		    	iftimer->Call(iftimer, (Msg)&test_mid);

			test_mid = M_Timer_TestMethod;
			printf ("Result of testmethod: %ld\n", iftimer->Call(iftimer, (Msg)&test_mid));

		    	
		    	DisposeObject((Object *)iftimer);
		    }
		    
		    DisposeObject(timer);
		}
		
		
		sp.Caller = FindTask(NULL);
		/* This will succeed since no signals have been allocated earlier */
		sp.SigBit = AllocSignal(-1L);
		
		
		
		D(bug("Creating server task\n"));
				
		
		servertask = CreateServerTask(&sp);
		if (servertask)
		{

		    Object *server;
		    
		    D(bug("server task created: %p\n", servertask));
		    
		    Wait(1L << sp.SigBit);
		    D(bug("server task has initialized itself: %p\n", servertask));


		    if ( (server = FindServer(MYSERVERID)) )
		    {
		        Object *timer;
			
			D(bug("Server found: %p\n", server));
			
		        if ( (timer = Server_FindObject(server, MYTIMERID)) )
			{
			    ULONG test_mid;
			    
			    ULONG i;
			    
			    printf("Doing %ld invocations using IPC\n",
			    		NUM_INVOCATIONS);
			    
			    test_mid = M_Timer_Start;
			    DoMethod(timer, (Msg)&test_mid);
			    
			    test_mid = M_Timer_TestMethod;
			    
			    for (i = 0; i < NUM_INVOCATIONS; i ++)
			    {
			    	DoMethod(timer, (Msg)&test_mid);
			    }
			    
			    test_mid = M_Timer_Stop;
			    DoMethod(timer, (Msg)&test_mid);
			    
			    printf("Time elapsed: ");
			
			    test_mid = M_Timer_PrintElapsed;
		    	    DoMethod(timer, (Msg)&test_mid);

			    test_mid = M_Timer_TestMethod;
			    printf ("Result of testmethod: %ld\n", DoMethod(timer, (Msg)&test_mid));

		    
			}
		    
		    }
		    
		}
		    
		FreeTimerClass(timercl);
	    }

        }
	CloseLibrary(OOPBase);
	
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

#ifdef GLOBAL_CLASS
Class *tcl;
#endif

VOID _Timer_Start(
#ifndef GLOBAL_CLASS
Class *tcl,
#endif
Object *o, Msg msg)
{
    struct TimerData *data;
    EnterFunc(bug("Timer::Start(o=%p)\n", o));
    
    data = INST_DATA(tcl, o);
    D(bug("data=%p\n", data));

    gettimeofday(&(data->start_time), NULL);
    
    ReturnVoid("Timer::Start");
}

VOID _Timer_Stop(
#ifndef GLOBAL_CLASS
Class *tcl,
#endif
Object *o, Msg msg)
{
    struct TimerData *data = INST_DATA(tcl, o);
    gettimeofday(&(data->elapsed_time), NULL);
    
    SubTime(&(data->elapsed_time), &(data->start_time));
    
    return;
}

VOID _Timer_PrintElapsed(
#ifndef GLOBAL_CLASS
Class *tcl,
#endif
Object *o, Msg msg)
{
    struct TimerData *data = INST_DATA(tcl, o);
    
    kprintf("%ld secs and %ld micros\n"
    	,data->elapsed_time.tv_sec
    	,data->elapsed_time.tv_usec);
	
}

IPTR _Timer_TestMethod(
#ifndef GLOBAL_CLASS
Class *tcl,
#endif
Object *o, Msg msg)
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
	{(IPTR (*)())_Timer_TestMethod,		MIDX_Timer_TestMethod},
	{NULL, 0UL}
	
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
#ifndef GLOBAL_CLASS
Class *tcl;
#endif

    
    
    tcl = (Class *)NewObjectA(NULL, METACLASS, tags);
    if (tcl)
    {
//    	AddClass(tcl);
    }
    
    return (tcl);
}

VOID FreeTimerClass(Class *cl)
{
    DisposeObject((Object *)cl);
    
    return;

}

VOID TaskEntryPoint(struct ServerParam *p)
{
    Object *server;
    
    BOOL success = FALSE;
    
    
    struct TagItem server_tags[] =
    {
	{ TAG_DONE, 0UL}
    };
    
    
    D(bug("Entering servertask...\n"));
    
    server = NewObjectA(NULL, SERVERCLASS, server_tags);
    if (server)
    {
    	if (AddServer(server, MYSERVERID))
	{

    	    Object *timer;
    
    	    struct TagItem timer_tags[] =
    	    {
		{ TAG_DONE, 0UL}
    	    };
		
	    timer = NewObjectA(timercl, NULL, timer_tags);
	    if (timer)
	    {
		if (Server_AddObject(server, timer, MYTIMERID))
		{
		     Signal(p->Caller, 1L << p->SigBit);
		     
		     Server_Run(server);
		     
		     Server_RemoveObject(server, MYTIMERID);
		     success = TRUE;
		     
		}    
		DisposeObject(timer);
	     
	    }
	    RemoveServer(MYSERVERID);
	}
	DisposeObject(server);
	
	
    }


    /* Just in case */
    if (!success)
    {
    	D(bug("st: No success\n"));
	Signal(p->Caller, 1L << p->SigBit);
    }
    return;

}

struct Task *CreateServerTask(APTR taskparams)
{
    struct Task *task;
    APTR stack;
    
    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (task)
    {
    	NEWLIST(&task->tc_MemEntry);
    	task->tc_Node.ln_Type=NT_TASK;
    	task->tc_Node.ln_Name="demoserver";
    	task->tc_Node.ln_Pri = 0;

    	stack=AllocMem(SERVERTASK_STACKSIZE, MEMF_PUBLIC);
    	if(stack != NULL)
    	{
	    task->tc_SPLower=stack;
	    task->tc_SPUpper=(BYTE *)stack + SERVERTASK_STACKSIZE;

#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET - sizeof(APTR);
	    ((APTR *)task->tc_SPUpper)[-1] = taskparams;
#else
	    task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET + sizeof(APTR);
	    *(APTR *)task->tc_SPLower = taskparams;
#endif

	    if(AddTask(task, TaskEntryPoint, NULL) != NULL)
	    {
	    	/* Everything went OK */
	    	return (task);
	    }	
	    FreeMem(stack, SERVERTASK_STACKSIZE);
    	}
        FreeMem(task,sizeof(struct Task));
    }
    return (NULL);

}

