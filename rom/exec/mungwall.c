/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: MungWall memory anti-trashing checker
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"
#include "mungwall.h"

#define DSCAN(x)

/*
 * Build a wall around the allocated chunk.
 * Returns updated pointer to the beginning of the chunk (actually a pointer to a usable area)
 *
 * 'res' is a pointer to the beginning of a raw memory block (inside which walls will be constructed)
 * 'origSize' is ORIGINAL allocation size (before adding mungwall size)
 */
APTR MungWall_Build(APTR res, APTR pool, IPTR origSize, ULONG requirements, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    if ((PrivExecBase(SysBase)->IntFlags & EXECF_MungWall) && res)
    {
    	struct MungwallHeader *header = res;

	D(bug("[MungWall] Allocated %u bytes at 0x%p\n", origSize, res + MUNGWALL_BLOCK_SHIFT));

	/*
	 * Build pre-wall starting from header, and up to MUNGWALL_BLOCK_SHIFT.
	 * This will fill also header padding. Will help in detecting issues.
	 */
        memset(res, 0xDB, MUNGWALL_BLOCK_SHIFT);

        /*
         * Now save allocation info in the header (there is one room of MUNGWALLHEADER_SIZE
	 * bytes before wall for such stuff (see above).
	 */
	header->mwh_magicid   = MUNGWALL_HEADER_ID;
	header->mwh_fault     = FALSE;
	header->mwh_allocsize = origSize;
	header->mwh_pool      = pool;
	header->mwh_AllocFunc = loc->function;
	header->mwh_Owner     = FindTask(NULL);
	header->mwh_Caller    = loc->caller;

	/* Skip to the start of data space */
        res += MUNGWALL_BLOCK_SHIFT;

	/* Fill the block with weird stuff to exploit bugs in applications */
	if (!(requirements & MEMF_CLEAR))
	    MUNGE_BLOCK(res, MEMFILL_ALLOC, origSize);

	/*
	 * Initialize post-wall.
	 * Fill only MUNGWALL_SIZE bytes, this is what we are guaranteed to have in the end.
	 * We can't assume anything about padding. AllocMem() does guarantee that the actual
	 * allocation start and size is a multiple of MEMCHUNK_TOTAL, but for example
	 * AllocPooled() doesn't. Pooled allocations are only IPTR-aligned (see InternalFreePooled(),
	 * in AROS pooled allocations are vectored, pool pointer is stored in an IPTR preceding the
	 * actual block in AllocVec()-alike manner).
	 * IPTR alignment is 100% correct since original AmigaOS(tm) autodocs say that even AllocMem()
	 * result is longword-aligned. AROS introduces additional assumption that AllocMem() result
	 * is aligned at least up to AROS_WORSTALIGN (CPU-specific value, see exec/memory.h), however
	 * this is still not true for other allocation functions (AllocVec() etc).
	 */
	memset(res + origSize, 0xDB, MUNGWALL_SIZE);

	Forbid();
    	AddHead((struct List *)&PrivExecBase(SysBase)->AllocMemList, (struct Node *)&header->mwh_node);
	Permit();
    }
    return res;
}

char *FormatMWContext(char *buffer, struct MungwallContext *ctx, struct ExecBase *SysBase)
{
    buffer = NewRawDoFmt("In %s, block at 0x%p, size %lu", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->freeFunc, (APTR)ctx->hdr + MUNGWALL_BLOCK_SHIFT, ctx->hdr->mwh_allocsize) - 1;
    buffer = NewRawDoFmt("\nAllocated using %s ", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->hdr->mwh_AllocFunc) - 1;
    buffer = FormatTask(buffer, "by task 0x%p (%s)", ctx->hdr->mwh_Owner, SysBase);
    buffer = FormatLocation(buffer, " at 0x%p", ctx->hdr->mwh_Caller, SysBase);

    if (ctx->hdr->mwh_magicid != MUNGWALL_HEADER_ID)
    	buffer = NewRawDoFmt("\nMUNGWALL_HEADER_ID mismatch", (VOID_FUNC)RAWFMTFUNC_STRING, buffer) - 1;

    if (ctx->freeSize)
    	buffer = NewRawDoFmt("\nFreeMem size %lu mismatch", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->freeSize) - 1;

    if (ctx->pre_start)
    	buffer = NewRawDoFmt("\nPre-wall broken at 0x%p - 0x%p", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->pre_start, ctx->pre_end) - 1;

    if (ctx->post_start)
    	buffer = NewRawDoFmt("\nPost-wall broken at 0x%p - 0x%p", (VOID_FUNC)RAWFMTFUNC_STRING, buffer, ctx->post_start, ctx->post_end) - 1;

    return buffer;
}

static APTR CheckWall(UBYTE *ptr, UBYTE fill, IPTR size, APTR *endptr)
{
    APTR start = NULL;
    APTR end   = NULL;

    while (size--)
    {
	if (*ptr != fill)
	{
	    if (!start)
	    	start = ptr;

	    end = ptr;
	}

	ptr++;
    }

    *endptr = end;
    return start;
}

