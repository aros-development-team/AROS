/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Host-side hepler code for Windows emul.handler
    Lang: english
*/

#define _WIN32_IE 0x0500
#include <windows.h>
#include <shlobj.h>

#include <stdio.h>
#include <aros/irq.h>

#include "emul_host.h"

#define DWINAPI(x)   /* WinAPI calls debug            */
#define DASYNC(x)    /* Asynchronous I/O thread debug */

HMODULE kernel_lib;
void (*CauseIRQ)(unsigned char irq, void *data);

/*********************************************************************************************/

unsigned long __declspec(dllexport) __aros EmulGetHome(const char *name, char *home)
{
    HRESULT res;

    /* TODO: currently username is ignored, however we should acquire an access token for it */
    DWINAPI(printf("[EmulHandler] SHGetFolderPath()\n"));

    res = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_DEFAULT, home);
    return res ? ERROR_FILE_NOT_FOUND : 0;
}

static DWORD WINAPI EmulThread(struct AsyncReaderControl *emsg)
{
    BOOL res;

    DASYNC(printf("[EmulHandler I/O] Thread started, handle 0x%08lX, host handle 0x%08lX, host ID %lu\n", THandle, THandle->handle, THandle->id));
    for (;;)
    {
        WaitForSingleObject(emsg->CmdEvent, INFINITE);
        DASYNC(printf("[EmulHandler I/O] Got command: 0xu\n", emsg->cmd));
        switch(emsg->cmd)
        {
        case ASYNC_CMD_SHUTDOWN:
            DASYNC(printf("[EmulHandler I/O] shutting down thread\n"));
	    return 0;

        case ASYNC_CMD_READ:
	    DASYNC(printf("[EmulHandler I/O] READ %lu bytes at 0x%p, file 0x%p\n", emsg->len, emsg->addr, emsg->fh));
	    DWORD actual;
	    res = ReadFile(emsg->fh, emsg->addr, emsg->len, &actual, NULL);
	    emsg->actual = actual;
	    emsg->error = res ? 0 : GetLastError();
	    DASYNC(printf("[EmulHandler I/O] %lu bytes transferred, result %ld, error %lu\n", emsg->actual, res, emsg->error));
	    KrnCauseIRQ(emsg->IrqNum);
	}
    }
}

struct AsyncReaderControl ControlStruct;

struct AsyncReaderControl * __declspec(dllexport) __aros Emul_Init_Native(void)
{
    HANDLE thread;
    DWORD id;
    long irq;

    irq = KrnAllocIRQ();
    if (irq != -1)
    {
        ControlStruct.IrqNum = irq;
        ControlStruct.CmdEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (ControlStruct.CmdEvent)
        {
    	    thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EmulThread, &ControlStruct, 0, &id);
    	    if (thread)
            {
		CloseHandle(thread);
    	        return &ControlStruct;
	    }
    	    CloseHandle(ControlStruct.CmdEvent);
	}
    }
    return NULL;
}

