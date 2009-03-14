/*
    Copyright � 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start up the ol' Dos boot process.
    Lang: english
*/

#define AROS_BOOT_CHECKSIG
#define DOSBOOT_DISCINSERT_SCREENPRINT

# define  DEBUG 0
# include <aros/debug.h>

#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

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
#include <devices/trackdisk.h>

#include <string.h>
#include <stdio.h>

#include "dosboot_intern.h"

#include LC_LIBDEFS_FILE

#define BNF_RETRY 0x8000 /* Private flag for the BootNode */

extern BOOL __dosboot_InitHidds(struct ExecBase *, struct DosLibrary *);
extern void __dosboot_Boot(struct ExecBase *SysBase, BOOL hidds_ok);

/** Support Functions **/
/* Attempt to start a handler for the DeviceNode */
BOOL __dosboot_RunHandler(struct DeviceNode *deviceNode, struct DosLibrary *DOSBase)
{
    struct MsgPort *mp;
    struct IOFileSys *iofs;
    BOOL ok = FALSE;

    D(bug("[DOSBoot] __dosboot_RunHandler()\n" ));
    mp = CreateMsgPort();

    if (mp != NULL)
    {
        iofs = (struct IOFileSys *)CreateIORequest(mp, sizeof(struct IOFileSys));

        if (iofs != NULL)
        {
	    STRPTR handler;
	    struct FileSysStartupMsg *fssm;
	    ULONG fssmFlags = 0;

	    if (deviceNode->dn_Handler == NULL)
	    {
		handler = "afs.handler";
	    }
	    else
	    {
		handler = AROS_BSTR_ADDR(deviceNode->dn_Handler);
	    }

	    /* FIXME: this assumes that dol_Startup points to struct FileSysStartupMsg.
	       This is not true for plain handlers, dol_Startup is a BSTR in this case.
	       In order to make use of this we should implement direct support for
	       packet-style handlers in dos.library */
	    fssm = (struct FileSysStartupMsg *)BADDR(deviceNode->dn_Startup);
	    if (fssm != NULL)
	    {
		iofs->io_Union.io_OpenDevice.io_DeviceName = AROS_BSTR_ADDR(fssm->fssm_Device);
		iofs->io_Union.io_OpenDevice.io_Unit       = fssm->fssm_Unit;
		iofs->io_Union.io_OpenDevice.io_Environ    = (IPTR *)BADDR(fssm->fssm_Environ);
		fssmFlags = fssm->fssm_Flags;
	    }
	    iofs->io_Union.io_OpenDevice.io_DosName    = deviceNode->dn_Ext.dn_AROS.dn_DevName;
	    iofs->io_Union.io_OpenDevice.io_DeviceNode = deviceNode;

	    D(bug("[DOSBoot] __dosboot_RunHandler: Starting up %s\n", handler));
	    if (!OpenDevice(handler, 0, &iofs->IOFS, fssmFlags) ||
        	!OpenDevice("packet.handler", 0, &iofs->IOFS, fssmFlags))
	    {
		/* Ok, this means that the handler was able to open. */
		D(bug("[DOSBoot] __dosboot_RunHandler: Handler started\n"));
		deviceNode->dn_Ext.dn_AROS.dn_Device = iofs->IOFS.io_Device;
		deviceNode->dn_Ext.dn_AROS.dn_Unit = iofs->IOFS.io_Unit;
		ok = TRUE;
	    }

	    DeleteIORequest(&iofs->IOFS);
	}

	DeleteMsgPort(mp);
    }
    return ok;
}