static void CheckHeader(struct MungwallHeader *header, IPTR byteSize, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    struct MungwallContext mwdata;

    /* Do not report the fault twice on the same header */
    if (header->mwh_fault)
    	return;

    if (header->mwh_magicid != MUNGWALL_HEADER_ID)
        header->mwh_fault = TRUE;

    if (byteSize && (header->mwh_allocsize != byteSize))
    {
    	mwdata.freeSize = byteSize;
	header->mwh_fault = TRUE;
    }
    else
	mwdata.freeSize = 0;

    mwdata.pre_start = CheckWall((UBYTE *)header + MUNGWALLHEADER_SIZE, 0xDB, MUNGWALL_SIZE, &mwdata.pre_end);
    if (mwdata.pre_start)
    	header->mwh_fault = TRUE;

    mwdata.post_start = CheckWall((UBYTE *)header + MUNGWALL_BLOCK_SHIFT + header->mwh_allocsize, 0xDB, MUNGWALL_SIZE, &mwdata.post_end);
    if (mwdata.post_start)
    	header->mwh_fault = TRUE;

    if (header->mwh_fault)
    {
    	/* Throw an alert with context */
    	mwdata.hdr      = header;
	mwdata.freeFunc = loc->function;
    	Exec_ExtAlert(AN_MemoryInsane, loc->caller, loc->stack, AT_MUNGWALL, &mwdata, SysBase);

    	/*
    	 * Our entry can be freed by another process while we are sitting in Alert().
    	 * This is 100% safe as long as we don't touch the entry after Alert().
    	 * Well, potentally dangerous case is list iteration in MungWall_Scan().
    	 * What to do then? Use a semaphore? Won't do because of RemTask(NULL),
    	 * during which SysBase->ThisTask becomes garbage, thus a semaphore can't
    	 * be used.
    	 */
    }
}

/*
 * Check integrity of walls around the specified block.
 *
 * 'memoryBlock' is an address of the block from user's point of view
 * (i. e. what was returned by allocation call)
 * 'byteSize' is length of the block (again, from user's point of view(
 *
 * Returns address of the raw block (what really needs to be deallocated)
 */
APTR MungWall_Check(APTR memoryBlock, IPTR byteSize, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
	struct MungwallHeader *header;

	D(bug("[MungWall] Freeing %u bytes at 0x%p\n", byteSize, memoryBlock));

	/* Align size and block to the requirements (needed because of AllocAbs) */
	byteSize += (IPTR)memoryBlock & (MEMCHUNK_TOTAL - 1);
	memoryBlock = (APTR)AROS_ROUNDDOWN2((IPTR)memoryBlock, MEMCHUNK_TOTAL);

	/* Take address of mungwall header */
	header = memoryBlock - MUNGWALL_BLOCK_SHIFT;

    	/*
    	 * Remove from PrivExecBase->AllocMemList.
    	 * Do it before checking, otherwise AvailMem() can hit into it and cause a deadlock
    	 * while the alert is displayed.
    	 */
	Forbid();
    	Remove((struct Node *)header);
	Permit();

	/* Reset fault state in order to see who is freeing the bad entry */
	header->mwh_fault = FALSE;

	CheckHeader(header, byteSize, loc, SysBase);

	/* Fill block with weird stuff to esploit bugs in applications
	 *
	 * DOH! There's some _BAD_ code around that assumes memory can still be
	 * accessed after freeing by just preventing task switching. In AROS,
	 * RemTask(NULL) suffers of this problem because DOS processes are
	 * created with their TCB placed in the tc_MemEntry list.
	 * The workaround is to avoid munging when current task is in TS_REMOVED
	 * state (RemTask() sets it). However RemTask() still needs reengineering
	 * before memory protection can be used. With MP deallocating memory can
	 * cause immediate blocking of access to it, so RemTask() needs to move
	 * the stack to some safe place and make sure that task structure is not
	 * accessed after freeing it.
	 */
	if (SysBase->ThisTask->tc_State != TS_REMOVED)
		MUNGE_BLOCK(memoryBlock, MEMFILL_FREE, byteSize);

	/* Return real start of the block to deallocate */
	memoryBlock = header;
    }

    return memoryBlock;
}

/*
 * Scan the whole allocations list, optionally removing entries
 * belonging to a particular pool.
 */
void MungWall_Scan(APTR pool, struct TraceLocation *loc, struct ExecBase *SysBase)
{
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
	struct MungwallHeader 	*allocnode;
	struct MungwallHeader	*tmp;

	DSCAN(bug("[Mungwall] Scan(), caller %s, SysBase 0x%p\n", function, SysBase));

	Forbid();

	ForeachNodeSafe(&PrivExecBase(SysBase)->AllocMemList, allocnode, tmp)
	{
	    DSCAN(bug("[Mungwall] allocnode 0x%p, next 0x%p, %s(%lu)\n", allocnode, tmp, allocnode->mwh_AllocFunc, allocnode->mwh_allocsize));

	    if (pool && (allocnode->mwh_pool == pool))
	    {
		/*
		 * If pool address is given, remove entries belonging to it.
		 * It's DeletePool() and they are going to be freed.
		 * Additionally we reset fault state on them. This will cause
		 * one more alert and we can track where the memory was freed.
	         * This will give us a hint on who was owning it.
		 */
		Remove((struct Node *)allocnode);
	    	allocnode->mwh_fault = FALSE;
	    }

	    CheckHeader(allocnode, 0, loc, SysBase);
	}

	Permit();
    }
}
