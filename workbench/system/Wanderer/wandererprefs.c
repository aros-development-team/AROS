/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/
#define DEBUG 0
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG
#define IFF_CHUNK_BUFFER_SIZE 1024

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/iffparse.h>

#include <string.h>

#include "wandererprefs.h"
#include "support.h"

#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>

/*** Instance Data **********************************************************/
struct WandererPrefs_DATA
{
    ULONG  wpd_NavigationMethod;
    ULONG  wpd_ToolbarEnabled;
    ULONG  wpd_ShowNetwork;
    ULONG  wpd_ShowUserFiles;
	
    ULONG  wpd_IconListMode;
    ULONG  wpd_IconTextMode;
    ULONG  wpd_IconTextMaxLen;

    ULONG  wpd_MultiLine;
    ULONG  wpd_MultiLineSelected;
	
	struct List wpd_Backgrounds;
};

struct WandererPrefs_BackgroundNode
{
	struct Node    wpbn_Node;
	char           *wpbn_Name;
	IPTR		   wpbn_Background;
    struct TagItem *wpbn_Options;
	Object         *wpbn_NotifyObject;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WandererPrefs_DATA *data = INST_DATA(CLASS, self)

/*** Utility Functions ******************************************************/
BOOL SetString(STRPTR *dst, STRPTR src)
{
    if (src != NULL)
    {
        if (  (*dst == NULL)  ||  (strcmp(src, *dst) != 0)  )
        {
            STRPTR tmp = StrDup(src);
            
            if (tmp != NULL)
            {
                FreeVec(*dst);
                *dst = tmp;
                
                return TRUE;
            }
        }
    }
       
    return FALSE;
}

/*** Methods ****************************************************************/
Object *WandererPrefs__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
    
    if (self != NULL)
    {
		SETUP_INST_DATA;

		NewList(&data->wpd_Backgrounds);
        DoMethod(self, MUIM_WandererPrefs_Reload);
    }
    
    return self;
}

IPTR WandererPrefs__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    
    return DoSuperMethodA(CLASS, self, (Msg)message);
}

IPTR WandererPrefs__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    const struct TagItem *tstate = message->ops_AttrList;
    struct TagItem *tag;
    
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
			case MUIA_WandererPrefs_ShowNetworkBrowser:
				data->wpd_ShowNetwork = (LONG)tag->ti_Data;
				break;

			case MUIA_WandererPrefs_ShowUserFolder:
				data->wpd_ShowUserFiles = (LONG)tag->ti_Data;
				break;

            case MUIA_WandererPrefs_NavigationMethod:
                data->wpd_NavigationMethod = (LONG)tag->ti_Data;
                break;

            case MUIA_WandererPrefs_Toolbar_Enabled:
                data->wpd_ToolbarEnabled = (LONG)tag->ti_Data;
			    break;

            case MUIA_WandererPrefs_IconList_IconListMode:
                data->wpd_IconListMode = (LONG)tag->ti_Data;
                break;

            case MUIA_WandererPrefs_LabelText_Mode:
                data->wpd_IconTextMode = (ULONG)tag->ti_Data;
                break;

            case MUIA_WandererPrefs_LabelText_MaxLineLen:
                data->wpd_IconTextMaxLen = (ULONG)tag->ti_Data;
                break;

            case MUIA_WandererPrefs_LabelText_MultiLine:
                data->wpd_MultiLine = (ULONG)tag->ti_Data;
                break;

            case MUIA_WandererPrefs_LabelText_OnlySelectedMultiLine:
                data->wpd_MultiLineSelected = (ULONG)tag->ti_Data;
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg)message);
}

