#ifndef MEMORYTRACKING_H
#define MEMORYTRACKING_H 1

/*************************************************************************/

#include <exec/types.h>
#include <libraries/iffparse.h>

/*************************************************************************/

#ifdef DEBUG /* only needed when debug is enabled */

#define MEMR_WALLSIZE 100  /* used before and after application memory. */

#define ID_POOL MAKE_ID('P','O','O','L')	/* identifies pooled memory */
#define ID_AVEC MAKE_ID('A','V','E','C')	/* identifies allocvec memory */

extern BYTE memr_walltype;

/*
** This structure will be added to the top of each memory block to keep debug and tracking information
*/

struct MEMR {
	APTR  MEMR_Next;             /* standard node stuff */
	APTR  MEMR_Prev;             /* standard node stuff */
	long  MEMR_ID;               /* ID to identify this structure */
	APTR  MEMR_IDHeader;         /* Pointer on ListHeader ro identify this structure */
	ULONG MEMR_OriginalSize;    /* Size the applications wants */
	ULONG MEMR_AllocSize;       /* size includung structure and walls */
	APTR  MEMR_PoolHeader;       /* handle used for allocation, must be equal when calling freepooled */
	BYTE  MEMR_WallType;         /* type of wall used for this block */
	BYTE  Reserved[3];           /* keep stuff long aligned */
	BYTE  MEMR_Wall_1[MEMR_WALLSIZE];
};

#define GETMEM(memr) (&((struct MEMR *) memr)->MEMR_Wall_1[MEMR_WALLSIZE])
#define GETMEMR(mem) ((struct MEMR *) ( ((long) mem) - sizeof( struct MEMR)))

#endif

/*
** Prototypes
*/


APTR  Memory_AllocPooled(APTR poolheader, ULONG size);
void  Memory_FreePooled( APTR poolheader, APTR memory);

void Memory_FreeVec(APTR memory);

#ifdef DEBUG

void *Memory_CreatePool( ULONG memflags, ULONG puddlesize, ULONG threshsize );
void  Memory_DeletePool( APTR poolheader );
void  Memory_CheckVec( void);
APTR  Memory_AllocVec(ULONG size);

#else

#define Memory_CreatePool( memflags, puddlesize, threshsize ) CreatePool(memflags,puddlesize,threshsize)
#define Memory_DeletePool( poolheader ) DeletePool( poolheader )
#define Memory_AllocVec( size ) AllocVec( size, MEMF_ANY|MEMF_CLEAR )
#define Memory_FreeVec( addr ) FreeVec( addr )
#define Memory_CheckVec()

#endif

#ifndef ASYNC_LIBDEV_H
extern APTR pool;
 #define MemoryAllocPooled( size ) Memory_AllocPooled( pool, size )
 #define MemoryFreePooled( mem ) Memory_FreePooled( pool , mem )
#else
 #define MemoryAllocPooled( size ) Memory_AllocPooled( libbase->MemoryPool, size )
 #define MemoryFreePooled( mem ) Memory_FreePooled( libbase->MemoryPool , mem )
#endif

/*************************************************************************/

#endif /* MEMORYTRACKING_H */

