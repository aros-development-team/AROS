/*
    Copyright © 2007-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include  "filesystems.h"

struct TagItem DummyTags[] = { {TAG_DONE, 0}};

/* define some nonglobal data that can be used by the display hook */

///strcrem()
static void strcrem(char *s, char *d, char c) 
{
    while (*s) {
        if (*s != c) *d++= *s;
        s++;
    }
    *d = 0;
}
///

///AskChoiceNew()
WORD AskChoiceNew(const char *title, const char *strg, const char *gadgets, UWORD sel, BOOL centered) 
{

    Object  *app, *win;
    Object  *button, *bObject, *selObject;
    LONG       back, old;
    BOOL    running = TRUE;
    ULONG   signals;
    ULONG   id;
    char    Buffer[64], Buffer1[64];

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
                Child, (IPTR)(bObject = MUI_NewObject(MUIC_Group,MUIA_Group_Horiz, TRUE,TAG_DONE)),
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
            old = SplitName(gadgets, '|', Buffer, old, sizeof(Buffer));
            if (old == -1) back = 10;
            strcrem(Buffer, Buffer1, '_');
            button = SimpleButton(Buffer1);
            if (button) 
            {
                if ((back-10) == sel) selObject = button;
                set(button, MUIA_CycleChain, 1);
                DoMethod(bObject, MUIM_Group_InitChange);
                DoMethod(bObject, OM_ADDMEMBER, button);
                DoMethod(bObject, MUIM_Group_ExitChange);
                DoMethod(button,MUIM_Notify,MUIA_Pressed, FALSE, app,2,MUIM_Application_ReturnID,back);
            }
            back++;
        }

        back = -1;

        if (centered) 
        {
            set (win, MUIA_Window_TopEdge,        MUIV_Window_TopEdge_Centered);
            set (win, MUIA_Window_LeftEdge,       MUIV_Window_LeftEdge_Centered);
        }
        if (selObject) set(win, MUIA_Window_ActiveObject, selObject);
        set(win,MUIA_Window_Open,TRUE);

        while (running) 
        {
            id = DoMethod(app,MUIM_Application_Input,&signals);
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
            if (running && signals) Wait(signals);
        }
        set(win,MUIA_Window_Open,FALSE);
        MUI_DisposeObject(app);
    }
    return back;
}
///

/// AskChoice()
WORD AskChoice(const char *title, const char *strg, const char *gadgets, UWORD sel) 
{
    return AskChoiceNew(title, strg, gadgets, sel, FALSE);
}
///

///AskChoiceCentered()
WORD AskChoiceCentered(const char *title, const char *strg, const char *gadgets, UWORD sel) 
{
    return AskChoiceNew(title, strg, gadgets, sel, TRUE);
}
///

///combinePath()
static char *combinePath(APTR pool, char *path, char *file)
{
    int l;
    char *out;
    if ((path == NULL) || (file == NULL)) return NULL;
    if (strlen(path) == 0) return NULL;
    if (strlen(file) == 0) return NULL;
    
    l = strlen(path) + strlen(file) + 1;
    if (path[strlen(path)-1] != '/') l++;

    if (pool == NULL) out = AllocVec(l, MEMF_CLEAR); 
    else out = AllocVecPooled(pool, l);
    
    if (out) 
    {
        strcpy(out, path);
        AddPart(out, file, l);
    }
    return out;
}
///

///allocPath()
static char *allocPath(APTR pool, char *str)
{
    char *s0, *s1, *s;
    int  l;

    s = NULL;
    s0 = str;

    s1 = PathPart(str);
    if (s1) 
    {
        for (l=0; s0 != s1; s0++,l++);
        
        s = AllocVecPooled(pool, l+1);
        if (s) strncpy(s, str, l);
    }
    return s;
}
///

///freeString()
void freeString(APTR pool, char *str) 
{
    if (str) 
    {
        if (pool == NULL) 
            FreeVec(str); 
        else 
            FreeVecPooled(pool, str);
    }
}
///

///allocString()
/*
** allocates memory for a string and copies them to the new buffer
**
** inputs:     str      source string
** return:     char     pointer to string or NULL
**
*/              
static char *allocString(APTR pool, char *str) 
{
    char  *b;
    int  l;

    if (str == NULL) return NULL;

    l = strlen(str);
    
    if (pool == NULL) 
        b = (char*) AllocVec(l+1, MEMF_CLEAR); 
    else b = (char*) AllocVecPooled(pool, l+1);
    
    if (b && (l>0)) strncpy (b, str, l);
    return b;
}
///

