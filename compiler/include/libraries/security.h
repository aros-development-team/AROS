#ifndef LIBRARIES_SECURITY_H
#define LIBRARIES_SECURITY_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: public definitions for security.library

    security.library is the AROS multi-user subsystem. Its API is derived
    from the MultiUserFileSystem (MuFS) multiuser.library by Geert
    Uytterhoeven and the MultiUser 2 / security.library work by Wez Furlong
    and Richard Smith; all identifiers use the "sec" prefix.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define SECURITYNAME                    "security.library"
#define SECURITYVERSION                 (45)

#define SECURITYCATALOGNAME             "System/security.catalog"
#define SECURITYCATALOGVERSION          (1)

/*
 * Reserved users/groups
 *
 * These are the original MultiUser values (root 0xffff, nobody 0), so that
 * the owner words on real MuFS volumes mean the same thing here.
 * WARNING: 0 is nobody, never a real user; 0xffff is the super user.
 */
#define secOWNER_SYSTEM                 (0xffffffff)    /* always owner */
#define secOWNER_NOBODY                 (0x00000000)    /* no rights    */

#define secMASK_UID                     (0xffff0000)    /* Mask for uid bits */
#define secMASK_GID                     (0x0000ffff)    /* Mask for gid bits */

#define secROOT_UID                     (0xffff)        /* super user uid */
#define secROOT_GID                     (0xffff)        /* super user gid */

#define secNOBODY_UID                   (0x0000)        /* nobody uid */
#define secNOBODY_GID                   (0x0000)        /* nobody gid */

#define secUSERIDSIZE                   (32)            /* Maximum size for a User ID       */
#define secGROUPIDSIZE                  (32)            /* Maximum size for a Group ID      */
#define secPASSWORDSIZE                 (32)            /* Maximum size for a Password      */
#define secUSERNAMESIZE                 (220)           /* Maximum size for a User Name     */
#define secGROUPNAMESIZE                (220)           /* Maximum size for a Group Name    */
#define secHOMEDIRSIZE                  (256)           /* Maximum size for a Home Directory*/
#define secSHELLSIZE                    (256)           /* Maximum size for a Shell         */

/*
 * Extended Owner Information Structure
 *
 * A pointer to this structure is returned by secGetTaskExtOwner().
 * You MUST use secFreeExtOwner() to deallocate it!!
 *
 * The structure is followed by a UWORD array containing the Secondary Group
 * Numbers; use secSecGroups() to access them (never on a NULL pointer).
 */
struct secExtOwner
{
    UWORD               uid;
    UWORD               gid;
    UWORD               NumSecGroups;   /* Number of Secondary Groups this user belongs to */
};

#define secSecGroups(x)         ((UWORD *)((UBYTE *)(x) + sizeof(struct secExtOwner)))

/* Convert an Extended Owner Information Structure to a ULONG (cfr. secGetTaskOwner()) */
#define secExtOwner2ULONG(x)    ((ULONG)((x) ? ((ULONG)(x)->uid << 16 | (x)->gid) : secOWNER_NOBODY))

/*
 * Public User Information Structure
 *
 * ALWAYS use secAllocUserInfo() to allocate this structure.
 */
struct secUserInfo
{
    char                UserID[secUSERIDSIZE];
    UWORD               uid;
    UWORD               gid;
    char                UserName[secUSERNAMESIZE];
    char                HomeDir[secHOMEDIRSIZE];
    UWORD               NumSecGroups;           /* Number of Secondary Groups this user belongs to */
    UWORD               *SecGroups;             /* Array of NumSecGroups Secondary Group Numbers   */
    char                Shell[secSHELLSIZE];
};

/*
 * Public Group Information Structure
 *
 * ALWAYS use secAllocGroupInfo() to allocate this structure.
 */
struct secGroupInfo
{
    char                GroupID[secGROUPIDSIZE];
    UWORD               gid;
    UWORD               MgrUid;                 /* Manager of this group */
    char                GroupName[secGROUPNAMESIZE];
};

/*
 * KeyTypes for secGetUserInfo() / secGetGroupInfo()
 */
#define secKeyType_First                (0)
#define secKeyType_Next                 (1)
#define secKeyType_gid                  (4)

