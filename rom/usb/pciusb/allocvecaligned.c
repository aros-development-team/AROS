/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate aligned memory within boundary
    Lang: english
*/

#ifdef AROS_USB30_CODE

#include <aros/debug.h>

#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/memory.h>

#include <proto/exec.h>

void FreeVecAligned(APTR allocation) {
}

APTR AllocVecAligned(IPTR byteSize, ULONG alignmentMin, IPTR boundary) {

	struct MemHeader *memheader;
	struct MemChunk *prevchunk, *memchunk, *newchunk;

    APTR alignedMemStart,alignedMemEnd;
    IPTR byteSizeRounded = AROS_ROUNDUP2(byteSize, MEMCHUNK_TOTAL);

    APTR res = NULL;

    if(alignmentMin<MEMCHUNK_TOTAL) {
        alignmentMin = MEMCHUNK_TOTAL;
    }

    /* Some sanity checks */
    if( ((boundary!=0) && (byteSize>boundary)) || (alignmentMin & (MEMCHUNK_TOTAL-1)) || (byteSize == 0) ) {
        bug("[AVA] Insane allocation request!\n");
        return res;
    }

    boundary = ~(boundary -1);

    Forbid();

    ForeachNode(&SysBase->MemList, memheader) {

	    if ( (MEMF_FAST & ~memheader->mh_Attributes) )
	        continue;

		prevchunk = (struct MemChunk *)(&memheader->mh_First);
		memchunk = memheader->mh_First;

        while ( memchunk != NULL ) {

            /* Minimum free space needed before doing alignments, might still not be enough */
            if( memchunk->mc_Bytes >= byteSizeRounded + MEMCHUNK_TOTAL ) {

                alignedMemStart = (APTR) AROS_ROUNDUP2( (IPTR) memchunk + MEMCHUNK_TOTAL , alignmentMin);
                alignedMemEnd = alignedMemStart + byteSizeRounded - 1;

                /* Check if the allocation is still within the chunk */
                while( (IPTR) alignedMemEnd <= (IPTR) (memchunk + memchunk->mc_Bytes - 1) ) {

                    //bug("alignedMem %p-%p %x\n",alignedMemStart, alignedMemEnd, byteSizeRounded );
                 
                    /* Does the allocation need to be within a boundary? */
                    if( boundary!=0 ) {
                        if( ((IPTR) alignedMemStart & boundary) == ((IPTR) alignedMemEnd & boundary) ) {
                            /* Do the bad thing and alter memchunks... */
                            res = alignedMemStart;
                            break;
                        } else {
                            /* Allocation spans across the boundary. Adjust the alignedMemStart to satisfy boundary requirements */
                            bug("[AVA] allocation crosses boundary -> reiterating!\n");
                            alignedMemStart = (APTR) ((IPTR) alignedMemEnd & boundary);
                            alignedMemEnd = alignedMemStart + byteSizeRounded - 1;
                        }
                    }else{
                        /* Do the bad thing and alter memchunks... */
                        res = alignedMemStart;
                        break;
                    }
                }

                if(res != NULL) {

                    bug("[AVA] MC %p AVA(%x->%x,%x,%x) -> %x(%x)-%x\n", \
                        memchunk, \
                        byteSize, \
                        byteSizeRounded, \
                        alignmentMin, \
                        (~boundary)+1, \
                        alignedMemStart, \
                        alignedMemStart-MEMCHUNK_TOTAL, \
                        alignedMemEnd \
                        );

                    newchunk = alignedMemEnd + 1;
                    newchunk->mc_Next = memchunk->mc_Next;

                    bug("newchunk %p\n", newchunk);

                    /* If the start of our chunk aligned allocation is near the start of the chunk we allocated from then we snap to it */
                    if( ((IPTR) alignedMemStart - (IPTR) memchunk) <= 0x100 ) {
                        /* Destroy this memchunk */
                        prevchunk->mc_Next = newchunk;
                        newchunk->mc_Bytes = memchunk->mc_Bytes - ((IPTR) newchunk - ((IPTR) memchunk - 1));
                    }else{
                        /* Reuse this memchunk */
                        memchunk->mc_Next = newchunk;

                        newchunk->mc_Bytes = memchunk->mc_Bytes - ((IPTR) newchunk - ((IPTR) memchunk - 1));
                        memchunk->mc_Bytes = (IPTR) alignedMemStart - (IPTR) memchunk - 1;
                    }

		            memheader->mh_Free -= byteSizeRounded + MEMCHUNK_TOTAL;
                    res = NULL;
                    break;
                }

            }

			/* Go to next chunk */
			prevchunk = memchunk;
			memchunk = memchunk->mc_Next;
        }

    }

    Permit();

    return res;
}

#endif