///CombineString()
char *CombineString(char *format, ...)
{
    int   cnt = 0, cnt1;
    int   len;
    char  *s, *s1, *str, *p;
    char  *back = NULL;

    va_list  ap;
    s = format;
    while ((s = strstr(s,"%s")) != NULL) {cnt++; s++; }

    if (cnt >0) 
    {
        len = strlen(format) - 2*cnt;
        va_start(ap, format);
        cnt1 = cnt;

        while (cnt1--) 
        {
            p = va_arg(ap, char *);
            len += strlen(p);
        };
        va_end(ap);
        len++;
        if (len>0) 
        {
            back = AllocVec(len, MEMF_CLEAR);
            if (back) 
            {
                str = back;
                s = format;
                va_start(ap, format);
                while ((s1 = strstr(s, "%s")) != NULL) 
                {
                    p = va_arg(ap, char *);
                    len = s1-s;
                    strncpy(str, s, len);
                    s = s1+2;
                    str += len;
                    strncpy(str, p, strlen(p));
                    str += strlen(p);
                };
                if (s) strncpy(str, s, strlen(s));
                va_end(ap);
            }
        }
    }
    return back;
}
///

///GetFileInfo()
static LONG GetFileInfo(char *name)
{
    struct  FileInfoBlock  *FIB;
    LONG       info,Success2;
    BPTR       nLock;

    info = -1;

    FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
    if (FIB) 
    {
        nLock = Lock(name, ACCESS_READ);
        if (nLock) 
        {
            Success2 = Examine(nLock,FIB);
            if (Success2) 
            {
                info = 0;
                if (FIB->fib_DirEntryType>0) info |= FILEINFO_DIR;
                if ((FIB->fib_Protection & FIBF_DELETE) != 0) info |= FILEINFO_PROTECTED;
                if ((FIB->fib_Protection & FIBF_WRITE) != 0) info |= FILEINFO_WRITE;
            }
            UnLock(nLock);
        }
        FreeDosObject (DOS_FIB,(APTR) FIB);
    }
    return info;
}
///

///GetProtectionInfo()
static LONG GetProtectionInfo(char *name)
{
    struct  FileInfoBlock  *FIB;
    LONG       info,Success2;
    BPTR       nLock;

    info = 0;

    FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
    if (FIB) 
    {
        nLock = Lock(name, ACCESS_READ);
        if (nLock) 
        {
            Success2 = Examine(nLock,FIB);
            if (Success2) 
            {
                info = FIB->fib_Protection;
            }
            UnLock(nLock);
        }
        FreeDosObject (DOS_FIB,(APTR) FIB);
    }
    return info;
}
///

///GetCommentInfo()
static char *GetCommentInfo(APTR pool, char *name)
{
    struct  FileInfoBlock  *FIB;
    LONG       Success2;
    BPTR       nLock;
    char       *info;

    info = NULL;

    FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
    if (FIB) 
    {
        nLock = Lock(name, ACCESS_READ);
        if (nLock) 
        {
            Success2 = Examine(nLock,FIB);
            if (Success2) 
            {
                info = allocString(pool, (char*) &FIB->fib_Comment);
            }
            UnLock(nLock);
        }
        FreeDosObject (DOS_FIB,(APTR) FIB);
    }
    return info;
}
///

///deleteFile()
static BOOL deleteFile(char *file)
{
    DeleteFile(file);
    return TRUE;
}
///

///copyFile()
static BOOL copyFile(APTR pool, char *file, char *destpath,
    struct FileInfoBlock *fileinfo, struct Hook *displayHook,
    struct dCopyStruct *display)
{
    struct  FileInfoBlock  *fib;
    char   *to;
    LONG   clen, wlen;
    LONG bufferlen = COPYLEN;
    LONG         filelen = 0;
    BOOL   quit = TRUE;
    BPTR   in, out;
    BYTE   *buffer;
    BPTR   nLock;

