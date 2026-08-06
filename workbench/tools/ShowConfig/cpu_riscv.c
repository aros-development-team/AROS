#include <resources/processor.h>
#include <proto/processor.h>

#include <stdio.h>

#include "cpuspecific.h"

#ifdef __riscv

static const struct
{
    ULONG id;
    const char *name;
} vendors[] =
{
    { RISCV_VENDOR_ANDES,  "Andes Technology" },
    { RISCV_VENDOR_SIFIVE, "SiFive"           },
    { RISCV_VENDOR_THEAD,  "T-Head"           },
    { 0, NULL }
};

#define FLAGS_NUM 9

static const char *features[] =
{
    "M",
    "A",
    "F",
    "D",
    "C",
    "V",
    "Zba",
    "Zbb",
    "Zbs"
};

void PrintCPUSpecificInfo(ULONG i, APTR ProcessorBase)
{
    BOOL found;
    ULONG vendor = 0;
    ULONG archid = 0;
    ULONG impid  = 0;
    ULONG hartid = 0;
    CONST_STRPTR isastring = NULL;
    BOOL flags[FLAGS_NUM];
    struct TagItem tags [] =
    {
        {GCIT_Vendor        , (IPTR)&vendor   },
        {GCIT_Model         , (IPTR)&archid   },
        {GCIT_Version       , (IPTR)&impid    },
        {GCIT_PhysicalID    , (IPTR)&hartid   },
        {GCIT_ISAString     , (IPTR)&isastring},
        {GCIT_SupportsRVM   , (IPTR)&flags[0] },
        {GCIT_SupportsRVA   , (IPTR)&flags[1] },
        {GCIT_SupportsRVF   , (IPTR)&flags[2] },
        {GCIT_SupportsRVD   , (IPTR)&flags[3] },
        {GCIT_SupportsRVC   , (IPTR)&flags[4] },
        {GCIT_SupportsRVV   , (IPTR)&flags[5] },
        {GCIT_SupportsZba   , (IPTR)&flags[6] },
        {GCIT_SupportsZbb   , (IPTR)&flags[7] },
        {GCIT_SupportsZbs   , (IPTR)&flags[8] },
        {TAG_DONE           , 0               }
    };

    if (!GetCoreInfo(i, tags))
        return;

    found = FALSE;
    for (i = 0; vendors[i].name; i++)
    {
        if (vendors[i].id == vendor)
        {
            printf("\t\t%s", vendors[i].name);
            found = TRUE;
            break;
        }
    }

    if (!found)
        printf("\t\tUnknown vendor (0x%X)", (unsigned int)vendor);

    printf(" hart %u archid 0x%X impid 0x%X\n", (unsigned int)hartid,
           (unsigned int)archid, (unsigned int)impid);

    if (isastring)
        printf("\t\tISA: %s\n", isastring);

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
