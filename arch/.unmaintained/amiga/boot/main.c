/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga bootloader -- main file
    Lang: C
*/

/*
 * Note: This whole thing may seem a bit kludgy. It is. It was originally
 * designed to load only one module. It has been extended to read a config
 * file and to load multiple modules. It needs some polishing.
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>

#include "boot.h"
#include "ils.h"
#include "config.h"
#include "version.h"

#define D(x) if (debug) x
#define bug Printf

struct SpecialResident
{
    struct Resident res;
    ULONG magiccookie;
    UBYTE *statusarray;
    UWORD maxslot;
};

#define SR_COOKIE 0x4afa4afb

struct Module *LoadModule(char *);
void FreeModules(struct ModuleList *);
BOOL BuildTagPtrs(struct ModuleList *);
ULONG *AllocMoveCookie(ULONG *, ULONG);
struct MemList *BuildMemList(struct ilsMemList *, ULONG *, ULONG);
void StuffTags(struct MemList *, ULONG *, ULONG);
void *FindResMod(struct ilsMemList *);
void PrintTagPtrs(void);
void FreeRDArgsAll(struct RDArgs *);
void PatchModule(struct Module *, struct List *);

char *version = "$VER: " BANNER " " VERSIONSTRING " " DATE;

ULONG stack = 16384;
ULONG memtype;
BOOL debug, quiet;
ULONG ils_table[3];
struct ilsMemList ils_mem;

char cmlargs[] = "CONFIGFILE,CHIP/S,FAST/S,KICK/S,LOCAL/S,CLEAR/S,CO=CLEARONLY/S,RESET/S,DEBUG/S,SIM=SIMULATE/S,FORCE/S,QUIET/S";

char usage[] =
    "Configfile : config file to use, default is \"arosboot.config\"\n"
    "Chip       : force modules to chip mem\n"
    "Fast       : force modules to fast mem\n"
    "Kick       : force modules to kick mem\n"
    "Local      : force modules to local mem\n"
    "Clear      : clear reset vectors before installing new modules\n"
    "Clearonly  : only clear reset vectors and exit\n"
    "Reset      : reset after installing new modules\n"
    "Debug      : output debugging information\n"
    "Simulate   : Simulate loading, do not actually install modules\n"
    "Force      : Force loading if modules were already loaded\n"
    "Quiet      : Be quiet [debug mode will override this]\n"
    "\n"
    "Order of precedence of memory: kick->local->fast->chip\n"
    "\n"
    "Enter options";

#define CML_CONFIGFILE 0
#define CML_CHIP       1
#define CML_FAST       2
#define CML_KICK       3
#define CML_LOCAL      4
#define CML_CLEAR      5
#define CML_CLEARONLY  6
#define CML_RESET      7
#define CML_DEBUG      8
#define CML_SIMULATE   9
#define CML_FORCE      10
#define CML_QUIET      11
#define CML_END        12

LONG cmdvec[CML_END];

/*
rescookie:
        dc.w    $4afc
        dc.l    rescookie-*
        dc.l    endskip-*
        dc.b    0       ; flags
        dc.b    41      ; version
        dc.b    0       ; NT_UNKNOWN
        dc.b    -120    ; priority
        dc.l    name-*
        dc.l    id-*
        dc.l    init-*
name:   dc.b    'arosboot.cookie',0
id:     dc.b    'arosboot.cookie 41.1 (22.3.1997)',0
        even
init:   moveq.l #0,d0
        rts
endskip:

    Relocate this by adding a field's contents to its address.
    rt_MatchTag also contains a (negative) offset, but unsigned pointers
    won't handle it correctly. Just set it to the base address.
*/
ULONG rescookie[] = {
    0x4AFCFFFF, 0xFFFE0000, 0x004A0029, 0x00880000,
    0x000C0000, 0x00180000, 0x00366172, 0x6F73626F,
    0x6F742E63, 0x6F6F6B69, 0x65006172, 0x6F73626F,
    0x6F742E63, 0x6F6F6B69, 0x65203431, 0x2E312028,
    0x32322E33, 0x2E313939, 0x37290000, 0x70004E75
};

