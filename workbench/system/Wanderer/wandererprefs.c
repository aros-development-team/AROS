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
#include <proto/aros.h>
#include <aros/arosbase.h>
#include <aros/inquire.h>

#include <string.h>

#include "wandererprefs.h"
#include "../../libs/muimaster/classes/iconlist_attributes.h"
#include "iconwindow_attributes.h"
#include "support.h"
#include "locale.h"
#include "version.h"

#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>

/*** Instance Data **********************************************************/
struct WandererPrefs_DATA
{
    ULONG  wpd_NavigationMethod;
    ULONG  wpd_ToolbarEnabled;
    ULONG  wpd_ShowNetwork;
    ULONG  wpd_ShowUserFiles;
    ULONG  wpd_ScreenTitleString[IFF_CHUNK_BUFFER_SIZE];
	
    ULONG  wpd_IconListMode;
    ULONG  wpd_IconTextMode;
    ULONG  wpd_IconTextMaxLen;

    ULONG  wpd_MultiLine;
    ULONG  wpd_MultiLineOnFocus;
	
	struct List wpd_ViewSettings;
};

struct WandererPrefs_ViewSettingsNode
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
static unsigned char strtochar(STRPTR st)
{
    return *st++;
}

/******** code from workbench/c/Info.c *******************/
static void fmtlarge(UBYTE *buf, ULONG num)
{
    UQUAD d;
    UBYTE ch;
    struct
    {
        ULONG val;
        LONG  dec;
    } array =
    {
        num,
        0
    };

    if (num >= 1073741824)
    {
        array.val = num >> 30;
        d = ((UQUAD)num * 10 + 536870912) / 1073741824;
        array.dec = d % 10;
        //ch = 'G';
        ch = strtochar((STRPTR)_(MSG_MEM_G));
    }
    else if (num >= 1048576)
    {
        array.val = num >> 20;
        d = ((UQUAD)num * 10 + 524288) / 1048576;
        array.dec = d % 10;
        //ch = 'M';
        ch = strtochar((STRPTR)_(MSG_MEM_M));
    }
    else if (num >= 1024)
    {
        array.val = num >> 10;
        d = (num * 10 + 512) / 1024;
        array.dec = d % 10;
        //ch = 'K';
        ch = strtochar((STRPTR)_(MSG_MEM_K));
    }
    else
    {
        array.val = num;
        array.dec = 0;
        d = 0;
        //ch = 'B';
    ch = strtochar((STRPTR)_(MSG_MEM_B));
    }

    if (!array.dec && (d > array.val * 10))
    {
        array.val++;
    }

    RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);
    while (*buf) { buf++; }
    *buf++ = ch;
    *buf   = '\0';
}

/* Case-insensitive FindName()
 * code from workbench/c/Version.c
 */
static
struct Node *findname(struct List *list, CONST_STRPTR name)
{
	struct Node *node;

	ForeachNode(list, node)
	{
		if (!Stricmp(node->ln_Name, (STRPTR) name))
		{
			return node;
		}
	}

	return NULL;
}

