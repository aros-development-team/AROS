#ifndef ADF_FILE_H
#define ADF_FILE_H 1

/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  adf_file.h
 *
 */

#include"prefix.h"

#include"adf_str.h"

RETCODE adfGetFileBlocks(struct Volume* vol, struct bFileHeaderBlock* entry,
    struct FileBlocks* );
RETCODE adfFreeFileBlocks(struct Volume* vol, struct bFileHeaderBlock *entry);
PREFIX long adfFileRealSize(unsigned long size, int blockSize, long *dataN, long *extN);

long adfPos2DataBlock(long pos, int blockSize, int *posInExtBlk, int *posInDataBlk, long *curDataN );

RETCODE adfWriteFileHdrBlock(struct Volume *vol, SECTNUM nSect, struct bFileHeaderBlock* fhdr);

RETCODE adfReadDataBlock(struct Volume *vol, SECTNUM nSect, void *data);
RETCODE adfWriteDataBlock(struct Volume *vol, SECTNUM nSect, void *data);
RETCODE adfReadFileExtBlock(struct Volume *vol, SECTNUM nSect, struct bFileExtBlock* fext);
RETCODE adfWriteFileExtBlock(struct Volume *vol, SECTNUM nSect, struct bFileExtBlock* fext);

PREFIX struct File* adfOpenFile(struct Volume *vol, char* name, char *mode);
PREFIX void adfCloseFile(struct File *file);
PREFIX long adfReadFile(struct File* file, long n, unsigned char *buffer);
PREFIX BOOL adfEndOfFile(struct File* file);
RETCODE adfReadNextFileBlock(struct File* file);
PREFIX long adfWriteFile(struct File *file, long n, unsigned char *buffer);
SECTNUM adfCreateNextFileBlock(struct File* file);
PREFIX void adfFlushFile(struct File *file);



#endif /* ADF_FILE_H */

