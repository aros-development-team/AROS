#include <resources/processor.h>
#include <proto/processor.h>

#include <stdio.h>

#include "cpuspecific.h"

#ifdef __arm__

/* First byte of these strings is implementer ID */
static const char *vendors[] =
{
    "\x41" "ARM Ltd",
    "\x44" "Digital Equipment Corp",
    "\x4D" "Freescale Semiconductor",
    "\x51" "Qualcomm",
    "\x56" "Marvell Inc",
    "\x69" "Intel Corporation",
    NULL
};

#define FLAGS_NUM 5

static const char *features[] =
{
    "VFP",
    "VFPv3",
    "Neon",
    "Thumb",
    "ThumbEE"
};

void PrintCPUSpecificInfo(ULONG i, APTR ProcessorBase)
{
    BOOL found;
    ULONG vendor  = 0;
    ULONG part    = 0;
    ULONG version = 0;
    BOOL flags[FLAGS_NUM];
    struct TagItem tags [FLAGS_NUM + 5] =
    {
        {GCIT_SelectedProcessor     , i               },
        {GCIT_Model                 , (IPTR)&part     },
        {GCIT_Version               , (IPTR)&version  },
        {GCIT_Vendor                , (IPTR)&vendor   },
        {GCIT_SupportsVFP	    , (IPTR)&flags[0 ]},
        {GCIT_SupportsVFPv3	    , (IPTR)&flags[1 ]},
        {GCIT_SupportsNeon	    , (IPTR)&flags[2 ]},
        {GCIT_SupportsThumb	    , (IPTR)&flags[3 ]},
        {GCIT_SupportsThumbEE	    , (IPTR)&flags[4 ]},
        {TAG_DONE                   , 0               }
    };

    GetCPUInfo(tags);

    found = FALSE;
    for (i = 0; vendors[i]; i++)
    {
        const char *name = vendors[i];
        
        if (name[0] == vendor)
        {
            printf("\t\t%s", &name[1]);
            found = TRUE;
            break;
        }
    }

    if (!found)
        printf("\t\tUnknown vendor (0x%X)", vendor);

    printf(" 0x%X revision %d variant %d\n", part, ARM_REVISION(version), ARM_VARIANT(version));

    printf("\t\tFeatures: ");

    found = FALSE;
    for (i = 0; i < FLAGS_NUM; i++)
    {
    	
        if (flags[i])
        {
            found = TRUE;
            printf("%s ", features[i]);
        }
    }
    
    if (!found)
        printf("None");
    printf("\n");
}

#endif
