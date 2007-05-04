/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"
#include "oop.h"
#include "hash.h"

#include "sysdep/sysdep.h"

#define SDEBUG 0
#define DEBUG 0
#include "debug.h"


#ifdef DIRECT_LOOKUP

#   define CallMethod(cl, o, msg) 					\
    {									\
    	return ( cl->MTable[msg->MethodID].MethodFunc(cl, o, msg)); 	\
    }
    
#endif /* DIRECT_LOOKUP */

/*********************** HASHED_METHODS ***************************/
#ifdef HASHED_METHODS


/* Using for () below, take 5,915 s.

#   define CallMethod(cl, o, msg) 					\
    {				    					\
    	register struct MethodBucket *b;				\
    	register ULONG mid = msg->MethodID;				\
									\
    	for (b = cl->HashTable[mid & cl->HashMask]; 		\
		b; b = b->Next)						\
    	{								\
       	    if (b->MethodID == mid)					\
	    {								\
	    	return (b->MethodFunc(b->mClass, o, msg));		\
	    }								\
    	}								\
    	return (NULL);							\
    }

*/

/* Using while() takes 5,915 s (Same as above)

#   define CallMethod(cl, o, msg) 					\
    {				    					\
    	register struct MethodBucket *b;				\
    	register ULONG mid = msg->MethodID;				\
									\
    	b = cl->HashTable[mid & cl->HashMask]; 				\
									\
	while (b)							\
    	{								\
       	    if (b->MethodID == mid)					\
	    {								\
	    	return (b->MethodFunc(b->mClass, o, msg));		\
	    }								\
	    b = b->Next;						\
	    								\
    	}								\
    	return (NULL);							\
    }
*/

/* Using if - goto: 5,78 s.
#   define CallMethod(cl, o, msg) 					\
    {				    					\
    	register struct MethodBucket *b;				\
    	register ULONG mid = msg->MethodID;				\
									\
    	b = cl->HashTable[mid & cl->HashMask]; 				\
loop:									\
	if (b)								\
    	{								\
       	    if (b->MethodID == mid)					\
	    {								\
	    	return (b->MethodFunc(b->mClass, o, msg));		\
	    }								\
	    b = b->Next;						\
	    goto loop;							\
    	}								\
    	return (NULL);							\
    }

*/

/* Surprisingly, when avoiding to put methodid into
** a separate var, it comes down to 5,109 s.
** even if MethodID is used (at least) two times
** pr. invocation.
*/
#   define CallMethod(cl, o, msg) 					\
    {				    					\
    	register struct MethodBucket *b;				\
									\
    	b = cl->HashTable[((msg->MethodID >> NUM_METHOD_BITS) ^ msg->MethodID) & cl->HashMask]; 		\
loop:									\
	if (b)								\
    	{								\
       	    if (b->MethodID == msg->MethodID)				\
	    {								\
	    	return (b->MethodFunc(b->mClass, o, msg));		\
	    }								\
	    b = b->Next;						\
	    goto loop;							\
    	}								\
    	return (NULL);							\
    }


#endif /* HASHED_METHODS */


/*********************** HASHED_IFS ***************************/
#ifdef HASHED_IFS


#   define CallMethod(cl, o, msg) 					\
    {				    					\
    	register struct InterfaceBucket *b;				\
    	register ULONG mid = msg->MethodID;				\
    	register ULONG ifid = mid >> NUM_METHOD_BITS;			\
    	register struct IFMethod *method;				\
    									\
    	mid &= METHOD_MASK;						\
									\
	b = cl->HashTable[ifid & cl->HashMask];				\
loop:   if (b) 								\
   	{								\
       	    if (b->InterfaceID == ifid)					\
	    {								\
	    	method = &(b->MethodTable[mid]);			\
	    	return (method->MethodFunc(method->mClass, o, msg));	\
	    }    							\
            b = b->Next;						\
	    goto loop;							\
    	}								\
    	return (NULL);							\
     }
#endif /* HASHED_IFS */

/*********************** HASHED_STRINGS ***************************/
#ifdef HASHED_STRINGS
#   define CallMethod(cl, o, msg)				\
    {								\
	register struct MethodBucket *b;	ULONG val;	\
	register STRPTR str1 = (STRPTR)msg->MethodID;		\
	register LONG i;					\
	register STRPTR str2;					\
	for (i = 0, val = 0; (i < MAX_HASH_CHARS) && *str1; str1 ++)		\
	    { val += *str1; i ++; }				\
	for (b = cl->HashTable[val & cl->HashMask];		\
		b; b = b->Next)					\
	{							\
	    str2 = (STRPTR)b->MethodID;				\
	    str1 = (STRPTR)msg->MethodID;			\
	    while ( (!(i = *str1 - *str2)) && *str1)		\
	    	{ str1 ++; str2 ++; }				\
	    if (!i)						\
	    	return (b->MethodFunc(b->mClass, o, msg));	\
	}							\
	return (NULL);						\
    }
    
#endif /* HASHED_STRINGS */
IPTR CoerceMethodA(Class *cl, Object *o, Msg msg)
{
    CallMethod(cl, o, msg);
}

IPTR DoMethodA(Object *o, Msg msg)
{
    register Class *cl = OCLASS(o);

    CallMethod(cl, o, msg);
}

IPTR DoSuperMethodA(Class *cl, Object *o, Msg msg)
{
    Class *super = cl->SuperClass;
    CallMethod(super, o, msg);
}
