#ifndef LIBRARIES_PAM_H
#define LIBRARIES_PAM_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - pluggable authentication for AROS.

          An application that has to establish who somebody is (a login
          program, a network service, Envoy) starts a transaction for a
          named service, lets the library run the administrator's stack of
          modules for that service, and talks to the person - if there is
          one - through a conversation hook. Concepts, names and values
          follow the common PAM interface so that ported software can use the pam_*()
          names through a link library.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#define PAMNAME                 "pam.library"
#define PAMVERSION              (1)

/* The opaque transaction handle */
struct PamHandle;

/*==========================================================================*/
/* Result codes (values as in the common PAM interface)                     */
/*==========================================================================*/

#define PAM_SUCCESS                 0
#define PAM_OPEN_ERR                1   /* module could not be loaded                  */
#define PAM_SYMBOL_ERR              2   /* module lacks the entry point                */
#define PAM_SERVICE_ERR             3   /* error in the service configuration          */
#define PAM_SYSTEM_ERR              4   /* system error                                */
#define PAM_BUF_ERR                 5   /* memory                                      */
#define PAM_PERM_DENIED             6   /* permission denied                           */
#define PAM_AUTH_ERR                7   /* authentication failure                      */
#define PAM_CRED_INSUFFICIENT       8   /* caller lacks the credentials to authenticate */
#define PAM_AUTHINFO_UNAVAIL        9   /* the authentication source cannot be reached */
#define PAM_USER_UNKNOWN            10  /* user not known to the module                */
#define PAM_MAXTRIES                11  /* retry limit reached; locked out             */
#define PAM_NEW_AUTHTOK_REQD        12  /* password must be changed first              */
#define PAM_ACCT_EXPIRED            13  /* account expired                             */
#define PAM_SESSION_ERR             14  /* session could not be opened/closed          */
#define PAM_CRED_UNAVAIL            15  /* credentials cannot be retrieved             */
#define PAM_CRED_EXPIRED            16  /* credentials expired                         */
#define PAM_CRED_ERR                17  /* credentials could not be set                */
#define PAM_NO_MODULE_DATA          18  /* PamGetData(): no such data                  */
#define PAM_CONV_ERR                19  /* the conversation failed                     */
#define PAM_AUTHTOK_ERR             20  /* password could not be changed               */
#define PAM_AUTHTOK_RECOVERY_ERR    21  /* old password could not be recovered         */
#define PAM_AUTHTOK_LOCK_BUSY       22  /* password database locked                    */
#define PAM_AUTHTOK_DISABLE_AGING   23
#define PAM_TRY_AGAIN               24  /* preliminary check failed; retry             */
#define PAM_IGNORE                  25  /* module result to be ignored by the stack    */
#define PAM_ABORT                   26  /* critical error                              */
#define PAM_AUTHTOK_EXPIRED         27
#define PAM_MODULE_UNKNOWN          28  /* no such module                              */
#define PAM_BAD_ITEM                29  /* PamSetItem()/PamGetItem(): no such item     */
#define PAM_CONV_AGAIN              30
#define PAM_INCOMPLETE              31

/*==========================================================================*/
/* Items (PamSetItem / PamGetItem)                                          */
/*==========================================================================*/

#define PAM_SERVICE                 1   /* (STRPTR) the service name                   */
#define PAM_USER                    2   /* (STRPTR) the user name                      */
#define PAM_TTY                     3   /* (STRPTR) terminal/console name, if any      */
#define PAM_RHOST                   4   /* (STRPTR) remote host of a network login     */
#define PAM_CONV                    5   /* (struct Hook *) the conversation hook       */
#define PAM_AUTHTOK                 6   /* (STRPTR) the token (password). Modules only */
#define PAM_OLDAUTHTOK              7   /* (STRPTR) the old token. Modules only        */
#define PAM_RUSER                   8   /* (STRPTR) the remote user name               */
#define PAM_USER_PROMPT             9   /* (STRPTR) prompt for the user name           */
#define PAM_FAIL_DELAY              10  /* (APTR) unused; see PamFailDelay()           */
#define PAM_XDISPLAY                11
#define PAM_XAUTHDATA               12
#define PAM_AUTHTOK_TYPE            13  /* (STRPTR) word used in password prompts      */

/* AROS additions */
#define PAM_AROS_INPUT              0x100   /* (BPTR) console input for the stock conversation  */
#define PAM_AROS_OUTPUT             0x101   /* (BPTR) console output                             */
#define PAM_AROS_PUBSCREEN          0x102   /* (STRPTR) public screen for a graphical one        */
#define PAM_AROS_TASK               0x103   /* (struct Task *) task PamSetCred() acts on;         */
                                            /* default: the calling task                         */
#define PAM_AROS_AUTHTOK_FORMAT     0x104   /* (IPTR) PAMTOK_#?: what PAM_AUTHTOK holds           */