int main(int argc, char **argv)
{
    struct BootConfig *config;
    struct ModNode *modnode;
    struct Module *module;
    STRPTR modname;
    struct RDArgs *rdargs;
    int returnvalue = RETURN_OK;
    BPTR oldcurrentdir, programdir;
    struct ModuleList ModuleList;
    BOOL reset, simulate, force;

    debug = quiet = reset = simulate = force = FALSE;

    if(SysBase->LibNode.lib_Version < 37)
    {
	PutStr("This program is for AmigaOS 2.04 (V37) and higher.\n\n");
	exit(RETURN_FAIL);
    }

    if(!(SysBase->AttnFlags & AFF_68020))
    {
	PutStr("The current AROS for 68k Amigas requires a 68020 or higher processor.\n\n");
	exit(RETURN_FAIL);
    }

    /*
     * The memory type MEMF_KICK has been added in V39 exec. Fall back to
     * MEMF_CHIP if we are on an earlier version.
     */
    if(SysBase->LibNode.lib_Version < 39)
    {
	memtype = MEMF_CHIP;
    } else
    {
	memtype = MEMF_KICK;
    }

    if( (rdargs = AllocDosObject(DOS_RDARGS, NULL)))
    {
	/* set default config file name */
	cmdvec[CML_CONFIGFILE] = (LONG)"arosboot.config";

	rdargs->RDA_ExtHelp = usage; /* FIX: why doesn't this work? */
	rdargs->RDA_Buffer = NULL;
	rdargs->RDA_BufSiz= 0;

	if(!(ReadArgs(cmlargs, cmdvec, rdargs)))
	{
	    PrintFault(IoErr(), "arosboot");
	    FreeDosObject(DOS_RDARGS, rdargs);
	    exit(RETURN_FAIL);
	}

    }
    else
    {
	PrintFault(ERROR_NO_FREE_STORE, "arosboot");
	exit(RETURN_FAIL);
    }

    if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    {
	FreeRDArgsAll(rdargs);
	PutStr("***Break\n");
	exit(RETURN_WARN);
    }

    if(cmdvec[CML_FORCE]) force = TRUE;
    if(cmdvec[CML_RESET]) reset = TRUE;
    if(cmdvec[CML_QUIET]) quiet = TRUE;
    if(cmdvec[CML_DEBUG])
    {
	debug = TRUE;
	quiet = FALSE;
    }
    if(cmdvec[CML_SIMULATE]) simulate = TRUE;

    if(!quiet) PutStr("\n" BANNER " " VERSIONSTRING "\n\n");

    /* Order of precedence: kick->local->fast->chip */
    if     (cmdvec[CML_KICK]) memtype = MEMF_KICK;
    else if(cmdvec[CML_LOCAL]) memtype = MEMF_LOCAL;
    else if(cmdvec[CML_FAST]) memtype = MEMF_FAST;
    else if(cmdvec[CML_CHIP]) memtype = MEMF_CHIP;

    if(cmdvec[CML_CLEARONLY])
    {
	if(!quiet) PutStr("Clearing reset vectors.\n");
	SysBase->KickMemPtr = NULL;
	SysBase->KickTagPtr = NULL;
	SysBase->KickCheckSum = NULL;
	FreeRDArgsAll(rdargs);
	if(reset)
	{
	    if(!quiet) PutStr("Resetting this machine in about 5 seconds...\n");
	    Delay(250);
	    ColdReboot(); /* never returns */
	}
	exit(RETURN_OK);
    }
    else if(cmdvec[CML_CLEAR]) /* no need to clear again if already done */
    {
	if(!quiet) PutStr("Clearing reset vectors.\n");
	SysBase->KickMemPtr = NULL;
	SysBase->KickTagPtr = NULL;
	SysBase->KickCheckSum = NULL;
    }

    D(bug("Debug mode\n"));

    if(!quiet) PrintTagPtrs();

    /*
     * InternalLoadSeg use.
     */
    ils_table[0] = (ULONG)&ils_read;
    ils_table[1] = (ULONG)&ils_alloc;
    ils_table[2] = (ULONG)&ils_free;

    NewList((struct List *)&ils_mem);

    if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    {
	FreeRDArgsAll(rdargs);
	PutStr("***Break\n");
	exit(RETURN_WARN);
    }

    if( (FindResident("arosboot.cookie")))
    {
	if(!force)
	{
	    if(!quiet)
	    {
		PutStr("AROS modules already loaded."); /* A smart compiler merges strings */
		PutStr(" Exiting.\n");
	    }
	    FreeRDArgsAll(rdargs);
	    exit(RETURN_OK);
	}
	else
	{
	    if(!quiet)
	    {
		PutStr("AROS modules already loaded.");
		PutStr(" Overriding.\n");
	    }
	}
    }

    /*
     * First check if we actually have a program dir (aka PROGDIR:). If we
     * don't have one, we're in the resident list (hey, you shouldn't put
     * us there!), and we stay right in the dir we were started from.
     */
    if( (programdir = GetProgramDir()))
    {
	oldcurrentdir = CurrentDir(programdir);
    }

    /*
     * Construct a List of filenames to process.
     */
    if(!(config = ReadConfig((char *)cmdvec[CML_CONFIGFILE])))
    {
	/* ReadConfig() prints it's own error string, just exit. */
	FreeRDArgsAll(rdargs);

	/* If we switched dirs, switch back. */
	if(oldcurrentdir) CurrentDir(oldcurrentdir);
	exit(RETURN_FAIL);
    }

    /* We no longer need them */
    FreeRDArgsAll(rdargs);

    if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    {
	FreeConfig(config);
	if(oldcurrentdir) CurrentDir(oldcurrentdir);
	PutStr("***Break\n");
	exit(RETURN_WARN);
    }

    if(!quiet) PutStr("Loading modules ...\n");

    NewList((struct List *)&ModuleList);
    ModuleList.ml_Num = 0;

    /*
     * Traverse this list and load the modules into memory.
     */
    for(modnode = (struct ModNode *)config->bc_Modules.lh_Head;
	modnode->mn_Node.ln_Succ;
	modnode = (struct ModNode *)modnode->mn_Node.ln_Succ)
    {
	modname = modnode->mn_Node.ln_Name;
	if( (module = LoadModule(modnode->mn_Node.ln_Name)) )
	{
	    PatchModule(module, &modnode->mn_FuncList);
	    AddTail((struct List *)&ModuleList, (struct Node *)module);
	    ModuleList.ml_Num++;
	    if(!quiet) Printf("\t%s\n", (ULONG)module->m_Resident->rt_IdString);
	}
	else
	{
	    modnode = NULL; /* on normal exit, this points to the last module processed */
	    break;
	}
    }

    /*
     * I duplicated a CTRL-C handler a few times, instead of making it a
     * subroutine. Else all local variables used here would have to be passed
     * to that subroutine. I should put common cleanup code into an atexit()
     * handler. Oh well, maybe next time.
     */
    if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    {
	FreeConfig(config);
	if(oldcurrentdir) CurrentDir(oldcurrentdir);
	PutStr("***Break\n");
	exit(RETURN_WARN);
    }

    if(modnode)
    {
	if(!simulate)
	{
	    /*
	     * If we're here, all modules we're loaded normally. Build all
	     * structures needed for the KickTag/Mem fields, and put them there.
	     */
	    if( (BuildTagPtrs(&ModuleList)) )
	    {
		if(!quiet & !reset) PutStr("\nAll modules loaded successfully, reset to activate.\n\n");
		else if(!quiet & reset) PutStr("\nAll modules loaded successfully.\n\n");
	    }
	    else
	    {
		if(!quiet) PutStr("\nError building KickTagPtrs!\n");
		FreeModules(&ModuleList);
		returnvalue = RETURN_FAIL;
	    }
	}
	else
	{
		if(!quiet) PutStr("Simulation complete, unloading modules.\n");
		FreeModules(&ModuleList);
		returnvalue = RETURN_OK;
	}
    }
    else
    {
	if(!quiet) Printf("Error loading \"%s\"\n", (ULONG)modname);
	FreeModules(&ModuleList);
	returnvalue = RETURN_FAIL;
    }

    FreeConfig(config);

    if(reset)
    {
	if(!quiet) PutStr("Resetting this machine in about 5 seconds...\n");
	Delay(250);
	ColdReboot(); /* never returns */
    }

    /* If we switched dirs, switch back. */
    if(oldcurrentdir) CurrentDir(oldcurrentdir);

    exit(returnvalue);
}

