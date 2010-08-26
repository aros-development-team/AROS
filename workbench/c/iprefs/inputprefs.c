/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
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

#define DEBUG 1
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
    ID_PREF, ID_INPT
};

/*********************************************************************************************/

static struct KeyMapNode *KeymapAlreadyOpen(struct KeyMapResource *KeyMapResource, STRPTR name)
{
    struct KeyMapNode *kmn = NULL;
    struct Node       *node;
    
    Forbid();
    
    ForeachNode(&KeyMapResource->kr_List, node)
    {
	if (!stricmp(name, node->ln_Name))
	{
	    kmn = (struct KeyMapNode *)node;
	    break;
	}
    }
    
    Permit();
    
    return kmn;
}

/*********************************************************************************************/

static void SetInputPrefs(struct FileInputPrefs *prefs)
{
    struct KeyMapResource *KeyMapResource;
    struct KeyMapNode	  *kmn;
    struct Preferences	   p;
    struct IOStdReq	   ioreq;
    
    if ((KeyMapResource = OpenResource("keymap.resource")))
    {
	char *keymap = prefs->ip_KeymapName;
	
	if (!keymap[0])
		keymap = prefs->ip_Keymap;
	D(bug("[InputPrefs] Keymap name: %s\n", keymap));

    	kmn = KeymapAlreadyOpen(KeyMapResource, keymap);
	
	if (!kmn)
	{
	    struct KeyMapNode *kmn_check;
	    BPTR lock, olddir, seg;
	    
	    lock = Lock("DEVS:Keymaps", SHARED_LOCK);
	    if (lock)
	    {
	    	olddir = CurrentDir(lock);
		
		if ((seg = LoadSeg(keymap)))
		{
	    	    kmn = (struct KeyMapNode *) (((UBYTE *)BADDR(seg)) + sizeof(APTR));
		    
		    Forbid();
		    if ((kmn_check = KeymapAlreadyOpen(KeyMapResource, keymap)))
		    {
		    	kmn = kmn_check;
		    }
		    else
		    {
		    	AddHead(&KeyMapResource->kr_List, &kmn->kn_Node);
            	    	seg = 0;
		    }
		    Permit();
		 
		    if (seg) UnLoadSeg(seg);   
		}
		
		CurrentDir(olddir);
		
	    	UnLock(lock);
		
	    } /* if (lock) */
	    
	} /* if (!kmn) */
	
	if (kmn) SetKeyMapDefault(&kmn->kn_KeyMap);
	
    } /* if ((KeyMapResource = OpenResource("keymap.resource"))) */

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
    
    if ((iff = CreateIFF(filename, stopchunks, 1)))
    {
	while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
	{
	    struct ContextNode   *cn;
	    struct FileInputPrefs inputprefs;

	    cn = CurrentChunk(iff);

   	    D(bug("InputPrefs_Handler: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
		  cn->cn_Type >> 24,
		  cn->cn_Type >> 16,
		  cn->cn_Type >> 8,
		  cn->cn_Type,
		  cn->cn_ID >> 24,
		  cn->cn_ID >> 16,
		  cn->cn_ID >> 8,
		  cn->cn_ID));

	    if (cn->cn_ID == ID_INPT)
	    {
		ULONG size = cn->cn_Size;

		if (size > sizeof(inputprefs))
		    size = sizeof(inputprefs);

    	    	D(bug("InputPrefs_Handler: ID_INPT chunk found.\n"));		

		memset(&inputprefs, 0, sizeof(inputprefs));

		if (ReadChunkBytes(iff, &inputprefs, size) == size)
    	    	    SetInputPrefs(&inputprefs);
	    }
	} /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */
	    
   	KillIFF(iff);
	
    } /* if ((iff = CreateIFF(filename))) */
    
    
    D(bug("In IPrefs:InputPrefs_Handler. Done.\n", filename));
}

/*********************************************************************************************/
