/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: public definitions for the security.library
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

#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif /* LIBRARIES_LOCALE_H */

#define SECURITYNAME         "security.library"
#define SECURITYVERSION      (40)

#define SECURITYCATALOGNAME    "security.catalog"
#define SECURITYCATALOGVERSION (1)

/* Public secMemInfo structure. Don't expect it to remain */

struct secMemInfo {
    APTR                Address;
    ULONG               Size;
    struct secMemInfo   *next;
};

/* This is what you get a pointer to from secLocksecBase() */

struct secPointers {
    struct MinList      *Monitors;
    struct MinList      *Segments;
    struct MinList      *Sessions;
    struct MinList      *Tasks;
    struct secVolume    *Volumes;
};

   /*
    *    Extended Owner Information Structure
    *
    *    A pointer to this structure is returned by secGetTaskExtOwner().
    *    You MUST use secFreeExtOwner() to deallocate it!!
    */

struct secExtOwner {
   UWORD                uid;
   UWORD                gid;
   UWORD                NumSecGroups;   /* Number of Secondary Groups this */
                                        /* user belongs too. */
};

   /* NOTE: This structure is followed by a UWORD array containing
    *       the Secondary Group Numbers
    *       Use the following macro to access these group numbers,
    *       e.g. sgid = secSecGroups(extowner)[i];
    *
    *       Do not use this macro on a NULL pointer!!
    */

#define secSecGroups(x) ((UWORD *)((UBYTE *)x+sizeof(struct secExtOwner)))


   /*
    *    Macro to convert an Extended Owner Information Structure to a ULONG
    *    (cfr. secGetTaskOwner())
    */

#define secExtOwner2ULONG(x) ((ULONG)(x ? (x)->uid<<16|(x)->gid : secOWNER_NOBODY))



/* Structures for the Packet Interceptor and rendevous with MUFS2 
 *
 * This code is ©1998 Wez Furlong, All Rights Reserved
 */

/* Overview:
 *
 * When the MU.lib is opened, it searches for MUFS volumes; if it finds them
 * it can then find its configuration and passwd files etc.
 *
 * If there are no MUFS volumes, then the library looks for a MsgPort created
 * by a bootstrap program; data in the MsgPort informs the library of the
 * location of the configuration files.
 *
 * A configuration file option specifies the location of a config file for the
 * interceptor code. The interceptor is started for each FS specified
 */

/* The Rendevous Port
 * 
 * We dont need the VOLUME argument of the keyfile, since the presence of the
 * bootstrap implies that there is a table of volumes 
 */

#define MUFS_ENFORCE_PORTNAME	"MUFSEnforcePort"

struct secFSEnforceRendevous {
	struct MsgPort          Port;
	STRPTR			PasswdDir;
	STRPTR			ConfigDir;
};

/* muFS 2 FileSystem Authors API
 * Copyright ©1998, Wez Furlong, All Rights Reserved
 *
 * defines for the different types of filesystem access required
 */

#define secAt_Read                      1
#define secAt_Write                     2
#define secAt_Execute                   4
#define secAt_Delete                    8

/*
 * Flags for use as the contextflags parameter to secAccess_Control()
 */

#define secAC_IGNORE_CONTEXT		0	/* Context parameter should be ignored */
#define secAC_FILESYSTEM_CONTEXT	1	/* Context parameter is the MsgPort for a filesystem */

/*
 * Values for returns from the secAccess_Control API
 */

#define secAC_PERMISSION_GRANTED	0	/* Access is allowed */
/* Permission denied */
#define secAC_PERMISSION_DENIED	        (1<<31)
/* The filesystem (or context) is mounted read-only and write access was
 * requested. */
#define secAC_READ_ONLY_FS              (1<<30)	

/* Task owns object, but permissions prevent access (ie: access can be forced) */
#define secAC_OWNER_DENIED              (1<<0)
/* Task is in the group of the object, but permissions prevent access (ie: access can be forced) */
#define secAC_GROUP_DENIED              (1<<1)
/* Task is the super-user, but access was denied due to some 'higher'
 * permission: eg: the filesystem is mounted read-only and a write was
 * requested. */
#define secAC_ROOT_DENIED               (1<<2)

   /*
    *    Reserved users/groups
    *
    *    WARNING: a uid may NOT be 65535!
    */

#define secOWNER_SYSTEM                 (0x00000000)   /* always owner */
#define secOWNER_NOBODY                 (0xffffffff)   /* no rights */

#define secMASK_UID                     (0xffff0000)   /* Mask for uid bits */
#define secMASK_GID                     (0x0000ffff)   /* Mask for gid bits */

#define secROOT_UID                     (0x0000)       /* super user uid */
#define secROOT_GID                     (0x0000)       /* super user gid */

#define secNOBODY_UID                   (0xffff)       /* nobody uid */

