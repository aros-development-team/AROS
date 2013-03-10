/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: show deltas of resources usage
    Lang: english
*/

/* It's like doing avail flush before and after executing a program to detect
 * memleaks, excepted that this program tries to detect much more leaks.
 * It has to not leak itself, and not even allocate memory between checks.
 * That's why lists and nodes are statically allocated (in a system that could
 * be improved to allow reuse of nodes :) (like using an Allocate-based own
 * memory allocator using a huge static array as heap)
 *
 * Use : launch in a new shell, and use ctrl-f whenever you want to check
 * for leak.
 * See the Aminet "Scout" program to see other things to track in system lists.
 */


#include <exec/memory.h>
#include <exec/tasks.h>
#ifndef EXEC_SEMAPHORES
#   include <exec/semaphores.h>
#endif
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

const TEXT version[] = "$VER: LeakWatch 0.2 (10.03.2013)\n";

static struct Library *GadToolsBase;

struct GadToolsBase_intern
{
    struct Library                lib;

    Class 			* buttonclass;
    Class 			* textclass;
    Class 			* sliderclass;
    Class 			* scrollerclass;
    Class 			* arrowclass;
    Class 			* stringclass;
    Class 			* listviewclass;
    Class 			* checkboxclass;
    Class 			* cycleclass;
    Class 			* mxclass;
    Class 			* paletteclass;
    
    /* Semaphore to protect the bevel object. */
    struct SignalSemaphore   	bevelsema;
    /* Actually an Object *. The image used for bevel boxes. */
    struct Image           	* bevel;
    
    /* RenderHook for GTListView class */
    struct Hook                 lv_RenderHook;

    /* Seglist pointer */
    BPTR			gt_SegList;

    /* Required libraies */
    APTR			gt_IntuitionBase;
    APTR			gt_UtilityBase;
    APTR			gt_GfxBase;
    APTR			gt_LayersBase;
};



/* struct arrays : ORN = OpenedResourceNode, TR = TrackedResources, RD = ResourceDiff
   MR = ModifiedResource STR=stringbuf
 */
#define MAX_ORN  30000
#define MAX_TR    3000
#define MAX_RD     300
#define MAX_STR 300000
#define MAX_MR    2000

/* All resources that have an opencount, eg. libs, devs, fonts */
struct OpenedResourceNode {
    struct Node  node;
    CONST_STRPTR type;
    CONST_STRPTR name;
    APTR         addr;
    ULONG        count;
};

/* All resources that LeakWatch handles. */
struct TrackedResources {
    struct List opened;   /* List of OpenedResourceNode */
    ULONG       freeMem;  /* total free memory */
};

struct ModifiedResource {
    struct Node  node;
    CONST_STRPTR type;
    CONST_STRPTR name;
    APTR         addr;
    ULONG        before_count;
    ULONG        after_count;
};

/* store eg. new libraries, or libraries with modified opencount
 *
 */
struct ResourceDiff {
    LONG memLost;
    struct List modifiedOpened;  /* contains ModifiedResource */
};

/* static storage to avoid interfering with memleaks debugging */
static struct OpenedResourceNode _ornbuf[MAX_ORN];
static struct TrackedResources _trbuf[MAX_TR];
static struct ResourceDiff _rdbuf[MAX_RD];
static UBYTE _strbuf[MAX_STR];
static struct ModifiedResource _mrbuf[MAX_MR];

static int next_orn = 0;
static int next_tr = 0; 
static int next_rd = 0; 
static int next_str = 0; 
static int next_mr = 0; 

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

CONST_STRPTR StaticStrDup (CONST_STRPTR str)
{   
    UBYTE *start = &_strbuf[next_str];
    UBYTE *t = start;
    int len;

    if (NULL == str)
	str = "<unnamed>";

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
static void DisplayStateDiff(const struct ResourceDiff *rd, int pagelines);
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
	orn->type = "Library";
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
	orn->type = "Device";
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
	orn->type = "Font";
	orn->name = StaticStrDup(tf->tf_Message.mn_Node.ln_Name);
	orn->addr = tf;
	orn->count = tf->tf_Accessors;
	Enqueue(opened, (struct Node *)orn);
    }
    Permit();
    return TRUE;
}

static BOOL AddNodeNames(struct List *opened, struct List *list, CONST_STRPTR type)
{
    struct Node *lib;

    Forbid();
    for(lib=(struct Node *)list->lh_Head;
        lib->ln_Succ!=NULL;
        lib=(struct Node *)lib->ln_Succ)
    {
	struct OpenedResourceNode *orn = get_orn();
	if (!orn)
	{
	    Permit();
	    return FALSE;
	}
	orn->type = type;
	orn->name = StaticStrDup(lib->ln_Name);
	orn->addr = lib;
	orn->count = 0;
	Enqueue(opened, (struct Node *)orn);
    }
    Permit();
    return TRUE;
}