    if (display != NULL) display->totallen = 0;
    if (display != NULL) display->actlen = 0;
    if (fileinfo) 
    {
        filelen = fileinfo->fib_Size;
        if (fileinfo->fib_Size <= COPYLEN) bufferlen = fileinfo->fib_Size;
        if (bufferlen < 8192) bufferlen = 8192;
        fib = fileinfo;
    }
    else
    {
        fib = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
        if (fib) 
        {
            nLock = Lock(file, ACCESS_READ);
            if (nLock) 
            {
                if (Examine(nLock,fib) == 0)
                {
                    UnLock(nLock);
                    return TRUE;
                }
                UnLock(nLock);
                filelen = fib->fib_Size;
                if (fib->fib_Size <= COPYLEN) bufferlen = fib->fib_Size;
                if (bufferlen < 8192) bufferlen = 8192;
            }
        }
        else
        {
            return TRUE;
        }
    }
    to = combinePath(pool, destpath, FilePart(file));
    if (to) 
    {
        buffer = AllocVecPooled(pool, bufferlen);
        if (buffer) 
        {
            in = Open(file, MODE_OLDFILE);
            if (in) 
            {
                out = Open(to, MODE_NEWFILE);
                if (out) 
                {
                    BOOL stop = FALSE;
                    unsigned int difftime = clock();
                    do 
                    {
                        clen = Read(in, buffer, bufferlen);
                        if ((clen !=0) && (clen != -1)) 
                        {
                            wlen = Write(out, buffer,clen);
                            if (display)
                            {
                                display->difftime = clock() - difftime;
                                if (display->difftime < 1) display->difftime = 1;
                                display->actlen = clen;
                                display->totallen += clen;
                                display->filelen = filelen;
                                if (displayHook)
                                {
                                    display->flags |= ACTION_UPDATE;
                                    stop = CallHook(displayHook, (Object *) display, NULL);
                                }
                            }

                            if (clen != wlen) clen = 0;
                        }
                    }
                    while ((clen !=0) && (clen != -1) && !stop);
                    
                    quit = stop;

                    Close(out);
                    
                    if (fib) 
                    {
                        SetComment(to, fib->fib_Comment);
                        SetProtection(to, fib->fib_Protection);
                    }
                }
                Close(in);
            }
            FreeVecPooled(pool, buffer);
        }
        freeString(pool, to);
    }
    if (fileinfo == NULL) FreeDosObject(DOS_FIB,(APTR) fib);

    return quit;
}
///

static VOID handleUnprotect(LONG info, char * spath, char * file, char * target, struct Hook * askHook, WORD * pmode, BOOL * unprotect)
{
    struct  dCopyStruct askDisplay;

    if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0)
    {
        if (*pmode != OPMODE_NONE)
        {
            if (
                (*pmode == OPMODE_ASK) || (*pmode == OPMODE_YES) ||
                (*pmode == OPMODE_ALL)  || (*pmode == OPMODE_NO)
            )
            {
                askDisplay.spath = spath;
                askDisplay.file = file;
                askDisplay.type = 1;
                askDisplay.filelen = 0;
                if (*pmode != OPMODE_ALL) *pmode = CallHook(askHook, (Object *) &askDisplay, NULL);
                if ((*pmode == OPMODE_ALL) || (*pmode == OPMODE_YES))
                {
                    SetProtection(target, 0);
                    *unprotect = TRUE;
                }
            }
        }
    }
    else *unprotect = TRUE;
}

