/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"

#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE

#undef SysBase
#undef DOSBase

/*
	-If ObtainExclusiveSem is called, the caller is the only allowed to access,
	 until it calls ReleaseExclusiveSem.
	-ObtainSharedSem ensures callers calling ObtainExclusiveSem to wait until
    they get excluive access.

	1. Obtain/Relase Shared should normally be as fast as possible.
	2. Time to Obtain/Release Exclusive doesn't matter that much.
   3. Obtain/Release Exclusive happens relatively seldom.
   4. The ObtainExclusiveSem function is allready exclusive by ObtainSemaphore(&camdroot->CLsemaphore);
   5. ObtainExclusive caller does not use much time, except for the Delay(1) wait.
      (which may leed to problems with small buffers by the way)

*/


void InitCamdMutEx(
	struct CamdMutEx *mutex
){
	InitSemaphore(&mutex->semaphore);
	mutex->shared=0;
	mutex->exclusive=FALSE;
}


ULONG ObtainSharedSem(
	struct CamdMutEx *mutex
){
	Forbid();
		if(mutex->exclusive==TRUE){
			Permit();
			ObtainSemaphore(&mutex->semaphore);
			mutex->shared++;
			return 1;
		}
		mutex->shared++;

	Permit();
	return 0;
}

void ReleaseSharedSem(
	struct CamdMutEx *mutex,
	ULONG lock
){
	mutex->shared--;
	if(lock!=0){
		ReleaseSemaphore(&mutex->semaphore);
	}
}


/* CLSemaphore must be obtained before calling any of the two next functions. */

void ObtainExclusiveSem(
	struct CamdMutEx *mutex
){
	mutex->exclusive=TRUE;
	ObtainSemaphore(&mutex->semaphore);
	while(mutex->shared>0) Delay(1);
}

void ReleaseExclusiveSem(
	struct CamdMutEx *mutex
){
	mutex->exclusive=FALSE;
	ReleaseSemaphore(&mutex->semaphore);
}