IPTR WandererPrefs__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        case MUIA_WandererPrefs_ShowNetworkBrowser:
            *store = (IPTR)data->wpd_ShowNetwork;
            break;

        case MUIA_WandererPrefs_ShowUserFolder:
            *store = (IPTR)data->wpd_ShowUserFiles;
            break;

        case MUIA_WandererPrefs_NavigationMethod:
            *store = (IPTR)data->wpd_NavigationMethod;
            break;

        case MUIA_WandererPrefs_Toolbar_Enabled:
            *store = (IPTR)data->wpd_ToolbarEnabled;
            break;

        case MUIA_WandererPrefs_IconList_IconListMode:
            *store = (IPTR)data->wpd_IconListMode;
            break;

        case MUIA_WandererPrefs_LabelText_Mode:
            *store = (IPTR)data->wpd_IconTextMode;
            break;
            
        case MUIA_WandererPrefs_LabelText_MaxLineLen:
            *store = (IPTR)data->wpd_IconTextMaxLen;
            break;

		case MUIA_WandererPrefs_LabelText_MultiLine:
			*store = (IPTR)data->wpd_MultiLine;
			break;

		case MUIA_WandererPrefs_LabelText_OnlySelectedMultiLine:
			*store = (IPTR)data->wpd_MultiLineSelected;
			break;

		default:
            rv = DoSuperMethodA(CLASS, self, (Msg)message);
    }
    
    return rv;
}

BOOL WandererPrefs_ProccessGlobalChunk(Class *CLASS, Object *self, struct TagItem *global_chunk)
{
    SETUP_INST_DATA;

	int i = 0;
	BOOL cont = TRUE;

D(bug("[WANDERER.PREFS] WandererPrefs_ProccessGlobalChunk()\n"));
#warning "TODO: fix problems with endian-ness?"

	for (i =0; i < WP_GLOBALTAGCOUNT; i++)
	{
		if (cont)
		{
			if ((IPTR)global_chunk[i].ti_Tag == TAG_DONE) cont = FALSE;
			SET(self, (IPTR)global_chunk[i].ti_Tag, (IPTR)global_chunk[i].ti_Data);
		}
	}

	return TRUE;
}

BOOL WPEditor_ProccessNetworkChunk(Class *CLASS, Object *self, UBYTE *background_chunk)
{
    SETUP_INST_DATA;

	struct TagItem *network_tags = background_chunk;
	SET(self, network_tags[0].ti_Tag, network_tags[0].ti_Data);

	return TRUE;
}

struct WandererPrefs_BackgroundNode *WandererPrefs_FindBackgroundNode(struct WandererPrefs_DATA *data, char *node_Name)
{
	struct WandererPrefs_BackgroundNode *current_Node = NULL;

	ForeachNode(&data->wpd_Backgrounds, current_Node)
	{
		if ((strcmp(current_Node->wpbn_Name, node_Name)) == 0) return current_Node;
	}
	return NULL;
}

