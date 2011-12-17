/*
  Copyright  2004-2010, The AROS Development Team. All rights reserved.
  $Id$
*/

#define ZCC_QUIET

#include "portable_macros.h"
#ifdef __AROS__
#define DEBUG 0
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG
#endif

#define IFF_CHUNK_BUFFER_SIZE 1024

#include <exec/types.h>
#include <libraries/mui.h>

#ifdef __AROS__
#include <zune/customclasses.h>
#else
#include <zune_AROS/customclasses.h>
#endif

#include <proto/workbench.h>

#include <proto/utility.h>

#include <proto/dos.h>

#ifdef __AROS__
#include <proto/alib.h>
#endif

#include <proto/iffparse.h>

#ifdef __AROS__
#include <proto/aros.h>
#endif

#include <aros/arosbase.h>
#include <aros/inquire.h>

#include <string.h>
#include <stdio.h>


#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#ifndef _PROTO_INTUITION_H
#include <proto/intuition.h>
#endif

#include <proto/muimaster.h>

#include "wanderer.h"
#include "wandererprefs.h"
#include "Classes/iconlist_attributes.h"
#include "iconwindow_attributes.h"
#include "support.h"
#include "locale.h"
#include "version.h"

#ifdef __AROS__
#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>
#else
#include <prefs_AROS/prefhdr.h>
#include <prefs_AROS/wanderer.h>
#endif


#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

static CONST_STRPTR                     wandererPrefs_PrefsFile = "ENV:SYS/Wanderer/global.prefs";

struct TagItem32 {
    ULONG ti_Tag;
    ULONG ti_Data;
};

/*** Instance Data **********************************************************/
struct WandererPrefs_DATA
{
    ULONG                       wpd_NavigationMethod;
    ULONG                       wpd_ToolbarEnabled;
    ULONG                       wpd_ShowNetwork;
    ULONG                       wpd_ShowUserFiles;
    ULONG                       wpd_ScreenTitleString[IFF_CHUNK_BUFFER_SIZE];

    struct List                 wpd_ViewSettings;

    struct NotifyRequest        wpd_PrefsNotifyRequest;
    struct Wanderer_FSHandler   wdp_PrefsFSHandler;
    
    BOOL                        wpd_PROCESSING;
};