/* KeyTypes for secGetUserInfo() only */
#define secKeyType_UserID               (2)     /* Case-sensitive */
#define secKeyType_uid                  (3)
#define secKeyType_gidNext              (5)
#define secKeyType_UserName             (6)     /* Case-insensitive */
#define secKeyType_WUserID              (7)     /* Case-insensitive, wild cards allowed */
#define secKeyType_WUserName            (8)     /* Case-insensitive, wild cards allowed */
#define secKeyType_WUserIDNext          (9)
#define secKeyType_WUserNameNext        (10)

/* KeyTypes for secGetGroupInfo() only */
#define secKeyType_GroupID              (11)    /* Case-sensitive */
#define secKeyType_WGroupID             (12)    /* Case-insensitive, wild cards allowed */
#define secKeyType_WGroupIDNext         (13)
#define secKeyType_GroupName            (14)    /* Case-insensitive */
#define secKeyType_WGroupName           (15)    /* Case-insensitive, wild cards allowed */
#define secKeyType_WGroupNameNext       (16)
#define secKeyType_MgrUid               (17)
#define secKeyType_MgrUidNext           (18)

/*
 * Tags for secLoginA(), secLogoutA(), secSetDefProtectionA(), secCheckPasswdA()
 */
#define secT_Input              (TAG_USER+1)    /* filehandle - default is Input()                  */
#define secT_Output             (TAG_USER+2)    /* filehandle - default is Output()                 */
#define secT_Graphical          (TAG_USER+3)    /* boolean - default is FALSE                       */
#define secT_PubScrName         (TAG_USER+4)    /* name of public screen                            */
#define secT_Task               (TAG_USER+5)    /* task (NOT the name!!)                            */
#define secT_Own                (TAG_USER+6)    /* make a task owned by this user                   */
#define secT_Global             (TAG_USER+7)    /* change it for all tasks on the same level        */
#define secT_Quiet              (TAG_USER+8)    /* for secLogout(), don't give a login prompt       */
#define secT_UserID             (TAG_USER+9)    /* UserID for secLoginA()                           */
#define secT_Password           (TAG_USER+10)   /* Password for secLoginA(), needs secT_UserID      */
#define secT_DefProtection      (TAG_USER+11)   /* Default protection bits (RWED GROUP R OTHER R)   */
#define secT_All                (TAG_USER+12)   /* for secLogout(), logout until user stack empty   */
#define secT_NoLog              (TAG_USER+13)   /* for secLoginA(), only root                       */
#define secT_Force              (TAG_USER+14)   /* for secKill(): RemTask() instead of CTRL-C       */

/*
 * Protection bits (see also <dos/dos.h>)
 */
#define secFIBB_SET_UID         (31)    /* Change owner during execution                     */
#define secFIBB_SET_GID         (30)    /* Change group during execution - not implemented   */

#define secFIBF_SET_UID         (1UL << secFIBB_SET_UID)
#define secFIBF_SET_GID         (1UL << secFIBB_SET_GID)

/* Default Protection Bits for new objects */
#define secDEFPROTECTION        (FIBF_OTR_READ | FIBF_GRP_READ)
#ifndef DEFPROTECTION
#define DEFPROTECTION           secDEFPROTECTION        /* MuFS compatibility */
#endif

/*
 * Relations returned by secGetRelationshipA()
 */
#define secRelB_ROOT_UID        (0)     /* User == super user                       */
#define secRelB_ROOT_GID        (1)     /* User belongs to the super user group     */
#define secRelB_NOBODY          (2)     /* User == nobody                           */
#define secRelB_UID_MATCH       (3)     /* User == owner                            */
#define secRelB_GID_MATCH       (4)     /* User belongs to owner group              */
#define secRelB_PRIM_GID        (5)     /* User's primary group == owner group      */
#define secRelB_NO_OWNER        (6)     /* Owner == nobody                          */

#define secRelF_ROOT_UID        (1 << secRelB_ROOT_UID)
#define secRelF_ROOT_GID        (1 << secRelB_ROOT_GID)
#define secRelF_NOBODY          (1 << secRelB_NOBODY)
#define secRelF_UID_MATCH       (1 << secRelB_UID_MATCH)
#define secRelF_GID_MATCH       (1 << secRelB_GID_MATCH)
#define secRelF_PRIM_GID        (1 << secRelB_PRIM_GID)
#define secRelF_NO_OWNER        (1 << secRelB_NO_OWNER)

