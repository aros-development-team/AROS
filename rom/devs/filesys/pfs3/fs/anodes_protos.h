/* Prototypes for functions defined in
Anodes.c
 */

struct anodechain *GetAnodeChain (ULONG anodenr, globaldata *g);
void DetachAnodeChain (struct anodechain *chain, globaldata *g);
BOOL NextBlockAC (struct anodechainnode **acnode, ULONG *anodeoffset, globaldata *g);
BOOL CorrectAnodeAC (struct anodechainnode **acnode, ULONG *anodeoffset, globaldata *g);

struct cindexblock *GetSuperBlock (UWORD nr, globaldata *g);
BOOL NextBlock(struct canode * , ULONG * , globaldata * );
BOOL CorrectAnode(struct canode * , ULONG * , globaldata * );
void GetAnode(struct canode * , ULONG , globaldata * );
void SaveAnode(struct canode * , ULONG , globaldata * );
ULONG AllocAnode(ULONG connect, globaldata *g);
void FreeAnode(ULONG , globaldata * );

struct cindexblock * GetIndexBlock(UWORD , globaldata * );
void RemoveFromAnodeChain(struct canode const * , ULONG , ULONG , globaldata * );
void InitAnodes(struct volumedata * , BOOL, globaldata * );