/*ProcessUserScreenTitle(): pattern matching of user screentitle...;*/
STRPTR ProcessUserScreenTitle(STRPTR screentitlestr)
{
  /*Work in progress :-)
   */
   
D(bug("[Wanderer] ProcessUserScreenTitle(),EXTERN screentitlestr=%s\n", screentitlestr));   

    if  (screentitlestr==NULL)
	{
D(bug("[Wanderer] ProcessUserScreenTitle(),EXTERN screentitle=NULL\n"));   
		return screentitlestr;
	}

    int screentitleleng = strlen(screentitlestr);

    if (screentitleleng<1)
	{
D(bug("[Wanderer] ProcessUserScreenTitle(),EXTERN screentitleleng=%d\n", screentitleleng));   
		return screentitleleng;
	}

    STATIC char title[256];
    char temp[256], buffer[256];
    char infostr[10];  

    strcpy(temp,screentitlestr);


    int i;
    for (i=0; i<screentitleleng; i++)
    {
	if (temp[i]=='%')
	{

		if (screentitleleng>=3)
		{
			BOOL found=FALSE;

			if (strncmp(temp+i,"%wv",3)==0)
			{
				struct Library *MyLibrary;
				MyLibrary = (struct Library *) findname(&SysBase->LibList, "workbench.library");
		
				sprintf(infostr,"%ld",MyLibrary->lib_Version);
				sprintf(infostr+strlen(infostr),".");
				sprintf(infostr+strlen(infostr),"%ld",MyLibrary->lib_Revision);	
				found=TRUE;			
			}	

			if (strncmp(temp+i,"%ov",3)==0)
			{
				struct Library *AROSBase=OpenLibrary(AROSLIBNAME, AROSLIBVERSION);
				if (AROSBase!=NULL)
				{
					UWORD ver, kickrev;
					ArosInquire(AI_ArosVersion, (IPTR) &ver,
						    TAG_DONE
						   );
					sprintf(infostr,"%d", ver);
					CloseLibrary(AROSBase);
					found=TRUE;
				}				
			}	

			
			if (strncmp(temp+i,"%os",3)==0)
			{
				struct Library *AROSBase=OpenLibrary(AROSLIBNAME, AROSLIBVERSION);
				if (AROSBase!=NULL)
				{
					ULONG ver, rev;
					ArosInquire(AI_ArosReleaseMajor, (IPTR) &ver,
						    AI_ArosReleaseMinor,(IPTR) &rev,
						    TAG_DONE
						   );
					sprintf(infostr,"%ld", ver);
					sprintf(infostr+strlen(infostr),".");
					sprintf(infostr+strlen(infostr),"%ld",rev);
					CloseLibrary(AROSBase);
					found=TRUE;
				}				
			}	
			
			if (strncmp(temp+i,"%wb",3)==0)
			{
				struct Library *AROSBase=OpenLibrary(AROSLIBNAME, AROSLIBVERSION);
				if (AROSBase!=NULL)
				{
					ULONG ver, rev;
					ArosInquire(AI_ArosReleaseMajor, (IPTR) &ver,
						    AI_ArosReleaseMinor,(IPTR) &rev,
						    TAG_DONE
						   );
					sprintf(infostr,"%d", WANDERERVERS);
					sprintf(infostr+strlen(infostr),".");
					sprintf(infostr+strlen(infostr),"%d",WANDERERREV);
					CloseLibrary(AROSBase);
					found=TRUE;
				}				
			}	


			if (strncmp(temp+i,"%pc",3)==0)
			{
				fmtlarge(infostr,AvailMem(MEMF_CHIP));
				found=TRUE;

			}	

			if (strncmp(temp+i,"%pf",3)==0)
			{
				fmtlarge(infostr,AvailMem(MEMF_FAST));
				found=TRUE;
			}

			if (strncmp(temp+i,"%pt",3)==0)
			{
				fmtlarge(infostr,AvailMem(MEMF_ANY));
				found=TRUE;
			}

			if (strncmp(temp+i,"%PC",3)==0)
			{
				fmtlarge(infostr,AvailMem(MEMF_CHIP|MEMF_TOTAL));
				found=TRUE;

			}

			if (strncmp(temp+i,"%PF",3)==0)
			{
				fmtlarge(infostr,AvailMem(MEMF_FAST|MEMF_TOTAL));
				found=TRUE;
			}

			if (strncmp(temp+i,"%PT",3)==0)
			{
				fmtlarge(infostr,AvailMem(MEMF_ANY|MEMF_TOTAL));
				found=TRUE;
			}

			if (found)
			{

				temp[i+1]='s';
				temp[i+2]=' ';

				sprintf(title,temp, infostr);
			
				i=i+strlen(infostr);
				strncpy(buffer, title, i);
				strcpy(&buffer[i],&temp[(i+3)-strlen(infostr)]);
				strcpy(temp, buffer);

				screentitleleng=screentitleleng+strlen(infostr);
			}
			else
			{
				temp[i]='?';
				temp[i+1]='?';
				temp[i+2]='?';
				sprintf(title,temp);
			}
		}
		else
		{
			switch (screentitleleng)
			{
				case 2:
					temp[i]='?';
					temp[i+1]='?';
					break;
				case 1:
		  			temp[i]='?';
		  	}
			sprintf(title,temp);
		}
	}
	
    }
    sprintf(title,temp);
   		
    return title;

}

