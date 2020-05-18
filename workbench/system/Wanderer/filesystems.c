/*
    Copyright 2007-2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "filesystems.h"
#include "filesystems_utilities.h"
#include "locale.h"

/**
 * Displays a requset window asking the user if he is absolutely sure to overwrite and unprotect a file. The answeres are 
 * recorded in the OpModes struct and the request window is opened by calling the askHook.
 * 
 * Params: destPath -> Target path
 *         opModes  -> OpModes struct where the answers are stored
 *         askHook  -> Hook responsible for opening a request window
 *         askData  -> FileCopyData struct containg the data displayed by the askHook
 *         fib      -> FileInfoBlock struct for the file in question
 * 
 * Result: TRUE if user accept to overwrite and unprotect (if necessary), FALSE if not
 */
static BOOL askToOverwriteAndUnprotect(CONST_STRPTR destPath, struct OpModes *opModes, struct Hook *askHook, struct FileCopyData *askData, struct FileInfoBlock *fib)
{
    BOOL retvalue = FALSE;

    STRPTR directory = GetPathPart(destPath);
    askData->spath = directory;

    if (opModes->overwritemode != OPMODE_NONE)
    {
        if (opModes->overwritemode != OPMODE_ALL)
        {
            // If user has not already accepted all files, ask
            askData->file = FilePart(destPath);
            askData->type = 2;

            opModes->overwritemode = CallHook(askHook, (Object *)askData, NULL);
        }

        // If user accepts to overwrite
        if ((opModes->overwritemode == OPMODE_ALL) || (opModes->overwritemode == OPMODE_YES))
        {
            // If protection bit is set, ask to unprotect
            if (((fib->fib_Protection & (FILEINFO_PROTECTED | FILEINFO_WRITE)) != 0) && (opModes->protectmode != OPMODE_NONE))
            {
                if (opModes->protectmode != OPMODE_ALL)
                {
                    // If user has not already accepted all files, ask
                    askData->file = FilePart(destPath);
                    askData->type = 1;

                    opModes->protectmode = CallHook(askHook, (Object *)askData, NULL);
                }

                if ((opModes->protectmode == OPMODE_ALL) || (opModes->protectmode == OPMODE_YES))
                {
                    // User accepted to unprotect file
                    SetProtection(destPath, 0);
                    retvalue = TRUE;
                }
            }
            else
            {
                // User accepted overwrite and protection bit not set
                retvalue = TRUE;
            }
        }
    }
    
    if (directory != NULL)
    {
        FreeVec(directory);
    }

    return retvalue;
}

/**
 * Displays a requset window asking the user if he is absolutely sure to delete and unprotect a file. The answeres are 
 * recorded in the OpModes struct and the request window is opened by calling the askHook.
 * 
 * Params: destPath -> Target path
 *         opModes  -> OpModes struct where the answers are stored
 *         askHook  -> Hook responsible for opening a request window
 *         askData  -> FileCopyData struct containg the data displayed by the askHook
 *         fib      -> Sourde file's FileInfoBlock struct
 * 
 * Result: TRUE if user accepted to delete and unprotect (if necessary), FALSE if not
 */
static BOOL askToDeleteAndUnprotect(CONST_STRPTR path, struct OpModes *opModes, struct Hook *askHook, CONST_STRPTR infoFilePath, struct FileInfoBlock *fib)
{
    BOOL retvalue = FALSE;
    STRPTR directory = GetPathPart(path);
    struct FileCopyData askData;

    // If deleting ask for confirmation, ask to unprotect
    if (opModes && (opModes->deletemode != OPMODE_NONE))
    {
        askData.file = FilePart(path);
        askData.spath = directory;
        askData.type = 0;

        if (opModes->deletemode != OPMODE_ALL)
        {
            opModes->deletemode = CallHook(askHook, (Object *)&askData, NULL);
        }
        if ((opModes->deletemode == OPMODE_ALL) || (opModes->deletemode == OPMODE_YES))
        {
            LONG protection = fib->fib_Protection;
            if ((protection & FIBB_DELETE) != 0 && (protection & FIBB_WRITE) != 0)
            {
                askData.type = 1;

                opModes->protectmode = CallHook(askHook, (Object *)&askData, NULL);
                if ((opModes->protectmode == OPMODE_ALL) || (opModes->protectmode == OPMODE_YES))
                {
                    SetProtection(path, 0);
                    if (infoFilePath != NULL)
                    {
                        SetProtection(infoFilePath, 0);
                    }
                    retvalue = TRUE;
                }
            }
            else
            {
                retvalue = TRUE;
            }
        }
    }

    return retvalue;
}

