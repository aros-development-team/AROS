/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/


/* Prevent inclusion of <intuition/classes.h>,
 * which is referenced in the amiga inline macros
 * imported below by <proto/exec.h>.
 */
#define INTUITION_CLASSES_H

#include <proto/oop.h>
#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <oop/server.h>

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>


#define SDEBUG 0
#define DEBUG 0
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
#define CLID_Timer "timerclass"

#define IID_Timer "Timer"
#define moTimer_Start 	0
#define moTimer_Stop  	1
#define moTimer_PrintElapsed 2
#define moTimer_TestMethod	3

#define TimerBase	(__ITimer)
#define M_Timer_Start		(TimerBase + moTimer_Start)
#define M_Timer_Stop		(TimerBase + moTimer_Stop)
#define M_Timer_PrintElapsed	(TimerBase + moTimer_PrintElapsed)
#define M_Timer_TestMethod	(TimerBase + moTimer_TestMethod)


// #define GLOBAL_CLASS

extern ULONG __OOPI_Timer;

/* -------------------------- */

struct Library *OOPBase;

OOP_Class *MakeTimerClass();
VOID FreeTimerClass(OOP_Class *cl);

ULONG __IMeta;
ULONG __ITimer;
ULONG __IMethod;
ULONG __IServer;
ULONG __IInterface;

OOP_Class *timercl;

struct ServerParam sp;

#define NUM_INVOCATIONS 10000L
#define NUM_IF_INVOCATIONS 10000000L

