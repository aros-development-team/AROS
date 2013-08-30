/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "lib.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdio.h>

#if !defined(__amigaos4__)
#include <dos/dostags.h>
#include <exec/execbase.h>
#endif

#include "debug.h"

/**************************************************************************/

#define FINDPORT_NUM  100  /* how many FindPort() to do while waiting */
#define FINDPORT_TIME  10  /* how many seconds to spread those FindPort() over */
#define FINDPORT_DTIME ((FINDPORT_TIME * TICKS_PER_SECOND)/FINDPORT_NUM)

struct placeHolder
{
    TEXT ph_Char;
    STRPTR ph_String;
};

#define PH_COUNT_BROWSER    2
#define PH_COUNT_MAILER     6
#define PH_COUNT_FTP        2

/**************************************************************************/

static STRPTR expandPlaceHolders(STRPTR template, struct placeHolder *ph, int num)
{
    STRPTR p, res = NULL;
    int i, length = 0;

    ENTER();

    for (p = template; *p; p++)
    {
        for (i = 0; i<num; i++)
        {
            if ((*p=='%') && (*(p+1)== ph[i].ph_Char))
                length += strlen(ph[i].ph_String);
        }

        length++;
    }

    if ((res = allocArbitrateVecPooled(length+1)) != NULL)
    {
        for (p = res; *template; template++)
        {
            for (i = 0; i<num; i++)
                if ((*template=='%') && (*(template+1)== ph[i].ph_Char))
                    break;

            if (i<num)
            {
                strcpy(p,ph[i].ph_String);
                p += strlen(ph[i].ph_String);
                template++;
                continue;
            }

            *p++ = *template;
        }

        *p = '\0';
    }

    RETURN(res);
    return res;
}

/**************************************************************************/

static BOOL writeToFile(STRPTR fileName, STRPTR str)
{
    BOOL res = FALSE;
    BPTR fh;

    ENTER();

    if((fh = Open(fileName, MODE_NEWFILE)))
    {
        LONG len = strlen(str);

        if(Write(fh, str, len)==len)
            res = TRUE;

        Close(fh);
    }

    RETURN(res);
    return res;
}

/**************************************************************************/

static STRPTR findRexxPort(struct List *list,STRPTR name)
{
    STRPTR portName = NULL;
    struct Node *n;
    ULONG len;

    ENTER();

    /* find a rexx port, allowing a .<number> extension */

    len = strlen(name);

    for(n = list->lh_Head; n->ln_Succ; n = n->ln_Succ)
    {
        if(n->ln_Name != NULL && strncmp(n->ln_Name, name, len) == 0 &&
            (n->ln_Name[len] == '\0' || (n->ln_Name[len] == '.' && isdigits(&n->ln_Name[len+1]))))
        {
            portName = n->ln_Name;
            break;
        }
    }

    RETURN(portName);
    return portName;
}

/**************************************************************************/

static STRPTR waitForRexxPort(STRPTR port)
{
    STRPTR name = NULL;
    int i;

    ENTER();

    /* (busy) wait for the port to appear */

    for(i = 0; i<FINDPORT_NUM; i++)
    {
        STRPTR rxport;

        Forbid();
        rxport = findRexxPort(&((struct ExecBase *)SysBase)->PortList, port);
        Permit();

        if(rxport != NULL)
        {
          name = rxport;
          break;
        }

        if(SetSignal(0, 0) & SIGBREAKF_CTRL_C)
        {
          name = NULL;
          break;
        }

        Delay(FINDPORT_DTIME);
    }

    RETURN(name);
    return name;
}

/**************************************************************************/

