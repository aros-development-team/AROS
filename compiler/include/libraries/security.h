/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: definitions for the security.library
    Lang: english
*/
#ifndef _SECURITY_H
#define _SECURITY_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif  /* EXEC_TYPES_H */

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif  /* EXEC_LISTS_H */

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif  /* EXEC_LIBRARIES_H */

#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif  /* EXEC_EXECBASE_H */

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif  /* EXEC_PORTS_H */

#ifndef LIBRARIES_DOS_H
#include <libraries/dos.h>
#endif  /* LIBRARIES_DOS_H */

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif  /* UTILITY_TAGITEM_H */

#define DEBUG                           1

#define TASKHASHVALUE	                23
#define MEMHASHVALUE                    101

/*
    Reserved users/groups

        WARNING: a uid may NOT be 65535!
*/

#define secOWNER_SYSTEM                 0x00000000                  /* always owner */
#define secOWNER_NOBODY                 0xffffffff                  /* no rights */

#define secMASK_UID                     0xffff0000                  /* Mask for uid bits */
#define secMASK_GID                     0x0000ffff                  /* Mask for gid bits */

#define secROOT_UID                     0x0000                      /* super user uid */
#define secROOT_GID                     0x0000                      /* super user gid */

#define secNOBODY_UID                   0xffff                      /* nobody uid */

#define secUSERIDSIZE                   32                          /* Maximum size for a User ID */
#define secGROUPIDSIZE                  32                          /* Maximum size for a Group ID */
#define secPASSWORDSIZE                 32                          /* Maximum size for a Password */
#define secUSERNAMESIZE                 220                         /* Maximum size for a User Name */
#define secGROUPNAMESIZE                220                         /* Maximum size for a Group Name */
#define secHOMEDIRSIZE                  256                         /* Maximum size for a Home Directory */
#define secSHELLSIZE                    256                         /* Maximum size for a Shell */

/*
    Tags for secLogoutA(), secLoginA() & secSetDefProtectionA()
*/

#define secT_Input                      (TAG_USER+1)                /* filehandle - default is Input() */
#define secT_Output                     (TAG_USER+2)                /* filehandle - default is Output() */
#define secT_Graphical                  (TAG_USER+3)                /* boolean - default is FALSE */
#define secT_PubScrName                 (TAG_USER+4)                /* name of public screen */
#define secT_Task                       (TAG_USER+5)                /* task (NOT the name!!) */
#define secT_Own                        (TAG_USER+6)                /* make a task owned by this user */
#define secT_Global                     (TAG_USER+7)                /* change it for all tasks on the */
                                                                    /*  same level as this one */
#define secT_Quiet                      (TAG_USER+8)                /* for secLogoutA(), don't give a */
                                                                    /*  login prompt, simply logout */
#define secT_UserID                     (TAG_USER+9)                /* UserID for secLoginA() */
#define secT_Password                   (TAG_USER+10)               /* Password for secLoginA(), secst */
                                                                    /*  be combined with secT_UserID!! */
#define secT_DefProtection              (TAG_USER+11)               /* Default protection bits */
                                                                    /*  default is RWED GROUP R OTHER R */
#define secT_All                        (TAG_USER+12)               /* for secLogoutA(), logout until */
                                                                    /*  user stack is empty */
#define secT_NoLog                      (TAG_USER+13)               /* for secLoginA(), only root */

/*
    KeyTypes for secGetUserInfo() and secGetGroupInfo()
*/

#define secKeyType_First                (0)
#define secKeyType_Next                 (1)
#define secKeyType_gid                  (4)

/*
    KeyTypes for secGetUserInfo() only
*/

#define secKeyType_UserID               (2)                         /* Case-sensitive */
#define secKeyType_uid                  (3)
#define secKeyType_gidNext              (5)
#define secKeyType_UserName             (6)                         /* Case-insensitive */
#define secKeyType_WUserID              (7)                         /* Case-insensitive, wild cards allowed */
#define secKeyType_WUserName            (8)                         /* Case-insensitive, wild cards allowed */
#define secKeyType_WUserIDNext          (9)
#define secKeyType_WUserNameNext        (10)

/*
    KeyTypes for secGetGroupInfo() only
*/

