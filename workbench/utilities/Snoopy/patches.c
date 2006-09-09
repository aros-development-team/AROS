/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

// Some code was taken from the SnoopDos program from Eddy Carroll

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
#define MAX_STR_LEN   200             /* Max. string length for misc stuff    */

#define MAX(x,y)      ((x) > (y) ? (x) : (y))
#define MIN(x,y)      ((x) < (y) ? (x) : (y))

// dos.library
#define LVO_CreateDir      ( -20  * LIB_VECTSIZE)
#define LVO_CurrentDir     ( -21  * LIB_VECTSIZE)
#define LVO_DeleteFile     ( -12  * LIB_VECTSIZE)
#define LVO_DeleteVar      ( -152 * LIB_VECTSIZE)
#define LVO_Execute        ( -37  * LIB_VECTSIZE)
#define LVO_FindVar        ( -153 * LIB_VECTSIZE)
#define LVO_GetVar         ( -151 * LIB_VECTSIZE)
#define LVO_LoadSeg        ( -25  * LIB_VECTSIZE)
#define LVO_Lock           ( -14  * LIB_VECTSIZE)
#define LVO_MakeLink       ( -74  * LIB_VECTSIZE)
#define LVO_NewLoadSeg     ( -128 * LIB_VECTSIZE)
#define LVO_Open           ( -5   * LIB_VECTSIZE)
#define LVO_Rename         ( -13  * LIB_VECTSIZE)
#define LVO_RunCommand     ( -84  * LIB_VECTSIZE)
#define LVO_SetVar         ( -150 * LIB_VECTSIZE)
#define LVO_SystemTagList  ( -101 * LIB_VECTSIZE)

// exec.library
#define LVO_FindPort       ( -65  * LIB_VECTSIZE)
#define LVO_FindResident   ( -16  * LIB_VECTSIZE)
#define LVO_FindSemaphore  ( -99  * LIB_VECTSIZE)
#define LVO_FindTask       ( -49  * LIB_VECTSIZE)
#define LVO_OpenDevice     ( -74  * LIB_VECTSIZE)
#define LVO_OpenLibrary    ( -92  * LIB_VECTSIZE)
#define LVO_OpenResource   ( -83  * LIB_VECTSIZE)

// intuition.library
#define LVO_LockPubScreen  ( -85  * LIB_VECTSIZE)

// graphics.library
#define LVO_OpenFont       ( -12  * LIB_VECTSIZE)

// icon.library
#define LVO_FindToolType   ( -16  * LIB_VECTSIZE)
#define LVO_MatchToolValue ( -17  * LIB_VECTSIZE)

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

//static char *MyNameFromLock(BPTR lock, char *filename, char *buf, int maxlen);
//static void GetVolName(BPTR lock, char *buf, int maxlen);

// ----------------------------------------------------------------------------------