static BOOL sendRexxMsg(STRPTR rxport, STRPTR rxcmd)
{
  BOOL res = FALSE;
  struct Process *proc;

  ENTER();

  D(DBF_STARTUP, "sending command '%s' to port '%s'", rxcmd, rxport);

  #if defined(__MORPHOS__)
  proc = CreateNewProcTags(NP_Entry,        handler,
                           NP_CodeType,     CODETYPE_PPC,
                           NP_PPCStackSize, 8192,
                           NP_StackSize,    4096,
                           NP_Name,         "OpenURL - Handler",
                           NP_CopyVars,     FALSE,
                           NP_Input,        NULL,
                           NP_CloseInput,   FALSE,
                           NP_Output,       NULL,
                           NP_CloseOutput,  FALSE,
                           NP_Error,        NULL,
                           NP_CloseError,   FALSE,
                           TAG_DONE);
  #else
  proc = CreateNewProcTags(NP_Entry,        handler,
                           NP_StackSize,    4096,
                           NP_Name,         "OpenURL - Handler",
                           NP_CopyVars,     FALSE,
                           NP_Input,        NULL,
                           NP_CloseInput,   FALSE,
                           NP_Output,       NULL,
                           NP_CloseOutput,  FALSE,
                           NP_Error,        NULL,
                           NP_CloseError,   FALSE,
                           TAG_DONE);
  #endif

  if(proc != NULL)
  {
    struct MsgPort *port;

    #if defined(__amigaos4__)
    port = AllocSysObjectTags(ASOT_PORT, TAG_DONE);
    #else
    port = CreateMsgPort();
    #endif

    if(port != NULL)
    {
      struct startMsg *smsg;

      #if defined(__amigaos4__)
      smsg = AllocSysObjectTags(ASOT_MESSAGE, ASOMSG_Size, sizeof(*smsg),
                                              ASOMSG_ReplyPort, port,
                                              TAG_DONE);
      #else
      smsg = allocArbitrateVecPooled(sizeof(*smsg));
      #endif

      if(smsg != NULL)
      {
        #if !defined(__amigaos4__)
        INITMESSAGE(smsg, port, sizeof(*smsg));
        #endif

        smsg->port = rxport;
        smsg->cmd = rxcmd;

        ObtainSemaphore(&OpenURLBase->libSem);
        OpenURLBase->rexx_use++;
        ReleaseSemaphore(&OpenURLBase->libSem);

        PutMsg(&proc->pr_MsgPort, (struct Message *)smsg);
        WaitPort(port);
        GetMsg(port);

        res = smsg->res;

        #if defined(__amigaos4__)
        FreeSysObject(ASOT_MESSAGE, smsg);
        #else
        freeArbitrateVecPooled(smsg);
        #endif
      }

      #if defined(__amigaos4__)
      FreeSysObject(ASOT_PORT, port);
      #else
      DeleteMsgPort(port);
      #endif
    }
  }

  RETURN(res);
  return res;
}

/****************************************************************************/

