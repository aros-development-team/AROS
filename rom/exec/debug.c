/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal debugger.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <ctype.h>
#include <string.h>

#include "exec_intern.h"

/****************************************************************************************/

char    GetK();
UQUAD   GetQ(char *);
ULONG   GetL(char *);
UWORD   GetW(char *);
UBYTE   GetB(char *);
int     get_irq_list(char *buf);

#if __WORDSIZE == 64
#define GetA (APTR)GetQ
#else
#define GetA (APTR)GetL
#endif

/****************************************************************************************/

static char *NextWord(char *s)
{
    /* Skip to first space or EOL */
    while (*s != ' ')
    {
        if (!*s)
            return s;
        s++;
    }

    /* Then skip to first non-space */
    while (*++s == ' ');

    return s;
}

/*****************************************************************************

    NAME */

        AROS_LH1(void, Debug,

/*  SYNOPSIS */
        AROS_LHA(unsigned long, flags, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 19, Exec)

/*  FUNCTION
        Runs SAD - internal debuger.

    INPUTS
        flags   not used. Should be 0 now.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        18-01-99    initial PC version.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    char comm[128];
    char *data;
    BOOL ignorelf = FALSE;

#ifdef KrnObtainInput
    /*
     * Try to obtain debug input from the kernel.
     * If it failed, we will hang up in RawMayGetChar(), so exit immediately.
     */
    if (!KrnObtainInput())
        return;
#endif

    RawIOInit();

    for (;;)
    {
        int i = 0;

        kprintf("SAD(%ld,%ld)>", SysBase->TDNestCnt, SysBase->IDNestCnt);

        /* Get Command code */
        do
        {
            char key = GetK(SysBase);
            BOOL t = ignorelf;

            /* We skip only single LF which immediately follows the CR. So we remember
               previous value of the flag and reset it when any character arrives. */
            ignorelf = FALSE;
            if (key == '\n') {
                if (t)
                    continue;
                else
                    break;
            }

            /* TABs are problematic to deal with, we ignore them */
            else if (key == 0x09)
                continue;

            /* If we've just got CR, we may get LF next and we'll need to skip it */
            else if (key == '\r') {
                ignorelf = TRUE;
                break;
            }

            /* Process backspace */
            else if (key == 0x08)
            {
                if (i > 0) {
                    /* Go backwards, erase the character, then go backwards again */
                    RawPutChar(key);
                    RawPutChar(' ');
                    RawPutChar(key);
                    i--;
                }
                continue;
            }

            RawPutChar(key);
            comm[i++] = key;
        }
        while (i < (int)sizeof(comm)-1);
        comm[i] = 0;
        RawPutChar('\n');

        /* Now get data for command */
        data = NextWord(comm);
        comm[2] = 0;

        /* Reboot command */
        if (strcmp(comm, "RE") == 0 && strcmp(data, "AAAAAAAA") == 0)
            ColdReboot();
        /* Restart command */
        else if (strcmp(comm, "RS") == 0 && strcmp(data, "FFFFFFFF") == 0)
            ShutdownA(SD_ACTION_COLDREBOOT);
        /* Forbid command */
        else if (strcmp(comm, "FO") == 0)
            Forbid();
        /* Permit command */
        else if (strcmp(comm, "PE") == 0)
            Permit();
        /* Disable command */
        else if (strcmp(comm, "DI") == 0)
            Disable();
        /* Show active task information */
        else if (strcmp(comm, "TI") == 0)
        {
            struct Task *t = SysBase->ThisTask;

            kprintf("Active task (%p = '%s'):\n"
                            "tc_Node.ln_Pri = %d\n"
                            "tc_SigAlloc = %04.4lx\n"
                            "tc_SPLower = %p\n"
                            "tc_SPUpper = %p\n"
                            "tc_Flags = %p\n"
                            "tc_SPReg = %p\n",
                            t, t->tc_Node.ln_Name,
                            t->tc_Node.ln_Pri,
                            t->tc_SigAlloc,
                            t->tc_SPLower,
                            t->tc_SPUpper,
                            t->tc_Flags,
                            t->tc_SPReg);                               
        }
        else if (strcmp(comm,"RI") == 0)
        {
            /*
             * TODO: this function is not useful at all in its current implementation.
             * When the task is running its context is not valid. It would be much better
             * to be able to examine contexts of other tasks.
             * I left this here for demonstration purposes.
             *
             * 24.12.2010: reference to kernel.resource's private includes is removed,
             * so PRINT_CPU_CONTEXT is not defined at all. Exec needs some CPU-specific
             * .c file where all CPU-dependent functionality needs to be gathered. This
             * is going to include full CPU context dump, stack trace, etc.
             */
#ifdef PRINT_CPU_CONTEXT
            struct ExceptionContext *r = SysBase->ThisTask->tc_UnionETask.tc_ETask->et_RegFrame;

            PRINT_CPU_CONTEXT(r);
#else
            kprintf("Not implemented on this platform.\n");
#endif
        }
        /* Enable command */
        else if (strcmp(comm, "EN") == 0)
            Enable();
        /* ShowLibs command */
        else if (strcmp(comm, "SL") == 0)
        {
            struct Node * node;

            kprintf("Available libraries:\n");

            /* Look through the list */
            for (node = GetHead(&SysBase->LibList); node; node = GetSucc(node))
            {
                kprintf("0x%p : %s\n", node, node->ln_Name);
            }
        }
        else if (strcmp(comm, "SI") == 0)
        {
/*          char buf[512];
            
            kprintf("Available interrupts:\n");
            
            get_irq_list(&buf);
            kprintf(buf);*/
            kprintf("Not implemented\n");
        }
        /* ShowResources command */
        else if (strcmp(comm, "SR") == 0)
        {
            struct Node * node;

            kprintf("Available resources:\n");

            /* Look through the list */
            for (node = GetHead(&SysBase->ResourceList); node; node = GetSucc(node))
            {
                kprintf("0x%p : %s\n", node, node->ln_Name);
            }
        }
        /* ShowDevices command */
        else if (strcmp(comm,"SD") == 0)
        {
            struct Node * node;

            kprintf("Available devices:\n");

            /* Look through the list */
            for (node=GetHead(&SysBase->DeviceList); node; node = GetSucc(node))
            {
                kprintf("0x%p : %s\n", node, node->ln_Name);
            }
        }
        /* ShowTasks command */
        else if (strcmp(comm, "ST") == 0)
        {
            struct Node * node;

            kprintf("Task List:\n");

            kprintf("0x%p T %d %s\n",SysBase->ThisTask,
                SysBase->ThisTask->tc_Node.ln_Pri,
                SysBase->ThisTask->tc_Node.ln_Name);

            /* Look through the list */
            for (node = GetHead(&SysBase->TaskReady); node; node = GetSucc(node))
            {
                kprintf("0x%p R %d %s\n", node, node->ln_Pri, node->ln_Name);
            }

            for (node = GetHead(&SysBase->TaskWait); node; node = GetSucc(node))
            {
                kprintf("0x%p W %d %s\n", node, node->ln_Pri, node->ln_Name);
            }

            kprintf("Idle called %d times\n", SysBase->IdleCount);
        }
        /* Help command */
        else if (strcmp(comm, "HE") == 0)
        {
            kprintf("SAD Help:\n");
            kprintf("RE AAAAAAAA - reboots AROS - ColdReboot()\n"
                    "RS FFFFFFFF - RESET\n"
                    "FO - Forbid()\n"
                    "PE - Permit()\n"
                    "DI - Disable()\n"
                    "EN - Enable()\n"
                    "SI - Show IRQ lines status\n"
                    "TI - Show Active task info\n"
                    "RI - Show registers inside task's context\n"
                    "AM xxxxxxxx yyyyyyyy - AllocVec - size=xxxxxxxx, "
                    "requiments=yyyyyyyy\n"
                    "FM xxxxxxxx - FreeVec from xxxxxxxx\n"
                    "RB xxxxxxxx - read byte from xxxxxxxx\n"
                    "RW xxxxxxxx - read word from xxxxxxxx\n"
                    "RL xxxxxxxx - read long from xxxxxxxx\n"
                    "WB xxxxxxxx bb - write byte bb at xxxxxxxx\n"
                    "WW xxxxxxxx wwww - write word wwww at xxxxxxxx\n"
                    "WL xxxxxxxx llllllll - write long llllllll at xxxxxxxx\n"
                    "RA xxxxxxxx ssssssss - read array(ssssssss bytes long) "
                    "from xxxxxxxx\n"
                    "RC xxxxxxxx ssssssss - read ascii (ssssssss bytes long) "
                    "from xxxxxxxx\n"
                    "QT 00000000 - quit SAD\n"
                    "SL - show all available libraries (libbase : libname)\n"
                    "SR - show all available resources (resbase : resname)\n"
                    "SD - show all available devices (devbase : devname)\n"
                    "ST - show tasks (T - this, R - ready, W - wait)\n"
                    "HE - this help.\n");
        }
        /* AllocMem command */
        else if (strcmp(comm, "AM") == 0)
        {
            ULONG size = GetL(data);
            ULONG requim = GetL(NextWord(data));

            kprintf("Allocated at 0x%p\n", AllocVec(size, requim));
        }
        /* FreeMem command */
        else if (strcmp(comm, "FM") == 0)
        {
            APTR base = GetA(&data[0]);

            kprintf("Freed at 0x%p\n", base);
            FreeVec(base);
        }
        /* ReadByte */
        else if (strcmp(comm, "RB") == 0)
        {
            UBYTE *addr = GetA(data);

            kprintf("Byte at 0x%p: %02X\n", addr, *addr);
        }
        /* ReadWord */
        else if (strcmp(comm, "RW") == 0)
        {
            UWORD *addr = GetA(data);

            kprintf("Word at 0x%p: %04X\n", addr, *addr);
        }
        /* ReadLong */
        else if (strcmp(comm, "RL") == 0)
        {
            ULONG *addr = GetA(data);

            kprintf("Long at 0x%p: %08X\n", addr, *addr);
        }
        /* WriteByte */
        else if (strcmp(comm,"WB") == 0)
        {
            UBYTE *addr = GetA(data);
            UBYTE val = GetB(NextWord(data));

            kprintf("Byte at 0x%p: %02X\n", addr, val);
            *addr = val;
        }
        /* WriteWord */
        else if (strcmp(comm, "WW") == 0)
        {
            UWORD *addr = GetA(data);
            UWORD val = GetW(NextWord(data));

            kprintf("Word at 0x%p: %04X\n", addr, val);
            *addr = val;
        }
        /* WriteLong */
        else if (strcmp(comm, "WL") == 0)
        {
            ULONG *addr = GetA(data);
            ULONG val = GetL(NextWord(data));

            kprintf("Long at 0x%p: %08X\n", addr, val);
            *addr = val;
        }
        /* ReadArray */
        else if (strcmp(comm, "RA") == 0)
        {
            UBYTE *ptr = GetA(data);
            ULONG cnt = GetL(NextWord(data));
            ULONG t;

            kprintf("Array from 0x%p (size=0x%08lX):\n", ptr, cnt);

            for(t = 1; t <= cnt; t++)
            {
                kprintf("%02X ", *ptr++);
                if(!(t % 16)) kprintf("\n");
            }
            kprintf("\n");
        }
        /* ReadASCII */
        else if (strcmp(comm, "RC") == 0)
        {
            char *ptr = GetA(data);
            ULONG cnt = GetL(NextWord(data));
            ULONG t;

            kprintf("ASCII from 0x%p (size=%08X):\n", ptr, cnt);

            for(t = 1; t <= cnt; t++)
            {
                RawPutChar(*ptr++);
                if(!(t % 70)) kprintf(" \n");
            }
            kprintf(" \n");
        }
        else if (strcmp(comm, "QT") == 0 && strcmp(data, "00000000") == 0)
        {
            kprintf("Quitting SAD...\n");

#ifdef KrnReleaseInput
            /* Release debug input */
            KrnReleaseInput();
#endif
            return;
        }
        else kprintf("?? Type HE for help\n");
    }

    AROS_LIBFUNC_EXIT
} /* Debug */

