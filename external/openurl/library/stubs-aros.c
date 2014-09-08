/* Automatically generated SDI stubs! Do not edit! */

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <aros/libcall.h>
#include <SDI_lib.h>

#include "lib_protos.h"

AROS_LH2(ULONG, URL_OpenA,
    AROS_LHA(STRPTR, ___url, A0),
    AROS_LHA(struct TagItem *, ___tags, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_OpenA, ___url, ___tags);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct URL_Prefs *, URL_OldGetPrefs,
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC_NP(URL_OldGetPrefs);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, URL_OldFreePrefs,
    AROS_LHA(struct URL_Prefs *, ___up, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_OldFreePrefs, ___up);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, URL_OldSetPrefs,
    AROS_LHA(struct URL_Prefs *, ___up, A0),
    AROS_LHA(BOOL, ___permanent, D0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_OldSetPrefs, ___up, ___permanent);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct URL_Prefs *, URL_OldGetDefaultPrefs,
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC_NP(URL_OldGetDefaultPrefs);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(ULONG, URL_OldLaunchPrefsApp,
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC_NP(URL_OldLaunchPrefsApp);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct URL_Prefs *, URL_GetPrefsA,
    AROS_LHA(struct TagItem *, ___tags, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_GetPrefsA, ___tags);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, URL_FreePrefsA,
    AROS_LHA(struct URL_Prefs *, ___prefs, A0),
    AROS_LHA(struct TagItem *, ___tags, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_FreePrefsA, ___prefs, ___tags);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, URL_SetPrefsA,
    AROS_LHA(struct URL_Prefs *, ___up, A0),
    AROS_LHA(struct TagItem *, ___tags, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_SetPrefsA, ___up, ___tags);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, URL_LaunchPrefsAppA,
    AROS_LHA(struct TagItem *, ___tags, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_LaunchPrefsAppA, ___tags);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, URL_GetAttr,
    AROS_LHA(ULONG, ___attr, D0),
    AROS_LHA(ULONG *, ___storage, A0),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(URL_GetAttr, ___attr, ___storage);
    AROS_LIBFUNC_EXIT
}


AROS_LH2(LONG, dispatch,
    AROS_LHA(struct RexxMsg *, ___msg, A0),
    AROS_LHA(STRPTR *, ___resPtr, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
    AROS_LIBFUNC_INIT
    return CALL_LFUNC(dispatch, ___msg, ___resPtr);
    AROS_LIBFUNC_EXIT
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