/* Relations that permit changing an object's properties (protection, comment, date, owner) */
#define secRelF_PROPERTY_ACCESS (secRelF_ROOT_UID | secRelF_UID_MATCH | secRelF_NO_OWNER)

/*
 * Monitor Structure
 *
 * The use of this structure is restricted to root.
 * Do not modify or reuse this structure while it is active!
 */
struct secMonitor
{
    struct MinNode      Node;
    ULONG               Mode;           /* see definitions below */
    ULONG               Triggers;       /* see definitions below */
    union
    {
        struct
        {                               /* for SEND_SIGNAL  */
            struct Task     *Task;
            ULONG           SignalNum;
        } Signal;
        struct
        {                               /* for SEND_MESSAGE */
            struct MsgPort  *Port;
        } Message;
    } u;
    /* NOTE: This structure may be extended in future! */
};

/* Monitor Modes */
#define secMon_IGNORE           (0)
#define secMon_SEND_SIGNAL      (1)
#define secMon_SEND_MESSAGE     (2)

/*
 * Monitor Message
 *
 * Sent to the application if SEND_MESSAGE is specified. Do NOT forget to reply!
 */
struct secMonMsg
{
    struct Message      ExecMsg;
    struct secMonitor   *Monitor;       /* The monitor that sent the message    */
    ULONG               Trigger;        /* The trigger that caused the message  */
    UWORD               From;
    UWORD               To;
    char                UserID[secUSERIDSIZE];
};

/* Monitor Triggers */
#define secTrgB_OwnerChange     (0)     /* Task Owner Change            From: old uid, To: new uid          */
#define secTrgB_Login           (1)     /* successful Login/Logout      From: old uid, To: new uid, UserID  */
#define secTrgB_LoginFail       (2)     /* unsuccessful Login/Logout    From: old uid, UserID               */
#define secTrgB_Passwd          (3)     /* successful Passwd            From: uid                           */
#define secTrgB_PasswdFail      (4)     /* unsuccessful Passwd          From: uid                           */
#define secTrgB_CheckPasswd     (5)     /* successful CheckPasswd       From: uid                           */
#define secTrgB_CheckPasswdFail (6)     /* unsuccessful CheckPasswd     From: uid                           */

#define secTrgF_OwnerChange     (1 << secTrgB_OwnerChange)
#define secTrgF_Login           (1 << secTrgB_Login)
#define secTrgF_LoginFail       (1 << secTrgB_LoginFail)
#define secTrgF_Passwd          (1 << secTrgB_Passwd)
#define secTrgF_PasswdFail      (1 << secTrgB_PasswdFail)
#define secTrgF_CheckPasswd     (1 << secTrgB_CheckPasswd)
#define secTrgF_CheckPasswdFail (1 << secTrgB_CheckPasswdFail)

/*
 * secMemInfo - public but not guaranteed to remain.
 */
struct secMemInfo
{
    APTR                Address;
    ULONG               Size;
    struct secMemInfo   *next;
};

/* This is what you get a pointer to from secLocksecBase() */
struct secVolume;

struct secPointers
{
    struct MinList      *Monitors;
    struct MinList      *Segments;
    struct MinList      *Sessions;
    struct MinList      *Tasks;
    struct secVolume    *Volumes;
};

/*==========================================================================*/
/*                      Configuration files                                 */
/*==========================================================================*/

/*
 * Password File
 *
 * For each user, the Password File must contain a line like this:
 *
 *   <UserID>|<Password>|<uid>|<gid>|<UserName>|<HomeDir>|<Shell>
 *
 *   <UserID>    User Login ID (max. secUSERIDSIZE-1 characters)
 *   <Password>  Encrypted Password
 *   <uid>       User Number (1 - 65534)
 *   <gid>       Primary Group Number (0 - 65534)
 *   <UserName>  Full User Name (max. secUSERNAMESIZE-1 characters)
 *   <HomeDir>   Home directory (max. secHOMEDIRSIZE-1 characters)
 *   <Shell>     Default Shell (max. secSHELLSIZE-1 characters)
 */
#define secPasswd_FileName      "passwd"

