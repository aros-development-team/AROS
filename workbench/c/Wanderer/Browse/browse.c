#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include <aros/debug.h>

#include "formatting.h"
#include "fileinfo.h"

/*** Global variables ********************************************************/

/*** Functions ***************************************************************/

AROS_UFH3
(
    APTR, listConstructFunction,
    AROS_UFHA( struct Hook *,          hook,    A0 ),
    AROS_UFHA( APTR,                   pool,    A2 ),
    AROS_UFHA( struct FileInfoBlock *, fib,     A1 )
)
{
    struct FileInfo *info = allocFileInfo( pool );

    if( !info ) goto error;

        info->fi_NameLength = strlen( fib->fib_FileName );
        info->fi_Name = AllocPooled( pool, info->fi_NameLength + 1 );

        if( !info->fi_Name ) goto error;

            strncpy( info->fi_Name, fib->fib_FileName, info->fi_NameLength + 1 );
            
            info->fi_Size = fib->fib_Size;
            
    return info;
    
error:
    freeFileInfo( info, pool );
    
    return NULL;
}  

AROS_UFH3
(
    void, listDestructFunction,
    AROS_UFHA( struct Hook *,     hook, A0 ),
    AROS_UFHA( APTR,              pool, A2 ),
    AROS_UFHA( struct FileInfo *, info, A1 )
)
{
    freeFileInfo( info, pool );
}

AROS_UFH3
(   
    void, listDisplayFunction,
    AROS_UFHA( struct Hook *,     hook,    A0 ),
    AROS_UFHA( char **,           strings, A2 ),
    AROS_UFHA( struct FileInfo *, entry,   A1 )
)
{
#   define BUFFERLENGTH 100

    static char sizebuffer[BUFFERLENGTH];
    
    if( entry )
    {
        formatSize( sizebuffer, BUFFERLENGTH, entry->fi_Size );
        strings[0] = entry->fi_Name;
        strings[1] = sizebuffer;
    }
    else
    {
        strings[0] = "Name";
        strings[1] = "\33rSize";
    }
    
#undef BUFFERLENGTH
}

void scanDirectory(STRPTR dirname, Object *list )
{
    struct FileInfoBlock fib;
    BPTR   lock = Lock( dirname, SHARED_LOCK );

    Examine( lock, &fib );

    while( ExNext( lock, &fib ) )
    {
        DoMethod( list, MUIM_List_InsertSingle, &fib, MUIV_List_Insert_Bottom );
    }

    UnLock( lock );
}

int main()
{
    Object *application;
    Object *window;
    Object *list;
    Object *string;


    struct Hook listConstructHook, listDestructHook, listDisplayHook;

    listConstructHook.h_Entry = (HOOKFUNC) listConstructFunction;
    listDestructHook.h_Entry  = (HOOKFUNC) listDestructFunction;
    listDisplayHook.h_Entry   = (HOOKFUNC) listDisplayFunction;

    application = ApplicationObject,
        MUIA_Application_Menustrip, MenuitemObject,
            MUIA_Family_Child, MenuitemObject,
                MUIA_Menuitem_Title, "Wanderer",
                MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title,    "Backdrop",
                    MUIA_Menuitem_Shortcut, "B",
                    MUIA_Menuitem_Checkit,  TRUE,
                End,
                MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title,    "Execute Command",
                    MUIA_Menuitem_Shortcut, "E",
                End,
                MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title,    "Quit",
                    MUIA_Menuitem_Shortcut, "Q",
                End,
            End,
        End,
        SubWindow, window = WindowObject,
            MUIA_Window_Title,   "Browse",
            MUIA_Window_Activate, TRUE,

            WindowContents, VGroup,
	        Child, string = StringObject,
		    StringFrame,
		End,
                Child, list = ListviewObject,
                    MUIA_Listview_List, ListObject,
                        InputListFrame,
                        MUIA_List_Title,         TRUE,
                        MUIA_List_Format,        ",",
                        MUIA_List_ConstructHook, &listConstructHook,
                        MUIA_List_DestructHook,  &listDestructHook,
                        MUIA_List_DisplayHook,   &listDisplayHook,
                    End,
                End,
            End,
        End,
    End;

    scanDirectory( "LIBS:", list );

    if( application )
    {
        ULONG signals = 0;
        
        set( window, MUIA_Window_Open, TRUE );

	DoMethod
        (
            window,
            MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );

    	while
        (
            (LONG) DoMethod( application, MUIM_Application_NewInput, &signals )
            != MUIV_Application_ReturnID_Quit
        )
	{
	    if( signals )
	    {
		signals = Wait( signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D );
		if( signals & SIGBREAKF_CTRL_C ) break;
		if( signals & SIGBREAKF_CTRL_D ) break;
	    }
	}

	MUI_DisposeObject( application );
	
	return RETURN_OK;
    }

    return RETURN_FAIL;
}
