/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <prefs/prefhdr.h>
#include <prefs/input.h>
#include <devices/input.h>
#include <devices/keymap.h>

#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

struct FileInputPrefs
{
    char    ip_Keymap[16];
    UBYTE   ip_PointerTicks[2];
    UBYTE   ip_DoubleClick_secs[4];
    UBYTE   ip_DoubleClick_micro[4];
    UBYTE   ip_KeyRptDelay_secs[4];
    UBYTE   ip_KeyRptDelay_micro[4];
    UBYTE   ip_KeyRptSpeed_secs[4];
    UBYTE   ip_KeyRptSpeed_micro[4];
    UBYTE   ip_MouseAccel[2];
    UBYTE   ip_ClassicKeyboard[4];
    char    ip_KeymapName[64];
    UBYTE   ip_SwitchMouseButtons[4];
};

/*********************************************************************************************/

static LONG stopchunks[] =
{
    ID_PREF, ID_INPT,
    ID_PREF, ID_KMSW
};

/*********************************************************************************************/

static void SetInputPrefs(struct FileInputPrefs *prefs)
{
    struct KeyMapNode	  *kmn;
    struct Preferences	   p;
    struct IOStdReq	   ioreq;

    char *keymap = prefs->ip_KeymapName;

    if (!keymap[0])
	keymap = prefs->ip_Keymap;
    D(bug("[InputPrefs] Keymap name: %s\n", keymap));

    kmn = OpenKeymap(keymap);
    if (kmn)
	SetKeyMapDefault(&kmn->kn_KeyMap);

    GetPrefs(&p, sizeof(p));

    #define GETLONG(x) ((x[0] << 24) | (x[1] << 16) | (x[2] << 8) | x[3])
    #define GETWORD(x) ((x[0] << 8) | x[1])
    
    p.PointerTicks         = GETWORD(prefs->ip_PointerTicks);
    p.DoubleClick.tv_secs  = GETLONG(prefs->ip_DoubleClick_secs);
    p.DoubleClick.tv_micro = GETLONG(prefs->ip_DoubleClick_micro);
    p.KeyRptDelay.tv_secs  = GETLONG(prefs->ip_KeyRptDelay_secs);
    p.KeyRptDelay.tv_micro = GETLONG(prefs->ip_KeyRptDelay_micro);
    p.KeyRptSpeed.tv_secs  = GETLONG(prefs->ip_KeyRptSpeed_secs);
    p.KeyRptSpeed.tv_micro = GETLONG(prefs->ip_KeyRptSpeed_micro);
    if (GETWORD(prefs->ip_MouseAccel))
    {
    	p.EnableCLI |= MOUSE_ACCEL;
    }
    else
    {
    	p.EnableCLI &= ~MOUSE_ACCEL;
    }     
    
    SetPrefs(&p, sizeof(p), FALSE);

    memset(&ioreq, 0, sizeof(ioreq));
    ioreq.io_Message.mn_Length = sizeof(ioreq);

    if (!OpenDevice("input.device", 0, (struct IORequest *)&ioreq, 0))
    {
	struct InputDevice *InputBase = (struct InputDevice *)ioreq.io_Device;

	D(bug("[InputPrefs] Opened input.device v%d.%d\n", InputBase->id_Device.dd_Library.lib_Version,
	      InputBase->id_Device.dd_Library.lib_Revision));
	/* AROS input.device support this since v41.3 */
	if ((InputBase->id_Device.dd_Library.lib_Version >= 41) &&
	    (InputBase->id_Device.dd_Library.lib_Revision >= 3))
	{
	    InputBase->id_Flags = prefs->ip_SwitchMouseButtons[3] ? IDF_SWAP_BUTTONS : 0;
	    D(bug("[InputPrefs] Flags set to 0x%08lX\n", InputBase->id_Flags));
	}

	CloseDevice((struct IORequest *)&ioreq);
    }
}

/*********************************************************************************************/

void InputPrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;
    
    D(bug("In IPrefs:InputPrefs_Handler\n"));
    
    if ((iff = CreateIFF(filename, stopchunks, 2)))
    {
	/*
	 * Disable keymap switcher.
	 * It will stay disabled if the file does not contain KMSW chunk.
	 * This is done for backwards compatibility.
	 */
	KMSBase->kms_SwitchCode = KMS_DISABLE;

	while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
	{
	    struct ContextNode *cn;
	    struct FileInputPrefs *inputprefs;
    	    struct KMSPrefs *kmsprefs;

	    D(bug("InputPrefs_Handler: ParseIFF okay\n"));
	    cn = CurrentChunk(iff);

	    switch (cn->cn_ID)
	    {
	    case ID_INPT:
		D(bug("InputPrefs_Handler: INPT chunk\n"));

		inputprefs = LoadChunk(iff, sizeof(struct FileInputPrefs), MEMF_ANY);
		if (inputprefs)
		{
		    SetInputPrefs(inputprefs);
		    FreeVec(inputprefs);
		}
		break;

	    case ID_KMSW:
		D(bug("InputPrefs_Handler: KMSW chunk\n"));

		kmsprefs = LoadChunk(iff, sizeof(struct KMSPrefs), MEMF_ANY);
		if (kmsprefs)
		{
		    if (kmsprefs->kms_Enabled)
		    {
			struct KeyMapNode *alt_km = OpenKeymap(kmsprefs->kms_AltKeymap);

			if (alt_km)
			{
			    KMSBase->kms_SwitchQual = AROS_BE2WORD(kmsprefs->kms_SwitchQual);
			    KMSBase->kms_SwitchCode = AROS_BE2WORD(kmsprefs->kms_SwitchCode);
			    KMSBase->kms_AltKeymap  = &alt_km->kn_KeyMap;
			}
		    }

		    FreeVec(kmsprefs);
		}
		break;
	    }
	} /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */

   	KillIFF(iff);

    } /* if ((iff = CreateIFF(filename))) */
    
    
    D(bug("In IPrefs:InputPrefs_Handler. Done.\n", filename));
}

/*********************************************************************************************/
