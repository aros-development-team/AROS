/*
    Copyright � 2004-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <exec/types.h>
//#include <exec/lists.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <dos/notify.h>
#include <workbench/handler.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/workbench.h>
#include <proto/layers.h>
#include <proto/alib.h>

#include <string.h>
#include <stdio.h>

#include <aros/debug.h>

#include <aros/detach.h>

#include <prefs/wanderer.h>

#include "iconwindow.h"
#include "wandererprefs.h"
#include "wandererprefsintern.h"
#include "wanderer.h"
#include "filesystems.h"

#include "locale.h"

#define VERSION "$VER: Wanderer 0.4� (22.10.2006) � AROS Dev Team"

extern IPTR InitWandererPrefs(void);
VOID DoAllMenuNotifies(Object *strip, char *path);
Object *FindMenuitem(Object* strip, int id);
Object * __CreateWandererIntuitionMenu__ ( );
void window_update(void);

extern Object *app;
struct Hook hook_standard;
struct Hook hook_action;

static char strtochar(char *st)
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
    ch = strtochar((char *)_(MSG_MEM_G));
    }
    else if (num >= 1048576)
    {
        array.val = num >> 20;
        d = ((UQUAD)num * 10 + 524288) / 1048576;
        array.dec = d % 10;
        //ch = 'M';
        ch = strtochar((char *)_(MSG_MEM_M));
    }
    else if (num >= 1024)
    {
        array.val = num >> 10;
        d = (num * 10 + 512) / 1024;
        array.dec = d % 10;
        //ch = 'K';
        ch = strtochar((char *)_(MSG_MEM_K));
    }
    else
    {
        array.val = num;
        array.dec = 0;
        d = 0;
        //ch = 'B';
    ch = strtochar((char *)_(MSG_MEM_B));
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

STRPTR GetScreenTitle(VOID)
{
    STATIC TEXT title[256];
    UBYTE chip[10], fast[10];
    fmtlarge(chip,AvailMem(MEMF_CHIP));
    fmtlarge(fast,AvailMem(MEMF_FAST));
    /* AROS probably don't have graphics mem but without it looks so empty */
    sprintf(title, _(MSG_SCREENTITLE), chip, fast);

    return title;
}

enum
{
    MEN_WANDERER = 1,
    MEN_WANDERER_BACKDROP,
    MEN_WANDERER_EXECUTE,
    MEN_WANDERER_SHELL,
    MEN_WANDERER_GUISETTINGS,
    MEN_WANDERER_ABOUT,
    MEN_WANDERER_QUIT,
	
	MEN_WINDOW_NEW_DRAWER,
	MEN_WINDOW_OPEN_PARENT,
	MEN_WINDOW_CLOSE,
	MEN_WINDOW_UPDATE,

	MEN_WINDOW_SELECT,
	MEN_WINDOW_CLEAR,

	MEN_WINDOW_SNAP_WIN,
	MEN_WINDOW_SNAP_ALL,

	MEN_WINDOW_VIEW_ICON,
	MEN_WINDOW_VIEW_DETAIL,
	MEN_WINDOW_VIEW_ALL,

	MEN_WINDOW_SORT_NOW,
    MEN_WINDOW_SORT_NAME,
    MEN_WINDOW_SORT_TYPE,
    MEN_WINDOW_SORT_DATE,
    MEN_WINDOW_SORT_SIZE,
    MEN_WINDOW_SORT_REVERSE,
    MEN_WINDOW_SORT_TOPDRAWERS,
	MEN_WINDOW_SORT_GROUP,
    
    MEN_ICON_OPEN,
    MEN_ICON_RENAME,
    MEN_ICON_INFORMATION,
    MEN_ICON_DELETE,
};



/**************************************************************************
 Open the execute window. Similar to below but you can also set the
 command. Called when item is openend
**************************************************************************/
void execute_open_with_command(BPTR cd, char *contents)
{
    BPTR lock;
    
    if (cd != NULL) lock = cd;
    else            lock = Lock("RAM:", ACCESS_READ);
        
    OpenWorkbenchObject
    (
        "WANDERER:Tools/ExecuteCommand",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, (IPTR) contents,
        TAG_DONE
    );
    
    if (cd == NULL) UnLock(lock);
}

/**************************************************************************
 Open the execute window

 This function will always get the current drawer as argument
**************************************************************************/
VOID execute_open(char **cdptr)
{
    //TODO: remove the char **cdptr from top
    //TODO:remove this commented out stuff
    //BPTR lock = NULL;
    //if (cdptr != NULL) lock = Lock(*cdptr, SHARED_LOCK);
    Object *win = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = XGET( win, MUIA_IconWindow_Drawer );
    BPTR cd = Lock(dr,SHARED_LOCK);
    execute_open_with_command(cd, NULL);
    if (cd) UnLock(cd);
}

/*******************************/

void shell_open(char **cd_ptr)
{
    //TODO: remove the char **cdptr from top
    //TODO:remove this commented out stuff
    //BPTR cd = Lock(*cd_ptr,ACCESS_READ);
    Object *win = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = XGET( win, MUIA_IconWindow_Drawer );
    BPTR cd = Lock(dr,ACCESS_READ);
    if (SystemTags("NewShell", NP_CurrentDir, (IPTR)cd, TAG_DONE) == -1)
    {
    	UnLock(cd);
    }
}

void wanderer_backdrop(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WANDERER_BACKDROP);
    Object *window = (Object *) XGET(app, MUIA_Wanderer_WorkbenchWindow);
    
    if (item != NULL)
    {
    	SET(window, MUIA_Window_Open, FALSE);
	    SET(window, MUIA_IconWindow_IsBackdrop, XGET(item, MUIA_Menuitem_Checked));
    	SET(window, MUIA_Window_Open, TRUE);
    }
}

