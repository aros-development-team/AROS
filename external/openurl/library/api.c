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

#include <dos/dostags.h>
#include <exec/execbase.h>

#include "version.h"
#include "debug.h"

#include <stdio.h>

/**************************************************************************/

ULONG LIBFUNC URL_OpenA(REG(a0,STRPTR URL), REG(a1,struct TagItem *attrs))
{
  struct List portList;
  TEXT buf[256];
  STRPTR pubScreenName = NULL;
  STRPTR fullURL = NULL;
  ULONG res;
  BOOL httpPrepend = FALSE;
  ULONG flags;

  ENTER();

  NewList(&portList);

  ObtainSemaphore(&OpenURLBase->prefsSem);

  /* parse arguments */
  pubScreenName = (STRPTR)GetTagData(URL_PubScreenName, (IPTR)"Workbench", attrs);
  flags = 0;
  if(GetTagData(URL_Show, OpenURLBase->prefs->up_DefShow, attrs))
    SET_FLAG(flags, SENDTOF_SHOW);
  if(GetTagData(URL_BringToFront, OpenURLBase->prefs->up_DefBringToFront, attrs))
    SET_FLAG(flags, SENDTOF_TOFRONT);
  if(GetTagData(URL_NewWindow, OpenURLBase->prefs->up_DefNewWindow, attrs))
    SET_FLAG(flags, SENDTOF_NEWWINDOW);
  if(GetTagData(URL_Launch, OpenURLBase->prefs->up_DefLaunch, attrs))
    SET_FLAG(flags, SENDTOF_LAUNCH);

  /* make a copy of the global list of named ports */
  Forbid();
  res = copyList(&portList, &((struct ExecBase *)SysBase)->PortList, sizeof(struct Node));
  Permit();

  if(res == TRUE)
  {
    /* prepend "http://" if URL has no method */
    if(isFlagSet(OpenURLBase->prefs->up_Flags, UPF_PREPENDHTTP))
    {
      STRPTR colon;

      colon = strchr((STRPTR)URL,':');
      if(colon == NULL)
        httpPrepend = TRUE;
      else
      {
        STRPTR p;

        for(p = URL; p<colon; p++)
        {
          if(!isalnum(*p) && *p != '+' && *p != '-')
          {
            httpPrepend = TRUE;
            break;
          }
        }
      }
    }

    if(httpPrepend == TRUE)
    {
      ULONG len = strlen(URL) + 8;

      if(len > sizeof(buf))
        fullURL = allocArbitrateVecPooled(strlen(URL)+8);
      else
        fullURL = buf;

      if(fullURL != NULL)
        snprintf(fullURL, len, "http://%s", URL);
    }
    else
      fullURL = URL;

    if(fullURL != NULL)
    {
      /* Be case insensitive - Piru */
      if(isFlagSet(OpenURLBase->prefs->up_Flags, UPF_DOMAILTO) && Strnicmp((STRPTR)URL,"mailto:", 7) == 0)
        res = sendToMailer(fullURL, &portList, flags, pubScreenName);
      else if(isFlagSet(OpenURLBase->prefs->up_Flags, UPF_DOFTP) && Strnicmp((STRPTR)URL,"ftp://", 6) == 0)
        res = sendToFTP(fullURL, &portList, flags, pubScreenName);
      else
        res = sendToBrowser(fullURL, &portList, flags, pubScreenName);
    }
  }

  ReleaseSemaphore(&OpenURLBase->prefsSem);
  freeList(&portList);
  if(httpPrepend == TRUE && fullURL != NULL && fullURL != buf)
    freeArbitrateVecPooled(fullURL);

  RETURN(res);
  return res;
}

/**************************************************************************/

struct URL_Prefs * LIBFUNC URL_GetPrefsA(REG(a0,struct TagItem *attrs))
{
  struct URL_Prefs *p = NULL;
  ULONG mode;

  ENTER();

  mode = GetTagData(URL_GetPrefs_Mode, URL_GetPrefs_Mode_Env, attrs);

