/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

/* It's like doing avail flush before and after executing a program to detect
 * memleaks, excepted that this program tries to detect much more leaks.
 * It has to not leak itself, and not even allocate memory between checks.
 * That's why lists and nodes are statically allocated (in a system that could
 * be improved to allow reuse of nodes :)
 *
 * Use : launch in a new shell, and use ctrl-f whenever you want to check
 * for leak.
 * See the Aminet "Scout" program to see other things to track in system lists.
 */


#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include <string.h>

static const char version[] = "$VER: LeakWatch 0.1 (25.12.2002)\n";

/* struct arrays : ORN = OpenedResourceNode, TR = TrackedResources, RD = ResourceDiff
 */
#define MAX_ORN 10000
#define MAX_TR 1000
#define MAX_RD 100
#define MAX_STR 100000
#define MAX_MR 1000
#define MAX_NR 1000

struct OpenedResourceNode {
    struct Node  node;
    CONST_STRPTR name;
    APTR         addr;
    ULONG        count;
};

struct TrackedResources {
    struct List opened;
    ULONG       freeMem;
};


struct ModifiedResource {
    struct Node node;
    struct OpenedResourceNode *before;
    struct OpenedResourceNode *after;
};

struct NewResource {
    struct Node node;
    struct OpenedResourceNode *orn;
};



/* store eg. new libraries, or libraries with modified opencount
 *
 */
struct ResourceDiff {
    LONG memLost;
    struct List newOpened;       /* contains NewResource */
    struct List modifiedOpened;  /* contains ModifiedResource */
};

static struct OpenedResourceNode _ornbuf[MAX_ORN];
static struct TrackedResources _trbuf[MAX_TR];
static struct ResourceDiff _rdbuf[MAX_RD];
static UBYTE _strbuf[MAX_STR];
static struct ModifiedResource _mrbuf[MAX_MR];
static struct NewResource _nrbuf[MAX_NR];

static int next_orn = 0;
static int next_tr = 0; 
static int next_rd = 0; 
static int next_str = 0; 
static int next_mr = 0; 
static int next_nr = 0; 

static struct OpenedResourceNode *get_orn()
{
    if (next_orn == MAX_ORN)
	return NULL;
    return &_ornbuf[next_orn++];
}

static void release_orn (struct OpenedResourceNode *orn)
{
}

static struct TrackedResources *get_tr()
{
    if (next_tr == MAX_TR)
	return NULL;
    return &_trbuf[next_tr++];
}

static void release_tr (struct TrackedResources *tr)
{
}

static struct ResourceDiff *get_rd()
{
    if (next_rd == MAX_RD)
	return NULL;
    return &_rdbuf[next_rd++];
}

static void release_rd (struct ResourceDiff *rd)
{
}

static struct ModifiedResource *get_mr()
{
    if (next_mr == MAX_MR)
	return NULL;
    return &_mrbuf[next_mr++];
}

static void release_mr (struct ModifiedResource *mr)
{
}

static struct NewResource *get_nr()
{
    if (next_nr == MAX_NR)
	return NULL;
    return &_nrbuf[next_nr++];
}

static void release_nr (struct NewResource *nr)
{
}


CONST_STRPTR StaticStrDup (CONST_STRPTR str)
{   
    UBYTE *start = &_strbuf[next_str];
    UBYTE *t = start;
    int len;

    len = strlen(str);
    if (len + next_str + 1 > MAX_STR)
	return NULL;

    while ((*t++ = *str++))
	;
    next_str += t - start;
    return (CONST_STRPTR)start;
}

void StrFree (CONST_STRPTR str)
{
}

/***********************************************************************/

static struct TrackedResources *NewResourcesState(void);
static void DeleteResourcesState(struct TrackedResources *rs);
static struct ResourceDiff *NewStateDiff(const struct TrackedResources *old,
					  const struct TrackedResources *new);
static void DisplayStateDiff(const struct ResourceDiff *rd);
static void DeleteStateDiff(struct ResourceDiff *rd);
static struct TrackedResources * CopyResourcesState(const struct TrackedResources *src);


static BOOL AddLibs(struct List *opened)
{
    struct Library *lib;

    Forbid();
    for(lib=(struct Library *)SysBase->LibList.lh_Head;
        lib->lib_Node.ln_Succ!=NULL;
        lib=(struct Library *)lib->lib_Node.ln_Succ)
    {
	struct OpenedResourceNode *orn = get_orn();
	if (!orn)
	{
	    Permit();
	    return FALSE;
	}
	orn->name = StaticStrDup(lib->lib_Node.ln_Name);
	orn->addr = lib;
	orn->count = lib->lib_OpenCnt;
	Enqueue(opened, (struct Node *)orn);
    }
    Permit();
    return TRUE;
}

