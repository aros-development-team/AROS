/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start up the ol' Dos boot process.
    Lang: english
*/


#ifdef __mc68000
/*
 * Load DEVS:system-configuration only on m68k.
 * Setup pre-2.0 boot disk colors and mouse cursors (for example)
 */
#define USE_SYSTEM_CONFIGURATION

#else

/*
 * Don't check for boot signature on m68k.
 * Original boot disks don't have it.
 */
#define AROS_BOOT_CHECKSIG ":AROS.boot"
/* Alternate variant: check if Shell is loadable
#define AROS_BOOT_CHECKEXEC ":C/Shell" */

#endif

#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#ifdef USE_SYSTEM_CONFIGURATION

#include <proto/intuition.h>

static void load_system_configuration(struct DosLibrary *DOSBase)
{
    BPTR fh;
    ULONG len;
    struct Preferences prefs;
    struct Library *IntuitionBase;
    
    fh = Open("DEVS:system-configuration", MODE_OLDFILE);
    if (!fh)
    	return;
    len = Read(fh, &prefs, sizeof prefs);
    Close(fh);
    if (len != sizeof prefs)
    	return;
    IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION);
    if (IntuitionBase)
	SetPrefs(&prefs, len, FALSE);
    CloseLibrary(IntuitionBase);
}

#else

#define load_system_configuration(DOSBase)

#endif

#include LC_LIBDEFS_FILE

#define BNF_MOUNTED 0x8000 /* Private flag for the BootNode */

#include "menu.h"
#include "dosboot_intern.h"

/*
 * This functions differs from normal DOS mount sequence in
 * that it first attempts to start a handler for the DeviceNode and
 * then adds it to DOSList only if succeeded.
 * This helps to get rid of non-functional DeviceNodes because of missing handlers.
 */
static struct MsgPort *__dosboot_Mount(struct BootNode *bootNode, struct DosLibrary * DOSBase)
{
    struct DeviceNode *dn = bootNode->bn_DeviceNode;
    struct MsgPort *rc;

    D(bug("[DOSBoot] Mounting BootNode: %p, bn_DeviceNode: %p, Name '%b', Priority %4d...",
          bootNode, dn, dn->dn_Name, bootNode->bn_Node.ln_Pri));

    /* RunHandler() is a private dos.library function */
    rc = RunHandler(dn, NULL);
    if (rc)
    {
        if (!AddDosEntry((struct DosList *) dn))
        {
            kprintf("Mounting node 0x%p (%b) failed at AddDosEntry() -- maybe it was already added by someone else!\n", dn, dn->dn_Name);
            Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
        }

        bootNode->bn_Flags |= BNF_MOUNTED;
        D(bug("Succesfully mounted %b\n", dn->dn_Name));
    }
    else
    {
        /* Since this is our private flag, make sure that noone has ocassionally set it */
        bootNode->bn_Flags &= ~BNF_MOUNTED;
        D(bug("Failed to mount %b\n", dn->dn_Name));
    }

    return rc;
}

