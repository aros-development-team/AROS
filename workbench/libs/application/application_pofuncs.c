/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: PrefsObjects functions.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <string.h>

#include "application_intern.h"
#include "application_po.h"
#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
#include <proto/application.h>

        AROS_LH3(PrefsObject *, PrefsBaseObjectA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, obj, A0),
        AROS_LHA(ULONG *, error, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 18, Application)

/*  FUNCTION
        Run methods on any PrefsObject. Only the ALPO_ methods common to all
        types apply; it cannot allocate.

    INPUTS
        obj   - the object, or NULL for the ALPO_Alloc methods.
        error - NULL, or where to store the ALPOEF_ flags of the methods
                that failed; 0 if all succeeded.
        tags  - the methods, run in order. Methods without a parameter
                take 0.

    RESULT
        obj, or the object an Alloc or ALPO_Copy method made; NULL after
        ALPO_Release freed it.

    SEE ALSO
        <libraries/application.h>

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return po_Dispatch(ALPOT_None, obj, error, tags);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(PrefsObject *, PrefsStringA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, obj, A0),
        AROS_LHA(ULONG *, error, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 19, Application)

/*  FUNCTION
        Run methods on a string: ALPO_ and ALPOSTR_ methods.

    INPUTS
        obj   - the object, or NULL for the ALPO_Alloc methods.
        error - NULL, or where to store the ALPOEF_ flags of the methods
                that failed; 0 if all succeeded.
        tags  - the methods, run in order. Methods without a parameter
                take 0.

    RESULT
        obj, or the object an Alloc or ALPO_Copy method made; NULL after
        ALPO_Release freed it.

    SEE ALSO
        <libraries/application.h>

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return po_Dispatch(ALPOT_String, obj, error, tags);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(PrefsObject *, PrefsNumberA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, obj, A0),
        AROS_LHA(ULONG *, error, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 20, Application)

/*  FUNCTION
        Run methods on a number (LONG, BOOL or double): ALPO_ and ALPONUM_
        methods.

    INPUTS
        obj   - the object, or NULL for the ALPO_Alloc methods.
        error - NULL, or where to store the ALPOEF_ flags of the methods
                that failed; 0 if all succeeded.
        tags  - the methods, run in order. Methods without a parameter
                take 0.

    RESULT
        obj, or the object an Alloc or ALPO_Copy method made; NULL after
        ALPO_Release freed it.

    SEE ALSO
        <libraries/application.h>

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return po_Dispatch(ALPOT_Number, obj, error, tags);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(PrefsObject *, PrefsDateA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, obj, A0),
        AROS_LHA(ULONG *, error, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 21, Application)

/*  FUNCTION
        Run methods on a date: ALPO_ and ALPODAT_ methods.

    INPUTS
        obj   - the object, or NULL for the ALPO_Alloc methods.
        error - NULL, or where to store the ALPOEF_ flags of the methods
                that failed; 0 if all succeeded.
        tags  - the methods, run in order. Methods without a parameter
                take 0.

    RESULT
        obj, or the object an Alloc or ALPO_Copy method made; NULL after
        ALPO_Release freed it.

    SEE ALSO
        <libraries/application.h>

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return po_Dispatch(ALPOT_Date, obj, error, tags);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(PrefsObject *, PrefsBinaryA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, obj, A0),
        AROS_LHA(ULONG *, error, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 22, Application)

/*  FUNCTION
        Run methods on binary data: ALPO_ and ALPOBIN_ methods. Prefer the
        other types, which can be extended later.

    INPUTS
        obj   - the object, or NULL for the ALPO_Alloc methods.
        error - NULL, or where to store the ALPOEF_ flags of the methods
                that failed; 0 if all succeeded.
        tags  - the methods, run in order. Methods without a parameter
                take 0.

    RESULT
        obj, or the object an Alloc or ALPO_Copy method made; NULL after
        ALPO_Release freed it.

    SEE ALSO
        <libraries/application.h>

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return po_Dispatch(ALPOT_Binary, obj, error, tags);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(PrefsObject *, PrefsArrayA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, obj, A0),
        AROS_LHA(ULONG *, error, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 23, Application)

/*  FUNCTION
        Run methods on an array, objects by index: ALPO_ and ALPOARR_
        methods.

    INPUTS
        obj   - the object, or NULL for the ALPO_Alloc methods.
        error - NULL, or where to store the ALPOEF_ flags of the methods
                that failed; 0 if all succeeded.
        tags  - the methods, run in order. Methods without a parameter
                take 0.

    RESULT
        obj, or the object an Alloc or ALPO_Copy method made; NULL after
        ALPO_Release freed it.

    NOTES
        The Add, Insert and Replace methods take over the caller's
        reference.
        Storing the object already held at the index drops one reference only if
        the object has more than one, since a borrowed pointer and a retained
        one cannot be told apart: an object also held elsewhere must be
        retained before it is stored over itself.

    SEE ALSO
        <libraries/application.h>

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return po_Dispatch(ALPOT_Array, obj, error, tags);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(PrefsObject *, PrefsDictionaryA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, obj, A0),
        AROS_LHA(ULONG *, error, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 24, Application)

/*  FUNCTION
        Run methods on a dictionary, objects by key: ALPO_ and ALPODICT_
        methods.

    INPUTS
        obj   - the object, or NULL for the ALPO_Alloc methods.
        error - NULL, or where to store the ALPOEF_ flags of the methods
                that failed; 0 if all succeeded.
        tags  - the methods, run in order. Methods without a parameter
                take 0.

    RESULT
        obj, or the object an Alloc or ALPO_Copy method made; NULL after
        ALPO_Release freed it.

    NOTES
        The Set, Insert and Replace methods take over the caller's
        reference.
        Storing the object already held for key drops one reference only if
        the object has more than one, since a borrowed pointer and a retained
        one cannot be told apart: an object also held elsewhere must be
        retained before it is stored over itself.

    SEE ALSO
        <libraries/application.h>

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return po_Dispatch(ALPOT_Dictionary, obj, error, tags);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(PrefsObject *, DictGetObjectForKey,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(CONST_STRPTR, key, A1),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 25, Application)

/*  FUNCTION
        The object stored in a dictionary for a key, or NULL. The
        dictionary keeps owning it.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    if (po_Type(dict) != ALPOT_Dictionary)
        return NULL;
    return po_DictGet(dict, key);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(BOOL, DictGetBoolForKey,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(CONST_STRPTR, key, A1),
        AROS_LHA(BOOL, defBool, D0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 26, Application)

/*  FUNCTION
        The number stored for a key as a BOOL, or defBool if dict is NULL
        or holds no number for key.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PObj *o;

    (void)AppBase;
    if (po_Type(dict) != ALPOT_Dictionary || po_Type((PrefsObject *)(o = PO(po_DictGet(dict, key)))) != ALPOT_Number)
        return defBool;
    return (o->u.num.type == ALPONUMT_Double) ? (o->u.num.d != 0) : (o->u.num.l != 0);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, DictGetIntegerForKey,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(CONST_STRPTR, key, A1),
        AROS_LHA(LONG, defInt, D0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 27, Application)

/*  FUNCTION
        The number stored for a key as a LONG, or defInt if dict is NULL
        or holds no number for key.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    if (po_Type(dict) != ALPOT_Dictionary)
        return defInt;
    return po_DictLong(dict, key, defInt);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(CONST_STRPTR, DictGetStringForKey,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(CONST_STRPTR, key, A1),
        AROS_LHA(CONST_STRPTR, defStr, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 28, Application)

/*  FUNCTION
        The string stored for a key, or defStr if dict is NULL or holds no
        string for key. The string belongs to the dictionary.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    if (po_Type(dict) != ALPOT_Dictionary)
        return defStr;
    return po_DictString(dict, key, defStr);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, DictGetOptionForKey,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(CONST_STRPTR, key, A1),
        AROS_LHA(CONST_STRPTR *, optionsTable, A2),
        AROS_LHA(LONG, defaultOption, D0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 29, Application)

/*  FUNCTION
        Look the string stored for key up in a NULL-terminated table,
        ignoring case.

    RESULT
        Its index in the table, or defaultOption if dict, the table or the
        string is missing or the string is not in the table.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR s;
    LONG i;

    (void)AppBase;
    if (!optionsTable || po_Type(dict) != ALPOT_Dictionary || !(s = po_DictString(dict, key, NULL)))
        return defaultOption;
    for (i = 0; optionsTable[i]; i++)
    {
        if (Stricmp(optionsTable[i], s) == 0)
            return i;
    }
    return defaultOption;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(BOOL, DictSetObjectForKey,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(PrefsObject *, obj, A1),
        AROS_LHA(CONST_STRPTR, key, A2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 30, Application)

/*  FUNCTION
        Store obj in the dictionary for key, releasing what was stored for
        key before. The dictionary takes over the caller's reference.

    RESULT
        FALSE if dict, obj or key is NULL or memory ran out; obj then
        still belongs to the caller.

    NOTES
        Storing the object already held for key drops one reference only if
        the object has more than one, since a borrowed pointer and a retained
        one cannot be told apart: an object also held elsewhere must be
        retained before it is stored over itself.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    if (po_Type(dict) != ALPOT_Dictionary)
        return FALSE;
    return po_DictSet(dict, key, obj);

    AROS_LIBFUNC_EXIT
}

/* The prefs file of an application, or NULL. */
static STRPTR pf_AppFile(struct ApplicationBase *AppBase, ULONG appID, STRPTR buf)
{
    struct AppEntry *ae;
    STRPTR name = NULL;

    ObtainSemaphore(&AppBase->ab_Lock);
    if ((ae = app_Find(AppBase, appID)) && ae->ae_PrefsName)
    {
        strncpy(buf, ae->ae_PrefsName, APP_PATHMAX - 1);
        buf[APP_PATHMAX - 1] = '\0';
        name = buf;
    }
    ReleaseSemaphore(&AppBase->ab_Lock);
    return name;
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, ReadPrefsA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 31, Application)