void window_new_drawer(char **cdptr)
{
    //TODO: remove the char **cdptr from top
    
    Object *win = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = XGET( win, MUIA_IconWindow_Drawer );
    D(bug("[wanderer] NewDrawer %s\n", dr));

    Object *actwindow = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object *wbwindow = (Object *) XGET(app, MUIA_Wanderer_WorkbenchWindow);
    if (actwindow == wbwindow)
    {
    	/* This check is necessary because WorkbenchWindow has path RAM: */
    	D(bug("[wanderer] Can't call WBNewDrawer for WorkbenchWindow\n"));
    	return;
    }
    if ( XGET(actwindow, MUIA_Window_Open) == FALSE )
    {
    	D(bug("[wanderer] Can't call WBNewDrawer: the active window isn't open\n"));
    	return;
    }

    BPTR lock = Lock(dr, ACCESS_READ);
    OpenWorkbenchObject
	(
        "WANDERER:Tools/WBNewDrawer",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, 0,
        TAG_DONE
	);
    UnLock(lock);
}

void window_open_parent(char **cdptr)
{
    //TODO: Remove the **cdptr stuff from top
    Object *win = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = XGET( win, MUIA_IconWindow_Drawer );
    	
    IPTR	path_len=0;
	char	*last_letter=NULL;
  	last_letter = (char*)(*((char *)(dr+strlen(dr)-1)));
	
	STRPTR thispath = FilePart(dr);
	
	if (last_letter==(char *)0x3a) return; /* Top Drawer has no parent to open */
	
	last_letter = (char*)(*((char *)(thispath-1)));
	
	if (last_letter==(char *)0x3a) path_len = (IPTR)(thispath-(IPTR)(dr));
	else path_len = (IPTR)((thispath-(IPTR)(dr))-1);
	
	STRPTR buf = AllocVec((path_len+1),MEMF_PUBLIC|MEMF_CLEAR);	
	CopyMem(dr, buf, path_len);
	
	Object *cstate = (Object*)(((struct List*)XGET(app, MUIA_Application_WindowList))->lh_Head);
	Object *child;
	
	while ((child = NextObject(&cstate)))
	{
		if (XGET(child, MUIA_UserData))
		{
			char *child_drawer = (char*)XGET(child, MUIA_IconWindow_Drawer);
			if (child_drawer && !Stricmp(buf,child_drawer))
			{
				int is_open = XGET(child, MUIA_Window_Open);
				if (!is_open)
					DoMethod(child, MUIM_IconWindow_Open);
				else
				{
					DoMethod(child, MUIM_Window_ToFront);
					set(child, MUIA_Window_Activate, TRUE);
				}
			FreeVec(buf);
			return; 
			}
		}
	}
	
	DoMethod(app, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf);
	FreeVec(buf);
}

void window_close()
{
	Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
	set(window, MUIA_Window_CloseRequest, TRUE);
}

void window_update()
{
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (iconList != NULL)
    {
    	DoMethod(iconList, MUIM_IconList_Update);
    }
}