struct Module *LoadModule(char *filename)
{
    BPTR fh, seglist;
    struct Module *mod;

    D(bug("LoadModule(\"%s\")\n", (ULONG)filename));
    if(!(fh = Open(filename, MODE_OLDFILE)))
    {
	if(!quiet) PrintFault(IoErr(), filename);
	return 0;
    }

    if(!(mod = AllocVec(sizeof(struct Module), MEMF_CLEAR)))
    {
	if(!quiet) PutStr("Could not allocate memory for module.\n");
	Close(fh);
	return 0;
    }
    D(bug("  module   = $%08lx (size %ld)\n", (ULONG)mod, sizeof(struct Module)));

    if( (seglist = InternalLoadSeg(fh, NULL, &ils_table[0], &stack)) )
    {
	mod->m_SegList = seglist;
	mod->m_Resident = FindResMod(&ils_mem);
	if(debug)
	{
	    if( ((struct SpecialResident *)mod->m_Resident)->magiccookie == SR_COOKIE)
		Printf("  Module is of patchable type\n");
	}
    }
    Close(fh);
    D(bug("  SegList  = $%08lx\n", BADDR(mod->m_SegList)));
    D(bug("  Resident = $%08lx\n", (ULONG)mod->m_Resident));

    /*
     * If this module contains a Resident structure, return it.
     */
    if ( (mod->m_Resident))
    {
	return mod;
    }