static BOOL __dosboot_IsBootable(CONST_STRPTR deviceName, struct DosLibrary * DOSBase)
{
    BPTR lock;
    BOOL result = FALSE;
    STRPTR buffer;
    LONG nameLength, bufferLength;

    D(bug("[DOSBoot] __dosboot_IsBootable('%s')\n", deviceName));
    
    nameLength = strlen(deviceName);

#if defined(AROS_BOOT_CHECKSIG)

    bufferLength = nameLength + sizeof(AROS_BOOT_CHECKSIG) + 1;

    if ((buffer = AllocMem(bufferLength, MEMF_ANY)) == NULL)
    {
        Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
    }

    CopyMem(deviceName, buffer, nameLength);
    strcpy(&buffer[nameLength], AROS_BOOT_CHECKSIG);
    D(bug("[DOSBoot] Opening '%s'...\n", buffer));

    lock = Open(buffer, MODE_OLDFILE);

    FreeMem(buffer, bufferLength);
    buffer = NULL;

    if (lock)
    {
        LONG readsize;
	struct FileInfoBlock *abfile_fib;

	D(bug("[DOSBoot] Opened succesfully\n"));

	abfile_fib = AllocDosObject(DOS_FIB, NULL);
	if (abfile_fib)
	{
    	    if (ExamineFH(lock, abfile_fib))
    	    {
            	bufferLength = abfile_fib->fib_Size + 1;

        	buffer = AllocMem(bufferLength, MEMF_ANY);
        	D(bug("[DOSBoot] Allocated %d bytes for Buffer @ %p\n", bufferLength, buffer));

		if (!buffer)
        	{
            	    Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
        	}

	        if ((readsize = Read(lock, buffer, (bufferLength - 1))) != -1)
        	{
            	    char *sigptr = NULL;

            	    if (readsize != 0)
            	    {
                	buffer[readsize] = '\0';

            	    	D(bug("[DOSBoot] __dosboot_IsBootable: Buffer contains '%s'\n", buffer));
            	    	if ((sigptr = strstr(buffer, AROS_CPU)) != 0)
            	    	{
                	    D(bug("[DOSBoot] __dosboot_IsBootable: Signature '%s' found\n", AROS_CPU));
                	    result = TRUE;
                	}
                    }
            	}
            }
            FreeDosObject(DOS_FIB, abfile_fib);
        }
    	Close(lock);
    }

#elif defined(AROS_BOOT_CHECKEXEC)

    bufferLength = nameLength + sizeof(AROS_BOOT_CHECKEXEC) + 1;

    if ((buffer = AllocMem(bufferLength, MEMF_PUBLIC)) == NULL)
    {
        Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
    }

    CopyMem(deviceName, buffer, nameLength);
    strcpy(&buffer[nameLength], AROS_BOOT_CHECKEXEC);

    D(bug("[DOSBoot] __dosboot_IsBootable: Trying to load '%s' as an executable\n", buffer));

    if ((lock = LoadSeg(buffer)))
    {
	D(bug("[DOSBoot] Success!\n"));

    	result = TRUE;
    	UnLoadSeg(lock);
    }

#else

    /* bootable if we can lock the device */
    bufferLength = nameLength + 2;

    buffer = AllocMem(bufferLength, MEMF_ANY);
    if (buffer)
    {
    	CopyMem(deviceName, buffer, nameLength);
    	buffer[nameLength    ] = ':';
    	buffer[nameLength + 1] = 0;
    	D(bug("[DOSBoot] __dosboot_IsBootable: Trying to lock '%s'...\n", buffer));

    	if ((lock = Lock(buffer, SHARED_LOCK)))
    	{
    	    D(bug("[DOSBoot] Success!\n"));

            result = TRUE;
            UnLock(lock);
    	}
    }

#endif

    if (buffer != NULL ) FreeMem(buffer, bufferLength);

    D(bug("[DOSBoot] __dosboot_IsBootable returned %d\n", result));

    return result;
}

static void AddBootAssign(CONST_STRPTR path, CONST_STRPTR assign)
{
    BPTR lock;
    if (!(lock = Lock(path, SHARED_LOCK)))
    	lock = Lock("SYS:", SHARED_LOCK);
    if (lock)
        AssignLock(assign, lock);
}

/** Boot Code **/