  switch(mode)
  {
    case URL_GetPrefs_Mode_Default:
    {
      if((p = allocArbitrateVecPooled(sizeof(*p))) != NULL)
        setDefaultPrefs(p);
    }
    break;

    case URL_GetPrefs_Mode_Env:
    case URL_GetPrefs_Mode_Envarc:
    {
      BOOL ok = FALSE;

      mode = (mode == URL_GetPrefs_Mode_Env) ? LOADPREFS_ENV : LOADPREFS_ENVARC;

      if((p = allocArbitrateVecPooled(sizeof(*p))) != NULL)
      {
        if(loadPrefs(p, mode) == TRUE)
        {
          ok = TRUE;
        }
        else if(GetTagData(URL_GetPrefs_FallBack, TRUE, attrs) != FALSE)
        {
          if(mode == LOADPREFS_ENV && loadPrefs(p, LOADPREFS_ENVARC) == TRUE)
          {
            ok = TRUE;
          }
          else
          {
            setDefaultPrefs(p);
            ok = TRUE;
          }
        }
      }

      if(ok == FALSE)
      {
        URL_FreePrefsA(p, NULL);
        p = NULL;
      }
    }
    break;

    default:
    {
      ObtainSemaphoreShared(&OpenURLBase->prefsSem);
      p = copyPrefs(OpenURLBase->prefs);
      ReleaseSemaphore(&OpenURLBase->prefsSem);
    }
    break;
  }

  RETURN(p);
  return p;
}

/**************************************************************************/

struct URL_Prefs * LIBFUNC URL_OldGetPrefs(void)
{
  return URL_GetPrefsA(NULL);
}

/**************************************************************************/

void LIBFUNC URL_FreePrefsA(REG(a0,struct URL_Prefs *p),UNUSED REG(a1,struct TagItem *attrs))
{
  ENTER();

  if(p != NULL)
  {
    freeList((struct List *)&p->up_BrowserList);
    freeList((struct List *)&p->up_MailerList);
    freeList((struct List *)&p->up_FTPList);
    freeArbitrateVecPooled(p);
  }

  LEAVE();
}

/**************************************************************************/

void LIBFUNC URL_OldFreePrefs(REG(a0,struct URL_Prefs *p))
{
  URL_FreePrefsA(p,NULL);
}

/**************************************************************************/

ULONG LIBFUNC URL_SetPrefsA(REG(a0,struct URL_Prefs *p),REG(a1,struct TagItem *attrs))
{
  ULONG res = FALSE;

  ENTER();

  if(p->up_Version==PREFS_VERSION)
  {
    struct URL_Prefs *newp;

    ObtainSemaphore(&OpenURLBase->prefsSem);

    if((newp = copyPrefs(p)) != NULL)
    {
      newp->up_Version = PREFS_VERSION;
      CLEAR_FLAG(newp->up_Flags, UPF_ISDEFAULTS);

      URL_FreePrefsA(OpenURLBase->prefs,NULL);
      OpenURLBase->prefs = newp;

      if(savePrefs((STRPTR)DEF_ENV, OpenURLBase->prefs) == TRUE)
      {
        if(GetTagData(URL_SetPrefs_Save, FALSE, attrs) != FALSE)
        {
          if(savePrefs((STRPTR)DEF_ENVARC, OpenURLBase->prefs) == TRUE)
            res = TRUE;
        }
        else
          res = TRUE;
      }
    }

    ReleaseSemaphore(&OpenURLBase->prefsSem);
  }

  RETURN(res);
  return res;
}

/**************************************************************************/

ULONG LIBFUNC URL_OldSetPrefs(REG(a0,struct URL_Prefs *p),REG(d0,ULONG save))
{
  struct TagItem stags[] = { { URL_SetPrefs_Save, 0         },
                             { TAG_DONE,          TAG_DONE  } };

  stags[0].ti_Data = save;

  return URL_SetPrefsA(p,stags);
}

/**************************************************************************/

struct URL_Prefs * LIBFUNC URL_OldGetDefaultPrefs(void)
{
  struct TagItem gtags[] = { { URL_GetPrefs_Mode, URL_GetPrefs_Mode_Default },
                             { TAG_DONE,          TAG_DONE                  } };

  return URL_GetPrefsA(gtags);
}

/**************************************************************************/

