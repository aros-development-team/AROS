/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <dos/dostags.h>

#define DEBUG 0
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
    BOOL    	nocloseafterpatch;
}
libtable[] =
{
    {&IntuitionBase     , "intuition.library"           , 39, FALSE },
    {&GfxBase           , "graphics.library"            , 39, FALSE },
    {&UtilityBase       , "utility.library"             , 39, TRUE  },
    {&IFFParseBase  	, "iffparse.library"	    	, 39, FALSE },
    {&LocaleBase    	, "locale.library"  	    	, 39, TRUE  },
    {&KeymapBase        , "keymap.library"              , 39, FALSE },
    {&LayersBase        , "layers.library"              , 39, FALSE },
    {&DataTypesBase     , "datatypes.library"           , 39, FALSE },
    {&DiskfontBase      , "diskfont.library"            , 39, FALSE },
    {NULL   	    	    	    	    	    	    	    }
};

/*********************************************************************************************/

static struct prefinfo
{
    STRPTR  	    	    filename;
    STRPTR  	    	    filenamebuffer;
    void    	    	    (*func)(STRPTR);
    struct NotifyRequest    nr;
    BOOL    	    	    notifystarted;
}
preftable[] =
{
    {"input"	    , inputprefsname    , NULL	    	    	},
    {"font" 	    , fontprefsname     , FontPrefs_Handler   	},
    {"screenmode"   , screenprefsname	, NULL	    	    	},
    {"locale"	    , localeprefsname	, LocalePrefs_Handler	},
    {"palette"	    , paletteprefsname  , NULL	    	    	},
    {"wbpattern"    , patternprefsname  , NULL	    	    	},
    {"icontrol"     , icontrolprefsname , NULL	    	    	},
    {"serial"	    , serialprefsname	, SerialPrefs_Handler 	},
    {"printer"	    , printerprefsname	, NULL	    	    	},
    {"pointer"	    , pointerprefsname  , NULL	    	    	},
    {"overscan"     , overscanprefsname , NULL	    	    	},
    {NULL   	    	    	    	      	    	    	}

};

/*********************************************************************************************/

static ULONG 	    notifysig;

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

LONG            __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process              = NULL;

void DoDetach(void)
{
    /* If there's a detacher, tell it to go away */
    if (__detacher_process)
    {
        Signal((struct Task *)__detacher_process, __detacher_must_wait_for_signal);
    }
}

/*********************************************************************************************/
void Cleanup(STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase /* && !((struct Process *)FindTask(NULL))->pr_CLI */)
	{
	    ShowMessage("IPrefs", msg, "Ok");
	}
    }


    KillNotifications();
    CloseLibs();
    DoDetach();

    exit(prog_exitcode);
}


/*********************************************************************************************/

static void OpenLibs(void)
{
    struct libinfo *li;

    for (li = libtable; li->var; li++)
    {
	if (!((*(struct Library **)li->var) = OpenLibrary(li->name,
							  li->version)))
	{
	    sprintf(s, "Can't open %s V%d!", li->name, li->version);
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
    	if (!patches_installed || !li->nocloseafterpatch)
	{
	    if (*(struct Library **)li->var) CloseLibrary((*(struct Library **)li->var));
	}
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
	preftable[i].nr.nr_Flags    	    	= NRF_SEND_MESSAGE | NRF_NOTIFY_INITIAL;
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

static void PreparePatches(void)
{
    struct IPrefsSem 	   *sem;
    BOOL    	    	   created_sem = FALSE;

    sem = AllocVec(sizeof(struct IPrefsSem), MEMF_PUBLIC | MEMF_CLEAR);
    if (!sem) Cleanup("Out of memory!");

    InitSemaphore(&sem->sem);
    sem->sem.ss_Link.ln_Name = sem->semname;
    strcpy(sem->semname, IPREFS_SEM_NAME);

    Forbid();
    if(!(iprefssem = (struct IPrefsSem *)FindSemaphore(IPREFS_SEM_NAME)))
    {
    	iprefssem = sem;
	AddSemaphore(&iprefssem->sem);
	
	created_sem = TRUE;
    }
    Permit();
    
    if (created_sem)
    {
    	InstallPatches();
    }
    else
    {
     	FreeVec(sem);
    }
        
}

/*********************************************************************************************/

static void HandleNotify(void)
{
    struct NotifyMessage *msg;

    while((msg = (struct NotifyMessage *)GetMsg(notifyport)))
    {
	WORD id = msg->nm_NReq->nr_UserData;

	D(bug("Received notify message. UserData = %d --> File = \"%s\"\n", id,
		    	    	    	    	    	    	    	    preftable[id].filenamebuffer));

	if (preftable[id].func)
	{
	    preftable[id].func(preftable[id].filenamebuffer);
	}

	ReplyMsg(&msg->nm_ExecMessage);
	
    } /* while((msg = (struct NotifyMessage *)GetMsg(notifyport))) */
}

/*********************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs;

    for(;;)
    {
	sigs = Wait(notifysig | SIGBREAKF_CTRL_C);

	if (sigs & SIGBREAKF_CTRL_C) break;

	if (sigs & notifysig) HandleNotify();

    } /* for(;;) */
}


/*********************************************************************************************/


int __nocommandline = 1;
STRPTR __detached_name = IPREFS_SEM_NAME;

int main(void)
{
    OpenLibs();
    GetENVName();
    StartNotifications();
    PreparePatches();
    HandleNotify();
    DoDetach();
    HandleAll();
    Cleanup(NULL);

    return 0;
}

/*********************************************************************************************/


