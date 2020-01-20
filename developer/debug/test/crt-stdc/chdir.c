/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"

BPTR lockin = BNULL, lockout = BNULL;

int main() 
{
    char *pathin, *pathout = NULL;

    pathin = "SYS:";
    TEST( (lockin = Lock( pathin, SHARED_LOCK )) != BNULL ); 
    TEST( chdir( pathin ) == 0 );
    pathout  = getcwd( NULL, 0 );
    TEST( (lockout = Lock( pathin, SHARED_LOCK )) != BNULL );
    TEST( SameLock( lockin, lockout ) == LOCK_SAME );
    free( pathout ); pathout = NULL;
    UnLock( lockin ); lockin = BNULL;
    UnLock( lockout ); lockout = BNULL;
  
    pathin = "SYS:Tools";
    TEST( (lockin = Lock( pathin, SHARED_LOCK )) != BNULL ); 
    TEST( chdir( pathin ) == 0 );
    pathout  = getcwd( NULL, 0 );
    TEST( (lockout = Lock( pathin, SHARED_LOCK )) != BNULL );
    TEST( SameLock( lockin, lockout ) == LOCK_SAME );
    free( pathout ); pathout = NULL;
    UnLock( lockin ); lockin = BNULL;
    UnLock( lockout ); lockout = BNULL;
  
    return OK;
}

void cleanup() 
{
    if ( lockin != BNULL )
        UnLock( lockin );
    if ( lockout != BNULL )
        UnLock( lockout );
}
