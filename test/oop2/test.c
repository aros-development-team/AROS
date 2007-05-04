/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "oop.h"
#include "timerclass.h"
#include "protos.h"
#include "support.h"
#include <stdio.h>

extern struct List ClassList;


#define NUM_ITERATIONS (10000000)

IPTR TestFunc(Class *cl, Object *o, Msg msg)
{
     return (12345678);
}


int main(int argc, char **argv)
{
   
    if (InitOOP())
    {
	Class *timercl;
	
    	printf("Object system initialized\n");
    	
    	
    	/* Initialize the timer class */
    	timercl = MakeTimerClass();
	if (timercl)
    	{
	
	    struct Node *n;
	    Object *timer;
	    
    	    printf("Class list:\n");
    	    ForeachNode(&ClassList, n)
    	    {
    	    	printf("%s\n", n->ln_Name);
    	    }
    	    printf("\n\n");
	    
	    printf("Hash table:\n");
	    
    	    /* Create a new instance */
    	    timer = NewObject(NULL, TIMERCLASS, NULL);
    	    if (timer)
    	    {
		ULONG i;
		ULONG methodid = M_Timer_TestMethod;
		
    	    	printf("Timer object: %p\n", timer);
    	
		printf ("Doing ten billion calls to test method...\n");
		
		
		/*  Normal ivocation test */
		printf ("Using normal invocation\n");

		Timer_Start(timer);
    	    	
    	    	for (i = 0; i < NUM_ITERATIONS; i ++)
		{
		    Timer_TestMethod(timer);
		}
		    
		Timer_Stop(timer);
		printf("Time elapsed: ");
		Timer_PrintElapsed(timer);


		/*  Function test */		
		printf("\nTen billion calls to empty *function*\n");
		Timer_Start(timer);
    	    	
    	    	for (i = 0; i < NUM_ITERATIONS; i ++)
		{
		    TestFunc(timercl, timer, (Msg)&methodid);
		}
		
		    
		Timer_Stop(timer);
		printf("Time elapsed: ");
		Timer_PrintElapsed(timer);
		
		

		/*  Loop test */
		printf("\nLooping ten billion times\n");
		Timer_Start(timer);
    	    	
    	    	for (i = 0; i < NUM_ITERATIONS; i ++)
		{
		    ULONG retval;
		    retval = 12345678;
		}
		    
		Timer_Stop(timer);
		printf("Time elapsed: ");
		Timer_PrintElapsed(timer);
		
		printf("\n\nTestMethod output: %ld\n", Timer_TestMethod(timer));
		
		/* Dispose object */
		DisposeObject(timer);
		
    	    }
    	    FreeTimerClass(timercl);
    	}
    	CleanupOOP();
    }
    
    return (0);
} 

