#ifndef __EMUL_HANDLER_INTERN_H
#define __EMUL_HANDLER_INTERN_H
/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
    APTR			  mempool;
    void			* EmulHandle;
    void			* KernelHandle;
    void			* ConsoleInt;
    struct AsyncReaderControl	* ConsoleReader;
};

struct filehandle
{
    char * hostname;	/* full host's pathname (includes volume root prefix 	  */
    char * name;	/* full name including pathname				  */
    int    type;	/* type flag, see below			       		  */
    char * pathname;	/* if type == FHD_FILE then you'll find the pathname here */
    char * volumename;	/* volume name						  */
    void * fd;
    ULONG  dirpos;
    struct DosList *dl;
};

/* type flags */
#define FHD_FILE      0x01
#define FHD_DIRECTORY 0x02
#define FHD_STDIO     0x80

struct EmulInterface
{
    struct AsyncReaderControl *(*EmulInitNative)(void);
    LONG (*EmulStat)(char *path, WIN32_FILE_ATTRIBUTE_DATA *FIB, ULONG *attrmask);
    ULONG (*EmulDelete)(char *filename);
    unsigned long (*EmulGetHome)(char *name, char *home);
    ULONG (*EmulStatFS)(char *path, struct InfoData *id);
};

#define InitNative EmulIFace->EmulInitNative
#define Stat EmulIFace->EmulStat
#define Delete EmulIFace->EmulDelete
#define GetHome EmulIFace->EmulGetHome
#define StatFS EmulIFace->EmulStatFS

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
};

#define OpenFile KernelIFace->CreateFile
#define DoClose KernelIFace->CloseHandle
#define DoRead KernelIFace->ReadFile
#define DoWrite KernelIFace->WriteFile
#define LSeek KernelIFace->SetFilePointer
#define SetEOF KernelIFace->SetEndOfFile
#define GetFileType KernelIFace->GetFileType
#define GetStdFile KernelIFace->GetStdHandle
#define DoRename KernelIFace->MoveFile
#define GetCWD KernelIFace->GetCurrentDirectory
#define FindFirst KernelIFace->FindFirstFile
#define FindNext KernelIFace->FindNextFile
#define FindEnd KernelIFace->FindClose
#define MKDir KernelIFace->CreateDirectory
#define Chmod KernelIFace->SetFileAttributes
#define GetLastError KernelIFace->GetLastError
#define Link KernelIFace->CreateHardLink
#define SymLink KernelIFace->CreateSymbolicLink
#define RaiseEvent KernelIFace->SetEvent
#define SetFileTime KernelIFace->SetFileTime

#endif

#endif /* __EMUL_HANDLER_INTERN_H */
