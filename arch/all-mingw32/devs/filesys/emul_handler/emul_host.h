#ifndef __EMUL_HOST_H
#define __EMUL_HOST_H
/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

/* This structure describes our virtual hardware registers */
struct AsyncReaderControl
{
    unsigned char cmd;	  /* Command				        */
    void *fh;		  /* File handle to operate on			*/
    void *addr;		  /* Buffer address				*/
    unsigned long len;	  /* Requested data length			*/
    unsigned long actual; /* Actual data length				*/
    unsigned long error;  /* Error code					*/
    void *CmdEvent;	  /* Event to trigger in order to tell us to go */
    unsigned char IrqNum; /* IRQ number on AROS side			*/
    unsigned long sig;	  /* AROS signal to use, used by IRQ handler	*/
    void *task;		  /* AROS task to signal, used by IRQ handler	*/
};

#define ASYNC_CMD_SHUTDOWN 0
#define ASYNC_CMD_READ     1

#ifdef __AROS__

#include "winapi.h"

struct EmulInterface
{
    struct AsyncReaderControl *(*EmulInitNative)(void);
    unsigned long (*EmulGetHome)(char *name, char *home);
};

/* The following functions are availible on not all Windows versions, so they are optional for us.
   If they are present, we will use them. If not, produce error or fail back to emulation (for example,
   softlinks can be implemented using shell shortcuts) */

struct KernelInterface
{
    void * __stdcall (*CreateFile)(char *lpFileName, ULONG dwDesiredAccess, ULONG dwShareMode, void *lpSecurityAttributes,
						 ULONG dwCreationDisposition, ULONG dwFlagsAndAttributes, void *hTemplateFile);
    ULONG  __stdcall (*CloseHandle)(void *hObject);
    ULONG  __stdcall (*ReadFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToRead, ULONG *lpNumberOfBytesRead, void *lpOverlapped);
    ULONG  __stdcall (*WriteFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToWrite, ULONG *lpNumberOfBytesWritten, void *lpOverlapped);
    ULONG  __stdcall (*SetFilePointer)(void *hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh, ULONG dwMoveMethod);
    ULONG  __stdcall (*SetEndOfFile)(void *hFile);
    ULONG  __stdcall (*GetFileType)(void *hFile);
    void * __stdcall (*GetStdHandle)(ULONG nStdHandle);
    ULONG  __stdcall (*MoveFile)(char *lpExistingFileName, char *lpNewFileName);
    ULONG  __stdcall (*GetCurrentDirectory)(ULONG nBufferLength, char *lpBuffer);
    void * __stdcall (*FindFirstFile)(char *lpFileName, LPWIN32_FIND_DATA lpFindFileData);
    ULONG  __stdcall (*FindNextFile)(void *hFindFile, LPWIN32_FIND_DATA lpFindFileData);
    ULONG  __stdcall (*FindClose)(void *hFindFile);
    ULONG  __stdcall (*CreateDirectory)(char *lpPathName, void *lpSecurityAttributes);
    ULONG  __stdcall (*SetFileAttributes)(char *lpFileName, ULONG dwFileAttributes);
    ULONG  __stdcall (*GetLastError)(void);
    ULONG  __stdcall (*CreateHardLink)(char *lpFileName, char *lpExistingFileName, void *lpSecurityAttributes);
    ULONG  __stdcall (*CreateSymbolicLink)(char *lpSymlinkFileName, char *lpTargetFileName, ULONG dwFlags);
    ULONG  __stdcall (*SetEvent)(void *hEvent);
    ULONG  __stdcall (*SetFileTime)(void *hFile, UQUAD *lpCreationTime, UQUAD *lpLastAccessTime, UQUAD *lpLastWriteTime);
    ULONG  __stdcall (*GetFileAttributes)(char *lpFileName);
    ULONG  __stdcall (*GetFileAttributesEx)(char *lpFileName, ULONG fInfoLevelId, void *lpFileInformation);
    ULONG  __stdcall (*_DeleteFile)(char *lpFileName);
    ULONG  __stdcall (*RemoveDirectory)(char *lpPathName);
    ULONG  __stdcall (*GetDiskFreeSpace)(char *lpRootPathName, ULONG *lpSectorsPerCluster, ULONG *lpBytesPerSector, ULONG *lpNumberOfFreeClusters, ULONG *lpTotalNumberOfClusters);
};

struct PlatformHandle
{
    char  *pathname;	/* Pathname with pattern for directory searching */
    ULONG  dirpos;	/* Current directory search position		 */
};

struct Emul_PlatformData
{
    void			*EmulHandle;
    void			*KernelHandle;
    void			*ConsoleInt;
    struct AsyncReaderControl	*ConsoleReader;
    struct EmulInterface	*EmulIFace;
    struct KernelInterface	*KernelIFace;
};

#endif
#endif
