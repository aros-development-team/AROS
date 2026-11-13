#include <resources/processor.h>
#include <proto/processor.h>

#include <stdio.h>
#include <string.h>

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

void PrintCPUSpecificInfo(char *buffer, LONG bufsize, ULONG i, APTR ProcessorBase)
{
    LONG slen;
    char *bufptr = buffer;

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
            snprintf(bufptr, bufsize, "%s", &name[1]);
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;

            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        snprintf(bufptr, bufsize, "Unknown vendor (0x%X)", vendor);
        slen = strlen(bufptr);
        bufptr += slen;
        bufsize -= slen;
    }

    snprintf(bufptr, bufsize, "0x%X revision %d variant %d\n",
        part, ARM_REVISION(version), ARM_VARIANT(version));
    slen = strlen(bufptr);
    bufptr += slen;
    bufsize -= slen;

    if (bufsize < 1)
        return;

    found = FALSE;
    for (i = 0; bufsize > 1 && i < FLAGS_NUM; i++)
    {
    	
        if (flags[i])
        {
            found = TRUE;
            snprintf(bufptr, bufsize, "%s ", features[i]);
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;
        }
    }

    if (bufsize > 5)
    {
        if (!found)
        {
            sprintf(bufptr, "None");
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;
        }
        sprintf(bufptr, "\n");
    }
}

#endif