BOOL sendToBrowser(STRPTR URL, struct List *portlist, ULONG flags, STRPTR pubScreenName)
{
    BOOL res = FALSE;
    STRPTR cmd = NULL;
    struct placeHolder ph[PH_COUNT_BROWSER];
    struct URL_BrowserNode *bn;

    ENTER();

    /* set up the placeholder mapping */

    ph[0].ph_Char = 'u'; ph[0].ph_String = URL;
    ph[1].ph_Char = 'p'; ph[1].ph_String = pubScreenName ? pubScreenName : (STRPTR)"Workbench";

    /* try to find one of the browsers in the list */

    for (bn = (struct URL_BrowserNode *)OpenURLBase->prefs->up_BrowserList.mlh_Head;
         bn->ubn_Node.mln_Succ;
         bn = (struct URL_BrowserNode *)bn->ubn_Node.mln_Succ)
    {
        STRPTR port;

        if(isFlagSet(bn->ubn_Flags, UNF_DISABLED))
            continue;
        if(bn->ubn_Path[0] == '\0')
            continue;

        port = findRexxPort(portlist,bn->ubn_Port);

        if (port)
        {
            /* send uniconify msg */

            if (isFlagSet(flags, SENDTOF_SHOW) && *bn->ubn_ShowCmd)
                sendRexxMsg(port,bn->ubn_ShowCmd);

            /* send screentofront command */

            if (isFlagSet(flags, SENDTOF_TOFRONT) && *bn->ubn_ToFrontCmd)
                sendRexxMsg(port,bn->ubn_ToFrontCmd);

            /* try sending openurl msg */

            if (!(cmd = expandPlaceHolders(isFlagSet(flags, SENDTOF_NEWWINDOW) ? bn->ubn_OpenURLWCmd : bn->ubn_OpenURLCmd,ph,PH_COUNT_BROWSER)))
                goto done;

            if (!(res = sendRexxMsg(port,cmd)))
            {
                freeArbitrateVecPooled(cmd);
                cmd = NULL;
            }
            else goto done;
        }
    }

    /* no running browser, launch a new one */

    if (isFlagClear(flags, SENDTOF_LAUNCH))
        goto done;

    for (bn = (struct URL_BrowserNode *)OpenURLBase->prefs->up_BrowserList.mlh_Head;
         bn->ubn_Node.mln_Succ;
         bn = (struct URL_BrowserNode *)bn->ubn_Node.mln_Succ)
    {
        ULONG  startOnly;
        STRPTR filePart;
        TEXT   c = '\0';
        BPTR   lock;
        LONG   error;

        if(isFlagSet(bn->ubn_Flags, UNF_DISABLED))
            continue;
        if(bn->ubn_Path[0] == '\0')
            continue;

        /* compose commandline */

        if (strstr(bn->ubn_Path,"%u")) startOnly = TRUE;
        else startOnly = FALSE;

        if (!(cmd = expandPlaceHolders(bn->ubn_Path,ph,PH_COUNT_BROWSER)))
            goto done;

        filePart = FilePart(bn->ubn_Path);

        if (filePart)
        {
            c         = *filePart;
            *filePart = '\0';
        }

        lock = Lock(bn->ubn_Path,ACCESS_READ);

        if (filePart) *filePart = c;

        /* start the browser */

        error = SystemTags(cmd,SYS_Asynch,    TRUE,
                               SYS_Input,     Open("NIL:",MODE_NEWFILE),
                               SYS_Output,    NULL,
                               SYS_Error,     NULL,
                               lock ? NP_CurrentDir : TAG_IGNORE, lock,
                               TAG_DONE);

        freeArbitrateVecPooled(cmd);
        cmd = NULL;

        if (error)
        {
            if (lock) UnLock(lock);
            continue;
        }

        if (!startOnly)
        {
            STRPTR rxport;

            /* send urlopen command */

            if (!(cmd = expandPlaceHolders(bn->ubn_OpenURLCmd,ph,PH_COUNT_BROWSER)))
                goto done;

            /* wait for the port to appear */

            if ((rxport = waitForRexxPort(bn->ubn_Port)))
                res = sendRexxMsg(rxport,cmd);

            break;
        }
        else
        {
            res = TRUE;
            break;
        }
    }

done:
    if(cmd)
      freeArbitrateVecPooled(cmd);

    RETURN(res);
    return res;
}

/**************************************************************************/

