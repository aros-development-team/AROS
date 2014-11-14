/*
    Copyright © 2007-2014, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void *thread_func1( void *vptr_args );
void *thread_func2( void *vptr_args );

volatile int m;

int main ( void )
{
    int j;

    pthread_t thread1, thread2;

    pthread_create( &thread1, NULL, &thread_func1, NULL );
    printf("thread 1 created.. thread: %d\n", thread1);

    pthread_create( &thread2, NULL, &thread_func2, NULL );
    printf("thread 2 created.. thread: %d\n", thread2);


    for( j= 0; j < 20; ++j )
    {
        printf("a\n");

        for( m= 99999999; m; --m );  /* use some CPU time */

        printf("trying to cancel thread 1\n");
        pthread_cancel( thread1 );
    }


    printf("join thread 2\n");
    pthread_join( thread2, NULL );

    printf("finished!\n");

    return 0;
}

volatile int f1, f2;

void *thread_func1( void *vptr_args )
{
    int j;

    for( j= 0; j < 20; ++j )
    {
        printf("b\n");

        for( f1= 99999999; f1; --f1 );  /* use some CPU time */
    }

    printf("func1 end\n");
    return NULL;
}

void *thread_func2( void *vptr_args )
{
    int j;

    for( j= 0; j < 20; ++j )
    {
        printf("c\n");

        for( f2= 199999999; f2; --f2 );  /* use some CPU time */
    }

    printf("func2 end\n");
    return NULL;
}