AROS_UFH2(BPTR, New_CreateDir,
    AROS_UFHA(CONST_STRPTR, name,    D1),
    AROS_UFHA(APTR,         libbase, A6)
)
{
    AROS_USERFUNC_INIT

    // result is exclusive lock or NULL
    BPTR result = AROS_UFC2(BPTR, patches[PATCH_CreateDir].oldfunc,
        AROS_UFCA(CONST_STRPTR, name,    D1),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_CreateDir].enabled)
    {
	main_output("CreateDir", name, 0, (LONG)result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(BPTR, New_CurrentDir,
    AROS_UFHA(BPTR, lock,    D1),
    AROS_UFHA(APTR, libbase, A6))
{
    AROS_USERFUNC_INIT
    //char lockbuf[MAX_LOCK_LEN+1];
    char *lockpath = "?";

    // returns lock to old directory, 0 means boot filesystem
    BPTR result = AROS_UFC2(BPTR, patches[PATCH_CurrentDir].oldfunc,
	AROS_UFCA(BPTR, lock,    D1),
	AROS_UFCA(APTR, libbase, A6));

    if (patches[PATCH_CurrentDir].enabled)
    {
	LONG err = IoErr();
	struct FileInfoBlock *fib = NULL;
	if (lock)
	{
	    fib = AllocDosObject(DOS_FIB, NULL);
	    if (fib)
	    {
		if (Examine(lock, fib))
		{
		    lockpath = fib->fib_FileName;
		}
	    }
	}
	main_output("CurrentDir", lockpath, 0, TRUE);

	if (fib) FreeDosObject(DOS_FIB, fib);
	SetIoErr(err);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(BOOL, New_DeleteFile,
    AROS_UFHA(CONST_STRPTR, name,    D1),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // true means deleting was OK
    BOOL result = AROS_UFC2(BOOL, patches[PATCH_DeleteFile].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,    D1),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_DeleteFile].enabled)
    {
	main_output("Delete", name, 0, result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(LONG, New_DeleteVar,
    AROS_UFHA(CONST_STRPTR, name,    D1),
    AROS_UFHA(ULONG ,       flags,   D2),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // true means variable was deleted
    LONG result = AROS_UFC3(LONG, patches[PATCH_DeleteVar].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,    D1),
	AROS_UFCA(ULONG ,       flags,   D2),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_DeleteVar].enabled)
    {
	CONST_STRPTR opt;
	if      (flags & GVF_GLOBAL_ONLY) opt = MSG(MSG_GLOBAL);
        else if ((flags & 7) == LV_VAR)   opt = MSG(MSG_LOCAL);
        else if ((flags & 7) == LV_ALIAS) opt = MSG(MSG_ALIAS);
        else                              opt = MSG(MSG_UNKNOWN);

	main_output("DeleteVar", name, opt, result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH4(LONG, New_Execute,
    AROS_UFHA(STRPTR, string,  D1),
    AROS_UFHA(BPTR,   input ,  D2),
    AROS_UFHA(BPTR,   output,  D3),
    AROS_UFHA(APTR,   libbase, A6))
{
    AROS_USERFUNC_INIT

    // true means command could be started
    LONG result = AROS_UFC4(LONG, patches[PATCH_Execute].oldfunc,
	AROS_UFCA(STRPTR, string,  D1),
	AROS_UFCA(BPTR,   input ,  D2),
	AROS_UFCA(BPTR,   output,  D3),
	AROS_UFCA(APTR,   libbase, A6));

    if (patches[PATCH_Execute].enabled)
    {
	main_output("Execute", string ,0 , result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(struct LocalVar *, New_FindVar,
    AROS_UFHA(CONST_STRPTR, name,    D1),
    AROS_UFHA(ULONG,        type,    D2),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // NULL means variable not found
    struct LocalVar *result = AROS_UFC3(struct LocalVar *, patches[PATCH_FindVar].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,    D1),
	AROS_UFCA(ULONG,        type,    D2),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_FindVar].enabled)
    {
	CONST_STRPTR opt;
	if      ((type & 7) == LV_VAR)   opt = MSG(MSG_LOCAL);
	else if ((type & 7) == LV_ALIAS) opt = MSG(MSG_ALIAS);
	else                             opt = MSG(MSG_UNKNOWN);

	main_output("FindVar", name, opt, (LONG)result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH5(LONG, New_GetVar,
    AROS_UFHA(CONST_STRPTR, name,    D1),
    AROS_UFHA(STRPTR,       buffer,  D2),
    AROS_UFHA(LONG,         size,    D3),
    AROS_UFHA(LONG,         flags,   D4),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // -1 means variable not defined
    LONG result = AROS_UFC5(LONG, patches[PATCH_GetVar].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,    D1),
	AROS_UFCA(STRPTR,       buffer,  D2),
	AROS_UFCA(LONG,         size,    D3),
	AROS_UFCA(LONG,         flags,   D4),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_GetVar].enabled)
    {
	CONST_STRPTR opt;
	if      (flags & GVF_GLOBAL_ONLY) opt = MSG(MSG_GLOBAL);
        else if ((flags & 7) == LV_ALIAS) opt = MSG(MSG_ALIAS);
        else if (flags & GVF_LOCAL_ONLY)  opt = MSG(MSG_LOCAL);
        else                              opt = MSG(MSG_ANY);

	main_output("GetVar", name, opt, result != -1);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(BPTR, New_LoadSeg,
    AROS_UFHA(CONST_STRPTR, name,    D1),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // 0 means load failed
    BPTR result = AROS_UFC2(BPTR, patches[PATCH_LoadSeg].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,    D1),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_LoadSeg].enabled)
    {
	main_output("LoadSeg", name, 0, (LONG)result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(BPTR, New_Lock,
    AROS_UFHA(CONST_STRPTR, name,       D1),
    AROS_UFHA(LONG,         accessMode, D2),
    AROS_UFHA(APTR,         libbase,    A6))
{
    AROS_USERFUNC_INIT

    // 0 means error
    BPTR result = AROS_UFC3(BPTR, patches[PATCH_Lock].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,       D1),
	AROS_UFCA(LONG,         accessMode, D2),
	AROS_UFCA(APTR,         libbase,    A6));

    if (patches[PATCH_Lock].enabled)
    {
	CONST_STRPTR opt;
	if      (accessMode == ACCESS_READ)  opt = MSG(MSG_READ);
	else if (accessMode == ACCESS_WRITE) opt = MSG(MSG_WRITE);
	else                                 opt = MSG(MSG_READ_ASK);

	CONST_STRPTR curname = name;
	if ( ! setup.showPaths &&  *curname == '\0')
	    curname = "\"\"";

	main_output("Lock", curname, opt, (LONG)result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH4(LONG, New_MakeLink,
    AROS_UFHA(STRPTR, name,    D1),
    AROS_UFHA(APTR,   dest,    D2),
    AROS_UFHA(LONG,   soft,    D3),
    AROS_UFHA(APTR,   libbase, A6))
{
    AROS_USERFUNC_INIT

    // result is boolean
    LONG result = AROS_UFC4(LONG, patches[PATCH_MakeLink].oldfunc,
	AROS_UFCA(STRPTR, name,    D1),
	AROS_UFCA(APTR,   dest,    D2),
	AROS_UFCA(LONG,   soft,    D3),
	AROS_UFCA(APTR,   libbase, A6));

    if (patches[PATCH_MakeLink].enabled)
    {
        //struct Process *myproc = (struct Process *)SysBase->ThisTask;

	CONST_STRPTR opt;
	if (soft) opt = "Softlink";
	else      opt = "Hardlink";

	//FIXME: MyNameFromLock crashes
#if 0
 
	int len = strlen(name);
	char namestr[MAX_STR_LEN + 1];
	if (len >= MAX_STR_LEN) {
	    strncpy(namestr, name, MAX_STR_LEN);
	    namestr[MAX_STR_LEN] = 0;
	} else {
	    if (setup.showPaths) {
		strcpy(namestr, MyNameFromLock(myproc->pr_CurrentDir,
			    name, namestr, MAX_STR_LEN-2));
		len = strlen(namestr);
	    } else
		strcpy(namestr, name);

	    strcat(namestr, " --> ");
	    if (soft) {
		strncat(namestr, (char *)dest, MAX_STR_LEN - len - 1);
		namestr[MAX_STR_LEN] = 0;
	    } else {
		strcat(namestr, MyNameFromLock((BPTR)dest, NULL, namestr+len+1,
			    MAX_STR_LEN-len-1));
	    }
	}
#endif
	main_output("MakeLink", name /*namestr */, opt, result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(BPTR, New_NewLoadSeg,
    AROS_UFHA(STRPTR,           file,    D1),
    AROS_UFHA(struct TagItem *, tags,    D2),
    AROS_UFHA(APTR,             libbase, A6))
{
    AROS_USERFUNC_INIT

    // 0 means load failed
    BPTR result = AROS_UFC3(BPTR, patches[PATCH_NewLoadSeg].oldfunc,
	AROS_UFCA(STRPTR,           file,    D1),
	AROS_UFCA(struct TagItem *, tags,    D2),
	AROS_UFCA(APTR,             libbase, A6));

    if (patches[PATCH_NewLoadSeg].enabled)
    {
	main_output("NewLoadSeg", file, 0, (LONG)result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3 (BPTR, New_Open,
    AROS_UFHA (CONST_STRPTR, name,       D1),
    AROS_UFHA (LONG,         accessMode, D2),
    AROS_UFHA (APTR,         libbase,    A6))

{
    AROS_USERFUNC_INIT

    // 0 means error
    BPTR result = AROS_UFC3(BPTR, patches[PATCH_Open].oldfunc,
	AROS_UFCA (CONST_STRPTR, name,       D1),
	AROS_UFCA (LONG,         accessMode, D2),
	AROS_UFCA (APTR,         libbase,    A6));

    if (patches[PATCH_Open].enabled)
    {
	CONST_STRPTR opt;
	if      (accessMode == MODE_OLDFILE)   opt = MSG(MSG_READ);
	else if (accessMode == MODE_NEWFILE)   opt = MSG(MSG_WRITE);
	else if (accessMode == MODE_READWRITE) opt = MSG(MSG_MODIFY);
	else                                   opt = MSG(MSG_UNKNOWN);

	main_output("Open", name, opt, (LONG)result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(LONG, New_Rename,
    AROS_UFHA(CONST_STRPTR, oldName, D1),
    AROS_UFHA(CONST_STRPTR, newName, D2),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // bool
    LONG result = AROS_UFC3(LONG, patches[PATCH_Rename].oldfunc,
	AROS_UFCA(CONST_STRPTR, oldName, D1),
	AROS_UFCA(CONST_STRPTR, newName, D2),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_Rename].enabled)
    {
	main_output("Rename", oldName, 0, result);
	main_output("to -->", newName, 0, result);
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH5(LONG, New_RunCommand,
    AROS_UFHA(BPTR,   segList,   D1),
    AROS_UFHA(ULONG,  stacksize, D2),
    AROS_UFHA(STRPTR, argptr,    D3),
    AROS_UFHA(ULONG,  argsize,   D4),
    AROS_UFHA(APTR,   libbase,   A6))
{
    AROS_USERFUNC_INIT

    // -1 means error
    LONG result = AROS_UFC5(LONG, patches[PATCH_RunCommand].oldfunc,
	AROS_UFCA(BPTR,   segList,   D1),
	AROS_UFCA(ULONG,  stacksize, D2),
	AROS_UFCA(STRPTR, argptr,    D3),
	AROS_UFCA(ULONG,  argsize,   D4),
	AROS_UFCA(APTR,   libbase,   A6));

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
	main_output("RunCommand", argstr, 0, result != -1);
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH5(BOOL, New_SetVar,
    AROS_UFHA(CONST_STRPTR, name,    D1),
    AROS_UFHA(CONST_STRPTR, buffer,  D2),
    AROS_UFHA(LONG,         size,    D3),
    AROS_UFHA(LONG,         flags,   D4),
    AROS_UFHA(void*,        libbase, A6))
{
    AROS_USERFUNC_INIT

    BOOL result = AROS_UFC5(BOOL, patches[PATCH_SetVar].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,    D1),
	AROS_UFCA(CONST_STRPTR, buffer,  D2),
	AROS_UFCA(LONG,         size,    D3),
	AROS_UFCA(LONG,         flags,   D4),
	AROS_UFCA(void*,        libbase, A6));

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
	main_output("SetVar", varstr, opt, result);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(LONG, New_SystemTagList,
    AROS_UFHA(CONST_STRPTR,     command, D1),
    AROS_UFHA(struct TagItem *, tags,    D2),
    AROS_UFHA(APTR,             libbase, A6))
{
    AROS_USERFUNC_INIT

    // -1 means error
    LONG result = AROS_UFC3(LONG, patches[PATCH_SystemTagList].oldfunc,
	AROS_UFCA(CONST_STRPTR,     command, D1),
	AROS_UFCA(struct TagItem *, tags,    D2),
	AROS_UFCA(APTR,             libbase, A6));

    if (patches[PATCH_SystemTagList].enabled)
    {
	char optstr[20];
	sprintf(optstr, "%ld", result);
	main_output("SystemTagList", command, optstr, result != -1);
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(struct MsgPort *, New_FindPort,
    AROS_UFHA(STRPTR, name,    A1),
    AROS_UFHA(APTR,   libbase, A6))
{
    AROS_USERFUNC_INIT

    // NULL means error
    struct MsgPort *result = AROS_UFC2(struct MsgPort *, patches[PATCH_FindPort].oldfunc,
	AROS_UFCA(STRPTR, name,    A1),
	AROS_UFCA(APTR,   libbase, A6));

    if (patches[PATCH_FindPort].enabled)
    {
	main_output("FindPort", name, 0, (LONG)result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(struct Resident *, New_FindResident,
    AROS_UFHA(const UBYTE *, name,    A1),
    AROS_UFHA(APTR,          libbase, A6))
{
    AROS_USERFUNC_INIT

    // NULL means error
    struct Resident *result = AROS_UFC2(struct Resident *, patches[PATCH_FindResident].oldfunc,
	AROS_UFCA(const UBYTE *, name,    A1),
	AROS_UFCA(APTR,          libbase, A6));

    if (patches[PATCH_FindResident].enabled)
    {
	main_output("FindResident", name, 0, (LONG)result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(struct SignalSemaphore *, New_FindSemaphore,
    AROS_UFHA(STRPTR, name,    A1),
    AROS_UFHA(APTR,   libbase, A6))
{
    AROS_USERFUNC_INIT

    // NULL means error
    struct SignalSemaphore *result = AROS_UFC2(struct SignalSemaphore *, patches[PATCH_FindSemaphore].oldfunc,
	AROS_UFCA(STRPTR, name,    A1),
	AROS_UFCA(APTR,   libbase, A6));

    if (patches[PATCH_FindSemaphore].enabled)
    {
	main_output("FindSemaphore", name, 0, (LONG)result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(struct Task *, New_FindTask,
    AROS_UFHA(STRPTR, name,    A1),
    AROS_UFHA(APTR,   libbase, A6))
{
    AROS_USERFUNC_INIT

    // NULL means error
    struct Task *result = AROS_UFC2(struct Task *, patches[PATCH_FindTask].oldfunc,
	AROS_UFCA(STRPTR, name,    A1),
	AROS_UFCA(APTR,   libbase, A6));

    if (patches[PATCH_FindTask].enabled)
    {
	main_output("FindTask", name, 0, (LONG)result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH5(BYTE, New_OpenDevice,
    AROS_UFHA(CONST_STRPTR,       devName,    A0),
    AROS_UFHA(ULONG,              unitNumber, D0),
    AROS_UFHA(struct IORequest *, iORequest,  A1),
    AROS_UFHA(ULONG,              flags,      D1),
    AROS_UFHA(APTR,               libbase,    A6))
{
    AROS_USERFUNC_INIT

    // 0 means OK
    BYTE result = AROS_UFC5(BYTE, patches[PATCH_OpenDevice].oldfunc,
	AROS_UFCA(CONST_STRPTR,       devName,    A0),
	AROS_UFCA(ULONG,              unitNumber, D0),
	AROS_UFCA(struct IORequest *, iORequest,  A1),
	AROS_UFCA(ULONG,              flags,      D1),
	AROS_UFCA(APTR,               libbase,    A6));

    if (patches[PATCH_OpenDevice].enabled)
    {
	char unitstr[20];
	sprintf(unitstr, "Unit %ld", unitNumber);
	main_output("OpenDevice", devName, unitstr, !result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(struct Library *, New_OpenLibrary,
    AROS_UFHA(CONST_STRPTR,  libName, A1),
    AROS_UFHA(ULONG,         version, D0),
    AROS_UFHA(APTR,          libbase, A6))
{
    AROS_USERFUNC_INIT

    // 0 means error
    struct Library *result = AROS_UFC3(struct Library *, patches[PATCH_OpenLibrary].oldfunc,
	AROS_UFCA(CONST_STRPTR,  libName, A1),
	AROS_UFCA(ULONG,         version, D0),
	AROS_UFCA(APTR,          libbase, A6));

    if (patches[PATCH_OpenLibrary].enabled)
    {
	char verstr[20];
	sprintf(verstr, MSG(MSG_VERSION), version);
	main_output("OpenLibrary", libName, verstr, (LONG)result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(APTR, New_OpenResource,
    AROS_UFHA(CONST_STRPTR, resName, A1),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // 0 means error
    APTR result = AROS_UFC2(APTR, patches[PATCH_OpenResource].oldfunc,
	AROS_UFCA(CONST_STRPTR, resName, A1),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_OpenResource].enabled)
    {
	main_output("OpenLibrary", resName, 0, (LONG)result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH2(struct Screen *, New_LockPubScreen,
    AROS_UFHA(CONST_STRPTR, name,    A0),
    AROS_UFHA(APTR,         libbase, A6))
{
    AROS_USERFUNC_INIT

    // 0 means error
    struct Screen *result = AROS_UFC2(struct Screen *, patches[PATCH_LockPubScreen].oldfunc,
	AROS_UFCA(CONST_STRPTR, name,    A0),
	AROS_UFCA(APTR,         libbase, A6));

    if (patches[PATCH_LockPubScreen].enabled)
    {
	main_output("LockPubScreen", name, 0, (LONG)result );
    }
    
    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------
    
AROS_UFH2(struct TextFont *, New_OpenFont,
    AROS_UFHA(struct TextAttr *, textAttr, A0),
    AROS_UFHA(APTR,              libbase,  A6))
{
    AROS_USERFUNC_INIT

    // 0 means error
    struct TextFont *result = AROS_UFC2(struct TextFont *, patches[PATCH_OpenFont].oldfunc,
	AROS_UFCA(struct TextAttr *, textAttr, A0),
	AROS_UFCA(APTR,              libbase,  A6));

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
	main_output("OpenFont", name, sizestr, (LONG)result );
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(UBYTE *, New_FindToolType,
    AROS_UFHA(CONST STRPTR *, toolTypeArray, A0),
    AROS_UFHA(CONST STRPTR,   typeName,      A1),
    AROS_UFHA(APTR,           libbase,       A6))
{
    AROS_USERFUNC_INIT

    // 0 means error
    UBYTE *result = AROS_UFC3(UBYTE *, patches[PATCH_FindToolType].oldfunc,
	AROS_UFCA(CONST STRPTR *, toolTypeArray, A0),
	AROS_UFCA(CONST STRPTR,   typeName,      A1),
	AROS_UFCA(APTR,           libbase,       A6));

    if (patches[PATCH_FindToolType].enabled)
    {
	main_output("FindToolType", typeName, 0, (LONG)result );
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

AROS_UFH3(BOOL, New_MatchToolValue,
    AROS_UFHA(UBYTE *, typeString, A0),
    AROS_UFHA(UBYTE *, value,      A1),
    AROS_UFHA(APTR,    libbase,    A6))
{
    AROS_USERFUNC_INIT

    BOOL result = AROS_UFC3(BOOL, patches[PATCH_MatchToolValue].oldfunc,
	AROS_UFCA(UBYTE *, typeString, A0),
	AROS_UFCA(UBYTE *, value,      A1),
	AROS_UFCA(APTR,    libbase,    A6));

    if (patches[PATCH_MatchToolValue].enabled)
    {
	main_output("MatchToolValue", typeString, value, result );
    }

    return result;

    AROS_USERFUNC_EXIT
}

// ----------------------------------------------------------------------------------

void patches_init(void)
{
    libbases[LIB_Exec]      = (struct Library*)SysBase;
    libbases[LIB_Dos]       = (struct Library*)DOSBase;
    libbases[LIB_Icon]      = IconBase;
    libbases[LIB_Intuition] = (struct Library*)IntuitionBase;
    libbases[LIB_Graphics]  = (struct Library*)GfxBase;

    patches[PATCH_CreateDir].newfunc      = (FP)&New_CreateDir;
    patches[PATCH_CurrentDir].newfunc     = (FP)&New_CurrentDir;
    patches[PATCH_DeleteFile].newfunc     = (FP)&New_DeleteFile;
    patches[PATCH_DeleteVar].newfunc      = (FP)&New_DeleteVar;
    patches[PATCH_Execute].newfunc        = (FP)&New_Execute;
    patches[PATCH_FindVar].newfunc        = (FP)&New_FindVar;
    patches[PATCH_GetVar].newfunc         = (FP)&New_GetVar;
    patches[PATCH_LoadSeg].newfunc        = (FP)&New_LoadSeg;
    patches[PATCH_Lock].newfunc           = (FP)&New_Lock;
    patches[PATCH_MakeLink].newfunc       = (FP)&New_MakeLink;
    patches[PATCH_NewLoadSeg].newfunc     = (FP)&New_NewLoadSeg;
    patches[PATCH_Open].newfunc           = (FP)&New_Open;
    patches[PATCH_Rename].newfunc         = (FP)&New_Rename;
    patches[PATCH_RunCommand].newfunc     = (FP)&New_RunCommand;
    patches[PATCH_SetVar].newfunc         = (FP)&New_SetVar;
    patches[PATCH_SystemTagList].newfunc  = (FP)&New_SystemTagList;
    patches[PATCH_FindPort].newfunc       = (FP)&New_FindPort;
    patches[PATCH_FindResident].newfunc   = (FP)&New_FindResident;
    patches[PATCH_FindSemaphore].newfunc  = (FP)&New_FindSemaphore;
    patches[PATCH_FindTask].newfunc       = (FP)&New_FindTask;
    patches[PATCH_OpenDevice].newfunc     = (FP)&New_OpenDevice;
    patches[PATCH_OpenLibrary].newfunc    = (FP)&New_OpenLibrary;
    patches[PATCH_OpenResource].newfunc   = (FP)&New_OpenResource;
    patches[PATCH_LockPubScreen].newfunc  = (FP)&New_LockPubScreen;
    patches[PATCH_OpenFont].newfunc       = (FP)&New_OpenFont;
    patches[PATCH_FindToolType].newfunc   = (FP)&New_FindToolType;
    patches[PATCH_MatchToolValue].newfunc = (FP)&New_MatchToolValue;

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

