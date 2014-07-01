/* Prototypes for functions defined in
lru.c
 */

BOOL InitLRU(globaldata *, UWORD);
void DeallocLRU(globaldata *);

struct cachedblock * AllocLRU(globaldata * );

void FlushBlock(struct cachedblock * , globaldata * );

void UpdateReference(ULONG , struct cdirblock * , globaldata * );

void UpdateLE(listentry_t * , globaldata * );

void UpdateLE_exa(lockentry_t * , globaldata * );

struct cachedblock * CheckCache(struct MinList * , UWORD , ULONG , globaldata * );

void ResToBeFreed(ULONG blocknr, globaldata *g);
