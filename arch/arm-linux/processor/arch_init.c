#include <aros/symbolsets.h>
#include <resources/processor.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/hostlib.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "arch_intern.h"
#include "processor_intern.h"

static char *getval(const char *key, char *s)
{
    while (*key)
    {
        if (*key++ != *s++)
            return NULL;
    }

    /* Skip whitespaces and colon */
    while (*s && (isspace(*s) || (*s == ':')))
        s++;

    return s;
}

static ULONG ParseFlags(char *opts, const char * const *FlagNames)
{
    ULONG ret = 0;
    unsigned int i;

    while (*opts)
    {
        /* Remember beginning of the word */
        char *p = opts;

        /* Find end of the word */
        while (*opts && (!isspace(*opts)))
            opts++;

        if (*opts)
        {
            /*
             * If this is not the end of line, split the string and advance to the next char.
             * Feature names are separated with one whitespace.
             */
            *opts++ = 0;
        }

        /* Decode flag name */
        for (i = 0; FlagNames[i]; i++)
        {
            if (!strcmp(p, FlagNames[i]))
            {
                /* Set bit corresponding to number of flag name in the list */
                ret |= (1UL << i);
                break;
            }
        }
    }

    return ret;
}

/*
 * This is what can be reported by Linux kernel (see their arch/arm/kernel/setup.c).
 * Order of these must be the same as order of family IDs.
 */
static const char *arch_ids[] =
{
    "3",
    "4",
    "4T",
    "5",
    "5T",
    "5TE",
    "5TEJ",
    "6TEJ",
    "7",
    NULL
};

/*
 * Feature flags as reported by kernel (quote not everything is supported).
 * Again, order of these repeats order of feature ID tags.
 */
static const char *feature_ids[] =
{
    "vfp",
    "vfpv3",
    "neon",
    "thumb",
    "thumbee",
    NULL
};

#ifdef HOST_OS_android
#define LIBC_NAME "libc.so"
#else
#define LIBC_NAME "libc.so.6"
#endif

static const char *libc_symbols[] =
{
    "fopen",
    "fclose",
    "fgets",
    NULL
};

/*
 * Unfortunately CPU identification instructions on ARM requires supervisor privileges,
 * so we can't use them on hosted. We can only ask host OS about our CPU.
 * On Linux (and Android) we can do this only by parsing human-readable text in /proc/cpuinfo.
 * We don't use unixio.hidd here because we want to start up as early as possible.
 */
static ULONG arch_Init(struct ProcessorBase *ProcessorBase)
{
    APTR HostLibBase, libc;
    struct LibCInterface *iface;
    ULONG i;
    struct LinuxArmProcessor *data = NULL;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
        return FALSE;

    libc = HostLib_Open(LIBC_NAME, NULL);
    if (!libc)
        return FALSE;

    iface = (struct LibCInterface *)HostLib_GetInterface(libc, libc_symbols, &i);
    if (iface)
    {
        if (!i)
        {
            char *buf = AllocMem(BUFFER_SIZE, MEMF_ANY);

            if (buf)
            {
                data = AllocMem(sizeof(struct LinuxArmProcessor), MEMF_CLEAR);
                if (data)
                {
                    UWORD variant  = 0;
                    UWORD revision = 0;
                    void *file;

                    HostLib_Lock();
                    file = iface->fopen("/proc/cpuinfo", "r");
                    HostLib_Unlock();

                    if (file)
                    {
                        char *res;

                        do
                        {
                            HostLib_Lock();
                            res = iface->fgets(buf, BUFFER_SIZE - 1, file);
                            HostLib_Unlock();

                            if (res)
                            {
                                char *val;

                                i = strlen(buf);
                                if (i)
                                {
                                    /* Strip the newline */
                                    i--;
                                    if (buf[i] == 0x0A)
                                        buf[i] = 0;

                                    if ((val = getval("Processor", buf)))
                                    {
                                        data->Model = StrDup(val);
                                    }
                                    else if ((val = getval("Features", buf)))
                                    {
                                        data->Features = ParseFlags(val, feature_ids);
                                    }
                                    else if ((val = getval("CPU implementer", buf)))
                                    {
                                        data->Implementer = strtoul(val, NULL, 0);
                                    }
                                    else if ((val = getval("CPU architecture", buf)))
                                    {
                                        ULONG family;

                                        for (family = 0; arch_ids[family]; family++)
                                        {
                                            if (!strcmp(arch_ids[family], val))
                                            {
                                                data->Arch = family + CPUFAMILY_ARM_3;
                                                break;
                                            }
                                        }
                                    }
                                    else if ((val = getval("CPU variant", buf)))
                                    {
                                        variant = strtoul(val, NULL, 0);
                                    }
                                    else if ((val = getval("CPU part", buf)))
                                    {
                                        data->Part = strtoul(val, NULL, 0);
                                    }
                                    else if ((val = getval("CPU revision", buf)))
                                    {
                                        revision = strtoul(val, NULL, 0);
                                    }
                                }
                            }
                        } while (res);

                        HostLib_Lock();
                        iface->fclose(file);
                        HostLib_Unlock();
                    }
                    data->Version = (revision << 16) | variant;
                }

                FreeMem(buf, BUFFER_SIZE);
            }
        }
        HostLib_DropInterface((void **)iface);
    }

    HostLib_Close(libc, NULL);

    ProcessorBase->Private1 = data;
    if (data)
    {
        if (data->Features & (FF_VFP | FF_VFPv3))
        {
            /*
             * Perhaps rather late, but i really would not like to duplicate this
             * code in kernel.resource.
             * Actually this definition can be used only by math code, i hope no
             * floating-point operations are performed at early startup.
             */
            SysBase->AttnFlags |= AFF_FPU;
        }
        return TRUE;
    }
    else
        return FALSE;
}

ADD2INITLIB(arch_Init, 1);
