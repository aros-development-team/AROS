#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"

BPTR lockin = NULL, lockout = NULL;

int main() 
{
    char *pathin, *pathout = NULL;

    pathin = "SYS:";
    TEST( (lockin = Lock( pathin, SHARED_LOCK )) != NULL ); 
    TEST( chdir( pathin ) == 0 );
    pathout  = getcwd( NULL, 0 );
    TEST( (lockout = Lock( pathin, SHARED_LOCK )) != NULL );
    TEST( SameLock( lockin, lockout ) == LOCK_SAME );
    free( pathout ); pathout = NULL;
    UnLock( lockin ); lockin = NULL;
    UnLock( lockout ); lockout = NULL;
  
    pathin = "SYS:Tools";
    TEST( (lockin = Lock( pathin, SHARED_LOCK )) != NULL ); 
    TEST( chdir( pathin ) == 0 );
    pathout  = getcwd( NULL, 0 );
    TEST( (lockout = Lock( pathin, SHARED_LOCK )) != NULL );
    TEST( SameLock( lockin, lockout ) == LOCK_SAME );
    free( pathout ); pathout = NULL;
    UnLock( lockin ); lockin = NULL;
    UnLock( lockout ); lockout = NULL;
  
    return OK;
}

void cleanup() 
{
    if ( lockin != NULL )
        UnLock( lockin );
    if ( lockout != NULL )
        UnLock( lockout );
}
