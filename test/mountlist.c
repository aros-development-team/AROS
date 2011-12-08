#include <dos/dos.h>
#include <dos/filehandler.h>
#include <libraries/expansionbase.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <ctype.h>
#include <stdio.h>

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
        printf("DeviceNode %p <%s>", dn, (char *)BADDR(dn->dn_Name));

        if (IsMounted(dn))
            printf(" [MOUNTED]");

        printf("\nType %d, Task %p, SegList %p\n", (int)dn->dn_Type, BADDR(dn->dn_Task), BADDR(dn->dn_SegList));

        if (dn->dn_Startup)
        {
            struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);

            printf("FileSysStartupMsg %p", fssm);
            if (fssm->fssm_Device && fssm->fssm_Device[0])
                printf(" <%s unit %ld flags 0x%08X>", AROS_BSTR_ADDR(fssm->fssm_Device), (long)fssm->fssm_Unit, (unsigned int)fssm->fssm_Flags);
            printf("\n");

            if (fssm->fssm_Environ)
            {
                struct DosEnvec *de = BADDR(fssm->fssm_Environ);

                printf("DosEnvec %p DosType 0x%08lX <", de, de->de_DosType);
                PrintDosType(de->de_DosType);
                printf("> BootPri %ld\n", de->de_BootPri);
            }
        }
        printf("\n");
    }

    CloseLibrary(&base->LibNode);
    return RETURN_OK;
}