/**
 * Function responsible for doing the actual file copy by reading and writing a set of bytes from the source file and writing them
 * to the target file. Is also responsible for updating the progress window displayed on the desktop.
 * 
 * Params: sourcePath  -> Source file path
 *         destPath    -> Target file path
 *         fib         -> Source file's FileInfoBlock struct
 *         displayHook -> Hook showing a progress window
 *         userdata    -> Structure containing pointers to various UI elements used by the displayHook
 * 
 * Result: FALSE if everything went ok, TRUE if user aborted the operation or anything failed
 */
static BOOL performDataCopy(CONST_STRPTR sourcePath, CONST_STRPTR destPath, struct FileInfoBlock *fib, struct Hook *displayHook, APTR userdata)
{
    struct FileCopyData hookData;

    STRPTR directory;
    LONG bufferlen = COPYLEN;
    LONG filelen = 0;
    BOOL quit = TRUE;
    BOOL stop = FALSE;
    BPTR in, out;
    BYTE *buffer;

    hookData.userdata = userdata;

    if (fib->fib_Size <= COPYLEN)
    {
        bufferlen = fib->fib_Size;
    }
    if (bufferlen < 8192)
    {
        bufferlen = 8192;
    }

    if (displayHook != NULL)
    {
        directory = GetPathPart(sourcePath);
        hookData.totallen = 0;
        hookData.actlen = 0;
        hookData.spath = directory;
        hookData.dpath = (STRPTR)destPath;
        hookData.filelen = fib->fib_Size;
        hookData.file = fib->fib_FileName;
        hookData.flags = ACTION_COPY;

        stop = CallHook(displayHook, (Object *)&hookData, NULL);
    }

    if (stop)
    {
        FreeVec(directory);
        return stop;
    }

    buffer = AllocVec(bufferlen, MEMF_CLEAR);
    if (buffer)
    {
        in = Open(sourcePath, MODE_OLDFILE);
        if (in > 0)
        {
            out = Open(destPath, MODE_NEWFILE);
            if (out > 0)
            {
                LONG clen, wlen;
                UWORD difftime = clock();

                do
                {
                    clen = Read(in, buffer, bufferlen);
                    if (clen > 0)
                    {
                        wlen = Write(out, buffer, clen);
                        hookData.difftime = clock() - difftime;
                        if (hookData.difftime < 1)
                        {
                            hookData.difftime = 1;
                        }
                        hookData.actlen = clen;
                        hookData.totallen += clen;

                        if (displayHook)
                        {
                            hookData.flags |= ACTION_UPDATE;
                            stop = CallHook(displayHook, (Object *)&hookData, NULL);
                        }

                        if (clen != wlen)
                        {
                            // Did not manage to write whole buffer, abort.
                            DisplayIOError(_(MSG_FAILED_TO_WRITE_TO_FILE), IoErr(), (IPTR)destPath);
                            clen = 0;
                        }
                    }
                } while (clen > 0 || stop);

                quit = stop;

                Close(out);
                CopyFileInfo(fib, sourcePath, destPath);
            }
            else
            {
                DisplayIOError(_(MSG_FAILED_TO_WRITE_TO_FILE), IoErr(), (IPTR)destPath);
            }

            Close(in);
        }
        else
        {
            DisplayIOError(_(MSG_FAILED_TO_ALLOCATE_MEMORY), -1, (IPTR)sourcePath);
        }

        FreeVec(buffer);
    }

    if (displayHook)
    {
        FreeVec(directory);
    }

    return quit;
}

/**
 * Deletes a file from the filesystem and updates a progress window accordingly.
 * 
 * Params: path        -> File to be deleted 
 *         displayHook -> Hook responsible for displaying a progress window
 *         userdata    -> Structure containing pointers to various UI elements used by the displayHook
 * 
 * Result: TRUE if operation was aborted or anything failed, FALSE if not
 */
static BOOL deleteSingleFile(CONST_STRPTR path, struct Hook *displayHook, APTR userdata)
{
    struct FileCopyData hookData;
    hookData.userdata = userdata;
    BOOL stop = FALSE;
    STRPTR directory;

    if (displayHook != NULL)
    {
        directory = GetPathPart(path);
        hookData.spath = directory;
        hookData.file = FilePart(path);
        hookData.flags = ACTION_DELETE;

        stop = CallHook(displayHook, (Object *)&hookData, NULL);
    }

    if (!stop)
    {
        BOOL success = DeleteFile(path);

        if (!success)
        {
            DisplayIOError(_(MSG_FAILED_TO_DELETE_FILE), IoErr(), (IPTR)path);
            stop = TRUE;
        }
    }

    if (displayHook)
    {
        FreeVec(directory);
    }

    return stop;
}