#define secUSERIDSIZE                   (32)           /* Maximum size for a User ID */
#define secGROUPIDSIZE                  (32)           /* Maximum size for a Group ID */
#define secPASSWORDSIZE                 (32)           /* Maximum size for a Password */
#define secUSERNAMESIZE                 (220)          /* Maximum size for a User Name */
#define secGROUPNAMESIZE                (220)          /* Maximum size for a Group Name */
#define secHOMEDIRSIZE                  (256)          /* Maximum size for a Home Directory */
#define secSHELLSIZE                    (256)          /* Maximum size for a Shell */


   /*
    *    Password File
    *
    *
    *    For each user, the Password File must contain a line like this:
    *
    *    <UserID>|<Password>|<uid>|<gid>|<UserName>|<HomeDir>|<Shell>
    *
    *    with:
    *
    *       <UserID>    User Login ID (max. secUSERIDSIZE-1 characters)
    *       <PassKey>   Encrypted Password
    *       <uid>       User Number (1 - 65535)
    *       <gid>       Primary Group Number (0 - 65535)
    *       <UserName>  Full User Name (max. secUSERNAMESIZE-1 characters)
    *       <HomeDir>   Home directory (max. secHOMEDIRSIZE-1 characters)
    *       <Shell>     Default Shell (max. secSHELLSIZE-1 characters)
    *                   (not used yet, AS225 compatibility)
    */

#define secPasswd_FileName              "passwd"  /* for AS225 compatibility */


   /*
    *    Group File
    *
    *
    *    This file gives more information about the groups and defines
    *    the secondary groups (other than the primary group) a user
    *    belongs to. It exists out of two parts, separated by a blank line.
    *
    *    The first part contains lines with the format:
    *
    *    <GroupID>|<gid>|<MgrUid>|<GroupName>
    *
    *    with:
    *
    *       <GroupID>   Group short ID (max. secGROUPIDSIZE-1 characters)
    *       <gid>       Group Number (0 - 65535)
    *       <MgrUid>    User Number of this group's manager, 0 for no
    *                   manager. A group's manager must not belong to the
    *                   group.
    *       <GroupName> Full Group Name (max. secGROUPNAMESIZE-1 characters)
    *
    *    NOTE: Not every group must have a line in this file, but at least
    *          one group must have one.
    *
    *
    *    The seconds part contains lines with the format:
    *
    *    <uid>:<gid>[,<gid>...]
    *
    *    with:
    *
    *       <uid>       User Number (0-65534)
    *       <gid>       Group Number (0 - 65535)
    *
    *    If you think you'll exceed the maximum line length (circa 1K),
    *    you may use more than one line per user.
    */

#define secGroup_FileName               "group"


   /*
    *    Configuration File
    *
    *
    *    This file contains lines with options in the form <OPT>=<val>.
    *    0 is used for OFF, 1 for ON.
    *    Defaults to the values between square brackets.
    *
    *    LIMITDOSSETPROTECTION   dos.library/SetProtection() cannot change
    *                            protection bits for GROUP and OTHER [1]
    *    PROFILE                 execute the Profile if it exists [1]
    *    LASTLOGINREQ            display the Lastlogin requester [1]
    *    LOGSTARTUP              log startup [0]
    *    LOGLOGIN                log successful logins [0]
    *    LOGLOGINFAIL            log failed logins [0]
    *    LOGPASSWD               log successful password changes [0]
    *    LOGPASSWDFAIL           log failed password changes [0]
    *    LOGCHECKPASSWD          log successful password checks [0]
    *    LOGCHECKPASSWDFAIL      log failed password checks [0]
    *    PASSWDUIDLEVEL          users with a uid less than or equal to
    *                            <val> can change their passwords [secNOBODY_UID]
    *    PASSWDGIDLEVEL          users with a gid less than or equal to
    *                            <val> can change their passwords [secNOBODY_UID]
    *
    *    NOTE: if a user has a uid less than the PASSWDUIDLEVEL AND a gid
    *          less than PASSWDGIDLEVEL he/she is NOT allowed to change
    *          his/her password!
    */

#define secConfig_FileName              "Security.config"


   /*
    *    Log File
    */

#define secLog_FileName                 "Security.log"


   /*
    *    Lastlogin File
    */

#define secLastLogin_FileName           ".lastlogin"


   /*
    *    Profile
    */

#define secProfile_FileName             ".profile"


   /*
    *    Plan file
    */

#define secPlan_FileName                ".plan"


   /*
    *    Key File
    *
    *
    *    This file must be present in the root directory of every volume
    *    that uses the MultiUserFileSystem. It must contain 3 lines:
    *
    *       - a pseudo random ASCII key (max. 1023 characters).
    *       - the directory of the password file, if located on this volume,
    *         otherwise an empty line (no spaces!).
    *           e.g. ":MultiUser"
    *           e.g. ":inet/db" for AS225 compatibility
    *       - the directory of the configuration file, if located on this
    *         volume, otherwise an empty line (no spaces!).
    *           e.g. ":MultiUser"
    *
    *    If there is ANY inconsistency the system will refuse to work!!
    */