void window_sort_name(Object **pstrip)
{
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = DoMethod(iconList, MUIM_IconList_GetSortBits);

		/*name = date and size bit both NOT set*/
		if( (sort_bits & ICONLIST_SORT_BY_DATE) || (sort_bits & ICONLIST_SORT_BY_SIZE) )
		{
			sort_bits &= ~(ICONLIST_SORT_BY_DATE | ICONLIST_SORT_BY_SIZE);
		}

    	DoMethod(iconList, MUIM_IconList_SetSortBits, sort_bits);
    	DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void window_sort_date(Object **pstrip)
{
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = DoMethod(iconList, MUIM_IconList_GetSortBits);

		/*exclude size bit*/
		if( sort_bits & ICONLIST_SORT_BY_SIZE )
		{
			sort_bits &= ~ICONLIST_SORT_BY_SIZE;
		}

		sort_bits |= ICONLIST_SORT_BY_DATE;

    	DoMethod(iconList, MUIM_IconList_SetSortBits, sort_bits);
    	DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void window_sort_size(Object **pstrip)
{
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = DoMethod(iconList, MUIM_IconList_GetSortBits);

		/*exclude date bit*/
		if( sort_bits & ICONLIST_SORT_BY_DATE )
		{
			sort_bits &= ~ICONLIST_SORT_BY_DATE;
		}

		sort_bits |= ICONLIST_SORT_BY_SIZE;

    	DoMethod(iconList, MUIM_IconList_SetSortBits, sort_bits);
    	DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void window_sort_type(Object **pstrip)
{
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = DoMethod(iconList, MUIM_IconList_GetSortBits);

		/*type = both date and size bits set*/
		sort_bits |= (ICONLIST_SORT_BY_DATE | ICONLIST_SORT_BY_SIZE);

    	DoMethod(iconList, MUIM_IconList_SetSortBits, sort_bits);
    	DoMethod(iconList, MUIM_IconList_Sort);
    }
}


void window_sort_reverse(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_REVERSE);
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
		ULONG sort_bits = DoMethod(iconList, MUIM_IconList_GetSortBits);

		if( XGET(item, MUIA_Menuitem_Checked) )
		{
			sort_bits |= ICONLIST_SORT_REVERSE;
		}
		else
		{
			sort_bits &= ~ICONLIST_SORT_REVERSE;
		}

    	DoMethod(iconList, MUIM_IconList_SetSortBits, sort_bits);
    	DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void window_sort_topdrawers(Object **pstrip)
{
	Object *strip = *pstrip;
	Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_TOPDRAWERS);
	Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
	Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
		ULONG sort_bits = DoMethod(iconList, MUIM_IconList_GetSortBits);

		if( XGET(item, MUIA_Menuitem_Checked) )
		{
			sort_bits &= !ICONLIST_SORT_DRAWERS_MIXED;
		}
		else
		{
			sort_bits |= ICONLIST_SORT_DRAWERS_MIXED;
		}

    	DoMethod(iconList, MUIM_IconList_SetSortBits, sort_bits);
    	DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void icon_open()
{
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    DoMethod(window, MUIM_IconWindow_DoubleClicked);
}

void icon_rename(void)
{
    Object                *window   = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextSelected_Start;
    
    do
    {
        DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
        
        if ((int)entry != MUIV_IconList_NextSelected_End) 
        {
            BPTR lock   = Lock(entry->filename, ACCESS_READ);
            BPTR parent = ParentDir(lock);
            UnLock(lock);
            
            D(bug("[wanderer] *** selected: %s\n", entry->filename));
            
            OpenWorkbenchObject
            (
                "WANDERER:Tools/WBRename",
                WBOPENA_ArgLock, (IPTR) parent,
                WBOPENA_ArgName, (IPTR) FilePart(entry->filename),
                TAG_DONE
            );
            
            D(bug("[wanderer] *** selected: %s\n", entry->filename));
            
            UnLock(parent);
        }
        else
        {
            break;
        }
    } while (TRUE);
}


void icon_information()
{
    Object                *window   = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);   
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextSelected_Start;
    
    do
    {
        DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
        
        if ((int)entry != MUIV_IconList_NextSelected_End)
        {
            BPTR lock   = Lock(entry->filename, ACCESS_READ);
            BPTR parent = ParentDir(lock);
            
            D(bug("[wanderer] selected: %s\n", entry->filename));
            
            WBInfo(lock, entry->filename, NULL);
            
            UnLock(parent);
            UnLock(lock);
        }
        else
        {
            break;
        }
    } while (TRUE);
}

void icon_delete(void)
{
    Object                *window   = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextSelected_Start;
    
    do
    {
        DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
        
        if ((int)entry != MUIV_IconList_NextSelected_End) 
        {
            BPTR lock   = Lock(entry->filename, ACCESS_READ);
            BPTR parent = ParentDir(lock);
            UnLock(lock);
            
            D(bug("[wanderer] selected: %s\n", entry->filename));
            
            OpenWorkbenchObject
            (
                "WANDERER:Tools/Delete",
                WBOPENA_ArgLock, (IPTR) parent,
                WBOPENA_ArgName, (IPTR) FilePart(entry->filename),
                TAG_DONE
            );
            
            D(bug("[wanderer] selected: %s\n", entry->filename));
            
            UnLock(parent);
        }
        else
        {
            break;
        }
    } while (TRUE);
}

void wanderer_guisettings(void)
{
    //DoMethod(app, MUIM_Application_OpenConfigWindow);
    OpenWorkbenchObject("SYS:Prefs/Zune",
                WBOPENA_ArgName, (IPTR) "WANDERER",
                TAG_DONE);
}

void wanderer_about(void)
{
    OpenWorkbenchObject("SYS:System/About", TAG_DONE);
}

void wanderer_quit(void)
{
    //if (MUI_RequestA(app, NULL, 0, "Wanderer", _(MSG_YESNO), _(MSG_REALLYQUIT), NULL))
	//DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    OpenWorkbenchObject("WANDERER:Tools/Quit", TAG_DONE);
}



AROS_UFH3
(
    void, hook_func_action,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct IconWindow_ActionMsg *, msg, A1)
)
{
    AROS_USERFUNC_INIT
    
    if (msg->type == ICONWINDOW_ACTION_OPEN)
    {
       static char buf[1024];
	   struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
	
	   DoMethod(msg->iconlist, MUIM_IconList_NextSelected, (IPTR) &ent);
	   if ((int)ent == MUIV_IconList_NextSelected_End) return;

	   if (msg->isroot)
	   {
	       strcpy(buf,ent->label);
	       strcat(buf,":");
	   }
       else
	   {
	       strcpy(buf,ent->filename);
       }

       
    	if  ( (ent->type == ST_ROOT) || (ent->type == ST_USERDIR) )
    	{
    	    Object *cstate = (Object*)(((struct List*)XGET(_app(obj), MUIA_Application_WindowList))->lh_Head);
    	    Object *prefs = (Object*) XGET(_app(obj), MUIA_Wanderer_Prefs);
    	    Object *child;

            /* open new window if root or classic navigation set */
            if ( (msg->isroot) || (XGET(prefs, MUIA_WandererPrefs_NavigationMethod) == WPD_NAVIGATION_CLASSIC) )
            {
        	    while ((child = NextObject(&cstate)))
        	    {
        	    	if (XGET(child, MUIA_UserData))
        	    	{
            		    char *child_drawer = (char*)XGET(child, MUIA_IconWindow_Drawer);
            		    if (child_drawer && !Stricmp(buf,child_drawer))
            		    {
            		    	int is_open = XGET(child, MUIA_Window_Open);
            		    	
            		    	if (!is_open)
            		    	{
            			       DoMethod(child, MUIM_IconWindow_Open);
                            }
                			else
                			{
                			    DoMethod(child, MUIM_Window_ToFront);
                			    set(child, MUIA_Window_Activate, TRUE);
                			}
                			
            		    	return;
            		    }
        	    	}
        	    } 
           
        		/* Check if the window for this drawer is already opened */
        		DoMethod(app, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf);
                        // FIXME: error handling
            }
            else
            {
                /* open drawer in same window */
                set(obj, MUIA_IconWindow_Drawer, buf);
            }
            

    	} 
        else if (ent->type == ST_FILE)
	    {
            BPTR newwd, oldwd, file;
    
    	    /* Set the CurrentDir to the path of the executable to be started */
    	    file = Lock(ent->filename, SHARED_LOCK);
    	    if(file)
    	    {
        		newwd = ParentDir(file);
        		oldwd = CurrentDir(newwd);
        		
        		if (!OpenWorkbenchObject(ent->filename, TAG_DONE))
        		{
        		    execute_open_with_command(newwd, FilePart(ent->filename));
        		}
        		
        		CurrentDir(oldwd);
        		UnLock(newwd);
        		UnLock(file);
	        }
         }
    } 
    else  if (msg->type == ICONWINDOW_ACTION_DIRUP)
    {     
                     
       char *actual_drawer = (char*)XGET(obj, MUIA_IconWindow_Drawer);
       char *parent_drawer = strrchr(actual_drawer,'/');
       char *root_drawer = strrchr(actual_drawer,':');
                 
       /* check if dir is not drive root dir */
       if ( strlen(root_drawer) > 1 )
       {
           /* check if second or third level directory*/
           if (!parent_drawer)
           {
               (*(root_drawer+1)) = 0;
               set(obj, MUIA_IconWindow_Drawer, actual_drawer);
               
           }
           else
           {
               (*parent_drawer) = 0;
               set(obj, MUIA_IconWindow_Drawer, actual_drawer);
           } 
           
           /* update the window */
           //window_update();
       }
    
    } 
    else if (msg->type == ICONWINDOW_ACTION_CLICK)
    {
    	if (!msg->click->shift)
    	{
    	    Object *cstate = (Object*)(((struct List*)XGET(app, MUIA_Application_WindowList))->lh_Head);
    	    Object *child;
    
    	    while ((child = NextObject(&cstate)))
    	    {
    	    	if (XGET(child, MUIA_UserData))
    	    	{
                     if (child != obj)  DoMethod(child, MUIM_IconWindow_UnselectAll);
                }
    	    }
    	}
    } 
    else if (msg->type == ICONWINDOW_ACTION_ICONDROP)
    {
        IPTR destination_path;

        struct IconList_wDrop *drop = (struct IconList_wDrop *)msg->drop;

        if (drop)
        {
             /* get path of DESTINATION iconlist*/
             get( drop->destination_iconlistobj, MUIA_IconDrawerList_Drawer, &destination_path);

             /* get SOURCE entries */
             struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
      
             /* process all selected entries */
             do {
                   DoMethod(drop->source_iconlistobj, MUIM_IconList_NextSelected, (IPTR) &ent);

                   /* if not end of selection, process */
                   if ( (int)ent != MUIV_IconList_NextSelected_End )
                   {
                         char iconfilenamebuffer[256];

                         D(bug("[WANDERER] drop entry: %s dropped in %s\n", ent->filename, (char *)destination_path);)
 
                         /* copy via filesystems.c */
                         D(bug("[WANDERER] CopyContent \"%s\" to \"%s\"\n", ent->filename, destination_path );)
                         CopyContent(ent->filename, destination_path, TRUE, ACTION_COPY, NULL, NULL, NULL);
                         
                         /* try to copy .info aswell  */
                         memset( iconfilenamebuffer, '\0', sizeof(iconfilenamebuffer) ); 
                         strcat( iconfilenamebuffer, ent->filename );
                         strcat( iconfilenamebuffer, ".info");
                         D(bug("[WANDERER] CopyContent \"%s\" to \"%s\"\n", iconfilenamebuffer, destination_path );)                
                         CopyContent(iconfilenamebuffer, destination_path, TRUE, ACTION_COPY, NULL, NULL, NULL);
                                                  
                         /* update list contents */
                         DoMethod(drop->destination_iconlistobj,MUIM_IconList_Update);
                   }
             } while ( (int)ent != MUIV_IconList_NextSelected_End );

           /* update the window */
           window_update();      
        }
    }
    else if (msg->type == ICONWINDOW_ACTION_APPWINDOWDROP)
    {
        struct Screen *wscreen;
        struct Layer *layer;

        /* get wanderer�s screen struct and the layer located at cursor position afterwards */
        get( obj, MUIA_Window_Screen, &wscreen);
        layer = WhichLayer(&wscreen->LayerInfo,wscreen->MouseX,wscreen->MouseY);

        if (layer)
        {
            struct Window *win = (struct Window *) layer->Window;
            if (win)
            {
                struct List AppList;
                ULONG files = 0;
                BOOL  fail  = FALSE;
                NewList(&AppList);

                struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
                /* process all selected entries */
                do {
                    DoMethod(msg->iconlist, MUIM_IconList_NextSelected, (IPTR) &ent);
                    /*  if not end of selection, process */
                    if ( (int)ent != MUIV_IconList_NextSelected_End )
                    {
                        struct AppW *a = AllocVec(sizeof(struct AppW), MEMF_CLEAR);
                        if (a)
                        {
                            a->name = AllocVec(strlen(ent->filename)+1, MEMF_CLEAR);
                            if (a->name)
                            {
                                files++;
                                strcpy(a->name, ent->filename);
                                AddTail(&AppList, (struct Node *) a);
                            }
                            else
                            {
                                FreeVec(a);
                                fail = TRUE;
                            }
                        } else fail = TRUE;
                    }
                } while ( ((int)ent != MUIV_IconList_NextSelected_End ) && !fail);
                if (!fail && (files > 0))
                {
                    char **filelist = AllocVec(sizeof(char *) * files, MEMF_CLEAR);
                    if (filelist)
                    {
                        char **flist = filelist;
                        if (!IsListEmpty(&AppList))
                        {
                            struct Node *succ;
                            struct Node *s = AppList.lh_Head;
                            while (((succ = ((struct Node*) s)->ln_Succ) != NULL) && !fail)
                            {
                                *flist ++ = ((struct AppW *) s)->name;
                                s =  succ;
                            }

                            D(bug("[WANDERER] AppWindowMsg: win:%s files:%s mx:%d my:%d\n",win->Title, filelist, wscreen->MouseX, wscreen->MouseY);)
                            /* send appwindow msg struct containing selected files to destination */
                            SendAppWindowMessage(win, files, filelist, 0, wscreen->MouseX, wscreen->MouseY, 0, 0);

                        }
                        FreeVec(filelist);
                    }
                }
                if (!IsListEmpty(&AppList))
                {
                    struct Node *succ;
                    struct Node *s = AppList.lh_Head;
                    while (((succ = ((struct Node*) s)->ln_Succ) != NULL))
                    {
                        FreeVec(((struct AppW *) s)->name);
                        FreeVec(s);
                        s =  succ;
                    }
                }

            }
        }    	
    	

    }

    AROS_USERFUNC_EXIT
}



