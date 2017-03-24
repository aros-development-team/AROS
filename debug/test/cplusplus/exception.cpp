/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * We use printf() here instead of C++ traditional cout because linking in cout
 * increases executable size up to 3 megabytes (!!!), making it difficult to
 * disassemble it. Anyway our aim is to test exceptions.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef __AROS__

#include <exec/alerts.h>
#include <proto/exec.h>

extern void *__eh_frame_start;

/*
 * Non-working call frame unwinding in AROS causes calling abort().
 * Here we manually override this function to call Alert(),
 * which can give us a stack trace.
 */
void abort(void)
{
    printf("abort() called\n");

    Alert(AN_Unknown);

    /* Calling exit here makes this a noreturn function
     * avoiding a compiler warning. */
    exit(20);
}

#endif

/* Just to make things a little bit more complex. */
int sub()
{
    printf("sub() entered\n");

    throw 20;
}

int main ()
{
#ifdef __AROS__
    printf("Exception frames start at %p\n", &__eh_frame_start);
#endif

    try
    {
	sub();
    }
    catch (int e)
    {
	printf("An exception occurred. Exception Nr. %d\n", e);
    }

    printf("Exiting\n");
    return 0;
}
