#ifndef __legacy_h
#define __legacy_h

/* NOTE: We want to lose this file eventually */

#define MULTIUSERNAME         "multiuser.library"
#define MULTIUSERVERSION      (40)

#define MULTIUSERCATALOGNAME    "multiuser.catalog"
#define MULTIUSERCATALOGVERSION (1)


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
    *       <UserID>    User Login ID (max. muUSERIDSIZE-1 characters)
    *       <PassKey>   Encrypted Password
    *       <uid>       User Number (1 - 65535)
    *       <gid>       Primary Group Number (0 - 65535)
    *       <UserName>  Full User Name (max. muUSERNAMESIZE-1 characters)
    *       <HomeDir>   Home directory (max. muHOMEDIRSIZE-1 characters)
    *       <Shell>     Default Shell (max. muSHELLSIZE-1 characters)
    *                   (not used yet, AS225 compatibility)
    */

#if !defined(secPasswd_FileName)
#define secPasswd_FileName     "passwd"  /* for AS225 compatibility */
#endif

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
    *       <GroupID>   Group short ID (max. muGROUPIDSIZE-1 characters)
    *       <gid>       Group Number (0 - 65535)
    *       <MgrUid>    User Number of this group's manager, 0 for no
    *                   manager. A group's manager must not belong to the
    *                   group.
    *       <GroupName> Full Group Name (max. muGROUPNAMESIZE-1 characters)
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

#if !defined(secGroup_FileName)
#define secGroup_FileName      "group"
#endif

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
    *                            <val> can change their passwords [muNOBODY_UID]
    *    PASSWDGIDLEVEL          users with a gid less than or equal to
    *                            <val> can change their passwords [muNOBODY_UID]
    *
    *    NOTE: if a user has a uid less than the PASSWDUIDLEVEL AND a gid
    *          less than PASSWDGIDLEVEL he/she is NOT allowed to change
    *          his/her password!
    */

#if !defined(muConfig_FileName)
#define muConfig_FileName     "MultiUser.config"
#endif

   /*
    *    Log File
    */

#if !defined(muLog_FileName)
#define muLog_FileName        "MultiUser.log"
#endif

   /*
    *    Lastlogin File
    */

#if !defined(secLastLogin_FileName)
#define secLastLogin_FileName  ".lastlogin"
#endif

   /*
    *    Profile
    */

#if !defined(secProfile_FileName)
#define secProfile_FileName    ".profile"
#endif

   /*
    *    Plan file
    */

#if !defined(secPlan_FileName)
#define secPlan_FileName       ".plan"
#endif

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

#if !defined(muKey_FileName)
#define muKey_FileName        ":.MultiUser.keyfile"
#endif

   /*
    *    Tags for muLogoutA()
    *             muLoginA()
    *             muSetDefProtectionA()
    */
#define secT_Input          (TAG_USER+1)   /* filehandle - default is Input() */
#define secT_Output         (TAG_USER+2)   /* filehandle - default is Output() */
#define secT_Graphical      (TAG_USER+3)   /* boolean - default is FALSE */
#define secT_PubScrName     (TAG_USER+4)   /* name of public screen */
#define secT_Task           (TAG_USER+5)   /* task (NOT the name!!) */
#define secT_Own            (TAG_USER+6)   /* make a task owned by this user */
#define secT_Global         (TAG_USER+7)   /* change it for all tasks on the */
                                          /* same level as this one */
#define secT_Quiet          (TAG_USER+8)   /* for muLogoutA(), don't give a */
                                          /* login prompt, simply logout */
#define secT_UserID         (TAG_USER+9)   /* UserID for muLoginA() */
#define secT_Password       (TAG_USER+10)  /* Password for muLoginA(), must */
                                          /* be combined with secT_UserID!! */
#define secT_DefProtection  (TAG_USER+11)  /* Default protection bits */
                                          /* default is RWED GROUP R OTHER R */
#define secT_All            (TAG_USER+12)  /* for muLogoutA(), logout until */
                                          /* user stack is empty */
#define secT_NoLog          (TAG_USER+13)  /* for muLoginA(), only root */
	 
   /*
    *    Public User Information Structure
    *
    *
    *    For future compatibility, you should ALWAYS use muAllocUserInfo()
    *    to allocate this structure. NEVER do it by yourself!!
    */

struct secUserInfo {
   char UserID[secUSERIDSIZE];
   UWORD uid;
   UWORD gid;
   char UserName[secUSERNAMESIZE];
   char HomeDir[secHOMEDIRSIZE];
   UWORD NumSecGroups;              /* Number of Secondary Groups this */
                                    /* user belongs to */
   UWORD *SecGroups;                /* Points to an array of NumSecGroups */
                                    /* Secondary Group Numbers */
   char Shell[secSHELLSIZE];
};


   /*
    *    Public Group Information Structure
    *
    *
    *    For future compatibility, you should ALWAYS use muAllocGroupInfo()
    *    to allocate this structure. NEVER do it by yourself!!
    */

struct secGroupInfo {
   char GroupID[secGROUPIDSIZE];
   UWORD gid;
   UWORD MgrUid;                    /* Manager of this group */
   char GroupName[secGROUPNAMESIZE];
};


   /*
    *    KeyTypes for muGetUserInfo()
    *                 muGetGroupInfo()
    */

#define secKeyType_First          (0)
#define secKeyType_Next           (1)
#define secKeyType_gid            (4)

   /*
    *    KeyTypes for muGetUserInfo() only
    */

