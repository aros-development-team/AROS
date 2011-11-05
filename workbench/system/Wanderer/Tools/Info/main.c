/*
    Copyright © 2003-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/icon.h>

#include <dos/dos.h>
#include <dos/datetime.h>
#include <exec/memory.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
#include <libraries/mui.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <devices/inputevent.h>

#include <mui/TextEditor_mcc.h>
#include <zune/iconimage.h>

#include "locale.h"

#include <stdio.h>

#include <string.h>

#define RETURNID_SLEEP      1
#define RETURNID_WAKE       2
#define RETURNID_ABOUT      3
#define RETURNID_NEWKEY     4
#define RETURNID_DELKEY     5
#define RETURNID_LVACK      6
#define RETURNID_STRINGACK  7
#define RETURNID_SAVE       8
#define RETURNID_TOOLACK    9
#define RETURNID_STACKACK   10
#define RETURNID_COMMENTACK 11
#define RETURNID_QUERYVERSION    12
#define RETURNID_PROTECT    13
#define RETURNID_SCANSIZE   14

#define  MAX_PATH_LEN  1024
#define  MAX_TOOLTYPE_LINE 256
#define BUFFERSIZE 1024

#define USE_TEXTEDITOR 1

#define SCANDIE 1
#define SCANRUN 0

static Object *window, *commentspace, *filename_string, *stackspace, *savebutton;
static Object *readobject, *writeobject, *executeobject, *deleteobject;
static Object *scriptobject, *pureobject, *archiveobject, *sizespace = NULL;

struct DirScanProcess
{
    struct Process    *scanProcess;
    Object         *scanSizeObj;
    char        *scanSize;
    char        *scanDir;
    IPTR        scanState;
};

static struct DirScanProcess *scanStruct = NULL;

#if USE_TEXTEDITOR
static Object *editor, *slider;
#else
static Object *list, *editor, *liststr;
#endif

BOOL file_altered = FALSE;
BOOL icon_altered = FALSE;

/* TODO: Use UQUAD for size */
void getReadableSize(UBYTE *buf, ULONG size)
{
    UQUAD d;
    char *ch;
    struct
    {
    IPTR val;
    IPTR  dec;
    } array =
    {
    size,
    0
    };

    /*
    if (size >= (1024 * 1024 * 1024 * 1024 * 1024 * 1024))
    {
    //Exabytes
    array.val = size >> 60;
    d = ((UQUAD)size * 10 + ((1024 * 1024 * 1024 * 1024 * 1024 * 1024) / 2)) / (1024 * 1024 * 1024 * 1024 * 1024 * 1024);
    array.dec = d % 10;
    ch = "EiB";
    }
    if (size >= (1024 * 1024 * 1024 * 1024 * 1024))
    {
    //Petabytes
    array.val = size >> 50;
    d = ((UQUAD)size * 10 + ((1024 * 1024 * 1024 * 1024 * 1024) / 2)) / (1024 * 1024 * 1024 * 1024 * 1024);
    array.dec = d % 10;
    ch = "PiB";
    }
    if (size >= (1024 * 1024 * 1024 * 1024))
    {
    //Terabytes
    array.val = size >> 40;
    d = ((UQUAD)size * 10 + ((1024 * 1024 * 1024 * 1024) / 2)) / (1024 * 1024 * 1024 * 1024);
    array.dec = d % 10;
    ch = "TiB";
    }*/
    if (size >= (1024 * 1024 * 1024))
    {
    //Gigabytes
    array.val = size >> 30;
    d = ((UQUAD)size * 10 + ((1024 * 1024 * 1024) / 2)) / (1024 * 1024 * 1024);
    array.dec = d % 10;
    ch = "GiB";
    }
    else if (size >= (1024 * 1024))
    {
    //Megabytes
    array.val = size >> 20;
    d = ((UQUAD)size * 10 + ((1024 * 1024) / 2)) / (1024 * 1024);
    array.dec = d % 10;
    ch = "MiB";
    }
    else if (size >= 1024)
    {
    //Kilobytes
    array.val = size >> 10;
    d = (size * 10 + (1024 / 2)) / 1024;
    array.dec = d % 10;
    ch = "KiB";
    }
    else
    {
    //Bytes
    array.val = size;
    array.dec = 0;
    d = 0;
    if (size == 1)
        ch = "Byte";
    else
        ch = "Bytes";
    }

    if (!array.dec && (d > array.val * 10))
    {
    array.val++;
    }

    RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);

    while (*buf)
    {
    buf++;
    }

    sprintf((char *)buf," %s", ch);
}

#define kExallBufSize          (4096)

ULONG calculateDirectorySize(struct DirScanProcess *scan, ULONG base, CONST_STRPTR directory)
{
    UBYTE    *buffer = NULL;
    BPTR    directoryLock = BNULL;
    ULONG    directorySize = 0;
    BOOL    loop = TRUE;

D(bug("[WBInfo] calculateDirectorySize('%s')\n", directory));

    directoryLock = Lock(directory, SHARED_LOCK);

    /* Allocate buffer for ExAll */
    if ((buffer = AllocVec(kExallBufSize, MEMF_CLEAR|MEMF_PUBLIC)) == NULL)
    {
        UnLock(directoryLock);
        return 0;
    }

    struct ExAllData *ead = (struct ExAllData*)buffer;
    struct ExAllControl  *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    eac->eac_LastKey = 0;

    struct ExAllData *oldEad = ead;

    do
    {
    ead = oldEad;
    loop = ExAll(directoryLock, ead, kExallBufSize, ED_COMMENT, eac);

    if(!loop && IoErr() != ERROR_NO_MORE_ENTRIES) break;

    if(eac->eac_Entries != 0)
    {
        do
        {
                if (ead->ed_Type == ST_FILE)
                {
                    directorySize += ead->ed_Size;
            getReadableSize(scan->scanSize, (base + directorySize));
            set(sizespace, MUIA_Text_Contents, (IPTR) scan->scanSize);
                }
                else if (ead->ed_Type == ST_USERDIR)
                {
            ULONG subdirlen = strlen(directory) + strlen(ead->ed_Name) + 1;
                    char * subdirectory = AllocVec(subdirlen + 1, MEMF_CLEAR);
            CopyMem(directory, subdirectory, strlen(directory));
            AddPart(subdirectory, ead->ed_Name, subdirlen + 1);
                    directorySize += calculateDirectorySize(scan, (base + directorySize), subdirectory);
                }
        ead = ead->ed_Next;
        } while((ead != NULL) && (scan->scanState == SCANRUN)); 
    }
    } while((loop) && (scan->scanState == SCANRUN)); 

    FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(directoryLock);
    FreeVec(buffer);

    return directorySize;
}

