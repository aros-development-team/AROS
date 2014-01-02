/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <stdio.h>

#include <resources/filesysres.h>

static void checkpatch(struct FileSysEntry *fse, UBYTE patchid)
{
    printf ("%c ", (fse->fse_PatchFlags & (1 << patchid)) ? '*' : ' ');
}

int main(int argc, char **argv)
{
    struct FileSysResource *fsr;
    
    fsr = OpenResource(FSRNAME);
    if (fsr)
    {
    	struct FileSysEntry *fse;
    	WORD cnt;

    	printf("FileSysResource at %p\n", fsr);
    	printf("Creator: '%s'\n", fsr->fsr_Creator);
    	printf("\n");
    	cnt = 0;
    	ForeachNode(&fsr->fsr_FileSysEntries, fse)
    	{
    	    char dostype[5];
    	    WORD i;

    	    printf("FileSysEntry %d at %p\n", cnt++, fse);
	    for (i = 0; i < 4; i++)
	    {
	    	dostype[i] = (fse->fse_DosType >> ((3 - i) * 8)) & 0xff;
		if (dostype[i] < 9)
		    dostype[i] += '0';
		else if (dostype[i] < 32)
		    dostype[i] = '.';
	    }
	    dostype[4] = 0;
    	    printf("DOSType     : %08lx (%s)\n", (unsigned long)fse->fse_DosType, dostype);
    	    printf("Version     : %08lx (%d.%d)\n", (unsigned long)fse->fse_Version, (int)(fse->fse_Version >> 16), (int)(fse->fse_Version & 0xffff));
    	    printf("PatchFlags  : %08lx\n", (unsigned long)fse->fse_PatchFlags);
    	    checkpatch(fse, FSEB_TYPE);
   	    printf("Type      : %08lx\n", (unsigned long)fse->fse_Type);
    	    checkpatch(fse, FSEB_TASK);
   	    printf("Task      : %p\n", (void *)fse->fse_Task);
    	    checkpatch(fse, FSEB_LOCK);
   	    printf("Lock      : %p\n", (APTR)fse->fse_Lock);
    	    checkpatch(fse, FSEB_HANDLER);
   	    printf("Handler   : %p (%s)\n", (APTR)fse->fse_Handler, AROS_BSTR_ADDR(fse->fse_Handler));
    	    checkpatch(fse, FSEB_STACKSIZE);
   	    printf("StackSize : %u\n", (unsigned int)fse->fse_StackSize);
    	    checkpatch(fse, FSEB_PRIORITY);
   	    printf("Priority  : %d\n", (int)fse->fse_Priority);
    	    checkpatch(fse, FSEB_STARTUP);
   	    printf("Startup   : %p\n", (APTR)fse->fse_Startup);
    	    checkpatch(fse, FSEB_SEGLIST);
   	    printf("SegList   : %p\n", (APTR)fse->fse_SegList);
    	    checkpatch(fse, FSEB_GLOBALVEC);
   	    printf("GlobalVec : %ld\n", (long)fse->fse_GlobalVec);
	    printf("\n");
   	}
    } else {
    	printf("FileSystem.resource failed to open!\n");
    }
    return 0;
}
