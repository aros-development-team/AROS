#include <stdio.h>
#include <string.h>

int main() {
    char *string = "test";
    char *p;

    p = strchr( string, '\0' );	
    if( p != NULL ) 
        printf( "OK, found the NULL char.\n" );
    else
        printf( "ERROR, didn't find NULL char.\n" );

    p = strchr( string, 't' );
    if( *p == 't' && p == &string[0] ) 
        printf( "OK, found first 't'\n" );
    else
        printf( "ERROR, didn't find first 't'\n" );

    p = strchr( ++p, 't' );
    if( *p == 't' && p == &string[3] ) 
        printf( "OK, found second 't'\n" );
    else 
        printf( "ERROR, didn't find second 't'\n" );

    p = strchr( ++p, 't' );
    if( p == NULL ) 
        printf( "OK, didn't find any more chars.\n" );
    else
        printf( "ERROR, found char '%c'\n", *p );

    return 0;
}