    return 0;
}

void FreeModules(struct ModuleList *modlist)
{
    struct Module *mod, *next;

    D(bug("FreeModules($%08lx)\n", (ULONG)modlist));

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
    ULONG *cookieptr;
    struct ilsMemNode *cookienode;

    D(bug("BuildTagPtrs...\n"));

    /*
     * Allocate memory for our KickTagPtr table:
     * number of modules loaded +2 (cookie and table terminator)
     */
    D(bug("Allocating KickTagPtr memory (size %ld)\n", (modlist->ml_Num+2)*sizeof(ULONG)));
    if( (modarray = AllocVec((modlist->ml_Num+2)*sizeof(ULONG),
			     memtype|MEMF_CLEAR|MEMF_REVERSE)) )
    {
	D(bug(" ok\n"));
	/*
	 * Fill the KickTagPtr array with pointers to our Resident modules.
	 */
	for(i = 0, mod = (struct Module *)modlist->ml_List.mlh_Head;
	    mod->m_Node.mln_Succ;
	    mod = (struct Module *)mod->m_Node.mln_Succ, i++)
	{
	    D(bug("  modarray[%ld] = $%lx (module)\n", i, (ULONG)mod->m_Resident));
	    modarray[i] = (ULONG)mod->m_Resident;
	}
	/* i will be incremented one last time before exiting */

	if( (cookieptr = AllocMoveCookie(rescookie, sizeof(rescookie))))
	{
	    /*
	     * Add the cookie module.
	     */
	    D(bug("  modarray[%ld] = $%lx (cookie)\n", i, (ULONG)cookieptr));
	    modarray[i] = (ULONG)cookieptr;

	    /*
	     * Terminate the module array.
	     */
	    D(bug("  modarray[%ld] = $%lx (termination)\n", i+1, (ULONG)NULL));
	    modarray[i+1] = NULL;

	    /*
	     * Add the cookie's memory address + size on the memlist.
	     */
	    if( (cookienode = AllocMem(sizeof(struct ilsMemNode), MEMF_CLEAR)) )
	    {
		cookienode->imn_Addr = cookieptr;
		cookienode->imn_Size = sizeof(rescookie);
		AddHead((struct List *)&ils_mem, (struct Node *)cookienode);
		ils_mem.iml_Num++;
		ils_mem.iml_NewNum++;

		/*
		 * Cache number of modules, since it will be modified in
		 * BuildMemList. +1 because of the cookie.
		 */
		nummods = modlist->ml_Num + 1;

		/*
		 * build a memlist to be put in KickMemPtr
		 */
		if( (memlist = BuildMemList(&ils_mem, modarray,
		    (modlist->ml_Num+2)*sizeof(ULONG) )) )
		{
		    StuffTags(memlist, modarray, nummods);
		    return TRUE;
		}

	    }
	    /* If we get here, something went wrong. Free stuff. */
	    FreeMem(cookieptr, sizeof(rescookie));
	}
	FreeVec(modarray);
    }
    return FALSE;
}

