/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Resident CLI command
    Lang: English
*/


#include <proto/dos.h>
#include <dos/dosextens.h>
#include <string.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include <string.h>

#include <aros/shcommands.h>

struct SegNode
{
    struct MinNode node;
    IPTR data[2];
};

static struct SegNode *NewSegNode(struct ExecBase *SysBase, STRPTR name, LONG uc);

AROS_SH7(Resident, 41.2,
AROS_SHA(STRPTR, ,NAME, ,NULL),
AROS_SHA(STRPTR, ,FILE, ,NULL),
AROS_SHA(BOOL, ,REMOVE,/S,FALSE),
AROS_SHA(BOOL, ,ADD,/S,FALSE),
AROS_SHA(BOOL, ,REPLACE,/S,FALSE),
AROS_SHA(BOOL,PURE=,FORCE,/S,FALSE),
AROS_SHA(BOOL, ,SYSTEM,/S,FALSE))
{
    AROS_SHCOMMAND_INIT


    if (SHArg(FILE) || SHArg(NAME))
    {
	STRPTR name, file;
	BPTR seglist;
	struct FileInfoBlock *fib;

	if (SHArg(FILE))
	{
	    name = SHArg(NAME);
	    file = SHArg(FILE);
        }
	else
	{
	    name = FilePart(SHArg(NAME));
	    file = SHArg(NAME);
        }

	SetIoErr(0);
	if (SHArg(REMOVE))
	{
	    struct Segment *found;

	    Forbid();
	    found = FindSegment(name, NULL, TRUE);
	    if (!found)
	    {
	        Permit();
		SetIoErr(ERROR_OBJECT_NOT_FOUND);
	    }
	    else
	    if (!RemSegment(found))
	    {
	        if (found->seg_UC == CMD_INTERNAL)
	            found->seg_UC = CMD_DISABLED;
		else
		    SetIoErr(ERROR_OBJECT_IN_USE);
            }

	    if (IoErr())
	    {
	        PrintFault(IoErr(), SHArg(NAME));
		return RETURN_FAIL;
	    }

	    return RETURN_OK;
	}

	if (SHArg(REPLACE))
	{
	    struct Segment *found;

	    Forbid();
	    found = FindSegment(name, NULL, TRUE);
	    if (found && found->seg_UC == CMD_DISABLED)
	    {
		found->seg_UC = CMD_INTERNAL;
		Permit();
		return RETURN_OK;
	    }
	    Permit();
	}

	if (!SHArg(ADD)) SHArg(REPLACE) = TRUE;

	if (!SHArg(FORCE) && (fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
	{
	    BPTR lock;

	    if ((lock = Lock(file, SHARED_LOCK)))
	    {
	        if (Examine(lock, fib))
		{
		    if ((fib->fib_Protection & FIBF_PURE) == 0)
		        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		}

		UnLock(lock);
	    }

	    FreeDosObject(DOS_FIB, fib);
	}

	if (IoErr() || !(seglist = LoadSeg(file)))
	{
	    PrintFault(IoErr(), file);
	    return RETURN_FAIL;
        }

	if (SHArg(REPLACE))
	{
	    struct Segment *found;

	    Forbid();
	    found = FindSegment(name, NULL, FALSE);

	    if (found)
	    {
		if (found->seg_UC != 0)
		{
		     Permit();
		     PrintFault(ERROR_OBJECT_IN_USE, file);
		     UnLoadSeg(seglist);
		     return RETURN_FAIL;
		}

  		UnLoadSeg(found->seg_Seg);
		found->seg_Seg = seglist;
		Permit();

		return RETURN_OK;
	    }
	/* Fall through */
	}

	/* WB1.x backwards compatibility hack, do not allow
	 * override of built-in resident or to add l:shell-seg (CLI) */
	if (!stricmp(name, "resident") || !stricmp(name, "cli")) {
	    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	    UnLoadSeg(seglist);
	    return 1; /* yes, return code = 1 in this special case */
	}

	if (!AddSegment(name, seglist, SHArg(SYSTEM)?CMD_SYSTEM:0))
	{
	    UnLoadSeg(seglist);
	    PrintFault(IoErr(), "Resident");
	    return RETURN_FAIL;
	}
    }
    else
    {

	struct MinList l;
	struct Segment *curr;
	struct SegNode *n;
	struct DosInfo *dinf = BADDR(DOSBase->dl_Root->rn_Info);
	BOOL isbreak = FALSE;

	NEWLIST((struct List *)&l);

	SetIoErr(0);
	Forbid();
	curr = (struct Segment *)BADDR(dinf->di_ResList);
	while (curr)
	{
 	    n = NewSegNode(SysBase, AROS_BSTR_ADDR(MKBADDR(&curr->seg_Name[0])), curr->seg_UC);

	    if (!n)
	    {
	        SetIoErr(ERROR_NO_FREE_STORE);
		break;
	    }

	    AddTail((struct List *)&l, (struct Node *)n);
	    curr = (struct Segment *)BADDR(curr->seg_Next);
	}
	Permit();

	if (IoErr())
	{
	    PrintFault(IoErr(), "Resident");
	    return RETURN_FAIL;
        }

	PutStr("NAME                           USECOUNT\n\n");
	while ((n = (struct SegNode *)RemHead((struct List *)&l)))
	{
            if (SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
                isbreak = TRUE;
            if (!isbreak) {
                if (n->data[1] == CMD_SYSTEM)
                    VPrintf("%-30s SYSTEM\n", n->data);
                else
                if (n->data[1] == CMD_INTERNAL)
                    VPrintf("%-30s INTERNAL\n", n->data);
                else
                if (n->data[1] == CMD_DISABLED)
                    VPrintf("%-30s DISABLED\n", n->data);
                else
                    VPrintf("%-30s %-ld\n", n->data);
            }
            FreeVec((APTR)n->data[0]);
            FreeVec(n);
        }
        if (isbreak) {
            SetIoErr(ERROR_BREAK);
            PrintFault(IoErr(), "Resident");
            return RETURN_FAIL;
        }
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}

static STRPTR StrDup(struct ExecBase *SysBase, STRPTR str)
{
    size_t len = strlen(str)+1;
    STRPTR ret = (STRPTR) AllocVec(len, MEMF_ANY);

    if (ret)
    {
        CopyMem(str, ret, len);
    }

    return ret;
}


static struct SegNode *NewSegNode(struct ExecBase *SysBase, STRPTR name,
				  LONG uc)
{
    struct SegNode *sn = AllocVec(sizeof (struct SegNode), MEMF_ANY);

    if (sn)
    {
        sn->data[0] = (IPTR) StrDup(SysBase, name);
	if (sn->data[0])
	{
 	    sn->data[1] = uc;
	    return sn;
     	}

	FreeVec(sn);
    }

    return NULL;
}
