/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_ALMOST_COMPATIBLE

#include <dos/dos.h>
#include <dos/filehandler.h>
#include <libraries/expansionbase.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef __AROS__
#define BNULL 0
#define AROS_BSTR_ADDR(s) ((char *)(s << 2) + 1)
#define AROS_BSTR_strlen(s) *((unsigned char *)(s << 2))
#endif

int __nocommandline = 1;

static void PrintDosType(ULONG dt)
{
    unsigned int i;

    for (i = 0; i < 4; i++)
    {
        unsigned char c = dt >> (24 - i * 8);

        if (isprint(c))
            putchar(c);
        else
            printf("\\%X", c);
    }
}

static BOOL IsMounted(struct DeviceNode *dn)
{
    BOOL ret = FALSE;
    struct DosList *dl = LockDosList(LDF_DEVICES|LDF_READ);
    
    while ((dl = NextDosEntry(dl, LDF_DEVICES)))
    {
        if (dl == (struct DosList *)dn)
        {
            ret = TRUE;
            break;
        }
    }

    UnLockDosList(LDF_DEVICES|LDF_READ);
    return ret;
}

int main(void)
{
    struct BootNode *n;
    struct ExpansionBase *base = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
    
    if (!base)
    {
        printf("Failed to open expansion.library!\n");
        return RETURN_FAIL;
    }
    
    ForeachNode(&base->MountList, n)
    {
        struct DeviceNode *dn = n->bn_DeviceNode;

        printf("BootNode %p, Flags 0x%08X, ConfigDev %p\n", n, n->bn_Flags, n->bn_Node.ln_Name);
        printf("DeviceNode %p <%s>", dn, AROS_BSTR_ADDR(dn->dn_Name));

        if (dn->dn_Handler)
            printf(" Handler %s", AROS_BSTR_ADDR(dn->dn_Handler));
        if (IsMounted(dn))
            printf(" [MOUNTED]");

        printf("\nType %d, Task %p, SegList %p\n", (int)dn->dn_Type, BADDR(dn->dn_Task), BADDR(dn->dn_SegList));

        if (dn->dn_Startup)
        {
            struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);

            printf("FileSysStartupMsg %p", fssm);
            if (fssm->fssm_Device != BNULL && AROS_BSTR_strlen(fssm->fssm_Device))
                printf(" <%s unit %ld flags 0x%08X>", AROS_BSTR_ADDR(fssm->fssm_Device), (long)fssm->fssm_Unit, (unsigned int)fssm->fssm_Flags);
            printf("\n");

            if (fssm->fssm_Environ)
            {
                struct DosEnvec *de = BADDR(fssm->fssm_Environ);

                printf("DosEnvec %p DosType 0x%08lX <", de, de->de_DosType);
                PrintDosType(de->de_DosType);
                printf("> BootPri %ld LowCyl %ld HighCyl %ld\n", de->de_BootPri, de->de_LowCyl, de->de_HighCyl);
            }
        }
        printf("\n");
    }

    CloseLibrary(&base->LibNode);
    return RETURN_OK;
}
