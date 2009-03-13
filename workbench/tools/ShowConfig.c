#include <aros/bootloader.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/aros.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdio.h>

ULONG ExtUDivMod32(ULONG a, ULONG b, ULONG *mod)
{
    *mod = a % b;

    return a/b;
}

void PrintNum(ULONG num)
{
    /* MBytes ? */
    if(num > 1023) 
    {
	ULONG  x, xx;
	char* fmt = "meg";
	
	/* GBytes ? */
	if(num > 0xfffff)
	{ 
	    num >>= 10; 
	    fmt = "gig";
	}
	
	num = ExtUDivMod32(UMult32(num, 100) >> 10, 100, &x);
	
	/* round */
	x = ExtUDivMod32(x, 10, &xx);
	
	if(xx > 4)
	{
	    if(++x > 9)
	    {
		x = 0;
		num++;
	    }
	}

        printf("%d.%d %s", num, x, fmt);
    }
    else 
    {
        printf("%d K", num);
    }
}

ULONG ComputeKBytes(APTR a, APTR b)
{
    IPTR result = b - a;

    return (ULONG)(result >> 10);
}

int __nocommandline;
char __stdiowin[]="CON:///500//AUTO/CLOSE";

int main()
{
    struct MemHeader *mh;
    APTR BootLoaderBase;
    STRPTR bootldr;
    struct List *args;
    struct Node *n;
    
    printf("VERS:\tAROS version %d.%d, Exec version %d.%d\n", ArosBase->lib_Version, ArosBase->lib_Revision,
	   SysBase->LibNode.lib_Version, SysBase->LibNode.lib_Revision);
    
    printf("RAM:");
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head; mh->mh_Node.ln_Succ; mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        char *memtype = "ROM";

        if (mh->mh_Attributes & MEMF_CHIP)
            memtype = "CHIP";
        if (mh->mh_Attributes & MEMF_FAST)
            memtype = "FAST";
        printf("\tNode Type 0x%X, Attributes 0x%X (%s), at %p-%p (", mh->mh_Node.ln_Type, mh->mh_Attributes, memtype, mh->mh_Lower, mh->mh_Upper);
        PrintNum(ComputeKBytes(mh->mh_Lower, mh->mh_Upper));
        printf(")\n");
    }

    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase) {
	bootldr = GetBootInfo(BL_LoaderName);

	if (bootldr)
    	    printf("BOOTLDR:\t%s\n", bootldr);

	args = GetBootInfo(BL_Args);
	if (args) {
            printf("ARGS:\t");
            for (n = args->lh_Head; n->ln_Succ; n = n->ln_Succ) {
        	printf("%s ", n->ln_Name);
        	printf("\n");
            }
	}
    }
    return 0;
}