ULONG LIBFUNC URL_LaunchPrefsAppA(REG(a0,UNUSED struct TagItem *attrs))
{
  ULONG result = FALSE;
  BPTR in;

  ENTER();

  if((in  = Open("NIL:",MODE_OLDFILE)))
  {
    BPTR out;

    if((out = Open("NIL:",MODE_OLDFILE)))
    {
      TEXT name[256];
      LONG len;

      if((len = GetVar("AppPaths/OpenURL", name+1, sizeof(name)-1, GVF_GLOBAL_ONLY)) <= 0)
      {
        // Ok let's try to be backward compatible
        if(GetVar("OpenURL_Prefs_Path", name, sizeof(name), GVF_GLOBAL_ONLY) <= 0)
        {
          strlcpy(name, "\"Sys:Prefs/Open URL\"", sizeof(name));
        }
      }
      else
      {
        name[0]='\"';
        strcpy(name+1+len,"/Open URL\"");
        name[len+11]='\0';
      }

      #if defined(__amigaos4__)
      #define NP_STACKSIZE 48000
      #else
      #define NP_STACKSIZE 16000
      #endif
      #if !defined(__MORPHOS__)
      #define NP_PPCStackSize TAG_IGNORE
      #endif

      SystemTags(name, SYS_Input,       (IPTR)in,
                       SYS_Output,      (IPTR)out,
                       SYS_Error,       NULL,
                       SYS_Asynch,      TRUE,
                       NP_StackSize,    NP_STACKSIZE,
                       NP_PPCStackSize, 32000,
                       TAG_END);

      result = TRUE;
    }

    if(result == FALSE)
      Close(in);
  }

  RETURN(result);
  return result;
}

/**************************************************************************/

ULONG LIBFUNC URL_OldLaunchPrefsApp(void)
{
  return URL_LaunchPrefsAppA(NULL);
}

/**************************************************************************/

ULONG LIBFUNC URL_GetAttr(REG(d0,ULONG attr),REG(a0,IPTR *storage))
{
  switch (attr)
  {
    case URL_GetAttr_Version:          *storage = LIB_VERSION;    return TRUE;
    case URL_GetAttr_Revision:         *storage = LIB_REVISION;   return TRUE;
    case URL_GetAttr_VerString:        *storage = (IPTR)"$VER: openurl.library " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT;; return TRUE;

    case URL_GetAttr_PrefsVer:         *storage = PREFS_VERSION;  return TRUE;

    case URL_GetAttr_HandlerVersion:   *storage = 0;              return TRUE;
    case URL_GetAttr_HandlerRevision:  *storage = 0;              return TRUE;
    case URL_GetAttr_HandlerVerString: *storage = (IPTR)"";       return TRUE;

    default: return FALSE;
  }
}

/**************************************************************************/

LONG LIBFUNC dispatch(REG(a0, struct RexxMsg *msg), REG(a1, STRPTR *resPtr))
{
    ULONG result = 17;
    STRPTR fun = (STRPTR)msg->rm_Args[0];
    ULONG na = msg->rm_Action & RXARGMASK;
    BOOL res = FALSE;

    ENTER();

    if(stricmp(fun, "OPENURL") == 0)
    {
        if(na >= 1)
        {
            struct TagItem tags[MAXRMARG+1];
            STRPTR url;
            int i, j;

            for(i = na, j = 0, url = NULL; i>0; i--)
            {
                STRPTR arg = (STRPTR)msg->rm_Args[i];
                Tag tag;

                if(arg == NULL || *arg == '\0')
                    continue;

                if(stricmp(arg, "SHOW") == 0 || stricmp(arg, "NOSHOW") == 0)
                    tag = URL_Show;
                else if(stricmp(arg, "TOFRONT") == 0 || stricmp(arg, "NOTOFRONT") == 0)
                    tag  = URL_BringToFront;
                else if(stricmp(arg, "NEWWIN") == 0 || stricmp(arg, "NONEWWIN") == 0)
                    tag = URL_NewWindow;
                else if(stricmp(arg, "LAUNCH") == 0 || stricmp(arg,"NOLAUNCH") == 0)
                    tag  = URL_Launch;
                else
                {
                    url = arg;
                    continue;
                }

                tags[j].ti_Tag  = tag;
                tags[j++].ti_Data = strnicmp(arg, "NO" , 2);
            }

            tags[j].ti_Tag = TAG_END;

            res = (url != NULL && URL_OpenA(url, tags));
        }
    }
    else
    {
        if (stricmp(fun, "OPENURLPREFS") == 0)
        {
            if(na == 0)
              res = URL_LaunchPrefsAppA(NULL);
        }
        else
            result = 1;
    }

    if((*resPtr = (STRPTR)CreateArgstring((STRPTR)(res ? "1" : "0"), 1)) != NULL)
      result = 0;
    else
      result = 3;

    RETURN(result);
    return result;
}

/**************************************************************************/

