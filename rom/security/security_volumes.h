#ifndef _SECURITY_VOLUMES_H
#define _SECURITY_VOLUMES_H

/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Configuration															*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/


#include <libraries/security.h>

/*
 *		MultiUserFileSystem Volumes
 */

struct secVolume {
    struct secVolume                    *Next;
    struct DosList                      *DosList;			/* DosList for this muFS Volume */
    struct MsgPort                      *Process;			/* Process for this muFS Volume */
    
    /* Extensions for MUFS2 */
    
    LONG	                        FS_Flags;						/* Allow set{g,u}id, read-only etc. */
    
    /* If FS_Flags == 0, the rest of this structure is ignored; this indicates
     * that the volume is a true MUFS volume */
    
    LONG	                        RootProtection;				/* Permissions for the root dir */
    ULONG	                        RootOwner;						/* UID:GID of owner of root dir */

    STRPTR                              FS_Name;						/* So we dont re-run on the same volume */
    struct MsgPort*	                OrigProc;		/* The real FS */
    struct MsgPort*	                RepPort;			/* For talking with the real FS */	
    struct MinList	                FHCache[TASKHASHVALUE];	/* HashList of cached FileHandles */
    struct FileInfoBlock                *fib;
    LONG	                        PassKey;							/* If non-zero, contains the 32 bit passkey
                                                                                             * needed to write enable the filesystem 
                                                                                             * with ACTION_WRITE_PROTECT */
    /* NEW proxy enforcer */
    struct MinList                      ProxyLocks;	/* List of locks */
    struct MinList                      ProxyHandles[TASKHASHVALUE];	/* HashList of proxy filehandles */
    struct DeviceNode                   *ProxyDosList;	/* Dos entry created by the proxy filesystem */
    struct DeviceList                   *ProxyDosListVolume;	/* Dos entry created by the proxy filesystem */
    ULONG                               LockCount;			/* Number of proxy locks in existence */
};


/*
 *		Function Prototypes
 */

extern BOOL InitVolumes(struct Library *secBase);
extern void FreeVolumes(struct Library *secBase);
extern BOOL CheckmuFSVolume(struct MsgPort *port);

#endif /* _SECURITY_VOLUMES_H */