/*
 * child process to calculate directory content size..
 */
AROS_UFH3(void, scanDir_Process,
    AROS_UFHA(STRPTR,              argPtr, A0),
    AROS_UFHA(ULONG,               argSize, D0),
    AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Task            *taskSelf = FindTask(NULL);
    struct DirScanProcess    *scan = taskSelf->tc_UserData;
    ULONG            directorySize = 0;

    if (scan->scanState == SCANRUN)
    {
    D(bug("[WBInfo] scanDir_Process('%s')\n", scan->scanDir));
    scan->scanSize = AllocVec(64, MEMF_CLEAR);
    directorySize = calculateDirectorySize(scan, directorySize, scan->scanDir);
    D(bug("[WBInfo] scanDir_Process: End size = %d bytes\n", directorySize));
    getReadableSize(scan->scanSize, directorySize);
    set(sizespace, MUIA_Text_Contents, (IPTR) scan->scanSize);
    FreeVec(scan->scanSize);
    }
    scan->scanProcess = NULL;
    
    AROS_USERFUNC_EXIT
}

UBYTE **BuildToolTypes(UBYTE **src_ttypes)
{
#if USE_TEXTEDITOR
    APTR     pool = CreatePool(MEMF_CLEAR, 200, 200);
    STRPTR   contents, sp, *dst_ttypes;
    ULONG    lines = 1;

    if (!pool) return NULL;
    contents = (STRPTR)DoMethod(editor, MUIM_TextEditor_ExportText);

    if (!contents || !contents[0])
    {
        DeletePool(pool);
    return NULL;
    }
    
    sp = contents;
    
    while((sp = strchr(sp, '\n')))
    {
        sp++;
    lines++;
    }
    
    dst_ttypes = AllocPooled(pool, (lines + 3) * sizeof(STRPTR));
    if (!dst_ttypes)
    {
        FreeVec(contents);
    DeletePool(pool);
    return NULL;
    }
    
    *dst_ttypes++ = (STRPTR)pool;
    *dst_ttypes++ = (STRPTR)contents;
    
    for(sp = contents, lines = 0; sp; lines++)
    {
    dst_ttypes[lines] = sp;
        sp = strchr(sp, '\n');
    if (sp)
    {
        *sp++ = '\0';
    }
    }
    dst_ttypes[lines] = 0;
     
    return dst_ttypes;   
#else
    APTR     pool = CreatePool(MEMF_CLEAR, 200, 200);
    Object  *listobj = list;
    WORD     list_index = 0, num_ttypes = 0, alloc_ttypes = 10;
    UBYTE **dst_ttypes;
    
    if (!pool) return NULL;
    
    dst_ttypes = AllocPooled(pool, (alloc_ttypes + 2) * sizeof(UBYTE *));
    if (!dst_ttypes)
    {
        DeletePool(pool);
        return NULL;
    }

    get(listobj, MUIA_List_Entries, &list_index);
    while(num_ttypes<list_index)
    {
        STRPTR text = NULL;
        DoMethod(listobj, MUIM_List_GetEntry, num_ttypes, (IPTR)&text);
        if (num_ttypes >= alloc_ttypes)
        {
            UBYTE **new_dst_ttypes = AllocPooled(pool, (alloc_ttypes + 10 + 2) * sizeof(UBYTE *));
            if (!new_dst_ttypes)
            {
                DeletePool(pool);
                return NULL;
            }
            CopyMem(dst_ttypes + 1, new_dst_ttypes + 1, alloc_ttypes * sizeof(UBYTE *));
            dst_ttypes = new_dst_ttypes;
            alloc_ttypes += 10;
        }
        dst_ttypes[num_ttypes + 1] = AllocPooled(pool, strlen(text) + 1);
        if (!dst_ttypes[num_ttypes + 1])
        {
            DeletePool(pool);
            return NULL;
        }
        CopyMem(text, dst_ttypes[num_ttypes + 1], strlen(text) + 1);
D(bug("[WBInfo] tooltype #%ld %s\n", num_ttypes + 1, text));
        num_ttypes++;
    }
    dst_ttypes[0] = (APTR)pool;
    return dst_ttypes + 1;
#endif    
}

void FreeToolTypes(UBYTE **ttypes)
{
#if USE_TEXTEDITOR
    if (ttypes)
    {
    FreeVec((APTR)ttypes[-1]);
        DeletePool((APTR)ttypes[-2]);
    }
#else
    if (ttypes)
    {
        DeletePool((APTR)ttypes[-1]);
    }
#endif
}

