#ifndef ADFLIB_H
#define ADFLIB_H 1

/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 * adflib.h
 *
 * include file
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "adf_defs.h"
#include "adf_str.h"

/* util */
PREFIX struct List* newCell(struct List* list, void* content);
PREFIX void freeList(struct List* list);

/* dir */
PREFIX RETCODE adfToRootDir(struct Volume *vol);
PREFIX RETCODE adfCreateDir(struct Volume* vol, SECTNUM parent, char* name);
PREFIX SECTNUM adfCurrentDir(struct Volume *vol);
PREFIX RETCODE adfChangeDir(struct Volume* vol, char *name);
PREFIX RETCODE adfParentDir(struct Volume* vol);
PREFIX RETCODE adfRemoveEntry(struct Volume *vol, SECTNUM pSect, char *name);
PREFIX struct List* adfGetDirEnt(struct Volume* vol, SECTNUM nSect );
PREFIX struct List* adfGetRDirEnt(struct Volume* vol, SECTNUM nSect, BOOL recurs );
PREFIX void printEntry(struct Entry* entry);
PREFIX void adfFreeDirList(struct List* list);
PREFIX void adfFreeEntry(struct Entry *);
PREFIX RETCODE adfRenameEntry(struct Volume *vol, SECTNUM, char *old,SECTNUM,char *new);
PREFIX RETCODE adfSetEntryAccess(struct Volume*, SECTNUM, char*, ULONG);
PREFIX RETCODE adfSetEntryComment(struct Volume*, SECTNUM, char*, char*);

/* file */
PREFIX ULONG adfFileRealSize(ULONG size, int blockSize, ULONG *dataN, ULONG *extN);
PREFIX struct File* adfOpenFile(struct Volume *vol, char* name, char *mode);
PREFIX void adfCloseFile(struct File *file);
PREFIX ULONG adfReadFile(struct File* file, ULONG n, unsigned char *buffer);
PREFIX BOOL adfEndOfFile(struct File* file);
PREFIX ULONG adfWriteFile(struct File *file, ULONG n, unsigned char *buffer);
PREFIX void adfFlushFile(struct File *file);

/* volume */
PREFIX RETCODE adfInstallBootBlock(struct Volume *vol,unsigned char*);
PREFIX struct Volume* adfMount( struct Device *dev, int nPart, BOOL readOnly );
PREFIX void adfUnMount(struct Volume *vol);
PREFIX void adfVolumeInfo(struct Volume *vol);

/* device */
PREFIX void adfDeviceInfo(struct Device *dev);
PREFIX struct Device* adfMountDev( char* filename,BOOL ro);
PREFIX void adfUnMountDev( struct Device* dev);
PREFIX RETCODE adfCreateHd(struct Device* dev, int n, struct Partition** partList );
PREFIX RETCODE adfCreateFlop(struct Device* dev, char* volName, int volType );
PREFIX RETCODE adfCreateHdFile(struct Device* dev, char* volName, int volType);

/* dump device */
PREFIX struct Device* adfCreateDumpDevice(char* filename, ULONG cyl, ULONG heads, ULONG sec);

/* env */
PREFIX void adfEnvInitDefault();
PREFIX void adfEnvCleanUp();
PREFIX void adfChgEnvProp(int prop, void *new);
PREFIX char* adfGetVersionNumber();
PREFIX char* adfGetVersionDate();
/* obsolete */
PREFIX void adfSetEnvFct( void(*e)(char*), void(*w)(char*), void(*v)(char*) );

/* link */
PREFIX RETCODE adfBlockPtr2EntryName(struct Volume *, SECTNUM, SECTNUM,char **, ULONG *);

/* salv */
PREFIX struct List* adfGetDelEnt(struct Volume *vol);
PREFIX RETCODE adfUndelEntry(struct Volume* vol, SECTNUM parent, SECTNUM nSect);
PREFIX void adfFreeDelList(struct List* list);
PREFIX RETCODE adfCheckEntry(struct Volume* vol, SECTNUM nSect, int level);

/* middle level API */

PREFIX BOOL isSectNumValid(struct Volume *vol, SECTNUM nSect);

/* low level API */

PREFIX RETCODE adfReadBlock(struct Volume* , ULONG nSect, unsigned char* buf);
PREFIX RETCODE adfWriteBlock(struct Volume* , ULONG nSect, unsigned char* buf);
PREFIX ULONG adfCountFreeBlocks(struct Volume* vol);


#ifdef __cplusplus
}
#endif

#endif /* ADFLIB_H */
/*##########################################################################*/
