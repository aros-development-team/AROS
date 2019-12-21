#include <resources/processor.h>
#include <proto/processor.h>

#include <stdio.h>

#include "cpuspecific.h"

#ifdef __m68k__

#define FLAGS_NUM 2

static const char *features[] =
{
    "AMMX",
    "Hyperthreading"
};

void PrintCPUSpecificInfo(ULONG i, APTR ProcessorBase)
{
    ULONG icache  = 0;
    ULONG dcache  = 0;
    BOOL flags[FLAGS_NUM];
    BOOL haveExtFeats = FALSE;

    struct TagItem tags [FLAGS_NUM + 3] =
    {
	{GCIT_L1InstructionCacheSize, (IPTR)&icache   },
	{GCIT_L1DataCacheSize       , (IPTR)&dcache   },
        {GCIT_SupportsHTT	    , (IPTR)&flags[0 ]},
        {GCIT_SupportsAMMX	    , (IPTR)&flags[1 ]},
        {TAG_DONE                   , 0               }
    };

    GetCPUInfo(tags);

    for (i = 0; i < FLAGS_NUM; i++)
    {
        if (flags[i])
            haveExtFeats = TRUE;
    }

    if (haveExtFeats)
    {
        printf("\t\tFeatures: ");

        for (i = 0; i < FLAGS_NUM; i++)
        {
            if (flags[i])
            {
                printf("%s ", features[i]);
            }
        }
        printf("\n");
    }   
    if (icache > 0)
	printf("L1 I-CACHE:\t%d bytes\n", icache);
 
    if (dcache > 0)
	printf("L1 D-CACHE:\t%d bytes\n", dcache);
}

#endif
