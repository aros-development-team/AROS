#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "test.h"

char *path = NULL;

int main() 
{
    char *path = NULL;
    int   error;
    
    chdir( "SYS:" );
    path  = getcwd( NULL, 0 );
    TEST( stricmp( path, "SYS:" ) == 0 );
    free( path ); path = NULL;
    
    chdir( "SYS:Tools" );
    path = getcwd( NULL, 0 );
    TEST( stricmp( path, "SYS:TOOLS" ) );
    free( path ); path = NULL;
    
    return OK;
}

void cleanup() 
{
    if( path != NULL )
    {
    	free( path );
    }
}
