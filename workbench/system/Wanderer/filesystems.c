/*
    Copyright © 2007-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include  "filesystems.h"

struct TagItem DummyTags[] = { {TAG_DONE, 0}};

/* define some nonglobal data that can be used by the display hook */

///strcrem()
void  strcrem(char *s, char *d, char c) 
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
char *combinePath(APTR pool, char *path, char *file) 
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
char *allocPath(APTR pool, char *str) 
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
char *allocString(APTR pool, char *str) 
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

///InfoRename()
void InfoRename(APTR pool, char *from, char *to) 
{
    char *frominfo, *toinfo;

    if ((from == NULL) || (to == NULL)) return;

    frominfo = AllocVecPooled(pool, strlen(from)+6);

    if (frominfo) 
    {
        strncpy (frominfo, from, strlen(from));
        strcat(frominfo,".info");
        toinfo = AllocVecPooled(pool, strlen(to)+6);

        if (toinfo) 
        {
            strncpy (toinfo, to, strlen(to));
            strcat(toinfo,".info");
            if (Rename(from, to)) Rename(frominfo, toinfo);
            freeString(pool, toinfo);
        }
        freeString(pool, frominfo);
    }
}
///

///allocPathFromLock()
char  *allocPathFromLock(APTR pool, BPTR lock) 
{
    char *pathb, *path;

    path = NULL;
    pathb = AllocVecPooled(pool, PATHBUFFERSIZE);
    if (pathb) 
    {
        if (NameFromLock(lock, pathb, PATHBUFFERSIZE)) 
        {
            path = allocString(pool, pathb);
        }
        FreeVecPooled(pool, pathb);
    }
    return path;
}
///

///combineString()
char  *CombineString(char *format, ...) {

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

///isPathRecursive()
ULONG isPathRecursive(APTR pool, char *source, char *destination) 
{
    BPTR          srcLock, destLock;
    ULONG     back;
    char        *p1, *p2;

    back = PATH_NOINFO;
    srcLock = Lock(source, SHARED_LOCK);
    if (srcLock) 
    {
        destLock = Lock(destination, SHARED_LOCK);
        if (destLock) 
        {
            p1 = allocPathFromLock(pool, srcLock);
            if (p1) 
            {
                p2 = allocPathFromLock(pool, destLock);
                if (p2) 
                {
                    if (strstr(p2, p1) == p2) back = PATH_RECURSIVE; else back = PATH_NONRECURSIVE;

                    freeString(pool, p2);
                }
                freeString(pool, p1);
            }
            UnLock(destLock);
        }
        UnLock(srcLock);
    }
    return back;
}
///

///FileExists()
BOOL FileExists(char *name) 
{
    BOOL       info;
    BPTR       nLock;
    APTR       win;
    struct   Task      *t;

    t = FindTask(NULL);
    win = ((struct Process *) t)->pr_WindowPtr;
    ((struct Process *) t)->pr_WindowPtr = (APTR) -1;     //disable error requester

    info = FALSE;
    nLock = Lock(name, SHARED_LOCK);
    if (nLock) 
    {
        UnLock(nLock);
        info = TRUE;
    }
    ((struct Process *) t)->pr_WindowPtr = win;      //enable error requester
    return info;
}
///

///GetFileLength()
LONG GetFileLength(char *name) 
{
    LONG       info = -1;
    BPTR       in;
    APTR       win;
    struct   Task      *t;

    t = FindTask(NULL);
    win = ((struct Process *) t)->pr_WindowPtr;
    ((struct Process *) t)->pr_WindowPtr = (APTR) -1;     //disable error requester

    in = Open(name, MODE_OLDFILE);
    if (in) {
        Seek(in, 0, OFFSET_END);
        info = Seek(in, 0, OFFSET_BEGINNING);
        Close(in);
    }
    ((struct Process *) t)->pr_WindowPtr = win;      //enable error requester
    return info;
}
///

///DisposeFileInformations()
void DisposeFileInformations(APTR pool, struct FileInfo *fi) 
{
    if (fi->comment) freeString(pool, fi->comment);
    fi->comment = NULL;
}
///

///GetFileInformations()
BOOL GetFileInformations(APTR pool, char *name, struct FileInfo *fi) 
{
    struct  FileInfoBlock  *FIB;
    LONG    Success2;
    BOOL    info = FALSE;
    BPTR    nLock;

    fi->len = 0;
    fi->comment = NULL;
    fi->protection = 0;
    
    FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
    if (FIB) 
    {
        nLock = Lock(name, ACCESS_READ);
        if (nLock) 
        {
            Success2 = Examine(nLock,FIB);
            if (Success2) 
            {
                info = TRUE;
                fi->len = FIB->fib_Size;
                if (strlen(FIB->fib_Comment) > 0) fi->comment = allocString(pool, FIB->fib_Comment);
                fi->protection = FIB->fib_Protection;
            }
            UnLock(nLock);
        }
        FreeDosObject (DOS_FIB,(APTR) FIB);
    }
    return info;
}
///

///GetFileInfo()
LONG GetFileInfo(char *name) 
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
LONG GetProtectionInfo(char *name) 
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
char *GetCommentInfo(APTR pool, char *name) 
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
BOOL  deleteFile(char *file) {
    DeleteFile(file);
    return TRUE;
}
///

///copyFile()
BOOL  copyFile(APTR pool, char *file, char *destpath, struct FileInfoBlock *fileinfo, struct Hook *displayHook, struct dCopyStruct *display) {
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
                        if (fileinfo == NULL) FreeDosObject (DOS_FIB,(APTR) fib);
                    }
                }
                Close(in);
            }
            FreeVecPooled(pool, buffer);
        }
        freeString(pool, to);
    }
    return quit;
}
///

