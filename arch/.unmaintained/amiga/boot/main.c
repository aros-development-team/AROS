#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "boot.h"
#include "registers.h"

/*
    Set this to 1 to make a non-test version:
*/
#define FINAL 1

struct MemList *BuildMemList(struct ilsMemList *, ULONG *, ULONG);
void StuffTags(struct MemList *, ULONG *, ULONG);
void *FindResMod(struct ilsMemList *);
extern LONG ils_read(BPTR __d1, void * __d2, LONG __d3, struct DosLibrary * __a6);
extern void *ils_alloc(ULONG __d0, ULONG __d1, struct ExecBase * __a6);
extern void ils_free(void * __a1, ULONG __d0, struct ExecBase * __a6);

#define BANNER "AROS system loader"
#define VERSIONSTRING "1.0"

char *version = "$VER: " BANNER " " VERSIONSTRING " (30.12.96)";

ULONG stack = 16384;
ULONG ils_table[3];
struct ilsMemList ils_mem;

int main(void)
{
    BPTR seglist, fh;
    struct Resident *resmod;
    struct MemList *memlist;
    ULONG *modlist;
    ULONG nummods = 0;

    Printf(BANNER " " VERSIONSTRING "\n");
    Printf("Checking if KickTagPtr is already in use... %s\n",
	SysBase->KickTagPtr ? (ULONG)(UBYTE *)"yes" : (ULONG)(UBYTE *)"no" );

    if(SysBase->KickTagPtr)
    {
	ULONG *list;

	Printf("Modules already in use:\n");

	list = SysBase->KickTagPtr;

	while(*list)
	{
	    Printf("\t0x%08lx", *list);
	    Printf("\t%s\n", (ULONG)((struct Resident *)*list)->rt_IdString);

	    list++;
	    if(*list & 0x80000000) list = (ULONG *)(*list & 0x7fffffff);
	}
    }

    /*
	InternalLoadSeg use
    */
    ils_table[0] = (ULONG)&ils_read;
    ils_table[1] = (ULONG)&ils_alloc;
    ils_table[2] = (ULONG)&ils_free;

    NewList((struct List *)&ils_mem);

    if(!(fh = Open("libs/exec.strap", MODE_OLDFILE)))
    {
	PrintFault(IoErr(), "Error");
	exit(10);
    }

    if( (seglist = InternalLoadSeg(fh, NULL, &ils_table[0], &stack)) )
    {
	resmod = FindResMod(&ils_mem);
	nummods++;

	Printf("Loaded: %s\n", (ULONG)resmod->rt_IdString);

	/*
	    Allocate memory for our KickTagPtr table:
	    number of modules loaded + 1 (table terminator)
	*/
	if( (modlist = AllocVec( (nummods+1)*sizeof(ULONG), MEMF_CLEAR|MEMF_KICK|MEMF_REVERSE)) )
	{
	    modlist[0] = (ULONG)resmod;
	    modlist[nummods] = NULL;

	    /*
		build a memlist to be put in KickMemPtr
	    */
	    if( (memlist = BuildMemList(&ils_mem, modlist, (nummods+1)*sizeof(ULONG) )) )
	    {
#if FINAL == 1
		StuffTags(memlist, modlist, nummods);
		Close(fh);
		Printf("Modules loaded, reset to activate\n");
		exit(0);
#else
		Printf("Test complete, cleaning up\n");
		FreeVec(memlist);
#endif
	    }
	    FreeVec(modlist);
	}

	/*
	    We come here if anything went wrong.
	*/
	InternalUnLoadSeg(seglist, &ils_free);
    }
    Close(fh);

    exit(10);
}

struct MemList *BuildMemList(struct ilsMemList *prelist, ULONG *modlist, ULONG modlistsize)
{
    struct MemList *memlist;
    struct MemEntry *me;
    struct ilsMemNode *node;

    /*
	Calculate needed number of extra MemEntry fields that need to be
	appended to the end of our MemList structure. This number+1 (there
	already is one MemEntry in the MemList) is what we'll be putting in
	memlist->ml_NumEntries later.
    */
    UWORD numentries = prelist->iml_Num + 1;