AROS_UFH3
(
    void, hook_func_standard,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1)
)
{
    AROS_USERFUNC_INIT
    
    void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);
    if (func) func((ULONG *)(funcptr + 1));
    
    AROS_USERFUNC_EXIT
}

/**************************************************************************
 This function returns a Menu Object with the given id
**************************************************************************/
Object *FindMenuitem(Object* strip, int id)
{
    return (Object*)DoMethod(strip, MUIM_FindUData, id);
}

/**************************************************************************
 This connects a notify to the given menu entry id
**************************************************************************/
VOID DoMenuNotify(Object* strip, int id, void *function, void *arg)
{
    Object *entry;
    entry = FindMenuitem(strip,id);
    if (entry)
    {
	DoMethod
        (
            entry, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, 
            (IPTR) entry, 4, MUIM_CallHook, (IPTR) &hook_standard,
            (IPTR) function, (IPTR) arg
        );
    }
}

VOID DoAllMenuNotifies(Object *strip, char *path)
{
    Object *item;

    if (!strip) return;

    DoMenuNotify(strip, MEN_WANDERER_EXECUTE,       execute_open,           path);
    DoMenuNotify(strip, MEN_WANDERER_SHELL,         shell_open,             path);
    DoMenuNotify(strip, MEN_WANDERER_GUISETTINGS,   wanderer_guisettings,   NULL);
    DoMenuNotify(strip, MEN_WANDERER_ABOUT,         wanderer_about,         NULL);
    DoMenuNotify(strip, MEN_WANDERER_QUIT,          wanderer_quit,          NULL);

    DoMenuNotify(strip, MEN_WINDOW_NEW_DRAWER,      window_new_drawer,      path);
    DoMenuNotify(strip, MEN_WINDOW_OPEN_PARENT,     window_open_parent,     path);
    DoMenuNotify(strip, MEN_WINDOW_CLOSE,           window_close,           NULL);
    DoMenuNotify(strip, MEN_WINDOW_UPDATE,          window_update,          NULL);
    DoMenuNotify(strip, MEN_WINDOW_SORT_NAME,       window_sort_name,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_TYPE,       window_sort_type,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_DATE,       window_sort_date,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_SIZE,       window_sort_size,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_REVERSE,    window_sort_reverse,    strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_TOPDRAWERS, window_sort_topdrawers, strip);

    DoMenuNotify(strip, MEN_ICON_OPEN,              icon_open,              NULL);
    DoMenuNotify(strip, MEN_ICON_RENAME,            icon_rename,            NULL);
    DoMenuNotify(strip, MEN_ICON_INFORMATION,       icon_information,       NULL);
    DoMenuNotify(strip, MEN_ICON_DELETE,            icon_delete,            NULL);
    
    if ((item = FindMenuitem(strip, MEN_WANDERER_BACKDROP)))
    {
	    DoMethod
        (
            item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, 
            (IPTR) app, 7, MUIM_Application_PushMethod, 
            (IPTR) app, 4, MUIM_CallHook, (IPTR) &hook_standard, 
            (IPTR) wanderer_backdrop, (IPTR) strip
        );
    }
}

