#ifndef ADF_DIR_H
#define ADF_DIR_H 1

/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_dir.h
 *
 */

#include"adf_str.h"
#include"adf_err.h"
#include"adf_defs.h"

PREFIX RETCODE adfToRootDir(struct Volume *vol);
BOOL isDirEmpty(struct bDirBlock *dir);
PREFIX RETCODE adfRemoveEntry(struct Volume *vol, SECTNUM pSect, char *name);
PREFIX struct List* adfGetDirEnt(struct Volume* vol, SECTNUM nSect );
PREFIX struct List* adfGetRDirEnt(struct Volume* vol, SECTNUM nSect, BOOL recurs );
PREFIX void adfFreeDirList(struct List* list);

RETCODE adfEntBlock2Entry(struct bEntryBlock *entryBlk, struct Entry *entry);
PREFIX void adfFreeEntry(struct Entry *entry);
RETCODE adfCreateFile(struct Volume* vol, SECTNUM parent, char *name,
    struct bFileHeaderBlock *fhdr);
PREFIX RETCODE adfCreateDir(struct Volume* vol, SECTNUM parent, char* name);
SECTNUM adfCreateEntry(struct Volume *vol, struct bEntryBlock *dir, char *name, SECTNUM );
PREFIX RETCODE adfRenameEntry(struct Volume *vol, SECTNUM, char *old,SECTNUM,char *new);


RETCODE adfReadEntryBlock(struct Volume* vol, SECTNUM nSect, struct bEntryBlock* ent);
RETCODE adfWriteDirBlock(struct Volume* vol, SECTNUM nSect, struct bDirBlock *dir);
RETCODE adfWriteEntryBlock(struct Volume* vol, SECTNUM nSect, struct bEntryBlock *ent);

char* adfAccess2String(ULONG acc);
char adfIntlToUpper(char c);
int adfGetHashValue(char *name, BOOL intl);
void myToUpper( char *ostr, char *nstr, int,BOOL intl );
PREFIX RETCODE adfChangeDir(struct Volume* vol, char *name);
PREFIX RETCODE adfParentDir(struct Volume* vol);
PREFIX RETCODE adfSetEntryAccess(struct Volume*, SECTNUM, char*, ULONG);
PREFIX RETCODE adfSetEntryComment(struct Volume*, SECTNUM, char*, char*);
SECTNUM adfNameToEntryBlk(struct Volume *vol, ULONG ht[], char* name, 
    struct bEntryBlock *entry, SECTNUM *);

PREFIX void printEntry(struct Entry* entry);
void adfFreeDirList(struct List* list);

#endif /* ADF_DIR_H */

