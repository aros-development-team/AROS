#undef MDEBUG
#define MDEBUG 1

#include <aros/debug.h>

#define MUNGWALL_HEADER_ID 0x1ADEBCA1

#define MUNGWALLHEADER_SIZE  (MEMCHUNK_TOTAL * 3)
#define MUNGWALL_BLOCK_SHIFT (MUNGWALLHEADER_SIZE + MUNGWALL_SIZE)
#define MUNGWALL_TOTAL_SIZE  (MUNGWALLHEADER_SIZE + MUNGWALL_SIZE * 2)

/*
 * This struct needs to be a multiple of MEMCHUNK_TOTAL for keeping proper
 *  alignment. This is why we round it up to MUNGWALLHEADER_SIZE.
 */
struct MungwallHeader
{   
    union
    {
    	struct
	{
    	    struct  MinNode 	node;
	    ULONG   	    	magicid;
    	    IPTR   	    	allocsize;
	    APTR		pool;
	} s;
	struct
	{
	    UBYTE   	    	blub[MUNGWALLHEADER_SIZE];
	} b;
    } u;    
};

#define mwh_node    	u.s.node
#define mwh_magicid 	u.s.magicid
#define mwh_allocsize 	u.s.allocsize
#define mwh_pool	u.s.pool

APTR MungWall_Build(APTR res, APTR pool, IPTR origSize, ULONG requirements, struct ExecBase *SysBase);
APTR MungWall_Check(APTR memoryBlock, IPTR byteSize, struct ExecBase *SysBase);
void MungWall_Scan(APTR pool, struct ExecBase *SysBase);
