#include "filereq.h"

#ifndef __AROS__
APTR REGARGS AllocVecPooled (APTR pool, ULONG memsize)
{
    if (pool)
    {
	IPTR *mem;

	memsize += sizeof(IPTR);

	if ((mem = AllocPooled(pool, memsize)))
	{
	    *mem++ = memsize;
	}

	return mem;
    }
    else
    {
        return AllocVec(memsize, MEMF_PUBLIC | MEMF_CLEAR);
    }
}

void REGARGS FreeVecPooled (APTR pool, APTR mem)
{
    if (mem)
    {
	if (pool)
	{
	    IPTR *imem = (IPTR *)mem;
	    IPTR size = *--imem;
 
	    FreePooled(pool, imem, size);
        }
	else
	{
             FreeVec(mem);
	}
    }
}
#endif