/****************************************************************************************/

char GetK(struct ExecBase *SysBase)
{
    int i;

    do
    {
        i = RawMayGetChar();
    } while(i == -1);

    return (char)i;
}

/****************************************************************************************/

UQUAD GetQ(char* string)
{
    UQUAD       ret = 0;
    int         i;
    char        digit;

    for(i = 0; i < 16; i++)
    {
        digit = toupper(string[i]);

        if (!isxdigit(digit))
            break;

        digit -= '0';
        if (digit > 9) digit -= 'A' - '0' - 10;
        ret = (ret << 4) + digit;
    }

    return ret;
}

/****************************************************************************************/

ULONG GetL(char* string)
{
    ULONG       ret = 0;
    int         i;
    char        digit;
    
    for(i = 0; i < 8; i++)
    {
        digit = toupper(string[i]);

        if (!isxdigit(digit))
            break;

        digit -= '0';
        if (digit > 9) digit -= 'A' - '0' - 10;
        ret = (ret << 4) + digit;
    }

    return ret;
}

/****************************************************************************************/

UWORD GetW(char* string)
{
    UWORD       ret = 0;
    int         i;
    char        digit;
    
    for(i = 0; i < 4; i++)
    {
        digit = toupper(string[i]);

        if (!isxdigit(digit))
            break;

        digit -= '0';
        if (digit > 9) digit -= 'A' - '0' - 10;
        ret = (ret << 4) + digit;
    }

    return ret;
}

/****************************************************************************************/

UBYTE GetB(char* string)
{
    UBYTE       ret = 0;
    int         i;
    char        digit;
    
    for(i = 0; i < 2; i++)
    {
        digit = toupper(string[i]);

        if (!isxdigit(digit))
            break;

        digit -= '0';
        if (digit > 9) digit -= 'A' - '0' - 10;
        ret = (ret << 4) + digit;
    }

    return ret;
}