/*  FUNCTION
        Empty a dictionary and fill it from a prefs file or a stream.

    INPUTS
        dict - the dictionary.
        tags - READPREFS_AppID (that application's prefs file),
               READPREFS_FileName (a name below ENV:, or a full path) or
               READPREFS_GetProcInfo (a stream). A file is read from ENV:,
               or from ENVARC: if that fails; READPREFS_ReadENV and
               READPREFS_ReadENVARC switch either off.

    RESULT
        0, or ALPOEF_ flags. dict is left empty on failure.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ALPOGetProcInfo *pi = (struct ALPOGetProcInfo *)GetTagData(READPREFS_GetProcInfo, 0, tags);
    CONST_STRPTR name = (CONST_STRPTR)GetTagData(READPREFS_FileName, 0, tags);
    ULONG appID = GetTagData(READPREFS_AppID, 0, tags);
    TEXT buf[APP_PATHMAX];

    if (pi)
        return xml_ReadStream(dict, pi);
    if (!name && appID && !(name = pf_AppFile(AppBase, appID, buf)))
        return ALPOEF_NOTFOUND;
    return prefs_Read(AppBase, dict, name,
                      GetTagData(READPREFS_ReadENV, TRUE, tags),
                      GetTagData(READPREFS_ReadENVARC, TRUE, tags));

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, WritePrefsA,

/*  SYNOPSIS */
        AROS_LHA(PrefsObject *, dict, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 32, Application)

