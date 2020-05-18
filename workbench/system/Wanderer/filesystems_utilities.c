#include "filesystems_utilities.h"

#include <stdarg.h>
#include <string.h>

#include <proto/dos.h>
#include <dos/dos.h>

#include <exec/exec.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/rawfmt.h>

#include <proto/muimaster.h>
#include <libraries/mui.h>

#include "locale.h"

#define BUFFER_SIZE 64
#define ERROR_LEN 1023

/*
** Copies every character in a string except for the specified character
**
** Params: s -> Source string pointer
**         d -> Destination string pointer
**         c -> Character that will not be copied
*/
static void strcrem(STRPTR s, STRPTR d, TEXT c)
{
    while (*s)
    {
        if (*s != c)
        {
            *d++ = *s;
        }
        s++;
    }
    *d = 0;
}

/**
 * Combines several strings into one string according to the format specified by using the VNewRawDoFmt 
 * function.
 *
 * Params: format -> Output string format
 *         vararg -> Strings to combine
 * Result: Concatenated and formatted string
 */
STRPTR CombineString(STRPTR format, ...)
{
    TEXT buffer[512];
	va_list args;
		
	va_start(args, format);
	VNewRawDoFmt(format, RAWFMTFUNC_STRING, buffer, args);
	va_end(args);

    WORD len = strlen(buffer);
    STRPTR retValue = AllocVec(len + 1, MEMF_CLEAR | MEMF_ANY);

    strcpy(retValue, buffer);

    return retValue;
}

/**
 * Displays a request window, can display a set of buttons each with a unique numeric return value
 * 
 * Params: title    -> Window title
 *         strg     -> Text displayed 
 *         gadgets  -> Buttons provided as a string split by the | character
 *         selected -> Position of the pre-selected button
 *         centered -> Centeres window on screen if true  
 * 
 * Result: The index of the button pressed, starting with 0
 */
WORD AskChoice(CONST_STRPTR title, CONST_STRPTR strg, CONST_STRPTR gadgets, UWORD selected, BOOL centered)
{
    Object *app, *win, *button, *buttonGroup, *selObject;
    LONG back, old;
    BOOL running = TRUE;
    ULONG signals;
    ULONG id;
    BYTE sourceBuffer[BUFFER_SIZE], destinationBuffer[BUFFER_SIZE];

    back = 0;

    app = MUI_NewObject(MUIC_Application,
    
        MUIA_Application_Title,     (IPTR)"Requester",
        MUIA_Application_Base,      (IPTR)"WANDERER_REQ",
    
        SubWindow, (IPTR)(win = MUI_NewObject(MUIC_Window,
            MUIA_Window_Title,           title,
            MUIA_Window_Activate,        TRUE,
            MUIA_Window_DepthGadget,     TRUE,
            MUIA_Window_SizeGadget,  FALSE,
            MUIA_Window_AppWindow,      FALSE,
            MUIA_Window_CloseGadget,    FALSE,
            MUIA_Window_Borderless,  FALSE,
            MUIA_Window_TopEdge,        MUIV_Window_TopEdge_Moused,
            MUIA_Window_LeftEdge,       MUIV_Window_LeftEdge_Moused,
        
            WindowContents, (IPTR)MUI_NewObject(MUIC_Group,
                Child, (IPTR)MUI_NewObject(MUIC_Text,
                    TextFrame,
                    MUIA_InnerLeft,(12),
                    MUIA_InnerRight,(12),
                    MUIA_InnerTop,(12),
                    MUIA_InnerBottom,(12),
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)strg,
                TAG_DONE),
                Child, (IPTR)(buttonGroup = MUI_NewObject(MUIC_Group,MUIA_Group_Horiz, TRUE,TAG_DONE)),
            TAG_DONE),
        TAG_DONE)),
    TAG_DONE);

    if (app)
    {
        old = 0;
        back = 11;
        selObject = NULL;

        while (old != -1)
        {
            old = SplitName(gadgets, '|', sourceBuffer, old, BUFFER_SIZE);
            if (old == -1)
            {
                back = 10;
            }

            strcrem(sourceBuffer, destinationBuffer, '_');
            button = SimpleButton(destinationBuffer);

            if (button)
            {
                if ((back - 10) == selected)
                {
                    selObject = button;
                }

                set(button, MUIA_CycleChain, 1);
                DoMethod(buttonGroup, MUIM_Group_InitChange);
                DoMethod(buttonGroup, OM_ADDMEMBER, button);
                DoMethod(buttonGroup, MUIM_Group_ExitChange);

                DoMethod(button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, back);
            }
            back++;
        }

        back = -1;

        if (centered)
        {
            set(win, MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered);
            set(win, MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered);
        }

        if (selObject)
        {
            set(win, MUIA_Window_ActiveObject, selObject);
        }

        set(win, MUIA_Window_Open, TRUE);

        while (running)
        {
            id = DoMethod(app, MUIM_Application_Input, &signals);
            switch (id)
            {
            case MUIV_Application_ReturnID_Quit:
                running = FALSE;
                break;
            case 10:
                running = FALSE;
                back = 0;
                break;
            case 11:
                running = FALSE;
                back = 1;
                break;
            case 12:
                running = FALSE;
                back = 2;
                break;
            case 13:
                running = FALSE;
                back = 3;
                break;
            case 14:
                running = FALSE;
                back = 4;
                break;
            case 15:
                running = FALSE;
                back = 5;
                break;
            case 16:
                running = FALSE;
                back = 6;
                break;
            case 17:
                running = FALSE;
                back = 7;
                break;
            case 18:
                running = FALSE;
                back = 8;
                break;
            }
            if (running && signals)
                Wait(signals);
        }

        set(win, MUIA_Window_Open, FALSE);
        MUI_DisposeObject(app);
    }

    return back;
}