static BOOL __dosboot_Mount(struct DeviceNode *dn, struct DosLibrary * DOSBase)
{
    BOOL rc;

    if (!dn->dn_Ext.dn_AROS.dn_Device)
    {
        D(bug("[DOSBoot] __dosboot_Mount: Attempting to mount\n"));
        rc = __dosboot_RunHandler(dn, DOSBase);
    }
    else
    {
        D(bug("[DOSBoot] __dosboot_Mount: Volume already mounted\n"));
        rc = TRUE;
    }

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
    BPTR            lock, seglist;
    STRPTR          buffer;
    LONG            bufferLength;

    D(bug("[DOSBoot] __dosboot_IsBootable('%s')\n", deviceName));

#if defined(AROS_BOOT_CHECKSIG)
#define AROSBOOTSIG_FILE ":AROS.boot"
    LONG readsize;
    struct FileInfoBlock abfile_fib;
    bufferLength = strlen(deviceName) + sizeof(AROSBOOTSIG_FILE) + 1;

    if ((buffer = AllocMem(bufferLength, MEMF_ANY)) == NULL)
    {
        Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
    }

    strcpy(buffer, deviceName);
    strcat(buffer, AROSBOOTSIG_FILE);

    if ((lock = Open(buffer, MODE_OLDFILE)) == 0)
    {
        D(bug("[DOSBoot] __dosboot_IsBootable: Failed to open '%s'\n", buffer));
        goto cleanup;
    }

    D(bug("[DOSBoot] __dosboot_IsBootable: Opened '%s'\n", buffer));
    FreeMem(buffer, bufferLength);
    buffer = NULL;

    if (ExamineFH(lock, &abfile_fib))
    {
        bufferLength = abfile_fib.fib_Size + 1;

        if ((buffer = AllocMem(bufferLength, MEMF_ANY)) == NULL)
        {
            Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
        }
        D(bug("[DOSBoot] __dosboot_IsBootable: Allocated %d bytes for Buffer @ %p\n", bufferLength, buffer));
        if ((readsize = Read(lock, buffer, (bufferLength - 1))) != -1)
        {
            IPTR sigptr = 0;

            if (readsize != 0)
                buffer[readsize] = '\0';
            else
                buffer[bufferLength - 1] = '\0';

            D(bug("[DOSBoot] __dosboot_IsBootable: Buffer contains '%s'\n", buffer));
            if ((sigptr = (IPTR)strstr(buffer, AROS_ARCHITECTURE)) != 0)
            {
                D(bug("[DOSBoot] __dosboot_IsBootable: Signature '%s' found\n", sigptr));
                result = TRUE;
            }
        }
    }
    Close(lock);
    lock = NULL;

#else
#define SHELL_FILE ":C/Shell"

    bufferLength = strlen(deviceName) + sizeof(SHELL_FILE) + 1;

    if ((buffer = AllocMem(bufferLength, MEMF_PUBLIC)) == NULL)
    {
        Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
    }

    strcpy(buffer, deviceName);
    strcat(buffer, SHELL_FILE);

    D(bug("[DOSBoot] __dosboot_IsBootable: "
        "Trying to load '%s' as an executable\n", buffer));

    if ((seglist = LoadSeg(buffer)) == (BPTR)NULL)
    {
        D(bug("[DOSBoot] __dosboot_IsBootable: could not load '%s'\n", buffer));
    }
    else
    {
        UnLoadSeg(seglist);
        result = TRUE;
    }
#endif

cleanup:
    if (buffer != NULL ) FreeMem(buffer, bufferLength);

    return result;
}

/** Boot Code **/

