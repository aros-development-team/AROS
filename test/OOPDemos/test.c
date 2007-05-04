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
#include "hash.h"
#include <stdio.h>

#define SDEBUG 1
#define DEBUG 1
#include "debug.h"

extern struct OOPBase *oopbase;
SDInit;


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
    	    ForeachNode(&(oopbase->ClassList), n)
    	    {
    	    	printf("%s\n", n->ln_Name);
    	    }
    	    printf("\n\n");
	    
#if (HASHED_IFS || HASHED_METHODS)
	    { ULONG i;
	    printf("Hash table:\n");
	    for (i = 0; i < HashSize(timercl->HashTable); i ++)
	    {
	    	struct Bucket *b;
		printf ("%ld:", i);
		for (b = (struct Bucket *)timercl->HashTable[i]; b; b = b->Next)
		    printf ("  %ld %p", b->ID, b);
		    
		printf("\n");
	    }
	    }
#endif
	    
#if (HASHED_STRINGS)
	    { ULONG i;
	    printf("Hash table:\n");
	    for (i = 0; i < HashSize(timercl->HashTable); i ++)
	    {
	    	struct Bucket *b;
		printf ("%ld:", i);
		for (b = (struct Bucket *)timercl->HashTable[i]; b; b = b->Next)
		    printf ("  %s %p", (STRPTR)b->ID, b);
		    
		printf("\n");
	    }
	    }
#endif

	    
    	    /* Create a new instance */
    	    timer = NewObject(NULL, TIMERCLASS, NULL);
    	    if (timer)
    	    {
		ULONG i;
		METHODID methodid = M_Timer_TestMethod;
		
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

