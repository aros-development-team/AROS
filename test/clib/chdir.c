#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "test.h"

char *path = NULL;

int main() 
{
    char *path = NULL;
    int   error;
    
    TEST( chdir( "SYS:" ) == 0 );
    path  = getcwd( NULL, 0 );
    TEST( stricmp( path, "SYS:" ) == 0 );
    free( path ); path = NULL;
    
    TEST( chdir( "SYS:Tools" ) == 0 );
    path = getcwd( NULL, 0 );
    TEST( strcmp( path, "SYS:Tools" ) == 0 );
    free( path ); path = NULL;
    
    return OK;
}

void cleanup() 
{
    if( path != NULL ) free( path );
}
