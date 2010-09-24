#include <errno.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"

void SetError(int error, struct TaskBase *libPtr)
{
    switch (libPtr->errnoSize)
    {
    case 8:
	*(UQUAD *)libPtr->errnoPtr = (UQUAD)error;
	break;

    case 4:
	*(ULONG *)libPtr->errnoPtr = (ULONG)error;
	break;

    case 2:
	*(UWORD *)libPtr->errnoPtr = (UWORD)error;
	break;

    case 1:
	*(UBYTE *)libPtr->errnoPtr = (UBYTE)error;
	break;

    default:
	D(bug("[SetErrno] Bogus errno size %u for TaskBase 0x%p\n", libPtr->errnoSize, libPtr));
	break;
    }
}
