#include <exec/io.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include "memory.h"

#define MEM_START 0x400
#define MEM_SIZE  0x100000-0x400

struct MemHeader * detect_memory(void)
{
	struct MemHeader * mh;
	mh=(struct MemHeader*)MEM_START;
	mh->mh_Node.ln_Type    = NT_MEMORY;
	mh->mh_Node.ln_Name    = "chip memory";
	mh->mh_Node.ln_Pri     = -5;
	mh->mh_Attributes      = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA |
	                         MEMF_KICK;
	mh->mh_First           = (struct MemChunk *)((UBYTE*)mh+MEMHEADER_TOTAL);
	mh->mh_First->mc_Next  = NULL;
	mh->mh_First->mc_Bytes = MEM_SIZE - MEMHEADER_TOTAL;
	mh->mh_Lower           = mh->mh_First;
	mh->mh_Upper           = (APTR)(MEM_START + MEM_SIZE);
	mh->mh_Free            = mh->mh_First->mc_Bytes;
	
	return mh;
}
