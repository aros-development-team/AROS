#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <devices/input.h>
#include <devices/rawkeycodes.h>
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
    if (kmsbase->pub.kms_SwitchQual != KMS_QUAL_DISABLE)
    {
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

    if (kmsbase->pub.kms_SwitchQual != KMS_QUAL_DISABLE)
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

AROS_UFH2(static struct InputEvent *, kms_InputHandler,
          AROS_UFHA(struct InputEvent * ,event, A0),
          AROS_UFHA(struct kms_base *, KMSBase,  A1))
{
    AROS_USERFUNC_INIT

    if (event->ie_Class == IECLASS_RAWKEY)
    {
	D(bug("[KMS] RAWKEY: qualifier 0x%04X, code 0x%04X\n", event->ie_Qualifier, event->ie_Code));

	/* 0x03FF masks out all mouse qualifiers. I'm too lazy to type them all by names. */
	if ((event->ie_Qualifier & 0x03FF) == kmsbase->pub.kms_SwitchQual)
	{
	    /*
	     * Qualifiers match. Now switch happens if:
	     * a) key codes match;
	     * b) kms_SwitchCode contains KMS_CODE_NOKEY and event code is any qualifier key.
	     * Without (b) sequence of pressing qualifier keys would matter, for example
	     * 'lshift + rshift' would be different from 'rshift + lshift',
	     */
	    if (((kmsbase->pub.kms_SwitchCode == KMS_CODE_NOKEY) &&
	         (event->ie_Code >= RAWKEY_LSHIFT) && (event->ie_Code <= RAWKEY_RAMIGA)) ||
	        (event->ie_Code == kmsbase->pub.kms_SwitchCode))
	    {
	    	/* Switch keymap and swallow the event */
	    	kmsbase->active = !kmsbase->active;
	    	return NULL;
	    }
	}
    }

    return event;

    AROS_USERFUNC_EXIT
}

static ULONG KMS_Init(struct kms_base *KMSBase)
{
    struct MsgPort *mp;
    struct IOStdReq *ioreq;
    LONG error = TRUE;

    KMSBase->kmr = OpenResource("keymap.resource");
    if (!KMSBase->kmr)
	return FALSE;

    mp = CreateMsgPort();
    if (!mp)
	return FALSE;

    ioreq = CreateIORequest(mp, sizeof(struct IOStdReq));
    if (ioreq)
    {
	error = OpenDevice("input.device", 0, (struct IORequest *)ioreq, 0);
	if (!error)
	{
	    /*
	     * Keymap switcher should work no matter what.
	     * So we use the highest possible priority.
	     */
	    KMSBase->input_Int.is_Node.ln_Name = "Keymap switcher";
	    KMSBase->input_Int.is_Node.ln_Pri  = 127;
	    KMSBase->input_Int.is_Code = (void (*)())kms_InputHandler;
	    KMSBase->input_Int.is_Data = KMSBase;

	    ioreq->io_Command = IND_ADDHANDLER;
	    ioreq->io_Data = &KMSBase->input_Int;

	    DoIO((struct IORequest *)ioreq);

	    D(bug("[KMS] Installed input handler\n"));
	    
	    CloseDevice((struct IORequest *)ioreq);
	}

	DeleteIORequest((struct IORequest *)ioreq);
    }
    DeleteMsgPort(mp);

    if (error)
	return FALSE;

    KMSBase->rom_MapRawKey = SetFunction(KeymapBase, (LONG)(-7 * LIB_VECTSIZE), AROS_SLIB_ENTRY(patch_MapRawKey, Kms));
    KMSBase->rom_MapANSI   = SetFunction(KeymapBase, (LONG)(-7 * LIB_VECTSIZE), AROS_SLIB_ENTRY(patch_MapRawKey, Kms));

    KMSBase->pub.kms_SwitchQual = KMS_QUAL_DISABLE;

    kmsbase = KMSBase;
    return TRUE;
}

ADD2INITLIB(KMS_Init, 0)
