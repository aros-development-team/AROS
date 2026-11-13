/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__
#define __KERNEL_NOEXTERNBASE__

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include <stdlib.h>

#include LC_LIBDEFS_FILE

#define D(x)
#define HYPERVDEBUGEXCEPTION    2

#include "hyperv-cpu.h"
#include "hyperv-tasks.h"

/*
 * AROS debug interface for Hyper-V. Use the following command in powershell
 * to send an NMI to AROS -:
 * 
 * debug-vm "Virtual Machine Name" -InjectNonMaskableInterrupt -Force
 */

static APTR KernelBase;
static char cmndbuff[1024];

/* read and return a single char */
static int DebugGetChar()
{
    LONG c;
    while ((c = RawMayGetChar()) < 0);

    return (int)c;
}

static char *HVDebug_GetInput()
{
    char inputBuffer[] = "", ch;
    int idx = 0;

    while ((ch = (DebugGetChar() & 0x7f)))
    {
        if (idx < 1023)
        {
            if (ch == 0x08)
            {
                idx -= 1;
                KrnPutChar(ch);
                KrnPutChar(' ');
                KrnPutChar(ch);
                ch = 0;
                cmndbuff[idx] = ch;
            }
            else
                cmndbuff[idx++] = ch;
        }
        /* handle enter/ctrl-x/escape */
        if ((cmndbuff[idx - 1] == 0xD) || (cmndbuff[idx - 1] == 0x18) || (cmndbuff[idx - 1] == 0x1B))
        {
            idx -= 1;
            break;
        }
        if (ch)
            KrnPutChar(ch);
    }
    cmndbuff[idx] = '\0';
    return cmndbuff;
}


void HVDebug_cmdTaskList(void *ctx, void *handlerData, char *params)
{
    //dump the Task List
    HVDEBUGDumpTasks(ctx, handlerData);
};

void HVDebug_cmdThisTask(void *ctx, void *handlerData, char *params)
{
    //dump the RUNNING Task
    HVDEBUGDumpTaskStats(handlerData, FindTask(NULL));
};

void HVDebug_cmdTask(void *ctx, void *handlerData, char *params)
{
#if (0)
    struct Task *dTask;
    char *end;

#if (__WORDSIZE == 64)
    end = &params[18];
    dTask = (struct Task *)(IPTR)strtoull(params, &end, 16);
#else
    end = &params[10];
    dTask = (struct Task *)(IPTR)strtoul(params, &end, 16);
#endif
    //dump the specified Task
    HVDEBUGDumpTaskStats(handlerData, dTask);
#endif
};

void HVDebug_cmdPrint(void *ctx, void *handlerData, char *params)
{
#if (0)
    APTR addr;
    char *end;

#if (__WORDSIZE == 64)
    end = &params[18];
    addr = (APTR)(IPTR)strtoull(params, &end, 16);
#else
    end = &params[10];
    addr = (APTR)(IPTR)strtoul(params, &end, 16);
#endif
#endif
};

void HVDebug_cmdCtx(void *ctx, void *handlerData, char *params)
{
    //dump the CPU Context
    HVDEBUGDumpCPUCtx(ctx);
};

struct HVDebugCmd
{
    char *cmdID;
    char *cmdInf;
    void (*cmdHandle)(void *ctx, void *handlerData, char *params);
} HVDebugCmdArray[] =
{
    { "ctx", "Dump the CPU context",                                            HVDebug_cmdCtx          },
    { "p 0x%p", "Print the contents of the specified location",                 HVDebug_cmdPrint        },
    { "tasklist", "Dump the complete scheduling Task Lists",                    HVDebug_cmdTaskList     },
    { "task 0x%p", "Dump the specifid task",                                    HVDebug_cmdTask         },
    { "task", "Dump the current task",                                          HVDebug_cmdThisTask     },
    { "exit", "Return to whatever was running before invoking the debugger",    NULL                    },
    { "halt", "Stop the system",                                                NULL                    },
    { NULL, NULL, NULL },
};

