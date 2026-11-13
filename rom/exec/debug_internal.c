#include <libraries/debug.h>
#include <proto/debug.h>

#include <string.h>

#include "debug_internal.h"
#include "exec_intern.h"
#include "exec_util.h"

/****************************************************************************************/

char *NextWord(char *s)
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

/*****************************************************************************/

/* Print a title, then the address and name of every node in a list */
static void ShowNodes(const char *title, struct List *list)
{
    struct Node * node;

    kprintf(title);

    /* Look through the list */
    for (node = GetHead(list); node; node = GetSucc(node))
    {
        kprintf("0x%p : %s\n", node, node->ln_Name);
    }
}

/* A command is the first two letters of the input line */
#define SADCMD(a, b) ((UWORD)(((UBYTE)(a) << 8) | (UBYTE)(b)))

void InternalDebug(void *stack)
{
    char comm[128];
    char *data;
    BOOL ignorelf = FALSE;
    UWORD cmd;

    /*
     * Try to obtain debug input from the kernel.
     * If it failed, we will hang up in RawMayGetChar(), so exit immediately.
     */
    if (!KrnObtainInput())
        return;

    struct Task *me = FindTask(NULL);

    RawIOInit();

    if (stack) {
        kprintf("Entering SAD. Quitting will resume execution at %p.\n",
                *(void **)stack);
    }
    for (;;)
    {
        int i = 0;

        kprintf("SAD(%d,%d)>", TDNESTCOUNT_GET, IDNESTCOUNT_GET);

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

        /* An empty or one-letter line gives a code which no command has */
        cmd = comm[0] ? SADCMD(comm[0], comm[1]) : 0;

        /* Reboot command */
        if (cmd == SADCMD('R','E') && strcmp(data, "AAAAAAAA") == 0)
            ColdReboot();
        /* Restart command */
        else if (cmd == SADCMD('R','S') && strcmp(data, "FFFFFFFF") == 0)
            ShutdownA(SD_ACTION_REBOOT);
        /* Forbid command */
        else if (cmd == SADCMD('F','O'))
            Forbid();
        /* Permit command */
        else if (cmd == SADCMD('P','E'))
            Permit();
        /* Disable command */
        else if (cmd == SADCMD('D','I'))
            Disable();
        /* Show task information */
        else if (cmd == SADCMD('T','I'))
        {
            struct Task *t = GetA(data);

            if (!t) {
              t = me;
            }
            if (!Exec_CheckTask(t, SysBase))
            {
                kprintf("Task 0x%p not found\n", t);
                continue;
            }

            kprintf("Task status (%p = '%s'):\n"
                            "tc_Node.ln_Pri = %d\n"
                            "tc_Flags       = %02x\n"
                            "tc_SigAlloc    = %04.4lx\n"
                            "tc_SigWait     = %04.4lx\n"
                            "tc_SPLower     = %p\n"
                            "tc_SPReg       = %p\n"
                            "tc_SPUpper     = %p\n"
                            "tc_IDNestCnt   = %d\n"
                            "tc_TDNestCnt   = %d\n",
                            t, t->tc_Node.ln_Name,
                            t->tc_Node.ln_Pri,
                            t->tc_Flags,
                            t->tc_SigAlloc,
                            t->tc_SigWait,
                            t->tc_SPLower,
                            t->tc_SPReg,
                            t->tc_SPUpper,
                            t->tc_IDNestCnt,
                            t->tc_TDNestCnt);
        }
        else if (cmd == SADCMD('R','I'))
        {
            struct Task *t = GetA(data);

            if (!t) {
              t = me;
            }
            if (!Exec_CheckTask(t, SysBase))
            {
                kprintf("Task 0x%p not found\n", t);
                continue;
            }

            kprintf("Task context (%p = '%s'):\n", t, t->tc_Node.ln_Name);
            FormatCPUContext(NULL, t->tc_UnionETask.tc_ETask->et_RegFrame, SysBase);
            /* If inspecting the task that called Debug, also show SP and PC
               from the time of the Debug call as that is likely more useful. */
            if ((t == me) && stack) {
                kprintf("Return address after SAD exit: %p (on stack address %p)\n",
                        *(void **)stack, stack);
            }
            RawPutChar('\n');
        }
        /* Enable command */
        else if (cmd == SADCMD('E','N'))
            Enable();
        /* ShowLibs command */
        else if (cmd == SADCMD('S','L'))
            ShowNodes("Available libraries:\n", &SysBase->LibList);
        else if (cmd == SADCMD('S','I'))
        {
/*          char buf[512];
            
            kprintf("Available interrupts:\n");
            
            get_irq_list(&buf);
            kprintf(buf);*/
            kprintf("Not implemented\n");
        }
        /* ShowResources command */
        else if (cmd == SADCMD('S','R'))
            ShowNodes("Available resources:\n", &SysBase->ResourceList);
        /* ShowDevices command */
        else if (cmd == SADCMD('S','D'))
            ShowNodes("Available devices:\n", &SysBase->DeviceList);
        /* ShowTasks command */
        else if (cmd == SADCMD('S','T'))
        {
            struct Node * node;

            kprintf("Task List:\n");

#if defined(__AROSEXEC_SMP__)
            ForeachNode(&PrivExecBase(SysBase)->TaskRunning, node)
            {
#else
            node = (struct Node *)GET_THIS_TASK;
#endif
            kprintf("0x%p T %d %s\n", node, node->ln_Pri, node->ln_Name);
#if defined(__AROSEXEC_SMP__)
            }
#endif
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
        else if (cmd == SADCMD('H','E'))
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
                    "RI xxxxxxxx - Show registers inside task's context\n"
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
                    "SS xxxxxxxx - show symbol for xxxxxxxx\n"
                    "ST - show tasks (T - this, R - ready, W - wait)\n"
                    "HE - this help.\n");
        }
        /* AllocMem command */
        else if (cmd == SADCMD('A','M'))
        {
            ULONG size = GetL(data);
            ULONG requim = GetL(NextWord(data));

            kprintf("Allocated at 0x%p\n", AllocVec(size, requim));
        }
        /* FreeMem command */
        else if (cmd == SADCMD('F','M'))
        {
            APTR base = GetA(&data[0]);

            kprintf("Freed at 0x%p\n", base);
            FreeVec(base);
        }
        /* ReadByte */
        else if (cmd == SADCMD('R','B'))
        {
            UBYTE *addr = GetA(data);

            kprintf("Byte at 0x%p: %02X\n", addr, *addr);
        }
        /* ReadWord */
        else if (cmd == SADCMD('R','W'))
        {
            UWORD *addr = GetA(data);

            kprintf("Word at 0x%p: %04X\n", addr, *addr);
        }
        /* ReadLong */
        else if (cmd == SADCMD('R','L'))
        {
            ULONG *addr = GetA(data);

            kprintf("Long at 0x%p: %08X\n", addr, *addr);
        }
        /* WriteByte */
        else if (cmd == SADCMD('W','B'))
        {
            UBYTE *addr = GetA(data);
            UBYTE val = GetB(NextWord(data));

            kprintf("Byte at 0x%p: %02X\n", addr, val);
            *addr = val;
        }
        /* WriteWord */
        else if (cmd == SADCMD('W','W'))
        {
            UWORD *addr = GetA(data);
            UWORD val = GetW(NextWord(data));

            kprintf("Word at 0x%p: %04X\n", addr, val);
            *addr = val;
        }
        /* WriteLong */
        else if (cmd == SADCMD('W','L'))
        {
            ULONG *addr = GetA(data);
            ULONG val = GetL(NextWord(data));

            kprintf("Long at 0x%p: %08X\n", addr, val);
            *addr = val;
        }
        /* ReadArray */
        else if (cmd == SADCMD('R','A'))
        {
            UBYTE *ptr = GetA(data);
            ULONG cnt = GetL(NextWord(data));
            ULONG t;

            kprintf("Array from 0x%p (size=0x%08X):\n", ptr, cnt);

            for(t = 1; t <= cnt; t++)
            {
                kprintf("%02X ", *ptr++);
                if(!(t % 16)) kprintf("\n");
            }
            kprintf("\n");
        }
        /* ReadASCII */
        else if (cmd == SADCMD('R','C'))
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
        else if (cmd == SADCMD('S','S')) {
            char *ptr = GetA(data);
            STRPTR modname = "(unknown)";
            STRPTR symname = "(unknown)";
            APTR   sym_l   = (APTR)(IPTR)0;
            APTR   sym_h   = (APTR)~(IPTR)0;
            struct TagItem tags[] = {
                { DL_ModuleName, (IPTR)&modname },
                { DL_SymbolName, (IPTR)&symname },
                { DL_SymbolStart, (IPTR)&sym_l  },
                { DL_SymbolEnd, (IPTR)&sym_h },
                { TAG_END }
            };

            if (DebugBase) {
                DecodeLocationA(ptr, tags);
            }

            kprintf("%p:%p %s %s+0x%p\n", sym_l, sym_h, modname, symname, (APTR)ptr - (APTR)sym_l);
        }
        else if (cmd == SADCMD('Q','T') && strcmp(data, "00000000") == 0)
        {
            kprintf("Quitting SAD...\n");
            KrnReleaseInput(); /* Release debug input */
            return;
        }
        else kprintf("?? Type HE for help\n");
    }
}

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

/* Parse up to 'digits' hexadecimal digits; stops at the first other character */
static IPTR GetHex(char *string, int digits)
{
    IPTR        ret = 0;

    while (digits-- > 0)
    {
        char digit = *string++;

        if (digit >= 'a' && digit <= 'f')
            digit -= 'a' - 'A';

        if (!((digit >= '0' && digit <= '9') || (digit >= 'A' && digit <= 'F')))
            break;

        digit -= '0';
        if (digit > 9) digit -= 'A' - '0' - 10;
        ret = (ret << 4) + digit;
    }

    return ret;
}

/****************************************************************************************/

#if __WORDSIZE == 64
UQUAD GetQ(char* string)
{
    return GetHex(string, 16);
}

/****************************************************************************************/
#endif

ULONG GetL(char* string)
{
    return GetHex(string, 8);
}

/****************************************************************************************/

UWORD GetW(char* string)
{
    return GetHex(string, 4);
}

/****************************************************************************************/

UBYTE GetB(char* string)
{
    return GetHex(string, 2);
}
