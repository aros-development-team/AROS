/*
**
** Memory Pooled
**
** (C) 2005-2007 Guido Mersmann
**
** Allows in project resource tracking.
**
** This file allows to use memory pools in the normal way, just by using
** different function names. Replacing functions would be an option, but
** may also lead into problem, when dealing with external pools and only
** having these functions left.
**
** If DEBUG is defined, all functions will be extended with full
** multithreading resource tracking including memory wall check.
** This allows test software without slowing down the system. It
** also provides information on the memory block caused the problem.
**
** It's no longer possible to forget freeing memory blocks, because
** a debug dump reminds you.
**
**
** Note:
**
** Interaction between memory_pooled.c and memory_vec.c is implemented,
** so if you free with the wrong function a warning is spotted and the
** memory gets freed correct.
**
** The memory wall byte gets changed every allocation, so no chance for
** random faults.
**
*/

/*************************************************************************/

//#define SOURCENAME "memory_pooled.c"
//#define NODEBUG  /* turns off debug of this file */

#include <exec/types.h>
#include <proto/exec.h>
#include <libraries/iffparse.h> /* make_id macro */

#include "debug.h"
#include "memory.h"

/*************************************************************************/

/* /// "Debug Data - Private" */

/*************************************************************************/

#if DEBUG

/* name of the module, just a make the debug lines smaller. */

static char *MEMR_POOLNAME = "ResourceTracking - Memory Pools";

/* Pre init lists! It's for debug, so who cares */

static struct  List PoolHeaderList = {
	 (struct Node *) &PoolHeaderList.lh_Tail,
     NULL,
	 (struct Node *) &PoolHeaderList.lh_Head,
	 0,0
};

static struct  List PoolMemList = {
	 (struct Node *) &PoolMemList.lh_Tail,
     NULL,
	 (struct Node *) &PoolMemList.lh_Head,
	 0,0
};

/*
** signal semaphore to avoid the list stuff is colliding, when used in multi threading
*/

static struct SignalSemaphore poolsemaphore;
static struct SignalSemaphore memsemaphore;

#endif /* DEBUG */
/* \\\ */

/*************************************************************************/
#if DEBUG
/* /// "Memory_CreatePool" */

/*************************************************************************/

void *Memory_CreatePool( ULONG memflags, ULONG puddlesize, ULONG threshsize )
{
void *poolheader;
struct Node *node;

/* if this is the first pool, we need to init the semaphores */
	if( !(poolsemaphore.ss_WaitQueue.mlh_Head)) {
		InitSemaphore( &poolsemaphore );
		InitSemaphore( &memsemaphore );
	}
/* Now create a pool, alloc a node and apply the poolheader, so we can track it */
	ObtainSemaphore( &poolsemaphore );
	if( (poolheader = CreatePool(memflags,puddlesize,threshsize)) ) {
		if( (node = AllocVec( sizeof( struct Node ), MEMF_ANY|MEMF_CLEAR )) ) {
			node->ln_Name = (STRPTR) poolheader;
			AddTail( &PoolHeaderList, node );
		}
	}
	ReleaseSemaphore( &poolsemaphore );
/* return to sender */
	return( poolheader );
}

/* \\\ */

/* /// "Memory_DeletePool" */

/*************************************************************************/

void Memory_DeletePool( APTR poolheader )
{
struct Node *node, *node2;

	ObtainSemaphore( &poolsemaphore );
	for( node = PoolHeaderList.lh_Head  ; node->ln_Succ ; node = node->ln_Succ ) {
		if( node->ln_Name == poolheader ) {

        	ObtainSemaphore( &memsemaphore );
			for( node2 = PoolMemList.lh_Head ; node2->ln_Succ ; node2 = node2->ln_Succ ) {
				if( ((struct MEMR *) node2)->MEMR_PoolHeader == poolheader ) {
					//#define APPMEMORY(appmem) &((struct MEMR *) appmem)->MEMR_Wall_1[MEMR_WALLSIZE]
					debug("\n\n%78m*\n%s:\n  Forgot to free pool memory! Pool: %08lx  Memory: %08lx!\n\nContents: %08lx (Size: %08lx)\n%128lh\n\n%78m*\n", MEMR_POOLNAME, poolheader, GETMEM(node2), GETMEM(node2), ((struct MEMR *) PoolMemList.lh_TailPred)->MEMR_OriginalSize, GETMEM(node2));
					Memory_FreePooled( poolheader, GETMEM(node2) );
					node2 = (APTR) &PoolMemList.lh_Head; /* List is now invalid, so restart for() loop */
					/* FIXME: This may loop forever, due invalid header (e.g. when memory ogt corrupted!! */
	            }
	        }
			ReleaseSemaphore( &memsemaphore );
        	DeletePool( poolheader );
			Remove( node );
			FreeVec( node );
			ReleaseSemaphore( &poolsemaphore );
			return;
		}
	}
	debug("\n\n%78m*\n%s:\n  Tried to delete unknown pool: %08lx!\n%78m*\n", MEMR_POOLNAME, poolheader );
	ReleaseSemaphore( &poolsemaphore );
}

/* \\\ */
#endif

/* /// "Memory_AllocPooled" */

/*************************************************************************/

#ifndef DEBUG
APTR Memory_AllocPooled( APTR poolheader, ULONG size)
{
ULONG* memory;

	size += 4;
	if( (memory = AllocPooled(poolheader, size))) {
		*memory++ = size;
	}
	return( memory );
}
#else /* DEBUG */