static BOOL AddDevs(struct List *opened)
{
    struct Device *dev;

    Forbid();
    for(dev=(struct Device *)SysBase->DeviceList.lh_Head;
        dev->dd_Library.lib_Node.ln_Succ!=NULL;
        dev=(struct Device *)dev->dd_Library.lib_Node.ln_Succ)
    {
	struct OpenedResourceNode *orn = get_orn();
	if (!orn)
	{
	    Permit();
	    return FALSE;
	}
	orn->name = StaticStrDup(dev->dd_Library.lib_Node.ln_Name);
	orn->addr = dev;
	orn->count = dev->dd_Library.lib_OpenCnt;
	Enqueue(opened, (struct Node *)orn);
    }
    Permit();
    return TRUE;
}

static BOOL AddFonts(struct List *opened)
{
    struct TextFont *tf;

    Forbid();
    for(tf=(struct TextFont *)GfxBase->TextFonts.lh_Head;
        tf->tf_Message.mn_Node.ln_Succ!=NULL;
        tf=(struct TextFont *)tf->tf_Message.mn_Node.ln_Succ)
    {
	struct OpenedResourceNode *orn = get_orn();
	if (!orn)
	{
	    Permit();
	    return FALSE;
	}
	orn->name = StaticStrDup(tf->tf_Message.mn_Node.ln_Name);
	orn->addr = tf;
	orn->count = tf->tf_Accessors;
	Enqueue(opened, (struct Node *)orn);
    }
    Permit();
    return TRUE;
}


static BOOL AddOpenedResources(struct List *opened)
{
    if (!AddLibs(opened))
	return FALSE;
    if (!AddDevs(opened))
	return FALSE;
    if (!AddFonts(opened))
	return FALSE;
    return TRUE;
}


static struct TrackedResources *NewResourcesState(void)
{
    struct TrackedResources *tr;

    tr = get_tr();
    if (!tr)
	return NULL;

    /* flush */
    FreeVec(AllocVec(~0ul/2, MEMF_CHIP));

    /* opencount-based stuff */
    NEWLIST(&tr->opened);
    AddOpenedResources(&tr->opened);

    /* memory */
    tr->freeMem = AvailMem(MEMF_ANY);

    return tr;
}

static void DeleteResourceNode(struct OpenedResourceNode *orn)
{
    if (!orn)
	return;
    StrFree((APTR)orn->name);
    orn->name = NULL;
    release_orn(orn);
}

static void DeleteResourcesState(struct TrackedResources *rs)
{
    struct OpenedResourceNode *orn;
    struct OpenedResourceNode *tmp;

    if (!rs)
	return;

    for(orn=(struct OpenedResourceNode *)rs->opened.lh_Head;
        orn->node.ln_Succ!=NULL;
        orn=tmp)
    {
	tmp = (struct OpenedResourceNode *)orn->node.ln_Succ;
	Remove((struct Node *)orn);
	DeleteResourceNode(orn);
    }
    release_tr(rs);
}

void DisplayResourcesState(const struct TrackedResources *rs)
{
    /* FIXME */
    struct OpenedResourceNode *orn;

    if (!rs)
	return;

    for(orn=(struct OpenedResourceNode *)rs->opened.lh_Head;
        orn->node.ln_Succ!=NULL;
        orn=(struct OpenedResourceNode *)orn->node.ln_Succ)
    {
	FPuts(Output(), (STRPTR)orn->name);
	FPuts(Output(), "\n");
    }

    
}


/* the ResourceDiff can have dangling pointers in old and nu, so dont clear
 * them before being done with rd in the processing loop
 */
static struct ResourceDiff *NewStateDiff(const struct TrackedResources *old,
					  const struct TrackedResources *nu)
{
    /* FIXME */
    struct OpenedResourceNode *orn;
    struct ResourceDiff *rd;

    rd = get_rd();
    if (!rd)
	return NULL;

    NEWLIST(&rd->newOpened);
    NEWLIST(&rd->modifiedOpened);

    for(orn=(struct OpenedResourceNode *)nu->opened.lh_Head;
        orn->node.ln_Succ!=NULL;
        orn=(struct OpenedResourceNode *)orn->node.ln_Succ)
    {
	struct OpenedResourceNode *other;
	BOOL seen = FALSE;

	for(other=(struct OpenedResourceNode *)old->opened.lh_Head;
	    other->node.ln_Succ!=NULL;
	    other=(struct OpenedResourceNode *)other->node.ln_Succ)
	{
	    if (orn->addr == other->addr)
	    {
		if (!strcmp(orn->name, other->name))
		{
		    seen = TRUE;
		    if (orn->count != other->count)
		    {
			struct ModifiedResource *mr = get_mr();
			if (!mr)
			    return NULL;
			mr->before = other;
			mr->after = orn;
			Enqueue(&rd->modifiedOpened, (struct Node *)mr);
		    }
		}
	    }
	}
	if (!seen)
	{
	    struct NewResource *nr = get_nr();
	    if (!nr)
		return NULL;
	    nr->orn = orn;
	    Enqueue(&rd->newOpened, (struct Node *)nr);
	}
    }


    rd->memLost = old->freeMem - nu->freeMem;

    return rd;
}