#define secKeyType_GroupID              (11)                        /* Case-sensitive */
#define secKeyType_WGroupID             (12)                        /* Case-insensitive, wild cards allowed */
#define secKeyType_WGroupIDNext         (13)
#define secKeyType_GroupName            (14)                        /* Case-insensitive */
#define secKeyType_WGroupName           (15)                        /* Case-insensitive, wild cards allowed */
#define secKeyType_WGroupNameNext       (16)
#define secKeyType_MgrUid               (17)
#define secKeyType_MgrUidNext           (18)

/*
    Protection bits (see also <dos/dos.h> :-)
*/

#define secFIBB_SET_UID                 (31)                        /* Change owner during execution */
#define secFIBB_SET_GID                 (30)                        /* Change group during execution - not yet implemented */

#define secFIBF_SET_UID                 (1<<secFIBB_SET_UID)
#define secFIBF_SET_GID                 (1<<secFIBB_SET_GID)

/*
    Default Protection Bits
*/

#define DEFPROTECTION                   (FIBF_OTR_READ | FIBF_GRP_READ)


/*
    Relations returned by secGetRelationshipA()
*/

#define secRelB_ROOT_UID                (0)                         /* User == super user */
#define secRelB_ROOT_GID                (1)                         /* User belongs to the super user group */
#define secRelB_NOBODY                  (2)                         /* User == nobody */
#define secRelB_UID_MATCH               (3)                         /* User == owner */
#define secRelB_GID_MATCH               (4)                         /* User belongs to owner group */
#define secRelB_PRIM_GID                (5)                         /* User's primary group == owner group */
#define secRelB_NO_OWNER                (6)                         /* Owner == nobody */

#define secRelF_ROOT_UID                (1<<secRelB_ROOT_UID)
#define secRelF_ROOT_GID                (1<<secRelB_ROOT_GID)
#define secRelF_NOBODY                  (1<<secRelB_NOBODY)
#define secRelF_UID_MATCH               (1<<secRelB_UID_MATCH)
#define secRelF_GID_MATCH               (1<<secRelB_GID_MATCH)
#define secRelF_PRIM_GID                (1<<secRelB_PRIM_GID)
#define secRelF_NO_OWNER                (1<<secRelB_NO_OWNER)

/*
    Monitor Modes
*/

#define secMon_IGNORE                   (0)
#define secMon_SEND_SIGNAL              (1)
#define secMon_SEND_MESSAGE             (2)

/*
    Monitor Triggers
*/

#define secTrgB_OwnerChange             (0)                         /* Task Owner Change */
                                                                    /*    From:    uid of old user */
                                                                    /*    To:      uid of new user */
#define secTrgB_Login                   (1)                         /* successful Login/Logout */
                                                                    /*    From:    uid of old user */
                                                                    /*    To:      uid of new user */
                                                                    /*    UserID:  UserID of new user */
#define secTrgB_LoginFail               (2)                         /* unsuccessful Login/Logout */
                                                                    /*    From:    uid of old user */
                                                                    /*    UserID:  UserID of new user */
#define secTrgB_Passwd                  (3)                         /* successful Passwd */
                                                                    /*    From:    uid of user */
#define secTrgB_PasswdFail              (4)                         /* unsuccessful Passwd */
                                                                    /*    From:    uid of user */
#define secTrgB_CheckPasswd             (5)                         /* successful CheckPasswd */
                                                                    /*    From:    uid of user */
#define secTrgB_CheckPasswdFail         (6)                         /* unsuccessful CheckPasswd */
                                                                    /*    From:    uid of user */

#define secTrgF_OwnerChange             (1<<secTrgB_OwnerChange)
#define secTrgF_Login                   (1<<secTrgB_Login)
#define secTrgF_LoginFail               (1<<secTrgB_LoginFail)
#define secTrgF_Passwd                  (1<<secTrgB_Passwd)
#define secTrgF_PasswdFail              (1<<secTrgB_PasswdFail)
#define secTrgF_CheckPasswd             (1<<secTrgB_CheckPasswd)
#define secTrgF_CheckPasswdFail         (1<<secTrgB_CheckPasswdFail)


/* Public secMemInfo structure. Don't expect it to remain */

struct secMemInfo 
{
	APTR                            Address;
	ULONG                           Size;
	struct secMemInfo               *next;
};