/* PAM_AROS_AUTHTOK_FORMAT */
#define PAMTOK_CLEAR                0       /* a clear-text password (default)                   */
#define PAMTOK_ENVOY                1       /* ACrypt(password, lower-cased user): the 11 chars  */
                                            /* an Envoy client sends; only a module that holds a */
                                            /* comparable value can accept it                    */

/*==========================================================================*/
/* Flags                                                                    */
/*==========================================================================*/

#define PAM_SILENT                  0x8000  /* no informational messages           */
#define PAM_DISALLOW_NULL_AUTHTOK   0x0001  /* PamAuthenticate(): refuse empty tokens */

#define PAM_ESTABLISH_CRED          0x0002  /* PamSetCred()                        */
#define PAM_DELETE_CRED             0x0004
#define PAM_REINITIALIZE_CRED       0x0008
#define PAM_REFRESH_CRED            0x0010

#define PAM_CHANGE_EXPIRED_AUTHTOK  0x0020  /* PamChAuthTok()                      */
#define PAM_PRELIM_CHECK            0x4000  /* PamChAuthTok(): first pass          */
#define PAM_UPDATE_AUTHTOK          0x2000  /* PamChAuthTok(): second pass         */

/*==========================================================================*/
/* Conversation                                                             */
/*==========================================================================*/

#define PAM_PROMPT_ECHO_OFF         1   /* ask, do not echo (a password)       */
#define PAM_PROMPT_ECHO_ON          2   /* ask, echo (a user name)             */
#define PAM_ERROR_MSG               3   /* show an error                       */
#define PAM_TEXT_INFO               4   /* show a message                      */

struct pam_message
{
    LONG                msg_style;      /* PAM_PROMPT_#?, PAM_ERROR_MSG, PAM_TEXT_INFO */
    CONST_STRPTR        msg;
};

struct pam_response
{
    STRPTR              resp;           /* AllocVec()'d answer, or NULL                */
    LONG                resp_retcode;   /* unused, 0                                   */
};

/*
 * The conversation hook is called as
 *     LONG conv(struct Hook *hook, struct PamHandle *handle, struct PamConvMsg *msg)
 * and returns PAM_SUCCESS with msg->Responses pointing to an AllocVec()'d
 * array of NumMsg responses (one per message, resp AllocVec()'d or NULL),
 * or PAM_CONV_ERR. The library frees the responses after wiping them.
 *
 * Without a hook the library uses one of its own:
 *   - PAMT_Interactive TRUE: the console (PAM_AROS_INPUT/OUTPUT, default
 *     the process's Input()/Output()), passwords unechoed;
 *   - otherwise the "preset" conversation, which answers prompts only from
 *     PAM_USER / PAM_AUTHTOK and fails any other question, so that a server
 *     can never block waiting for a person.
 */
struct PamConvMsg
{
    LONG                        NumMsg;
    const struct pam_message    **Messages;
    struct pam_response         *Responses;     /* out */
    APTR                        UserData;       /* from PAMT_ConvData */
};

/*==========================================================================*/
/* Tags for PamStartA()                                                     */
/*==========================================================================*/

#define PAMT_Base                   (TAG_USER + 0x5000)
#define PAMT_Interactive            (PAMT_Base + 1)     /* (BOOL) use the console conversation when no hook is given */
#define PAMT_Input                  (PAMT_Base + 2)     /* (BPTR) as PAM_AROS_INPUT                                   */
#define PAMT_Output                 (PAMT_Base + 3)     /* (BPTR) as PAM_AROS_OUTPUT                                  */
#define PAMT_PubScreen              (PAMT_Base + 4)     /* (STRPTR) as PAM_AROS_PUBSCREEN                             */
#define PAMT_RemoteHost             (PAMT_Base + 5)     /* (STRPTR) as PAM_RHOST                                      */
#define PAMT_RemoteUser             (PAMT_Base + 6)     /* (STRPTR) as PAM_RUSER                                      */
#define PAMT_AuthTok                (PAMT_Base + 7)     /* (STRPTR) preset token (password)                           */
#define PAMT_AuthTokFormat          (PAMT_Base + 8)     /* (IPTR) PAMTOK_#?                                           */
#define PAMT_Task                   (PAMT_Base + 9)     /* (struct Task *) as PAM_AROS_TASK                           */
#define PAMT_ConvData               (PAMT_Base + 10)    /* (APTR) passed to the hook in PamConvMsg.UserData           */

/*==========================================================================*/
/* Module-side                                                              */
/*==========================================================================*/

/* Cleanup function for PamSetData() */
typedef void (*PamDataCleanup)(struct PamHandle *handle, APTR data, LONG status);

/* PamLog() levels */
#define PAM_LOG_ERROR               0
#define PAM_LOG_WARNING             1
#define PAM_LOG_INFO                2
#define PAM_LOG_DEBUG               3

/* Where modules live and where the per-service files are */
#define PAM_MODULE_DIR              "SYS:Libs/PAM"
#define PAM_MODULE_SUFFIX           ".pam"
#define PAM_CONFIG_DIR              "SYS:Security/PAM"
#define PAM_DEFAULT_SERVICE         "other"

#endif /* LIBRARIES_PAM_H */