/*** Methods ****************************************************************/
Object *WandererPrefs__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
    
    if (self != NULL)
    {
		SETUP_INST_DATA;

		NewList(&data->wpd_ViewSettings);
        DoMethod(self, MUIM_WandererPrefs_Reload);
    }
    
    return self;
}

IPTR WandererPrefs__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    //SETUP_INST_DATA;
    
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
			case MUIA_IconWindowExt_NetworkBrowser_Show:
				data->wpd_ShowNetwork = (LONG)tag->ti_Data;
				break;

			case MUIA_IconWindowExt_UserFiles_ShowFilesFolder:
				data->wpd_ShowUserFiles = (LONG)tag->ti_Data;
				break;

			case MUIA_IconWindowExt_ScreenTitle_String:
				strcpy(data->wpd_ScreenTitleString,tag->ti_Data);
				//data->wpd_ScreenTitleString = (LONG)tag->ti_Data;
				break;

            case MUIA_IconWindowExt_Toolbar_Enabled:
                data->wpd_ToolbarEnabled = (LONG)tag->ti_Data;
			    break;

            case MUIA_IconWindowExt_Toolbar_NavigationMethod:
                data->wpd_NavigationMethod = (LONG)tag->ti_Data;
                break;

/* The Following attributes will be moved to the ViewSettings Specific Chunks */
			
            case MUIA_IconList_IconListMode:
                data->wpd_IconListMode = (LONG)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_Mode:
                data->wpd_IconTextMode = (ULONG)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_MaxLineLen:
                data->wpd_IconTextMaxLen = (ULONG)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_MultiLine:
                data->wpd_MultiLine = (ULONG)tag->ti_Data;
                break;

            case MUIA_IconList_LabelText_MultiLineOnFocus:
                data->wpd_MultiLineOnFocus = (ULONG)tag->ti_Data;
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
        case MUIA_IconWindowExt_NetworkBrowser_Show:
            *store = (IPTR)data->wpd_ShowNetwork;
            break;

        case MUIA_IconWindowExt_UserFiles_ShowFilesFolder:
            *store = (IPTR)data->wpd_ShowUserFiles;
            break;

	case MUIA_IconWindowExt_ScreenTitle_String:
            *store = (IPTR)data->wpd_ScreenTitleString;
D(bug("[WANDERER.PREFS] WandererPrefs__GET@@@@@@@@@: SCREENTITLE= %s\n", data->wpd_ScreenTitleString));
            break;

        case MUIA_IconWindowExt_Toolbar_NavigationMethod:
            *store = (IPTR)data->wpd_NavigationMethod;
            break;

        case MUIA_IconWindowExt_Toolbar_Enabled:
            *store = (IPTR)data->wpd_ToolbarEnabled;
            break;

        case MUIA_IconList_IconListMode:
            *store = (IPTR)data->wpd_IconListMode;
            break;

        case MUIA_IconList_LabelText_Mode:
            *store = (IPTR)data->wpd_IconTextMode;
            break;
            
        case MUIA_IconList_LabelText_MaxLineLen:
            *store = (IPTR)data->wpd_IconTextMaxLen;
            break;

		case MUIA_IconList_LabelText_MultiLine:
			*store = (IPTR)data->wpd_MultiLine;
			break;

		case MUIA_IconList_LabelText_MultiLineOnFocus:
			*store = (IPTR)data->wpd_MultiLineOnFocus;
			break;

		default:
            rv = DoSuperMethodA(CLASS, self, (Msg)message);
    }
    
    return rv;
}

BOOL WandererPrefs_ProccessGlobalChunk(Class *CLASS, Object *self, struct TagItem *global_chunk)
{
    //SETUP_INST_DATA;

	int i = 0;
	BOOL cont = TRUE;

D(bug("[WANDERER.PREFS] WandererPrefs_ProccessGlobalChunk()\n"));
#warning "TODO: fix problems with endian-ness?"

	for (i =0; i < WP_GLOBALTAGCOUNT; i++)
	{
		if (cont)
		{
			if ((IPTR)global_chunk[i].ti_Tag == TAG_DONE) cont = FALSE;
			else SET(self, (IPTR)global_chunk[i].ti_Tag, (IPTR)global_chunk[i].ti_Data);
		}
	}

	return TRUE;
}

