/*
    Copyright © 2006-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include <aros/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "main.h"
#include "patches.h"
#include "setup.h"
#include "locale.h"


#define MAX_LOCK_LEN  100             /* Max. length of a lock string */

#define MAX(x,y)      ((x) > (y) ? (x) : (y))
#define MIN(x,y)      ((x) < (y) ? (x) : (y))

// dos.library
#define LVO_CreateDir      ( -20  * (WORD)LIB_VECTSIZE)
#define LVO_CurrentDir     ( -21  * (WORD)LIB_VECTSIZE)
#define LVO_DeleteFile     ( -12  * (WORD)LIB_VECTSIZE)
#define LVO_DeleteVar      ( -152 * (WORD)LIB_VECTSIZE)
#define LVO_Execute        ( -37  * (WORD)LIB_VECTSIZE)
#define LVO_FindVar        ( -153 * (WORD)LIB_VECTSIZE)
#define LVO_GetVar         ( -151 * (WORD)LIB_VECTSIZE)
#define LVO_LoadSeg        ( -25  * (WORD)LIB_VECTSIZE)
#define LVO_Lock           ( -14  * (WORD)LIB_VECTSIZE)
#define LVO_MakeLink       ( -74  * (WORD)LIB_VECTSIZE)
#define LVO_NewLoadSeg     ( -128 * (WORD)LIB_VECTSIZE)
#define LVO_Open           ( -5   * (WORD)LIB_VECTSIZE)
#define LVO_Rename         ( -13  * (WORD)LIB_VECTSIZE)
#define LVO_RunCommand     ( -84  * (WORD)LIB_VECTSIZE)
#define LVO_SetVar         ( -150 * (WORD)LIB_VECTSIZE)
#define LVO_SystemTagList  ( -101 * (WORD)LIB_VECTSIZE)

// exec.library
#define LVO_FindPort       ( -65  * (WORD)LIB_VECTSIZE)
#define LVO_FindResident   ( -16  * (WORD)LIB_VECTSIZE)
#define LVO_FindSemaphore  ( -99  * (WORD)LIB_VECTSIZE)
#define LVO_FindTask       ( -49  * (WORD)LIB_VECTSIZE)
#define LVO_OpenDevice     ( -74  * (WORD)LIB_VECTSIZE)
#define LVO_OpenLibrary    ( -92  * (WORD)LIB_VECTSIZE)
#define LVO_OpenResource   ( -83  * (WORD)LIB_VECTSIZE)

// intuition.library
#define LVO_LockPubScreen  ( -85  * (WORD)LIB_VECTSIZE)

// graphics.library
#define LVO_OpenFont       ( -12  * (WORD)LIB_VECTSIZE)

// icon.library
#define LVO_FindToolType   ( -16  * (WORD)LIB_VECTSIZE)
#define LVO_MatchToolValue ( -17  * (WORD)LIB_VECTSIZE)

struct Library *libbases[LIB_last];

typedef LONG (*FP)();

struct Patches
{
    LONG (*oldfunc)();
    LONG (*newfunc)();
    LONG libidx;
    struct Library *lib;
    LONG lvo;
    BOOL enabled;
} patches[PATCH_last] = 
{
    // must be in same order as in enum in patches.h
    {NULL, NULL, LIB_Dos,       0, LVO_CreateDir,      FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_CurrentDir,     FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_DeleteFile,     FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_DeleteVar,      FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_Execute,        FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_FindVar,        FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_GetVar,         FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_LoadSeg,        FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_Lock,           FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_MakeLink,       FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_NewLoadSeg,     FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_Open,           FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_Rename,         FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_RunCommand,     FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_SetVar,         FALSE},
    {NULL, NULL, LIB_Dos,       0, LVO_SystemTagList,  FALSE},
    {NULL, NULL, LIB_Exec,      0, LVO_FindPort,       FALSE},
    {NULL, NULL, LIB_Exec,      0, LVO_FindResident,   FALSE},
    {NULL, NULL, LIB_Exec,      0, LVO_FindSemaphore,  FALSE},
    {NULL, NULL, LIB_Exec,      0, LVO_FindTask,       FALSE},
    {NULL, NULL, LIB_Exec,      0, LVO_OpenDevice,     FALSE},
    {NULL, NULL, LIB_Exec,      0, LVO_OpenLibrary,    FALSE},
    {NULL, NULL, LIB_Exec,      0, LVO_OpenResource,   FALSE},
    {NULL, NULL, LIB_Intuition, 0, LVO_LockPubScreen,  FALSE},
    {NULL, NULL, LIB_Graphics,  0, LVO_OpenFont,       FALSE},
    {NULL, NULL, LIB_Icon,      0, LVO_FindToolType,   FALSE},
    {NULL, NULL, LIB_Icon,      0, LVO_MatchToolValue, FALSE},
};

// ----------------------------------------------------------------------------------

