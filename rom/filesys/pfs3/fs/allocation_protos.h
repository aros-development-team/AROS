/* Prototypes for functions defined in
allocation.c
 */

void InitAllocation (struct volumedata * , globaldata * );

BOOL AllocateBlocks (ULONG , ULONG , globaldata * );
BOOL AllocateBlocksAC (struct anodechain *achain, ULONG size, struct fileinfo *ref, globaldata *g);
VOID FreeBlocksAC (struct anodechain *achain, ULONG size, enum freeblocktype freetype, globaldata *g);
void UpdateFreeList(globaldata * );

ULONG AllocReservedBlock(globaldata * );
ULONG AllocReservedBlockSave(globaldata * );
void FreeReservedBlock(ULONG , globaldata * );

cindexblock_t * GetBitmapIndex(UWORD , globaldata * );
struct cbitmapblock * NewBitmapBlock(UWORD , globaldata * );
struct cbitmapblock *GetBitmapBlock (UWORD seqnr, globaldata *g);