#define secKey_FileName                 ":.MultiUser.keyfile"

/*-------------- Plugins ------------------*/

/* All plugins have this suffix */

#define secPLUGIN_SUFFIX	        ".asecplugin"

/* Return values for plugin functions */
#define secpiTRUE		        0UL	/* operation completed with success */
#define secpiFALSE	                1UL	/* operation completed but failed: handling stops at this stage */
#define secpiASYNC	                2UL	/* operation will continue asynchronously and notify
									the system when complete.  The system will provide
									the secSPacket that prompted the call and this
									should be passed to the completion function. */
#define secpiFALSECONT	                3UL	/* operation completed but failed; the system may continue with other handlers
											to see if they can satisfy the request */
#define secpiNOTSUPP	                4UL	/* Operation/Handler not supported */

/* Plugins overview;
 *
 * A plugin is a special form of Amiga executable.
 *
 * The plugin should return a pointer to a 'Plugin' which describes what kind
 * of plugin it is, so that security can add it to the relevant list internally.
 *
 * A plugin is recognized by a magic marker within the first 256 bytes of the
 * file.
 *
 * All plugins are kept in the config dir, so the the library doesn't have to
 * search paths to find them.
 *
 * The plugin is free to allocate memory/spawn processes (they will be
 * super-user priveleged) etc. without fear of being UnLoadSeg'd without the
 * library first calling its Incompatible function, which should cause the plugin
 * to free its resources.  It will them be UnLoadSeg'd.  Note that this only
 * occurs when a plugin is rejected for some reason (wrong version ?).
 *
 */

#define MUFS_PLUGIN_IFACE1		1UL
#define MUFS_PLUGIN_INTERFACE	        MUFS_PLUGIN_IFACE1

#ifndef MAKE_ID /* also defined in <libraries/iffparse.h> */
#define MAKE_ID(a,b,c,d)                ((ULONG)(a)<<24 | (ULONG)(b)<<16 | (ULONG)(c)<<8 | (ULONG)(d))
#endif

/* internal plugin records */
typedef struct {
    struct MinNode              Node;
    ULONG                       reference_count;
    BPTR                        SegList;
    struct secPluginHeader      *header;                /* For locating the init/fini functions */
    UBYTE                       modulename[64];         /* For displaying */
} secPluginModule;

/* Function called to initialize the plugin.
 * The plugin should perform any initialization it needs, and then formally
 * register itself with the security.library.
 * module should be passed to the security.library when a handler is registered.
 * */
typedef BOOL (*secInitPluginFunc)(struct Library * secbase, secPluginModule * module);

/* Function called to shut-down the plugin.
 * The plugin should free any resources it has allocated, and un-register itself
 * from the security.library.
 * */
typedef VOID (*secTerminatePluginFunc)(void);
	


/* Plugin Module Definition - must be located in the executeable */
struct secPluginHeader	{
#define secPLUGIN_RECOGNITION	MAKE_ID('m', 'S', 'p', 'I')
    /* This should be secPLUGIN_RECOGNITION */
    ULONG                       plugin_magic;
    /* Module Descriptor */
    ULONG	                Version;	/* = MUFS_PLUGIN_INTERFACE */
    secInitPluginFunc           Initialize;
    secTerminatePluginFunc      Terminate;
};

struct plugin_ops	{
    ULONG                       HandlerType;	/* = ID_PLUGIN_XXX */
    secPluginModule             *module;		/* As provied by the Initialization function  */
};

/* Encryption plugin */

#define ID_PLUGIN_ENCRYPTION	MAKE_ID('c', 'r', 'p', 't')
struct plugin_crypt_ops	{
    struct plugin_ops           ops;
    /* Encrypt the supplied text using the supplied key and place the result in
     * buffer */
    ULONG                       (*Crypt)(STRPTR buffer, STRPTR key, STRPTR setting);
    /* Return the maximum length of an encrypted password */
    ULONG                       (*MaxPwdLen)(void);
    /* Check a password to see if it is valid */
    ULONG                       (*CheckPassword)(STRPTR userid, STRPTR thepass, STRPTR suppliedpass);
    /* Encrypt Password for storing in a userdb */
    ULONG                       (*EncryptPassword)(STRPTR buffer, STRPTR userid, STRPTR thepass);
};

/* User DataBase Operations Plugin */

#define ID_PLUGIN_USER_DATABASE	MAKE_ID('u', 'r', 'd', 'b')
/* Interface to be defined */

/* $Id: security.h,v 2.4 2000/02/19 14:44:35 wez Exp $
 * $Log: security.h,v $
 * Revision 2.4  2000/02/19 14:44:35  wez
 * Created a context system, and removed a redundant Makefile.
 *
 * */

#endif /* _SECURITY_H */