AROS_UFH3(void, __dosboot_BootProcess,
    AROS_UFHA(APTR, argString, A0),
    AROS_UFHA(ULONG, argSize, D0),
    AROS_UFHA(struct ExecBase *,SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct ExpansionBase *ExpansionBase;
    struct DosLibrary *DOSBase;
    struct UtilityBase *UtilityBase;
    LIBBASETYPEPTR LIBBASE = FindTask(NULL)->tc_UserData;
    struct BootNode *bootNode = NULL;
    struct Node *tmpNode = NULL;
    STRPTR BootDevice = NULL;
    STRPTR bootName;
    LONG bootNameLength;
    BPTR lock;
    struct Screen *bootScreen = NULL;

    D(bug("[DOSBoot] __dosboot_BootProcess()\n"));

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

    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 36);
    if (!UtilityBase)
    {
        D(bug("[DOSBoot] __dosboot_BootProcess: Failed to open utility.library.\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AN_DOSLib | AO_UtilityLib);
    }

    /**** Try to mount all filesystems in the MountList ****************************/
    D(bug("[DOSBoot] Checking expansion.library/MountList for usable nodes...\n"));

    ForeachNode(&ExpansionBase->MountList, bootNode)
    {
        /* Try to mount the filesystem. If it succeeds, it marks the BootNode as mounted. */
        __dosboot_Mount(bootNode, DOSBase);
    }

    LIBBASE->delayTicks = 500;

    /**** Try to find a bootable filesystem ****************************************/
    while (BootDevice == NULL)
    {	
        ForeachNode(&ExpansionBase->MountList, bootNode)
        {
	    if (bootNode->bn_Flags & BNF_MOUNTED)
	    {
	        struct DeviceNode *dn = bootNode->bn_DeviceNode;
	        STRPTR deviceName = AROS_BSTR_ADDR(dn->dn_Name);

            	DB2(bug("[DOSBoot] Trying to boot from '%s' (priority %d)...\n", deviceName, bootNode->bn_Node.ln_Pri));

		if (UtilityBase && LIBBASE->db_BootDevice)
		{
		    /*
		     * If we have boot device specified in either command line or boot menu,
		     * we can boot up only from it regardless of its priority. Ignore everything else.
		     */
		    if (Stricmp(LIBBASE->db_BootDevice, deviceName))
		    	continue;
		}
		else
		{
		    /* Devices marked as not bootable will have priority == -128 */
		    if (bootNode->bn_Node.ln_Pri == -128)
		    	continue;
		}

            	/*
             	 * Check if the mounted filesystem is bootable. If it's not,
             	 * it's probably some kind of transient error (ie. no disk
             	 * in drive or wrong disk) so we will retry after some time.
             	 */
            	if (__dosboot_IsBootable(deviceName, DOSBase))
            	{
                    BootDevice = deviceName;
                    break;
                }
            }
        }

        if (!BootDevice)
        {
            ULONG t;

	    if (!bootScreen)
		bootScreen = NoBootMediaScreen(LIBBASE);

            DB2(kprintf("No bootable disk was found.\n"));
            DB2(kprintf("Please insert a bootable disk in any drive.\n"));
            DB2(kprintf("Retrying in 3 seconds...\n"));

	    for (t = 0; t < 150; t += LIBBASE->delayTicks)
	    {
	        Delay(LIBBASE->delayTicks);
	        anim_Animate(bootScreen, DOSBootBase);
	    }

            /*
             * Retry to mount stuff -- there might be some additional device in the meanwhile
             * (this for example happens when USB stick is inserted and a new device has been
             * added for it.
             */
            ForeachNode(&ExpansionBase->MountList, bootNode)
            {
                if (!(bootNode->bn_Flags & BNF_MOUNTED))		    
                    __dosboot_Mount(bootNode, DOSBase);
            }
        }
    }

    if (bootScreen)
    {
        anim_Stop(DOSBootBase);
	CloseBootScreen(bootScreen, LIBBASE);
    }

    if (BootDevice != NULL)
    {
	struct Library *psdBase;

        /* Construct the complete device name of the boot device */
        bootNameLength = strlen(BootDevice) + 2;

        if ((bootName = AllocMem(bootNameLength, MEMF_ANY|MEMF_CLEAR)) == NULL)
        {
            Alert(AT_DeadEnd | AG_NoMemory | AO_DOSLib | AN_StartMem);
        }

        strcpy(bootName, BootDevice);
        strcat(bootName, ":");

        D(bug("[DOSBoot] __dosboot_BootProcess: Booting from device '%s'\n", bootName));

        /* Lock the boot device and add some default assigns */
        lock =  Lock(bootName, SHARED_LOCK);
        if (lock) {
            DOSBase->dl_SYSLock = DupLock(lock);
            DOSBase->dl_Root->rn_BootProc = ((struct FileLock*)BADDR(lock))->fl_Task;
            SetFileSysTask(DOSBase->dl_Root->rn_BootProc);
        }

        if ((lock != BNULL) && (DOSBase->dl_SYSLock != BNULL))
        {
            AssignLock("SYS", lock);
        }
        else
        {
            Alert(AT_DeadEnd | AG_BadParm | AN_DOSLib);
        }

        FreeMem( bootName, bootNameLength );

        if ((lock = Lock("SYS:", SHARED_LOCK)) != BNULL)
        {
            CurrentDir(lock);
        }
        else
        {
            Alert(AT_DeadEnd | AG_BadParm | AN_DOSLib);
        }

        /*
         * If we have poseidon, ensure that ENV: exists to avoid missing volume requester.
         * We do it before other assigns because as soon as LIBS: is available it will open
         * muimaster.library and run popup GUI task.
         */
	psdBase = OpenLibrary("poseidon.library", 0);

        if (psdBase)
        {
            BPTR lock;
            	
            CloseLibrary(psdBase);

            lock = CreateDir("RAM:ENV");
            if (lock)
            {
            	/*
            	 * CreateDir() returns exclusive lock, while AssignLock() will work correctly
            	 * only with shared one.
            	 * CHECKME: Is it the same in original AmigaOS(tm), or AssignLock() needs fixing?
            	 */
            	if (ChangeMode(CHANGE_LOCK, lock, SHARED_LOCK))
                    AssignLock("ENV", lock);
                else
                    UnLock(lock);
            }
        }

        AddBootAssign("SYS:C", "C");
        AddBootAssign("SYS:S", "S");
        AddBootAssign("SYS:Libs", "LIBS");
        AddBootAssign("SYS:Devs", "DEVS");
        AddBootAssign("SYS:L", "L");
        AddBootAssign("SYS:Fonts", "FONTS");

#if !(mc68000)
        if ((lock = Lock("DEVS:Drivers", SHARED_LOCK)) != BNULL)
        {
            AssignLock("DRIVERS", lock);
            AssignAdd("LIBS", lock);        /* Let hidds in DRIVERS: directory be found by OpenLibrary */
        }
#endif

        /* Late binding ENVARC: assign, only if used */
        AssignLate("ENVARC", "SYS:Prefs/env-archive");
        load_system_configuration(DOSBase);

        /*
         * Attempt to mount filesystems which are not mounted yet.
         * Here we already can load disk-based handlers.
         * If mounting fails again, remove the BootNode from the list.
         */
	D(bug("[DOSBoot] Assigns done, retrying mounting handlers\n"));
        ForeachNodeSafe(&ExpansionBase->MountList, bootNode, tmpNode)
        {
            if (!(bootNode->bn_Flags & BNF_MOUNTED))
            {
                if (!__dosboot_Mount(bootNode, DOSBase))
                {
                    Forbid();
                    REMOVE( bootNode );
                    Permit();
                }
            }
        }
        ExpansionBase->Flags |= EBF_BOOTFINISHED;

        /* We don't need expansion.library any more */
	D(bug("[DOSBoot] Closing expansion.library\n"));
        CloseLibrary((struct Library *)ExpansionBase );

#if !(mc68000)
        /* Initialize HIDDs */
	if (!(LIBBASE->BootFlags & BF_NO_DISPLAY_DRIVERS))
	{
	    D(bug("[DOSBoot] Loading display drivers\n"));
            __dosboot_InitHidds(DOSBase);
	}
#endif
        /* We now call the system dependant boot - should NEVER return! */
	D(bug("[DOSBoot] Calling bootstrap code\n"));
        __dosboot_Boot(DOSBase, LIBBASE->BootFlags);
    }

    CloseLibrary((struct Library *)UtilityBase);
    CloseLibrary((struct Library *)DOSBase);

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
    LIBBASE->BootFlags = 0;

    bootmenu_Init(LIBBASE);

    if (CreateNewProc(bootprocess) == NULL)
    {
        D(bug("[DOSBoot] dosboot_Init: CreateNewProc() failed with %ld\n", ((struct Process *)FindTask(NULL))->pr_Result2));
        Alert( AT_DeadEnd | AN_DOSLib | AG_ProcCreate );
    }
    return TRUE;
}

ADD2INITLIB(dosboot_Init, 1)