int main (int argc, char **argv)
{
    SDInit();



    OOPBase = OpenLibrary(AROSOOP_NAME, 0);
    if (OOPBase)
    {
	D(bug("Got OOPBase\n"));
    	if ( 
	       ( __IMeta   	  = OOP_GetAttrBase( IID_Meta	))
	    && ( __ITimer  	  = OOP_GetAttrBase( IID_Timer	)) 
	    && ( __IMethod 	  = OOP_GetAttrBase( IID_Method	)) 
	    && ( __IServer 	  = OOP_GetAttrBase( IID_Server	)) 
	    && ( __IInterface 	  = OOP_GetAttrBase( IID_Interface	)) 
	    
	    )
	{

	    D(bug("Got IDs\n"));

	    timercl = MakeTimerClass();
	    printf("Timercl: %p\n", timercl);
	    if (timercl)
	    {
	        /* Create the server task */
		struct Task *servertask;
		OOP_Object *timer;
		struct TagItem tags[] = {{TAG_DONE, 0UL}};
		
		D(bug("Creating new timer object\n"));

		timer = OOP_NewObject(timercl, NULL, tags);
		D(bug("timer object created\n"));

		if (timer)
		{
		    OOP_Method *m;
		    ULONG test_mid = OOP_GetMethodID(IID_Timer, moTimer_TestMethod);
		    struct TagItem iftags[] =
		    {
		    	{ aInterface_TargetObject,	(IPTR)timer},
			{ aInterface_InterfaceID,	(IPTR)IID_Timer},
			{ TAG_DONE, 0UL }
		    };
		    struct TagItem mtags[] =
		    {
		    	{aMethod_TargetObject, (IPTR)timer		},
			{aMethod_Message,	(IPTR)&test_mid	},
			{aMethod_MethodID,	test_mid	},
			{TAG_DONE,}
		    };
		    
		    register OOP_Interface *iftimer;
		    m = (OOP_Method *)OOP_NewObject(NULL, CLID_Method, mtags);
		    if (m)
		    {
		        printf("Method object created, output: %ld\n",
				OOP_CallMethod(m));
				
			OOP_DisposeObject((OOP_Object *)m);
		    }
		    
		    

		    D(bug("Local timer obj created\n"));
		    
		    iftimer = (OOP_Interface *)OOP_NewObject(NULL, CLID_Interface, iftags);
		    if (iftimer)
		    {
			ULONG test_mid;

			register OOP_Msg msg = (OOP_Msg)&test_mid;

			ULONG i;

			D(bug("iftimer object created\n"));

			printf("Doing %ld invocations using interface objects\n",
			    		NUM_IF_INVOCATIONS);

			test_mid = M_Timer_Start;
			iftimer->callMethod(iftimer, (OOP_Msg)&test_mid);

			test_mid = M_Timer_TestMethod;

			for (i = 0; i < NUM_IF_INVOCATIONS; i ++)
			{
			    iftimer->callMethod(iftimer, msg);
			}

			test_mid = M_Timer_Stop;
			iftimer->callMethod(iftimer, (OOP_Msg)&test_mid);

			printf("Time elapsed: ");

			test_mid = M_Timer_PrintElapsed;
		    	iftimer->callMethod(iftimer, (OOP_Msg)&test_mid);

			test_mid = M_Timer_TestMethod;
			printf ("Result of testmethod: %ld\n", iftimer->callMethod(iftimer, (OOP_Msg)&test_mid));


		    	OOP_DisposeObject((OOP_Object *)iftimer);
		    }

		    OOP_DisposeObject(timer);
		}
		
		
		sp.Caller = FindTask(NULL);

		sp.SigBit = AllocSignal(-1L);



		D(bug("Creating server task\n"));


		servertask = CreateServerTask(&sp);
		if (servertask)
		{

		    OOP_Object *server;

		    D(bug("server task created: %p\n", servertask));

		    Wait(1L << sp.SigBit);
		    D(bug("server task has initialized itself: %p\n", servertask));


		    if ( (server = OOP_FindServer(MYSERVERID)) )
		    {
		        OOP_Object *timer;

			D(bug("Server found: %p\n", server));

		        if ( (timer = Server_FindObject(server, MYTIMERID)) )
			{
			    ULONG test_mid;

			    ULONG i;

			    printf("Doing %ld invocations using IPC\n",
			    		NUM_INVOCATIONS);

			    test_mid = OOP_GetMethodID(IID_Timer, moTimer_Start);
			    OOP_DoMethod(timer, (OOP_Msg)&test_mid);

			    test_mid = OOP_GetMethodID(IID_Timer, moTimer_TestMethod);
			    for (i = 0; i < NUM_INVOCATIONS; i ++)
			    {
			    	OOP_DoMethod(timer, (OOP_Msg)&test_mid);
			    }

			    test_mid = OOP_GetMethodID(IID_Timer, moTimer_Stop);
			    OOP_DoMethod(timer, (OOP_Msg)&test_mid);

			    printf("Time elapsed: ");

			    test_mid = OOP_GetMethodID(IID_Timer, moTimer_PrintElapsed);
		    	    OOP_DoMethod(timer, (OOP_Msg)&test_mid);

			    test_mid = OOP_GetMethodID(IID_Timer, moTimer_TestMethod);
			    printf ("Result of testmethod: %ld\n", OOP_DoMethod(timer, (OOP_Msg)&test_mid));


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
OOP_Class *tcl,
#endif
OOP_Object *o, OOP_Msg msg)
{
    struct TimerData *data;
    EnterFunc(bug("Timer::Start(o=%p)\n", o));

    data = OOP_INST_DATA(tcl, o);
    D(bug("data=%p\n", data));

    gettimeofday(&(data->start_time), NULL);

    ReturnVoid("Timer::Start");
}

VOID _Timer_Stop(
#ifndef GLOBAL_CLASS
OOP_Class *tcl,
#endif
OOP_Object *o, OOP_Msg msg)
{
    struct TimerData *data = OOP_INST_DATA(tcl, o);
    gettimeofday(&(data->elapsed_time), NULL);

    SubTime(&(data->elapsed_time), &(data->start_time));

    return;
}

VOID _Timer_PrintElapsed(
#ifndef GLOBAL_CLASS
OOP_Class *tcl,
#endif
OOP_Object *o, OOP_Msg msg)
{
    struct TimerData *data = OOP_INST_DATA(tcl, o);

    kprintf("%ld secs and %ld micros\n"
    	,data->elapsed_time.tv_sec
    	,data->elapsed_time.tv_usec);

}

IPTR _Timer_TestMethod(
#ifndef GLOBAL_CLASS
OOP_Class *tcl,
#endif
OOP_Object *o, OOP_Msg msg)
{
    return (12345678);
}


OOP_Class *MakeTimerClass()
{

    struct OOP_MethodDescr methods[] =
    {
	{(IPTR (*)())_Timer_Start,		moTimer_Start},
	{(IPTR (*)())_Timer_Stop,		moTimer_Stop},
	{(IPTR (*)())_Timer_PrintElapsed,	moTimer_PrintElapsed},
	{(IPTR (*)())_Timer_TestMethod,		moTimer_TestMethod},
	{NULL, 0UL}

    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
    	{ methods, "Timer", 4},
	{ NULL, 0UL, 0UL}
    };

    struct TagItem tags[] =
    {
        {aMeta_SuperID,		(IPTR)CLID_Root},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_ID,			(IPTR)CLID_Timer},
	{aMeta_InstSize,		(IPTR)sizeof (struct TimerData)},
	{TAG_DONE, 0UL}
    };
#ifndef GLOBAL_CLASS
OOP_Class *tcl;
#endif

    
    tcl = (OOP_Class *)OOP_NewObject(NULL, CLID_MIMeta, tags);


    if (tcl)
    {
//    	OOP_AddClass(tcl);
    }

    return (tcl);
}

VOID FreeTimerClass(OOP_Class *cl)
{
    OOP_DisposeObject((OOP_Object *)cl);

    return;

}

VOID TaskEntryPoint(struct ServerParam *p)
{
    OOP_Object *server;

    BOOL success = FALSE;


    struct TagItem server_tags[] =
    {
	{ TAG_DONE, 0UL}
    };


    D(bug("Entering servertask...\n"));
    
    server = OOP_NewObject(NULL, CLID_Server, server_tags);
    if (server)
    {
    	D(bug("st: server created\n"));
    	if (OOP_AddServer(server, MYSERVERID))
	{

    	    OOP_Object *timer;

    	    struct TagItem timer_tags[] =
    	    {
		{ TAG_DONE, 0UL}
    	    };
    	    D(bug("st: server added\n"));

	    timer = OOP_NewObject(timercl, NULL, timer_tags);
	    if (timer)
	    {
    	    	D(bug("st: timer created\n"));
		if (Server_AddObject(server, timer, MYTIMERID))
		{
		     D(bug("st: timer added to server\n"));

		     Signal(p->Caller, 1L << p->SigBit);

		     Server_Run(server);

		     Server_RemoveObject(server, MYTIMERID);
		     success = TRUE;

		}
		OOP_DisposeObject(timer);

	    }
	    OOP_RemoveServer(MYSERVERID);
	}
	OOP_DisposeObject(server);


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