/*** Instance Data **********************************************************/
struct Wanderer_DATA
{
    Object                      *wd_Prefs,
                                *wd_ActiveWindow,
                                *wd_WorkbenchWindow;

    struct MUI_InputHandlerNode  wd_TimerIHN;
    struct MsgPort              *wd_CommandPort;
    struct MUI_InputHandlerNode  wd_CommandIHN;
    struct MsgPort              *wd_NotifyPort;
    struct MUI_InputHandlerNode  wd_NotifyIHN;
    struct NotifyRequest         pnr;
    IPTR                         wd_PrefsIntern;
    
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct Wanderer_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *Wanderer__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Application_Title,       (IPTR) "Wanderer",
	    MUIA_Application_Base,        (IPTR) "WANDERER",
	    MUIA_Application_Version,     (IPTR) VERSION,
	    MUIA_Application_Description, (IPTR) _(MSG_DESCRIPTION),
	    MUIA_Application_SingleTask,         TRUE,
    	
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        /*-- Setup hooks structures ----------------------------------------*/
        hook_standard.h_Entry = (HOOKFUNC) hook_func_standard;
        hook_action.h_Entry   = (HOOKFUNC) hook_func_action;
        
        // ---
        if ((data->wd_CommandPort = CreateMsgPort()) == NULL)
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
        }
    
        if ((data->wd_NotifyPort = CreateMsgPort()) == NULL)
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
        }
    
        RegisterWorkbench(data->wd_CommandPort);
        
        /* Setup command port handler --------------------------------------*/ 
        data->wd_CommandIHN.ihn_Signals = 1UL << data->wd_CommandPort->mp_SigBit;
        data->wd_CommandIHN.ihn_Object  = self;
        data->wd_CommandIHN.ihn_Method  = MUIM_Wanderer_HandleCommand;
        
        DoMethod
        (
            self, MUIM_Application_AddInputHandler, (IPTR) &data->wd_CommandIHN
        );
        
        /* Setup timer handler ---------------------------------------------*/
        data->wd_TimerIHN.ihn_Flags  = MUIIHNF_TIMER;
        data->wd_TimerIHN.ihn_Millis = 3000;
        data->wd_TimerIHN.ihn_Object = self;
        data->wd_TimerIHN.ihn_Method = MUIM_Wanderer_HandleTimer;
        
        DoMethod
        (
            self, MUIM_Application_AddInputHandler, (IPTR) &data->wd_TimerIHN
        );

        /* Setup filesystem notification handler ---------------------------*/
        data->wd_NotifyIHN.ihn_Signals = 1UL << data->wd_NotifyPort->mp_SigBit;
        data->wd_NotifyIHN.ihn_Object  = self;
        data->wd_NotifyIHN.ihn_Method  = MUIM_Wanderer_HandleNotify;
        
        DoMethod
        (
            self, MUIM_Application_AddInputHandler, (IPTR) &data->wd_NotifyIHN
        );