/**
 * Formats an error message and displays it using the AskChoice function
 * 
 * Params: errormessage -> Error message 
 *         ioError      -> Error number from IoErr() function
 *         varargs      -> Additional information used to enrich the error message
 *  
 */
VOID DisplayIOError(CONST_STRPTR errormessage, IPTR ioError, ...)
{
    CONST_STRPTR title = _(MSG_WANDERER_ERROR_FILE_OPERATION);
    TEXT buffer[ERROR_LEN + 1];
    va_list args;

    va_start(args, ioError);
    VNewRawDoFmt(errormessage, RAWFMTFUNC_STRING, buffer, args);
    va_end(args);

    if (ioError > 0)
    {
        strcat(buffer, _(MSG_FAILED_ERROR));
        Fault(ioError, buffer, buffer, ERROR_LEN);
    }

    AskChoice(title, buffer, _(MSG_MEN_ICONSCLOSE), 0, TRUE);
}

/**
 * Checks if string ends with .info
 * 
 * Input: path -> Filepath/name string
 * Result: TRUE if filename ends with .info
*/
BOOL CheckIfInfoFile(CONST_STRPTR path)
{
    return (strncmp(path + strlen(path) - 5, ".info", 5) == 0);
}

/**
 * Allocates a FileInfoBlock struct and passes it to the DOS Examine function which 
 * fills it with data for the specified path. 
 * 
 * Params: path -> File to be examined
 * Result: FileInfoBlock struct pointer for path or NULL if any error occurs
*/
struct FileInfoBlock *GetFileInfoBlock(CONST_STRPTR path)
{
    struct TagItem DummyTags[] = {{TAG_DONE, 0}};
    struct FileInfoBlock *fib;
    LONG info, result;
    BPTR nLock;

    info = 0;

    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, DummyTags);
    if (fib != NULL)
    {
        nLock = Lock(path, ACCESS_READ);
        if (nLock)
        {
            result = Examine(nLock, fib);

            if (!result)
            {
                FreeVec(fib);
                fib = NULL;
                DisplayIOError(_(MSG_FAILED_TO_READ_FILE_INFO_BLOCK), IoErr(), (IPTR)path);
            }

            UnLock(nLock);
        }
        else
        {
            FreeVec(fib);
            fib = NULL;
            DisplayIOError(_(MSG_FAILED_TO_LOCK_PATH), IoErr(), (IPTR)path);
        }
    }
    else
    {
        DisplayIOError(_(MSG_FAILED_TO_ALLOCATE_MEMORY), -1, (IPTR)path);
    }

    return fib;
}

/**
 * Populates a provided FileInfoBlock struct pointer with data and returns
 * a read lock for the given path.
 * 
 * Params: path -> Full path
 *         fib  -> Pre-allocated FileInfoBlock struct pointer
 * Result: Shared lock for path
 */
BPTR ReadFileInfoBlockAndLockPath(CONST_STRPTR path, struct FileInfoBlock *fib)
{
    BPTR nLock;

    nLock = Lock(path, ACCESS_READ);
    if (nLock)
    {
        BOOL result = Examine(nLock, fib);
        if (result == DOSFALSE)
        {
            DisplayIOError(_(MSG_FAILED_TO_READ_FILE_INFO_BLOCK), IoErr(), (IPTR)path);
        }
    }
    else
    {
        fib = NULL;
        DisplayIOError(_(MSG_FAILED_TO_LOCK_PATH), IoErr(), (IPTR)path);
    }

    return nLock;
}

/**
 * Allocates a new string and copies the path part of the provided filepath to it.
 * 
 * Params: path -> Filepath for which path is extracted
 * Result: String pointer containing path part
 */