/**
 * Moves a file from one location to another. 
 * 
 * Params: sourcePath  -> Source file path 
 *         dstDir      -> Target file path
 *         newDir      -> If set to true, file already exists check is skipped. 
 * 
 * Result: TRUE if operation was aborted or anything failed, FALSE if not
 */
static BOOL moveFile(CONST_STRPTR sourcePath, CONST_STRPTR destDir, BOOL newDir)
{
    STRPTR to;
    BOOL stop = FALSE;

    to = CombinePath(FilePart(sourcePath), destDir);

    if (!newDir)
    {
        if (FileExists(to))
        {
            stop = TRUE;

            DisplayIOError(_(MSG_FAILED_TO_MOVE_FILE), IoErr(), (IPTR)sourcePath);
        }
    }

    if (to && !stop)
    {
        if (Rename(sourcePath, to) == DOSFALSE)
        {
            stop = TRUE;
            DisplayIOError(_(MSG_FAILED_TO_MOVE_FILE), IoErr(), (IPTR)sourcePath);
        }
        FreeVec(to);
    }

    return stop;
}

/**
 * Helper function called by DeleteContent which calls the DeleteContent function for all files in a directory
 */
static BOOL deleteDirectoryContents(CONST_STRPTR path, struct OpModes *opModes, struct Hook *askHook, struct FileInfoBlock *fib, struct Hook *displayHook, APTR userdata)
{
    BOOL stop = FALSE;
    BPTR sourceDirectoryLock = ReadFileInfoBlockAndLockPath(path, fib);
    if (sourceDirectoryLock == 0)
    {
        return TRUE;
    }
    BOOL success = FALSE;

    success = ExNext(sourceDirectoryLock, fib);

    while (success && !stop)
    {
        STRPTR nextPath = CombinePath(fib->fib_FileName, path);
        if (nextPath != NULL)
        {
            stop = DeleteContent(nextPath, opModes, askHook, displayHook, userdata);
            FreeVec(nextPath);
        }
        success = ExNext(sourceDirectoryLock, fib);
    }

    if (!stop)
    {
        LONG error = IoErr();
        if (error != ERROR_NO_MORE_ENTRIES)
        {
            DisplayIOError(_(MSG_FAILED_TO_READ_DIRECTORY_CONTENT), IoErr(), (IPTR)path);
            stop = TRUE;
        }
    }

    UnLock(sourceDirectoryLock);

    return stop;
}

/**
 * Helper function that checks if target file/directory already exists 
 */
BOOL checkIfAlreadyExists(CONST_STRPTR targetPath, CONST_STRPTR sourcePath, struct Hook *askHook, struct OpModes *opModes)
{
    BOOL stop = FALSE;

    if (FileExists(targetPath))
    {
        struct FileInfoBlock *destFib = GetFileInfoBlock(targetPath);

        // Ask to overwrite and then ask to unprotect
        if (destFib != NULL && opModes && (opModes->overwritemode != OPMODE_NONE))
        {
            struct FileCopyData *askData = AllocVec(sizeof(struct FileCopyData), MEMF_CLEAR);

            stop = !askToOverwriteAndUnprotect(targetPath, opModes, askHook, askData, destFib);

            FreeVec(askData);
        }

        FreeDosObject(DOS_FIB, (APTR)destFib);
    }
    return stop;
}

/**
 * Copies a singe file from one path to another. 
 * 
 * Params: sourcePath         -> Source file path
 *         sourceInfoFilePath -> Source .info file path
 *         targetDir          -> Target file path 
 *         hasInfoFile        -> If true the file being copied has an .info file
 *         askHook            -> Hook responsible for displaying a query window
 *         opModes            -> Structure responsible for holding answers to the queries 
 *         userdata           -> Structure containing pointers to various UI elements used by the displayHook
 *         displayHook        -> Hook responsible for showing a progress window
 *         fib                -> Source file's FileInfoBlock struct
 *         inDir              -> If true the file being copied is part of a directory copy
 * 
 * Result: TRUE if the operation was aborted or anything failed, FALSE if not
 *
 */