BOOL sendToFTP(STRPTR URL, struct List *portlist, ULONG flags, STRPTR pubScreenName)
{
    BOOL res = FALSE;
    STRPTR cmd = NULL;
    struct placeHolder ph[PH_COUNT_FTP];
    struct URL_FTPNode *fn;

    ENTER();

    /* set up the placeholder mapping */

    ph[0].ph_Char = 'u'; /*ph[0].ph_String = URL;*/
    ph[1].ph_Char = 'p'; ph[1].ph_String = pubScreenName ? pubScreenName : (STRPTR)"Workbench";

    /* try to find one of the ftp client in the list */

    for (fn = (struct URL_FTPNode *)OpenURLBase->prefs->up_FTPList.mlh_Head;
         fn->ufn_Node.mln_Succ;
         fn = (struct URL_FTPNode *)fn->ufn_Node.mln_Succ)
    {
        STRPTR port;

        if(isFlagSet(fn->ufn_Flags, UNF_DISABLED))
            continue;

        port = findRexxPort(portlist,fn->ufn_Port);

        if (port)
        {
            /* send uniconify msg */

            if (isFlagSet(flags, SENDTOF_SHOW) && *fn->ufn_ShowCmd)
                sendRexxMsg(port,fn->ufn_ShowCmd);

            /* send screentofront command */

            if (isFlagSet(flags, SENDTOF_TOFRONT) && *fn->ufn_ToFrontCmd)
                sendRexxMsg(port,fn->ufn_ToFrontCmd);

            /* try sending openurl msg */

            if(isFlagSet(fn->ufn_Flags, UFNF_REMOVEFTP) && !Strnicmp(URL,"ftp://",6))
                ph[0].ph_String = URL+6;
            else
                ph[0].ph_String = URL+6;

            if (!(cmd = expandPlaceHolders(isFlagSet(flags, SENDTOF_NEWWINDOW) ? fn->ufn_OpenURLWCmd : fn->ufn_OpenURLCmd,ph,PH_COUNT_FTP)))
                goto done;

            if (!(res = sendRexxMsg(port,cmd)))
            {
                freeArbitrateVecPooled(cmd);
                cmd = NULL;
            }
            else goto done;
        }
    }

    /* no running ftp client, launch a new one */

    if (isFlagClear(flags, SENDTOF_LAUNCH))
        goto done;

    for (fn = (struct URL_FTPNode *)OpenURLBase->prefs->up_FTPList.mlh_Head;
         fn->ufn_Node.mln_Succ;
         fn = (struct URL_FTPNode *)fn->ufn_Node.mln_Succ)
    {
        ULONG  startOnly;
        STRPTR filePart;
        TEXT   c = '\0';
        BPTR   lock;
        LONG   error;

        if(isFlagSet(fn->ufn_Flags, UNF_DISABLED))
            continue;
        if(fn->ufn_Path[0] == '\0')
            continue;

        /* compose commandline */

        if (strstr(fn->ufn_Path,"%u"))
            startOnly = TRUE;
        else
            startOnly = FALSE;

        if(isFlagSet(fn->ufn_Flags, UFNF_REMOVEFTP) && !Strnicmp(URL,"ftp://",6))
            ph[0].ph_String = URL+6;
        else
            ph[0].ph_String = URL+6;

        if (!(cmd = expandPlaceHolders(fn->ufn_Path,ph,PH_COUNT_FTP)))
            goto done;

        filePart = FilePart(fn->ufn_Path);

        if (filePart)
        {
            c         = *filePart;
            *filePart = '\0';
        }

        lock = Lock(fn->ufn_Path,ACCESS_READ);

        if (filePart) *filePart = c;

        /* start the ftp client */

        error = SystemTags(cmd,SYS_Asynch,    TRUE,
                               SYS_Input,     Open("NIL:",MODE_NEWFILE),
                               SYS_Output,    NULL,
                               SYS_Error,     NULL,
                               lock ? NP_CurrentDir : TAG_IGNORE, lock,
                               TAG_DONE);

        freeArbitrateVecPooled(cmd);
        cmd = NULL;

        if (error)
        {
            if (lock) UnLock(lock);
            continue;
        }

        if (!startOnly)
        {
            STRPTR rxport;

            /* send urlopen command */

            if (!(cmd = expandPlaceHolders(fn->ufn_OpenURLCmd,ph,PH_COUNT_FTP)))
                goto done;

            /* wait for the port to appear */

            if ((rxport = waitForRexxPort(fn->ufn_Port)))
                res = sendRexxMsg(rxport,cmd);

            break;
        }
        else
        {
            res = TRUE;
            break;
        }
    }

done:
    if(cmd)
      freeArbitrateVecPooled(cmd);

    RETURN(res);
    return res;
}

/**************************************************************************/

static WORD trans[256];

