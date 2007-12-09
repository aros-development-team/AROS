/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ROMTag scanner. Adapted from the original i386-native to become
    system independent.
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include "exec_intern.h"

#define DEBUG 0
#include <aros/debug.h>
/*
 * RomTag scanner.
 *
 * This function scans kernel for existing Resident modules. If two modules
 * with the same name are found, the one with higher version or priority
 * wins.
 *
 * After building list of kernel modules, the KickTagPtr and KickMemPtr are
 * checksummed. If checksum is proper and all memory pointed in KickMemPtr
 * may be allocated, then all modules from KickTagPtr are added to RT list
 *
 * Afterwards the proper RomTagList is created (see InitCode() for details)
 * and memory after list and nodes is freed.
 *
 * The array ranges gives a [ start, end ] pair to scan, with an entry of
 * -1 used to break the loop.
 */

struct rt_node
{
    struct Node     node;
    struct Resident *module;
};

ULONG **AROS_SLIB_ENTRY(RomTagScanner,Exec)
(
    struct ExecBase *SysBase,
    UWORD	    *ranges[]
)
{
    struct List     rtList;             /* List of modules */
    UWORD	    *end;
    UWORD	    *ptr;		/* Start looking here */

    struct Resident *res;               /* module found */

    int     i;
    ULONG   **RomTag;

    /* Initialize list */
    NEWLIST(&rtList);

    /* Look in whole kernel for resident modules */
    while (*ranges != (UWORD *)~0)
    {
	ptr = *ranges++;
	end = *ranges++;

	kprintf("RomTagScanner: Start = %p, End = %p\n", ptr, end);
	do
	{
	    res = (struct Resident *)ptr;

	    /* Do we have RTC_MATCHWORD and rt_MatchTag*/
	    if (    res->rt_MatchWord	== RTC_MATCHWORD
		&&  res->rt_MatchTag	== res
	    )
	    {
		/* Yes, it is Resident module */
		struct rt_node  *node;

		/* Check if there is module with such name already */
		node = (struct rt_node*)FindName(&rtList, res->rt_Name);
		if (node)
		{
		    /*
			Rules for replacing modules:
			1. Higher version always wins.
			2. If the versions are equal, then lower priority
			    looses.
		    */
		    if
		    (
			node->module->rt_Version < res->rt_Version
			||
			(
			    node->module->rt_Version == res->rt_Version
			    && node->node.ln_Pri <= res->rt_Pri
			)
		    )
		    {
			node->node.ln_Pri   = res->rt_Pri;
			node->module        = res;

			/* Have to re-add the node at it's new position. */
			Remove((struct Node *)node);
			Enqueue(&rtList, (struct Node *)node);
		    }
		}
		else
		{
		    /* New module. Allocate some memory for it */
		    node = (struct rt_node *)
			AllocMem(sizeof(struct rt_node),MEMF_PUBLIC|MEMF_CLEAR);

		    if (node)
		    {
			node->node.ln_Name  = res->rt_Name;
			node->node.ln_Pri   = res->rt_Pri;
			node->module        = res;

			Enqueue(&rtList,(struct Node*)node);
		    }
		}

		/* Get address of EndOfResident from RomTag but only when
		 * it's higher then present one - this avoids strange locks
		 * when not all modules have Resident structure in .text
		 * section */
		ptr = ((ULONG)res->rt_EndSkip > (ULONG)ptr)
			?   (UWORD *)res->rt_EndSkip - 2
			:   ptr;

		if ((ULONG)ptr & 0x01)
		   ptr = (UWORD *)((ULONG)ptr+1);
	    }

	    /* Get next address... */
	    ptr++;
	} while (ptr < (UWORD*)end);
    }

    /*
     * By now we have valid (and sorted) list of kernel resident modules.
     *
     * Now, we will have to analyze used-defined RomTags (via KickTagPtr and
     * KickMemPtr)
     */
#warning TODO: Implement external modules!

    /*
     * Everything is done now. Allocate buffer for normal RomTag and convert
     * list to RomTag
     */

    ListLength(&rtList,i);      /* Get length of the list */

    RomTag = AllocMem((i+1)*4,MEMF_PUBLIC | MEMF_CLEAR);

    kprintf("Resident modules (addr: pri version name):\n");
    if (RomTag)
    {
        int             j;
        struct rt_node  *n;

        for (j=0; j<i; j++)
        {
            n = (struct rt_node *)RemHead(&rtList);
            kprintf("+ 0x%08.8lx: %3d %3d \"%s\"\n",
                n->module,
                n->node.ln_Pri,
                n->module->rt_Version,
                n->node.ln_Name);
            RomTag[j] = (ULONG*)n->module;

            FreeMem(n, sizeof(struct rt_node));
        }
        RomTag[i] = 0;
    }
    return RomTag;
}