static BOOL copySingleFile(CONST_STRPTR sourcePath, CONST_STRPTR sourceInfoFilePath, CONST_STRPTR targetDir, BOOL hasInfoFile, struct Hook *askHook, struct OpModes *opModes, APTR userdata, struct Hook *displayHook, struct FileInfoBlock *fib, BOOL inDir)
{
    BOOL doCopy = TRUE;
    BOOL stop = FALSE;
    struct FileInfoBlock *infoFib;
    struct FileCopyData *askData;
    STRPTR targetInfoFilePath;
    CONST_STRPTR fileName = FilePart(sourcePath);
    STRPTR targetPath = CombinePath(fileName, targetDir);

    if (hasInfoFile)
    {
        CONST_STRPTR infoFileName = FilePart(sourceInfoFilePath);

        targetInfoFilePath = CombinePath(infoFileName, targetDir);
        if (targetInfoFilePath == NULL)
        {
            stop = TRUE;
        }

        infoFib = GetFileInfoBlock(sourceInfoFilePath);
        if (infoFib == NULL)
        {
            stop = TRUE;
        }
    }

    // If we are copying a directory, we only need to check if the directory exists and not every single file
    if (!inDir)
    {
        stop = checkIfAlreadyExists(targetPath, sourcePath, askHook, opModes);
    }

    if (doCopy && !stop)
    {
        stop = performDataCopy(sourcePath, targetPath, fib, displayHook, userdata);
        if (hasInfoFile && !stop)
        {
            stop = performDataCopy(sourceInfoFilePath, targetInfoFilePath, infoFib, displayHook, userdata);
        }
    }

    FreeVec(targetPath);

    if (hasInfoFile)
    {
        FreeVec(targetInfoFilePath);
        FreeDosObject(DOS_FIB, (APTR)infoFib);
    }

    return stop;
}
/**
 * Helper function called by CopyContent function which calls the CopyContent function for all files in a directory
 */
static BOOL copyDirectory(CONST_STRPTR sourcePath, CONST_STRPTR targetDir, struct Hook *askHook, struct OpModes *opModes, APTR userdata, struct Hook *displayHook, struct FileInfoBlock *sourceFib, BPTR sourceDirectoryLock)
{
    BOOL success = FALSE;
    BOOL stop = FALSE;

    success = ExNext(sourceDirectoryLock, sourceFib);

    // Iterates the directory and calls CopyContent for every file found
    while (success && !stop)
    {
        CONST_STRPTR nextSourcePath = CombinePath(sourceFib->fib_FileName, sourcePath);

        stop = CopyContent(nextSourcePath, targetDir, displayHook, askHook, opModes, userdata, FALSE);

        FreeVec((APTR)nextSourcePath);

        success = ExNext(sourceDirectoryLock, sourceFib);
    }

    // If the error is not ERROR_NO_MORE_ENTRIES, we display an error message
    LONG error = IoErr();
    if (error != ERROR_NO_MORE_ENTRIES)
    {
        DisplayIOError(_(MSG_FAILED_TO_READ_DIRECTORY_CONTENT), error, (IPTR)sourcePath);
    }

    return stop;
}

/**
 * Helper function called by DeleteContent, CopyContent or MoveContent for determining if the source path points
 * to a file or .info file. It also checks if any of them exist.
 */
BOOL infoFileSetup(CONST_STRPTR sourcePath, STRPTR *sourceInfoFilePath, STRPTR *localSourcePath, BOOL *hasInfoFile)
{
    // Wanderer handles a file and its companion .info file together. The input file path will only contain a pointer to the actual file and
    // not the .info file. This is true even though for situations where the only file being copied is the .info file, and the actual file does
    // not exist. Therefore we need to determine if:
    // 1. The file begin copied is an .info file
    // 2. The file being copied has an .info file
    //
    // If 1, the sourcePath is set to the .info file path and hasInfoFile is set to false (as this would then mean .info.info ...).
    // If 2, we need to handle the .info file as well as the actual file

    BOOL doFileExists = FileExists(sourcePath);

    *sourceInfoFilePath = ConstructInfofileName(sourcePath);
    if (sourceInfoFilePath == NULL)
    {
        return TRUE;
    }
    *hasInfoFile = FileExists(*sourceInfoFilePath);
    if (!doFileExists && *hasInfoFile)
    {
        *localSourcePath = *sourceInfoFilePath;
        *hasInfoFile = FALSE;
    }

    return FALSE;
}



/* API functions */

