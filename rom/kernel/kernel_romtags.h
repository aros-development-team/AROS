typedef struct ExecBase *(INITFUNC)(struct MemHeader *, struct TagItem *);

/* Look for ROMTags in specified address ranges and build resident list */
APTR krnRomTagScanner(struct MemHeader *mh, UWORD *ranges[]);

/* Find exec.library early init entry point */
INITFUNC *findExecInit(struct Resident **resList);