/*  FUNCTION
        Write a dictionary to a prefs file or a stream.

    INPUTS
        dict - the dictionary.
        tags - WRITEPREFS_AppID, WRITEPREFS_FileName or
               WRITEPREFS_PutProcInfo, as for ReadPrefsA(). A file is
               written to ENV: (WRITEPREFS_WriteENV, default TRUE) and to
               ENVARC: (WRITEPREFS_WriteENVARC, default FALSE).

    RESULT
        0, or ALPOEF_ flags.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ALPOPutProcInfo *pi = (struct ALPOPutProcInfo *)GetTagData(WRITEPREFS_PutProcInfo, 0, tags);
    CONST_STRPTR name = (CONST_STRPTR)GetTagData(WRITEPREFS_FileName, 0, tags);
    ULONG appID = GetTagData(WRITEPREFS_AppID, 0, tags);
    TEXT buf[APP_PATHMAX];

    if (pi)
        return xml_WriteStream(dict, pi);
    if (!name && appID && !(name = pf_AppFile(AppBase, appID, buf)))
        return ALPOEF_NOTFOUND;
    return prefs_Write(AppBase, dict, name,
                       GetTagData(WRITEPREFS_WriteENV, TRUE, tags),
                       GetTagData(WRITEPREFS_WriteENVARC, FALSE, tags));

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, BeginDeserialization,

/*  SYNOPSIS */
        AROS_LHA(struct ALPOGetProcInfo *, info, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 33, Application)

/*  FUNCTION
        Read a stream up to its first object's open tag, for reading
        PrefsObjects XML by hand: allocate an object of the type returned
        and ALPO_Deserialize it with the same info.

    INPUTS
        info - getChProc, unGetChProc, encoding, and key_otag pointing to
               TAGBUFFER_SIZE bytes.

    RESULT
        The ALPOT_ type of the first object, or ALPOT_None.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    (void)AppBase;
    return xml_Begin(info);

    AROS_LIBFUNC_EXIT
}
