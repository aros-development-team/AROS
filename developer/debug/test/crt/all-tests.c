/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "test.h"

char *stdc_tests[] =
{
    "stdc/strchr",
    "stdc/stpblk",
    "stdc/strtok",
    "stdc/tmpfile",
    "stdc/ctype",
    "stdc/sscanf",

    NULL
};

char *posixc_tests[] =
{
    "posixc/chdir",
    "posixc/mnt_names",
    "posixc/execl2",
    "posixc/execl2_vfork",
    "posixc/argv0_test1",
    "posixc/argv0_test2",
    "posixc/argv0_test3",
    "posixc/argv0_test4",

    NULL
};

int total = 0, failed = 0;

void runtests(char *buffer, char *tests[])
{
    int i, rc;

    for( i = 0; tests[i] != NULL; i++ )
    {
    	total++;
	sprintf( buffer, "%s >T:TestOutput/test-%d.log", tests[i], i );

	rc = system( buffer );

	if( rc == OK ) 
	{
	    printf( "." );
	    fflush( stdout );
	} 
	else 
	{
	    failed++;
	    printf( "F" );
	    fflush( stdout );
	}
    }
}

int main()
{
    char buffer[128];

    mkdir( "T:TestOutput", 0777 );

    printf( "\nRunning C Standard Library tests...\n");
    runtests(buffer, stdc_tests);
    printf( "\nRunning C POSIX Library tests...\n");
    runtests(buffer, posixc_tests);

    printf( "\n\nPerformed %d tests: %d succeeded, %d failed.\n",
        total, total - failed, failed );  

    if( failed > 0 ) 
    {
    	printf( "\nError messages:\n" );
    	system( "join T:TestOutput/test-#? as T:TestOutput/all-tests.log" );
	system( "type T:TestOutput/all-tests.log" );
    }

    system( "delete T:TestOutput ALL QUIET" );

    return 0;
}
