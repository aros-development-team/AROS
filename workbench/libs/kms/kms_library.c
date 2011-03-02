#include <aros/symbolsets.h>
#include <libraries/kms.h>
#include <proto/exec.h>
#include <proto/keymap.h>

#include "kms_intern.h"

/* Global base pointer, for patch functions */
static struct kms_base *kmsbase;

AROS_LH4(WORD, patch_MapRawKey,
	 AROS_LHA(struct InputEvent *, event, A0),
	 AROS_LHA(STRPTR             , buffer, A1),
	 AROS_LHA(LONG               , length, D1),
	 AROS_LHA(struct KeyMap     *, keyMap, A2),
	 struct Library *, KeymapBase, 7, Kms)
{
    AROS_LIBFUNC_INIT

    /*
     * Just pass the call on if we are disabled.
     * Otherwise we can stick with alternate keymap
     * and no way to switch back to default one.
     */
    if (kmsbase->pub.kms_SwitchCode != KMS_DISABLE)
    {
	if ((event->ie_Class == IECLASS_RAWKEY) &&
	    (event->ie_Qualifier == kmsbase->pub.kms_SwitchQual) &&
	    (event->ie_Code      == kmsbase->pub.kms_SwitchCode))
	{
	    /* Process keymap switching - just flip the flag and exit */
	    kmsbase->active = !kmsbase->active;
	    return 0;
	}

	if (kmsbase->active)
	    keyMap = kmsbase->pub.kms_AltKeymap;
    }

    return AROS_CALL4(WORD, kmsbase->rom_MapRawKey,
		      AROS_LCA(struct InputEvent *, event, A0),
		      AROS_LCA(STRPTR             , buffer, A1),
		      AROS_LCA(LONG               , length, D1),
		      AROS_LCA(struct KeyMap     *, keyMap, A2),
		      struct Library *, KeymapBase);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(LONG, patch_MapANSI,
	 AROS_LHA(STRPTR         , string, A0),
	 AROS_LHA(LONG           , count, D0),
	 AROS_LHA(STRPTR         , buffer, A1),
	 AROS_LHA(LONG           , length, D1),
	 AROS_LHA(struct KeyMap *, keyMap, A2),
	 struct Library *, KeymapBase, 8, Kms)
{
    AROS_LIBFUNC_INIT

    if (kmsbase->pub.kms_SwitchCode != KMS_DISABLE)
    {
	if (kmsbase->active)
	    keyMap = kmsbase->pub.kms_AltKeymap;
    }

    return AROS_CALL5(LONG, kmsbase->rom_MapANSI,
		      AROS_LCA(STRPTR         , string, A0),
		      AROS_LCA(LONG           , count, D0),
		      AROS_LCA(STRPTR         , buffer, A1),
		      AROS_LCA(LONG           , length, D1),
		      AROS_LCA(struct KeyMap *, keyMap, A2),
		      struct Library *, KeymapBase);

    AROS_LIBFUNC_EXIT
}

static ULONG KMS_Init(struct kms_base *KMSBase)
{
    KMSBase->kmr = OpenResource("keymap.resource");
    if (!KMSBase->kmr)
	return FALSE;

    KMSBase->rom_MapRawKey = SetFunction(KeymapBase, -7 * LIB_VECTSIZE, AROS_SLIB_ENTRY(patch_MapRawKey, Kms));
    KMSBase->rom_MapANSI   = SetFunction(KeymapBase, -7 * LIB_VECTSIZE, AROS_SLIB_ENTRY(patch_MapRawKey, Kms));

    KMSBase->pub.kms_SwitchCode = KMS_DISABLE;

    kmsbase = KMSBase;
    return TRUE;
}

ADD2INITLIB(KMS_Init, 0)
