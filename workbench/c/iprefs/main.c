/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 1
#include <aros/debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*********************************************************************************************/

static struct libinfo
{
    APTR        var;
    STRPTR      name;
    WORD        version;
}
libtable[] =
{
    {&IntuitionBase     , "intuition.library"           , 39    },
    {&GfxBase           , "graphics.library"            , 39    },
    {&UtilityBase       , "utility.library"             , 39    },
    {&IFFParseBase  	, "iffparse.library"	    	, 39	},
    {&LocaleBase    	, "locale.library"  	    	, 39	},
    {&KeymapBase        , "keymap.library"              , 39    },
    {&LayersBase        , "layers.library"              , 39    },
    {&DataTypesBase     , "datatypes.library"           , 39    },
    {&DiskfontBase      , "diskfont.library"            , 39    },
    {NULL                                                       }
};

/*********************************************************************************************/

static struct prefinfo
{
    STRPTR  	    	    filename;
    STRPTR  	    	    filenamebuffer;
    void    	    	    (*func)(void);
    struct NotifyRequest    nr;
    BOOL    	    	    notifystarted;
}
preftable[] =
{
    {"input"	    , inputprefsname    , NULL},
    {"font" 	    , fontprefsname     , NULL},
    {"screenmode"   , screenprefsname	, NULL},
    {"locale"	    , localeprefsname	, NULL},
    {"palette"	    , paletteprefsname  , NULL},
    {"wbpattern"    , patternprefsname  , NULL},
    {"icontrol"     , icontrolprefsname , NULL},
    {"serial"	    , serialprefsname	, NULL},
    {"printer"	    , printerprefsname	, NULL},
    {"pointer"	    , pointerprefsname  , NULL},
    {"overscan"     , overscanprefsname , NULL},
    {NULL   	    	    	    	      }
    
};

/*********************************************************************************************/

static ULONG notifysig;

/*********************************************************************************************/

static void CloseLibs(void);
static void KillNotifications(void);

/*********************************************************************************************/

WORD ShowMessage(STRPTR title, STRPTR text, STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return EasyRequestArgs(NULL, &es, NULL, NULL);  
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("IPrefs", msg, "Ok");     
	} else {
	    printf("IPrefs: %s\n", msg);
	}
    }
    
    KillNotifications();
    CloseLibs();
    
    exit(prog_exitcode);
}


/*********************************************************************************************/

static void OpenLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (!((*(struct Library **)li->var) = OpenLibrary(li->name, li->version)))
	{
	    sprintf(s, "Can't open %s V%ld!", li->name, li->version);
	    Cleanup(s);
	}       
    }
       
}

/*********************************************************************************************/

static void CloseLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (*(struct Library **)li->var) CloseLibrary((*(struct Library **)li->var));
    }
}

/*********************************************************************************************/

static void GetENVName(void)
{
    BPTR lock;
    BOOL ok = FALSE;
    
    lock = Lock("ENV:", SHARED_LOCK);
    if (lock)
    {
    	if (NameFromLock(lock, envname, 256)) ok = TRUE;
    	UnLock(lock);
    }
    
    if (!ok) Cleanup("Error expanding \"ENV:\" to full name!");
}

/*********************************************************************************************/

static void StartNotifications(void)
{
    WORD i;
    
    notifyport = CreateMsgPort();
    if (!notifyport) Cleanup("Can't create notification msg port!\n");
    
    notifysig = 1L << notifyport->mp_SigBit;
    
    for(i = 0; preftable[i].filename; i++)
    {
    	strcpy(preftable[i].filenamebuffer, envname);
	AddPart(preftable[i].filenamebuffer, "Sys", 256);
	AddPart(preftable[i].filenamebuffer, preftable[i].filename, 256);
	strcat(preftable[i].filenamebuffer, ".prefs");
	
	preftable[i].nr.nr_Name     	    	= preftable[i].filenamebuffer;
	preftable[i].nr.nr_UserData 	    	= i;
	preftable[i].nr.nr_Flags    	    	= NRF_SEND_MESSAGE; // | NRF_NOTIFY_INITIAL;
	preftable[i].nr.nr_stuff.nr_Msg.nr_Port = notifyport;
	
	D(bug("\nTrying to start notification for file \"%s\".\n", preftable[i].filenamebuffer));
	
	if (StartNotify(&preftable[i].nr))
	{
	    D(bug("Notification successfully started.\n"));
	    
	    preftable[i].notifystarted = TRUE;
	}
	else
	{
	    D(bug("Notification start failed!! Continuing anyway!\n"));
	}
	
    } /* for(i = 0; preftable[i].filename; i++) */
}

/*********************************************************************************************/

static void KillNotifications(void)
{
    WORD i;
    
    for(i = 0; preftable[i].filename; i++)
    {
    	if (preftable[i].notifystarted)
	{
	    EndNotify(&preftable[i].nr);
	    preftable[i].notifystarted = FALSE;
	}
    }
    
    if (notifyport)
    {
    	DeleteMsgPort(notifyport);
	notifyport = NULL;
	notifysig = NULL;
    }
}

/*********************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs;
    
    for(;;)
    {
    	sigs = Wait(notifysig | SIGBREAKF_CTRL_C);
	
	if (sigs & SIGBREAKF_CTRL_C) break;
	
	if (sigs & notifysig)
	{
	    struct NotifyMessage *msg;
	    
	    while((msg = (struct NotifyMessage *)GetMsg(notifyport)))
	    {
	    	D(bug("Received notify message. UserData = %d --> File = \"%s\"\n", msg->nm_NReq->nr_UserData,
		    	    	    	    	    	    	    	    	    preftable[msg->nm_NReq->nr_UserData].filenamebuffer));
		
	    	ReplyMsg(&msg->nm_ExecMessage);
		
	    } /* while((msg = (struct NotifyMessage *)GetMsg(notifyport))) */
	    
	} /* if (sigs & notifysig) */
	
    } /* for(;;) */
}

/*********************************************************************************************/

int main(void)
{
    OpenLibs();
    GetENVName();
    StartNotifications();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