/*
 * Group File
 *
 * Two parts separated by a blank line. Part 1 lines:
 *
 *   <GroupID>|<gid>|<MgrUid>|<GroupName>
 *
 *   <GroupID>   Group short ID (max. secGROUPIDSIZE-1 characters)
 *   <gid>       Group Number (0 - 65534)
 *   <MgrUid>    User Number of this group's manager, 0 for no manager.
 *   <GroupName> Full Group Name (max. secGROUPNAMESIZE-1 characters)
 *
 * Part 2 lines (secondary group membership):
 *
 *   <uid>:<gid>[,<gid>...]
 */
#define secGroup_FileName       "group"

/*
 * Configuration File
 *
 * One option per line, ReadArgs style ("<OPT> <val>" or "<OPT>=<val>"),
 * 0 for OFF and 1 for ON. Defaults in square brackets.
 *
 *   LIMITDOSSETPROTECTION  dos.library/SetProtection() cannot change GROUP and OTHER bits [1]
 *   PROFILE                execute the Profile if it exists [1]
 *   LASTLOGINREQ           display the Lastlogin requester [1]
 *   LOGSTARTUP             log startup [0]
 *   LOGLOGIN               log successful logins [0]
 *   LOGLOGINFAIL           log failed logins [0]
 *   LOGPASSWD              log successful password changes [0]
 *   LOGPASSWDFAIL          log failed password changes [0]
 *   LOGCHECKPASSWD         log successful password checks [0]
 *   LOGCHECKPASSWDFAIL     log failed password checks [0]
 *   PASSWDUIDLEVEL         users with a uid <= <val> can change their passwords [secNOBODY_UID]
 *   PASSWDGIDLEVEL         users with a gid <= <val> can change their passwords [secNOBODY_UID]
 *   FSTAB                  read the fstab file and enforce the listed volumes [0]
 *   LOADPLUGIN             name of a plugin to load (may be repeated)
 */
#define secConfig_FileName      "Security.config"

#define secLog_FileName         "Security.log"
#define secLastLogin_FileName   ".lastlogin"
#define secProfile_FileName     ".profile"
#define secPlan_FileName        ".plan"
#define secFSTab_FileName       "fstab"

/*
 * Key File
 *
 * Present in the root directory of every volume using a muFS-aware filesystem
 * with a key. It contains 3 lines: a pseudo random ASCII key (max. 1023
 * characters), the directory of the password file on this volume (or an empty
 * line), and the directory of the configuration file (or an empty line).
 * If there is ANY inconsistency the system will refuse to work!
 */
#define secKey_FileName         ":.MultiUser.keyfile"

/*
 * Default location of the configuration files when no key file is found.
 * An assign SECURITY: is created by the library pointing at this directory.
 */
#define secConfig_DirName       "SYS:Security"
#define secConfig_AssignName    "SECURITY"

/*==========================================================================*/
/*                      FileSystem Authors' API                             */
/*==========================================================================*/

/*
 * Packets a muFS-aware filesystem should support in addition to
 * ACTION_SET_OWNER (see <dos/dosextens.h>).
 */
#define ACTION_IS_SECFS                 (2999)  /* Res1: DOSTRUE if the handler enforces ownership */
#define ACTION_GET_SECFS_VERSION        (2998)  /* Res1: version of the FS API implemented         */
#define secFS_API_VERSION               (2)

/* Access types for secAccess_Control() (may be OR'ed) */
#define secAt_Read                      (1)
#define secAt_Write                     (2)
#define secAt_Execute                   (4)
#define secAt_Delete                    (8)

/* contextflags for secAccess_Control() */
#define secAC_IGNORE_CONTEXT            (0)     /* Context parameter should be ignored                  */
#define secAC_FILESYSTEM_CONTEXT        (1)     /* Context parameter is the MsgPort for a filesystem    */

/* Return values of secAccess_Control() */
#define secAC_PERMISSION_GRANTED        (0)             /* Access is allowed                             */
#define secAC_PERMISSION_DENIED         (1L << 31)      /* Permission denied                             */
#define secAC_READ_ONLY_FS              (1L << 30)      /* FS is read-only and write access was requested*/
#define secAC_OWNER_DENIED              (1L << 0)       /* Task owns object, but bits deny access        */
#define secAC_GROUP_DENIED              (1L << 1)       /* Task is in the group, but bits deny access    */
#define secAC_ROOT_DENIED               (1L << 2)       /* Task is root, but a higher rule denied access */