ULONG *AllocMoveCookie(ULONG *cookie, ULONG size)
{
    struct Resident *res;

    D(bug("AllocMoveCookie()\n"));

    if( (res = AllocMem(size, memtype|MEMF_REVERSE)))
    {
	D(bug("  Moving cookie\n"));
	/* Relocate the cookie. */
	CopyMem(cookie, (APTR)res, size);

	D(bug("  Relocating cookie pointers\n"));
	/* Relocate pointers in cookie. */
	res->rt_MatchTag  = res;
	res->rt_EndSkip  += (ULONG)&res->rt_EndSkip;
	res->rt_Name     += (ULONG)&res->rt_Name;
	res->rt_IdString += (ULONG)&res->rt_IdString;
	res->rt_Init     += (ULONG)&res->rt_Init;

	D(bug("  cookie @ $%lx, name @ $%lx, idstring @ $%lx\n",
	    (ULONG)res, (ULONG)res->rt_Name, (ULONG)res->rt_IdString));

	if(!quiet) Printf("\t%s\n", (ULONG)res->rt_IdString);

	return (ULONG *)res;
    }

    return 0;
}

struct MemList *BuildMemList(struct ilsMemList *prelist,
			     ULONG *		modlist,
			     ULONG 		modlistsize)
{
    struct MemList *memlist;
    struct MemEntry *me;
    struct ilsMemNode *node;
    UWORD numentries;
    ULONG size;

    D(bug("BuildMemList()\n"));
    /*
     * Calculate needed number of extra MemEntry fields that need to be
     * appended to the end of our MemList structure. This number+1 (there
     * already is one MemEntry in the MemList) is what we'll be putting in
     * memlist->ml_NumEntries later.
     */
    numentries = prelist->iml_Num + 1;
    D(bug(" number of mem areas = %ld\n", numentries+1));

    size = ( sizeof(struct MemList) + (sizeof(struct MemEntry) * numentries) );

    /*
     * Allocate memory from the top of the memory list, to keep fragmentation
     * down, and hopefully to keep all our applications in one place.
     */
    if(!(memlist = AllocVec(size, memtype|MEMF_CLEAR|MEMF_REVERSE))) return(NULL);

    /*
     * We have the memory. Assure that this MemList is allocated during a
     * reset, and put it in the first MemEntry field.
     */
    memlist->ml_ME[0].me_Addr = memlist;
    memlist->ml_ME[0].me_Length = size;

    /*
     * Indicate that this node is part of a KickMem MemList, and indicate how
     * many MemEntries there are.
     */
    memlist->ml_Node.ln_Type = NT_KICKMEM;
    memlist->ml_NumEntries = numentries+1;

    /*
     * Put the modlist in the next MemEntry.
     */
    me = (struct MemEntry *)( ((ULONG)memlist) + sizeof(struct MemList) );
    me->me_Addr = modlist;
    me->me_Length = modlistsize;

    /*
     * Increase to the next MemEntry.
     */
    me = (struct MemEntry *)( ((ULONG)me) + sizeof(struct MemEntry) );

    /*
     * Now put the data from our prelist in the remaining MemEntry fields,
     * removing the nodes in our prelist and freeing them in the process.
     */
    while(prelist->iml_Num)
    {
	/*
	 * pop a node off the list
	 */
	node = (struct ilsMemNode *)RemHead((struct List *)prelist);

	me->me_Addr = node->imn_Addr;
	me->me_Length = node->imn_Size;

	FreeMem(node, sizeof(struct ilsMemNode));

	/*
	 * Point to the next MemEntry, decrease our counter.
	 */
	me = (struct MemEntry *)(((ULONG)me) + sizeof(struct MemEntry));
	prelist->iml_Num--;
    }

    return memlist;
}

void StuffTags(struct MemList *memlist, ULONG *modlist, ULONG nummods)
{
    D(bug("StuffTags(memlist $%08lx  modlist $%08lx  nummods %ld)\n",
     (ULONG)memlist, (ULONG)modlist, nummods));

    Forbid();

    if(SysBase->KickCheckSum != (APTR)SumKickData())
    {
	/* Vectors had incorrect checksum. Just clear them now. */
	D(bug("Vectors had incorrect checksum. Clearing before loading.\n"));
	SysBase->KickTagPtr = SysBase->KickMemPtr = SysBase->KickCheckSum = NULL;
    }

    if(SysBase->KickMemPtr)
    {
	/*
	 * Prepend to an existing chain of MemLists.
	 */
	D(bug("Prepending to existing memlist (old ptr = $%lx)\n", (ULONG)SysBase->KickMemPtr));
	memlist->ml_Node.ln_Succ = SysBase->KickMemPtr;
    }
    SysBase->KickMemPtr = memlist;

    if(SysBase->KickTagPtr)
    {
	/*
	 * Prepend to an existing chain of module lists.
	 */
	D(bug("Prepending to existing tagptr (old ptr = $%lx)\n", (ULONG)SysBase->KickTagPtr));
	modlist[nummods] = (ULONG)SysBase->KickTagPtr | 0x80000000;
    }
    SysBase->KickTagPtr = modlist;

    /*
     * And checksum the KickPtrs. KickCheckSum is defined wrong in the include
     * file exec/execbase.h. It is defined as APTR, but really is a ULONG.
     */
    SysBase->KickCheckSum = (APTR)SumKickData();

    /*
     * Flush caches.
     */
    CacheClearU();

    Permit();
}

