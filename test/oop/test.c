/*
   (C) 1997-98 AROS - The Amiga Replacement OS
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
    	
    	    /* Create a new instance */
    	    timer = NewObject(NULL, TIMERCLASS, NULL);
    	    if (timer)
    	    {
		ULONG i;
		
		Method *test_m;
		
    	    	printf("Timer object: %p\n", timer);
    	
		printf ("Doing ten billion calls to test method...\n");
		
		printf ("Using normal invocation\n");

		Timer_Start(timer);
    	    	
    	    	for (i = 0; i < NUM_ITERATIONS; i ++)
		{
		    Timer_TestMethod(timer);
		}
		    
		Timer_Stop(timer);
		printf("Time elapsed: ");
		Timer_PrintElapsed(timer);


		printf ("\nUsing fast invocation\n");
		
		test_m = GetMethod(timer, M_Timer_TestMethod);

		Timer_Start(timer);
    	    	
    	    	for (i = 0; i < NUM_ITERATIONS; i ++)
		{
		    ULONG methodid = M_Timer_TestMethod;
		   
		    CallMethodFast(timer, test_m, &methodid);
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

