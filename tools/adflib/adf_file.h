#ifndef ADF_FILE_H
#define ADF_FILE_H 1

/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  adf_file.h
 *
 */

#include"adf_str.h"

RETCODE adfGetFileBlocks(struct Volume* vol, struct bFileHeaderBlock* entry,
    struct FileBlocks* );
RETCODE adfFreeFileBlocks(struct Volume* vol, struct bFileHeaderBlock *entry);
PREFIX ULONG adfFileRealSize(ULONG size, int blockSize, ULONG *dataN, ULONG *extN);

ULONG adfPos2DataBlock(ULONG pos, int blockSize, int *posInExtBlk, int *posInDataBlk, ULONG *curDataN );

RETCODE adfWriteFileHdrBlock(struct Volume *vol, SECTNUM nSect, struct bFileHeaderBlock* fhdr);

RETCODE adfReadDataBlock(struct Volume *vol, SECTNUM nSect, void *data);
RETCODE adfWriteDataBlock(struct Volume *vol, SECTNUM nSect, void *data);
RETCODE adfReadFileExtBlock(struct Volume *vol, SECTNUM nSect, struct bFileExtBlock* fext);
RETCODE adfWriteFileExtBlock(struct Volume *vol, SECTNUM nSect, struct bFileExtBlock* fext);

PREFIX struct File* adfOpenFile(struct Volume *vol, char* name, char *mode);
PREFIX void adfCloseFile(struct File *file);
PREFIX ULONG adfReadFile(struct File* file, ULONG n, unsigned char *buffer);
PREFIX BOOL adfEndOfFile(struct File* file);
RETCODE adfReadNextFileBlock(struct File* file);
PREFIX ULONG adfWriteFile(struct File *file, ULONG n, unsigned char *buffer);
SECTNUM adfCreateNextFileBlock(struct File* file);
PREFIX void adfFlushFile(struct File *file);



#endif /* ADF_FILE_H */

