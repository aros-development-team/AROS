
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_memory.h"

/*
 *      Memory Pool Characteristics
 */

#define MEM_PUDDLESIZE	4000
#define MEM_THRESHSIZE	4000

/*
 *      Our Private Memory Pool
 */

APTR Pool = NULL;

/*
 *      Access Control Semaphore
 */

struct SignalSemaphore Semaphore;

/*
 *      Initialisation
 */

BOOL InitMemory(void)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    InitSemaphore(&Semaphore);
    ObtainSemaphore(&Semaphore);
    Pool = CreatePool(MEMF_PUBLIC, MEM_PUDDLESIZE, MEM_THRESHSIZE);

    D(
        if(Pool)
            bug( DEBUG_NAME_STR " %s: Pool @ %p\n", __func__, Pool);
      )

    ReleaseSemaphore(&Semaphore);
    return((BOOL)(Pool ? TRUE : FALSE));
}

/*
 *      Clean Up
 */

void CleanUpMemory(void)
{
    ObtainSemaphore(&Semaphore);
    if (Pool) {
        DeletePool(Pool);
        Pool = NULL;
    }
    ReleaseSemaphore(&Semaphore);
}


/*
 *      Private implementation for AllocMem()
 */

APTR MAlloc(ULONG size)
{
    ULONG *block;

    if (size==0)	{
    D(bug( DEBUG_NAME_STR " %s: FAILED, size == 0!\n", __func__);)
    return NULL;
    }

    ObtainSemaphore(&Semaphore);
    if ((block = AllocPooled(Pool, size)))
        SetMem(block, 0, size);
    ReleaseSemaphore(&Semaphore);
    D(
        if (block==NULL)	{
            bug( DEBUG_NAME_STR " %s: FAILED, size = %ld\n", __func__, size);
        }
      )
    return(block);
}


/*
 *      Private implementation for FreeMem()
 */

void Free(APTR block, ULONG size)
{
    if (block) {
        ObtainSemaphore(&Semaphore);
        FreePooled(Pool, block, size);
        ReleaseSemaphore(&Semaphore);
    }
    D(else
        bug( DEBUG_NAME_STR " %s: FAILED, block == NULL (size%ld)\n", __func__, size);)
}


/*
*       Private implementation for AllocVec()
*/

APTR MAllocV(ULONG size)
{
    IPTR *block;

    if (size==0)	{
        D(bug( DEBUG_NAME_STR " %s: FAILED, size == 0!\n", __func__);)
        return NULL;
    }

    ObtainSemaphore(&Semaphore);
    if ((block = AllocPooled(Pool, size + sizeof(IPTR)))) {
        *(block++) = size + sizeof(IPTR);
        SetMem(block, 0, size);
    }
    ReleaseSemaphore(&Semaphore);
    D(
        if (block==NULL)	{
            bug( DEBUG_NAME_STR " %s: FAILED, size = %d!\n", __func__, size);
        }
      )
    return(block);
}


/*
*      Private implementation for FreeVec()
*/

void FreeV(APTR block)
{
    if (block) {
        ObtainSemaphore(&Semaphore);
        FreePooled(Pool, (APTR)((IPTR)block-sizeof(IPTR)), *(IPTR *)((IPTR)block-sizeof(IPTR)));
        ReleaseSemaphore(&Semaphore);
    }
    D(else
        bug( DEBUG_NAME_STR " %s: FAILED, block == NULL!\n");)
}
