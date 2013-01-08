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
    /* Does not work as expected, will leave 20 bytes unfreed */
    bug("[FVA] %p->%p,%x\n", allocation, *(((IPTR *)allocation)-1), *(((IPTR *)allocation)-2));
    FreeMem(*(((IPTR *)allocation)-1), *(((IPTR *)allocation)-2));
}

APTR AllocVecAligned(IPTR byteSize, ULONG alignmentMin, IPTR boundary) {

	struct MemHeader *memheader;
	struct MemChunk *prevchunk, *memchunk, *newchunk;

    APTR alignedMemStart = NULL, alignedMemEnd = NULL;
    IPTR bytesremoved, byteSizeRounded = AROS_ROUNDUP2(byteSize, MEMCHUNK_TOTAL);

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
                            alignedMemStart -= MEMCHUNK_TOTAL;
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
                        alignedMemStart -= MEMCHUNK_TOTAL;
                        break;
                    }
                }

                if(res != NULL) {

                    bug("[AVA] MC %p %x AVA(%x->%x,%x,%x) -> %x(%x)-%x\n", \
                        memchunk, \
                        memchunk->mc_Bytes, \
                        byteSize, \
                        byteSizeRounded, \
                        alignmentMin, \
                        (~boundary)+1, \
                        res, \
                        alignedMemStart, \
                        alignedMemEnd \
                        );

                    /* alignedMemEnd = memchunk end or near */
                    if( (((IPTR)memchunk + memchunk->mc_Bytes - 1) - (IPTR)alignedMemEnd) <=0x100 ){
                        bug("[AVA] Snap to memchunk end %p->%p \n", alignedMemEnd, memchunk + memchunk->mc_Bytes - 1);
                        alignedMemEnd = memchunk + memchunk->mc_Bytes - 1;

                        /* alignedMemStart = memchunk start */
                        if( ((IPTR) alignedMemStart - (IPTR) memchunk) <= 0x100 ) {
                            bug("[AVA] Snap to memchunk start %p->%p \n", res, memchunk);
                            alignedMemStart = memchunk;

                            /* We have allocated all that this memchunk has */
                            bytesremoved = memchunk->mc_Bytes;
                            prevchunk->mc_Next = memchunk->mc_Next;

                        }else{
                            bug("[AVA] Left some at the beginning\n");
                            /* We left some at the beginning */
                            bytesremoved = (IPTR)alignedMemEnd - (IPTR)alignedMemStart + 1;
                            memchunk->mc_Bytes -=bytesremoved; 
                        }

                    /* alignedMemEnd < memchunk end */
                    }else{

                        /* alignedMemStart = memchunk start or near */
                        if( ((IPTR) alignedMemStart - (IPTR) memchunk) <= 0x100 ) {
                            bug("[AVA] Snap to memchunk start %p->%p \n", res, memchunk);
                            alignedMemStart = memchunk;

                            bug("[AVA] We left some at the end\n");
                            /* We left some at the end */
                            bytesremoved = (IPTR)alignedMemEnd - (IPTR)alignedMemStart + 1;

                            /* Create new memchunk */
                            newchunk = alignedMemEnd + 1;
                            newchunk->mc_Next = memchunk->mc_Next;
                            newchunk->mc_Bytes = memchunk->mc_Bytes - bytesremoved;

                            /* Prevchunk points to newchunk */
                            prevchunk->mc_Next = newchunk;

                        }else{

                            bug("[AVA] We are somewhere in between\n");
                            /* We are somewhere in between */
                            bytesremoved = (IPTR)alignedMemEnd - (IPTR)alignedMemStart + 1;

                            /* Create new memchunk */
                            newchunk = alignedMemEnd + 1;
                            newchunk->mc_Next = memchunk->mc_Next;
                            newchunk->mc_Bytes = ((IPTR)memchunk + memchunk->mc_Bytes) - ((IPTR)alignedMemEnd + 1);

                            /* memchunk points to newchunk */
                            memchunk->mc_Next = newchunk;
                            memchunk->mc_Bytes -= (bytesremoved + newchunk->mc_Bytes);
                        }

                    }

    	            memheader->mh_Free -= bytesremoved;
                    //res = NULL;
                    break;
                }

            }

			/* Go to next chunk */
			prevchunk = memchunk;
			memchunk = memchunk->mc_Next;
        }

    }

    Permit();
    memset(res, 0, byteSizeRounded);

    *(((IPTR *)res)-1) = (IPTR)alignedMemStart;
    *(((IPTR *)res)-2) = (IPTR)alignedMemEnd - (IPTR)alignedMemStart + 1;

    return res;
}

#endif
