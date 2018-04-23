#include <resources/processor.h>
#include <proto/processor.h>

#include <stdio.h>

#include "cpuspecific.h"

#ifdef __m68k__

void PrintCPUSpecificInfo(ULONG i, APTR ProcessorBase)
{
    ULONG icache  = 0;
    ULONG dcache  = 0;
    struct TagItem tags [3] =
    {
	{GCIT_L1InstructionCacheSize, (IPTR)&icache   },
	{GCIT_L1DataCacheSize       , (IPTR)&dcache   },
        {TAG_DONE                   , 0               }
    };

    GetCPUInfo(tags);

    if (icache > 0)
	printf("L1 I-CACHE:\t%d bytes\n", icache);
 
    if (dcache > 0)
	printf("L1 D-CACHE:\t%d bytes\n", dcache);
}

#endif
