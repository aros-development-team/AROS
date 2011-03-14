/* Look for ROMTags in specified address ranges and build resident list */
APTR krnRomTagScanner(struct MemHeader *mh, UWORD *ranges[]);

/* Find ROMTags and call exec.library early entry point to create ExecBase */
struct ExecBase *krnPrepareExecBase(UWORD *ranges[], struct MemHeader *mh, struct TagItem *bootMsg);