/**
 * Creates a new directory for the give path and returns a write lock for the newly created directory.
 * If the directory already exists, a write lock for the existing directory is returned.
 * 
 * Params: path -> Full path to the new directory
 * 
 * Result: write lock for directory 
 */
BPTR CreateDirectory(CONST_STRPTR path)
{
    struct FileInfoBlock *fib;
    BPTR lock = 0;

    if (FileExists(path))
    {
        fib = GetFileInfoBlock(path);

        if (fib)
        {
            if (fib->fib_DirEntryType > 0)
            {
                lock = Lock(path, ACCESS_WRITE);
                if (!lock)
                {
                    DisplayIOError(_(MSG_FAILED_TO_LOCK_PATH), IoErr(), (IPTR)path);
                }
            }

            FreeDosObject(DOS_FIB, (APTR)fib);
        }
    }
    else
    {
        lock = CreateDir(path);
        if (!lock)
        {
            DisplayIOError(_(MSG_FAILED_TO_CREATE_DIRECTORY), IoErr(), (IPTR)path);
        }
    }

    return lock;
}

/**
 * Checks if two files or directories resides on the same device
 *
 * Params: path1 -> First path to compare
 *         path2 -> Second path to compare
 *
 * Result: TRUE if on same device, FALSE if not
 */
BOOL IsOnSameDevice(CONST_STRPTR path1, CONST_STRPTR path2)
{
    BOOL returnValue = FALSE;

    BPTR slock = Lock(path1, SHARED_LOCK);
    if (slock)
    {
        BPTR tlock = Lock(path2, SHARED_LOCK);
        if (tlock)
        {
            if (SameDevice(slock, tlock) != DOSFALSE)
            {
                returnValue = TRUE;
            }
        }
        UnLock(tlock);
    }
    UnLock(slock);

    return returnValue;
}

/**
 * Moves all content from one path to another. The path can point to a directory, regular file or info file. 
 * 
 * Params: sourcePath -> Source file path
 *         targetDir  -> Target directory
 * 
 * Result: FALSE if operation ended successfully, TRUE if it was stopped prematurely
 */
BOOL MoveContent(CONST_STRPTR sourcePath, CONST_STRPTR targetDir)
{
    BOOL stop = FALSE;
    BOOL hasInfoFile = FALSE;
    STRPTR sourceInfoFilePath;
    STRPTR localSourcePath = (STRPTR)sourcePath;

    BOOL quit = infoFileSetup(sourcePath, &sourceInfoFilePath, &localSourcePath, &hasInfoFile);
    if (quit)
    {
        return TRUE;
    }

    if (!stop)
    {
        STRPTR nextTargetFile = CombinePath(FilePart(sourcePath), targetDir);

        if (FileExists(nextTargetFile))
        {
            stop = TRUE;
            DisplayIOError(_(MSG_FAILED_TO_MOVE_FILE), IoErr(), (IPTR)sourcePath);
        }

        if (!stop)
        {
            BPTR sourceDirectoryLock = LockDirectory(localSourcePath);
            BPTR targetDirectoryLock = Lock(targetDir, ACCESS_WRITE);

            stop = moveFile(localSourcePath, targetDir, FALSE);

            if (hasInfoFile && !stop)
            {
                stop = moveFile(sourceInfoFilePath, targetDir, FALSE);
            }

            UnLock(targetDirectoryLock);
            UnLock(sourceDirectoryLock);
        }
    }

    FreeVec(sourceInfoFilePath);

    return stop;
}

/**
 * Copies all content from one path to another. The source path can point to a directory, regular file or info file. 
 * 
 * Params: sourcePath  -> Source path 
 *         targetDir   -> Target path
 *         displayHook -> Hook used to display a progress window
 *         askHook     -> Hook used to display a request window
 *         opModes     -> Holds data for various user choices
 *         userdata    -> Contains a pointer to a struct with all the Zune objects used by the display Hook
 *         first       -> Must be set to true for the initial call (if copying a directory, this function will be called recursively)
 * 
 * Result: FALSE if operation ended successfully, TRUE if it was stopped prematurely
 */
