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
	

	return 0;
}