#define dmode (opModes->deletemode)
#define pmode (opModes->protectmode)
#define omode (opModes->overwritemode)
///actionDir()
static BOOL actionDir(APTR pool, ULONG flags, char *source, char *dest,
    BOOL quit, struct Hook *dHook, struct OpModes * opModes, APTR userdata)
{
    struct  FileInfoBlock  *FIB, *FIB2;
    struct  dCopyStruct    display;
    struct  dCopyStruct    askDisplay;
    struct  FileEntry    *fe, *fef, *fel;

    BPTR       NewLock, cDir, nDir, nLock;
    ULONG      Success, Success1, Success2, DosError, len;
    char       *dname, *comment, *dpath;
    BOOL       del, created, unprotect, failure;
    BOOL       overwrite;
    LONG       info, prot;

    if (quit) return TRUE;

    created = FALSE;

    display.userdata = userdata;
    askDisplay.userdata = userdata;

    fef = NULL;
    fel = NULL;
    
    FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
    if (FIB) 
    {
        FIB2 = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
        if (FIB2) 
        {
            NewLock = Lock(source ,ACCESS_READ);
            if (NewLock) 
            {
                cDir = CurrentDir(NewLock);

                Success1=Examine(NewLock,FIB);
                if (Success1) 
                {
                    do 
                    {
                        /* For each entry in directory ... */
                        Success=ExNext(NewLock,FIB);

                        /* If OPMODE_NONE, stop all actions */
                        if ((flags & (ACTION_DELETE | ACTION_COPY)) == ACTION_DELETE) 
                        {
                            if (dmode == OPMODE_NONE) Success = FALSE;
                        }

                        if (Success && Success1) 
                        {
                            if (FIB->fib_DirEntryType>0) 
                            {
                                /* The entry is a directory */
                                dname = NULL;

                                /* If copying, check if the destination directory exists and create if needed */
                                if (((flags & ACTION_COPY) != 0) && dest) 
                                {
                                    dname = combinePath(pool, dest, FIB->fib_FileName);
                                    if (dname) 
                                    {
                                        created = FALSE;
                                        nLock = Lock(dname, ACCESS_READ);
                                        if (nLock) 
                                        {
                                            Success2 = Examine(nLock,FIB2);
                                            if (Success2) if (FIB2->fib_DirEntryType>0) created = TRUE;
                                            UnLock(nLock);
                                        }
                                        if (!created) 
                                        {
                                            nDir = CreateDir(dname);
                                            if (nDir) 
                                            {
                                                prot = GetProtectionInfo(FIB->fib_FileName);
                                                comment = GetCommentInfo(pool, FIB->fib_FileName);
                                                if (comment) SetComment(dname, comment);
                                                SetProtection(dname, prot);
                                                freeString(pool, comment);

                                                created = TRUE;
                                                UnLock(nDir);
                                            }
                                        }
                                    }
                                }
                                unprotect = FALSE;
                                del = FALSE;

                                /* If deleting, ask for confirmation and ask to unprotect */
                                if (opModes && (dmode != OPMODE_NONE) && ((flags & ACTION_DELETE) != 0))
                                {
                                    if ((dmode == OPMODE_ASK) || (dmode == OPMODE_YES) || (dmode == OPMODE_ALL) || (dmode == OPMODE_NO))
                                    {
                                        askDisplay.spath = FIB->fib_FileName;
                                        askDisplay.file = NULL;
                                        askDisplay.type = 0;
                                        askDisplay.filelen = FIB->fib_Size;
                                        if (dmode != OPMODE_ALL) dmode = CallHook(opModes->askhook, (Object *) &askDisplay, NULL);
                                        if ((dmode == OPMODE_ALL) || (dmode == OPMODE_YES))
                                        {

                                            unprotect = FALSE;
                                            info = GetFileInfo(FIB->fib_FileName);

                                            handleUnprotect(info, FIB->fib_FileName, NULL, FIB->fib_FileName, opModes->askhook, &pmode, &unprotect);
                                            
                                            if (unprotect) 
                                            {
                                                del = TRUE;
                                            }
                                        }
                                    }
                                }

                                /* If directory was created or we are deleting, trawl deeper and repeat actions */
                                if (created || ((flags & ACTION_DELETE) !=0)) 
                                {
                                    if (((dmode == OPMODE_NO) || (dmode == OPMODE_NONE)) && (flags == ACTION_DELETE))
                                    {
                                        quit = FALSE;
                                    } 
                                    else 
                                    {
                                        quit = actionDir(pool, flags, FIB->fib_FileName, dname, quit, dHook, opModes, userdata);
                                    }
                                }

                                /* If deleting and all actions succeeded in deeper directory, add the directory to "to be deleted" list */
                                if (!quit && del && unprotect) 
                                {
                                    if (FIB->fib_FileName) 
                                    {
                                        len = strlen(FIB->fib_FileName);
                                        if (len>0) 
                                        {
                                            
                                            fe = AllocVecPooled(pool, sizeof(struct FileEntry) + len);
                                            if (fe) 
                                            {
                                                strcpy(fe->name, FIB->fib_FileName);
                                                if (fel) 
                                                {
                                                    fel->next = fe;
                                                    fel = fe;
                                                } 
                                                else 
                                                {
                                                    fef = fe;
                                                    fel = fe;
                                                }
                                            }
                                        }
                                    }
                                }
                                freeString(pool, dname);
                            } 
                            else 
                            {
                                /* The entry is a file */
                                if (dHook) 
                                {
                                    display.file = FIB->fib_FileName;
                                    display.filelen = FIB->fib_Size;
                                    display.spath = source;
                                    display.dpath = dest;
                                    display.flags = (flags &= ~ACTION_UPDATE);
                                    display.totallen = 0;
                                    display.actlen = 0;

                                    quit = CallHook(dHook, (Object *) &display, NULL);
                                }

                                overwrite = TRUE;

                                /* If copying, ask for overwrite destinatin and ask for unprotect destination */
                                if (((flags & ACTION_COPY) != 0) && dest)
                                {
                                    dpath = combinePath(pool, dest, FIB->fib_FileName);
                                    if (dpath) 
                                    {
                                        info = GetFileInfo(dpath);
                                        if (info != -1) 
                                        {
                                            overwrite = FALSE;
                                            if (opModes && (omode != OPMODE_NONE))
                                            {
                                                if (
                                                    (omode == OPMODE_ASK) || (omode == OPMODE_YES) ||
                                                    (omode == OPMODE_ALL) || (omode == OPMODE_NO)
                                                ) 
                                                {
                                                    askDisplay.spath = dest;
                                                    askDisplay.file = FIB->fib_FileName;
                                                    askDisplay.type = 2;
                                                    askDisplay.filelen = 0;
                                                    if (omode != OPMODE_ALL) omode = CallHook(opModes->askhook, (Object *) &askDisplay, NULL);
                                                    if ((omode == OPMODE_ALL) || (omode == OPMODE_YES))
                                                    {
                                                        handleUnprotect(info, dest, FIB->fib_FileName, dpath, opModes->askhook, &pmode, &overwrite);
                                                    }
                                                }
                                            }
                                        }
                                        freeString(pool, dpath);
                                    }
                                }

                                /* Copy the file */
                                failure = FALSE;
                                if (!quit && ((flags & ACTION_COPY) !=0) && overwrite)
                                {
                                    display.flags = (flags &= ~ACTION_UPDATE);
                                    failure = copyFile(pool, FIB->fib_FileName, dest, FIB, dHook, &display);
                                }

                                /* If failed, ask if process should continue */
                                if (failure && !quit) 
                                {
                                    if (opModes)
                                    {
                                        askDisplay.spath = source;
                                        askDisplay.file = FIB->fib_FileName;
                                        askDisplay.type = 3;
                                        askDisplay.filelen = 0;
                                        if (CallHook(opModes->askhook, (Object *) &askDisplay, NULL) == ACCESS_SKIP)
                                            quit = FALSE; else quit = TRUE;
                                    } 
                                    else quit = FALSE;
                                }

                                /* If deleting ask for confirmation and ask to unprotect */
                                if (!quit && opModes && (dmode != OPMODE_NONE) && ((flags & ACTION_DELETE) !=0))
                                {
                                    if (
                                        (dmode == OPMODE_ASK) || (dmode == OPMODE_YES) ||
                                        (dmode == OPMODE_ALL) || (dmode == OPMODE_NO)
                                    ) 
                                    {
                                        askDisplay.spath = source;
                                        askDisplay.file = FIB->fib_FileName;
                                        askDisplay.type = 0;
                                        askDisplay.filelen = FIB->fib_Size;
                                        if (dmode != OPMODE_ALL) dmode = CallHook(opModes->askhook, (Object *) &askDisplay, NULL);
                                        if ((dmode == OPMODE_ALL) || (dmode == OPMODE_YES))
                                        {

                                            info = GetFileInfo(FIB->fib_FileName);
                                            unprotect = FALSE;

                                            handleUnprotect(info, source, FIB->fib_FileName, FIB->fib_FileName, opModes->askhook, &pmode, &unprotect);

                                            /* If file ready to be deleted, add it to "to be deleted" list */
                                            if (unprotect) 
                                            {
                                                if (FIB->fib_FileName) 
                                                {
                                                    len = strlen(FIB->fib_FileName);
                                                    if (len>0) 
                                                    {
                                                        
                                                        fe = AllocVecPooled(pool, sizeof(struct FileEntry) + len);
                                                        if (fe) 
                                                        {
                                                            strcpy(fe->name, FIB->fib_FileName);
                                                            if (fel) 
                                                            {
                                                                fel->next = fe;
                                                                fel = fe;
                                                            } 
                                                            else 
                                                            {
                                                                fef = fe;
                                                                fel = fe;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                            }
                        } 
                        else 
                        {
                            DosError=IoErr();
                            /* Is this the end of entries in directory ? */
                            if (DosError!=ERROR_NO_MORE_ENTRIES) Success=TRUE;
                        }
                    } 
                    while (Success && !quit);
                }

                /* Delete all directories and files which were put on the "to be deleted" list */
                while (fef) 
                {
                    len = strlen(fef->name);
                    if (len > 0) 
                    {
                        deleteFile(fef->name);
                    }
                    fe = fef->next;
                    FreeVecPooled(pool, fef);
                    fef = fe;
                }

                CurrentDir(cDir);
                UnLock(NewLock);
            }
            FreeDosObject (DOS_FIB,(APTR) FIB2);
        }
        FreeDosObject (DOS_FIB,(APTR) FIB);
    }

    return quit;
}
///
#undef dmode
#undef pmode
#undef omode

///CopyContent()
BOOL CopyContent(APTR p, char *s, char *d, BOOL makeparentdir, ULONG flags, struct Hook *displayHook, struct OpModes *opModes, APTR userdata) 
{

    struct  FileInfoBlock  *FIB;
    struct  dCopyStruct    display;
    struct  dCopyStruct    askDisplay;
    char       *destname, *dest, *path, *comment, *dpath, *infoname, *destinfo;
    //LONG       len;
    LONG        Success2, prot;
    BPTR       nLock, nDir;
    APTR        pool;
    BOOL       created = FALSE;
    BOOL       dir = TRUE;
    BOOL       back = FALSE;
    BOOL       deletesrc, unprotectsrc;
    LONG       info;

    if (p == NULL) 
    {
        pool = CreatePool(MEMF_CLEAR|MEMF_ANY, POOLSIZE, POOLSIZE);
    } 
    else pool = p;

    if (pool == NULL) return FALSE;

    infoname = AllocVecPooled(pool, strlen(s)+6);
    display.userdata = userdata;
    askDisplay.userdata = userdata;
    
    if (infoname) 
    {
        strncpy (infoname, s, strlen(s));
        strcat(infoname,".info");
    }

    if (d) destinfo = AllocVecPooled(pool, strlen(d)+6); else destinfo = NULL;

    if (destinfo) 
    {
        strncpy (destinfo, d, strlen(d));
        strcat(destinfo,".info");
    }
    
    destname = FilePart(s);

    info = GetFileInfo(s);

    if (info == -1) 
    {
        freeString(pool, infoname);
        freeString(pool, destinfo);
        if (p == NULL) DeletePool(pool);
        return TRUE;
    }

    if ((info & FILEINFO_DIR) != 0) dir = TRUE; else dir = FALSE;

    dest = NULL;

    if ((flags & ACTION_COPY) !=0 ) dest = allocString(pool, d);

    /* If copying a directory, create target directory */
    if (makeparentdir && dir && dest) 
    {
        if (destname) 
        {
            if (strlen(destname)>0) 
            {
                freeString(pool, dest);
                dest = NULL;
                FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
                if (FIB) 
                {
                    dest = combinePath(pool, d, destname);
                    if (dest) 
                    {
                        nLock = Lock(dest, ACCESS_READ);
                        if (nLock) 
                        {
                            Success2 = Examine(nLock,FIB);
                            if (Success2) if (FIB->fib_DirEntryType>0) created = TRUE;
                            UnLock(nLock);
                        }
                        if (!created) 
                        {
                            nDir = CreateDir(dest);
                            if (nDir) 
                            {
                                created = TRUE;
                                UnLock(nDir);
                                prot = GetProtectionInfo(s);
                                comment = GetCommentInfo(pool, s);
                                if (comment) SetComment(dest, comment);
                                SetProtection(dest, prot);
                                freeString(pool, comment);
                            }
                        }
                        if (!created) 
                        {
                            freeString(pool, dest);
                            dest = NULL;
                            created = FALSE;
                        }
                    }
                    FreeDosObject (DOS_FIB,(APTR) FIB);
                }
            }
        }
    }

    path = NULL;

    deletesrc = FALSE;
    unprotectsrc = TRUE;
    /* If deleting ask for confirmation, ask to unprotect */
    if (opModes && (opModes->deletemode != OPMODE_NONE) && ((flags & ACTION_DELETE) != 0) && ((makeparentdir && dir) || !dir))
    {
        if (dir) 
        {
            askDisplay.spath = s;
            askDisplay.file = NULL;
        } 
        else 
        {
            path = allocPath(pool, s);
            askDisplay.spath = path;
            askDisplay.file = FilePart(s);
        }
        askDisplay.type = 0;

        if (opModes->deletemode != OPMODE_ALL) opModes->deletemode = CallHook(opModes->askhook, (Object *) &askDisplay, NULL);
        if ((opModes->deletemode == OPMODE_ALL) || (opModes->deletemode == OPMODE_YES))
        {
            deletesrc = TRUE;
            if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0)
            {
                askDisplay.type = 1;
                unprotectsrc = FALSE;
                opModes->protectmode = CallHook(opModes->askhook, (Object *) &askDisplay, NULL);
                if ((opModes->protectmode == OPMODE_ALL) || (opModes->protectmode == OPMODE_YES))
                {
                    SetProtection(s, 0);
                    if (infoname) SetProtection(infoname, 0);
                    unprotectsrc = TRUE;
                }
            }
        }
    }

    /* If copying ask to overwrite, ask to unprotect */
    if (dest) 
    {
        if (opModes && !dir)
        {
            dpath = combinePath(pool, d, FilePart(s));
            if (dpath) 
            {
                info = GetFileInfo(dpath);
                if (info != -1) 
                {
                    if (opModes && (opModes->overwritemode != OPMODE_NONE))
                    {
                        if (
                            (opModes->overwritemode == OPMODE_ASK) || (opModes->overwritemode == OPMODE_YES) ||
                            (opModes->overwritemode == OPMODE_ALL) || (opModes->overwritemode == OPMODE_NO)
                        ) 
                        {
                            askDisplay.spath = d;
                            askDisplay.file = FilePart(s);
                            askDisplay.type = 2;
                            if (opModes->overwritemode != OPMODE_ALL) opModes->overwritemode = CallHook(opModes->askhook, (Object *) &askDisplay, NULL);
                            if ((opModes->overwritemode == OPMODE_ALL) || (opModes->overwritemode == OPMODE_YES))
                            {
                                if (((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0) && (opModes->protectmode != OPMODE_NONE))
                                {
                                    if (
                                        (opModes->protectmode == OPMODE_ASK) || (opModes->protectmode == OPMODE_YES) ||
                                        (opModes->protectmode == OPMODE_ALL) || (opModes->protectmode == OPMODE_NO)
                                    ) 
                                    {
                                        askDisplay.spath = d;
                                        askDisplay.file = FilePart(s);
                                        askDisplay.type = 1;
                                        if (opModes->protectmode != OPMODE_ALL) opModes->protectmode = CallHook(opModes->askhook, (Object *) &askDisplay, NULL);
                                        if ((opModes->protectmode == OPMODE_ALL) || (opModes->protectmode == OPMODE_YES))
                                        {
                                            SetProtection(dpath, 0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                freeString(pool, dpath);
            }
        }
    }
    freeString(pool, path);

    if (dir) 
    {
        if (dest || ((flags & ACTION_DELETE) != 0))
        {
            if (
                ((opModes->deletemode == OPMODE_NONE) || (opModes->deletemode == OPMODE_NO)) &&
                (flags & (ACTION_DELETE|ACTION_COPY)) == ACTION_DELETE
            ) 
            {
                back = FALSE; 
            }
            else 
            {
                back = actionDir(pool, flags, s, dest, FALSE, displayHook, opModes, userdata);
            }
        } 
        else back = TRUE;
    } 
    else 
    {
        if (flags == ACTION_DELETE) back = FALSE; 
        else
        {
            STRPTR path = allocPath(pool, s);
            display.file = FilePart(s);
            display.filelen = 0;
            display.totallen = 0;
            display.actlen = 0;

            if (path) display.spath = path; else display.spath = s;
            display.dpath = d;
            display.flags = (flags &= ~ACTION_UPDATE);
            if (displayHook) CallHook(displayHook, (Object *) &display, NULL);
            back = copyFile(pool, s, d, NULL, displayHook, &display);
            freeString(pool, path);
        }
    }

    if (!back && destinfo && infoname) 
    {
        SetProtection(destinfo, 0);
        copyFile(pool, infoname, d, NULL, NULL, NULL);
    }

    if (!back && opModes && (opModes->deletemode != OPMODE_NONE) && ((flags & ACTION_DELETE) !=0))
    {
        if (unprotectsrc && deletesrc)
        {
            deleteFile(s);
            if (infoname) deleteFile(infoname);
        }
    }
    
    freeString(pool, infoname);
    freeString(pool, destinfo);
    freeString(pool, dest);

    if (p == NULL) 
        DeletePool(pool);
    return !back;
}
///