// All the following should be moved to InitWandererPrefs
        
        /* Setup notification on prefs file --------------------------------*/
        data->pnr.nr_Name                 = "ENV:SYS/Wanderer.prefs";
        data->pnr.nr_Flags                = NRF_SEND_MESSAGE;
        data->pnr.nr_stuff.nr_Msg.nr_Port = data->wd_NotifyPort;
        
        if (StartNotify(&data->pnr))
        {
            D(bug("Wanderer: prefs notification setup ok\n"));
        }
        else
        {
            D(bug("Wanderer: prefs notification setup FAILED\n"));
        }
        
        data->wd_Prefs = WandererPrefsObject, End; // FIXME: error handling
        data->wd_PrefsIntern = InitWandererPrefs();
    }
    
    return self;
}

IPTR Wanderer__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    
    if (data->wd_CommandPort)
    {
	/*
            They only have been added if the creation of the msg port was
	    successful
        */
	    DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_TimerIHN);
	    DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_CommandIHN);
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_NotifyIHN);
	
        UnregisterWorkbench(data->wd_CommandPort);
	
        EndNotify(&data->pnr);
        
        DeleteMsgPort(data->wd_NotifyPort);
        data->wd_NotifyPort = NULL;
        
        DeleteMsgPort(data->wd_CommandPort);
	    data->wd_CommandPort = NULL;
        
        DisposeObject(data->wd_Prefs);
	    data->wd_Prefs = NULL;
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Wanderer__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem((const struct TagItem**)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Wanderer_ActiveWindow:
                data->wd_ActiveWindow = (Object *) tag->ti_Data;
                bug("*** wanderer active window: %p\n", tag->ti_Data);
                break;
		
	    case MUIA_Application_Iconified:
	        /* Wanderer itself cannot be iconified, 
		   just hide, instead.  */
	        tag->ti_Tag  = MUIA_ShowMe;
		    tag->ti_Data = !tag->ti_Data;
		break; 
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Wanderer__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        case MUIA_Wanderer_Prefs:
            *store = (IPTR) data->wd_Prefs;
            break;
            
        case MUIA_Wanderer_ActiveWindow:
            *store = (IPTR) data->wd_ActiveWindow;
            break;
        
        case MUIA_Wanderer_WorkbenchWindow:
            *store = (IPTR) data->wd_WorkbenchWindow;
            break;
            
        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

IPTR Wanderer__MUIM_Application_Execute
(
    Class *CLASS, Object *self, Msg message 
)
{
    SETUP_INST_DATA;
    
    data->wd_WorkbenchWindow = (Object *) DoMethod
    (
        self, MUIM_Wanderer_CreateDrawerWindow, (IPTR) NULL
    );
    
    if (data->wd_WorkbenchWindow != NULL)
    {
        DoMethod
        (
            data->wd_WorkbenchWindow, MUIM_KillNotify, MUIA_Window_CloseRequest
        );
        
        DoMethod
        (
            data->wd_WorkbenchWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) self, 3, MUIM_CallHook, (IPTR) &hook_standard, (IPTR) wanderer_quit
        );

        Detach();
        
	DoSuperMethodA(CLASS, self, message);
        
        return RETURN_OK;
    }
    
    // FIXME: report error...
    
    return RETURN_ERROR;
}

IPTR Wanderer__MUIM_Wanderer_HandleTimer
(
    Class *CLASS, Object *self, Msg message
)
{
    Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
    Object *child;
    char *scr_title = GetScreenTitle();

    while ((child = NextObject(&cstate)))
	set(child, MUIA_Window_ScreenTitle, (IPTR) scr_title);
    
    return TRUE;
}