BOOL sendToMailer(STRPTR URL, struct List *portlist, ULONG flags, STRPTR pubScreenName)
{
    struct placeHolder    ph[PH_COUNT_MAILER];
    struct URL_MailerNode *mn;
    STRPTR                start, end, data, address = NULL, subject = NULL, body = NULL,
                          cmd = NULL, *tag;
    TEXT                  fileName[32];
    BOOL                  res = FALSE, written = FALSE;
    UWORD                 offset, len;

    ENTER();

    /* setup trans */
    ObtainSemaphore(&OpenURLBase->libSem);
    if(isFlagClear(OpenURLBase->flags, BASEFLG_Trans))
    {
        for (len = 0; len<256; len++) trans[len] = -1;

        trans['0'] = 0;
        trans['1'] = 1;
        trans['2'] = 2;
        trans['3'] = 3;
        trans['4'] = 4;
        trans['5'] = 5;
        trans['6'] = 6;
        trans['7'] = 7;
        trans['8'] = 8;
        trans['9'] = 9;
        trans['A'] = trans['a'] = 10;
        trans['B'] = trans['b'] = 11;
        trans['C'] = trans['c'] = 12;
        trans['D'] = trans['d'] = 13;
        trans['E'] = trans['e'] = 14;
        trans['F'] = trans['f'] = 15;

        SET_FLAG(OpenURLBase->flags, BASEFLG_Trans);
    }
    ReleaseSemaphore(&OpenURLBase->libSem);

    /* parse the URL "mailto:user@host.domain?subject=Subject&body=Body" */

    start = URL;
    while (start)
    {
        tag = NULL;
        end = NULL;
        offset = 1;

        /* Use utility.library - Piru */

        if (!Strnicmp(start,"mailto:",7))
        {
            tag = &address;
            offset = 7;
            end = strchr(start+offset,'?');
        }
        else if ((!Strnicmp(start,"?subject=",9)) || (!Strnicmp(start,"&subject=",9)))
        {
            tag = &subject;
            offset = 9;
            end = strchr(start+offset,'&');
        }
        else if ((!Strnicmp(start,"?body=",6)) || (!Strnicmp(start,"&body=",6)))
        {
            tag = &body;
            offset = 6;
            end = strchr(start+offset,'&');
        }

        /* if we found some data && we even found it the first time ! */
        if (tag && !*tag)
        {
            data=start+offset;

            if (end) len=end-data;
            else len=strlen(data);

            if(!(*tag = allocArbitrateVecPooled(len+1)))
              goto done;

            strncpy(*tag,data,len);
            *((*tag)+len)='\0';

            /* decode %XX sequences in urls */
            data=*tag;
            while (data)
            {
                if ((data=strchr(data,'%')) && (trans[(int)data[1]]!=-1) && (trans[(int)data[2]]!=-1))
                {
                    *data=(trans[(int)data[1]]<<4)|trans[(int)data[2]];
                    data++;
                    memmove(data,data+2,strlen(data+2)+1);
                }
            }
        }

        start = end;
    }

    if(body)
      snprintf(fileName, sizeof(fileName), "T:OpenURL-MailBody.%016lx", (IPTR)FindTask(NULL));
    else
    {
      written = TRUE;
      strlcpy(fileName, "NIL:", sizeof(fileName));
    }

    /* set up the placeholder mapping */

    ph[0].ph_Char = 'a'; ph[0].ph_String = address ? address : (STRPTR)"";
    ph[1].ph_Char = 's'; ph[1].ph_String = subject ? subject : (STRPTR)"";//URL;
    ph[2].ph_Char = 'b'; ph[2].ph_String = body ? body : (STRPTR)"";
    ph[3].ph_Char = 'f'; ph[3].ph_String = fileName;
    ph[4].ph_Char = 'u'; ph[4].ph_String = URL;
    ph[5].ph_Char = 'p'; ph[5].ph_String = pubScreenName ? pubScreenName : (STRPTR)"Workbench";

    /* try to find one of the mailers in the list */

    for (mn = (struct URL_MailerNode *)OpenURLBase->prefs->up_MailerList.mlh_Head;
         mn->umn_Node.mln_Succ;
         mn = (struct URL_MailerNode *)mn->umn_Node.mln_Succ)
    {
        STRPTR rxport;

        if(isFlagSet(mn->umn_Flags, UNF_DISABLED))
            continue;

        rxport = findRexxPort(portlist,mn->umn_Port);

        if (rxport)
        {
            /* send uniconify msg */

            if (isFlagSet(flags, SENDTOF_SHOW) && *mn->umn_ShowCmd)
                sendRexxMsg(rxport,mn->umn_ShowCmd);

            /* send screentofront command */

            if (isFlagSet(flags, SENDTOF_TOFRONT) && *mn->umn_ToFrontCmd)
                sendRexxMsg(rxport,mn->umn_ToFrontCmd);

            /* write to temp file */

            if (!written && strstr(mn->umn_WriteMailCmd,"%f"))
                written = writeToFile(fileName,body);

            /* try sending writemail msg */

            if (!(cmd = expandPlaceHolders(mn->umn_WriteMailCmd,ph,PH_COUNT_MAILER)))
                goto done;

            /* now split each message at the ';' and send fragments */
            start = end = cmd;
            while (*start)
            {
                while ((*end) && (*end!=';'))
                {
                    end++;
                    /* skip data, which is enclosed in "" */
                    if (*end=='"')
                    {
                        end++;
                        while ((*end) && (*end!='"')) end++;
                        if (*end=='"') end++;
                    }
                }
                /* are there more commands */
                if (*end==';')
                {
                    *end='\0';
                    end++;
                }
                if (!(res = sendRexxMsg(rxport,start)))
                {
                    /* send failed, try next mailer */
                    freeArbitrateVecPooled(cmd);
                    start = cmd = NULL;
                }
                else start=end;
            }
            /* cmd processed succesfully, return */
            if (cmd) goto done;
        }
    }

    /* no running ftp client, launch a new one */

    if (isFlagClear(flags, SENDTOF_LAUNCH))
        goto done;

    for (mn = (struct URL_MailerNode *)OpenURLBase->prefs->up_MailerList.mlh_Head;
         mn->umn_Node.mln_Succ;
         mn = (struct URL_MailerNode *)mn->umn_Node.mln_Succ)
    {
        ULONG  startOnly;
        STRPTR filePart;
        TEXT   c = '\0';
        BPTR   lock;
        LONG   error;

        if(isFlagSet(mn->umn_Flags, UNF_DISABLED))
            continue;
        if(mn->umn_Path[0] == '\0')
            continue;

        /* compose commandline */

        if (strstr(mn->umn_Path,"%a"))
            startOnly = TRUE;
        else
            startOnly = FALSE;

        if (!written && strstr(mn->umn_Path,"%f"))
            written = writeToFile(fileName,body);

        if (!(cmd = expandPlaceHolders(mn->umn_Path,ph,PH_COUNT_MAILER)))
            goto done;

        filePart = FilePart(mn->umn_Path);

        if (filePart)
        {
            c         = *filePart;
            *filePart = '\0';
        }

        lock = Lock(mn->umn_Path,ACCESS_READ);

        if (filePart) *filePart = c;

        /* start the mailer */

        error = SystemTags(cmd,SYS_Asynch,    TRUE,
                               SYS_Input,     Open("NIL:", MODE_NEWFILE),
                               SYS_Output,    NULL,
                               SYS_Error,     NULL,
                               lock ? NP_CurrentDir : TAG_IGNORE, lock,
                               TAG_DONE);

        freeArbitrateVecPooled(cmd);
        cmd = NULL;

        if (error)
        {
            if (lock) UnLock(lock);
            continue;
        }

        if (!startOnly)
        {
            STRPTR rxport;

            /* send write mail command */

            if (!written && strstr(mn->umn_WriteMailCmd,"%f"))
                /*written = */writeToFile(fileName,body);

            if (!(cmd = expandPlaceHolders(mn->umn_WriteMailCmd,ph,PH_COUNT_MAILER)))
                goto done;

            /* wait for the port to appear */

            if ((rxport = waitForRexxPort(mn->umn_Port)))
            {
                start = end = cmd;
                while (*start)
                {
                    while ((*end) && (*end!=';'))
                    {
                        end++;
                        /* skip data, which is enclosed in "" */
                        if (*end=='"')
                        {
                            end++;
                            while ((*end) && (*end!='"')) end++;
                            if (*end=='"') end++;
                        }
                    }
                    /* are there more commands */
                    if (*end==';')
                    {
                        *end='\0';
                        end++;
                    }
                    if (!(res = sendRexxMsg(rxport,start)))
                    {
                        /* send failed, try next mailer */
                        freeArbitrateVecPooled(cmd);
                        start = cmd = NULL;
                    }
                    else start=end;
                }
            }
            break;
        }
        else
        {
            res = TRUE;
            break;
        }
    }

done:
    if (cmd)     freeArbitrateVecPooled(cmd);
    if (body)    freeArbitrateVecPooled(body);
    if (subject) freeArbitrateVecPooled(subject);
    if (address) freeArbitrateVecPooled(address);

    RETURN(res);
    return res;
}