static BOOL HVDebug_CommandMatch(char *cmd, char *match, int *arg)
{
    BOOL retval = FALSE;
    int offs = 0;

    while (match[offs] != '\0')
    {
        if (retval && ((match[offs] == '#') || ((match[offs] == '0') && (match[offs+ 1] == 'x'))))
        {
            if (arg)
            {
                if (match[offs] == '#')
                    *arg = offs + 1;
                else
                    *arg = offs;
            }
            break;
        }
        if (cmd[offs] != match[offs])
        {
            retval = FALSE;
            break;
        }
        retval = TRUE;
        offs++;
    }
    return retval;
}

static void HVDebug_DispatchCommand(void *ctx, void *handlerData, char *cmd)
{
    int cmdNo, cmdArg;
    for (cmdNo = 0; (HVDebugCmdArray[cmdNo].cmdHandle); cmdNo++)
    {
        if (HVDebug_CommandMatch(cmd, HVDebugCmdArray[cmdNo].cmdID, &cmdArg))
            HVDebugCmdArray[cmdNo].cmdHandle(ctx, handlerData, &cmd[cmdArg]);
    }
}

/* Main Exception Handler */
static int HVExceptionHandler(void *ctx, void *handlerData, void *handlerData2)
{
    int ExitFlag = 0;
    char *cmd;

    kprintf("\nAROS Hyper-V Debug Interface v1.0 (16.12.2020)\n");

    kprintf("\n");
    kprintf("System state:\n      SysBase @ 0x%p, KernelBase @ 0x%p\n", SysBase, KernelBase);
    kprintf("        SysBase->DispCount = %u\n", SysBase->DispCount);

    while (!(ExitFlag))
    {
        kprintf("\nHVDEBUG> ");
   
        cmd = HVDebug_GetInput();
        if ((cmd[0] == 's') && (cmd[1] == 'a') && (cmd[2] == 'd'))
        {
            Debug(0);
            continue;
        }
        if ((cmd[0] == 'h') && (cmd[1] == 'a') && (cmd[2] == 'l') && (cmd[3] == 't'))
        {
            ExitFlag = 1;
            break;
        }
        if ((cmd[0] == 'e') && (cmd[1] == 'x') && (cmd[2] == 'i') && (cmd[3] == 't'))
        {
            ExitFlag = 2;
            break;
        }
        if ((cmd[0] == '?') || ((cmd[0] == 'h') && (cmd[1] == 'e') && (cmd[2] == 'l') && (cmd[3] == 'p')))
        {
            int cmdNo;
            kprintf("\nSupported Commands:");
            for (cmdNo = 0; (HVDebugCmdArray[cmdNo].cmdID); cmdNo++)
            {
                kprintf("\n%s - %s", HVDebugCmdArray[cmdNo].cmdID, HVDebugCmdArray[cmdNo].cmdInf);
            }
            continue;
        }
        kprintf("\n");
        HVDebug_DispatchCommand(ctx, handlerData, cmd);
    }

    if (ExitFlag == 1)
    {
        kprintf("Halting System, Please Power Off or Reset\n", __func__);
        while (1) asm volatile ("hlt");
    }
    else
    {
        /* return from the exception */
        kprintf("\n");
        KrnExitInterrupt(ctx);
    }
}

static LONG HVDebug_Init(LIBBASETYPE LIBBASE)
{
    KernelBase = OpenResource("kernel.resource");
    APTR ehandle;

    D(kprintf("[HyperV:DEBUG] %s()\n", __func__);)

    ehandle = KrnAddExceptionHandler(HYPERVDEBUGEXCEPTION, HVExceptionHandler, KernelBase, NULL);
    if (ehandle)
    {
        D(kprintf("[HyperV:DEBUG] %s: ExceptionHandler installed\n", __func__);)
    }

    return TRUE;
}


ADD2INITLIB(HVDebug_Init, 0)