STRPTR GetPathPart(CONST_STRPTR path)
{
    CONST_STRPTR pathPos = PathPart(path);
    ULONG length = pathPos - path;
    STRPTR directory = AllocVec(length + 1, MEMF_CLEAR | MEMF_ANY);

    if (directory)
    {
        strncpy(directory, path, length);
        directory[length] = '\0';
    }
    else
    {
        directory = NULL;
        DisplayIOError(_(MSG_FAILED_TO_ALLOCATE_MEMORY), -1, (IPTR)&length);
    }

    return directory;
}

/*
** Concatenates the directory and filename into one filepath which is copied to a newly allocated string.
**
** Params: dir  -> Path part
**         file -> Filename
** Result: Concatenated string
*/
STRPTR CombinePath(CONST_STRPTR file, CONST_STRPTR dir)
{
    WORD length;
    WORD dirLength = strlen(dir);
    STRPTR out = NULL;

    length = dirLength + strlen(file) + 1;

    if (dir[dirLength - 1] != '/' && dir[dirLength - 1] != ':')
    {
        length++;
    }

    out = AllocVec(length, MEMF_CLEAR | MEMF_ANY);

    if (out != NULL)
    {
        strcpy(out, dir);
        AddPart(out, file, length);
    }
    else
    {
        DisplayIOError(_(MSG_FAILED_TO_ALLOCATE_MEMORY), -1, (IPTR)&length);
    }

    return out;
}

/**
 * Copies the comment and protection data from the source to the target.
 * If a FileInfoBlock struct pointer is provided, it will be used. If not, it will read the 
 * FileInfoBlock for the source path.
 * 
 * Params: fib        -> Existing FileInfoBlock struct pointer (optional)
 *         sourcePath -> Source path
 *         destPath   -> Target path
 * Result: TRUE if the file info were copied successfully
 */
BOOL CopyFileInfo(struct FileInfoBlock *fib, CONST_STRPTR sourcePath, CONST_STRPTR destPath)
{
    struct FileInfoBlock *localfib = (fib == NULL) ? GetFileInfoBlock(sourcePath) : fib;

    if (localfib != NULL)
    {
        CONST_STRPTR comment = localfib->fib_Comment;
        LONG protection = localfib->fib_Protection;

        SetComment(destPath, comment);
        SetProtection(destPath, protection);

        if (fib == NULL)
        {
            FreeDosObject(DOS_FIB, (APTR)localfib);
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * Checks if file exists by attempting to obtain a lock on the file
 * 
 * Params: path -> File path to check
 * Result: TRUE if it exists, FALSE if not
 */
BOOL FileExists(CONST_STRPTR path)
{
    BPTR lock = Lock(path, ACCESS_READ);

    if (!lock)
    {
        return FALSE;
    }

    UnLock(lock);
    return TRUE;
}

/**
 * Allocates a new string and fills it with the file path + .info  
 * 
 * Input: path -> File path 
 * Result: Constructed file path 
 */
STRPTR ConstructInfofileName(CONST_STRPTR path)
{
    STRPTR infoFileName = AllocVec(strlen(path) + 6, MEMF_CLEAR | MEMF_ANY);

    if (infoFileName)
    {
        
        strcpy(infoFileName, path);
        strcat(infoFileName, ".info");
    }
    else
    {
        infoFileName = NULL;
        DisplayIOError(_(MSG_FAILED_TO_ALLOCATE_MEMORY), -1, (IPTR)path);
    }

    return infoFileName;
}

/**
 * Acquires a read lock for path
 * 
 * Params: path -> File path to obtain a lock for
 * Result: lock for path, 0 if anything failes
 */
BPTR LockDirectory(CONST_STRPTR path)
{
    STRPTR directory = GetPathPart(path);

    if (directory)
    {
        BPTR sourceDirectoryLock = Lock(directory, ACCESS_READ);
        FreeVec(directory);

        if (sourceDirectoryLock != 0)
        {
            return sourceDirectoryLock;
        }
        else
        {
            DisplayIOError(_(MSG_FAILED_TO_LOCK_PATH), IoErr(), (IPTR)path);
        }
    }

    return 0;
}

/** 
 * Checks if path points to a directory
 * 
 * Params: path to check
 * Result: TRUE if path points to a directory, FALSE if not
 */
BOOL IsDirectory(CONST_STRPTR path)
{
    BOOL retValue = FALSE;
    struct FileInfoBlock *fib = GetFileInfoBlock(path);

    if (fib != NULL)
    {
        retValue = fib->fib_DirEntryType == 2;
        FreeDosObject(DOS_FIB, fib);
    }

    return retValue;
}