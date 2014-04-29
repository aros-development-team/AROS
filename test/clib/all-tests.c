/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "test.h"

char *tests[] =
{
    "chdir",
    "strchr",
    "stpblk",
    "strtok",
    "tmpfile",
    "ctype",
    "sscanf",
    "mnt_names",
    "execl2",
    "execl2_vfork",
    "argv0_test1",
    "argv0_test2",
    "argv0_test3",
    "argv0_test4",

     NULL
};

int main()
{
    int total = 0, failed = 0;
    int i, rc;
    char buffer[128];

    mkdir( "T:TestOutput", 0777 );

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
    
    printf( "\n\nA total of %d tests run: %d succeeded, %d failed.\n",
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
