#ifndef __EMUL_HANDLER_INTERN_H
#define __EMUL_HANDLER_INTERN_H
/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

struct EmulThreadMessage
{
    unsigned char op;
    void *task;
    void *fh;
    void *addr;
    unsigned long len;
    unsigned long actual;
    unsigned long error;
};

#define EMUL_CMD_READ     0
#define EMUL_CMD_WRITE    1

#ifdef __AROS__

#include <aros/hostthread.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <hidd/hidd.h>

#include "winapi.h"

struct emulbase
{
    struct Device		  device;
    				/* nlorentz: Cal it eb_std* because std* is reserved */
    struct filehandle  		* eb_stdin;
    struct filehandle 		* eb_stdout;
    struct filehandle 		* eb_stderr;
    void			* stdin_handle;
    void			* stdout_handle;
    void			* stderr_handle;
    struct SignalSemaphore	  sem;
    struct SignalSemaphore	  memsem;
    APTR			  mempool;
    void			* EmulHandle;
    void			* KernelHandle;
    struct ThreadHandle		* HostThread;
    struct EmulThreadMessage	  EmulMsg;
    struct Interrupt		  EmulInt;
};


struct filehandle
{
    char * hostname; /* full host's pathname (includes volume root prefix 		       */
    char * name;     /* full name including pathname					       */
    int    type;     /* type can either be FHD_FILE or FHD_DIRECTORY			       */
    char * pathname; /* if type == FHD_FILE then you'll find the pathname here		       */
    char * volumename;
    void * fd;
    ULONG  dirpos;
    struct DosList *dl;
};
#define FHD_FILE      0
#define FHD_DIRECTORY 1

struct EmulInterface
{
    ULONG (*EmulThread)(struct ThreadHandle *myhandle);
    ULONG (*EmulStat)(char *path, struct FileInfoBlock *FIB);
    ULONG (*EmulDelete)(char *filename);
    unsigned long (*EmulGetHome)(char *name, char *home);
    ULONG (*EmulStatFS)(char *path, struct InfoData *id);
    ULONG (*EmulChmod)(char *path, int protect);
    ULONG (*EmulMKDir)(char *path, int protect);
    ULONG (*EmulErrno)(void);
};

#define Chmod EmulIFace->EmulChmod
#define MKDir EmulIFace->EmulMKDir
#define Stat EmulIFace->EmulStat
#define Errno EmulIFace->EmulErrno
#define Delete EmulIFace->EmulDelete
#define GetHome EmulIFace->EmulGetHome
#define StatFS EmulIFace->EmulStatFS

/* The following functions are availible on not all Windows versions, so they are optional for us.
   If they are present, we will use them. If not, produce error or fail back to emulation (for example,
   softlinks can be implemented using shell shortcuts) */

struct KernelInterface
{
    __attribute__((stdcall)) void *(*CreateFile)(char *lpFileName, ULONG dwDesiredAccess, ULONG dwShareMode, void *lpSecurityAttributes,
						 ULONG dwCreationDisposition, ULONG dwFlagsAndAttributes, void *hTemplateFile);
    __attribute__((stdcall)) ULONG (*CloseHandle)(void *hObject);
    __attribute__((stdcall)) ULONG (*ReadFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToRead, ULONG *lpNumberOfBytesRead, void *lpOverlapped);
    __attribute__((stdcall)) ULONG (*WriteFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToWrite, ULONG *lpNumberOfBytesWritten, void *lpOverlapped);
    __attribute__((stdcall)) ULONG (*SetFilePointer)(void *hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh, ULONG dwMoveMethod);
    __attribute__((stdcall)) ULONG (*GetFileType)(void *hFile);
    __attribute__((stdcall)) void *(*GetStdHandle)(ULONG nStdHandle);
    __attribute__((stdcall)) ULONG (*MoveFile)(char *lpExistingFileName, char *lpNewFileName);
    __attribute__((stdcall)) ULONG (*GetCurrentDirectory)(ULONG nBufferLength, char *lpBuffer);
    __attribute__((stdcall)) void *(*FindFirstFile)(char *lpFileName, LPWIN32_FIND_DATA lpFindFileData);
    __attribute__((stdcall)) ULONG (*FindNextFile)(void *hFindFile, LPWIN32_FIND_DATA lpFindFileData);
    __attribute__((stdcall)) ULONG (*FindClose)(void *hFindFile);
    __attribute__((stdcall)) ULONG (*CreateHardLink)(char *lpFileName, char *lpExistingFileName, void *lpSecurityAttributes);
    __attribute__((stdcall)) ULONG (*CreateSymbolicLink)(char *lpSymlinkFileName, char *lpTargetFileName, ULONG dwFlags);
};

#define OpenFile KernelIFace->CreateFile
#define DoClose KernelIFace->CloseHandle
#define DoRead KernelIFace->ReadFile
#define DoWrite KernelIFace->WriteFile
#define LSeek KernelIFace->SetFilePointer
#define GetFileType KernelIFace->GetFileType
#define GetStdFile KernelIFace->GetStdHandle
#define DoRename KernelIFace->MoveFile
#define GetCWD KernelIFace->GetCurrentDirectory
#define FindFirst KernelIFace->FindFirstFile
#define FindNext KernelIFace->FindNextFile
#define FindEnd KernelIFace->FindClose
#define Link KernelIFace->CreateHardLink
#define SymLink KernelIFace->CreateSymbolicLink

#endif

#endif /* __EMUL_HANDLER_INTERN_H */