void SaveIcon(struct DiskObject *icon, STRPTR name, BPTR cd)
{
    STRPTR tool = NULL, stack = NULL;
    BPTR restored_cd;
    long int ls;
    UBYTE **ttypes = NULL, **old_ttypes = NULL;

    restored_cd = CurrentDir(cd);

    switch(icon->do_Type)
    {
        case WBPROJECT:
            get(filename_string, MUIA_String_Contents, &tool);
            icon->do_DefaultTool = tool;
            get(stackspace, MUIA_String_Contents, &stack);
            stcd_l(stack, &ls);
            icon->do_StackSize = (LONG)ls;
            break;
        case WBTOOL:
            get(stackspace, MUIA_String_Contents, &stack);
            stcd_l(stack, &ls);
            icon->do_StackSize = (LONG)ls;
            break;
    }

    old_ttypes = (UBYTE **)icon->do_ToolTypes;
    ttypes = BuildToolTypes(old_ttypes);
    icon->do_ToolTypes = ttypes;

    PutIconTags(name, icon, TAG_DONE);
    
    icon->do_ToolTypes = old_ttypes;
    if (ttypes)
    {
        FreeToolTypes(ttypes);
    }
    CurrentDir(restored_cd);
    icon_altered = FALSE;
}

void SaveFile(struct AnchorPath * ap, BPTR cd)
{
    ULONG protection = 0;
    STRPTR text=NULL;
    BPTR restored_cd;
    restored_cd = CurrentDir(cd);

    get(commentspace, MUIA_String_Contents, &text);
    if (text)
        SetComment(ap->ap_Buf, text);

    if (XGET(scriptobject, MUIA_Selected))
        protection |= FIBF_SCRIPT;
    if (XGET(pureobject, MUIA_Selected))
        protection |= FIBF_PURE;
    if (XGET(archiveobject, MUIA_Selected))
        protection |= FIBF_ARCHIVE;

    if (! XGET(readobject, MUIA_Selected))
        protection |= FIBF_READ;
    if (! XGET(writeobject, MUIA_Selected))
        protection |= FIBF_WRITE;
    if (! XGET(executeobject, MUIA_Selected))
        protection |= FIBF_EXECUTE;
    if (! XGET(deleteobject, MUIA_Selected))
        protection |= FIBF_DELETE;
    
    SetProtection(ap->ap_Buf, protection);

    CurrentDir(restored_cd);
    file_altered = FALSE;
}

#if !USE_TEXTEDITOR
void ListToString(void)
{
    STRPTR text = NULL;
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR) &text);
    if (text)
    {
        set(liststr, MUIA_String_Contents, text);
        nnset(liststr, MUIA_Disabled, FALSE);
        set(window, MUIA_Window_ActiveObject, (IPTR)liststr);
    }
}

void StringToKey(void)
{
    STRPTR  text = NULL;
    STRPTR  input = NULL;

    icon_altered = TRUE;

    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR) &text);
    if (text)
    {
        get(liststr, MUIA_String_Contents, &input);
            DoMethod(list, MUIM_List_InsertSingle, input, MUIV_List_Insert_Active);
            set(list, MUIA_List_Active, MUIV_List_Active_Down);
            DoMethod(list, MUIM_List_Remove, MUIV_List_Remove_Active);
            nnset(liststr, MUIA_String_Contents, (IPTR) "");
            nnset(liststr, MUIA_Disabled, TRUE);
            set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
    }
}

void NewKey(void)
{
    nnset(liststr, MUIA_String_Contents, "");
    nnset(liststr, MUIA_Disabled, FALSE);
    DoMethod(list, MUIM_List_InsertSingle, "", MUIV_List_Insert_Bottom);
    nnset(list, MUIA_List_Active, MUIV_List_Active_Bottom);
    set(window, MUIA_Window_ActiveObject, (IPTR) liststr);
}

void DelKey(void)
{
    STRPTR  text = NULL;
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&text);

    if (text) 
    {
        DoMethod(list, MUIM_List_Remove, MUIV_List_Remove_Active);
        DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&text);
        if (!text) 
        {
            nnset(liststr, MUIA_String_Contents, (IPTR) "");
            nnset(liststr, MUIA_Disabled, TRUE);
            ListToString();
        } else { 
            ListToString();
        }
    }
}
#endif

static char * GetVersion(STRPTR name, BPTR cd)
{
D(bug("[WBINFO/Getversion] Name %s\n", name));
    STRPTR commandline = NULL;
    ULONG commandlineSize;
    static TEXT result[100]; // must be static because we return its address
    char *retval = result;
    TEXT tmpfilename[40];
    int nr = 0;
    int i;
    BPTR output = BNULL;
    BPTR restored_cd = (BPTR)-1;

    if (name == NULL)
    {
D(bug("[WBINFO/Getversion] Name is Null\n"));
        goto exit;
    }

    memset(result, 0, sizeof(result));
    result[0] = '?';

    commandlineSize = strlen(name) + 20;
    commandline = AllocVec(commandlineSize, MEMF_CLEAR);
    if (commandline == NULL)
    {
D(bug("[WBInfo/GetVersion] Can't allocate RAM for commandline\n"));
        goto exit;
    }

    restored_cd = CurrentDir(cd);

    //search for non-existing temp file name
    do
    {
        nr++;
        if (nr > 30)
        {
D(bug("[WBINFO/Getversion] Can't find non-existing tmpfile"));
            goto exit;
        }
        sprintf(tmpfilename, "t:tmp_version_%d", nr);
        output = Open(tmpfilename, MODE_OLDFILE);
        if (output)
        {
            Close(output);
        }
    } while (output != BNULL);

    D(bug("[WBInfo/Getversion] tmpfilename %s\n", tmpfilename));
    output = Open(tmpfilename, MODE_NEWFILE);
    if (output == BNULL)
    {
D(bug("[WBInfo/Getversion] Can't create tmpfile\n"));
        goto exit;
    }

    // call c:version
    sprintf(commandline, "c:version \"%s\" full", name);
D(bug("[WBInfo/GetVersion] Commandline %s\n", commandline));
    if (SystemTags(commandline,
                SYS_Asynch,          FALSE,
                SYS_Output,   (IPTR) output,
                //SYS_Error,    (IPTR) NULL,
                NP_StackSize,        16000,
                TAG_DONE) != 0)
    {
D(bug("[WBInfo/Getversion] SystemTags failed\n"));
        goto exit;
    }

    // Seek didn't work for me on RAM-Disk, so we reopen the file
    Close(output);
    output = Open(tmpfilename, MODE_OLDFILE);
    if (output == BNULL)
    {
D(bug("[WBInfo/GetVersion] Can't open tmpfile\n"));
        goto exit;
    }
    // read result
    if (Read(output, result, sizeof(result) - 1) == -1)
    {
D(bug("[WBInfo/GetVersion] Can't read from tmpfile\n"));
        goto exit;
    }
D(bug("[WBInfo/GetVersion] Result %s\n", result));

    // remove illegal chars (e.g. newline) from result
    for (i = 0 ; result[i] != 0 ; i++)
    {
        if (result[i] < 32)
            result[i] = ' ';
    }

    // set string pointer after program name
    while (*retval != ' ')
        retval++;

exit:
    FreeVec(commandline);
    if (output)
    {
        Close(output);
        DeleteFile(tmpfilename);
    }
    if (restored_cd != (BPTR)-1)
        CurrentDir(restored_cd);

    return retval;
}

