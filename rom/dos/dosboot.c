/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start up the ol' Dos boot process.
    Lang: english 
*/

/* This definition enables mounting
   disk-based handlers (e.g. fat.handler) at bootup.
   Comment it out in case if some critical bugs appear.
#define MOUNT_DISK_HANDLERS
*/

#define DOSBOOT_DISCINSERT_SCREENPRINT

# define  DEBUG 0
# include <aros/debug.h>

#include <aros/macros.h>
#include <aros/asmcall.h>

#include <proto/bootmenu.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <dos/filesystem.h>
#include <libraries/expansionbase.h>
#include <libraries/bootmenu.h>
#include <devices/trackdisk.h>

#include <string.h>

#include "dos_intern.h"

#define BNF_RETRY 0x8000 /* Private flag for the BootNode */

extern BOOL init_hidds(struct ExecBase *, struct DosLibrary *);
extern void boot();

BOOL attemptingboot = FALSE;
BOOL bootdevicefound = FALSE;

/** Support Functions **/
static BOOL __dosboot_Mount(struct DeviceNode *dn, struct DosLibrary * DOSBase)
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

static BOOL __dosboot_IsBootable(CONST_STRPTR deviceName, struct DosLibrary * DOSBase)
{
    BOOL            result = FALSE;
    BPTR            lock;
    STRPTR          buffer;
    LONG            bufferLength;
    struct InfoData info;

#define STARTUP_SEQUENCE_FILE ":S/Startup-Sequence"

    bufferLength = strlen(deviceName) + sizeof(STARTUP_SEQUENCE_FILE) + 1;

    if ((buffer = AllocMem(bufferLength, MEMF_ANY)) == NULL)
    {
        Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
    }

    strcpy(buffer, deviceName);
    strcat(buffer, STARTUP_SEQUENCE_FILE);

    D(bug("[DOS] __dosboot_IsBootable: Trying to get a lock on '%s'\n", buffer));

    if ((lock = Lock(buffer, SHARED_LOCK)) == 0)
    {
        D(bug("[DOS] __dosboot_IsBootable: could not lock '%s'\n", buffer));
        goto cleanup;
    }

    if (!Info(lock, &info))
    {
        D(bug("[DOS] __dosboot_IsBootable: could not get info on '%s'\n", buffer));
        goto cleanup;
    }

    if (info.id_DiskType != ID_NO_DISK_PRESENT)
    {
        result = TRUE;
    }

cleanup:
    if (buffer != NULL ) FreeMem(buffer, bufferLength);
    if (lock   != 0    ) UnLock(lock);

    return result;
}

/** Boot Code **/