BOOL WPEditor_ProccessNetworkChunk(Class *CLASS, Object *self, UBYTE *_viewSettings_Chunk)
{
    //SETUP_INST_DATA;

	struct TagItem *network_tags =(struct TagItem *) _viewSettings_Chunk;
	SET(self, network_tags[0].ti_Tag, network_tags[0].ti_Data);

	return TRUE;
}

BOOL WPEditor_ProccessScreenTitleChunk(Class *CLASS, Object *self, UBYTE *_ScreenTitle_Chunk)
{
    //SETUP_INST_DATA;

	
D(bug("[WANDERER.PREFS] WandererPrefs__ProccessScreenTitleChunk@@@@@@@@@: SCREENTITLE to write= %s\n", _ScreenTitle_Chunk));

	char *userscreentitle=	ProcessUserScreenTitle(_ScreenTitle_Chunk);
D(bug("[WANDERER.PREFS] WandererPrefs__ProccessScreenTitleChunk@@@@@@@@@: SCREENTITLE returned= %s\n", userscreentitle));

	if (userscreentitle==NULL)
		SET(self, MUIA_IconWindowExt_ScreenTitle_String,_ScreenTitle_Chunk);
	else	
		SET(self, MUIA_IconWindowExt_ScreenTitle_String,userscreentitle);
D(bug("[WANDERER.PREFS] WandererPrefs__ProccessScreenTitleChunk@@@@@@@@@: SCREENTITLE setted\n"));
	
	return TRUE;
}

struct WandererPrefs_ViewSettingsNode *WandererPrefs_FindViewSettingsNode(struct WandererPrefs_DATA *data, char *node_Name)
{
	struct WandererPrefs_ViewSettingsNode *current_Node = NULL;

	ForeachNode(&data->wpd_ViewSettings, current_Node)
	{
		if ((strcmp(current_Node->wpbn_Name, node_Name)) == 0) return current_Node;
	}
	return NULL;
}

