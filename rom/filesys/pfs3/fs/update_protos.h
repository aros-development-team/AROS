/* Prototypes for functions defined in
update.c
 */

BOOL UpdateDisk(globaldata * );

BOOL MakeBlockDirty(struct cachedblock * , globaldata * );


void CheckUpdate (ULONG rtbf_threshold, globaldata *);

void UpdateDatestamp (struct cachedblock *blk, globaldata *g);