IPTR Wanderer__MUIM_Wanderer_HandleCommand
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    struct WBHandlerMessage *wbhm;
    
    D(bug("Wanderer: Recieved signal at notify port\n"));
    
    while ((wbhm = WBHM(GetMsg(data->wd_CommandPort))) != NULL)
    {
        D(bug("Wanderer: Recieved message from handler, type = %ld\n", wbhm->wbhm_Type));
        
        switch (wbhm->wbhm_Type)
        {
            case WBHM_TYPE_SHOW:
                D(bug("Wanderer: WBHM_TYPE_SHOW\n"));
                set(self, MUIA_ShowMe, TRUE);
                break;
            
            case WBHM_TYPE_HIDE:
                D(bug("Wanderer: WBHM_TYPE_HIDE\n"));
                set(self, MUIA_ShowMe, FALSE);
                break;
                
            case WBHM_TYPE_UPDATE:
                D(bug("Wanderer: WBHM_TYPE_UPDATE\n"));
                {
                    CONST_STRPTR name = wbhm->wbhm_Data.Update.Name;
                    ULONG        length;
                    
                    switch (wbhm->wbhm_Data.Update.Type)
                    {
                        case WBDISK:
                        case WBDRAWER:
                        case WBGARBAGE:
                            length = strlen(name);
                            break;
                            
                        default:
                            length = PathPart(name) - name;
                            break;
                    }
                    
                    D(bug("Wanderer: name = %s, length = %ld\n", name, length));
                    
                    {
                        Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
                        Object *child;
                        
                        while ((child = NextObject(&cstate)))
                        {
                            if (XGET(child, MUIA_UserData))
                            {
                                char *child_drawer = (char*)XGET(child, MUIA_IconWindow_Drawer);
                                
                                if
                                (
                                       child_drawer != NULL 
                                    && strncmp(name, child_drawer, length) == 0
                                    && strlen(child_drawer) == length
                                )
                                {
                                    Object *iconlist = (Object *) XGET(child, MUIA_IconWindow_IconList);
                                    
                                    D(bug("Wanderer: Drawer found: %s!\n", child_drawer));
                                    
                                    if (iconlist != NULL)
                                    {
                                        DoMethod(iconlist,MUIM_IconList_Update);
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
                
            case WBHM_TYPE_OPEN:
                D(bug("Wanderer: WBHM_TYPE_OPEN\n"));
            
                {
                    Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
                    Object *child;
                    CONST_STRPTR buf = wbhm->wbhm_Data.Open.Name;
                    
                    while ((child = NextObject(&cstate)))
                    {
                        if (XGET(child, MUIA_UserData))
                        {
                            char *child_drawer = (char*)XGET(child, MUIA_IconWindow_Drawer);
                            if (child_drawer && !Stricmp(buf,child_drawer))
                            {
                                int is_open = XGET(child, MUIA_Window_Open);
                                if (!is_open)
                                    DoMethod(child, MUIM_IconWindow_Open);
                                else
                                {
                                    DoMethod(child, MUIM_Window_ToFront);
                                    set(child, MUIA_Window_Activate, TRUE);
                                }
                                return 0; 
                            }
                        }
                    }
                    
                    DoMethod
                    (
                        app, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf
                    );
                }
                break;
        } /* switch */
        
        ReplyMsg((struct Message *) wbhm);
    }
    
    return 0;
}


IPTR Wanderer__MUIM_Wanderer_HandleNotify
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    struct Message *notifyMessage;
    
    D(bug("Wanderer: got prefs change notify!\n"));
    
    while ((notifyMessage = GetMsg(data->wd_NotifyPort)) != NULL)
    {
        ReplyMsg(notifyMessage);
    }
    
    /* reload prefs file */
    DoMethod(data->wd_Prefs, MUIM_WandererPrefs_Reload);

    /* upadte prefs for all open windows */
    Object *cstate = (Object*)(((struct List*)XGET(_app(self), MUIA_Application_WindowList))->lh_Head);
    Object *prefs = (Object*) XGET(_app(self), MUIA_Wanderer_Prefs);
    Object *child;
    
    while ((child = NextObject(&cstate)))
    { 
    	if (XGET(child, MUIA_UserData))
    	{
            /* update the toolbar prefs for every open window*/
            set(child, MUIA_IconWindow_Toolbar_Enabled, XGET(data->wd_Prefs, MUIA_WandererPrefs_Toolbar_Enabled) );
            set(child, MUIA_Window_Activate, TRUE);
    	}
    } 

    return 0;
}

Object * __CreateWandererIntuitionMenu__ ( )
{
    struct NewMenu nm[] = {
    {NM_TITLE,     _(MSG_MEN_WANDERER)},
        {NM_ITEM,  _(MSG_MEN_BACKDROP),_(MSG_MEN_SC_BACKDROP), CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WANDERER_BACKDROP},
        {NM_ITEM,  _(MSG_MEN_EXECUTE), _(MSG_MEN_SC_EXECUTE) , 0                         , 0, (APTR) MEN_WANDERER_EXECUTE},

        {NM_ITEM,  _(MSG_MEN_SHELL),   _(MSG_MEN_SC_SHELL)   , 0                         , 0, (APTR) MEN_WANDERER_SHELL},
        {NM_ITEM,  _(MSG_MEN_GUISET),  NULL                  , 0                         , 0, (APTR) MEN_WANDERER_GUISETTINGS},
        {NM_ITEM,  _(MSG_MEN_ABOUT),   _(MSG_MEN_SC_ABOUT)   , 0                         , 0, (APTR) MEN_WANDERER_ABOUT},
        {NM_ITEM,  _(MSG_MEN_QUIT) ,   _(MSG_MEN_SC_QUIT)    , 0                         , 0, (APTR) MEN_WANDERER_QUIT},

    {NM_TITLE,     _(MSG_MEN_WINDOW),  NULL, NM_MENUDISABLED},

        {NM_ITEM,  _(MSG_MEN_NEWDRAW), _(MSG_MEN_SC_NEWDRAW) , 0                         , 0, (APTR) MEN_WINDOW_NEW_DRAWER},
        {NM_ITEM,  _(MSG_MEN_OPENPAR),  NULL                 , 0                         , 0, (APTR) MEN_WINDOW_OPEN_PARENT},
        {NM_ITEM,  _(MSG_MEN_CLOSE),   _(MSG_MEN_SC_CLOSE)   , 0                         , 0, (APTR) MEN_WINDOW_CLOSE},
        {NM_ITEM,  _(MSG_MEN_UPDATE),  NULL                  , 0                         , 0, (APTR) MEN_WINDOW_UPDATE},
        {NM_ITEM, NM_BARLABEL},
        {NM_ITEM, _(MSG_MEN_CONTENTS), _(MSG_MEN_SC_CONTENTS), 0                         , 0, (APTR) MEN_WINDOW_SELECT},
        {NM_ITEM,  _(MSG_MEN_CLRSEL),  _(MSG_MEN_SC_CLRSEL)  , 0                         , 0, (APTR) MEN_WINDOW_CLEAR},
        {NM_ITEM, NM_BARLABEL},
        {NM_ITEM,  _(MSG_MEN_SNAPSHT) },
        {NM_SUB,   _(MSG_MEN_WINDOW),  NULL                  , 0                         , 0, (APTR) MEN_WINDOW_SNAP_WIN},
        {NM_SUB,   _(MSG_MEN_ALL),     NULL                  , 0                         , 0, (APTR) MEN_WINDOW_SNAP_ALL},
        {NM_ITEM, NM_BARLABEL},
        {NM_ITEM,  _(MSG_MEN_VIEW)},
        {NM_SUB,   _(MSG_MEN_ICVIEW),  NULL                  , CHECKIT|CHECKED      ,8+16+32, (APTR) MEN_WINDOW_VIEW_ICON},
        {NM_SUB,   _(MSG_MEN_DCVIEW),  NULL                  , CHECKIT              ,4+16+32, (APTR) MEN_WINDOW_VIEW_DETAIL},
        {NM_SUB, NM_BARLABEL},
        {NM_SUB,   _(MSG_MEN_ALLFIL),  NULL                  , CHECKIT|MENUTOGGLE        , 0, (APTR) MEN_WINDOW_VIEW_ALL},
        {NM_ITEM,  _(MSG_MEN_SORTIC)},
        {NM_SUB,   _(MSG_MEN_CLNUP),   _(MSG_MEN_SC_CLNUP)   , 0                         , 0, (APTR) MEN_WINDOW_SORT_NOW},
        {NM_SUB, NM_BARLABEL},
        {NM_SUB,   _(MSG_MEN_BYNAME),  NULL                  , CHECKIT|CHECKED      ,8+16+32, (APTR) MEN_WINDOW_SORT_NAME},
        {NM_SUB,   _(MSG_MEN_BYDATE),  NULL                  , CHECKIT              ,4+16+32, (APTR) MEN_WINDOW_SORT_DATE},
        {NM_SUB,   _(MSG_MEN_BYSIZE),  NULL                  , CHECKIT               ,4+8+32, (APTR) MEN_WINDOW_SORT_SIZE},
      //{NM_SUB,   "..by Type",           NULL, CHECKIT,4+8+16, (APTR) MEN_WINDOW_SORT_TYPE},
        {NM_SUB, NM_BARLABEL},
        {NM_SUB,  _(MSG_MEN_REVERSE),  NULL                  , CHECKIT|MENUTOGGLE        , 0, (APTR) MEN_WINDOW_SORT_REVERSE},
        {NM_SUB,  _(MSG_MEN_DRWFRST),  NULL                  , CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WINDOW_SORT_TOPDRAWERS},
      //{NM_SUB,  "Group Icons",           NULL, CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WINDOW_SORT_GROUP},

    {NM_TITLE,    _(MSG_MEN_ICON),     NULL, NM_MENUDISABLED},
        {NM_ITEM,  _(MSG_MEN_OPEN), _(MSG_MEN_SC_OPEN), 0, 0, (APTR) MEN_ICON_OPEN},
  //    {NM_ITEM,  "Close","C" },
        {NM_ITEM,  _(MSG_MEN_RENAME), _(MSG_MEN_SC_RENAME), 0, 0, (APTR) MEN_ICON_RENAME},
        {NM_ITEM,  _(MSG_MEN_INFO), _(MSG_MEN_SC_INFO), 0, 0, (APTR) MEN_ICON_INFORMATION},
  //    {NM_ITEM,  "Snapshot", "S" },
  //    {NM_ITEM,  "Unsnapshot", "U" },
  //    {NM_ITEM,  "Leave Out", "L" },
  //    {NM_ITEM,  "Put Away", "P" },
        {NM_ITEM, NM_BARLABEL},
        {NM_ITEM,  _(MSG_MEN_DELETE), NULL, 0, 0, (APTR) MEN_ICON_DELETE},
  //    {NM_ITEM,  "Format Disk..." },
  //    {NM_ITEM,  "Empty Trash..." },

    {NM_TITLE, _(MSG_MEN_TOOLS),          NULL, NM_MENUDISABLED},
  //    {NM_ITEM,  "ResetWanderer" },
    {NM_END}
    };
    Object *menustrip = MUI_MakeObject(MUIO_MenustripNM, nm, (IPTR) NULL);
    return menustrip;
}

Object *Wanderer__MUIM_Wanderer_CreateDrawerWindow
(
    Class *CLASS, Object *self, 
    struct MUIP_Wanderer_CreateDrawerWindow *message
)
{
    SETUP_INST_DATA;
    Object *window = NULL;
    BOOL    isWorkbenchWindow = message->drawer == NULL ? TRUE : FALSE;
    BOOL    hasToolbar = XGET(data->wd_Prefs, MUIA_WandererPrefs_Toolbar_Enabled);
    

    IPTR    useFont = (IPTR)NULL;
    if (data->wd_PrefsIntern)
    {
      useFont = (IPTR)((struct WandererInternalPrefsData *)data->wd_PrefsIntern)->WIPD_IconFont;
    }

    Object *menustrip = __CreateWandererIntuitionMenu__ ( );
    
    /* Create a new icon drawer window with the correct drawer being set */
    window = IconWindowObject,
        MUIA_UserData,                     1,
        MUIA_IconWindow_Font,              useFont,
        MUIA_Window_ScreenTitle,    (IPTR) GetScreenTitle(),
        MUIA_Window_Menustrip,      (IPTR) menustrip,
        MUIA_IconWindow_ActionHook, (IPTR) &hook_action,
        
        MUIA_IconWindow_IsRoot,            isWorkbenchWindow ? TRUE : FALSE,
        /*MUIA_IconWindow_IsSubWindow,       isWorkbenchWindow ? FALSE : TRUE,*/
        MUIA_IconWindow_IsBackdrop,        isWorkbenchWindow ? TRUE : FALSE,
        MUIA_IconWindow_Toolbar_Enabled,   hasToolbar ? TRUE : FALSE,
        isWorkbenchWindow ? 
            TAG_IGNORE    : 
            MUIA_IconWindow_Drawer, (IPTR) message->drawer,
    End;
    
    if (window != NULL)
    {
        /* Get the drawer path back so we can use it also outside this function */
        STRPTR drw;
        
        if (!isWorkbenchWindow) drw = (STRPTR) XGET(window, MUIA_IconWindow_Drawer);
        else                    drw = "RAM:";
        
        /* FIXME: should remove + dispose the window (memleak!) */
        DoMethod
        (
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR) window, 3, MUIM_Set, MUIA_Window_Open, FALSE
        );
        
        DoMethod
        (
            window, MUIM_Notify, MUIA_Window_Activate, TRUE,
            (IPTR) _app(self), 3, MUIM_Set, MUIA_Wanderer_ActiveWindow, (IPTR) window
        );
        
        /* If "Execute Command" entry is clicked open the execute window */
        DoAllMenuNotifies(menustrip, drw);        
        
        /* Add the window to the application */
        DoMethod(_app(self), OM_ADDMEMBER, (IPTR) window);
        
        /* And now open it */
        DoMethod(window, MUIM_IconWindow_Open);
    }
    
    return window;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_9
(
    Wanderer, NULL, MUIC_Application, NULL,
    OM_NEW,                           struct opSet *,
    OM_DISPOSE,                       Msg,
    OM_SET,                           struct opSet *,
    OM_GET,                           struct opGet *,
    MUIM_Application_Execute,         Msg,
    MUIM_Wanderer_HandleTimer,        Msg,
    MUIM_Wanderer_HandleCommand,      Msg,
    MUIM_Wanderer_HandleNotify,       Msg,
    MUIM_Wanderer_CreateDrawerWindow, struct MUIP_Wanderer_CreateDrawerWindow *
);