AROS_UFH3(void, __dosboot_IntBoot,
    AROS_UFHA(APTR, argString, A0),
    AROS_UFHA(ULONG, argSize, D0),
    AROS_UFHA(struct ExecBase *,SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct ExpansionBase *ExpansionBase = NULL;
    struct DosLibrary    *DOSBase       = NULL;
    struct BootMenuBase  *BootMenuBase  = NULL;

    struct BootNode      *bootNode      = NULL;
    STRPTR                bootName;
    LONG                  bootNameLength;        
    BPTR                  lock;

    struct MsgPort *mp;         // Message port used with timer.device
    struct timerequest *tr = NULL;     // timer's time request message

#define deviceName (((struct DosList *) bootNode->bn_DeviceNode)->dol_Ext.dol_AROS.dol_DevName)

    /**** Open all required libraries **********************************************/
    if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0)) == NULL)
    {
        D(bug("[DOS] __dosboot_IntBoot: Could not open dos.library, something's wrong!\n" ));
        Alert(AT_DeadEnd| AG_OpenLib | AN_DOSLib | AO_DOSLib);
    }

    if ((ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0)) == NULL)
    {
        D(bug("[DOS] __dosboot_IntBoot: Could not open expansion.library, something's wrong!\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AN_DOSLib | AO_ExpansionLib);
    }

    if ((BootMenuBase = (struct BootMenuBase *)OpenResource("bootmenu.resource")) == NULL)
    {
        D(bug("[DOS] __dosboot_IntBoot: Could not open bootmenu.resource, something's wrong!\n"));
    }

    if ((mp = CreateMsgPort()) != NULL)
    {
        if ((tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest))) != NULL)
        {
            if ((OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0)) == 0)
            {
                #define ioStd(x) ((struct IOStdReq *)x)
                ioStd(tr)->io_Command = TR_ADDREQUEST;
                tr->tr_time.tv_secs = 5;
                tr->tr_time.tv_micro = 0;
            }
            else
            {
                D(bug("[DOS] __dosboot_IntBoot: Could not open timer.device, something's wrong!\n"));
                DeleteMsgPort(mp);
                DeleteIORequest((struct IORequest *)tr);
                tr = NULL;
            }
        }
        else
        {
            DeleteMsgPort(mp);
        }
    }

    /* Check if bootmenu is requested ... and setup base options */
    if (BootMenuBase != NULL)
        bootmenu_CheckAndDisplay();

    /**** Try to mount all filesystems in the MountList ****************************/
    D(bug("[DOS] __dosboot_IntBoot: Checking MountList for useable nodes:\n" ));

    ForeachNode(&ExpansionBase->MountList, bootNode)
    {
        D(bug("[DOS] __dosboot_IntBoot: BootNode: %p, bn_DeviceNode: %p, Name '%s', Priority %4d\n",
                bootNode, bootNode->bn_DeviceNode,
                deviceName ? deviceName : "(null)", bootNode->bn_Node.ln_Pri
        ));
        /* 
            Try to mount the filesystem. If it fails, mark the BootNode
            so DOS doesn't try to boot from it later but will retry to
            mount it after boot device is found and system directories
            assigned.
        */ 

        if( !__dosboot_Mount( (struct DeviceNode *) bootNode->bn_DeviceNode , 
                    (struct DosLibrary *) DOSBase))
        {
#ifdef MOUNT_DISK_HANDLERS
            bootNode->bn_Flags |= BNF_RETRY;
#else
            REMOVE(bootNode);
#endif
        }
        else
            bootNode->bn_Flags &= ~BNF_RETRY;
    }

    /**** Try to find a bootable filesystem ****************************************/   
    while (bootdevicefound == FALSE)
    {
        ForeachNode(&ExpansionBase->MountList, bootNode)
        {
            /*  Check if the mounted filesystem is bootable. If it's not,
                it's probably some kind of transient error (ie. no disk
                in drive or wrong disk) so we only move it to the end of
                the list. */
            if ((!(bootNode->bn_Flags & BNF_RETRY)) && __dosboot_IsBootable( deviceName, (struct DosLibrary *)DOSBase ) )
            {
                bootdevicefound = TRUE;
                break;
            }
        }

        if (!bootdevicefound)
        {
            if (!attemptingboot)
            {
#warning "TODO: Show insert disc animation !"
                attemptingboot = TRUE;
            }
            else
            {
#warning "TODO: re-run insert disc animation !"
            }
#if defined(DOSBOOT_DISCINSERT_SCREENPRINT)
            kprintf("No bootable disk was found.\n");
            kprintf("Please insert a bootable disk in any drive.\n");

            kprintf("Retrying in 5 seconds...\n");
#endif

            if (tr != NULL)
                DoIO((struct IORequest *)tr);
            else
                Delay(500);
        }
    }

    if (mp)
        DeleteMsgPort(mp);

    if (tr)
    {
        CloseDevice((struct IORequest *)tr);
        DeleteIORequest((struct IORequest *)tr);
    }

    if (bootdevicefound)
    {
        /* Construct the complete device name of the boot device */
        bootNameLength = strlen(deviceName) + 2;

        if ((bootName = AllocMem(bootNameLength, MEMF_ANY|MEMF_CLEAR)) == NULL)
        {
            Alert(AT_DeadEnd | AG_NoMemory | AO_DOSLib | AN_StartMem);
        }

        strcpy(bootName, deviceName);
        strcat(bootName, ":");

        D(bug("[DOS] __dosboot_IntBoot: Booting from device '%s'\n", bootName));

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

        if ((lock = Lock("SYS:", SHARED_LOCK)) != NULL)
        {
            CurrentDir(lock);
        }
        else
        {
            Alert(AT_DeadEnd | AG_BadParm | AN_DOSLib);
        }

        if ((lock = Lock("SYS:C", SHARED_LOCK)) != NULL)
        {
            AssignLock("C", lock);
        }

        if ((lock = Lock("SYS:S", SHARED_LOCK)) != NULL)
        {
            AssignLock("S", lock);
        }

        if ((lock = Lock("SYS:Libs", SHARED_LOCK)) != NULL)
        {
            AssignLock("LIBS", lock);
        }

        if ((lock = Lock("SYS:Devs", SHARED_LOCK)) != NULL)
        {
            AssignLock("DEVS", lock);
        }

        if ((lock = Lock("DEVS:Drivers", SHARED_LOCK)) != NULL)
        {
            AssignLock("DRIVERS", lock);
            AssignAdd("LIBS", lock);        /* Let hidds in DRIVERS: directory be found by OpenLibrary */
        }

        if ((lock = Lock("SYS:L", SHARED_LOCK)) != NULL)
        {
            AssignLock("L", lock);
        }

        /* Late binding ENVARC: assign, only if used */
        AssignLate("ENVARC", "SYS:Prefs/env-archive");

        /* 
            Attempt to mount filesystems marked for retry. If it fails again,
            remove the BootNode from the list.
        */
        ForeachNode(&ExpansionBase->MountList, bootNode)
        {
            if (bootNode->bn_Flags & BNF_RETRY)
            {
                D(bug("[DOS] __dosboot_IntBoot: Retrying node: %p, DevNode: %p, Name = %s\n", bootNode, bootNode->bn_DeviceNode, deviceName ? deviceName : "(null)" ));
                if( !__dosboot_Mount((struct DeviceNode *)bootNode->bn_DeviceNode, (struct DosLibrary *)DOSBase))
                {
                    REMOVE( bootNode );
                }
            }
        }

        /* We don't need expansion.library any more */
        CloseLibrary( (struct Library *) ExpansionBase );

        /* Initialize HIDDs */
        init_hidds(SysBase, (struct DosLibrary *)DOSBase);

        /* We now call the system dependant boot - should NEVER return! */
        AROS_UFC3(void, boot, 
              AROS_UFCA(STRPTR, argString, A0),
              AROS_UFCA(ULONG, argSize, D0),
              AROS_UFCA(struct ExecBase *, SysBase, A6));
    }

    //We Should NEVER reach here!
#undef deviceName

    AROS_USERFUNC_EXIT
}

void DOSBoot(struct ExecBase *SysBase, struct DosLibrary *DOSBase)
{
    struct TagItem bootprocess[] =
    {
        { NP_Entry,	        (IPTR) __dosboot_IntBoot    },
        { NP_Name,	        (IPTR) "Boot Process"       },
        { NP_Input,	        (IPTR) NULL                 },
        { NP_Output,	    (IPTR) NULL                 },
        { NP_WindowPtr,     -1                          },
        { NP_CurrentDir,    (IPTR) NULL                 },
        { NP_StackSize,     AROS_STACKSIZE * 2          },
        { NP_Cli,	        (IPTR) 0                    },
        { TAG_END,                                      }
    };
    
    if (CreateNewProc(bootprocess) == NULL)
    {
        D(bug("[DOS] DOSBoot: CreateNewProc() failed with %ld\n", ((struct Process *)FindTask(NULL))->pr_Result2));
        Alert( AT_DeadEnd | AN_DOSLib | AG_ProcCreate );
    }
}