static BOOL AddASemaphore(struct List *opened,
			  struct SignalSemaphore *ss,
			  CONST_STRPTR name)
{
    struct OpenedResourceNode *orn = get_orn();

    if (!orn)
    {
	return FALSE;
    }

    Forbid();
    orn->type = "Semaphore";
    orn->name = StaticStrDup(name);
    orn->addr = ss;
    orn->count = ss->ss_NestCount;
    Permit();

    Enqueue(opened, (struct Node *)orn);
    return TRUE;
}


static BOOL AddAClass(struct List *opened,
		      Class *cl, CONST_STRPTR name)
{
    struct OpenedResourceNode *orn;

    if (NULL == cl)
	return TRUE;

    orn = get_orn();
    if (!orn)
    {
	return FALSE;
    }

    Forbid();
    orn->type = "Class";
    if (NULL == name)
	name = cl->cl_ID;
    orn->name = StaticStrDup(name);
    orn->addr = cl;
    orn->count = cl->cl_ObjectCount;
    Permit();

    Enqueue(opened, (struct Node *)orn);
    return TRUE;
}


/* Add opencount-based resources to the tracking list. */
static BOOL AddOpenedResources(struct List *opened)
{
    struct DosInfo *di = BADDR(DOSBase->dl_Root->rn_Info);

    if (!AddLibs(opened))
	return FALSE;
    if (!AddDevs(opened))
	return FALSE;
    if (!AddFonts(opened))
	return FALSE;
    if (!AddNodeNames(opened, &SysBase->ResourceList, "Resource"))
	return FALSE;
    if (!AddNodeNames(opened, &SysBase->IntrList, "Interrupt"))
	return FALSE;
    if (!AddNodeNames(opened, &SysBase->PortList, "Port"))
	return FALSE;
    if (!AddNodeNames(opened, &SysBase->SemaphoreList, "Semaphore"))
	return FALSE;
    if (!AddASemaphore(opened, &di->di_DevLock, "di_DevLock"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->buttonclass, "button"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->textclass, "text"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->sliderclass, "slider"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->scrollerclass, "scroller"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->arrowclass, "arrow"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->stringclass, "string"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->listviewclass, "listview"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->checkboxclass, "checkbox"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->cycleclass, "cycle"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->mxclass, "mx"))
	return FALSE;
    if (!AddAClass(opened, ((struct GadToolsBase_intern *)GadToolsBase)->paletteclass, "palette"))
	return FALSE;
    return TRUE;
}

/* Get a snapshot of current resources */
static struct TrackedResources *NewResourcesState(void)
{
    struct TrackedResources *tr;

    tr = get_tr();
    if (!tr)
	return NULL;

    /* flush */
    FreeVec(AllocVec((ULONG)(~0ul/2), MEMF_ANY));

    /* opencount-based stuff */
    NEWLIST(&tr->opened);
    if (!AddOpenedResources(&tr->opened))
	return NULL;

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

/* Free snapshot of current resources */
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

void DisplayResourcesState(const struct TrackedResources *rs, int pagelines)
{
    /* FIXME */
    struct OpenedResourceNode *orn;
    IPTR tmp[4];
    IPTR mem[1];
    int currentlines;

    if (!rs)
	return;

    FPuts(Output(), "LeakWatch snapshot:\n");

    mem[0] = rs->freeMem;
    VFPrintf(Output(), " Free memory : %ld bytes\n", mem);

    FPuts(Output(), " Opened resources:\n");
    currentlines = 3;
    for(orn=(struct OpenedResourceNode *)rs->opened.lh_Head;
        orn->node.ln_Succ!=NULL;
        orn=(struct OpenedResourceNode *)orn->node.ln_Succ)
    {
	tmp[0] = (IPTR)orn->type;
	tmp[1] = (IPTR)orn->name;
	tmp[2] = (IPTR)orn->addr;
	tmp[3] = (IPTR)orn->count;
	
	if (currentlines >= (pagelines - 2))
	{
	    ULONG buf;
	    currentlines = 0;
	    FPuts(Output(), "--- Press a key to continue ---\n");
	    Flush(Input());
	    Read(Input(), &buf, 1);
	    Flush(Input());
	}
	VFPrintf(Output(), " %s: %s (0x%lx) : %lu\n", tmp);
	currentlines++;
    }
    FPuts(Output(), "-- end of state\n");
}

/* Compute the delta between 2 resources snapshots.
 * the ResourceDiff can have dangling pointers in old and nu, so dont clear
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
			mr->type = other->type;
			mr->name = other->name;
			mr->addr = other->addr;
			mr->before_count = other->count;
			mr->after_count = orn->count;
			Enqueue(&rd->modifiedOpened, (struct Node *)mr);
		    }
		}
	    }
	}
	if (!seen)
	{
	    struct ModifiedResource *mr = get_mr();
	    if (!mr)
		return NULL;
	    
	    mr->type = orn->type;
	    mr->name = orn->name;
	    mr->addr = orn->addr;
	    mr->before_count = 0;
	    mr->after_count = orn->count;

	    Enqueue(&rd->modifiedOpened, (struct Node *)mr);
	}
    }


    rd->memLost = old->freeMem - nu->freeMem;

    return rd;
}

static void DisplayStateDiff(const struct ResourceDiff *rd, int pagelines)
{
    /* FIXME */
    IPTR mem[2];
    IPTR modified[5];
    struct ModifiedResource *mr;
    int currentlines;

