#include <dos/dos.h>
#include <dos/filehandler.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>

#include <stdio.h>

int __nocommandline = 1;

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
        printf("DeviceNode %p <%b>\n", dn, dn->dn_Name);
        printf("Type %d, Task %p, SegList %p\n", dn->dn_Type, dn->dn_Task, dn->dn_SegList);

        if (dn->dn_Startup)
        {
            struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);

            printf("FileSysStartupMsg %p", fssm);
            if (fssm->fssm_Device)
                printf(" <%s unit %ld flags 0x%08X>", fssm->fssm_Device, fssm->fssm_Unit, fssm->fssm_Flags);
            printf("\n");

            if (fssm->fssm_Environ)
            {
                struct DosEnvec *de = BADDR(fssm->fssm_Environ);
                
                printf("DosEnvec %p DosType 0x%08lX BootPri %ld\n", de, de->de_DosType, de->de_BootPri);
            }
        }
        printf("\n");
    }

    CloseLibrary(&base->LibNode);
    return RETURN_OK;
}