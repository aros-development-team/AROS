/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: MungWall memory anti-trashing checker
    Lang: english
*/

#include <proto/exec.h>

#include "exec_intern.h"
#include "memory.h"
#include "mungwall.h"

/*
 * Build a wall around the allocated chunk.
 * Returns updated pointer to the beginning of the chunk (actually a pointer to a usable area)
 *
 * 'res' is a pointer to the beginning of a raw memory block (inside which walls will be constructed)
 * 'origSize' is ORIGINAL allocation size (before adding mungwall size)
 */
APTR MungWall_Build(APTR res, APTR pool, IPTR origSize, ULONG requirements, struct ExecBase *SysBase)
{
    if ((PrivExecBase(SysBase)->IntFlags & EXECF_MungWall) && res)
    {
    	struct MungwallHeader *header;

	D(bug("[MungWall] Allocated %u bytes at 0x%p\n", origSize, res + MUNGWALL_BLOCK_SHIFT));

        /* Save orig byteSize before wall (there is one room of MUNGWALLHEADER_SIZE
	   bytes before wall for such stuff (see above).
	*/

	header = (struct MungwallHeader *)res;

	header->mwh_magicid   = MUNGWALL_HEADER_ID;
	header->mwh_allocsize = origSize;
	header->mwh_pool      = pool;

	/* Skip to the start of the pre-wall */
        res += MUNGWALLHEADER_SIZE;

	/* Initialize pre-wall */
	BUILD_WALL(res, 0xDB, MUNGWALL_SIZE);

	/* move over the block between the walls */
	res += MUNGWALL_SIZE;

	/* Fill the block with weird stuff to exploit bugs in applications */
	if (!(requirements & MEMF_CLEAR))
	    MUNGE_BLOCK(res, MEMFILL_ALLOC, origSize);

	/* Initialize post-wall */
	BUILD_WALL(res + origSize, 0xDB, MUNGWALL_SIZE + AROS_ROUNDUP2(origSize, MEMCHUNK_TOTAL) - origSize);

	Forbid();
    	AddHead((struct List *)&PrivExecBase(SysBase)->AllocMemList, (struct Node *)&header->mwh_node);
	Permit();
    }
    return res;
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
APTR MungWall_Check(APTR memoryBlock, IPTR byteSize, struct ExecBase *SysBase)
{
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
	struct MungwallHeader *header;

	D(bug("[MungWall] Freeing %u bytes at 0x%p\n", byteSize, memoryBlock));

	/* Align size to the requirements (needed because of AllocAbs) */
	byteSize+=(IPTR)memoryBlock&(MEMCHUNK_TOTAL-1);

	/* Align the block as well (needed because of AllocAbs) */
	memoryBlock=(APTR)AROS_ROUNDDOWN2((IPTR)memoryBlock,MEMCHUNK_TOTAL);

	/* Take address of mungwall header */
	header = (struct MungwallHeader *)(memoryBlock - MUNGWALL_BLOCK_SHIFT);

	if (header->mwh_magicid != MUNGWALL_HEADER_ID)
	{
	    struct Task *__t = FindTask(NULL);	\
	    kprintf("\x07" "MUNGWALL_HEADER_ID mismatch mem = %p "
		    "allocsize = %lu  freesize = %lu   Task: 0x%p, Name: %s\n", \
		    memoryBlock,
		    header->mwh_allocsize,
		    byteSize,
		    __t,
		    __t->tc_Node.ln_Name);\
	}

	if (header->mwh_allocsize != byteSize)
	{
	    struct Task *__t = FindTask(NULL);	\
	    kprintf("\x07" "FreeMem size mismatches AllocMem size mem = %p "
		    "allocsize = %lu  freesize = %lu   Task: 0x%x, Name: %s\n", \
		    memoryBlock,
		    header->mwh_allocsize,
		    byteSize,
		    __t,
		    __t->tc_Node.ln_Name);\
	}

	CHECK_WALL((UBYTE *)header + MUNGWALLHEADER_SIZE, 0xDB, MUNGWALL_SIZE);
	CHECK_WALL((UBYTE *)memoryBlock + byteSize, 0xDB,
		MUNGWALL_SIZE + AROS_ROUNDUP2(byteSize, MEMCHUNK_TOTAL) - byteSize);

    	/* Remove from PrivExecBase->AllocMemList */

	Forbid();
    	Remove((struct Node *)&header->mwh_node);
	Permit();

	/* Fill block with weird stuff to esploit bugs in applications
	 *
	 * DOH! There's some _BAD_ code around that assumes memory can still be
	 * accessed after freeing by just preventing task switching. In AROS,
	 * RemTask(NULL) suffers of this problem because DOS processes are
	 * created with their TCB placed in the tc_MemEntry list. The workaround
	 * is to avoid munging when FreeMem() is called with task switching disabled.
	 */
	/* DOH! it doesn't work even this way. What's wrong???
	 * It's stack which is also in tc_MemEntry - Pavel Fedin.
	 * 
	 * if ((SysBase->TDNestCnt < 0) && (SysBase->IDNestCnt < 0))
	 *	MUNGE_BLOCK(memoryBlock, MEMFILL_FREE, byteSize);
	 */
	 
	/* Return real start of the block to deallocate */
	memoryBlock = header;
    }

    return memoryBlock;
}

/*
 * Scan the whole allocations list, optionally removing entries
 * belonging to a particular pool.
 */
void MungWall_Scan(APTR pool, struct ExecBase *SysBase)
{
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
	struct MungwallHeader 	*allocnode;
	struct MungwallHeader	*tmp;
	ULONG	    	    	 alloccount = 0;
	IPTR	    	    	 allocsize = 0;

	/*
	 * Do not print summary if called from within DeletePool().
	 * Otherwise it gets really slow.
	 */
	if (!pool)
	    kprintf("\n=== MUNGWALL MEMORY CHECK ============\n");

	Forbid();

	ForeachNodeSafe(&PrivExecBase(SysBase)->AllocMemList, allocnode, tmp)
	{
	    if (allocnode->mwh_magicid != MUNGWALL_HEADER_ID)
	    {
		kprintf(" #%05x BAD MUNGWALL_HEADER_ID mem = %p allocsize = %lu\n",
			alloccount,
			(APTR)allocnode + MUNGWALL_BLOCK_SHIFT,
			allocnode->mwh_allocsize);
	    }

	    CHECK_WALL((UBYTE *)allocnode + MUNGWALLHEADER_SIZE, 0xDB, MUNGWALL_SIZE);
	    CHECK_WALL((UBYTE *)allocnode + MUNGWALL_BLOCK_SHIFT + allocnode->mwh_allocsize, 0xDB,
		       MUNGWALL_SIZE + AROS_ROUNDUP2(allocnode->mwh_allocsize, MEMCHUNK_TOTAL) - allocnode->mwh_allocsize);

	    if (pool && (allocnode->mwh_pool == pool))
	    {
		/*
		 * If pool address is given, remove entries belonging to it.
		 * They are considered freed, so we don't count them.
		 */
		Remove((struct Node *)&allocnode->mwh_node);
	    }
	    else
	    {
		allocsize += allocnode->mwh_allocsize;
		alloccount++;
	    }
	}

	Permit();

	if (!pool)
	    kprintf("\n Num allocations: %u   Memory allocated %lu\n", alloccount, allocsize);
    }
}
