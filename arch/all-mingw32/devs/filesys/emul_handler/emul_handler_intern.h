#ifndef __EMUL_HANDLER_INTERN_H
#define __EMUL_HANDLER_INTERN_H
/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id: emul_handler_intern.h 29350 2008-08-31 18:05:43Z neil $

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
    struct Unit       		* eb_stdin;
    struct Unit       		* eb_stdout;
    struct Unit       		* eb_stderr;
    struct SignalSemaphore 	  sem;
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
    void  * DIR;      /* both of these vars will be filled in by examine *only* (at the moment) */
    char * volume;
    char * volumename;
    long   fd;
};
#define FHD_FILE      0
#define FHD_DIRECTORY 1

struct EmulInterface
{
    ULONG (*EmulOpen)(const char *path, int mode, int protect);
    ULONG (*EmulClose)(int fd);
    void *(*EmulOpenDir)(const char *path);
    ULONG (*EmulCloseDir)(void *dir);
    ULONG (*EmulStat)(const char *path, struct FileInfoBlock *FIB);
    const char *(*EmulDirName)(void *dir);
    ULONG (*EmulTellDir)(void *dir);
    void (*EmulSeekDir)(void *dir, long loc);
    void (*EmulRewindDir)(void *dir);
    void (*EmulDelete)(const char *filename);
    ULONG (*EmulRename)(const char *spath, const char *dpath);
    unsigned long (*EmulGetHome)(const char *name, char *home);
    char *(*EmulGetCWD)(char *buf, long len);
    ULONG (*EmulStatFS)(const char *path, struct InfoData *id);
    ULONG (*EmulChDir)(const char *path);
    ULONG (*EmulIsatty)(int fd);
    LONG (*EmulLSeek)(int fd, long offset, long base);
    ULONG (*EmulChmod)(const char *path, int protect);
    ULONG (*EmulMKDir)(const char *path, int protect);
    ULONG (*EmulRead)(int fd, char *buf, long len);
    ULONG (*EmulWrite)(int fd, char *buf, long len);
    ULONG (*EmulErrno)(void);
};

#define Chmod EmulIFace->EmulChmod
#define Isatty EmulIFace->EmulIsatty
#define LSeek EmulIFace->EmulLSeek
#define MKDir EmulIFace->EmulMKDir
#define DoRead EmulIFace->EmulRead
#define DoWrite EmulIFace->EmulWrite
#define Stat EmulIFace->EmulStat
#define Errno EmulIFace->EmulErrno
#define CloseDir EmulIFace->EmulCloseDir
#define DoClose EmulIFace->EmulClose
#define DoOpen EmulIFace->EmulOpen
#define OpenDir EmulIFace->EmulOpenDir
#define ChDir EmulIFace->EmulChDir
#define DirName EmulIFace->EmulDirName
#define TellDir EmulIFace->EmulTellDir
#define SeekDir EmulIFace->EmulSeekDir
#define RewindDir EmulIFace->EmulRewindDir
#define Delete EmulIFace->EmulDelete
#define DoRename EmulIFace->EmulRename
#define GetCWD EmulIFace->EmulGetCWD
#define GetHome EmulIFace->EmulGetHome
#define StatFS EmulIFace->EmulStatFS

/* The following functions are availible on not all Windows versions, so they are optional for us.
   If they are present, we will use them. If not, produce error or fail back to emulation (for example,
   softlinks can be implemented using shell shortcuts) */

struct KernelInterface
{
    ULONG (*CreateHardLink)(const char *lpFileName, const char *lpExistingFileName, void *lpSecurityAttributes);
    ULONG (*CreateSymbolicLink)(const char *lpSymlinkFileName, const char *lpTargetFileName, ULONG dwFlags);
};

#define Link KernelIFace->CreateHardLink
#define SymLink KernelIFace->CreateSymbolicLink

#endif /* __EMUL_HANDLER_INTERN_H */