    FPuts(Output(), "LeakWatch report:\n");
    mem[0] = rd->memLost;
    mem[1] = (IPTR)((rd->memLost > 1) ? "s" : "");
    VFPrintf(Output(), " Memory lost : %ld byte%s\n", mem);

    FPuts(Output(), " Open count:\n");
    currentlines = 3;
    for(mr=(struct ModifiedResource *)rd->modifiedOpened.lh_Head;
	mr->node.ln_Succ!=NULL;
	mr=(struct ModifiedResource *)mr->node.ln_Succ)
    {
	modified[0] = (IPTR)mr->type;
	modified[1] = (IPTR)mr->name;
	modified[2] = (IPTR)mr->addr;
	modified[3] = mr->before_count;
	modified[4] = mr->after_count;
	
	if (currentlines >= (pagelines - 2))
	{
	    ULONG buf;
	    currentlines = 0;
	    FPuts(Output(), "--- Press a key to continue ---\n");
	    Flush(Input());
	    Read(Input(), &buf, 1);
	    Flush(Input());
	}
	VFPrintf(Output(), " %s: %s (0x%lx) : %lu -> %lu\n", modified);
	currentlines++;
    }

    FPuts(Output(), "-- end of diff\n");
    
}

static void DeleteStateDiff(struct ResourceDiff *rd)
{
    /* FIXME */
    struct ModifiedResource *mr;
    struct ModifiedResource *tmpmr;

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

void open_libs()
{
    GadToolsBase = OpenLibrary ( "gadtools.library", 0L );
}

void close_libs()
{
    CloseLibrary ( GadToolsBase );
}

int main(void)
{
    struct TrackedResources *crs = NULL;
    struct TrackedResources *start_rs = NULL;
    BOOL quitme = FALSE;
    int numlines = 30;
    ULONG  portsig;
    struct MsgPort *port;

    port = CreateMsgPort();
    if (!port)
	return 2;
    port->mp_Node.ln_Name = "LeakWatch";
    port->mp_Node.ln_Pri  = 0;
    AddPort(port);

    portsig = 1L << port->mp_SigBit;

    open_libs();

    FPuts(Output(), "LeakWatch running, CTRL-C to exit, CTRL-E to watch for leaks since beginning, CTRL-F to watch for leaks since last CTRL-F, CTRL-D for an usage snapshot\n");

    crs = NewResourcesState();
    if (NULL == crs)
	quitme = TRUE;
    else
	start_rs = CopyResourcesState(crs);

    while(!quitme)
    {
	ULONG signals;

	signals = Wait(portsig | SIGBREAKF_CTRL_F | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_C);

	if (signals & SIGBREAKF_CTRL_D)
	{
	    struct TrackedResources *tr;

	    tr = NewResourcesState();
	    if (NULL == tr)
	    {
		quitme = TRUE;
		break;
	    }
	    DisplayResourcesState(tr, numlines);
	    DeleteResourcesState(tr);
	}
	if (signals & SIGBREAKF_CTRL_E)
	{
	    struct ResourceDiff *rd = NULL;

	    DeleteResourcesState(crs);
	    crs = NewResourcesState();
	    if (NULL == crs)
	    {
		quitme = TRUE;
		break;
	    }
	    /*  DisplayResourcesState(crs); */ /* only for debug */
	    rd = NewStateDiff(start_rs, crs);
	    DisplayStateDiff(rd, numlines);
	    DeleteStateDiff(rd);
	}
	if (signals & SIGBREAKF_CTRL_F)
	{
	    struct TrackedResources *ors = crs;
	    struct ResourceDiff *rd = NULL;

	    crs = NewResourcesState();
	    if (NULL == crs)
	    {
		quitme = TRUE;
		break;
	    }
	    rd = NewStateDiff(ors, crs);
	    DisplayStateDiff(rd, numlines);
	    DeleteStateDiff(rd);
	    DeleteResourcesState(ors);
	}
	if (signals & SIGBREAKF_CTRL_C)
	{
	    quitme = TRUE;
	}
	if (signals & portsig)
	{
	    struct Message *msg;

	    while((msg = (struct Message *)GetMsg(port)))
	    {
		D(bug("Received watch message.\n"));

		ReplyMsg(msg);
	    }

	}
    } /* while(!quitme) */

    DeleteResourcesState(crs);
    DeleteResourcesState(start_rs);

    close_libs();

    if (port)
    {
	RemPort(port);
    	DeleteMsgPort(port);
    }
    return 0;
}

