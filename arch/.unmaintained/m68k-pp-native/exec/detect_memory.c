#include <exec/io.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include "memory.h"

#define DBEUG 1
#include <aros/debug.h>

#include "pp_exec_internal.h"

#define MEM_START 0x400


/*
 * Detect memory in step sizes of 1kb. Keep it that way...
 */
#define STEPSIZE 1024 

UBYTE * check_memory(UBYTE * address) 
{
	int found = FALSE;
	*(ULONG *)ADDRESS_ERROR_MARKER_ADDRESS = 0;
	while (1) {
		*(UBYTE *)address = 0xcc;
		if (0xcc != *(UBYTE *)address) {
			break;
		}
		if (0 == *(ULONG *)ADDRESS_ERROR_MARKER_ADDRESS) {
			found = TRUE;
		} else {
			break;
		}
		address += STEPSIZE;
	}
	address -= STEPSIZE;
	
	if (FALSE == found)
		return NULL;
	return address;
}

/*
 * Detect some initial memory. This should be enough
 * to get the OS up and running. More detection follows
 * later...
 */
struct MemHeader * detect_memory(void)
{
	struct MemHeader * mh;
	/*
	 * There is no SysBase available here!!
	 * It has not been initialized, yet.
	 */

	/*
	 * Must initialize the BusError handler
	 */
	UBYTE * address = (UBYTE *)1024;

	*(ULONG *)(2*4) = (ULONG)dm_bus_error_handler;
	*(ULONG *)(3*4) = (ULONG)dm_addr_error_handler;

	address = check_memory(address);

	mh=(struct MemHeader*)MEM_START;
	mh->mh_Node.ln_Succ    = NULL;
	mh->mh_Node.ln_Pred    = NULL;
	mh->mh_Node.ln_Type    = NT_MEMORY;
	mh->mh_Node.ln_Name    = "chip memory";
	mh->mh_Node.ln_Pri     = -5;
	mh->mh_Attributes      = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA |
	                         MEMF_KICK;
	mh->mh_First           = (struct MemChunk *)((UBYTE*)mh+MEMHEADER_TOTAL);
	mh->mh_First->mc_Next  = NULL;
	mh->mh_First->mc_Bytes = ((ULONG)address - MEM_START) - MEMHEADER_TOTAL;

	mh->mh_Lower           = mh->mh_First;
	mh->mh_Upper           = (APTR)(address);
	mh->mh_Free            = mh->mh_First->mc_Bytes;

	return mh;
}

/*
 * The locations where memory can be. 
 * Beware: Whatever is at 0x0 is also at 0x10000000 for 1MB.
 *         So I am trying to avoid detecting this part again
 *         and start later in memory.
 */
struct memories 
{
	UBYTE * start;
	UBYTE   pri;
};

static struct memories memory[] = {{(UBYTE *)0x10100000,-10},
                                   {(UBYTE *)0xf0000000,-10},
                                   {(UBYTE *)NULL,0}};

/*
 * Detect the rest of the memory available on this device.
 */
void detect_memory_rest(struct ExecBase * SysBase)
{
	int c = 0;
	*(ULONG *)(2*4) = (ULONG)dm_bus_error_handler;
	*(ULONG *)(3*4) = (ULONG)dm_addr_error_handler;

	while (0 != memory[c].start) {
		UBYTE * end;
		/*
		 * Now try to detect sram size
		 */
		end = check_memory(memory[c].start);
		if (end != memory->start[c] && NULL != end) {
			AddMemList((ULONG)end-(ULONG)memory[c].start,
			           MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA | MEMF_KICK,
			           memory[c].pri,
			           memory[c].start,
			           "fast memory");
		}
		c++;
	}
}
