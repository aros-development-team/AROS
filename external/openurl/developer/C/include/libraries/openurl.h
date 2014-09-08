#ifndef LIBRARIES_OPENURL_H
#define LIBRARIES_OPENURL_H

/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#if defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack(2)
  #elif defined(__VBCC__)
    #pragma amiga-align
  #endif
#endif

/**************************************************************************/
/*
** Names
*/

#define OPENURLNAME "openurl.library"
#define OPENURLVER  7
#define OPENURLREV  5

/**************************************************************************/
/*
** Tags
*/

#define URL_Tagbase                  (0x81480000UL)

#define URL_Show                     (URL_Tagbase +  1) /* BOOL    - Uniconify browser                   */
#define URL_BringToFront             (URL_Tagbase +  2) /* BOOL    - Bring browser to front              */
#define URL_NewWindow                (URL_Tagbase +  3) /* BOOL    - Open URL in new window              */
#define URL_Launch                   (URL_Tagbase +  4) /* BOOL    - Launch browser when not running     */
#define URL_PubScreenName            (URL_Tagbase +  5) /* UBYTE * - Name of public screen to launch at  */

#define URL_GetPrefs_Mode            (URL_Tagbase + 20) /* BOOL    - Get default prefs                   */
#define URL_GetPrefs_FallBack        (URL_Tagbase + 21) /* BOOL    - Do not fail (TRUE)                  */

#define URL_SetPrefs_Save            (URL_Tagbase + 30) /* BOOL    - Save prefs to ENVARC: also          */

#define URL_GetAttr_Version          (URL_Tagbase + 60) /* ULONG   - Library version                     */
#define URL_GetAttr_Revision         (URL_Tagbase + 61) /* ULONG   - Library revision                    */
#define URL_GetAttr_VerString        (URL_Tagbase + 62) /* STRPTR  - "openurl.library 6.4 (27.7.2005)"   */
#define URL_GetAttr_PrefsVer         (URL_Tagbase + 63) /* ULONG   - Library preferences version         */

#define URL_GetAttr_HandlerVersion   (URL_Tagbase + 64) /* Obsolete !!! DON'T USE !!!                    */
#define URL_GetAttr_HandlerRevision  (URL_Tagbase + 65) /* Obsolete !!! DON'T USE !!!                    */
#define URL_GetAttr_HandlerVerString (URL_Tagbase + 66) /* Obsolete !!! DON'T USE !!!                    */

enum
{
    URL_GetPrefs_Mode_Env,
    URL_GetPrefs_Mode_Envarc,
    URL_GetPrefs_Mode_Default,
    URL_GetPrefs_Mode_InUse,
};

/**************************************************************************/

#define REXX_CMD_LEN       64
#define NAME_LEN           32
#define PATH_LEN          256
#define PORT_LEN           32

#define SHOWCMD_LEN       REXX_CMD_LEN
#define TOFRONTCMD_LEN    REXX_CMD_LEN
#define OPENURLCMD_LEN    REXX_CMD_LEN
#define OPENURLWCMD_LEN   REXX_CMD_LEN
#define WRITEMAILCMD_LEN  (REXX_CMD_LEN*2)

/**************************************************************************/
/*
** Version 4 Prefs
*/

#define PREFS_VERSION ((UBYTE)4)

struct URL_Prefs
{
   UBYTE          up_Version;         /* always check this version number!     */
   struct MinList up_BrowserList;     /* list of struct URL_BrowserNodes       */
   struct MinList up_MailerList;      /* list of struct URL_MailerNodes        */
   struct MinList up_FTPList;         /* list of struct URL_MailerNodes        */

   ULONG          up_Flags;           /* flags, see below                      */

   ULONG          up_DefShow;         /* these BOOLs are the defaults for      */
   ULONG          up_DefBringToFront; /* the similarly named tags              */
   ULONG          up_DefNewWindow;    /* they are all new with Version 2       */
   ULONG          up_DefLaunch;
};

