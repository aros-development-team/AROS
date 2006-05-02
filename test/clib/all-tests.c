#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "test.h"

char *tests[] =
{
    "chdir",
    "strchr",
    "stpblk",
    "tmpfile",

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
    
    printf( "\n\nA total of %d tests run: %d succeded, %d failed.\n", total, total - failed, failed );  
    
    if( failed > 0 ) 
    {
    	printf( "\nError messages:\n" );
    	system( "join T:TestOutput/test-#? as T:TestOutput/all-tests.log" );
	system( "type T:TestOutput/all-tests.log" );
    }
    
    system( "delete T:TestOutput ALL QUIET" );
    
    return 0;
}
