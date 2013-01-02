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

APTR AllocVecAligned(IPTR byteSize, ULONG boundary, ULONG alignmentMin) {

	struct MemHeader *memheader;
	struct MemChunk *memchunk, *prevchunk;

    APTR alignedMemStart,alignedMemEnd;
    APTR res = NULL;

    /*
        We only allocate from MEMF_FAST
    */
    ULONG requirements = MEMF_FAST;

    bug("AllocVecAligned(%d,%x,%d)\n", byteSize, boundary, alignmentMin);

    /*
        Minimum alignment is MEMCHUNK_TOTAL
    */
    alignmentMin = AROS_ROUNDUP2(alignmentMin, MEMCHUNK_TOTAL);

    bug("AllocVecAligned(%d,%x,%d)\n", byteSize, boundary, alignmentMin);

    Forbid();

    ForeachNode(&SysBase->MemList, memheader) {

	    if ( (requirements & ~memheader->mh_Attributes) )
	        continue;

		prevchunk = (struct MemChunk *)(&memheader->mh_First);
		memchunk = memheader->mh_First;

        while (memchunk != NULL) {
            /*
                We need more than the requested size (IPTR address and IPTR size)
            */
            if(memchunk->mc_Bytes > AROS_ROUNDUP2((byteSize + sizeof(IPTR)*2), MEMCHUNK_TOTAL)) {

                bug("Chunk %p %d\n", memchunk, memchunk->mc_Bytes);

                /* We are at MEMCHUNK_TOTAL alignment (or should be...), preserve space for internal allocation address and size and align to alignmentMin */
                alignedMemStart = (APTR) AROS_ROUNDUP2(((IPTR)memchunk + sizeof(IPTR)*2), alignmentMin);

                alignedMemEnd = alignedMemStart + byteSize;

                /* Check if the allocation is still within the chunk */
                while(alignedMemEnd <= (APTR) (memchunk+memchunk->mc_Bytes) ) {

                    /* Check if we can fit the allocation within boundary (we didn't round the bytesize at the start because of this comparison) */
                    if( ((IPTR) alignedMemStart & ~(boundary-1)) == ((IPTR) alignedMemEnd & ~(boundary-1))) {
                        bug("Aligned %p-%p\n", alignedMemStart, alignedMemEnd);
                        /* Do the bad thing and alter memchunks... */
                        break;
                    } else {
                        /* Nope! Adjust the alignedMemStart to boundary */
                        bug("Nope!\n");
                        alignedMemStart = (APTR) AROS_ROUNDUP2((IPTR) alignedMemStart, boundary);
                        alignedMemEnd = alignedMemStart + byteSize; 
                    }
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