BOOL WandererPrefs_ProccessViewSettingsChunk(Class *CLASS, Object *self, char *_viewSettings_ViewName, UBYTE *_viewSettings_Chunk, IPTR chunk_size)
{
    SETUP_INST_DATA;

D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk()\n"));
	BOOL                                 background_node_found = FALSE;
	struct WandererPrefs_ViewSettingsNode  *_viewSettings_Node = NULL;

	_viewSettings_Node = WandererPrefs_FindViewSettingsNode(data, _viewSettings_ViewName);

	if (_viewSettings_Node)
	{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: Updating Existing node @ %x\n", _viewSettings_Node));
		if (_viewSettings_Node->wpbn_Background) FreeVec(_viewSettings_Node->wpbn_Background);
	}
	else
	{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: Creating new node for '%s'\n", _viewSettings_ViewName));
		_viewSettings_Node = AllocMem(sizeof(struct WandererPrefs_ViewSettingsNode), MEMF_CLEAR|MEMF_PUBLIC);

		_viewSettings_Node->wpbn_Name = AllocVec(strlen(_viewSettings_ViewName) + 1, MEMF_CLEAR|MEMF_PUBLIC);
		strcpy(_viewSettings_Node->wpbn_Name, _viewSettings_ViewName);

		_viewSettings_Node->wpbn_NotifyObject = NotifyObject, End;

		AddTail(&data->wpd_ViewSettings, &_viewSettings_Node->wpbn_Node);
	}

	_viewSettings_Node->wpbn_Background =(IPTR) AllocVec(strlen(_viewSettings_Chunk) + 1, MEMF_CLEAR|MEMF_PUBLIC);
	strcpy((char *)_viewSettings_Node->wpbn_Background, _viewSettings_Chunk);
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: NAME BACKGROUND= %s\n",_viewSettings_Chunk));
	SET(_viewSettings_Node->wpbn_NotifyObject, MUIA_Background, _viewSettings_Chunk);
	
	if (chunk_size > (strlen(_viewSettings_Chunk) + 1))
	{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: Chunk has options Tag data ..\n"));
		UBYTE _viewSettings_TagOffset = ((strlen(_viewSettings_Chunk)  + 1)/4);

		if ((_viewSettings_TagOffset * 4) != (strlen(_viewSettings_Chunk)  + 1))
		{
			_viewSettings_TagOffset = (_viewSettings_TagOffset + 1) * 4;
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: String length unalined - rounding up (length %d, rounded %d) \n", strlen(_viewSettings_Chunk) + 1, _viewSettings_TagOffset ));
		}
		else
		{
			_viewSettings_TagOffset = _viewSettings_TagOffset * 4;
D(bug("[WPEditor] WPEditor_ProccessBackgroundChunk: String length doesnt need aligned (length %d) \n", strlen(_viewSettings_Chunk) + 1));
		}

		IPTR _viewSettings_TagCount  = ((chunk_size - _viewSettings_TagOffset)/sizeof(struct TagItem));

D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: %d Tags at offset %d ..\n", _viewSettings_TagCount, _viewSettings_TagOffset));

		if (_viewSettings_Node->wpbn_Options != NULL)
		{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: Freeing old background tag's @ %x\n", _viewSettings_Node->wpbn_Options));
			FreeVec(_viewSettings_Node->wpbn_Options);
			_viewSettings_Node->wpbn_Options = NULL;
		}
		
		_viewSettings_Node->wpbn_Options = AllocVec((_viewSettings_TagCount + 1) * sizeof(struct TagItem), MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: New tag storage @ %x\n", _viewSettings_Node->wpbn_Options));

		CopyMem(_viewSettings_Chunk + _viewSettings_TagOffset, _viewSettings_Node->wpbn_Options, (_viewSettings_TagCount) * sizeof(struct TagItem));
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: Tags copied to storage \n"));

		_viewSettings_Node->wpbn_Options[_viewSettings_TagCount].ti_Tag = TAG_DONE;

		int i = 0;
		for (i = 0; i < _viewSettings_TagCount; i++)
		{
D(bug("[WANDERER.PREFS] WandererPrefs_ProccessViewSettingsChunk: Setting Tag %x Value %d\n", _viewSettings_Node->wpbn_Options[i].ti_Tag, _viewSettings_Node->wpbn_Options[i].ti_Data));
			SET(_viewSettings_Node->wpbn_NotifyObject, _viewSettings_Node->wpbn_Options[i].ti_Tag, _viewSettings_Node->wpbn_Options[i].ti_Data);
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
    BOOL                   success = TRUE;
    LONG                   error;
    IPTR                   iff_parse_mode = IFFPARSE_SCAN;
    UBYTE                  chunk_buffer[IFF_CHUNK_BUFFER_SIZE];

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
					
				if ((error=ReadChunkBytes(handle, chunk_buffer, IFF_CHUNK_BUFFER_SIZE)))
				{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: ReadChunkBytes() Chunk matches Prefs Header size ..\n"));
					struct WandererPrefsIFFChunkHeader *this_header =(struct WandererPrefsIFFChunkHeader *) chunk_buffer;
					char                               *this_chunk_name = NULL;
					IPTR                               this_chunk_size = this_header->wpIFFch_ChunkSize;
						

					if ((this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,MEMF_ANY|MEMF_CLEAR)))
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

								error = ReadChunkBytes(
									 		handle, 
											chunk_buffer,
											this_chunk_size
										      );
									
								if (error == this_chunk_size)
								{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: ReadChunkBytes() Chunk matches Prefs Data size .. (%d)\n", error));
									if ((strcmp(this_chunk_name, "wanderer:global")) == 0)
									{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: Process data for wanderer global chunk ..\n"));
										WandererPrefs_ProccessGlobalChunk(CLASS, self,(struct TagItem *) chunk_buffer);
									}
									else if ((strcmp(this_chunk_name, "wanderer:network")) == 0)
									{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer network config chunk ..\n"));
										WPEditor_ProccessNetworkChunk(CLASS, self, chunk_buffer);
									}
									else if ((strcmp(this_chunk_name, "wanderer:screentitle")) == 0)
									{
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer screentitle config chunk ..size=%d\n", error));
D(bug("[WPEditor] WPEditor__MUIM_PrefsEditor_ImportFH: Process data for wanderer screentitle STRING= %s\n", chunk_buffer));
										WPEditor_ProccessScreenTitleChunk(CLASS, self, chunk_buffer);
									}

									else if ((strncmp(this_chunk_name, "wanderer:viewsettings", strlen("wanderer:viewsettings"))) == 0)
									{
										char *view_name = this_chunk_name + strlen("wanderer:viewsettings") + 1;
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: Process data for wanderer background chunk '%s'..\n", view_name));
										WandererPrefs_ProccessViewSettingsChunk(CLASS, self, view_name, chunk_buffer, this_chunk_size);
									}
								}//END if (error == this_chunk_size)	
								if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
								{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_Reload: End of Data chunk ..\n"));
								}
							}//END if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
						}//END if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)				
					}//END if ((this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,MEMF_ANY|MEMF_CLEAR)))
				}//END if ((error=ReadChunkBytes(handle, chunk_buffer, IFF_CHUNK_BUFFER_SIZE)))
			}
			else
			{
D(bug("[WANDERER.PREFS] ParseIFF() failed, returncode %ld!\n", error));
				//success = FALSE;
			}//END if ((error = ParseIFF(handle, iff_parse_mode)) == 0)

		} while (error != IFFERR_EOF);
		SET(self, MUIA_WandererPrefs_Processing, FALSE);
	}
	else
	{
D(bug("[WANDERER.PREFS] StopChunk() failed, returncode %ld!\n", error));
		//success = FALSE;
	}

        CloseIFF(handle);
    }
    else
    {
D(bug("[WANDERER.PREFS] Failed to open stream!, returncode %ld!\n", error));
        //ShowError(_(MSG_CANT_OPEN_STREAM));
	success = FALSE;
    }//END if ((error = StopChunk(handle, ID_PREF, ID_WANDR)) == 0)

    Close((APTR)handle->iff_Stream);

    FreeIFF(handle);
    
    return success;
}