BOOL CopyContent(CONST_STRPTR sourcePath, CONST_STRPTR targetDir, struct Hook *displayHook, struct Hook *askHook, struct OpModes *opModes, APTR userdata, BOOL first)
{
    BOOL isDir;
    BOOL stop = FALSE;
    BOOL hasInfoFile = FALSE;
    BOOL doFileExists;
    STRPTR sourceInfoFilePath;
    STRPTR localSourcePath = (STRPTR)sourcePath;
    struct FileInfoBlock *sourceFib;

    BOOL quit = infoFileSetup(sourcePath, &sourceInfoFilePath, &localSourcePath, &hasInfoFile);
    if (quit)
    {
        return TRUE;
    }

    sourceFib = GetFileInfoBlock(localSourcePath);
    if (sourceFib == NULL)
    {
        stop = TRUE;
    }
    else
    {
        isDir = sourceFib->fib_DirEntryType > 0;
    }

    if (isDir && !stop)
    {
        STRPTR nextTargetDir = CombinePath(FilePart(sourcePath), targetDir);
        if (nextTargetDir != NULL)
        {
            if (first && FileExists(nextTargetDir))
            {
                stop = checkIfAlreadyExists(nextTargetDir, sourcePath, askHook, opModes);
            }

            if (!stop)
            {
                BPTR targetDirectoryLock = CreateDirectory(nextTargetDir);
                BPTR sourceDirectoryLock = ReadFileInfoBlockAndLockPath(sourcePath, sourceFib);
                stop = copyDirectory(sourcePath, nextTargetDir, askHook, opModes, userdata, displayHook, sourceFib, sourceDirectoryLock);
                UnLock(targetDirectoryLock);

                // need to copy .info file for directory
                targetDirectoryLock = Lock(targetDir, ACCESS_WRITE);
                if (hasInfoFile && first)
                {
                    STRPTR targetPath = CombinePath(FilePart(sourceInfoFilePath), targetDir);
                    if (targetPath != NULL)
                    {
                        stop = performDataCopy(sourceInfoFilePath, targetPath, sourceFib, displayHook, userdata);
                        FreeVec(targetPath);
                    }
                    else
                    {
                        stop = TRUE;
                    }
                }

                UnLock(targetDirectoryLock);
                UnLock(sourceDirectoryLock);
            }
            FreeVec(nextTargetDir);
        }
        else
        {
            stop = TRUE;
        }
    }
    else if (!stop)
    {
        BPTR sourceDirectoryLock = LockDirectory(localSourcePath);
        BPTR targetDirectoryLock = Lock(targetDir, ACCESS_WRITE);

        stop = copySingleFile(localSourcePath, sourceInfoFilePath, targetDir, first && hasInfoFile, askHook, opModes, userdata, displayHook, sourceFib, !first);

        UnLock(targetDirectoryLock);
        UnLock(sourceDirectoryLock);
    }

    FreeDosObject(DOS_FIB, (APTR)sourceFib);
    FreeVec(sourceInfoFilePath);

    return stop;
}

/**
 * Deletes all content from the given path. The path can point to a directory, regular file or info file. 
 * 
 * Params: path        -> File path
 *         opModes     -> Holds data for various user choices
 *         askHook     -> Hook used to display a request window
 *         displayHook -> Hook used to display a progress window
 *         userdata    -> Contains a pointer to a struct with all the Zune objects used by the display Hook
 * 
 * Result: FALSE if operation ended successfully, TRUE if it was stopped prematurely
 */
BOOL DeleteContent(CONST_STRPTR path, struct OpModes *opModes, struct Hook *askHook, struct Hook *displayHook, APTR userdata)
{
    BOOL stop = FALSE;
    BOOL isDir = FALSE;
    BOOL doDelete = FALSE;
    BOOL hasInfoFile = FALSE;
    STRPTR infoFilePath;
    STRPTR localPath = (STRPTR)path;

    BOOL quit = infoFileSetup(path, &infoFilePath, &localPath, &hasInfoFile);
    if (quit)
    {
        return TRUE;
    }

    struct FileInfoBlock *sourceFib = GetFileInfoBlock(localPath);
    if (sourceFib != NULL)
    {
        isDir = sourceFib->fib_DirEntryType > 0;

        doDelete = askToDeleteAndUnprotect(localPath, opModes, askHook, hasInfoFile ? infoFilePath : NULL, sourceFib);

        if (doDelete)
        {
            if (isDir)
            {
                stop = deleteDirectoryContents(localPath, opModes, askHook, sourceFib, displayHook, userdata);
            }

            if (!stop)
            {
                stop = deleteSingleFile(localPath, displayHook, userdata);

                if (hasInfoFile && !stop)
                {
                    stop = deleteSingleFile(infoFilePath, displayHook, userdata);
                }
            }
        }
    }
    else
    {
        stop = TRUE;
    }

    FreeVec((APTR)infoFilePath);

    return stop;
}