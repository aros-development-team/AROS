#ifndef WANDERER_FILESYSTEMS_UTILITIES_H
#define WANDERER_FILESYSTEMS_UTILITIES_H

#include <exec/types.h>
#include <proto/dos.h>

WORD AskChoice(CONST_STRPTR title, CONST_STRPTR strg, CONST_STRPTR gadgets, UWORD sel, BOOL centered);
STRPTR CombineString(STRPTR format, ...);
VOID DisplayIOError(CONST_STRPTR errormessage, IPTR ioError, ...);
BOOL CheckIfInfoFile(CONST_STRPTR path);
struct FileInfoBlock *GetFileInfoBlock(CONST_STRPTR path);
BPTR ReadFileInfoBlockAndLockPath(CONST_STRPTR path, struct FileInfoBlock *fib);
STRPTR GetPathPart(CONST_STRPTR path);
STRPTR CombinePath(CONST_STRPTR file, CONST_STRPTR dir);
BOOL CopyFileInfo(struct FileInfoBlock *fib, CONST_STRPTR sourcePath, CONST_STRPTR destPath);
BOOL FileExists(CONST_STRPTR path);
STRPTR ConstructInfofileName(CONST_STRPTR path);
BPTR LockDirectory(CONST_STRPTR path);
BOOL IsDirectory(CONST_STRPTR path);

#endif