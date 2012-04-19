/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>

int main(void)
{
     while(!CheckSignal(SIGBREAKF_CTRL_C))
     {
         int size;
         for(size = 5000; size < 6000; size++)
         {
             UBYTE *mem = AllocMem(size,MEMF_ANY);
             UBYTE *mem2 = AllocMem(size,MEMF_ANY);

             if (mem && mem2)
             {
                 int i;
                 for(i = 0; i < 10; i++)
                 {
                     //Forbid();
                     CopyMem(mem, mem2, size);
                     //Permit();

                     if (memcmp(mem, mem2, size))
                     {
                         kprintf("=== memcmp failure!!\n");
                         asm volatile("int3");
                     }
                 }
             }
             else
             {
                 kprintf("== out of mem\n");
             }
             FreeMem(mem, size);
             FreeMem(mem2, size);
         }
     }
     kprintf("== exit\n");
     return 0;
}