IPTR WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject
(
    Class *CLASS, Object *self, struct MUIP_WandererPrefs_ViewSettings_GetNotifyObject *message
)
{
	SETUP_INST_DATA;
	struct WandererPrefs_ViewSettingsNode *current_Node = NULL;

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject()\n"));

    if ((current_Node = WandererPrefs_FindViewSettingsNode(data, message->Background_Name)))
	{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject: Returning Object for existing record\n"));
		return (IPTR) current_Node->wpbn_NotifyObject;
	}

	current_Node = AllocMem(sizeof(struct WandererPrefs_ViewSettingsNode), MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject: Created new node ..\n"));

	current_Node->wpbn_Name = AllocVec(strlen(message->Background_Name) + 1, MEMF_CLEAR|MEMF_PUBLIC);
	strcpy(current_Node->wpbn_Name, message->Background_Name);

	current_Node->wpbn_NotifyObject = NotifyObject, End;
	
	AddTail(&data->wpd_ViewSettings, &current_Node->wpbn_Node);

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject: Notify Object @ %x\n", current_Node->wpbn_NotifyObject));

	return (IPTR) current_Node->wpbn_NotifyObject;
}


IPTR WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute
(
    Class *CLASS, Object *self, struct MUIP_WandererPrefs_ViewSettings_GetAttribute *message
)
{
	SETUP_INST_DATA;
	struct WandererPrefs_ViewSettingsNode *current_Node = NULL;

D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute()\n"));

    if ((current_Node = WandererPrefs_FindViewSettingsNode(data, message->Background_Name)))
	{
D(bug("[WANDERER.PREFS] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute: Found Background Record ..\n"));
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
    MUIM_WandererPrefs_ViewSettings_GetNotifyObject,     struct MUIP_WandererPrefs_ViewSettings_GetNotifyObject *,
    MUIM_WandererPrefs_ViewSettings_GetAttribute,        struct MUIP_WandererPrefs_ViewSettings_GetAttribute *
);