BOOL WandererPrefs_ProccessBackgroundChunk(Class *CLASS, Object *self, char *background_name, UBYTE *background_chunk, IPTR chunk_size)
{
    SETUP_INST_DATA;

D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk()\n"));
	BOOL                                 background_node_found = FALSE;
	struct WandererPrefs_BackgroundNode  *background_Node = NULL;

	background_Node = WandererPrefs_FindBackgroundNode(data, background_name);

	if (background_Node)
	{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: Updating Existing node @ %x\n", background_Node));
		if (background_Node->wpbn_Background) FreeVec(background_Node->wpbn_Background);
	}
	else
	{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: Creating new node for '%s'\n", background_name));
		background_Node = AllocMem(sizeof(struct WandererPrefs_BackgroundNode), MEMF_CLEAR|MEMF_PUBLIC);

		background_Node->wpbn_Name = AllocVec(strlen(background_name) + 1, MEMF_CLEAR|MEMF_PUBLIC);
		strcpy(background_Node->wpbn_Name, background_name);

		background_Node->wpbn_NotifyObject = NotifyObject, End;

		AddTail(&data->wpd_Backgrounds, &background_Node->wpbn_Node);
	}

	background_Node->wpbn_Background = AllocVec(strlen(background_chunk) + 1, MEMF_CLEAR|MEMF_PUBLIC);
	strcpy(background_Node->wpbn_Background, background_chunk);

	SET(background_Node->wpbn_NotifyObject, MUIA_Background, background_chunk);
	
	if (chunk_size > (strlen(background_chunk) + 1))
	{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: Chunk has options Tag data ..\n"));
		UBYTE bgtag_offset = ((strlen(background_chunk)  + 1)/4);

		if ((bgtag_offset * 4) != (strlen(background_chunk)  + 1))
		{
			bgtag_offset = (bgtag_offset + 1) * 4;
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: String length unalined - rounding up (length %d, rounded %d) \n", strlen(background_chunk) + 1, bgtag_offset ));
		}
		else
		{
			bgtag_offset = bgtag_offset * 4;
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: String length doesnt need aligned (length %d) \n", strlen(background_chunk) + 1));
		}

		IPTR bgtag_count  = ((chunk_size - bgtag_offset)/sizeof(struct TagItem));

D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: %d Tags at offset %d ..\n", bgtag_count, bgtag_offset));

		if (background_Node->wpbn_Options != NULL)
		{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: Freeing old background tag's @ %x\n", background_Node->wpbn_Options));
			FreeVec(background_Node->wpbn_Options);
			background_Node->wpbn_Options = NULL;
		}
		
		background_Node->wpbn_Options = AllocVec((bgtag_count + 1) * sizeof(struct TagItem), MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: New tag storage @ %x\n", background_Node->wpbn_Options));

		CopyMem(background_chunk + bgtag_offset, background_Node->wpbn_Options, (bgtag_count) * sizeof(struct TagItem));
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: Tags copied to storage \n"));

		background_Node->wpbn_Options[bgtag_count].ti_Tag = TAG_DONE;

		int i = 0;
		for (i = 0; i < bgtag_count; i++)
		{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessBackgroundChunk: Setting Tag %x Value %d\n", background_Node->wpbn_Options[i].ti_Tag, background_Node->wpbn_Options[i].ti_Data));
			SET(background_Node->wpbn_NotifyObject, background_Node->wpbn_Options[i].ti_Tag, background_Node->wpbn_Options[i].ti_Data);
		}
	}
	return TRUE;
}

IPTR WandererPrefs__MUIM_WandererPrefs_Reload
(
    Class *CLASS, Object *self, Msg message
)
{
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    BOOL                    success = TRUE;
    LONG                    error;
	IPTR                    iff_parse_mode = IFFPARSE_SCAN;
	
	UBYTE                    chunk_buffer[IFF_CHUNK_BUFFER_SIZE];

    if (!(handle = AllocIFF()))
        return FALSE;
    
    handle->iff_Stream = (IPTR) Open("ENV:SYS/Wanderer.prefs", MODE_OLDFILE); 

    if (!handle->iff_Stream) return FALSE;
    
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
		if ((error = StopChunk(handle, ID_PREF, ID_WANDR)) == 0)
		{
			SET(self, MUIA_WandererPrefs_Processing, TRUE);
			do
			{				
				if ((error = ParseIFF(handle, iff_parse_mode)) == 0)
				{
					context = CurrentChunk(handle);
					iff_parse_mode = IFFPARSE_STEP;

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: Context %x\n", context));
					
					error = ReadChunkBytes
					(
						handle, chunk_buffer, IFF_CHUNK_BUFFER_SIZE
					);
					
					if (error == sizeof(struct WandererPrefsIFFChunkHeader))
					{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: ReadChunkBytes() Chunk matches Prefs Header size ..\n"));
						struct WandererPrefsIFFChunkHeader *this_header = chunk_buffer;
						char                               *this_chunk_name = NULL;
						IPTR                               this_chunk_size = this_header->wpIFFch_ChunkSize;
						
						if (this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,MEMF_CLEAR|MEMF_PUBLIC))
						{
							strcpy(this_chunk_name, this_header->wpIFFch_ChunkType);
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: Prefs Header for '%s' data size %d bytes\n", this_chunk_name, this_chunk_size));

							if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
							{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: End of header chunk ..\n"));

								if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
								{
									context = CurrentChunk(handle);

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: Context %x\n", context));

									error = ReadChunkBytes
									(
										handle, chunk_buffer, this_chunk_size
									);
									
									if (error == this_chunk_size)
									{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: ReadChunkBytes() Chunk matches Prefs Data size .. (%d)\n", error));
										if ((strcmp(this_chunk_name, "wanderer:global")) == 0)
										{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: Process data for wanderer global chunk ..\n"));
											WandererPrefs_ProccessGlobalChunk(CLASS, self, chunk_buffer);
										}
										else if ((strcmp(this_chunk_name, "wanderer:network")) == 0)
										{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer network config chunk ..\n"));
											WPEditor_ProccessNetworkChunk(CLASS, self, chunk_buffer);
										}
										else if ((strncmp(this_chunk_name, "wanderer:background", strlen("wanderer:background"))) == 0)
										{
											char *bg_name = this_chunk_name + strlen("wanderer:background") + 1;
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: Process data for wanderer background chunk '%s'..\n", bg_name));
											WandererPrefs_ProccessBackgroundChunk(CLASS, self, bg_name, chunk_buffer, this_chunk_size);
										}
									}	
									if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
									{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: End of Data chunk ..\n"));
									}
								}
							}				
						}
					}
				}
				else
				{
D(bug("[WANDERER.PREFS] ParseIFF() failed, returncode %ld!\n", error));
					success = FALSE;
				}

			} while (error != IFFERR_EOF);
			SET(self, MUIA_WandererPrefs_Processing, FALSE);
		}
		else
		{
D(bug("[WANDERER.PREFS] StopChunk() failed, returncode %ld!\n", error));
			success = FALSE;
		}

        CloseIFF(handle);
    }
    else
    {
D(bug("[WANDERER.PREFS] Failed to open stream!, returncode %ld!\n", error));
        //ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    Close((APTR)handle->iff_Stream);

    FreeIFF(handle);
    
    return FALSE;
}