/* This is what you get a pointer to from secLocksecBase() */

struct secPointers
{
	struct MinList                  *Monitors;
	struct MinList                  *Segments;
	struct MinList                  *Sessions;
	struct MinList                  *Tasks;
	struct secVolume                *Volumes;
};

/*
    Public User Information Structure


        For future compatibility, you should ALWAYS use secAllocUserInfo()
        to allocate this structure. NEVER do it by yourself!!
*/

struct secUserInfo 
{
   char                                 UserID[secUSERIDSIZE];
   UWORD                                uid;
   UWORD                                gid;
   char                                 UserName[secUSERNAMESIZE];
   char                                 HomeDir[secHOMEDIRSIZE];
   UWORD                                NumSecGroups;               /* Number of Secondary Groups this */
                                                                    /*  user belongs to */
   UWORD                                *SecGroups;                 /* Points to an array of NumSecGroups */
                                                                    /*  Secondary Group Numbers */
   char                                 Shell[secSHELLSIZE];
};


/*
    Public Group Information Structure


        For future compatibility, you should ALWAYS use secAllocGroupInfo()
        to allocate this structure. NEVER do it by yourself!!
*/

struct secGroupInfo 
{
   char                                 GroupID[secGROUPIDSIZE];
   UWORD                                gid;
   UWORD                                MgrUid;                     /* Manager of this group */
   char                                 GroupName[secGROUPNAMESIZE];
};

/*
    Extended Owner Information Structure

        A pointer to this structure is returned by secGetTaskExtOwner().
        You MUST use secFreeExtOwner() to deallocate it!!
*/

struct secExtOwner 
{
   UWORD                                uid;
   UWORD                                gid;
   UWORD                                NumSecGroups;               /* Number of Secondary Groups this */
                                                                    /* user belongs too. */
};

/*
    Monitor Structure

        The use of this structure is restricted to root.
        Do not modify or reuse this structure while it is active!
*/

struct secMonitor 
{
   struct MinNode                       Node;
   ULONG                                Mode;                       /* see definitions below */
   ULONG                                Triggers;                   /* see definitions below */
   union
   {
      struct
      {                                                             /* for SEND_SIGNAL */
         struct Task                    *Task;
         ULONG                          SignalNum;
      } Signal;

      struct
      {                                                             /* for SEND_MESSAGE */
         struct MsgPort                 *Port;
      } Message;
   } u;

   /* NOTE: This structure may be extended in future! */
};

/*
    Monitor Message


        Sent to the application if SEND_MESSAGE is specified.
        Do NOT forget to reply!
*/

struct secMonMsg 
{
   struct Message ExecMsg;
   struct secMonitor *Monitor;                                      /* The monitor that sent the message */
   ULONG Trigger;                                                   /* The trigger that caused the message */
   UWORD From;
   UWORD To;
   char UserID[secUSERIDSIZE];
};

/****************************************************************************

                -------------- Plugins ------------------

 ****************************************************************************/

/* All plugins have this suffix */

#define secPLUGIN_SUFFIX                ".secp"

/* Return values for plugin functions */
#define secpiTRUE	                0UL	                    /* operation completed with success */
#define secpiFALSE	                1UL	                    /* operation completed but failed: handling stops at this stage */
#define secpiASYNC	                2UL	                    /* operation will continue asynchronously and notify
									the system when complete.  The system will provide
									the secSPacket that prompted the call and this
									should be passed to the completion function. */
#define secpiFALSECONT	                3UL	                    /* operation completed but failed; the system may continue with other handlers
									to see if they can satisfy the request */
#define secpiNOTSUPP	                4UL	                    /* Operation/Handler not supported */

/* 
    Plugins overview -:

        A plugin is a special form of executable.

        The plugin should return a pointer to a 'Plugin' which describes what kind
        of plugin it is, so that secFS can add it to the relevant list internally.

        A plugin is recognized by a magic marker within the first 256 bytes of the
        file.

        All plugins are kept in the config dir, so the the library doesn't have to
        search paths to find them.

        The plugin is free to allocate memory/spawn processes (they will be
        super-user priveleged) etc. without fear of being UnLoadSeg'd without the
        library first calling its Incompatible function, which should cause the plugin
        to free its resources.  It will them be UnLoadSeg'd.  Note that this only
        occurs when a plugin is rejected for some reason (wrong version ?).

 */