AROS_LH1(BPTR, New_CreateDir,
    AROS_LHA(CONST_STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 20, Dos)
{
    AROS_LIBFUNC_INIT

    // result is exclusive lock or NULL
    BPTR result = AROS_CALL1(BPTR, patches[PATCH_CreateDir].oldfunc,
        AROS_LDA(CONST_STRPTR, name,    D1),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_CreateDir].enabled)
    {
	main_output("CreateDir", name, 0, (IPTR)result, TRUE, TRUE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(BPTR, New_CurrentDir,
    AROS_LHA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 21, Dos)
{
    AROS_LIBFUNC_INIT
    char lockbuf[MAX_LOCK_LEN+1];
    char *lockpath;

    // returns lock to old directory, 0 means boot filesystem
    BPTR result = AROS_CALL1(BPTR, patches[PATCH_CurrentDir].oldfunc,
	AROS_LDA(BPTR, lock,    D1),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_CurrentDir].enabled)
    {
        lockpath = MyNameFromLock(lock, NULL, lockbuf, MAX_LOCK_LEN);
	main_output("CurrentDir", lockpath, 0, TRUE, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(BOOL, New_DeleteFile,
    AROS_LHA(CONST_STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 12, Dos)
{
    AROS_LIBFUNC_INIT

    // true means deleting was OK
    BOOL result = AROS_CALL1(BOOL, patches[PATCH_DeleteFile].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    D1),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_DeleteFile].enabled)
    {
	main_output("Delete", name, 0, result, TRUE, TRUE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(LONG, New_DeleteVar,
    AROS_LHA(CONST_STRPTR, name, D1),
    AROS_LHA(ULONG , flags, D2),
    struct DosLibrary *, DOSBase, 152, Dos)
{
    AROS_LIBFUNC_INIT

    // true means variable was deleted
    LONG result = AROS_CALL2(LONG, patches[PATCH_DeleteVar].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    D1),
	AROS_LDA(ULONG ,       flags,   D2),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_DeleteVar].enabled)
    {
	CONST_STRPTR opt;
	if      (flags & GVF_GLOBAL_ONLY) opt = MSG(MSG_GLOBAL);
        else if ((flags & 7) == LV_VAR)   opt = MSG(MSG_LOCAL);
        else if ((flags & 7) == LV_ALIAS) opt = MSG(MSG_ALIAS);
        else                              opt = MSG(MSG_UNKNOWN);

	main_output("DeleteVar", name, opt, result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH3(LONG, New_Execute,
    AROS_LHA(CONST_STRPTR, string, D1),
    AROS_LHA(BPTR  , input , D2),
    AROS_LHA(BPTR  , output, D3),
    struct DosLibrary *, DOSBase, 37, Dos)
{
    AROS_LIBFUNC_INIT

    // true means command could be started
    LONG result = AROS_CALL3(LONG, patches[PATCH_Execute].oldfunc,
	AROS_LDA(CONST_STRPTR, string,  D1),
	AROS_LDA(BPTR,   input ,  D2),
	AROS_LDA(BPTR,   output,  D3),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_Execute].enabled)
    {
	main_output("Execute", string ,0 , result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(struct LocalVar *, New_FindVar,
    AROS_LHA(CONST_STRPTR, name, D1),
    AROS_LHA(ULONG       , type, D2),
    struct DosLibrary *, DOSBase, 153, Dos)
{
    AROS_LIBFUNC_INIT

    // NULL means variable not found
    struct LocalVar *result = AROS_CALL2(struct LocalVar *, patches[PATCH_FindVar].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    D1),
	AROS_LDA(ULONG,        type,    D2),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_FindVar].enabled)
    {
	CONST_STRPTR opt;
	if      ((type & 7) == LV_VAR)   opt = MSG(MSG_LOCAL);
	else if ((type & 7) == LV_ALIAS) opt = MSG(MSG_ALIAS);
	else                             opt = MSG(MSG_UNKNOWN);

	main_output("FindVar", name, opt, (IPTR)result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH4(LONG, New_GetVar,
    AROS_LHA(CONST_STRPTR, name,   D1),
    AROS_LHA(STRPTR,       buffer, D2),
    AROS_LHA(LONG,         size,   D3),
    AROS_LHA(LONG,         flags,  D4),
    struct DosLibrary *, DOSBase, 151, Dos)
{
    AROS_LIBFUNC_INIT

    // -1 means variable not defined
    LONG result = AROS_CALL4(LONG, patches[PATCH_GetVar].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    D1),
	AROS_LDA(STRPTR,       buffer,  D2),
	AROS_LDA(LONG,         size,    D3),
	AROS_LDA(LONG,         flags,   D4),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_GetVar].enabled)
    {
	CONST_STRPTR opt;
	if      (flags & GVF_GLOBAL_ONLY) opt = MSG(MSG_GLOBAL);
        else if ((flags & 7) == LV_ALIAS) opt = MSG(MSG_ALIAS);
        else if (flags & GVF_LOCAL_ONLY)  opt = MSG(MSG_LOCAL);
        else                              opt = MSG(MSG_ANY);

	main_output("GetVar", name, opt, result != -1, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(BPTR, New_LoadSeg,
    AROS_LHA(CONST_STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 25, Dos)
{
    AROS_LIBFUNC_INIT

    // 0 means load failed
    BPTR result = AROS_CALL1(BPTR, patches[PATCH_LoadSeg].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    D1),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_LoadSeg].enabled)
    {
	main_output("LoadSeg", name, 0, (IPTR)result, TRUE, TRUE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(BPTR, New_Lock,
    AROS_LHA(CONST_STRPTR, name,       D1),
    AROS_LHA(LONG,         accessMode, D2),
    struct DosLibrary *, DOSBase, 14, Dos)
{
    AROS_LIBFUNC_INIT

    // 0 means error
    BPTR result = AROS_CALL2(BPTR, patches[PATCH_Lock].oldfunc,
	AROS_LDA(CONST_STRPTR, name,       D1),
	AROS_LDA(LONG,         accessMode, D2),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_Lock].enabled)
    {
	CONST_STRPTR opt;
	if      (accessMode == ACCESS_READ)  opt = MSG(MSG_READ);
	else if (accessMode == ACCESS_WRITE) opt = MSG(MSG_WRITE);
	else                                 opt = MSG(MSG_READ_ASK);

	CONST_STRPTR curname = name;
	if ( ! curname)
	{
	    curname="NULL";
	}
	else if ( ! setup.showPaths &&  *curname == '\0')
	{
	    curname = "\"\"";
	}

	main_output("Lock", curname, opt, (IPTR)result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH3(LONG, New_MakeLink,
    AROS_LHA(CONST_STRPTR, name, D1),
    AROS_LHA(APTR,   dest, D2),
    AROS_LHA(LONG  , soft, D3),
    struct DosLibrary *, DOSBase, 74, Dos)
{
    AROS_LIBFUNC_INIT

    // result is boolean
    LONG result = AROS_CALL3(LONG, patches[PATCH_MakeLink].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    D1),
	AROS_LDA(APTR,   dest,    D2),
	AROS_LDA(LONG,   soft,    D3),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_MakeLink].enabled)
    {
        struct Process *myproc = (struct Process *)SysBase->ThisTask;

	CONST_STRPTR opt;
	if (soft) opt = "Softlink";
	else      opt = "Hardlink";

	int len = strlen(name);
	char namestr[MAX_STR_LEN + 1];
	if (len >= MAX_STR_LEN)
        {
	    strncpy(namestr, name, MAX_STR_LEN);
	    namestr[MAX_STR_LEN] = 0;
	}
        else
        {
	    if (setup.showPaths)
            {
		strcpy(namestr, MyNameFromLock(myproc->pr_CurrentDir,
			    name, namestr, MAX_STR_LEN-2));
		len = strlen(namestr);
	    }
            else
		strcpy(namestr, name);

	    strcat(namestr, " --> ");
	    if (soft)
            {
		strncat(namestr, (char *)dest, MAX_STR_LEN - len - 1);
		namestr[MAX_STR_LEN] = 0;
	    }
            else
            {
		strcat(namestr, MyNameFromLock((BPTR)dest, NULL, namestr+len+1,
			    MAX_STR_LEN-len-1));
	    }
	}

	main_output("MakeLink", namestr, opt, result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(BPTR, New_NewLoadSeg,
    AROS_LHA(CONST_STRPTR, file, D1),
    AROS_LHA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 128, Dos)
{
    AROS_LIBFUNC_INIT

    // 0 means load failed
    BPTR result = AROS_CALL2(BPTR, patches[PATCH_NewLoadSeg].oldfunc,
	AROS_LDA(CONST_STRPTR,           file,    D1),
	AROS_LDA(struct TagItem *, tags,    D2),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_NewLoadSeg].enabled)
    {
	main_output("NewLoadSeg", file, 0, (IPTR)result, TRUE, TRUE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(BPTR, New_Open,
    AROS_LHA(CONST_STRPTR, name,       D1),
    AROS_LHA(LONG,         accessMode, D2),
    struct DosLibrary *, DOSBase, 5, Dos)
{
    AROS_LIBFUNC_INIT

    // 0 means error
    BPTR result = AROS_CALL2(BPTR, patches[PATCH_Open].oldfunc,
	AROS_LDA (CONST_STRPTR, name,       D1),
	AROS_LDA (LONG,         accessMode, D2),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_Open].enabled)
    {
	CONST_STRPTR opt = NULL;
	char optstr[10];
	if      (accessMode == MODE_OLDFILE)   opt = MSG(MSG_READ);
	else if (accessMode == MODE_NEWFILE)   opt = MSG(MSG_WRITE);
	else if (accessMode == MODE_READWRITE) opt = MSG(MSG_MODIFY);
	else
            opt = MSG(MSG_UNKNOWN);

	main_output("Open", name, opt ? opt : (STRPTR)optstr, (IPTR)result, TRUE, TRUE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(LONG, New_Rename,
    AROS_LHA(CONST_STRPTR, oldName, D1),
    AROS_LHA(CONST_STRPTR, newName, D2),
    struct DosLibrary *, DOSBase, 13, Dos)
{
    AROS_LIBFUNC_INIT

    // bool
    LONG result = AROS_CALL2(LONG, patches[PATCH_Rename].oldfunc,
	AROS_LDA(CONST_STRPTR, oldName, D1),
	AROS_LDA(CONST_STRPTR, newName, D2),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_Rename].enabled)
    {
	main_output("Rename", oldName, 0, result, FALSE, TRUE);
	main_output("to -->", newName, 0, result, TRUE, TRUE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH4(LONG, New_RunCommand,
    AROS_LHA(BPTR,   segList,   D1),
    AROS_LHA(ULONG,  stacksize, D2),
    AROS_LHA(CONST_STRPTR, argptr,    D3),
    AROS_LHA(ULONG,  argsize,   D4),
    struct DosLibrary *, DOSBase, 84, Dos)
{
    AROS_LIBFUNC_INIT

    // -1 means error
    LONG result = AROS_CALL4(LONG, patches[PATCH_RunCommand].oldfunc,
	AROS_LDA(BPTR,   segList,   D1),
	AROS_LDA(ULONG,  stacksize, D2),
	AROS_LDA(CONST_STRPTR, argptr,    D3),
	AROS_LDA(ULONG,  argsize,   D4),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_RunCommand].enabled)
    {
	char argstr[MAX_STR_LEN + 1];
	int pos;
	for (pos = 0; pos < MAX_STR_LEN && argptr[pos] != 0 ; pos++)
	{
	    if (argptr[pos] == '\n')
		argstr[pos] = ' ';
	    else
		argstr[pos] = argptr[pos];
	}

	argstr[pos] = 0;
	main_output("RunCommand", argstr, 0, result != -1, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH4(BOOL, New_SetVar,
    AROS_LHA(CONST_STRPTR, name, D1),
    AROS_LHA(CONST_STRPTR, buffer, D2),
    AROS_LHA(LONG        , size, D3),
    AROS_LHA(LONG        , flags, D4),
    struct DosLibrary *, DOSBase, 150, Dos)
{
    AROS_LIBFUNC_INIT

    BOOL result = AROS_CALL4(BOOL, patches[PATCH_SetVar].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    D1),
	AROS_LDA(CONST_STRPTR, buffer,  D2),
	AROS_LDA(LONG,         size,    D3),
	AROS_LDA(LONG,         flags,   D4),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_SetVar].enabled)
    {
	CONST_STRPTR opt;
	char varstr[MAX_STR_LEN + 1];
        int vlen;

	if      (flags & GVF_GLOBAL_ONLY) opt = MSG(MSG_GLOBAL);
	else if ((flags & 7) == LV_VAR)   opt = MSG(MSG_LOCAL);
	else if ((flags & 7) == LV_ALIAS) opt = MSG(MSG_ALIAS);
	else                              opt = MSG(MSG_UNKNOWN);

	/*
	 *              Now create a string that looks like "Variable=Value"
	 *
	 *              We go to some pains to ensure we don't overwrite our
	 *              string length
	 */
	vlen = strlen(name);
	if (vlen > (MAX_STR_LEN-1)) {
	    strncpy(varstr, name, MAX_STR_LEN);
	    varstr[MAX_STR_LEN] = 0;
	} else {
	    strcpy(varstr, name);
	    strcat(varstr, "=");
	    vlen = 98 - vlen;
	    if (size != -1)
		vlen = MIN(vlen, size);

	    strncat(varstr, buffer, vlen);
	    varstr[MAX_STR_LEN] = 0;
	}
	main_output("SetVar", varstr, opt, result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(LONG, New_SystemTagList,
    AROS_LHA(CONST_STRPTR    , command, D1),
    AROS_LHA(struct TagItem *, tags,    D2),
    struct DosLibrary *, DOSBase, 101, Dos)
{
    AROS_LIBFUNC_INIT

    // -1 means error
    LONG result = AROS_CALL2(LONG, patches[PATCH_SystemTagList].oldfunc,
	AROS_LDA(CONST_STRPTR,     command, D1),
	AROS_LDA(struct TagItem *, tags,    D2),
	struct DosLibrary *, DOSBase);

    if (patches[PATCH_SystemTagList].enabled)
    {
	char optstr[20];
	sprintf(optstr, "%d", (int)result);
	main_output("SystemTagList", command, optstr, result != -1, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(struct MsgPort *, New_FindPort,
    AROS_LHA(STRPTR, name, A1),
    struct ExecBase *, SysBase, 65, Exec)
{
    AROS_LIBFUNC_INIT

    // NULL means error
    struct MsgPort *result = AROS_CALL1(struct MsgPort *, patches[PATCH_FindPort].oldfunc,
	AROS_LDA(STRPTR, name,    A1),
	struct ExecBase *, SysBase);

    if (patches[PATCH_FindPort].enabled)
    {
	main_output("FindPort", name, 0, (IPTR)result, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(struct Resident *, New_FindResident,
    AROS_LHA(const UBYTE *, name, A1),
    struct ExecBase *, SysBase, 16, Exec)
{
    AROS_LIBFUNC_INIT

    // NULL means error
    struct Resident *result = AROS_CALL1(struct Resident *, patches[PATCH_FindResident].oldfunc,
	AROS_LDA(const UBYTE *, name,    A1),
	struct ExecBase *, SysBase);

    if (patches[PATCH_FindResident].enabled)
    {
	main_output("FindResident", name, 0, (IPTR)result, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(struct SignalSemaphore *, New_FindSemaphore,
    AROS_LHA(STRPTR, name, A1),
    struct ExecBase *, SysBase, 99, Exec)
{
    AROS_LIBFUNC_INIT

    // NULL means error
    struct SignalSemaphore *result = AROS_CALL1(struct SignalSemaphore *, patches[PATCH_FindSemaphore].oldfunc,
	AROS_LDA(STRPTR, name,    A1),
	struct ExecBase *, SysBase);

    if (patches[PATCH_FindSemaphore].enabled)
    {
	main_output("FindSemaphore", name, 0, (IPTR)result, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(struct Task *, New_FindTask,
    AROS_LHA(STRPTR, name, A1),
    struct ExecBase *, SysBase, 49, Exec)
{
    AROS_LIBFUNC_INIT

    // NULL means error
    struct Task *result = AROS_CALL1(struct Task *, patches[PATCH_FindTask].oldfunc,
	AROS_LDA(STRPTR, name,    A1),
	struct ExecBase *, SysBase);

    if ((name != NULL) && patches[PATCH_FindTask].enabled)
    {
	main_output("FindTask", name, 0, (IPTR)result, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH4(LONG, New_OpenDevice,
    AROS_LHA(CONST_STRPTR,       devName,    A0),
    AROS_LHA(IPTR,               unitNumber, D0),
    AROS_LHA(struct IORequest *, iORequest,  A1),
    AROS_LHA(ULONG,              flags,      D1),
    struct ExecBase *, SysBase, 74, Exec)
{
    AROS_LIBFUNC_INIT

    // 0 means OK
    LONG result = AROS_CALL4(LONG, patches[PATCH_OpenDevice].oldfunc,
	AROS_LDA(CONST_STRPTR,       devName,    A0),
	AROS_LDA(IPTR,               unitNumber, D0),
	AROS_LDA(struct IORequest *, iORequest,  A1),
	AROS_LDA(ULONG,              flags,      D1),
	struct ExecBase *, SysBase);

    if (patches[PATCH_OpenDevice].enabled)
    {
	char unitstr[20];
        // FIXME: unitNumber can be a pointer
	sprintf(unitstr, "Unit %d", (int)unitNumber);
	main_output("OpenDevice", devName, unitstr, !result, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(struct Library *, New_OpenLibrary,
    AROS_LHA(CONST_STRPTR,  libName, A1),
    AROS_LHA(ULONG,         version, D0),
    struct ExecBase *, SysBase, 92, Exec)
{
    AROS_LIBFUNC_INIT

    // 0 means error
    struct Library *result = AROS_CALL2(struct Library *, patches[PATCH_OpenLibrary].oldfunc,
	AROS_LDA(CONST_STRPTR,  libName, A1),
	AROS_LDA(ULONG,         version, D0),
	struct ExecBase *, SysBase);

    if (patches[PATCH_OpenLibrary].enabled)
    {
	char verstr[20];
	sprintf(verstr, MSG(MSG_VERSION), version);
	main_output("OpenLibrary", libName, verstr, (IPTR)result, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(APTR, New_OpenResource,
    AROS_LHA(CONST_STRPTR, resName, A1),
    struct ExecBase *, SysBase, 83, Exec)
{
    AROS_LIBFUNC_INIT

    // 0 means error
    APTR result = AROS_CALL1(APTR, patches[PATCH_OpenResource].oldfunc,
	AROS_LDA(CONST_STRPTR, resName, A1),
	struct ExecBase *, SysBase);

    if (patches[PATCH_OpenResource].enabled)
    {
	main_output("OpenLibrary", resName, 0, (IPTR)result, TRUE, FALSE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(struct Screen *, New_LockPubScreen,
    AROS_LHA(CONST_STRPTR, name, A0),
    struct IntuitionBase *, IntuitionBase, 85, Intuition)
{
    AROS_LIBFUNC_INIT

    // 0 means error
    struct Screen *result = AROS_CALL1(struct Screen *, patches[PATCH_LockPubScreen].oldfunc,
	AROS_LDA(CONST_STRPTR, name,    A0),
	struct IntuitionBase *, IntuitionBase);

    if (patches[PATCH_LockPubScreen].enabled)
    {
	main_output("LockPubScreen", name, 0, (IPTR)result, TRUE, TRUE);
    }
    
    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH1(struct TextFont *, New_OpenFont,
    AROS_LHA(struct TextAttr *, textAttr, A0),
    struct GfxBase *, GfxBase, 12, Graphics)
{
    AROS_LIBFUNC_INIT

    // 0 means error
    struct TextFont *result = AROS_CALL1(struct TextFont *, patches[PATCH_OpenFont].oldfunc,
	AROS_LDA(struct TextAttr *, textAttr, A0),
	struct GfxBase *, GfxBase);

    if (patches[PATCH_OpenFont].enabled)
    {
	char *name;
	char sizestr[20];

	if (textAttr) {
	    sprintf(sizestr, MSG(MSG_SIZE), textAttr->ta_YSize);
	    name = textAttr->ta_Name;
	} else {
	    *sizestr = '\0';
	    name = "\"\"";
	}
	main_output("OpenFont", name, sizestr, (IPTR)result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(UBYTE *, New_FindToolType,
    AROS_LHA(CONST STRPTR *, toolTypeArray, A0),
    AROS_LHA(CONST STRPTR,   typeName,      A1),
    struct Library *, IconBase, 16, Icon)
{
    AROS_LIBFUNC_INIT

    // 0 means error
    UBYTE *result = AROS_CALL2(UBYTE *, patches[PATCH_FindToolType].oldfunc,
	AROS_LDA(CONST STRPTR *, toolTypeArray, A0),
	AROS_LDA(CONST STRPTR,   typeName,      A1),
	struct Library *, IconBase);

    if (patches[PATCH_FindToolType].enabled)
    {
	main_output("FindToolType", typeName, 0, (IPTR)result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_LH2(BOOL, New_MatchToolValue,
    AROS_LHA(UBYTE *, typeString, A0),
    AROS_LHA(UBYTE *, value, A1),
    struct Library *, IconBase, 17, Icon)
{
    AROS_LIBFUNC_INIT

    BOOL result = AROS_CALL2(BOOL, patches[PATCH_MatchToolValue].oldfunc,
	AROS_LDA(UBYTE *, typeString, A0),
	AROS_LDA(UBYTE *, value,      A1),
	struct Library *, IconBase);

    if (patches[PATCH_MatchToolValue].enabled)
    {
	main_output("MatchToolValue", typeString, value, result, TRUE, FALSE);
    }

    return result;

    AROS_LIBFUNC_EXIT
}

// ----------------------------------------------------------------------------------

void patches_init(void)
{
    libbases[LIB_Exec]      = (struct Library*)SysBase;
    libbases[LIB_Dos]       = (struct Library*)DOSBase;
    libbases[LIB_Icon]      = IconBase;
    libbases[LIB_Intuition] = (struct Library*)IntuitionBase;
    libbases[LIB_Graphics]  = (struct Library*)GfxBase;

    patches[PATCH_CreateDir].newfunc      = (FP)AROS_SLIB_ENTRY(New_CreateDir, Dos, 20);
    patches[PATCH_CurrentDir].newfunc     = (FP)AROS_SLIB_ENTRY(New_CurrentDir, Dos, 21);
    patches[PATCH_DeleteFile].newfunc     = (FP)AROS_SLIB_ENTRY(New_DeleteFile, Dos, 12);
    patches[PATCH_DeleteVar].newfunc      = (FP)AROS_SLIB_ENTRY(New_DeleteVar, Dos, 152);
    patches[PATCH_Execute].newfunc        = (FP)AROS_SLIB_ENTRY(New_Execute, Dos, 37);
    patches[PATCH_FindVar].newfunc        = (FP)AROS_SLIB_ENTRY(New_FindVar, Dos, 153);
    patches[PATCH_GetVar].newfunc         = (FP)AROS_SLIB_ENTRY(New_GetVar, Dos, 151);
    patches[PATCH_LoadSeg].newfunc        = (FP)AROS_SLIB_ENTRY(New_LoadSeg, Dos, 25);
    patches[PATCH_Lock].newfunc           = (FP)AROS_SLIB_ENTRY(New_Lock, Dos, 14);
    patches[PATCH_MakeLink].newfunc       = (FP)AROS_SLIB_ENTRY(New_MakeLink, Dos, 74);
    patches[PATCH_NewLoadSeg].newfunc     = (FP)AROS_SLIB_ENTRY(New_NewLoadSeg, Dos, 128);
    patches[PATCH_Open].newfunc           = (FP)AROS_SLIB_ENTRY(New_Open, Dos, 5);
    patches[PATCH_Rename].newfunc         = (FP)AROS_SLIB_ENTRY(New_Rename, Dos, 13);
    patches[PATCH_RunCommand].newfunc     = (FP)AROS_SLIB_ENTRY(New_RunCommand, Dos, 84);
    patches[PATCH_SetVar].newfunc         = (FP)AROS_SLIB_ENTRY(New_SetVar, Dos, 150);
    patches[PATCH_SystemTagList].newfunc  = (FP)AROS_SLIB_ENTRY(New_SystemTagList, Dos, 101);
    patches[PATCH_FindPort].newfunc       = (FP)AROS_SLIB_ENTRY(New_FindPort, Exec, 65);
    patches[PATCH_FindResident].newfunc   = (FP)AROS_SLIB_ENTRY(New_FindResident, Exec, 16);
    patches[PATCH_FindSemaphore].newfunc  = (FP)AROS_SLIB_ENTRY(New_FindSemaphore, Exec, 99);
    patches[PATCH_FindTask].newfunc       = (FP)AROS_SLIB_ENTRY(New_FindTask, Exec, 49);
    patches[PATCH_OpenDevice].newfunc     = (FP)AROS_SLIB_ENTRY(New_OpenDevice, Exec, 74);
    patches[PATCH_OpenLibrary].newfunc    = (FP)AROS_SLIB_ENTRY(New_OpenLibrary, Exec, 92);
    patches[PATCH_OpenResource].newfunc   = (FP)AROS_SLIB_ENTRY(New_OpenResource, Exec, 83);
    patches[PATCH_LockPubScreen].newfunc  = (FP)AROS_SLIB_ENTRY(New_LockPubScreen, Intuition, 85);
    patches[PATCH_OpenFont].newfunc       = (FP)AROS_SLIB_ENTRY(New_OpenFont, Graphics, 12);
    patches[PATCH_FindToolType].newfunc   = (FP)AROS_SLIB_ENTRY(New_FindToolType, Icon, 16);
    patches[PATCH_MatchToolValue].newfunc = (FP)AROS_SLIB_ENTRY(New_MatchToolValue, Icon, 17);

    patches_set();

    int i;
    for (i=0; i<PATCH_last; i++)
    {
	if (patches[i].newfunc);
	{
	    Forbid();
	    patches[i].oldfunc = SetFunction(libbases[patches[i].libidx], patches[i].lvo, patches[i].newfunc);
	    Permit();
	}
    }
}

// ----------------------------------------------------------------------------------

void patches_set(void)
{
    patches[PATCH_CreateDir].enabled      = setup.enableMakeDir;
    patches[PATCH_CurrentDir].enabled     = setup.enableChangeDir;
    patches[PATCH_DeleteFile].enabled     = setup.enableDelete;
    patches[PATCH_DeleteVar].enabled      = setup.enableSetVar;
    patches[PATCH_Execute].enabled        = setup.enableExecute;
    patches[PATCH_FindVar].enabled        = setup.enableGetVar;
    patches[PATCH_GetVar].enabled         = setup.enableGetVar;
    patches[PATCH_LoadSeg].enabled        = setup.enableLoadSeg;
    patches[PATCH_Lock].enabled           = setup.enableLock;
    patches[PATCH_MakeLink].enabled       = setup.enableMakeLink;
    patches[PATCH_NewLoadSeg].enabled     = setup.enableLoadSeg;
    patches[PATCH_Open].enabled           = setup.enableOpen;
    patches[PATCH_Rename].enabled         = setup.enableRename;
    patches[PATCH_RunCommand].enabled     = setup.enableRunCommand;
    patches[PATCH_SetVar].enabled         = setup.enableSetVar;
    patches[PATCH_SystemTagList].enabled  = setup.enableSystem;
    patches[PATCH_FindPort].enabled       = setup.enableFindPort;
    patches[PATCH_FindResident].enabled   = setup.enableFindResident;
    patches[PATCH_FindSemaphore].enabled  = setup.enableFindSemaphore;
    patches[PATCH_FindTask].enabled       = setup.enableFindTask;
    patches[PATCH_OpenDevice].enabled     = setup.enableOpenDevice;
    patches[PATCH_OpenLibrary].enabled    = setup.enableOpenLibrary;
    patches[PATCH_OpenResource].enabled   = setup.enableOpenResource;
    patches[PATCH_LockPubScreen].enabled  = setup.enableLockScreen;
    patches[PATCH_OpenFont].enabled       = setup.enableOpenFont;
    patches[PATCH_FindToolType].enabled   = setup.enableReadToolTypes;
    patches[PATCH_MatchToolValue].enabled = setup.enableReadToolTypes;
  
}

// ----------------------------------------------------------------------------------

void patches_reset(void)
{
    int i;

    for (i=0; i<PATCH_last; i++)
    {
	patches[i].enabled = FALSE;
    }
    
    for (i=0; i<PATCH_last; i++)
    {
	if (patches[i].oldfunc)
	{
	    Forbid();
	    SetFunction(libbases[patches[i].libidx], patches[i].lvo, patches[i].oldfunc);
	    Permit();
	    patches[i].oldfunc = NULL;
	}
    }
}


/*
 * GetVolName(lock, buf, maxlen)
 *
 * Copies the volume name associated with lock into the buffer,
 * with terminating ':'. If lock is NULL, the volume address is
 * taken directly from volume.
 *
 * If UseDevNames is true, the device list is searched looking
 * for the device node associated with the volume node (i.e. two
 * nodes sharing the same task address).
 *
 * WARNING: This function must not be called from within a DOS
 * device handler due to potential deadlock errors!
 */
void GetVolName(BPTR lock, char *buf, int maxlen)
{
    struct DeviceList *vol;
    struct DosList *dl;
    int gotdev = 0;

    if (lock == NULL)
    {
        NameFromLock(lock, buf, maxlen);
        return;
    }
    vol = BADDR(((struct FileLock *)BADDR(lock))->fl_Volume);

    if (setup.useDevNames == 0 || vol->dl_Task == NULL)
    {
/*
* Use volume name, especially if the volume isn't currently
* mounted!
*/
        UBYTE *volname = AROS_BSTR_ADDR(vol->dl_Name);
        int len = MIN(maxlen-2, AROS_BSTR_strlen(volname));

        memcpy(buf, volname, len);
        buf[len++] = ':';
        buf[len] = '\0';
        return;
    }

/*
* The user wants the device name. The only way to obtain this
* is to search the device list looking for the device node with
* the same task address as this volume.
*/
    dl = LockDosList(LDF_DEVICES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_DEVICES)))
    {
        if (dl->dol_Task == vol->dl_Task)
        {
/*
* Found our task, so now copy device name
*/
            UBYTE *devname =  AROS_BSTR_ADDR(dl->dol_Name);
            int len = MIN(maxlen-2, AROS_BSTR_strlen(devname));

            memcpy(buf, devname, len);
            buf[len++] = ':';
            buf[len] = '\0';
            gotdev = 1;
            break;
        }
    }
    UnLockDosList(LDF_DEVICES | LDF_READ);
    if (!gotdev)
        strcpy(buf, "???:");
}

/*
 * MyNameFromLock(lock, filename, buf, maxlen)
 *
 * This is a custom version of the DOS function NameFromLock()
 * which expands a disk lock into a full path.
 *
 * Our version adds the following features. The device name will be
 * given as either the physical device (like DH0:) if UseDevNames
 * is true, or the volume name (like System3.0:) if UseDevNames is
 * false.
 *
 * If filename is non-NULL, then it will be appended to the lock
 * path. If filename contains path info, then this will be taken
 * into account when generating the lock, so that an absolute path
 * in filename will not have any effect, and a relative filename
 * (like //directory) will cause the original lock to be ParentDir()'d
 * twice before being resolved.
 *
 * This function can't fail. In the event of an error (string too
 * long, or something like that), the buffer will be filled accordingly.
 * It is assumed that the buffer will always be big enough to hold short
 * error messages or a volume name.
 *
 * Returns a pointer to the path (which will not necessarily be at
 * the start of the buffer, but is guaranteed null-terminated.)
 * Note that it's even possible that the pointer returned will be
 * to the original filename if no path expansion was required.
 *
 * New: We now preserve the IoErr() that was present on entry, since
 * it may have been set by the calling function if OnlyShowFails is
 * true. Otherwise, IoErr() will be screwed up by the operations we
 * do here (e.g. SAS/C deleting a non-existent file when OnlyShowFails
 * is true).
 *
 * WARNING: This function must not be called from within a DOS
 * device handler due to potential deadlock errors!
 */
char *MyNameFromLock(BPTR lock, char *filename, char *buf, int maxlen)
{
    struct Process *myproc = (struct Process *)SysBase->ThisTask;
    int pos = maxlen - 1;
    D_S(fib, struct FileInfoBlock);
    LONG savedioerr = IoErr();
    BPTR curlock;
    BPTR newlock;
    void *savewinptr;
    char *p;
    int len;
    int skipfirstslash = 0; /* If true, skip first slash when building name */
    int err = 0;

/*
* Check for special magic filehandle
*/
    if (filename && *filename)
    {
        if (strcmp(filename, "*") == 0)
            return (filename);

/*
* First determine if we have any work to do.
*/
        if (*filename == ':')
        {
/*
* Got a reference relative to the root directory. Simply
* grab the volume (or device) name from the lock and go
* with that.
*/
            int len;

            GetVolName(lock, buf, maxlen);
            len = strlen(buf);
            strncat(buf+len, filename+1, maxlen-len);
            buf[maxlen-1] = '\0';
            SetIoErr(savedioerr);
            return (buf);
        }
        for (p = filename; *p; p++)
        {
            if (*p == ':') /* If absolute path name, leave it alone */
                return (filename);
        }
    }
    else
    {
/*
* Filename is null, so indicate we want to skip the first
* slash when building the directory path
*/
        skipfirstslash = 1;
    }

    savewinptr = myproc->pr_WindowPtr;
    myproc->pr_WindowPtr = (APTR)-1;	/* Disable error requesters */

    newlock = DupLock(lock);
    if (lock && !newlock)
    {
        GetVolName(lock, buf, 20);
        if (filename)
        {
            strcat(buf, ".../");
            strcat(buf, filename);
        }
        myproc->pr_WindowPtr = savewinptr;	/* Re-enable error requesters */
        SetIoErr(savedioerr);
        return (buf);
    }
    buf[pos] = '\0';
    curlock = newlock;
    if (filename)
    {
        while (newlock && *filename == '/')
        {
/*
* Handle leading /'s by moving back a directory level
* but nothing else
*/
            newlock = ParentDir(curlock);
            if (newlock)
            {
                UnLock(curlock);
                curlock = newlock;
                filename++;
            }
        }
        len = strlen(filename);
        if (len > (pos-2))
        {
            memcpy(buf+2, filename+len-pos, pos-2);
            buf[0] = buf[1] = '.';
            pos = 0;
            UnLock(curlock);
        }
        else
        {
            pos -= len;
            memcpy(buf+pos, filename, len);
        }
    }

/*
* At this point, we have buf containing the filename (minus any
* leading /'s), starting at the index given by pos. If filename
* was NULL or empty, then pos indexes to a \0 terminator.
*
* Next, we want to pre-pend directory names to the front of
* the filename (assuming there _is_ a filename) until we get
* to the device root.
*/
    newlock = curlock;
    while (newlock)
    {
        if (!Examine(curlock, fib))
        {
            err++;
            break;
        }
        len = strlen(fib->fib_FileName);
        if (len > (pos-3))
        {
/*
* Not enough room: prefix dots at start to indicate
* an overrun. We use pos-3 since we need one char
* for a possible slash and two more to accomodate a
* leading ".."
*/
            memcpy(buf+2, fib->fib_FileName+len-pos+3, pos-2);
            buf[0] = buf[1] = '.';
            buf[pos-1] = '/';
            pos = 0;
            break;
        }
        newlock = ParentDir(curlock);
        if (newlock)
        {
            UnLock(curlock);
            curlock = newlock;
            pos -= len + 1;
            memcpy(buf + pos, fib->fib_FileName, len);
            if (skipfirstslash)
            {
                skipfirstslash = 0;
                buf[pos+len] = '\0';
            }
            else
                buf[pos+len] = '/';
        }
    }
/*
* Now we've built the path components; add the volume node
* to the beginning if possible.
*/
    if (err)
    {
/*
* If an error occurred, the volume is probably not mounted,
* so we include a ".../" component in the path to show
* we couldn't get all the info
*/
        pos -= 4;
        memcpy(buf + pos, ".../", 4);
    }
    if (pos > 0)
    {
        char volname[20];
        int len;
        char *p;

        GetVolName(curlock, volname, 20);
        len = strlen(volname);
        if (len > pos)
        {
            p = volname + len - pos;
            len = pos;
        }
        else
            p = volname;
        pos -= len;
        memcpy(buf + pos, p, len);
    }
    if (curlock)
        UnLock(curlock);

    myproc->pr_WindowPtr = savewinptr;	/* Re-enable error requesters */
    SetIoErr(savedioerr);
    return (buf+pos);
}
