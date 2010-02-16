/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: unixio.h 30792 2009-03-07 22:40:04Z neil $

    Desc: Unix filedescriptor/socket IO include file
*/

#ifndef HIO_H
#define HIO_H

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/libraries.h>
#include <oop/oop.h>
#include <proto/exec.h>

#include "winapi.h"

/* Internal structures */
struct File_Handle
{
    APTR handle;   /* Actual file handle		       */
    OVERLAPPED io; /* Overlapped I/O control structure	       */
    UBYTE flags;
};

#define HANDLE_CLONED 0x01

struct KernelInterface
{
    __attribute__((stdcall)) void *(*CreateFile)(char *lpFileName, ULONG dwDesiredAccess, ULONG dwShareMode, void *lpSecurityAttributes,
						 ULONG dwCreationDisposition, ULONG dwFlagsAndAttributes, void *hTemplateFile);
    __attribute__((stdcall)) ULONG (*CloseHandle)(void *hObject);
    __attribute__((stdcall)) ULONG (*ReadFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToRead, ULONG *lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
    __attribute__((stdcall)) ULONG (*WriteFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToWrite, ULONG *lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
    __attribute__((stdcall)) ULONG (*DeviceIoControl)(void *hDevice, ULONG dwIoControlCode, void *lpInBuffer, ULONG nInBufferSize,
						       void *lpOutBuffer, ULONG nOutBufferSize, ULONG *lpBytesReturned, LPOVERLAPPED lpOverlapped);
    __attribute__((stdcall)) ULONG (*GetLastError)(void);
    __attribute__((stdcall)) ULONG (*GetOverlappedResult)(void *hFile, LPOVERLAPPED lpOverlapped, ULONG *lpNumberOfBytesTransferred, ULONG bWait);
};

struct AROSInterface
{
    long (*KrnAllocIRQ)(void);
    void (*KrnFreeIRQ)(unsigned char irq);
    void *(*KrnGetIRQObject)(unsigned char irq);
};

#define CreateFile  HD(cl)->KernelIFace->CreateFile
#define CloseHandle HD(cl)->KernelIFace->CloseHandle
#define ReadFile HD(cl)->KernelIFace->ReadFile
#define WriteFile HD(cl)->KernelIFace->WriteFile
#define DeviceIoControl HD(cl)->KernelIFace->DeviceIoControl
#define GetLastError HD(cl)->KernelIFace->GetLastError
#define GetOverlappedResult HD(cl)->KernelIFace->GetOverlappedResult

#define KrnAllocIRQ HD(cl)->AROSIFace->KrnAllocIRQ
#define KrnFreeIRQ HD(cl)->AROSIFace->KrnFreeIRQ
#define KrnGetIRQObject HD(cl)->AROSIFace->KrnGetIRQObject

/* instance data for the hostioclass */
struct HostIOData
{
    long irq;
    void *irqobj;
};

/* static data for the unixioclass */
struct hio_data
{
    APTR		    kernel_handle;
    APTR		    aros_handle;
    struct KernelInterface *KernelIFace;
    struct AROSInterface   *AROSIFace;
    struct Task 	   *hd_WaitForIO;
    struct MsgPort	   *hd_Port;
};

struct hostio_base
{
    struct Library	    hio_lib;
    BPTR		    hio_SegList;
    OOP_Class		   *hio_hostioclass;
    struct hio_data	    hio_csd;
};

#define HD(cl) (&((struct hostio_base *)cl->UserData)->hio_csd)

#endif /* HIO */