///actionDir()
BOOL  actionDir(APTR pool, ULONG flags, char *source, char *dest, BOOL quit, UWORD delmode, UWORD protectmode, UWORD overwritemode, struct Hook *dHook, struct Hook *delHook, APTR userdata) 
{
    struct  FileInfoBlock  *FIB, *FIB2;
    struct  dCopyStruct    display;
    struct      dCopyStruct        delDisplay;
    struct  FileEntry    *fe, *fef, *fel;

    BPTR       NewLock, cDir, nDir, nLock;
    WORD       dmode, pmode, omode, dm, pm, om;
    ULONG      Success, Success1, Success2, DosError, len;
    char       *dname, *comment, *dpath;
    BOOL       del, created, unprotect, failure;
#if 0 /* unused */
    BOOL       stop;
#endif
    BOOL       overwrite;
    LONG       info, prot;

    if (quit) return TRUE;

    created = FALSE;

    display.userdata = userdata;
    delDisplay.userdata = userdata;

    dmode = delmode;
    omode = overwritemode;
    pmode = protectmode;

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
#if 0 /* unused */
                    stop = quit;
#endif
                    do 
                    {
                        Success=ExNext(NewLock,FIB);

                        if ((flags & (ACTION_DELETE | ACTION_COPY)) == ACTION_DELETE) 
                        {
                            if (dmode == OPMODE_NONE) Success = FALSE;
                        }

                        if (Success && Success1) 
                        {
                            if (FIB->fib_DirEntryType>0) 
                            {
                                
                                dname = NULL;

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

                                if (delHook && (dmode != OPMODE_NONE) && ((flags & ACTION_DELETE) != 0))
                                {
                                    if ((dmode == OPMODE_ASK) || (dmode == OPMODE_DELETE) || (dmode == OPMODE_ALL) || (dmode == OPMODE_NO))
                                    {
                                        delDisplay.spath = FIB->fib_FileName;
                                        delDisplay.file = NULL;
                                        delDisplay.type = 0;
                                        delDisplay.filelen = FIB->fib_Size;
                                        if (dmode != OPMODE_ALL) dmode = CallHook(delHook, (Object *) &delDisplay, NULL);
                                        if ((dmode == OPMODE_ALL) || (dmode == OPMODE_DELETE))
                                        {

                                            unprotect = FALSE;
                                            info = GetFileInfo(FIB->fib_FileName);
                                            if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0) 
                                            {
                                                if (pmode != OPMODE_NONE)
                                                {
                                                    if (
                                                        (pmode == OPMODE_ASK) || (pmode == OPMODE_DELETE) ||
                                                        (pmode == OPMODE_ALL)  || (pmode == OPMODE_NO)
                                                    ) 
                                                    {
                                                        delDisplay.spath = FIB->fib_FileName;
                                                        delDisplay.file = NULL;
                                                        delDisplay.type = 1;
                                                        delDisplay.filelen = 0;
                                                        if (pmode != OPMODE_ALL) pmode = CallHook(delHook, (Object *) &delDisplay, NULL);
                                                        if ((pmode == OPMODE_ALL) || (pmode == OPMODE_DELETE))
                                                        {
                                                            SetProtection(FIB->fib_FileName, 0);
                                                            unprotect = TRUE;
                                                        }
                                                    }
                                                }
                                            } 
                                            else unprotect = TRUE;
                                            
                                            if (unprotect) 
                                            {
                                                del = TRUE;
                                            }
                                        }
                                    }
                                }

                                dm = dmode;
                                om = omode;
                                pm = pmode;

                                if (om == OPMODE_NO) om = OPMODE_NONE;
                                if (om == OPMODE_DELETE) om = OPMODE_ALL;

                                if (pm == OPMODE_NO) pm = OPMODE_NONE;
                                if (pm == OPMODE_DELETE) pm = OPMODE_ALL;

                                if (dm == OPMODE_NO) dm = OPMODE_NONE;
                                if (dm == OPMODE_DELETE) dm = OPMODE_ALL;

                                if (created || ((flags & ACTION_DELETE) !=0)) 
                                {
                                    if (((dmode == OPMODE_NO) || (dmode == OPMODE_NONE)) && (flags == ACTION_DELETE))
                                    {
                                        quit = FALSE;
                                    } 
                                    else 
                                    {
                                        quit = actionDir(pool, flags, FIB->fib_FileName, dname, quit, dm, pm, om, dHook, delHook, userdata);
                                    }
                                }

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
                                if (dname) freeString(pool, dname);
                            } 
                            else 
                            {
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

                                if (((flags & ACTION_COPY) != 0) && dest)
                                {
                                    dpath = combinePath(pool, dest, FIB->fib_FileName);
                                    if (dpath) 
                                    {
                                        info = GetFileInfo(dpath);
                                        if (info != -1) 
                                        {
                                            overwrite = FALSE;
                                            if (delHook && (omode != OPMODE_NONE))
                                            {
                                                if (
                                                    (omode == OPMODE_ASK) || (omode == OPMODE_DELETE) ||
                                                    (omode == OPMODE_ALL) || (omode == OPMODE_NO)
                                                ) 
                                                {
                                                    delDisplay.spath = dest;
                                                    delDisplay.file = FIB->fib_FileName;
                                                    delDisplay.type = 2;
                                                    delDisplay.filelen = 0;
                                                    if (omode != OPMODE_ALL) omode = CallHook(delHook, (Object *) &delDisplay, NULL);
                                                    if ((omode == OPMODE_ALL) || (omode == OPMODE_DELETE))
                                                    {
                                                        if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) !=0)
                                                        {
                                                            if (pmode != OPMODE_NONE)
                                                            {
                                                                if (
                                                                    (pmode == OPMODE_ASK) || (pmode == OPMODE_DELETE) ||
                                                                    (pmode == OPMODE_ALL)  || (pmode == OPMODE_NO)
                                                                ) 
                                                                {
                                                                    delDisplay.spath = dest;
                                                                    delDisplay.file = FIB->fib_FileName;
                                                                    delDisplay.type = 1;
                                                                    delDisplay.filelen = 0;
                                                                    if (pmode != OPMODE_ALL) pmode = CallHook(delHook, (Object *) &delDisplay, NULL);
                                                                    if ((pmode == OPMODE_ALL) || (pmode == OPMODE_DELETE))
                                                                    {
                                                                        overwrite = TRUE;
                                                                        SetProtection(dpath, 0);
                                                                    }
                                                                }
                                                            }
                                                        } 
                                                        else overwrite = TRUE;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (dpath) freeString(pool, dpath);
                                }

                                failure = FALSE;
                                if (!quit && ((flags & ACTION_COPY) !=0) && overwrite)
                                {
                                    display.flags = (flags &= ~ACTION_UPDATE);
                                    failure = copyFile(pool, FIB->fib_FileName, dest, FIB, dHook, &display);
                                }

                                if (failure && !quit) 
                                {
                                    if (delHook) 
                                    {
                                        delDisplay.spath = source;
                                        delDisplay.file = FIB->fib_FileName;
                                        delDisplay.type = 3;
                                        delDisplay.filelen = 0;
                                        if (CallHook(delHook, (Object *) &delDisplay, NULL) == ACCESS_SKIP) 
                                            quit = FALSE; else quit = TRUE;
                                    } 
                                    else quit = FALSE;
                                }

                                if (!quit && delHook && (dmode != OPMODE_NONE) && ((flags & ACTION_DELETE) !=0))
                                {
                                    if (
                                        (dmode == OPMODE_ASK) || (dmode == OPMODE_DELETE) ||
                                        (dmode == OPMODE_ALL) || (dmode == OPMODE_NO)
                                    ) 
                                    {
                                        delDisplay.spath = source;
                                        delDisplay.file = FIB->fib_FileName;
                                        delDisplay.type = 0;
                                        delDisplay.filelen = FIB->fib_Size;
                                        if (dmode != OPMODE_ALL) dmode = CallHook(delHook, (Object *) &delDisplay, NULL);
                                        if ((dmode == OPMODE_ALL) || (dmode == OPMODE_DELETE))
                                        {

                                            info = GetFileInfo(FIB->fib_FileName);
                                            unprotect = FALSE;
                                            if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0)
                                            {
                                                if (pmode != OPMODE_NONE)
                                                {
                                                    if (
                                                        (pmode == OPMODE_ASK) || (pmode == OPMODE_DELETE) ||
                                                        (pmode == OPMODE_ALL)  || (pmode == OPMODE_NO)
                                                    ) 
                                                    {
                                                        delDisplay.spath = source;
                                                        delDisplay.file = FIB->fib_FileName;
                                                        delDisplay.type = 1;
                                                        delDisplay.filelen = 0;
                                                        if (pmode != OPMODE_ALL) pmode = CallHook(delHook, (Object *) &delDisplay, NULL);
                                                        if ((pmode == OPMODE_ALL) || (pmode == OPMODE_DELETE))
                                                        {
                                                            unprotect = TRUE;
                                                            SetProtection(FIB->fib_FileName, 0);
                                                        }
                                                    }
                                                }
                                            } 
                                            else unprotect = TRUE;
                                            
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
                            if (DosError!=ERROR_NO_MORE_ENTRIES) Success=TRUE;
                        }
                    } 
                    while (Success && !quit);
                }

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

///CopyContent()
BOOL CopyContent(APTR p, char *s, char *d, BOOL makeparentdir, ULONG flags, struct Hook *displayHook, struct OpModes *opModes, APTR userdata) 
{

    struct  FileInfoBlock  *FIB;
    struct  dCopyStruct    display;
    struct  dCopyStruct    delDisplay;
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
    UWORD      pmode = OPMODE_ASK;
    UWORD      omode = OPMODE_ASK;

    if (p == NULL) 
    {
        pool = CreatePool(MEMF_CLEAR|MEMF_ANY, POOLSIZE, POOLSIZE);
    } 
    else pool = p;

    if (pool == NULL) return FALSE;

    infoname = AllocVecPooled(pool, strlen(s)+6);
    display.userdata = userdata;
    delDisplay.userdata = userdata;
    
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
    if (opModes && ((flags & ACTION_DELETE) != 0) && ((makeparentdir && dir) || !dir))
    {
        if (dir) 
        {
            delDisplay.spath = s;
            delDisplay.file = NULL;
        } 
        else 
        {
            path = allocPath(pool, s);
            delDisplay.spath = path;
            delDisplay.file = FilePart(s);
        }
        delDisplay.type = 0;

        if (opModes->deletemode != OPMODE_ALL) opModes->deletemode = CallHook(opModes->askhook, (Object *) &delDisplay, NULL);
        if ((opModes->deletemode == OPMODE_ALL) || (opModes->deletemode == OPMODE_DELETE))
        {
            deletesrc = TRUE;
            if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0)
            {
                delDisplay.type = 1;
                unprotectsrc = FALSE;
                pmode = CallHook(opModes->askhook, (Object *) &delDisplay, NULL);
                if ((pmode == OPMODE_ALL) || (pmode == OPMODE_DELETE))
                {
                    SetProtection(s, 0);
                    if (infoname) SetProtection(infoname, 0);
                    unprotectsrc = TRUE;
                }
            }
        }
    }

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
                    if (opModes && (omode != OPMODE_NONE))
                    {
                        if (
                            (omode == OPMODE_ASK) || (omode == OPMODE_DELETE) ||
                            (omode == OPMODE_ALL) || (omode == OPMODE_NO)
                        ) 
                        {
                            delDisplay.spath = d;
                            delDisplay.file = FilePart(s);
                            delDisplay.type = 2;
                            if (omode != OPMODE_ALL) omode = CallHook(opModes->askhook, (Object *) &delDisplay, NULL);
                            if ((omode == OPMODE_ALL) || (omode == OPMODE_DELETE))
                            {
                                if (((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0) && (pmode != OPMODE_NONE))
                                {
                                    if (
                                        (pmode == OPMODE_ASK) || (pmode == OPMODE_DELETE) ||
                                        (pmode == OPMODE_ALL) || (pmode == OPMODE_NO)
                                    ) 
                                    {
                                        delDisplay.spath = d;
                                        delDisplay.file = FilePart(s);
                                        delDisplay.type = 1;
                                        if (pmode != OPMODE_ALL) pmode = CallHook(opModes->askhook, (Object *) &delDisplay, NULL);
                                        if ((pmode == OPMODE_ALL) || (pmode == OPMODE_DELETE))
                                        {
                                            SetProtection(dpath, 0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            freeString(pool, dpath);
        }
        freeString(pool, path);
    }

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
                back = actionDir(
                    pool, flags, s, dest, FALSE, opModes->deletemode, pmode, omode, displayHook, opModes->askhook, userdata
                );
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
            if (path) freeString(pool, path);
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
