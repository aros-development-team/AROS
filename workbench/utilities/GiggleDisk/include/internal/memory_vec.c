/*
**
** Memory Vec
**
** (C) 2005-2007 Guido Mersmann
**
** Allows in project resource tracking.
**
** This file allows to use memory allocations in the normal way, just by using
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

//#define SOURCENAME "memory_vec.c"
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

static char *MEMR_VECNAME = "ResourceTracking - Memory AllocVec";

/* Pre init lists! It's for debug, so who cares */

static struct  List VecMemList = {
	 (struct Node *) &VecMemList.lh_Tail,
     NULL,
	 (struct Node *) &VecMemList.lh_Head,
	 0,0
};

/*
** signal semaphore to avoid the list stuff is colliding, when used in multi threading
*/

static struct SignalSemaphore memsemaphore;

#endif /* DEBUG */

/* \\\ */

/*************************************************************************/

/* /// "Memory_AllocVec" */

#ifdef DEBUG

/*************************************************************************/

APTR Memory_AllocVec( ULONG size)
{
struct MEMR *memory;
BYTE *wall1;
BYTE *wall2;
ULONG i;

	i = size + sizeof( struct MEMR ) + MEMR_WALLSIZE;

	if( (memory = (struct MEMR *) AllocVec( i, MEMF_ANY|MEMF_CLEAR ) )) {

        memory->MEMR_OriginalSize = size;
		memory->MEMR_AllocSize    = i;
		memory->MEMR_WallType     = memr_walltype++;
		memory->MEMR_ID           = ID_AVEC;
		memory->MEMR_IDHeader     = memory;

		wall1 = (BYTE *) &(memory->MEMR_Wall_1);
        wall2 = wall1 + MEMR_WALLSIZE + size;

        for( i = MEMR_WALLSIZE ; i  ; i-- ) {
            *wall1++ = memory->MEMR_WallType;
            *wall2++ = memory->MEMR_WallType;
        }

		if( !(memsemaphore.ss_WaitQueue.mlh_Head)) {
			InitSemaphore( &memsemaphore );
		}

		ObtainSemaphore( &memsemaphore );
		AddTail( &VecMemList, (struct Node *) memory );
		ReleaseSemaphore( &memsemaphore );

        memory = (struct MEMR *) (((BYTE *) memory) +  sizeof( struct MEMR));
	}
	return( memory );
}

#endif

/* \\\ */

/* /// "Memory_FreeVec" */

/*************************************************************************/

#ifndef DEBUG

void Memory_FreeVec( APTR memory)
{
    if( memory) {
		FreeVec( memory );
    }
}

#else

void Memory_FreeVec( APTR mem)
{
struct MEMR *memory;
struct MEMR *node;
BYTE *wall;
ULONG i;

    if( mem) {

		memory = GETMEMR( mem );
		 //(struct MEMR *) ( ((long) mem) - sizeof( struct MEMR));

/* Check if memory block has our ID! If not, then free the normal way */
		if( memory->MEMR_ID != ID_AVEC || memory->MEMR_IDHeader != memory ) {
			if( memory->MEMR_ID == ID_POOL ) {
				debug("\n\n%78m*\n%s:\n  Tried to free AllocPooled() memory as AllocVec(): %08lx! (Size: unknown)\n%40lh\n\n%78m*\n", MEMR_VECNAME, mem, mem);
				Memory_FreePooled( memory->MEMR_PoolHeader, mem );
			} else {
				debug("\n\n%78m*\n%s:\n  Tried to free unknown memory node: %08lx! (Size: unknown)\n%128lh\n\n%78m*\n", MEMR_VECNAME, mem, mem);
				/* FreeVec( mem); We don't free memory, because pointer may wrong, or AllocMem pointer. */
			}
            return;
        }

/* The memory block is confirmed to be pooled type debug.  If the memory block isn't in list, then it's tried to be freed twice! */

		if( !(memsemaphore.ss_WaitQueue.mlh_Head)) {
			InitSemaphore( &memsemaphore );
		}

		ObtainSemaphore( &memsemaphore );
		for( node = (struct MEMR *) VecMemList.lh_Head ; node->MEMR_Next ; node = node->MEMR_Next ) {
            if( node == memory ) {
                break;
            }
        }

        if( node == memory ) {
            Remove( (struct Node *) memory);
        } else {
			debug("\n\n%78m*\n%s:\n  Tried to free node twice: %08lx!\n%78m*\n", MEMR_VECNAME, mem);
			ReleaseSemaphore( &memsemaphore );
            return;
        }
		ReleaseSemaphore( &memsemaphore );

/* Now the memory is confirmed to be pooled and freeable. Now we check our memory walls to be intact */

		wall = (BYTE *) &(memory->MEMR_Wall_1);

        for( i = MEMR_WALLSIZE ; i  ; i-- ) {
            if( *wall++ != memory->MEMR_WallType) {
				debug("\n\n%78m*\n%s:\n  Wall Hit: Memory: %08lx caused lower wall hit!\n\nContents: %08lx (Size: %08lx)\n%128lh\n\n%78m*\n", MEMR_VECNAME, mem, mem, memory->MEMR_OriginalSize, mem);
                break;
            }
        }

		wall = ((BYTE *) &(memory->MEMR_Wall_1)) + MEMR_WALLSIZE + memory->MEMR_OriginalSize;

        for( i = MEMR_WALLSIZE ; i  ; i-- ) {
            if( *wall++ != memory->MEMR_WallType) {
				debug("\n\n%78m*\n%s:\n  Wall Hit: Memory: %08lx caused upper wall hit!\n\nContents: %08lx (Size: %08lx)\n%128lh\n\n%78m*\n", MEMR_VECNAME, mem, mem, memory->MEMR_OriginalSize, mem);
                break;
            }
        }

/* Finaly dump memory */
		FreeVec( memory );
    }
}

#endif
/* \\\ */

/* /// "Memory_CheckVec" */

/*************************************************************************/

#ifdef DEBUG

void Memory_CheckVec( void)
{
BYTE *mem;

	if( !(memsemaphore.ss_WaitQueue.mlh_Head)) {
		InitSemaphore( &memsemaphore );
	}
	ObtainSemaphore( &memsemaphore );

	while( VecMemList.lh_TailPred != (struct Node *) &VecMemList ) {

		mem = GETMEM( VecMemList.lh_TailPred );
		debug("\n\n%78m*\n%s:  Forgot to free memory block: %08lx!\n\nContents: %08lx (Size: %08lx)\n%128lh\n\n%78m*\n", MEMR_VECNAME, mem, mem, ((struct MEMR *) VecMemList.lh_TailPred)->MEMR_OriginalSize, mem);
        Memory_FreeVec( mem );
    }
	ReleaseSemaphore( &memsemaphore );
}
#endif
/* \\\ */