/* up_Flags */
enum
{
   UPF_ISDEFAULTS  = 1<<0, /* structure contains the default settings     */
   UPF_PREPENDHTTP = 1<<1, /* prepend "http://" to URLs w/o scheme        */
   UPF_DOMAILTO    = 1<<2, /* mailto: URLs get special treatment          */
   UPF_DOFTP       = 1<<3, /* ftp:// URLs get special treatment           */
};

/**************************************************************************/
/*
** Common #?_Flags values
*/

enum
{
   UNF_DISABLED = 1<<1,  /* The entry is disabled */

   UNF_NEW      = 1<<16, /* Reserved for OpenURL preferences application */
   UNF_NTALLOC  = 1<<17, /* Reserved for OpenURL preferences application */
};

/**************************************************************************/
/*
** Browsers
*/

struct URL_BrowserNode
{
   struct MinNode ubn_Node;
   ULONG          ubn_Flags;                        /* flags, see below                 */
   char           ubn_Name[NAME_LEN];               /* name of webbrowser                */
   char           ubn_Path[PATH_LEN];               /* complete path to browser          */
   char           ubn_Port[PORT_LEN];               /* webbrowser arexx port             */
   char           ubn_ShowCmd[SHOWCMD_LEN];         /* command to show/uniconify browser */
   char           ubn_ToFrontCmd[TOFRONTCMD_LEN];   /* command to bring browser to front */
   char           ubn_OpenURLCmd[OPENURLCMD_LEN];   /* command to open url               */
   char           ubn_OpenURLWCmd[OPENURLWCMD_LEN]; /* command to open url in new window */
};

/* ubn_Flags */
enum
{
    /*
    ** If set, browser supports getting an URL on
    ** the commandline when launched. obsolete as
    ** of V3 - use %u on commandline instead
    */
    UBNF_URLONCMDLINE = 1<<0,
};

/**************************************************************************/
/*
** Mailers
*/

struct URL_MailerNode
{
   struct MinNode umn_Node;
   ULONG          umn_Flags;                          /* flags, none defined              */
   char           umn_Name[NAME_LEN];                 /* name of mailer                   */
   char           umn_Path[PATH_LEN];                 /* complete path to mailer          */
   char           umn_Port[PORT_LEN];                 /* mailer arexx port                */
   char           umn_ShowCmd[SHOWCMD_LEN];           /* command to show/uniconify mailer */
   char           umn_ToFrontCmd[TOFRONTCMD_LEN];     /* command to bring mailer to front */
   char           umn_WriteMailCmd[WRITEMAILCMD_LEN]; /* command to write mail            */
};

/**************************************************************************/
/*
** FTPs
*/

struct URL_FTPNode
{
   struct MinNode ufn_Node;
   ULONG          ufn_Flags;                        /* flags, see below                     */
   char           ufn_Name[NAME_LEN];               /* name of ftp client                   */
   char           ufn_Path[PATH_LEN];               /* complete path to ftp client          */
   char           ufn_Port[PORT_LEN];               /* webbrowser arexx port                */
   char           ufn_ShowCmd[SHOWCMD_LEN];         /* command to show/uniconify ftp client */
   char           ufn_ToFrontCmd[TOFRONTCMD_LEN];   /* command to bring ftp client to front */
   char           ufn_OpenURLCmd[OPENURLCMD_LEN];   /* command to open url                  */
   char           ufn_OpenURLWCmd[OPENURLWCMD_LEN]; /* command to open url in new window    */
};

/* ufn_Flags */
enum
{
   /* If set, ftp:// ise removed from the URL */
   UFNF_REMOVEFTP = 1<<0,
};

/**************************************************************************/

#if defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack()
  #elif defined(__VBCC__)
    #pragma default-align
  #endif
#endif

#endif /* LIBRARIES_OPENURL_H */