/**************************************************************************/

BOOL copyList(struct List *dst, struct List *src, ULONG size)
{
    BOOL success = TRUE;
    struct Node *n, *new;

    ENTER();

    /* copy src list into dst, and return success */

    for(n = src->lh_Head; n->ln_Succ; n = n->ln_Succ)
    {
        if((new = allocArbitrateVecPooled(size)) == NULL)
        {
            freeList(dst);
            success = FALSE;
            break;
        }

        CopyMem(n,new,size);
        AddTail(dst,new);
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void freeList(struct List *list)
{
  struct Node *n;

  ENTER();

  while((n = RemHead(list)) != NULL)
    freeArbitrateVecPooled(n);

  LEAVE();
}

/**************************************************************************/

BOOL isdigits(STRPTR str)
{
  BOOL result = FALSE;

  ENTER();

  for(;;)
  {
    if(*str == '\0')
    {
      result = TRUE;
      break;
    }
    else if(!isdigit(*str))
      break;

    str++;
  }

  RETURN(result);
  return result;
}

#if !defined(HAVE_ALLOCVECPOOLED)
APTR allocVecPooled(APTR pool, ULONG size)
{
  ULONG *mem;

  ENTER();

  size += sizeof(ULONG);
  if((mem = AllocPooled(pool, size)))
    *mem++ = size;

  RETURN(mem);
  return mem;
}
#endif

/****************************************************************************/

#if !defined(HAVE_FREEVECPOOLED)
void freeVecPooled(APTR pool,APTR mem)
{
  ENTER();

  FreePooled(pool,(LONG *)mem - 1,*((LONG *)mem - 1));

  LEAVE();
}
#endif

/****************************************************************************/

APTR reallocVecPooled(APTR pool, APTR mem, ULONG oldSize, ULONG newSize)
{
  ULONG *newMem;

  ENTER();

  if((newMem = allocVecPooled(pool, newSize)) != NULL)
  {
    memcpy(newMem, mem, (oldSize < newSize) ? oldSize : newSize);

    freeVecPooled(pool, mem);
  }

  RETURN(newMem);
  return newMem;
}

/****************************************************************************/

APTR allocArbitrateVecPooled(ULONG size)
{
  ULONG *mem;

  ENTER();

  ObtainSemaphore(&OpenURLBase->poolSem);
  mem = allocVecPooled(OpenURLBase->pool, size);
  ReleaseSemaphore(&OpenURLBase->poolSem);

  RETURN(mem);
  return mem;
}

/****************************************************************************/

void freeArbitrateVecPooled(APTR mem)
{
  ENTER();

  ObtainSemaphore(&OpenURLBase->poolSem);
  freeVecPooled(OpenURLBase->pool, mem);
  ReleaseSemaphore(&OpenURLBase->poolSem);

  LEAVE();
}

/****************************************************************************/

APTR reallocArbitrateVecPooled(APTR mem, ULONG oldSize, ULONG newSize)
{
  ENTER();

  ObtainSemaphore(&OpenURLBase->poolSem);
  mem = reallocVecPooled(OpenURLBase->pool, mem, oldSize, newSize);
  ReleaseSemaphore(&OpenURLBase->poolSem);

  RETURN(mem);
  return mem;
}

/****************************************************************************/