#define secFS_PLUGIN_IFACE1		1UL
#define secFS_PLUGIN_INTERFACE	        SEC_PLUGIN_IFACE1

#ifndef MAKE_ID /* also defined in <libraries/iffparse.h> */
#define MAKE_ID(a,b,c,d)                ((ULONG)(a)<<24 | (ULONG)(b)<<16 | (ULONG)(c)<<8 | (ULONG)(d))
#endif

#define secPLUGIN_RECOGNITION	        MAKE_ID('m', 'S', 'p', 'I')

/*
    Function called to initialize the plugin.
    The plugin should perform any initialization it needs, and then formally
    register itself with the security.library.
    module should be passed to the security.library when a handler is registered.
*/


/* Plugin Module Definition - secst be located in the executeable */
struct secPluginHeader
{
    ULONG                               plugin_magic;               /* This should be secPLUGIN_RECOGNITION */
    /* Module Descriptor */
    ULONG	                        Version;	            /* = SEC_PLUGIN_INTERFACE */
    APTR                                Initialize;
    APTR                                Terminate;
//    secInitPluginFunc                 Initialize;
//    secTerminatePluginFunc            Terminate;
};

/* internal plugin records */
typedef struct
{
    struct MinNode                      Node;
    ULONG                               reference_count;
    BPTR                                SegList;
    struct secPluginHeader *            header;	                    /* For locating the init/fini functions */
    UBYTE                               modulename[64];	            /* For displaying */
} secPluginModule;

struct plugin_ops
{
	ULONG	                        HandlerType;	            /* = ID_PLUGIN_XXX */
	secPluginModule *               module;		            /* As provied by the Initialization function  */
};

/* Encryption plugin */

#define ID_PLUGIN_ENCRYPTION	        MAKE_ID('c', 'r', 'p', 't')
struct plugin_crypt_ops	
{
	struct plugin_ops               ops;
	//ULONG                         (*Crypt)(STRPTR buffer, STRPTR key, STRPTR setting);                  /* Encrypt the supplied text using the supplied key and place the result in buffer */
	//ULONG                         (*MaxPwdLen)(void);                                                   /* Return the maximum length of an encrypted password */
	//ULONG                         (*CheckPassword)(STRPTR userid, STRPTR thepass, STRPTR suppliedpass); /* Check a password to see if it is valid */
	//ULONG                         (*EncryptPassword)(STRPTR buffer, STRPTR userid, STRPTR thepass);     /* Encrypt Password for storing in a userdb */
};

/* User DataBase Operations Plugin */

#define ID_PLUGIN_USER_DATABASE	        MAKE_ID('u', 'r', 'd', 'b')

struct sec_passwd   /* Compatible with standard unix struct passwd */	
{
	char                            *pw_name;                   /* user name */
	char                            *pw_passwd;                 /* user password */
	int                             pw_uid;                     /* user id */
	int                             pw_gid;                     /* group id */
	char                            *pw_gecos;                  /* real name */
	char                            *pw_dir;                    /* home directory */
	char                            *pw_shell;                  /* shell program */
};

struct plugin_userdb_ops	
{
	struct plugin_ops               ops;
//	ULONG                           (*getpwent)(void);
};

/* Volumes */

struct secVolume 
{
    struct secVolume                     *Next;
    struct DosList                      *DosList;			/* DosList for this Volume */
    struct MsgPort                      *Process;			/* Process for this Volume */
    
    /* Extensions from MUFS2 */
    
    LONG	                        FS_Flags;			/* Allow set{g,u}id, read-only etc. */
    
    /* If FS_Flags == 0, the rest of this structure is ignored; this indicates
	* that the volume is a true MUFS compatable volume */
    
    LONG	                        RootProtection;		        /* Permissions for the root dir */
    ULONG	                        RootOwner;	                /* UID:GID of owner of root dir */