/*
 * Rendezvous port for bootstrap programs that tell the library where the
 * configuration lives when no muFS volume is present.
 */
#define secENFORCE_PORTNAME             "SecurityEnforcePort"

struct secFSEnforceRendezvous
{
    struct MsgPort      Port;
    STRPTR              PasswdDir;
    STRPTR              ConfigDir;
};

/*==========================================================================*/
/*                              Plugins                                     */
/*==========================================================================*/

/* All plugins have this suffix and live in the configuration directory */
#define secPLUGIN_SUFFIX                ".secplugin"

/* Return values for plugin functions */
#define secpiTRUE                       (0UL)   /* operation completed with success                           */
#define secpiFALSE                      (1UL)   /* operation completed but failed: handling stops here        */
#define secpiASYNC                      (2UL)   /* operation continues asynchronously, see                    */
                                                /* secPluginOperationComplete()                               */
#define secpiFALSECONT                  (3UL)   /* operation failed; other handlers may be tried              */
#define secpiNOTSUPP                    (4UL)   /* Operation/Handler not supported                            */

#define secPLUGIN_IFACE1                (1UL)
#define secPLUGIN_INTERFACE             secPLUGIN_IFACE1

#ifndef MAKE_ID /* also defined in <libraries/iffparse.h> */
#define MAKE_ID(a,b,c,d)                ((ULONG)(a)<<24 | (ULONG)(b)<<16 | (ULONG)(c)<<8 | (ULONG)(d))
#endif

/* internal plugin records */
typedef struct
{
    struct MinNode              Node;
    ULONG                       reference_count;
    BPTR                        SegList;
    struct secPluginHeader      *header;                /* For locating the init/fini functions */
    UBYTE                       modulename[64];         /* For displaying */
} secPluginModule;

/*
 * Function called to initialize the plugin. The plugin should perform any
 * initialization it needs, and then formally register itself with
 * security.library via secRegisterHandler(); module must be stored in the
 * plugin_ops it registers.
 */
typedef BOOL (*secInitPluginFunc)(struct Library *secbase, secPluginModule *module);

/* Function called to shut-down the plugin: free resources, un-register. */
typedef VOID (*secTerminatePluginFunc)(void);

/*
 * Plugin Module Definition - must be located in the executable and be
 * exported as the symbol "secPluginHeader" (the library looks for the magic
 * marker in the first segment if the symbol cannot be resolved).
 */
#define secPLUGIN_RECOGNITION           MAKE_ID('m', 'S', 'p', 'I')

struct secPluginHeader
{
    ULONG                       plugin_magic;   /* = secPLUGIN_RECOGNITION */
    ULONG                       Version;        /* = secPLUGIN_INTERFACE   */
    secInitPluginFunc           Initialize;
    secTerminatePluginFunc      Terminate;
};

struct plugin_ops
{
    ULONG                       HandlerType;    /* = ID_PLUGIN_XXX                          */
    secPluginModule             *module;        /* As provided by the Initialization function */
};

/* Encryption plugin */
#define ID_PLUGIN_ENCRYPTION            MAKE_ID('c', 'r', 'p', 't')

struct plugin_crypt_ops
{
    struct plugin_ops           ops;
    /* Encrypt the supplied text using the supplied key and place the result in buffer */
    ULONG                       (*Crypt)(STRPTR buffer, STRPTR key, STRPTR setting);
    /* Return the maximum length of an encrypted password */
    ULONG                       (*MaxPwdLen)(void);
    /* Check a password to see if it is valid */
    ULONG                       (*CheckPassword)(STRPTR userid, STRPTR thepass, STRPTR suppliedpass);
    /* Encrypt Password for storing in a userdb */
    ULONG                       (*EncryptPassword)(STRPTR buffer, STRPTR userid, STRPTR thepass);
};

/* User DataBase Operations Plugin - interface to be defined */
#define ID_PLUGIN_USER_DATABASE         MAKE_ID('u', 'r', 'd', 'b')

/* Authentication plugin - interface to be defined (see MUFS3.md, PAM-style) */
#define ID_PLUGIN_AUTH                  MAKE_ID('a', 'u', 't', 'h')

#endif /* LIBRARIES_SECURITY_H */