static void AddStackField(Object * application, Object * group, STRPTR stack)
{
    Object * stacklabel = NULL;

    stackspace = (Object *)StringObject,
        StringFrame,
        MUIA_String_MaxLen, 16,
        MUIA_String_Contents, (IPTR)stack,
        MUIA_String_Format, MUIV_String_Format_Right,
        MUIA_String_Accept, (IPTR)"0123456789",
        End;

    stacklabel = MUI_MakeObject(MUIO_Label, _(MSG_STACK), NULL);

    if ((stackspace != NULL) && (stacklabel != NULL))
    {
        DoMethod(group, MUIM_Group_InitChange);
        DoMethod(group, OM_ADDMEMBER, stacklabel);
        DoMethod(group, OM_ADDMEMBER, stackspace);
        DoMethod(group, MUIM_Group_ExitChange);
        SetAttrs(stackspace, MUIA_CycleChain, 1, TAG_DONE);
        DoMethod(stackspace, MUIM_Notify,
        MUIA_String_Acknowledge, MUIV_EveryTime,
        (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_STACKACK);
    }
}

static void AddToolField(Object * application, Object * group, STRPTR tool)
{
    Object * toolspace = MUI_NewObject(MUIC_Popasl, ASLFR_DoSaveMode, TRUE,
        MUIA_Popstring_String, (IPTR)(filename_string = (Object *)StringObject,
        StringFrame,
        MUIA_String_MaxLen, MAX_PATH_LEN,
        MUIA_String_Contents, (IPTR)tool,
        End),
        MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopFile), TAG_DONE);

    Object * toollabel = MUI_MakeObject(MUIO_Label, _(MSG_DEFTOOL), NULL);

    if ((toolspace != NULL) && (toollabel != NULL)&&(filename_string != NULL))
    {
        DoMethod(group, MUIM_Group_InitChange);
        DoMethod(group, OM_ADDMEMBER, toollabel);
        DoMethod(group, OM_ADDMEMBER, toolspace);
        DoMethod(group, MUIM_Group_ExitChange);

        set(toolspace, MUIA_CycleChain, 1);
        DoMethod(filename_string, MUIM_Notify,
        MUIA_String_Acknowledge, MUIV_EveryTime,
        (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_TOOLACK);
    }
}

static void AddEmptySpace(Object * group)
{
    Object * emptyspace = MUI_NewObject(MUIC_Rectangle, TAG_DONE);
    if ((emptyspace != NULL))
    {
        DoMethod(group, MUIM_Group_InitChange);
        DoMethod(group, OM_ADDMEMBER, emptyspace);
        DoMethod(group, MUIM_Group_ExitChange);
    }
}

int main(int argc, char **argv)
{
    Object *application, *grouptool, *groupstack = NULL;
    Object *registergroup = NULL, *infomenu = NULL, *protectmenu = NULL, *toolmenu = NULL;
    Object *datespace = NULL, *typespace = NULL, *quit = NULL, *aboutmenu = NULL;
    Object *sizegrp = NULL, *versiongrp = NULL, *versionspace = NULL;
    Object *cancelbutton = NULL;
#if !USE_TEXTEDITOR
    Object *newkey = NULL, *delkey = NULL;
#endif
    struct WBStartup *startup;
    struct DiskObject *icon = NULL;
    struct AnchorPath *ap = NULL;
    struct DateStamp *ds = NULL;
    struct DateTime dt;
#if 0 /* unused */
    IPTR dte;
#endif
    STRPTR name = NULL, file = NULL, type = NULL;
    BPTR cd, lock;
    LONG  returnid = 0;
    ULONG protection;
    char stack[16];
    char deftool[MAX_PATH_LEN];
    char comment[MAXCOMMENTLENGTH];
    char size[64];
    char date[LEN_DATSTRING];
    char time[LEN_DATSTRING];
    char  dow[LEN_DATSTRING];
    char datetime[2*LEN_DATSTRING];
    UBYTE flags[8] = {0}, lname[MAXFILENAMELENGTH];

    char *pages[] = {_(MSG_INFORMATION),_(MSG_PROTECTION),_(MSG_TOOLTYPES),NULL};
    char * typeNames[] =
    {
         NULL,      /* not used */
        _(MSG_DISK),    /* 1 */
        _(MSG_DRAWER),  /* 2 */
        _(MSG_TOOL),    /* 3 */
        _(MSG_PROJECT), /* 4 */
        _(MSG_GARBAGE), /* 5 */
        _(MSG_DEVICE),  /* 6 */
        _(MSG_KICK),    /* 7 */
        _(MSG_APPICON)  /* 8 */
    };

    int retval = RETURN_OK;
    
    if (argc != 0)
    {
        /* start from wanderer only */
        PrintFault(ERROR_FILE_NOT_OBJECT, argv[0]);
        retval = RETURN_FAIL;
    goto funcmain_exit;
    }

    startup = (struct WBStartup *) argv;

    if (startup->sm_NumArgs < 2)
    {
        /* need atleast 1 arg */
        PrintFault(ERROR_REQUIRED_ARG_MISSING, argv[0]);
D(bug("[WBInfo] required arg missing\n"));
        retval = RETURN_FAIL;
    goto funcmain_exit;
    }

    lock = startup->sm_ArgList[1].wa_Lock;
    NameFromLock(lock, lname, MAXFILENAMELENGTH);
D(bug("[WBInfo] arg parent lock 0x%p: '%s'\n", lock, lname));

    if (startup->sm_ArgList[1].wa_Name != NULL)
    {
    if ((name = AllocVec(strlen(startup->sm_ArgList[1].wa_Name) + 1, MEMF_CLEAR)) != NULL)
    {
        CopyMem(startup->sm_ArgList[1].wa_Name, name, strlen(startup->sm_ArgList[1].wa_Name));
        if ((strlen(name) > 5)
        && (strcmp(name + strlen(name) - 5, ".info") == 0))
        {
        file = AllocVec(strlen(name) - 4, MEMF_CLEAR);
        CopyMem(name, file , strlen(name) - 5);
        }
        else
        {
        file = AllocVec(strlen(name) + 1, MEMF_CLEAR);
        CopyMem(name, file, strlen(name));
        }
    D(bug("[WBInfo] arg name 0x%p: '%s', file = '%s'\n", name, name, file));
    }
    }
    cd = CurrentDir(lock);
    if (name == NULL)
    {
        /* directory not found*/
        PrintFault(ERROR_DIR_NOT_FOUND, argv[0]);
D(bug("[WBInfo] dir not found\n"));
        retval = RETURN_FAIL;
    goto funcmain_exit;
    };

    ap = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN, MEMF_CLEAR);
    if (ap == NULL)
    {
        PrintFault(ERROR_NO_FREE_STORE, argv[0]);
D(bug("[WBInfo] no free store\n"));
        retval = RETURN_FAIL;
    goto funcmain_exit;
    }

    ap->ap_Strlen = MAX_PATH_LEN;

    if (0 != MatchFirst(name, ap))
    {
D(bug("[WBInfo] pass to diskinfo\n"));
        OpenWorkbenchObject(
            "WANDERER:Tools/DiskInfo",
            WBOPENA_ArgLock, (IPTR) startup->sm_ArgList[1].wa_Lock,
            WBOPENA_ArgName, (IPTR) startup->sm_ArgList[1].wa_Name,
            TAG_DONE);

        retval = RETURN_OK;
    goto funcmain_exit;
    };

    ap->ap_BreakBits = SIGBREAKF_CTRL_C;

    if (!(ap->ap_Flags & APF_DIDDIR))
    {
D(bug("[WBInfo] scan file\n"));
        /* fill comment */
        sprintf(comment,"%s",ap->ap_Info.fib_Comment);

        /* fill date and time */
        ds = &ap->ap_Info.fib_Date;
        DateStamp((struct DateStamp *)&dt);
        CopyMem(ds, &dt.dat_Stamp, sizeof(struct DateStamp));
        dt.dat_Format = FORMAT_DOS;
        dt.dat_Flags = 0;
        dt.dat_StrDay = dow;
        dt.dat_StrDate = date;
        dt.dat_StrTime = time;
#if 0 /* unused */
        dte =
#endif
               DateToStr(&dt);
        sprintf(datetime, "%s %s", time, date);

        /* fill size */
        getReadableSize(size, ap->ap_Info.fib_Size);

        /* fill protection */
        protection = ap->ap_Info.fib_Protection;

    /* Convert the protection bits to a boolean */
        flags[0] = protection & FIBF_SCRIPT  ? 1 : 0; /* s */
        flags[1] = protection & FIBF_PURE    ? 1 : 0; /* p */
        flags[2] = protection & FIBF_ARCHIVE ? 1 : 0; /* a */

    /* The following flags are high-active! */
        flags[3] = protection & FIBF_READ    ? 0 : 1; /* r */
        flags[4] = protection & FIBF_WRITE   ? 0 : 1; /* w */
        flags[5] = protection & FIBF_EXECUTE ? 0 : 1; /* e */
        flags[6] = protection & FIBF_DELETE  ? 0 : 1; /* d */
        flags[7] = 0x00;                              /* h */
    }
    
    /* read icon */
    icon = GetIconTags(file,
            ICONGETA_FailIfUnavailable, FALSE,
            ICONGETA_IsDefaultIcon, FALSE,
            TAG_DONE);

    if (icon != NULL)
    {
D(bug("[WBInfo] file has icon\n"));
        type = (char *) typeNames[icon->do_Type];
D(bug("[WBInfo] icon type is: %s\n", type));
        sprintf(stack, "%d", (int)icon->do_StackSize);
        if (icon->do_DefaultTool)
            sprintf(deftool, "%s", icon->do_DefaultTool);
        else
            *deftool = '\0';
    } else {
        PrintFault(ERROR_OBJECT_WRONG_TYPE, argv[0]);

    retval = RETURN_FAIL;
        goto funcmain_exit;
    }

    if (icon->do_Type == 2)
    {
    sizespace = (Object *)TextObject,
        ButtonFrame,
        MUIA_Background, MUII_TextBack,
        MUIA_Text_PreParse, (IPTR) "\33r",
        MUIA_Text_Contents, (IPTR) "?",
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        End;
    
    versionspace = (Object *)TextObject,
        TextFrame,
        MUIA_Background, MUII_TextBack,
        MUIA_Text_PreParse, (IPTR) "\33r",
        MUIA_Text_Contents, (IPTR) "N/A",
        End;
    }
    else
    {
    sizespace = (Object *)TextObject,
        TextFrame,
        MUIA_Background, MUII_TextBack,
        MUIA_Text_PreParse, (IPTR) "\33r",
        MUIA_Text_Contents, (IPTR) size,
        End;
    
    versionspace = (Object *)TextObject,
        ButtonFrame,
        MUIA_Background, MUII_TextBack,
        MUIA_Text_PreParse, (IPTR) "\33r",
        MUIA_Text_Contents, (IPTR) "?",
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        End;
    }

    application = (Object *)ApplicationObject,
        MUIA_Application_Title,  __(MSG_TITLE),
        MUIA_Application_Version, (IPTR) "$VER: Info 0.6 ("ADATE") © 2003-2010 The AROS Dev Team",
        MUIA_Application_Description,  __(MSG_DESCRIPTION),
        MUIA_Application_Base, (IPTR) "INFO",
        MUIA_Application_Menustrip, (IPTR) MenuitemObject,
            MUIA_Family_Child, (IPTR) MenuitemObject,
                MUIA_Menuitem_Title,  __(MSG_PROJECT),
                MUIA_Family_Child, (IPTR)(infomenu = (Object *)MenuitemObject, MUIA_Menuitem_Title,
                        __(MSG_INFORMATION), MUIA_Menuitem_Shortcut, (IPTR) "I", End),
                MUIA_Family_Child, (IPTR)(protectmenu = (Object *)MenuitemObject, MUIA_Menuitem_Title,
                        __(MSG_PROTECTION),MUIA_Menuitem_Shortcut, (IPTR) "P", End),
                MUIA_Family_Child, (IPTR)(toolmenu = (Object *)MenuitemObject, MUIA_Menuitem_Title,
                        __(MSG_TOOLTYPES),MUIA_Menuitem_Shortcut, (IPTR) "T", End),
                MUIA_Family_Child, (IPTR) MenuitemObject, MUIA_Menuitem_Title, ~0, End,
                MUIA_Family_Child, (IPTR) (aboutmenu = (Object *)MenuitemObject, MUIA_Menuitem_Title,
                    __(MSG_ABOUT), (IPTR) MUIA_Menuitem_Shortcut, (IPTR) "?", End),
                MUIA_Family_Child, (IPTR) MenuitemObject, MUIA_Menuitem_Title, ~0, End,
                MUIA_Family_Child, (IPTR) (quit = (Object *)MenuitemObject, MUIA_Menuitem_Title,
                    __(MSG_QUIT), MUIA_Menuitem_Shortcut, (IPTR) "Q", End),
                End,
            End,
        SubWindow, (IPTR) (window = (Object *)WindowObject,
            MUIA_Window_Title, (IPTR) __(MSG_TITLE),
            MUIA_Window_ID, MAKE_ID('I','N','F','O'),
            MUIA_Window_Activate, TRUE,
            WindowContents, (IPTR) VGroup,
                Child, (IPTR) HGroup,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) IconImageObject,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_IconImage_File, (IPTR) file,
                        End,
                    End,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_Text_PreParse, (IPTR) "\33l",
                            MUIA_Text_Contents, (IPTR) FilePart(file),
                        End,
                    End,
                End,
                Child, (IPTR) (registergroup = (Object *)RegisterGroup(pages),
                    MUIA_CycleChain, 1,
                    Child, (IPTR) HGroup, /* hgroup information pannel */
                        Child, (IPTR) VGroup,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) ColGroup(2),
                                    Child, (IPTR) Label2(__(MSG_TYPE)),
                                    Child, (IPTR) (typespace = (Object *)TextObject,
                                        TextFrame,
                                        MUIA_Background, MUII_TextBack,
                                        MUIA_Text_PreParse, (IPTR) "\33r",
                                        MUIA_Text_Contents, (IPTR) type,
                                    End),
                                   Child, (IPTR) Label2(__(MSG_DATE)),
                                    Child, (IPTR) (datespace = (Object *)TextObject,
                                        TextFrame,
                                        MUIA_Background, MUII_TextBack,
                                        MUIA_Text_PreParse, (IPTR) "\33r",
                                        MUIA_Text_Contents, (IPTR) datetime,
                                    End),
                                    Child, (IPTR) Label2(__(MSG_VERSION)),
                                    Child, (IPTR) (versiongrp = (Object *)HGroup,
                    Child, (IPTR) versionspace,
                    End),
                                    Child, (IPTR) Label2(__(MSG_SIZE)),
                                    Child, (IPTR) (sizegrp = (Object *)HGroup,
                    Child, (IPTR) sizespace,
                    End),
                                 End,
                            End,
                            Child, (IPTR) HVSpace,
                            Child, (IPTR) (grouptool = (Object *)HGroup,
                            End),
                            Child, (IPTR) (groupstack = (Object *)HGroup,
                            End),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) Label2(__(MSG_COMMENT)),
                                Child, (IPTR) (commentspace = (Object *)StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                End),
                            End,
                        End,
                    End, /* end hgroup information pannel */
                    Child, (IPTR) HGroup, /* hgroup protections pannel */
            Child, (IPTR) HVSpace,
                        Child, (IPTR) VGroup,
                Child, (IPTR) HVSpace,
                Child, (IPTR) ColGroup(4),
                Child, (IPTR) Label2(_(MSG_READ)),
                Child, (IPTR) (readobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                Child, (IPTR) Label2(_(MSG_SCRIPT)),
                Child, (IPTR) (scriptobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                Child, (IPTR) Label2(_(MSG_WRITE)),
                Child, (IPTR) (writeobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                Child, (IPTR) Label2(_(MSG_PURE)),
                Child, (IPTR) (pureobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                Child, (IPTR) Label2(_(MSG_EXECUTE)),
                Child, (IPTR) (executeobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                Child, (IPTR) Label2(_(MSG_ARCHIVED)),
                Child, (IPTR) (archiveobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                Child, (IPTR) Label2(_(MSG_DELETE)),
                Child, (IPTR) (deleteobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                Child, (IPTR) HVSpace,
                Child, (IPTR) HVSpace,
                End,
                Child, (IPTR) HVSpace,
                        End,
            Child, (IPTR) HVSpace,
                    End, /* end hgroup protections pannel */
                    Child, (IPTR) HGroup, /* hgroup tooltypes pannel */
                        Child, (IPTR) VGroup,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) VGroup,
                                GroupSpacing(0),
                #if !USE_TEXTEDITOR
                                    Child, (IPTR) ListviewObject,
                                        MUIA_Listview_List, (IPTR) (list = ListObject,
                                        InputListFrame,
                                            MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                                            MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                                            MUIA_List_AutoVisible, TRUE,
                                        End),
                                    End,
                                    Child, (IPTR) (liststr = StringObject,
                                        MUIA_Disabled, TRUE,
                                        StringFrame,
                                    End),
                #else
                    Child, (IPTR) HGroup,
                        GroupSpacing(0),
                        Child, (IPTR) (editor = MUI_NewObject(MUIC_TextEditor,
                        TAG_DONE)),
                    Child, (IPTR) (slider = (Object *)ScrollbarObject,
                    End),
                    End,
                #endif
                                End,
                            End,
            #if !USE_TEXTEDITOR
                            Child, (IPTR) HGroup,
                                MUIA_Group_SameSize, TRUE,
                                Child, (IPTR) (newkey = SimpleButton(__(MSG_NEW_KEY))),
                                Child, (IPTR) (delkey = SimpleButton(__(MSG_DELETE_KEY))),
                            End,
            #endif
                        End,
                    End, /* end hgroup tooltypes pannel */
                End),
                Child, (IPTR) HGroup,
                    MUIA_Group_SameSize, TRUE,
                    Child, (IPTR) (savebutton = 
                     ImageButton(_(MSG_SAVE),"THEME:Images/Gadgets/Prefs/Save")),
                    Child, (IPTR) (cancelbutton = 
                     ImageButton(_(MSG_CANCEL),"THEME:Images/Gadgets/Prefs/Cancel")),
                End,
            End,
        End),
    End;

    CurrentDir(cd);

    if (application != NULL)
    {
        ULONG signals = 0;

    #if !USE_TEXTEDITOR
        set(liststr, MUIA_String_AttachedList, (IPTR)list);
    #endif
    
        // menu
        DoMethod(aboutmenu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_ABOUT);
        DoMethod(infomenu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) registergroup, 3, MUIM_Set, MUIA_Group_ActivePage, 0);
        DoMethod(protectmenu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) registergroup, 3, MUIM_Set, MUIA_Group_ActivePage, 1);
        DoMethod(toolmenu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) registergroup, 3, MUIM_Set, MUIA_Group_ActivePage, 2);
        DoMethod(quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit);

        DoMethod(cancelbutton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        DoMethod(savebutton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_SAVE);
        DoMethod(application, MUIM_Notify, MUIA_Application_Sleep, FALSE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_WAKE);
        DoMethod(application, MUIM_Notify, MUIA_Application_Sleep, TRUE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_SLEEP);
        DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR) application, 2, MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit );
    #if !USE_TEXTEDITOR
        DoMethod(newkey, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_NEWKEY);
        DoMethod(delkey, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_DELKEY);
        DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_LVACK);
        DoMethod(liststr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_STRINGACK);
    #endif
        DoMethod(commentspace, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_COMMENTACK);
        DoMethod(versionspace, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_QUERYVERSION);
    DoMethod(sizespace, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR) application, 2,
        MUIM_Application_ReturnID, RETURNID_SCANSIZE);
        DoMethod(readobject, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_PROTECT);
        DoMethod(writeobject, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_PROTECT);
        DoMethod(executeobject, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_PROTECT);
        DoMethod(deleteobject, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_PROTECT);
        DoMethod(scriptobject, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_PROTECT);
        DoMethod(pureobject, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_PROTECT);
        DoMethod(archiveobject, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_PROTECT);

    #if USE_TEXTEDITOR
        set(editor, MUIA_TextEditor_Slider, slider);

        if (icon->do_ToolTypes)
        {
            char *tt = NULL, *contents;
            int   i  = 0;
        ULONG len = 0;
        
            while ((tt = icon->do_ToolTypes[i]) != NULL)
            {
            len += strlen(icon->do_ToolTypes[i]) + 1;
        i++;
        }
        
        contents = AllocVec(len + 1, MEMF_ANY);
        if (contents)
        {
            contents[0] = 0;
        i = 0;
        BOOL first = TRUE;

            while ((icon->do_ToolTypes[i]) != NULL)
            {
            if ( ! first )
            {
            strcat(contents, "\n");
            }
            first = FALSE;
            strcat(contents, icon->do_ToolTypes[i]);

            i++;
        }

                set(editor, MUIA_TextEditor_Contents, contents);
        FreeVec(contents);
            }
        }
    #else
        
        if (icon->do_ToolTypes)
        {
            char *tt = NULL;
            int   i  = 0;
            while ((tt = icon->do_ToolTypes[i]) != NULL)
            {
                DoMethod(list, MUIM_List_InsertSingle, tt, MUIV_List_Insert_Bottom);
                i++;
            }
        }
    #endif
    
        switch(icon->do_Type)
        {
            case WBPROJECT:
                AddToolField(application, grouptool, deftool);
                AddStackField(application, groupstack, stack);
                break;
            case WBTOOL:
                AddEmptySpace(grouptool);
                AddStackField(application, groupstack, stack);
                break;
            case WBDRAWER:
                AddEmptySpace(grouptool);
                AddEmptySpace(groupstack);
                break;
        }

        if (comment != NULL)
            set(commentspace, MUIA_String_Contents, comment);

        nnset(readobject, MUIA_Selected, flags[3]);
        nnset(writeobject, MUIA_Selected, flags[4]);
        nnset(executeobject, MUIA_Selected, flags[5]);
        nnset(deleteobject, MUIA_Selected, flags[6]);

        nnset(scriptobject, MUIA_Selected, flags[0]);
        nnset(pureobject, MUIA_Selected, flags[1]);
        nnset(archiveobject, MUIA_Selected, flags[2]);

        SetAttrs(window, MUIA_Window_Open, TRUE, TAG_DONE);

        while (returnid != MUIV_Application_ReturnID_Quit)
        { 
#ifdef DEBUG
            if (returnid)
            {
D(bug("[WBInfo] broker command received: %ld\n", returnid));
#endif
                switch(returnid)
                {
                    case RETURNID_SLEEP:
                        set(window, MUIA_Window_Open, FALSE);
                        set(window, MUIA_Window_Activate, TRUE);
                        break;
                    case RETURNID_WAKE:
                        set(window, MUIA_Window_Open, TRUE);
                        break;
                    case RETURNID_ABOUT:
                        if (MUI_RequestA(application, NULL, 0,
                            _(MSG_ABOUT), _(MSG_OK), _(MSG_COPYRIGHT), NULL))
                        break;
        #if !USE_TEXTEDITOR
                    case RETURNID_NEWKEY:
                        NewKey();
                        break;
                    case RETURNID_DELKEY:
                        DelKey();
                        icon_altered = TRUE;
                        break;
                    case RETURNID_LVACK:
                        ListToString();
                        break;  
                    case RETURNID_STRINGACK:
                        StringToKey();
                        icon_altered = TRUE;
                        break;
        #endif
                    case RETURNID_SAVE:
            #if !USE_TEXTEDITOR
                        if (icon_altered)
            #endif
                            SaveIcon(icon, file, lock);
                        if (file_altered)
                            SaveFile(ap, lock);
                        DoMethod(application, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                        break;
                    case RETURNID_STACKACK:
                        set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
                        icon_altered = TRUE;
                        break;
                    case RETURNID_TOOLACK:
                        set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
                        icon_altered = TRUE;
                        break;
                    case RETURNID_COMMENTACK:
                        set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
                        file_altered = TRUE;
                        break;
                    case RETURNID_QUERYVERSION:
            {
                Object * oldversionspace = versionspace;

D(bug("[WBInfo: RETURNID_QUERYVERSION\n"));

                versionspace = (Object *)TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_SetMin, FALSE,
                    MUIA_Text_PreParse, (IPTR) "\33r",
                    MUIA_Text_Contents, (IPTR) GetVersion(name, lock),
                End;

                DoMethod(versiongrp, MUIM_Group_InitChange);
                DoMethod(versiongrp, OM_REMMEMBER, oldversionspace);
                DoMethod(versiongrp, OM_ADDMEMBER, versionspace);
                DoMethod(versiongrp, MUIM_Group_ExitChange);
            }
                        break;
            case RETURNID_SCANSIZE:
            {
                Object * oldsizespace = sizespace;

D(bug("[WBInfo]: RETURNID_SCANSIZE\n"));
                sizespace = (Object *)TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR) "\33r",
                End;

                DoMethod(sizegrp, MUIM_Group_InitChange);
                DoMethod(sizegrp, OM_REMMEMBER, oldsizespace);
                DoMethod(sizegrp, OM_ADDMEMBER, sizespace);
                DoMethod(sizegrp, MUIM_Group_ExitChange);

                if (scanStruct == NULL)
                scanStruct = AllocMem(sizeof(struct DirScanProcess), MEMF_CLEAR);

                scanStruct->scanState = SCANDIE;
waitscan:

                if (scanStruct->scanProcess != NULL)
                goto waitscan;

                ULONG dirnamelen = strlen(lname) + strlen(name);
                scanStruct->scanState = SCANRUN;
                scanStruct->scanDir = AllocVec(dirnamelen + 1, MEMF_CLEAR);
D(bug("[WBInfo]: lname '%s', name '%s', (%d bytes)\n", lname, name, dirnamelen));

                CopyMem(lname, scanStruct->scanDir, strlen(lname));
                AddPart(scanStruct->scanDir, name, dirnamelen + 1);

                char * tmp_Name = AllocVec(strlen(scanStruct->scanDir) + 24, MEMF_CLEAR);
                sprintf(tmp_Name, "Calculating size of %s ..", scanStruct->scanDir);

                /* Fire up child process to perform scan of content size .. */
                scanStruct->scanProcess = CreateNewProcTags(
                NP_Entry, (IPTR)scanDir_Process,
                NP_Name, tmp_Name,
                NP_Synchronous , FALSE,
                NP_Priority, 0,
                NP_UserData, (IPTR)scanStruct,
                NP_StackSize, 140960,
                TAG_DONE);

                FreeVec(tmp_Name);
            }
            break;
                    case RETURNID_PROTECT:
                        file_altered = TRUE;
D(bug("[WBInfo: Protection bits changed\n"));
                        break;
                }
#ifdef DEBUG
            }
#endif
            if (signals)
            {
                signals = Wait(signals | SIGBREAKF_CTRL_C);
                if(signals & SIGBREAKF_CTRL_C) break;
            }

        returnid = ((LONG) DoMethod(application, MUIM_Application_NewInput, (IPTR) &signals));
        }
    if (scanStruct)
    {
        scanStruct->scanState = SCANDIE;
waitscan2:
        if (scanStruct->scanProcess != NULL)
        goto waitscan2;
    }
        SetAttrs(window, MUIA_Window_Open, FALSE, TAG_DONE);
        MUI_DisposeObject(application);
    } else {
        PrintFault(ERROR_INVALID_RESIDENT_LIBRARY, argv[0]);
D(bug("[WBInfo: Couldn't create app\n"));
    retval = RETURN_FAIL;
    }
   
funcmain_exit:
    if (scanStruct) FreeMem(scanStruct, sizeof(struct DirScanProcess));
    if (icon) FreeDiskObject(icon);
    if (ap) FreeVec(ap);
    if (file) FreeVec(file);
    if (name) FreeVec(name);
    return retval;
}