    STRPTR FS_Name;						        /* So we dont re-run on the same volume */
    struct MsgPort*	                OrigProc;		        /* The real FS */
    struct MsgPort*	                RepPort;			/* For talking with the real FS */	
    struct MinList	                FHCache[TASKHASHVALUE];	        /* HashList of cached FileHandles */
    struct FileInfoBlock                *fib;
    LONG	                        PassKey;			/* If non-zero, contains the 32 bit passkey
									    needed to write enable the filesystem 
									    with ACTION_WRITE_PROTECT */
    /* proxy enforcer */
    struct MinList                      ProxyLocks;	                /* List of locks */
    struct MinList                      ProxyHandles[TASKHASHVALUE];	/* HashList of proxy filehandles */
    struct DeviceNode                   *ProxyDosList;	                /* Dos entry created by the proxy filesystem */
    struct DeviceList                   *ProxyDosListVolume;	        /* Dos entry created by the proxy filesystem */
    ULONG                               LockCount;			/* Number of proxy locks in existence */

};

/* MultiUser Configuration */

struct secConfig
{
    ULONG                               Flags;				/* See definitions below */
    ULONG                               LogFlags;			/* See definitions below */
    UWORD                               PasswduidLevel;			/* Lowest uid for users who can */
									/* change their passwords */
    UWORD                               PasswdgidLevel;			/* Lowest gid for users who can */
									/* change their passwords */
};

/****************************************************************************

                -------------- Library Base  ------------------

 ****************************************************************************/


struct secBase 
{
    struct Library                      LibNode;
    UBYTE                               Flags;
    UBYTE                               Pad;
    BPTR                                SegList;

	    /* The Server's Process */

    struct Process                      *Server;

	    /* The Server's Packet MsgPort */

    struct MsgPort                      *ServerPort;

	    /* List of sessions */

    struct MinList                      SessionsList;

	    /* List of Tasks and their Owner(s) */

    struct SignalSemaphore              TaskOwnerSem;
    struct MinList                      TaskOwnerList[TASKHASHVALUE];

	    /* List of memory chunks and address, size, owner.
                It's using the tasksemaphore since one usually wants that one
                too when dealing with this list. */

/*	struct MinList                  MemOwnerList[MEMHASHVALUE];*/

	    /* List of Segments and their Owner */

    struct SignalSemaphore              SegOwnerSem;
    struct MinList                      SegOwnerList;

    /* Configuration */

    struct secConfig                    Config;

    /* Signals for Passwd File Notification and Consistency Check */

    ULONG                               NotifySig;
    ULONG                               ConsistencySig;

    /* Security violation flag */

    BOOL                                SecurityViolation;

    /* MultiUser Volumes */

    struct SignalSemaphore              VolumesSem;
    struct secVolume                    *Volumes;

    /* Monitoring */

    struct SignalSemaphore              MonitorSem;
    struct MinList                      MonitorList;
    struct MsgPort                      *MonitorPort;

    /* Task Control */

    struct MinList                      Frozen;
    struct MinList                      Zombies;

    /* LocaleInfo for logfile */

    APTR                                LogInfo;

    /* You must get this one if you intend to get more than one sem. */

    struct SignalSemaphore              SuperSem;

    /* Plugins */
    
    struct SignalSemaphore              PluginModuleSem;
    struct MinList                      PluginModuleList;	/* List of loaded plugin modules */

/** OBSOLETE DEFINITIONS (or they will be once we integrate into aros..) */
	    /* Old AddTask()/RemTask() */

//    AddTaskFunc		                OLDAddTask;
//    RemTaskFunc		                OLDRemTask;

	    /* Old AllocMem()/FreeMem() */
	
//    AllocMemFunc	                OLDAllocMem;
//    FreeMemFunc		                OLDFreeMem;

	    /* Old LoadSeg()/NewLoadSeg()/UnLoadSeg()/InternalLoadSeg()/
                InternalUnLoadSeg()/CreateProc()/CreateNewProc()/RunCommand()/
                SetProtection()	*/

//    LoadSegFunc				OLDLoadSeg;
//    NewLoadSegFunc			OLDNewLoadSeg;
//    UnLoadSegFunc			OLDUnLoadSeg;
//    InternalLoadSegFunc		        OLDInternalLoadSeg;
//    InternalUnLoadSegFunc	        OLDInternalUnLoadSeg;
//    CreateProcFunc			OLDCreateProc;
//    CreateNewProcFunc			OLDCreateNewProc;
//    RunCommandFunc			OLDRunCommand;
//    SetProtectionFunc			OLDSetProtection;
};

#endif /* _SECURITY_H */