#define secKeyType_UserID         (2)   /* Case-sensitive */
#define secKeyType_uid            (3)
#define secKeyType_gidNext        (5)
#define secKeyType_UserName       (6)   /* Case-insensitive */
#define secKeyType_WUserID        (7)   /* Case-insensitive, wild cards allowed */
#define secKeyType_WUserName      (8)   /* Case-insensitive, wild cards allowed */
#define secKeyType_WUserIDNext    (9)
#define secKeyType_WUserNameNext  (10)

   /*
    *    KeyTypes for muGetGroupInfo() only
    */

#define secKeyType_GroupID        (11)  /* Case-sensitive */
#define secKeyType_WGroupID       (12)  /* Case-insensitive, wild cards allowed */
#define secKeyType_WGroupIDNext   (13)
#define secKeyType_GroupName      (14)  /* Case-insensitive */
#define secKeyType_WGroupName     (15)  /* Case-insensitive, wild cards allowed */
#define secKeyType_WGroupNameNext (16)
#define secKeyType_MgrUid         (17)
#define secKeyType_MgrUidNext     (18)

   /*
    *    Packet types (see also <dos/dosextens.h> :-)
    */

/* #define ACTION_SET_OWNER        1036 */


   /*
    *    Protection bits (see also <dos/dos.h> :-)
    */

#define secFIBB_SET_UID        (31)  /* Change owner during execution */
#define secFIBB_SET_GID        (30)  /* Change group during execution - not yet implemented */

#define secFIBF_SET_UID        (1<<secFIBB_SET_UID)
#define secFIBF_SET_GID        (1<<secFIBB_SET_GID)

   /*
    *    Default Protection Bits
    */

#define DEFPROTECTION (FIBF_OTR_READ | FIBF_GRP_READ)


   /*
    *    Relations returned by muGetRelationshipA()
    */

#define secRelB_ROOT_UID    (0)   /* User == super user */
#define secRelB_ROOT_GID    (1)   /* User belongs to the super user group */
#define secRelB_NOBODY      (2)   /* User == nobody */
#define secRelB_UID_MATCH   (3)   /* User == owner */
#define secRelB_GID_MATCH   (4)   /* User belongs to owner group */
#define secRelB_PRIM_GID    (5)   /* User's primary group == owner group */
#define secRelB_NO_OWNER    (6)   /* Owner == nobody */

#define secRelF_ROOT_UID    (1<<secRelB_ROOT_UID)
#define secRelF_ROOT_GID    (1<<secRelB_ROOT_GID)
#define secRelF_NOBODY      (1<<secRelB_NOBODY)
#define secRelF_UID_MATCH   (1<<secRelB_UID_MATCH)
#define secRelF_GID_MATCH   (1<<secRelB_GID_MATCH)
#define secRelF_PRIM_GID    (1<<secRelB_PRIM_GID)
#define secRelF_NO_OWNER    (1<<secRelB_NO_OWNER)


   /*
    *    Monitor Structure
    *    The use of this structure is restricted to root.
    *    Do not modify or reuse this structure while it is active!
    */

struct secMonitor {
   struct MinNode Node;
   ULONG Mode;                      /* see definitions below */
   ULONG Triggers;                  /* see definitions below */
   union {
      struct {                      /* for SEND_SIGNAL */
         struct Task *Task;
         ULONG SignalNum;
      } Signal;

      struct {                      /* for SEND_MESSAGE */
         struct MsgPort *Port;
      } Message;
   } u;

   /* NOTE: This structure may be extended in future! */
};

   /*
    *    Monitor Modes
    */

#define secMon_IGNORE       (0)
#define secMon_SEND_SIGNAL  (1)
#define secMon_SEND_MESSAGE (2)

   /*
    *    Monitor Message
    *
    *
    *    Sent to the application if SEND_MESSAGE is specified.
    *    Do NOT forget to reply!
    */

struct secMonMsg {
   struct Message ExecMsg;
   struct secMonitor *Monitor;       /* The monitor that sent the message */
   ULONG Trigger;                   /* The trigger that caused the message */
   UWORD From;
   UWORD To;
   char UserID[secUSERIDSIZE];
};

   /*
    *    Monitor Triggers
    */

#define secTrgB_OwnerChange       (0)   /* Task Owner Change */
                                       /*    From:    uid of old user */
                                       /*    To:      uid of new user */
#define secTrgB_Login             (1)   /* successful Login/Logout */
                                       /*    From:    uid of old user */
                                       /*    To:      uid of new user */
                                       /*    UserID:  UserID of new user */
#define secTrgB_LoginFail         (2)   /* unsuccessful Login/Logout */
                                       /*    From:    uid of old user */
                                       /*    UserID:  UserID of new user */
#define secTrgB_Passwd            (3)   /* successful Passwd */
                                       /*    From:    uid of user */
#define secTrgB_PasswdFail        (4)   /* unsuccessful Passwd */
                                       /*    From:    uid of user */
#define secTrgB_CheckPasswd       (5)   /* successful CheckPasswd */
                                       /*    From:    uid of user */
#define secTrgB_CheckPasswdFail   (6)   /* unsuccessful CheckPasswd */
                                       /*    From:    uid of user */

#define secTrgF_OwnerChange       (1<<secTrgB_OwnerChange)
#define secTrgF_Login             (1<<secTrgB_Login)
#define secTrgF_LoginFail         (1<<secTrgB_LoginFail)
#define secTrgF_Passwd            (1<<secTrgB_Passwd)
#define secTrgF_PasswdFail        (1<<secTrgB_PasswdFail)
#define secTrgF_CheckPasswd       (1<<secTrgB_CheckPasswd)
#define secTrgF_CheckPasswdFail   (1<<secTrgB_CheckPasswdFail)


#endif  /* __legacy_h */