APTR Memory_AllocPooled( APTR poolheader, ULONG size)
{
struct MEMR *memory;
BYTE *wall1;
BYTE *wall2;
ULONG i;

	i = size + sizeof( struct MEMR ) + MEMR_WALLSIZE;

	if( (memory = (struct MEMR *) AllocPooled( poolheader, i ) )) {

        memory->MEMR_OriginalSize = size;
		memory->MEMR_PoolHeader   = poolheader;
		memory->MEMR_AllocSize    = i;
		memory->MEMR_WallType     = memr_walltype++;
		memory->MEMR_ID           = ID_POOL;
		memory->MEMR_IDHeader     = memory;

		wall1 = (BYTE *) &(memory->MEMR_Wall_1);
        wall2 = wall1 + MEMR_WALLSIZE + size;

        for( i = MEMR_WALLSIZE ; i  ; i-- ) {
            *wall1++ = memory->MEMR_WallType;
            *wall2++ = memory->MEMR_WallType;
        }

		ObtainSemaphore( &memsemaphore );
		AddTail( &PoolMemList, (struct Node *) memory );
		ReleaseSemaphore( &memsemaphore );

        memory = (struct MEMR *) (((BYTE *) memory) +  sizeof( struct MEMR));
	}
	return( memory );
}

#endif /* DEBUG */

/* \\\ */

/* /// "Memory_FreePooled" */

/*************************************************************************/

#ifndef DEBUG
void Memory_FreePooled( APTR poolheader, APTR memory)
{
	ULONG size;
    if( memory) {
		memory = (APTR) (((ULONG) memory)-4);
		size = *((ULONG*) memory);
		FreePooled(poolheader, memory, size);
    }
}
#else
void Memory_FreePooled( APTR poolheader, APTR mem)
{
struct MEMR *memory;
struct MEMR *node;
BYTE *wall;
ULONG i;

    if( mem) {

		memory = GETMEMR( mem );

/* Check if memory block has our ID! If not, then free the normal way */
		if( memory->MEMR_ID != ID_POOL || memory->MEMR_IDHeader != memory ) {
			if( memory->MEMR_ID == ID_AVEC ) {
				debug("\n\n%78m*\n%s:\n  Tried to free AllocVec() memory as pooled: %08lx! (Size: unknown)\n%40lh\n\n%78m*\n", MEMR_POOLNAME, mem, mem);
				Memory_FreeVec( mem );
			} else {
				debug("\n\n%78m*\n%s:\n  Tried to free unknown memory node: %08lx! (Size: unknown)\n%128lh\n\n%78m*\n", MEMR_POOLNAME, mem, mem);
				/* FreeVec( mem); We don't free memory, because pointer may wrong, or AllocMem pointer. */
			}
            return;
        }
/* now check if the PoolHeader is correct */

		if( memory->MEMR_PoolHeader != poolheader ) {
			debug("\n\n%78m*\n%s:\n  PoolHeader wrong for memory block: %08lx!\nContents: %08lx (Size: %08lx)\n%128lh\n\n%78m*\n", MEMR_POOLNAME, mem, mem, memory->MEMR_OriginalSize, mem);
			/* now we correct the error and continue */
			poolheader = memory->MEMR_PoolHeader;
		}

/* The memory block is confirmed to be pooled type debug.  If the memory block isn't in list, then it's tried to be freed twice! */

		ObtainSemaphore( &memsemaphore );
		for( node = (struct MEMR *) PoolMemList.lh_Head ; node->MEMR_Next ; node = node->MEMR_Next ) {
            if( node == memory ) {
                break;
            }
        }

        if( node == memory ) {
            Remove( (struct Node *) memory);
        } else {
			debug("\n\n%78m*\n%s:\n  Tried to free node twice: %08lx!\n%78m*\n", MEMR_POOLNAME, mem);
			ReleaseSemaphore( &memsemaphore );
            return;
        }
		ReleaseSemaphore( &memsemaphore );

/* Now the memory is confirmed to be pooled and freeable. Now we check our memory walls to be intact */

		wall = (BYTE *) &(memory->MEMR_Wall_1);

        for( i = MEMR_WALLSIZE ; i  ; i-- ) {
            if( *wall++ != memory->MEMR_WallType) {
				debug("\n\n%78m*\n%s:\n  Wall Hit: Memory: %08lx caused lower wall hit!\n\nContents: %08lx (Size: %08lx)\n%128lh\n\n%78m*\n", MEMR_POOLNAME, mem, mem, memory->MEMR_OriginalSize, mem);
                break;
            }
        }

		wall = ((BYTE *) &(memory->MEMR_Wall_1)) + MEMR_WALLSIZE + memory->MEMR_OriginalSize;

        for( i = MEMR_WALLSIZE ; i  ; i-- ) {
            if( *wall++ != memory->MEMR_WallType) {
				debug("\n\n%78m*\n%s:\n  Wall Hit: Memory: %08lx caused upper wall hit!\n\nContents: %08lx (Size: %08lx)\n%128lh\n\n%78m*\n", MEMR_POOLNAME, mem, mem, memory->MEMR_OriginalSize, mem);
                break;
            }
        }

/* Finaly dump memory */
		FreePooled( poolheader, memory, memory->MEMR_AllocSize);
    }
}

#endif
/* \\\ */