static void DisplayStateDiff(const struct ResourceDiff *rd)
{
    /* FIXME */
    IPTR mem[2];
    IPTR modified[4];
    IPTR newv[3];
    struct ModifiedResource *mr;
    struct NewResource *nr;

    FPuts(Output(), "LeakWatch report:\n");
    mem[0] = rd->memLost;
    mem[1] = (IPTR)((rd->memLost > 1) ? "s" : "");
    VFPrintf(Output(), " Memory lost : %ld byte%s\n", mem);

    FPuts(Output(), " Open count:\n");
    for(mr=(struct ModifiedResource *)rd->modifiedOpened.lh_Head;
	mr->node.ln_Succ!=NULL;
	mr=(struct ModifiedResource *)mr->node.ln_Succ)
    {
	modified[0] = (IPTR)mr->before->name;
	modified[1] = (IPTR)mr->before->addr;
	modified[2] = mr->before->count;
	modified[3] = mr->after->count;
	
	VFPrintf(Output(), "  %s (0x%lx) : %ld -> %d\n", modified);
    }

    FPuts(Output(), " Newly opened:\n");
    for(nr=(struct NewResource *)rd->newOpened.lh_Head;
	nr->node.ln_Succ!=NULL;
	nr=(struct NewResource *)nr->node.ln_Succ)
    {
	newv[0] = (IPTR)nr->orn->name;
	newv[1] = (IPTR)nr->orn->addr;
	newv[2] = nr->orn->count;
	VFPrintf(Output(), "  %s (0x%lx) : %ld\n", newv);
    }

    FPuts(Output(), "--\n");
    
}

static void DeleteStateDiff(struct ResourceDiff *rd)
{
    /* FIXME */
    struct ModifiedResource *mr;
    struct ModifiedResource *tmpmr;
    struct NewResource *nr;
    struct NewResource *tmpnr;

    if (!rd)
	return;

    for(mr=(struct ModifiedResource *)rd->modifiedOpened.lh_Head;
        mr->node.ln_Succ!=NULL;
        mr=tmpmr)
    {
	tmpmr = (struct ModifiedResource *)mr->node.ln_Succ;
	Remove((struct Node *)mr);
	
	release_mr(mr);
    }
    
    for(nr=(struct NewResource *)rd->newOpened.lh_Head;
        nr->node.ln_Succ!=NULL;
        nr=tmpnr)
    {
	tmpnr = (struct NewResource *)nr->node.ln_Succ;
	Remove((struct Node *)nr);
	
	release_nr(nr);
    }

    release_rd(rd);
}

static struct OpenedResourceNode * CopyResourcesNode(const struct OpenedResourceNode *src)
{
    struct OpenedResourceNode *orn = get_orn();
    if (!orn)
	return NULL;
    orn->name = StaticStrDup(src->name);
    orn->addr = src->addr;
    orn->count = src->count;
    return orn;
}

static struct TrackedResources * CopyResourcesState(const struct TrackedResources *src)
{
    struct TrackedResources *tr;
    struct OpenedResourceNode *orn;

    tr = get_tr();
    if (!tr)
	return NULL;

    /* opencount-based stuff */
    NEWLIST(&tr->opened);

    for(orn=(struct OpenedResourceNode *)src->opened.lh_Head;
        orn->node.ln_Succ!=NULL;
        orn=(struct OpenedResourceNode *)orn->node.ln_Succ)
    {
	struct OpenedResourceNode *nc;

	nc = CopyResourcesNode(orn);
	Enqueue(&tr->opened, (struct Node *)nc);
    }

    /* memory */
    tr->freeMem = src->freeMem;

    return tr;
}

int __nocommandline;

int main(void)
{
    struct TrackedResources *crs = NULL;
    struct TrackedResources *start_rs = NULL;
    BOOL quitme = FALSE;

    FPuts(Output(), "LeakWatch running, CTRL-C to exit, CTRL-E to watch for leaks since beginning, CTRL-F to watch for leaks since last CTRL-F\n");
    crs = NewResourcesState();
    start_rs = CopyResourcesState(crs);

    while(!quitme)
    {
	ULONG signals;

	signals = Wait(SIGBREAKF_CTRL_F | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_C);
	if (signals & SIGBREAKF_CTRL_E)
	{
	    struct ResourceDiff *rd = NULL;

	    DeleteResourcesState(crs);
	    crs = NewResourcesState();
	    /*  DisplayResourcesState(crs); */ /* only for debug */
	    rd = NewStateDiff(start_rs, crs);
	    DisplayStateDiff(rd);
	    DeleteStateDiff(rd);
	}
	if (signals & SIGBREAKF_CTRL_F)
	{
	    struct TrackedResources *ors = crs;
	    struct ResourceDiff *rd = NULL;

	    crs = NewResourcesState();
	    rd = NewStateDiff(ors, crs);
	    DisplayStateDiff(rd);
	    DeleteStateDiff(rd);
	    DeleteResourcesState(ors);
	}
	if (signals & SIGBREAKF_CTRL_C)
	{
	    quitme = TRUE;
	}
    } /* while(!quitme) */

    DeleteResourcesState(crs);
    DeleteResourcesState(start_rs);

    return 0;
}
