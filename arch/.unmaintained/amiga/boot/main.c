/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Amiga bootloader -- main file
    Lang: C
*/

/*
    Note: This whole thing may seem a bit kludgy. It is.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "boot.h"
#include "ils.h"
#include "config.h"
#include "version.h"

LONG Printf(STRPTR format, ...);

struct Module *LoadModule(char *);
void FreeModules(struct ModuleList *);
BOOL BuildTagPtrs(struct ModuleList *);
struct MemList *BuildMemList(struct ilsMemList *, ULONG *, ULONG);
void StuffTags(struct MemList *, ULONG *, ULONG);
void *FindResMod(struct ilsMemList *);
void PrintTagPtrs(void);

char *version = "$VER: " BANNER " " VERSIONSTRING " " DATE;
char *configfile = "boot.config";

ULONG stack = 16384;
ULONG memtype;
ULONG ils_table[3];
struct ilsMemList ils_mem;

char usage[] =
{
    " -c -- force to chipmem\n"
    " -f -- force to fastmem\n"
    " -k -- force to kickmem\n"
};

int main(int argc, char **argv)
{
    struct FileList *filelist;
    struct Node *node;
    struct Module *module;
    struct ModuleList ModuleList;

    PutStr("\n" BANNER " " VERSIONSTRING "\n\n");

    if(SysBase->LibNode.lib_Version < 37)
    {
	PutStr("This utility is for AmigaOS 2.04 (V37) and higher\n\n");
	exit(RETURN_WARN);
    }

    /*
	The memory type MEMF_KICK has been added in V39 exec. Fall back to
	MEMF_CHIP if we are on an earlier version.
    */
    if(SysBase->LibNode.lib_Version < 39)
    {
	memtype = MEMF_CHIP;
    } else
    {
	memtype = MEMF_KICK;
    }

    for (argc--, argv++; argc; argc--, argv++)
    {
	if (**argv != '-')
	{
	    continue;
	}
	switch(argv[0][1])
	{
	    case 'h':
		PutStr(usage);
		exit(RETURN_OK);
		break;
	    case 'c':
		memtype = MEMF_CHIP;
		break;
	    case 'f':
		memtype = MEMF_FAST;
		break;
	    case 'k':
		memtype = MEMF_KICK;
		break;
	}
    }

    PrintTagPtrs();

    /*
	InternalLoadSeg use.
    */
    ils_table[0] = (ULONG)&ils_read;
    ils_table[1] = (ULONG)&ils_alloc;
    ils_table[2] = (ULONG)&ils_free;

    NewList((struct List *)&ils_mem);

    /*
	Construct a List of filenames to process.
    */
    if(!(filelist = ReadConfig(configfile)))
    {
	PutStr("Error reading config file\n");
	exit(RETURN_ERROR);
    }

    PutStr("Loading modules ...\n");

    NewList((struct List *)&ModuleList);
    ModuleList.ml_Num = 0;

    /*
	Traverse this list and load the modules into memory.
    */
    for(node = filelist->fl_List.lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
	if( (module = LoadModule(node->ln_Name)) )
	{
	    AddTail((struct List *)&ModuleList, (struct Node *)module);
	    ModuleList.ml_Num++;
	    Printf("\t%s\n", (ULONG)module->m_Resident->rt_IdString);
	}
	else
	{
	    node = 0; /* on normal exit, this points to the last module processed */
	    break;
	}
    }

    if(node)
    {
	/*
	    If we're here, all modules we're loaded normally. Build all
	    structures needed for the KickTag/Mem fields, and put them there.
	*/
	if( (BuildTagPtrs(&ModuleList)) )
	{
	    PutStr("\nAll modules loaded successfully, reset to activate.\n\n");
	}
	else
	{
	    FreeModules(&ModuleList);
	}
    }
    else
    {
	PutStr("Error loading one of the modules\n");
	FreeModules(&ModuleList);
    }

    FreeConfig(filelist);

    exit(RETURN_OK);
}

struct Module *LoadModule(char *filename)
{
    BPTR fh, seglist;
    struct Module *mod = 0;

    if(!(fh = Open(filename, MODE_OLDFILE)))
    {
	PrintFault(IoErr(), filename);
	return 0;
    }

