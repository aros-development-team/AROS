/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
    struct MsgPort  	*inputmp;
    struct timerequest  *inputio;
    struct KeyMapResource *KeyMapResource;
    struct KeyMapNode	  *kmn;

    if ((KeyMapResource = OpenResource("keymap.resource")))
    {
    	kmn = KeymapAlreadyOpen(KeyMapResource, prefs->ip_Keymap);
	
	if (!kmn)
	{
	    struct KeyMapNode *kmn_check;
	    BPTR lock, olddir, seg;
	    
	    lock = Lock("DEVS:Keymaps", SHARED_LOCK);
	    if (lock)
	    {
	    	olddir = CurrentDir(lock);
		
		if ((seg = LoadSeg(prefs->ip_Keymap)))
		{
	    	    kmn = (struct KeyMapNode *) (((UBYTE *)BADDR(seg)) + sizeof(APTR));
		    
		    Forbid();
		    if ((kmn_check = KeymapAlreadyOpen(KeyMapResource, prefs->ip_Keymap)))
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

    if ((inputmp = CreateMsgPort()))
    {
    	if ((inputio = (struct timerequest *)CreateIORequest(inputmp, sizeof(struct IOStdReq))))
	{
	    if (!OpenDevice("input.device", 0, (struct IORequest *)inputio, 0))
	    {
	    	/* Set repeat rate */
		
	    	inputio->tr_node.io_Command = IND_SETPERIOD;
		
		inputio->tr_time.tv_secs = (prefs->ip_KeyRptSpeed_secs[0] << 24) |
		    	    	    	   (prefs->ip_KeyRptSpeed_secs[1] << 16) |
					   (prefs->ip_KeyRptSpeed_secs[2] << 8) |
					   (prefs->ip_KeyRptSpeed_secs[3]);
					  
		inputio->tr_time.tv_micro = (prefs->ip_KeyRptSpeed_micro[0] << 24) |
		    	    	    	    (prefs->ip_KeyRptSpeed_micro[1] << 16) |
					    (prefs->ip_KeyRptSpeed_micro[2] << 8) |
					    (prefs->ip_KeyRptSpeed_micro[3]);
					    
	    	DoIO((struct IORequest *)inputio);
		
		/* Set repeat delay */

	    	inputio->tr_node.io_Command = IND_SETTHRESH;
		
		inputio->tr_time.tv_secs = (prefs->ip_KeyRptDelay_secs[0] << 24) |
		    	    	    	   (prefs->ip_KeyRptDelay_secs[1] << 16) |
					   (prefs->ip_KeyRptDelay_secs[2] << 8) |
					   (prefs->ip_KeyRptDelay_secs[3]);

		inputio->tr_time.tv_micro = (prefs->ip_KeyRptDelay_micro[0] << 24) |
		    	    	    	    (prefs->ip_KeyRptDelay_micro[1] << 16) |
					    (prefs->ip_KeyRptDelay_micro[2] << 8) |
					    (prefs->ip_KeyRptDelay_micro[3]);
					  
	    	DoIO((struct IORequest *)inputio);

	    	CloseDevice((struct IORequest *)inputio);
		
	    } /* if (!OpenDevice("input.device", 0, (struct IORequest *)inputio, 0)) */
	    
	    DeleteIORequest((struct IORequest *)inputio);
	    
	} /* if ((inputio = (struct IoStdReq *)CreateIORequest(inputmp, sizeof(*inputio)))) */
	
	DeleteMsgPort(inputmp);
	
    } /* if ((inputmp = CreateMsgPort())) */
    
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

	    if ((cn->cn_ID == ID_INPT) && (cn->cn_Size == sizeof(inputprefs)))
	    {
    	    	D(bug("InputPrefs_Handler: ID_INPT chunk with correct size found.\n"));

		if (ReadChunkBytes(iff, &inputprefs, sizeof(inputprefs)) == sizeof(inputprefs))
		{
    	    	    SetInputPrefs(&inputprefs);
		    
 		} /* if (ReadChunkBytes(iff, &inputprefs, sizeof(inputprefs)) == sizeof(inputprefs)) */
		
	    } /* if ((cn->cn_ID == ID_INPT) && (cn->cn_Size == sizeof(inputprefs))) */

	} /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */
	    
   	KillIFF(iff);
	
    } /* if ((iff = CreateIFF(filename))) */
    
    
    D(bug("In IPrefs:InputPrefs_Handler. Done.\n", filename));
}

/*********************************************************************************************/