IPTR WandererPrefs__MUIM_WandererPrefs_Background_GetNotifyObject
(
    Class *CLASS, Object *self, struct MUIP_WandererPrefs_Background_GetNotifyObject *message
)
{
	SETUP_INST_DATA;
	struct WandererPrefs_BackgroundNode *current_Node = NULL;

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Background_GetNotifyObject()\n"));

    if (current_Node = WandererPrefs_FindBackgroundNode(data, message->Background_Name))
	{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Background_GetNotifyObject: Returning Object for existing record\n"));
		return current_Node->wpbn_NotifyObject;
	}

	current_Node = AllocMem(sizeof(struct WandererPrefs_BackgroundNode), MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Background_GetNotifyObject: Created new node ..\n"));

	current_Node->wpbn_Name = AllocVec(strlen(message->Background_Name) + 1, MEMF_CLEAR|MEMF_PUBLIC);
	strcpy(current_Node->wpbn_Name, message->Background_Name);

	current_Node->wpbn_NotifyObject = NotifyObject, End;
	
	AddTail(&data->wpd_Backgrounds, &current_Node->wpbn_Node);

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Background_GetNotifyObject: Notify Object @ %x\n", current_Node->wpbn_NotifyObject));

	return current_Node->wpbn_NotifyObject;
}


IPTR WandererPrefs__MUIM_WandererPrefs_Background_GetAttribute
(
    Class *CLASS, Object *self, struct MUIP_WandererPrefs_Background_GetAttribute *message
)
{
	SETUP_INST_DATA;
	struct WandererPrefs_BackgroundNode *current_Node = NULL;

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Background_GetAttribute()\n"));

    if (current_Node = WandererPrefs_FindBackgroundNode(data, message->Background_Name))
	{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Background_GetAttribute: Found Background Record ..\n"));
		if (message->AttributeID == MUIA_Background)
		{
			if (current_Node->wpbn_Background) return current_Node->wpbn_Background;
		}
		else if (current_Node->wpbn_Options)
		{
			return GetTagData(message->AttributeID, (IPTR)-1, current_Node->wpbn_Options);
		}
	}
	return -1;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_7
(
    WandererPrefs, NULL, MUIC_Notify, NULL,
    OM_NEW,                                              struct opSet *,
    OM_DISPOSE,                                          Msg,
    OM_SET,                                              struct opSet *,
    OM_GET,                                              struct opGet *,
    MUIM_WandererPrefs_Reload,                           Msg,
    MUIM_WandererPrefs_Background_GetNotifyObject,       struct MUIP_WandererPrefs_Background_GetNotifyObject *,
    MUIM_WandererPrefs_Background_GetAttribute,          struct MUIP_WandererPrefs_Background_GetAttribute *
);