    if(!(mod = AllocVec(sizeof(struct Module), MEMF_CLEAR)))
    {
	PutStr("Could not allocate memory for module.\n");
	Close(fh);
	return 0;
    }

    if( (seglist = InternalLoadSeg(fh, NULL, &ils_table[0], &stack)) )
    {
	mod->m_SegList = seglist;
	mod->m_Resident = FindResMod(&ils_mem);
    }

    Close(fh);
    return mod;
}

void FreeModules(struct ModuleList *modlist)
{
    struct Module *mod, *next;

    for(mod = (struct Module *)modlist->ml_List.mlh_Head; mod->m_Node.mln_Succ; mod = next)
    {
	/* get next node here, because after the Remove() it is undefined */
	next = (struct Module *)mod->m_Node.mln_Succ;
	Remove((struct Node *)mod);
	InternalUnLoadSeg(mod->m_SegList, &ils_free);
	FreeVec(mod);
    }
}

BOOL BuildTagPtrs(struct ModuleList *modlist)
{
    struct MemList *memlist;
    ULONG *modarray;
    ULONG i, nummods;
    struct Module *mod;

    /*
	Allocate memory for our KickTagPtr table:
	number of modules loaded + 1 (table terminator)
    */
    if( (modarray = AllocVec((modlist->ml_Num+1)*sizeof(ULONG),
			     memtype|MEMF_CLEAR|MEMF_REVERSE)) )
    {
	/*
	    Fill the KickTagPtr array with pointers to our Resident modules.
	*/
	for(i = 0, mod = (struct Module *)modlist->ml_List.mlh_Head;
	    mod->m_Node.mln_Succ;
	    mod = (struct Module *)mod->m_Node.mln_Succ, i++)
	{
	    modarray[i] = (ULONG)mod->m_Resident;
	}
	/*
	    Terminate the module array.
	*/
	modarray[i+1] = NULL;

	/*
	    Cache number of modules, since it will be modified in BuildMemList.
	*/
	nummods = modlist->ml_Num;

	/*
	    build a memlist to be put in KickMemPtr
	*/
	if( (memlist = BuildMemList(&ils_mem, modarray,
				    (modlist->ml_Num+1)*sizeof(ULONG) )) )
	{
	    StuffTags(memlist, modarray, nummods);
	    return TRUE;
	}
	FreeVec(modarray);
    }
    return FALSE;
}

struct MemList *BuildMemList(struct ilsMemList *prelist,
			     ULONG *		modlist,
			     ULONG 		modlistsize)
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
    if(!(memlist = AllocVec(size, memtype|MEMF_CLEAR|MEMF_REVERSE))) return(NULL);

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
	Put the modlist in the next MemEntry.
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
	    pop a node off the list
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

	Update: this didn't help. I still have to run boot twice to get AROSfA
	loaded for the first time. If AROSfA is already loaded, I can just clear
	the vectors and load boot again, and it will load correctly. Strange.
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
    ULONG num, counter;

    /*
	Search all memory blocks for the Resident struct. Only new nodes
	will be searched.
    */
    for(node = (struct ilsMemNode *)ils_mem.iml_List.mlh_Head, num = ils_mem.iml_NewNum;
	node->imn_Node.mln_Succ || num;
	node = (struct ilsMemNode *)node->imn_Node.mln_Succ, num--)
    {
	/*
	    In each block: skip to the magic word, if present.
	    Searching by word: divide by 2
	*/
	counter = node->imn_Size/2;

	for(ptr = node->imn_Addr; counter; ptr++, counter--)
	{
	    if(RTC_MATCHWORD == *ptr)
	    {
		/*
		    Reset the counter, so that next time only new nodes will
		    be searched.
		*/
		ils_mem.iml_NewNum = 0;

		return((void *)ptr);
	    }
	}
    }

    return 0;
}

void PrintTagPtrs(void)
{
    if(SysBase->KickTagPtr)
    {
	ULONG *list;

	PutStr("Modules already in use:\n");

	list = SysBase->KickTagPtr;

	while(*list)
	{
	    Printf("\t0x%08lx", *list);
	    Printf("\t%s\n", (ULONG)((struct Resident *)*list)->rt_IdString);

	    list++;
	    if(*list & 0x80000000) list = (ULONG *)(*list & 0x7fffffff);
	}
    }
}

/* main.c */
