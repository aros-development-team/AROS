/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start up the ol' Dos boot process.
    Lang: english 
*/

/* Uncomment this definition in order to enable mounting
   disk-based handlers (e.g. fat.handler) at bootup.
   This code works however fat.handler itself currently has
   problems, so this option is disabled for now
#define MOUNT_DISK_HANDLERS */

# define  DEBUG 0
# include <aros/debug.h>

#include <aros/macros.h>

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <libraries/expansionbase.h>
#include <aros/asmcall.h>
#include <string.h>
#include <dos/filehandler.h>
#include <dos/filesystem.h>
#include <devices/trackdisk.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "dos_intern.h"

#define BNF_RETRY 0x8000 /* Private flag for the BootNode */

extern void boot();
extern void InitKernelBases(void);

BOOL init_hidds( struct ExecBase *, struct DosLibrary * );
BOOL mount( struct DeviceNode *dn, struct DosLibrary * ); 
BOOL isBootable( CONST_STRPTR deviceNam, struct DosLibrary * );

AROS_UFH3(void, intBoot,
    AROS_UFHA(APTR, argString, A0),
    AROS_UFHA(ULONG, argSize, D0),
    AROS_UFHA(struct ExecBase *,SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct ExpansionBase *ExpansionBase = NULL;
    struct DosLibrary    *DOSBase       = NULL;
    struct BootNode      *bootNode      = NULL;
    STRPTR                bootName;
    LONG                  bootNameLength;        
    BPTR                  lock;

#   define deviceName (((struct DosList *) bootNode->bn_DeviceNode)->dol_Ext.dol_AROS.dol_DevName)
    
    /**** Open all required libraries **********************************************/
    DOSBase       = (struct DosLibrary *)    OpenLibrary( "dos.library", 0 );
    ExpansionBase = (struct ExpansionBase *) OpenLibrary( "expansion.library", 0 );

    if( DOSBase == NULL )
    {
        D(bug( "Could not open dos.library, something's wrong!\n" ));
	Alert(AT_DeadEnd| AG_OpenLib | AN_DOSLib | AO_DOSLib);
    }
    
    if( ExpansionBase == NULL )
    {
	D(bug( "Could not open expansion.library, something's wrong!\n"));
	Alert(AT_DeadEnd | AG_OpenLib | AN_DOSLib | AO_ExpansionLib);
    }

    /* Initialise library bases used by linklib functions within the kernel */
    InitKernelBases();

    /**** Try to mount all filesystems in the MountList ****************************/
    D(bug( "Examining MountList:\n" ));
    
    ForeachNode( &ExpansionBase->MountList, bootNode )
    {
	D(bug
        (
            "Node: %p, DevNode: %p, Name = %s\n", 
            bootNode, bootNode->bn_DeviceNode,
	    deviceName ? deviceName : "(null)" 
	));
    	/* 
            Try to mount the filesystem. If it fails, mark the BootNode
	    so DOS doesn't try to boot from it later but will retry to
	    mount it after boot device is found and system directories
	    assigned.
        */ 

	if( !mount( (struct DeviceNode *) bootNode->bn_DeviceNode , 
	            (struct DosLibrary *) DOSBase))
	{
#ifdef MOUNT_DISK_HANDLERS
	    bootNode->bn_Flags |= BNF_RETRY;
#else
	    REMOVE( bootNode );
#endif
	}
	else
	    bootNode->bn_Flags &= ~BNF_RETRY;
    }
   
    /**** Try to find a bootable filesystem ****************************************/   
    while( TRUE )
    {
        ForeachNode( &ExpansionBase->MountList, bootNode )
        {
            /* 
                Check if the mounted filesystem is bootable. If it's not,
                it's probably some kind of transient error (ie. no disk
                in drive or wrong disk) so we only move it to the end of
                the list. 
            */
        
            if((!(bootNode->bn_Flags & BNF_RETRY)) && isBootable( deviceName, (struct DosLibrary *)DOSBase ) )
            {
                goto boot;
            }
        }
        
        kprintf( "No bootable disk was found.\n" );
        kprintf( "Please insert a bootable disk in any drive.\n" );

        kprintf( "Retrying in 5 seconds...\n" );
        Delay( 500 );
        
        /* FIXME: Should there be a prompt instead of a automatic retry? */        
    }

boot:

    /* Construct the complete device name of the boot device */
    bootNameLength = strlen( deviceName ) + 2;
    bootName       = AllocMem( bootNameLength, MEMF_ANY);

    if( bootName == NULL )
    {
	Alert(AT_DeadEnd | AG_NoMemory | AO_DOSLib | AN_StartMem);
    }

    strcpy( bootName, deviceName );
    strcat( bootName, ":" );
    
    kprintf("[DOS] Booting from device %s\n",bootName);
    
    /* Lock the boot device and add some default assigns */
    lock =  Lock(bootName, SHARED_LOCK);
    if (lock) DOSBase->dl_SYSLock = DupLock(lock);
        
    if ((lock != NULL) && (DOSBase->dl_SYSLock != NULL))
    {
	AssignLock("SYS", lock);
    }
    else
    {
	Alert(AT_DeadEnd | AG_BadParm | AN_DOSLib);
    }

    FreeMem( bootName, bootNameLength );
    
    lock = Lock("SYS:", SHARED_LOCK);

    if (lock != NULL)
    {
	CurrentDir(lock);
    }
    else
    {
	Alert(AT_DeadEnd | AG_BadParm | AN_DOSLib);
    }
    
    lock = Lock("SYS:C", SHARED_LOCK);

    if (lock != NULL)
    {
	AssignLock("C", lock);
    }
    
    lock = Lock("SYS:S", SHARED_LOCK);

    if (lock != NULL)
    {
	AssignLock("S", lock);
    }

    lock = Lock("SYS:Libs", SHARED_LOCK);

    if (lock != NULL)
    {
	AssignLock("LIBS", lock);
    }
    
    lock = Lock("SYS:Devs", SHARED_LOCK);

    if (lock != NULL)
    {
	AssignLock("DEVS", lock);
    }
    
    lock = Lock("DEVS:Drivers", SHARED_LOCK);
    
    if (lock != NULL)
    {
        AssignLock("DRIVERS", lock);
        AssignAdd("LIBS", lock);        /* Let hidds in DRIVERS: directory be found by OpenLibrary */
    }

    lock = Lock("SYS:L", SHARED_LOCK);
    
    if (lock != NULL)
    {
        AssignLock("L", lock);
    }

    /* Late binding ENVARC: assign, only if used */
    AssignLate("ENVARC", "SYS:Prefs/env-archive");

    /* 
        Retry to mount marked filesystems. If it fails again,
	remove the BootNode from the list.
    */ 
    ForeachNode( &ExpansionBase->MountList, bootNode )
    {
	if (bootNode->bn_Flags & BNF_RETRY) {
	    D(bug
    	    (
        	"Retry node: %p, DevNode: %p, Name = %s\n", 
        	bootNode, bootNode->bn_DeviceNode,
		deviceName ? deviceName : "(null)" 
	    ));
	    if( !mount( (struct DeviceNode *) bootNode->bn_DeviceNode , 
	                (struct DosLibrary *) DOSBase))
	    {
		REMOVE( bootNode );
	    }
	}
    }

    /* We don't need expansion.library any more */
    CloseLibrary( (struct Library *) ExpansionBase );

    /* Initialize HIDDs */
    init_hidds(SysBase, (struct DosLibrary *)DOSBase);

    /* We now call the system dependant boot - should never return. */
    AROS_UFC3(void, boot, 
	      AROS_UFCA(STRPTR, argString, A0),
	      AROS_UFCA(ULONG, argSize, D0),
	      AROS_UFCA(struct ExecBase *, SysBase, A6));

#   undef deviceName

    AROS_USERFUNC_EXIT
}

void DOSBoot(struct ExecBase *SysBase, struct DosLibrary *DOSBase)
{
    struct TagItem bootprocess[] =
    {
	{ NP_Entry,	 (IPTR) intBoot },
	{ NP_Name,	 (IPTR) "Boot Process" },
	{ NP_Input,	 (IPTR) NULL },
	{ NP_Output,	 (IPTR) NULL },
	{ NP_WindowPtr,  -1 },
	{ NP_CurrentDir, (IPTR) NULL },
	{ NP_StackSize, AROS_STACKSIZE * 2},
	{ NP_Cli,	 (IPTR) 0 },
	{ TAG_END, }
    };
    
    if( CreateNewProc( bootprocess ) == NULL )
    {
	D(bug
        (
            "CreateNewProc() failed with %ld\n",
	    ((struct Process *) FindTask( NULL ))->pr_Result2
        ));
	Alert( AT_DeadEnd | AN_DOSLib | AG_ProcCreate );
    }
}

BOOL mount( struct DeviceNode *dn, struct DosLibrary * DOSBase ) 
{
    BOOL rc; 

    if (!dn->dn_Ext.dn_AROS.dn_Device)
	rc = RunHandler(dn, DOSBase);
    else
	rc = TRUE;
    if (rc)    
    {
	if (!AddDosEntry((struct DosList *) dn))
	{
	    Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
	}
    }
    return rc;
}

BOOL isBootable( CONST_STRPTR deviceName, struct DosLibrary * DOSBase )
{
    BOOL            result = FALSE;
    BPTR            lock;
    STRPTR          buffer;
    LONG            bufferLength;
    struct InfoData info;
    
    #define STARTUP_SEQUENCE_FILE ":S/Startup-Sequence"
    
    bufferLength = strlen( deviceName ) + sizeof(STARTUP_SEQUENCE_FILE) + 1;
    
    if( (buffer = AllocMem( bufferLength, MEMF_ANY ) ) == NULL ) 
    {
        Alert( AT_DeadEnd | AG_NoMemory | AN_DOSLib );
    }
    
    strcpy( buffer, deviceName );        
    strcat( buffer, STARTUP_SEQUENCE_FILE );
    
    D(bug("Trying to get a lock on '%s'\n", buffer));
       
    if( (lock = Lock( buffer, SHARED_LOCK )) == 0 )
    {
        D(bug( "dosboot: could not lock '%s'\n", buffer ));
        goto cleanup;
    }
    
    if( !Info( lock, &info ) )
    {
        D(bug( "dosboot: could not get info on '%s'\n", buffer ));
        goto cleanup;
    }
    
    if( info.id_DiskType != ID_NO_DISK_PRESENT )
    {
        result = TRUE;
    }
   
cleanup:
    if( buffer != NULL ) FreeMem( buffer, bufferLength );
    if( lock   != 0    ) UnLock( lock );
    
    return result;
}