AROS_UFH3(void, __dosboot_BootProcess,
    AROS_UFHA(APTR, argString, A0),
    AROS_UFHA(ULONG, argSize, D0),
    AROS_UFHA(struct ExecBase *,SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct ExpansionBase        *ExpansionBase = NULL;
    struct DosLibrary           *DOSBase       = NULL;
    LIBBASETYPEPTR              LIBBASE = FindTask(NULL)->tc_UserData;

    struct BootNode             *bootNode      = NULL;
    struct Node                 *tmpNode       = NULL;
    STRPTR                      bootName;
    LONG                        bootNameLength;
    BPTR                        lock;
    BOOL                        hidds_ok;

    struct MsgPort *mp;         // Message port used with timer.device
    struct timerequest *tr = NULL;     // timer's time request message

    D(bug("[DOSBoot] __dosboot_BootProcess()\n" ));

#define deviceName (((struct DosList *) bootNode->bn_DeviceNode)->dol_Ext.dol_AROS.dol_DevName)

    /**** Open all required libraries **********************************************/
    if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0)) == NULL)
    {
        D(bug("[DOSBoot] __dosboot_BootProcess: Failed to open dos.library.\n" ));
        Alert(AT_DeadEnd| AG_OpenLib | AN_DOSLib | AO_DOSLib);
    }

    if ((ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0)) == NULL)
    {
        D(bug("[DOSBoot] __dosboot_BootProcess: Failed to open expansion.library.\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AN_DOSLib | AO_ExpansionLib);
    }

    if ((mp = CreateMsgPort()) != NULL)
    {
        if ((tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest))) != NULL)

        {
            if ((OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0)) == 0)
                #define ioStd(x) ((struct IOStdReq *)x)
                ioStd(tr)->io_Command = TR_ADDREQUEST;
            else
            {
                D(bug("[DOSBoot] __dosboot_BootProcess: Failed to open timer.device.\n"));
                DeleteMsgPort(mp);
                DeleteIORequest((struct IORequest *)tr);
                tr = NULL;
            }
        }

        if (tr == NULL)
        {
            DeleteMsgPort(mp);
        }
    }

    /**** Try to mount all filesystems in the MountList ****************************/
    D(bug("[DOSBoot] __dosboot_BootProcess: Checking expansion.library/MountList for useable nodes:\n"));

    ForeachNode(&ExpansionBase->MountList, bootNode)
    {
        D(bug("[DOSBoot] __dosboot_BootProcess: BootNode: %p, bn_DeviceNode: %p, Name '%s', Priority %4d\n",
                bootNode, bootNode->bn_DeviceNode,
                deviceName ? deviceName : "(null)",
                bootNode->bn_Node.ln_Pri
        ));
        /*
            Try to mount the filesystem. If it fails, mark the BootNode
            so DOS doesn't try to boot from it later but will retry to
            mount it after boot device is found and system directories
            assigned.
        */

        if (!(__dosboot_Mount( (struct DeviceNode *) bootNode->bn_DeviceNode ,
                    (struct DosLibrary *) DOSBase)))
        {
            bootNode->bn_Flags |= BNF_RETRY;
            D(bug("[DOSBoot] __dosboot_BootProcess: Marked '%s' as needing retry\n",
                    deviceName ? deviceName : "(null)"));
        }
        else
        {
            bootNode->bn_Flags &= ~BNF_RETRY;
            D(bug("[DOSBoot] __dosboot_BootProcess: Marked '%s' as useable\n",
                    deviceName ? deviceName : "(null)"));
        }
    }

    /**** Try to find a bootable filesystem ****************************************/
    while (LIBBASE->db_BootDevice == NULL)
    {
        ForeachNode(&ExpansionBase->MountList, bootNode)
        {
            D(bug("[DOSBoot] __dosboot_BootProcess: Trying '%s' ...\n", deviceName ? deviceName : "(null)"));
            /*  Check if the mounted filesystem is bootable. If it's not,
                it's probably some kind of transient error (ie. no disk
                in drive or wrong disk) so we only move it to the end of
                the list. */
            if ((!(bootNode->bn_Flags & BNF_RETRY)) && (bootNode->bn_Node.ln_Pri != -128) &&
		__dosboot_IsBootable(deviceName, (struct DosLibrary *)DOSBase))
            {
                LIBBASE->db_BootDevice = deviceName;
                break;
            }
        }

        if (!(LIBBASE->db_BootDevice))
        {
            /* Check if Gfx are up .. and if so show insert media animation */
            if (LIBBASE->db_attemptingboot == FALSE)
            {
#warning "TODO: Show insert disc animation !"
                LIBBASE->db_attemptingboot = TRUE;
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

            if (tr != NULL) {
                tr->tr_time.tv_secs = 5;
                tr->tr_time.tv_micro = 0;
                DoIO((struct IORequest *)tr);
            } else
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

    if (LIBBASE->db_BootDevice != NULL)
    {
        /* Construct the complete device name of the boot device */
        bootNameLength = strlen( LIBBASE->db_BootDevice) + 2;

        if ((bootName = AllocMem(bootNameLength, MEMF_ANY|MEMF_CLEAR)) == NULL)
        {
            Alert(AT_DeadEnd | AG_NoMemory | AO_DOSLib | AN_StartMem);
        }

        strcpy(bootName,  LIBBASE->db_BootDevice);
        strcat(bootName, ":");

        bug("[DOSBoot] __dosboot_BootProcess: Booting from device '%s'\n", bootName);

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
        ForeachNodeSafe(&ExpansionBase->MountList, bootNode, tmpNode)
        {
            if (bootNode->bn_Flags & BNF_RETRY)
            {
                D(bug("[DOSBoot] __dosboot_BootProcess: Retrying node: %p, DevNode: %p, Name = %s\n", bootNode, bootNode->bn_DeviceNode, deviceName ? deviceName : "(null)" ));
                if( !__dosboot_Mount((struct DeviceNode *)bootNode->bn_DeviceNode, (struct DosLibrary *)DOSBase))
                {
                    REMOVE( bootNode );
                }
            }
        }

        /* We don't need expansion.library any more */
        CloseLibrary( (struct Library *) ExpansionBase );

        /* Initialize HIDDs */
        hidds_ok = __dosboot_InitHidds(SysBase, (struct DosLibrary *)DOSBase);

        /* We now call the system dependant boot - should NEVER return! */
        __dosboot_Boot(SysBase, hidds_ok);
    }

    //We Should NEVER reach here!
#undef deviceName

    AROS_USERFUNC_EXIT
}

int dosboot_Init(LIBBASETYPEPTR LIBBASE)
{
    struct TagItem bootprocess[] =
    {
        { NP_Entry,             (IPTR) __dosboot_BootProcess    },
        { NP_Name,              (IPTR) "Boot Process"           },
        { NP_UserData,          (IPTR) LIBBASE                  },
        { NP_Input,             (IPTR) NULL                     },
        { NP_Output,            (IPTR) NULL                     },
        { NP_WindowPtr,         -1                              },
        { NP_CurrentDir,        (IPTR) NULL                     },
        { NP_StackSize,         AROS_STACKSIZE * 2              },
        { NP_Cli,               (IPTR) 0                        },
        { TAG_END,                                              }
    };

    D(bug("[DOSBoot] dosboot_Init()\n"));
    D(bug("[DOSBoot] dosboot_Init: Launching Boot Process control task ..\n"));

    LIBBASE->db_BootDevice = NULL;
    LIBBASE->db_attemptingboot = FALSE;

    if (CreateNewProc(bootprocess) == NULL)
    {
        D(bug("[DOSBoot] dosboot_Init: CreateNewProc() failed with %ld\n", ((struct Process *)FindTask(NULL))->pr_Result2));
        Alert( AT_DeadEnd | AN_DOSLib | AG_ProcCreate );
    }
    return TRUE;
}

ADD2INITLIB(dosboot_Init, 0)