    ULONG size = ( sizeof(struct MemList) + (sizeof(struct MemEntry) * numentries) );

    /*
	Allocate memory from the top of the memory list, to keep fragmentation
	down, and hopefully to keep all our applications in one place.
    */
    if(!(memlist = AllocVec(size, MEMF_CLEAR|MEMF_KICK|MEMF_REVERSE))) return(NULL);

    /*
	We have the memory. Assure that this MemList is allocated during a
	reset, and put it in the first MemEntry field.
    */
    memlist->ml_ME[0].me_Addr = memlist;
    memlist->ml_ME[0].me_Length = size;

    /*
	Indicate that this node is part of a KickMem MemList, and indicate how
	many MemEntries there are.
    */
    memlist->ml_Node.ln_Type = NT_KICKMEM;
    memlist->ml_NumEntries = numentries+1;

    /*
	Put the modlist in the MemEntry.
    */
    me = (struct MemEntry *)( ((ULONG)memlist) + sizeof(struct MemList) );
    me->me_Addr = modlist;
    me->me_Length = modlistsize;

    /*
	Increase to the next MemEntry.
    */
    me = (struct MemEntry *)( ((ULONG)me) + sizeof(struct MemEntry) );

    /*
	Now put the data from our prelist in the remaining MemEntry fields,
	removing the nodes in our prelist and freeing them in the process.
    */
    while(prelist->iml_Num)
    {
	/*
	    pop a node off the list (FIFO)
	*/
	node = (struct ilsMemNode *)RemHead((struct List *)prelist);

	me->me_Addr = node->imn_Addr;
	me->me_Length = node->imn_Size;

	FreeMem(node, sizeof(struct ilsMemNode));

	/*
	    Point to the next MemEntry, decrease our counter.
	*/
	me = (struct MemEntry *)(((ULONG)me) + sizeof(struct MemEntry));
	prelist->iml_Num--;
    }

    return memlist;
}

void StuffTags(struct MemList *memlist, ULONG *modlist, ULONG nummods)
{
    /* Fix 970102 ldp: protect the Kick ptrs with Forbid/Permit */
    Forbid();

    if(SysBase->KickMemPtr)
    {
	/*
	    Prepend to an existing chain of MemLists.
	*/
	memlist->ml_Node.ln_Succ = SysBase->KickMemPtr;
    }
    SysBase->KickMemPtr = memlist;

    if(SysBase->KickTagPtr)
    {
	/*
	    Prepend to an existing chain of module lists.
	*/
	modlist[nummods] = (ULONG)SysBase->KickTagPtr | 0x80000000;
    }
    SysBase->KickTagPtr = modlist;

    /*
	Flush caches. I have some problems with having to boot twice to get
	into AROS. Maybe this will help? Or it may be the BlizKick util I'm
	using that gets in the way? Or maybe something totally unexpected? :)
    */
    CacheClearU();

    /*
	And checksum the KickPtrs. KickCheckSum is defined wrong in the include
        file exec/execbase.h. It is defined as APTR, but really is a ULONG.
    */
    SysBase->KickCheckSum = (APTR)SumKickData();

    /*
	Flush caches (highly recommended).
    */
    CacheClearU();

    Permit();
}

void *FindResMod(struct ilsMemList *list)
{
    struct ilsMemNode *node;
    UWORD *ptr;
    ULONG counter;

    /*
	search all our memory blocks for the Resident struct
    */
    for(node = (struct ilsMemNode *)ils_mem.iml_List.mlh_Head; node->imn_Node.mln_Succ; node = (struct ilsMemNode *)node->imn_Node.mln_Succ)
    {
	/*
	    In each block: skip to the magic word, if present.
	    Searching by word: divide by 2
	*/
	counter = node->imn_Size/2;

	for(ptr = node->imn_Addr; counter; ptr++, counter--)
	{
	    if(*ptr == RTC_MATCHWORD) return((void *)ptr);
	}
    }

    return 0;
}
