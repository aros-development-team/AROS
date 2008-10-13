#ifndef __EMUL_HANDLER_INTERN_H
#define __EMUL_HANDLER_INTERN_H
/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <hidd/hidd.h>

/* POSIX includes */
//#define timeval sys_timeval
//#include <dirent.h>
//#include <sys/types.h>
//#undef timeval

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
    char    	    	    	* current_volume;
    APTR			  mempool;
    void			* EmulHandle;
    void			* KernelHandle;
};


struct filehandle
{
    char * name;     /* full name including pathname                 */
    int    type;     /* type can either be FHD_FILE or FHD_DIRECTORY */
    char * pathname; /* if type == FHD_FILE then you'll find the pathname here */
    long   dirpos;   /* and how to reach it via seekdir(.,dirpos) here. */
    void * DIR;      /* both of these vars will be filled in by examine *only* (at the moment) */
    char * volume;
    char * volumename;
    void * fd;
};
#define FHD_FILE      0
#define FHD_DIRECTORY 1

struct EmulInterface
{
    void *(*EmulOpenDir)(const char *path);
    ULONG (*EmulCloseDir)(void *dir);
    ULONG (*EmulStat)(const char *path, struct FileInfoBlock *FIB);
    const char *(*EmulDirName)(void *dir);
    ULONG (*EmulTellDir)(void *dir);
    void (*EmulSeekDir)(void *dir, long loc);
    void (*EmulRewindDir)(void *dir);
    ULONG (*EmulDelete)(const char *filename);
    unsigned long (*EmulGetHome)(const char *name, char *home);
    ULONG (*EmulStatFS)(const char *path, struct InfoData *id);
    ULONG (*EmulChmod)(const char *path, int protect);
    ULONG (*EmulMKDir)(const char *path, int protect);
    ULONG (*EmulErrno)(void);
};

#define Chmod EmulIFace->EmulChmod
#define MKDir EmulIFace->EmulMKDir
#define Stat EmulIFace->EmulStat
#define Errno EmulIFace->EmulErrno
#define CloseDir EmulIFace->EmulCloseDir
#define OpenDir EmulIFace->EmulOpenDir
#define DirName EmulIFace->EmulDirName
#define TellDir EmulIFace->EmulTellDir
#define SeekDir EmulIFace->EmulSeekDir
#define RewindDir EmulIFace->EmulRewindDir
#define Delete EmulIFace->EmulDelete
#define GetHome EmulIFace->EmulGetHome
#define StatFS EmulIFace->EmulStatFS

/* The following functions are availible on not all Windows versions, so they are optional for us.
   If they are present, we will use them. If not, produce error or fail back to emulation (for example,
   softlinks can be implemented using shell shortcuts) */

struct KernelInterface
{
    __attribute__((stdcall)) void *(*CreateFile)(const char *lpFileName, ULONG dwDesiredAccess, ULONG dwShareMode, void *lpSecurityAttributes,
						 ULONG dwCreationDisposition, ULONG dwFlagsAndAttributes, void *hTemplateFile);
    __attribute__((stdcall)) ULONG (*CloseHandle)(void *hObject);
    __attribute__((stdcall)) ULONG (*ReadFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToRead, ULONG *lpNumberOfBytesRead, void *lpOverlapped);
    __attribute__((stdcall)) ULONG (*WriteFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToWrite, ULONG *lpNumberOfBytesWritten, void *lpOverlapped);
    __attribute__((stdcall)) ULONG (*SetFilePointer)(void *hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh, ULONG dwMoveMethod);
    __attribute__((stdcall)) ULONG (*GetFileType)(void *hFile);
    __attribute__((stdcall)) void *(*GetStdHandle)(ULONG nStdHandle);
    __attribute__((stdcall)) ULONG (*MoveFile)(const char *lpExistingFileName, const char *lpNewFileName);
    __attribute__((stdcall)) ULONG (*GetCurrentDirectory)(ULONG nBufferLength, char *lpBuffer);
    __attribute__((stdcall)) ULONG (*SetCurrentDirectory)(const char *lpPathName);
    __attribute__((stdcall)) ULONG (*CreateHardLink)(const char *lpFileName, const char *lpExistingFileName, void *lpSecurityAttributes);
    __attribute__((stdcall)) ULONG (*CreateSymbolicLink)(const char *lpSymlinkFileName, const char *lpTargetFileName, ULONG dwFlags);
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
#define ChDir KernelIFace->SetCurrentDirectory
#define Link KernelIFace->CreateHardLink
#define SymLink KernelIFace->CreateSymbolicLink

#endif /* __EMUL_HANDLER_INTERN_H */