struct WandererPrefs_ViewSettingsNode
{
    struct Node                 wpbn_Node;
    char                        *wpbn_Name;
    IPTR                        wpbn_Background;
    struct TagItem32            *wpbn_Options;
    Object                      *wpbn_NotifyObject;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WandererPrefs_DATA *data = INST_DATA(CLASS, self)

/*** Utility Functions ******************************************************/
///SetString()
BOOL SetString(STRPTR *dst, STRPTR src)
{
    if (src != NULL)
    {
        if ((*dst == NULL) || (strcmp(src, *dst) != 0))
        {
            STRPTR tmp =(STRPTR) StrDup(src);

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
///

///strtochar()
static unsigned char strtochar(STRPTR st)
{
    return *st++;
}
///
/******** code from workbench/c/Info.c **************************************/
///fmtlarge()
static void fmtlarge(UBYTE *buf, ULONG num)
{
  UQUAD d;
  UBYTE ch;
  struct
  {
    IPTR val;
    IPTR  dec;
  } array =
  {
    num,
    0
  };

  if (num >= 1073741824)
  {
    //Gigabytes
    array.val = num >> 30;
    d = ((UQUAD)num * 10 + 536870912) / 1073741824;
    array.dec = d % 10;
    ch = strtochar((STRPTR)_(MSG_MEM_G));
  }
  else if (num >= 1048576)
  {
    //Megabytes
    array.val = num >> 20;
    d = ((UQUAD)num * 10 + 524288) / 1048576;
    array.dec = d % 10;
    ch = strtochar((STRPTR)_(MSG_MEM_M));
  }
  else if (num >= 1024)
  {
    //Kilobytes
    array.val = num >> 10;
    d = (num * 10 + 512) / 1024;
    array.dec = d % 10;
    ch = strtochar((STRPTR)_(MSG_MEM_K));
  }
  else
  {
    //Bytes
    array.val = num;
    array.dec = 0;
    d = 0;
    ch = strtochar((STRPTR)_(MSG_MEM_B));
  }

  if (!array.dec && (d > array.val * 10))
  {
    array.val++;
  }

  RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);

  while (*buf)
  {
    buf++;
  }

  *buf++ = ch;
  *buf   = '\0';
}
///

///findname()
/* Case-insensitive FindName()
 * code from workbench/c/Version.c
 */
static
struct Node *findname(struct List *list, CONST_STRPTR name)
{
  struct Node *node;
  #ifdef __AROS__
  ForeachNode(list, node)
  #else
  Foreach_Node(list, node);
  #endif
  {
    if (!Stricmp(node->ln_Name, (STRPTR) name))
    {
      return node;
    }
  }

  return NULL;
}
///

///ProcessUserScreenTitle()
/*pattern matching of user screentitle...;*/
STRPTR ProcessUserScreenTitle(STRPTR screentitle_Template)
{
/* Work in progress :-) */
  int screentitle_TemplateLen;
  STATIC char title[256];
  char temp[256], buffer[256];
  char infostr[10];
  int screentitle_curChar;

  if (screentitle_Template == NULL)
  {
D(bug("[Wanderer:Prefs] ProcessUserScreenTitle(),EXTERN screentitle = NULL\n"));   
    return screentitle_Template;
  }
  else
  {
D(bug("[Wanderer:Prefs] ProcessUserScreenTitle('%s')\n", screentitle_Template));
  }

  screentitle_TemplateLen = strlen(screentitle_Template);

  if (screentitle_TemplateLen > sizeof(temp)-1)
  {
D(bug("[Wanderer:Prefs] ProcessUserScreenTitle: EXTERN screentitle_TemplateLen = %d\n", screentitle_TemplateLen));   
    return (STRPTR)NULL;
  }
  
  strcpy(temp, screentitle_Template);
  
  for (screentitle_curChar = 0; screentitle_curChar < screentitle_TemplateLen; screentitle_curChar++)
  {
    if (temp[screentitle_curChar]=='%')
    {
      if (screentitle_TemplateLen >= 3)
      {
        BOOL found = FALSE;

        if (strncmp(temp + screentitle_curChar, "%wv", 3) == 0)
        {
          struct Library *MyLibrary = NULL;

        #ifdef __AROS__
          MyLibrary = (struct Library *)findname(&SysBase->LibList, "workbench.library");
          //workbench.library is just opened, what is the sense of this istruction?
        #else
          MyLibrary = WorkbenchBase;
        #endif

          sprintf(infostr, "%ld.%ld",(long int) MyLibrary->lib_Version,(long int) MyLibrary->lib_Revision);
          found = TRUE;
        }

        if (strncmp(temp + screentitle_curChar, "%ov", 3) == 0)
        {
          struct Library *AROSBase = OpenLibrary(AROSLIBNAME, AROSLIBVERSION);

          if (AROSBase!=NULL)
          {
            UWORD ver = 0;
            //UWORD    kickrev = 0;

            ArosInquire
              (
                AI_ArosVersion, (IPTR)&ver,
                TAG_DONE
              );
            sprintf(infostr, "%d", ver);
            CloseLibrary(AROSBase);
            found = TRUE;
          }
        }

        if (strncmp(temp + screentitle_curChar, "%os", 3) == 0)
        {
          struct Library *AROSBase = OpenLibrary(AROSLIBNAME, AROSLIBVERSION);

          if (AROSBase != NULL)
          {
            ULONG ver = 0,
                rev = 0;

            ArosInquire
              (
                AI_ArosReleaseMajor, (IPTR)&ver,
                AI_ArosReleaseMinor, (IPTR)&rev,
                TAG_DONE
              );
            sprintf(infostr, "%d.%d", (int)ver, (int)rev);
            CloseLibrary(AROSBase);
            found = TRUE;
          }
        }

        if (strncmp(temp + screentitle_curChar, "%wb", 3) == 0)
        {
          struct Library *AROSBase = OpenLibrary(AROSLIBNAME, AROSLIBVERSION);

          if (AROSBase != NULL)
          {
            ULONG ver = 0;
            ULONG    rev = 0;

            ArosInquire
              (
                AI_ArosReleaseMajor, (IPTR)&ver,
                AI_ArosReleaseMinor, (IPTR)&rev,
                TAG_DONE
              );
            sprintf(infostr, "%d.%d", WANDERERVERS, WANDERERREV);
            CloseLibrary(AROSBase);
            found = TRUE;
          }
        }

        if (strncmp(temp + screentitle_curChar, "%pc", 3) == 0)
        {
          fmtlarge(infostr, AvailMem(MEMF_CHIP));
          found = TRUE;
        }

        if (strncmp(temp + screentitle_curChar, "%pf", 3) == 0)
        {
          fmtlarge(infostr, AvailMem(MEMF_FAST));
          found = TRUE;
        }

        if (strncmp(temp + screentitle_curChar, "%pt", 3) == 0)
        {
          fmtlarge(infostr, AvailMem(MEMF_ANY));
          found = TRUE;
        }

        if (strncmp(temp + screentitle_curChar, "%PC", 3) == 0)
        {
          fmtlarge(infostr, AvailMem(MEMF_CHIP|MEMF_TOTAL));
          found = TRUE;
        }

        if (strncmp(temp + screentitle_curChar, "%PF", 3) == 0)
        {
          fmtlarge(infostr, AvailMem(MEMF_FAST|MEMF_TOTAL));
          found = TRUE;
        }

        if (strncmp(temp + screentitle_curChar, "%PT", 3) == 0)
        {
          fmtlarge(infostr, AvailMem(MEMF_ANY|MEMF_TOTAL));
          found = TRUE;
        }

        if (found)
        {
          temp[screentitle_curChar + 1] = 's';
          temp[screentitle_curChar + 2] = ' ';

          sprintf(title, temp, infostr);

          screentitle_curChar = screentitle_curChar + strlen(infostr);
          strncpy(buffer, title, screentitle_curChar);
          strcpy(&buffer[screentitle_curChar], &temp[(screentitle_curChar + 3) - strlen(infostr)]);
          strcpy(temp, buffer);

          screentitle_TemplateLen = screentitle_TemplateLen + strlen(infostr);
        }
        else
        {
          temp[screentitle_curChar] = '?';
          temp[screentitle_curChar + 1] = '?';
          temp[screentitle_curChar + 2] = '?';
          strcpy(title, temp);
        }
      }
      else
      {
        switch (screentitle_TemplateLen)
        {
          case 2:
            temp[screentitle_curChar]= '?';
            temp[screentitle_curChar + 1]= '?';
            break;
          case 1:
            temp[screentitle_curChar] = '?';
        }
        strcpy(title, temp);
      }
    }
  }
  strcpy(title, temp);

  return title;
}
///

///ExpandEnvName()
/* Expand a passed in env: string to its full location */
/* Wanderer doesnt free this mem at the moment but should 
   incase it is every closed */
static CONST_STRPTR ExpandEnvName(CONST_STRPTR env_path)
{
    BOOL     ok = FALSE;
    char     tmp_envbuff[1024];
    STRPTR   fullpath = NULL;
    BPTR     env_lock = (BPTR) NULL;

    env_lock = Lock("ENV:", SHARED_LOCK);
    if (env_lock)
    {
        if (NameFromLock(env_lock, tmp_envbuff, 256)) ok = TRUE;
        UnLock(env_lock);
    }
    
    if (ok)
    {
        if ((fullpath = AllocVec(strlen(tmp_envbuff) + strlen(env_path) + 1 + 1 - 4, MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
        {
            strcpy(fullpath, tmp_envbuff);
            AddPart(fullpath, env_path + 4, 1019);
            return fullpath;
        }     
    }

    //We couldnt expand it so just use as is ..
    return env_path;
}
///


IPTR WandererPrefs__HandleFSUpdate(Object *prefs, struct NotifyMessage *msg)
{
    DoMethod(prefs, MUIM_WandererPrefs_Reload);
    return 0;
}

/*** Methods ****************************************************************/

///OM_NEW()
Object *WandererPrefs__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    IPTR                _wandererPrefs__FSNotifyPort = 0;
    D(bug("[Wanderer:Prefs]:New()\n"));

    _wandererPrefs__FSNotifyPort = GetTagData(MUIA_Wanderer_FileSysNotifyPort, (IPTR) NULL, message->ops_AttrList);

    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
 
    if (self != NULL)
    {
        SETUP_INST_DATA;

        /* Setup notification on prefs file --------------------------------*/
        if (_wandererPrefs__FSNotifyPort != 0)
        {
            data->wdp_PrefsFSHandler.target                         = self;
            data->wdp_PrefsFSHandler.fshn_Node.ln_Name              = (STRPTR)ExpandEnvName(wandererPrefs_PrefsFile);
            data->wdp_PrefsFSHandler.HandleFSUpdate                 = WandererPrefs__HandleFSUpdate;
            data->wpd_PrefsNotifyRequest.nr_Name                    = data->wdp_PrefsFSHandler.fshn_Node.ln_Name;
            data->wpd_PrefsNotifyRequest.nr_Flags                   = NRF_SEND_MESSAGE;
            data->wpd_PrefsNotifyRequest.nr_stuff.nr_Msg.nr_Port    = (struct MsgPort *)_wandererPrefs__FSNotifyPort;
            data->wpd_PrefsNotifyRequest.nr_UserData                = (IPTR)&data->wdp_PrefsFSHandler;

            if (StartNotify(&data->wpd_PrefsNotifyRequest))
            {
                D(bug("[Wanderer:Prefs] Wanderer__OM_NEW: Prefs-notification setup on '%s'\n", data->wpd_PrefsNotifyRequest.nr_Name));
            }
            else
            {
                D(bug("[Wanderer:Prefs] Wanderer__OM_NEW: FAILED to setup Prefs-notification!\n"));
                data->wdp_PrefsFSHandler.fshn_Node.ln_Name = NULL;
                data->wpd_PrefsNotifyRequest.nr_Name = NULL;
            }
        }
        D(bug("[Wanderer:Prefs]:New - reloading\n"));

        NewList(&data->wpd_ViewSettings);

        data->wpd_PROCESSING = FALSE;

        DoMethod(self, MUIM_WandererPrefs_Reload);
    }

    D(bug("[Wanderer:Prefs] obj = %ld\n", self));
    return self;
}
///

///OM_DISPOSE()
IPTR WandererPrefs__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
  SETUP_INST_DATA;
  EndNotify(&data->wpd_PrefsNotifyRequest);
  return DoSuperMethodA(CLASS, self, (Msg)message);
}
///

///OM_SET()
IPTR WandererPrefs__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
  SETUP_INST_DATA;
  const struct TagItem *tstate = message->ops_AttrList;
  struct TagItem *tag;
  
  while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
  {
    switch (tag->ti_Tag)
    {
      case MUIA_WandererPrefs_Processing:
        data->wpd_PROCESSING = (BOOL)tag->ti_Data;
        break;
      case MUIA_IconWindowExt_NetworkBrowser_Show:
        data->wpd_ShowNetwork = (LONG)tag->ti_Data;
        break;

      case MUIA_IconWindowExt_UserFiles_ShowFilesFolder:
        data->wpd_ShowUserFiles = (LONG)tag->ti_Data;
        break;

      case MUIA_IconWindowExt_ScreenTitle_String:
        strcpy((STRPTR)data->wpd_ScreenTitleString, (STRPTR)tag->ti_Data);
        //data->wpd_ScreenTitleString = (LONG)tag->ti_Data;
        break;

      case MUIA_IconWindow_WindowNavigationMethod:
        data->wpd_NavigationMethod = (LONG)tag->ti_Data;
        break;
    }
  }
  
  return DoSuperMethodA(CLASS, self, (Msg)message);
}
///

///OM_GET()
IPTR WandererPrefs__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
  SETUP_INST_DATA;
  IPTR *store = message->opg_Storage;
  IPTR  rv    = TRUE;

  switch (message->opg_AttrID)
  {
    case MUIA_WandererPrefs_Processing:
      *store = (IPTR)data->wpd_PROCESSING;
      break;

    case MUIA_IconWindowExt_NetworkBrowser_Show:
      *store = (IPTR)data->wpd_ShowNetwork;
      break;

    case MUIA_IconWindowExt_UserFiles_ShowFilesFolder:
      *store = (IPTR)data->wpd_ShowUserFiles;
      break;

  case MUIA_IconWindowExt_ScreenTitle_String:
      *store = (IPTR)data->wpd_ScreenTitleString;
D(bug("[Wanderer:Prefs] WandererPrefs__GET: MUIA_IconWindowExt_ScreenTitle_String '%s'\n", data->wpd_ScreenTitleString));
      break;

    case MUIA_IconWindow_WindowNavigationMethod:
      *store = (IPTR)data->wpd_NavigationMethod;
      break;

    default:
      rv = DoSuperMethodA(CLASS, self, (Msg)message);
  }

  return rv;
}
///

///ProcessGlobalChunk()
BOOL WandererPrefs_ProccessGlobalChunk(Class *CLASS, Object *self, struct TagItem32 *global_chunk, IPTR chunk_size)
{
  //SETUP_INST_DATA;

  int i = 0, tag_count = (chunk_size / sizeof(struct TagItem32));
  BOOL cont = TRUE;

D(bug("[Wanderer:Prefs] WandererPrefs_ProccessGlobalChunk()\n"));
/* TODO: fix problems with endian-ness? */

  for (i = 0; i < tag_count; i++)
  {
    if (cont)
    {
      /* prefs file is stored in little endian */
      if (AROS_LE2LONG(global_chunk[i].ti_Tag) == TAG_DONE)
      {
          cont = FALSE;
      }
      else if (AROS_LE2LONG(global_chunk[i].ti_Tag) == MUIA_WandererPrefs_DefaultStack)
      {
/* TODO: We should have an option to set the DefaultStackSize in wanderers prefs, and push it onto workbench.library */
          struct TagItem wbca_Tags[] =
          {
                { WBCTRLA_SetDefaultStackSize, (IPTR)AROS_LE2LONG(global_chunk[i].ti_Data)      },
                { TAG_DONE, 0                                                                   }
          };
/* TODO: What should we use for the name arg in WorkbenchControlA */
          WorkbenchControlA("", wbca_Tags);
      }
      else
      {
          SET(self, AROS_LE2LONG(global_chunk[i].ti_Tag), AROS_LE2LONG(global_chunk[i].ti_Data));
      }
    }
  }

  return TRUE;
}
///
                    
///WPEditor_ProccessNetworkChunk()
BOOL WPEditor_ProccessNetworkChunk(Class *CLASS, Object *self, UBYTE *_viewSettings_Chunk)
{
  //SETUP_INST_DATA;

  struct TagItem *network_tags = (struct TagItem *)_viewSettings_Chunk;
  SET(self, AROS_LE2LONG(network_tags[0].ti_Tag), AROS_LE2LONG(network_tags[0].ti_Data));

  return TRUE;
}
///

///WPEditor_ProccessScreenTitleChunk()
BOOL WPEditor_ProccessScreenTitleChunk(Class *CLASS, Object *self, UBYTE *_ScreenTitle_Chunk)
{
  //SETUP_INST_DATA;
  char *displayed_screentitle = _ScreenTitle_Chunk;
  char *userscreentitle = NULL;

D(bug("[Wanderer:Prefs] WandererPrefs__ProccessScreenTitleChunk@@@@@@@@@: ScreenTitle Template = '%s'\n", _ScreenTitle_Chunk));

  if ((userscreentitle = ProcessUserScreenTitle(_ScreenTitle_Chunk)) != NULL)
  {
D(bug("[Wanderer:Prefs] WandererPrefs__ProccessScreenTitleChunk@@@@@@@@@: ProcessUserScreenTitle returns '%s'\n", userscreentitle));
    displayed_screentitle = userscreentitle;
  }

  SET(self, MUIA_IconWindowExt_ScreenTitle_String, displayed_screentitle);
D(bug("[Wanderer:Prefs] WandererPrefs__ProccessScreenTitleChunk@@@@@@@@@: SCREENTITLE set\n"));

  return TRUE;
}
///

///WandererPrefs_FindViewSettingsNode()
struct WandererPrefs_ViewSettingsNode *WandererPrefs_FindViewSettingsNode(struct WandererPrefs_DATA *data, char *node_Name)
{
  struct WandererPrefs_ViewSettingsNode *current_Node = NULL;
  
  #ifdef __AROS__
  ForeachNode(&data->wpd_ViewSettings, current_Node)
  #else
  Foreach_Node(&data->wpd_ViewSettings, current_Node);
  #endif
  {
    if ((strcmp(current_Node->wpbn_Name, node_Name)) == 0) return current_Node;
  }
  return NULL;
}
///

///WandererPrefs_ProccessViewSettingsChunk()
BOOL WandererPrefs_ProccessViewSettingsChunk(Class *CLASS, Object *self, char *_viewSettings_ViewName, UBYTE *_viewSettings_Chunk, IPTR chunk_size)
{
  SETUP_INST_DATA;

  struct WandererPrefs_ViewSettingsNode  *_viewSettings_Node = NULL;

D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk()\n"));

  _viewSettings_Node = WandererPrefs_FindViewSettingsNode(data, _viewSettings_ViewName);

  if (_viewSettings_Node)
  {
D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: Updating Existing node @ 0x%p\n", _viewSettings_Node));
    if (_viewSettings_Node->wpbn_Background)
        FreeVec((APTR)_viewSettings_Node->wpbn_Background);
    /* TODO: Free any Cached backgrounds here .. */
  }
  else
  {
D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: Creating new node for '%s'\n", _viewSettings_ViewName));
    _viewSettings_Node = AllocMem(sizeof(struct WandererPrefs_ViewSettingsNode), MEMF_CLEAR|MEMF_PUBLIC);

    _viewSettings_Node->wpbn_Name = AllocVec(strlen(_viewSettings_ViewName) + 1, MEMF_CLEAR|MEMF_PUBLIC);
    strcpy(_viewSettings_Node->wpbn_Name, _viewSettings_ViewName);
    #ifdef __AROS__
    _viewSettings_Node->wpbn_NotifyObject = (Object *)NotifyObject, End;
    #else
    _viewSettings_Node->wpbn_NotifyObject = MUI_NewObject(MUIC_Notify, TAG_DONE);
    #endif

    AddTail(&data->wpd_ViewSettings, &_viewSettings_Node->wpbn_Node);
  }

  _viewSettings_Node->wpbn_Background =(IPTR) AllocVec(strlen(_viewSettings_Chunk) + 1, MEMF_CLEAR|MEMF_PUBLIC);
  strcpy((char *)_viewSettings_Node->wpbn_Background, _viewSettings_Chunk);
D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: NAME BACKGROUND= %s\n",_viewSettings_Chunk));
  SET(_viewSettings_Node->wpbn_NotifyObject, MUIA_Background, _viewSettings_Chunk);

/* TODO: Cache backgrounds here .. */

  if (chunk_size > (strlen(_viewSettings_Chunk) + 1))
  {
    UBYTE _viewSettings_TagOffset = ((strlen(_viewSettings_Chunk)  + 1)/4);
    IPTR _viewSettings_TagCount;

D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: Chunk has options Tag data ..\n"));

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

    _viewSettings_TagCount  = ((chunk_size - _viewSettings_TagOffset)/sizeof(struct TagItem32));

D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: %d Tags at offset %d ..\n", _viewSettings_TagCount, _viewSettings_TagOffset));

    if (_viewSettings_Node->wpbn_Options != NULL)
    {
D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: Freeing old background tag's @ 0x%p\n", _viewSettings_Node->wpbn_Options));
      FreeVec(_viewSettings_Node->wpbn_Options);
      _viewSettings_Node->wpbn_Options = NULL;
    }
    
    _viewSettings_Node->wpbn_Options = AllocVec((_viewSettings_TagCount + 1) * sizeof(struct TagItem32), MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: New tag storage @ 0x%p\n", _viewSettings_Node->wpbn_Options));

    CopyMem(_viewSettings_Chunk + _viewSettings_TagOffset, _viewSettings_Node->wpbn_Options, (_viewSettings_TagCount) * sizeof(struct TagItem32));
D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: Tags copied to storage \n"));

    _viewSettings_Node->wpbn_Options[_viewSettings_TagCount].ti_Tag = TAG_DONE;
    
    {
      int i = 0;
      for (i = 0; i < _viewSettings_TagCount; i++)
      {
#if AROS_BIG_ENDIAN
      _viewSettings_Node->wpbn_Options[i].ti_Tag  = AROS_LE2LONG(_viewSettings_Node->wpbn_Options[i].ti_Tag);
      _viewSettings_Node->wpbn_Options[i].ti_Data = AROS_LE2LONG(_viewSettings_Node->wpbn_Options[i].ti_Data);
#endif
D(bug("[Wanderer:Prefs] WandererPrefs_ProccessViewSettingsChunk: Setting Tag 0x%p Value %d\n", _viewSettings_Node->wpbn_Options[i].ti_Tag, _viewSettings_Node->wpbn_Options[i].ti_Data));
      SET(_viewSettings_Node->wpbn_NotifyObject, _viewSettings_Node->wpbn_Options[i].ti_Tag, _viewSettings_Node->wpbn_Options[i].ti_Data);
      }
    }
  }
  return TRUE;
}
///

///WandererPrefs__MUIM_WandererPrefs_Reload()
IPTR WandererPrefs__MUIM_WandererPrefs_Reload
(
  Class *CLASS, Object *self, Msg message
)
{
  D(struct ContextNode     *context);
  struct IFFHandle       *handle;
  BOOL                   success = TRUE;
  LONG                   error;
  IPTR                   iff_parse_mode = IFFPARSE_SCAN;
  UBYTE                  chunk_buffer[IFF_CHUNK_BUFFER_SIZE];

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload()\n"));

  if (!(handle = AllocIFF()))
    return FALSE;

  handle->iff_Stream = (IPTR)Open(wandererPrefs_PrefsFile, MODE_OLDFILE); 

  if (!handle->iff_Stream)
    return FALSE;

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
          D(context = CurrentChunk(handle));
          iff_parse_mode = IFFPARSE_STEP;

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: Context 0x%p\n", context));
            
          if ((error=ReadChunkBytes(handle, chunk_buffer, IFF_CHUNK_BUFFER_SIZE)))
          {
            struct WandererPrefsIFFChunkHeader *this_header =(struct WandererPrefsIFFChunkHeader *) chunk_buffer;
            char                               *this_chunk_name = NULL;
            IPTR                               this_chunk_size = AROS_LE2LONG(this_header->wpIFFch_ChunkSize);

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: ReadChunkBytes() Chunk matches Prefs Header size ..\n"));

            if ((this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,MEMF_ANY|MEMF_CLEAR)))
            {
              strcpy(this_chunk_name, this_header->wpIFFch_ChunkType);
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: Prefs Header for '%s' data size %d bytes\n", this_chunk_name, this_chunk_size));

              if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
              {
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: End of header chunk ..\n"));

                if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
                {
                  D(context = CurrentChunk(handle));

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: Context 0x%p\n", context));

                  error = ReadChunkBytes
                        (
                          handle, 
                          chunk_buffer,
                          this_chunk_size
                        );

                  if (error == this_chunk_size)
                  {
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: ReadChunkBytes() Chunk matches Prefs Data size .. (%d)\n", error));
                    if ((strcmp(this_chunk_name, "wanderer:global")) == 0)
                    {
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: Process data for wanderer global chunk ..\n"));
                      WandererPrefs_ProccessGlobalChunk(CLASS, self,(struct TagItem32 *) chunk_buffer, this_chunk_size);
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
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: Process data for wanderer background chunk '%s'..\n", view_name));
                      WandererPrefs_ProccessViewSettingsChunk(CLASS, self, view_name, chunk_buffer, this_chunk_size);
                    }
                  }//END if (error == this_chunk_size)  
                  if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)
                  {
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_Reload: TAG_DONE) of Data chunk ..\n"));
                  }
                }//END if ((error = ParseIFF(handle, IFFPARSE_STEP)) == 0)
              }//END if ((error = ParseIFF(handle, IFFPARSE_STEP)) == IFFERR_EOC)       
            }//END if ((this_chunk_name = AllocVec(strlen(this_header->wpIFFch_ChunkType) +1,MEMF_ANY|MEMF_CLEAR)))
          }//END if ((error=ReadChunkBytes(handle, chunk_buffer, IFF_CHUNK_BUFFER_SIZE)))
        }
        else
        {
D(bug("[Wanderer:Prefs] ParseIFF() failed, returncode %ld!\n", error));
          //success = FALSE;
        }//END if ((error = ParseIFF(handle, iff_parse_mode)) == 0)

      } while (error != IFFERR_EOF);
      SET(self, MUIA_WandererPrefs_Processing, FALSE);
    }
    else
    {
D(bug("[Wanderer:Prefs] StopChunk() failed, returncode %ld!\n", error));
      //success = FALSE;
    }

    CloseIFF(handle);
  }
  else
  {
D(bug("[Wanderer:Prefs] Failed to open stream!, returncode %ld!\n", error));
    //ShowError(_(MSG_CANT_OPEN_STREAM));
    success = FALSE;
  }//END if ((error = StopChunk(handle, ID_PREF, ID_WANDR)) == 0)

  Close((BPTR)handle->iff_Stream);

  FreeIFF(handle);

  return success;
}
///

///WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject()
IPTR WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject
(
  Class *CLASS, Object *self, struct MUIP_WandererPrefs_ViewSettings_GetNotifyObject *message
)
{
  SETUP_INST_DATA;
  struct WandererPrefs_ViewSettingsNode *current_Node = NULL;

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject()\n"));

  if ((current_Node = WandererPrefs_FindViewSettingsNode(data, message->Background_Name)))
  {
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject: Returning Object for existing record\n"));
    return (IPTR) current_Node->wpbn_NotifyObject;
  }

  current_Node = AllocMem(sizeof(struct WandererPrefs_ViewSettingsNode), MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject: Created new node ..\n"));

  current_Node->wpbn_Name = AllocVec(strlen(message->Background_Name) + 1, MEMF_CLEAR|MEMF_PUBLIC);
  strcpy(current_Node->wpbn_Name, message->Background_Name);
    #ifdef __AROS__
    current_Node->wpbn_NotifyObject = (Object *)NotifyObject, End;
    #else
    current_Node->wpbn_NotifyObject = MUI_NewObject(MUIC_Notify, TAG_DONE);
    #endif
  AddTail(&data->wpd_ViewSettings, &current_Node->wpbn_Node);

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetNotifyObject: Notify Object @ 0x%p\n", current_Node->wpbn_NotifyObject));

  return (IPTR) current_Node->wpbn_NotifyObject;
}
///

///NextTag32Item()
/* TODO: Replace with propper 64bit check */
/* 32bit replacements for utility.library tag funcs */
struct TagItem32 *  NextTag32Item(struct TagItem32 ** tagListPtr)
{
  if(!(*tagListPtr)) return NULL;

  while (TRUE)
  {
    switch ((*tagListPtr)->ti_Tag)
    {
    case TAG_IGNORE:
      break;

    case TAG_END:
      (*tagListPtr) = NULL;
      return NULL;

    case TAG_SKIP:
      (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
      continue;

    default:
      /* Use post-increment (return will return the current value and
      then tagListPtr will be incremented) */
      return (struct TagItem32 *)(*tagListPtr)++;
    }

    (*tagListPtr) ++;
  }
}
///

///FindTag32Item()
struct TagItem32 * FindTag32Item(ULONG tagValue, struct TagItem32 *tagList)
{
  struct TagItem32 *tag;
  const struct TagItem32 *tagptr = tagList;

  while ((tag = NextTag32Item((struct TagItem32 **)&tagptr)))
  {
    if (tag->ti_Tag == tagValue) return tag;
  }

  return NULL;

}
///

///GetTag32Data()
ULONG GetTag32Data(ULONG tagValue, ULONG defaultVal, struct TagItem32 *tagList)
{
  struct TagItem32 *ti = NULL;

  if ((tagList != NULL) && (ti = FindTag32Item(tagValue, tagList)))
  return ti->ti_Data;

  return defaultVal;
}
///

///WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute()
IPTR WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute
(
  Class *CLASS, Object *self, struct MUIP_WandererPrefs_ViewSettings_GetAttribute *message
)
{
  SETUP_INST_DATA;
  struct WandererPrefs_ViewSettingsNode *current_Node = NULL;

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute()\n"));

  if ((current_Node = WandererPrefs_FindViewSettingsNode(data, message->Background_Name)) != NULL)
  {
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute: Found Background Record ..\n"));
    if (message->AttributeID == MUIA_Background)
    {
      if (current_Node->wpbn_Background) return current_Node->wpbn_Background;
    }
    else if (current_Node->wpbn_Options)
    {
      if (sizeof(IPTR) > 4)
      {
        ULONG retVal = GetTag32Data(message->AttributeID, (ULONG)-1,(struct TagItem32 *) current_Node->wpbn_Options);

D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute: Using internal GetTag32Data()\n"));

        if (retVal != (ULONG)-1)
        return (IPTR)retVal;
      }
      else
      {
D(bug("[Wanderer:Prefs] WandererPrefs__MUIM_WandererPrefs_ViewSettings_GetAttribute: Using utility.library->GetTagData()\n"));
        return (IPTR)GetTagData(message->AttributeID, (IPTR)-1, (struct TagItem *) current_Node->wpbn_Options);
      }
    }
  }
  return (IPTR)-1;
}
///
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