void *FindResMod(struct ilsMemList *list)
{
    struct ilsMemNode *node;
    UWORD *ptr;
    ULONG num, counter;

    D(bug("Finding Resident Module..."));

    /*
     * Search all memory blocks for the Resident struct. Only new nodes
     * will be searched.
     */
    for(node = (struct ilsMemNode *)ils_mem.iml_List.mlh_Head, num = ils_mem.iml_NewNum;
	node->imn_Node.mln_Succ || num;
	node = (struct ilsMemNode *)node->imn_Node.mln_Succ, num--)
    {
	/*
	 * In each block: skip to the magic word, if present.
	 * Searching by word: divide by 2
	 */
	counter = node->imn_Size/2;

	for(ptr = node->imn_Addr; counter; ptr++, counter--)
	{
	    if(RTC_MATCHWORD == *ptr)
	    {
		/*
		 * Reset the counter, so that next time only new nodes will
		 * be searched.
		 */
		ils_mem.iml_NewNum = 0;

		D(bug(" at $%08lx\n", (ULONG)ptr));
		return((void *)ptr);
	    }
	}
    }

    D(bug(" not found\n"));
    return 0;
}

void PrintTagPtrs(void)
{
    if(SumKickData() == (ULONG)SysBase->KickCheckSum)
    {
	if(SysBase->KickTagPtr)
	{
	    ULONG *list;

	    PutStr("Modules already in use:\n");

	    list = SysBase->KickTagPtr;

	    while(*list)
	    {
		Printf("\t$%08lx", *list);
		Printf("\t%s\n", (ULONG)((struct Resident *)*list)->rt_IdString);

		list++;
		if(*list & 0x80000000) list = (ULONG *)(*list & 0x7fffffff);
	    }
	}
    }
    else
    {
	/* Don't print message if all vectors are NULL */
	if(SysBase->KickMemPtr || SysBase->KickTagPtr || SysBase->KickCheckSum)
	    Printf("Vectors have incorrect checksum.\n");
    }
}

void FreeRDArgsAll(struct RDArgs *rdargs)
{
    FreeArgs(rdargs);
    FreeDosObject(DOS_RDARGS, rdargs);
}

/* Turn functions on/off in module according to the function slots in funclist */
void PatchModule(struct Module *module, struct List *funclist)
{
    struct FuncNode *funcnode;
    UBYTE *array;
    UWORD maxslot;

    if( ((struct SpecialResident *)module->m_Resident)->magiccookie == SR_COOKIE)
    {
	/* ok, cookie found, let's patch this module */

	array = ((struct SpecialResident *)module->m_Resident)->statusarray;
	maxslot = ((struct SpecialResident *)module->m_Resident)->maxslot;

	for(funcnode = (struct FuncNode *)funclist->lh_Head;
	    funcnode->fn_Node.ln_Succ;
	    funcnode = (struct FuncNode *)funcnode->fn_Node.ln_Succ)
	{
	    /* walk function list, (un)setting entries in array */

	    /* Check if in range of library, do not touch first 4 vectors */
	    if(funcnode->fn_Slot <= maxslot && funcnode->fn_Slot > 4)
	    {
		D(bug("Setting function %ld (slot %ld) %s\n",
		    funcnode->fn_Slot * -6,
		    funcnode->fn_Slot,
		    funcnode->fn_Status ? (ULONG)"on" : (ULONG)"off"
		));

		array[funcnode->fn_Slot] = funcnode->fn_Status ? 1 : 0;
	    }
	    else
	    {
		if(!quiet)
		    Printf("Function %ld (slot %ld) outside scope of module (max %ld, %ld). Ignored.\n",
			funcnode->fn_Slot * -6,
			funcnode->fn_Slot,
			maxslot * -6,
			maxslot
		    );
	    }
	}
    }

    return;
}

/* main.c */
