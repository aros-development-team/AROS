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

#include <prefs/prefhdr.h>

#include "debug.h"

/**************************************************************************/

#define ID_BRWS MAKE_ID('B','R','W','S')
#define ID_MLRS MAKE_ID('M','L','R','S')
#define ID_FTPS MAKE_ID('F','T','P','S')
#define ID_FLGS MAKE_ID('F','L','G','S')
#define ID_DEFS MAKE_ID('D','E','F','S')

#define BRWS_SIZE (sizeof(struct URL_BrowserNode)-sizeof(struct MinNode))
#define MLRS_SIZE (sizeof(struct URL_MailerNode)-sizeof(struct MinNode))
#define FTPS_SIZE (sizeof(struct URL_FTPNode)-sizeof(struct MinNode))
#define FLGS_SIZE (sizeof(ULONG))
#define DEFS_SIZE (4 * sizeof(ULONG))

/**************************************************************************/

struct URL_Prefs *copyPrefs(struct URL_Prefs *old)
{
  struct URL_Prefs *new = NULL;

  ENTER();

  if(old->up_Version == PREFS_VERSION)
  {
    if((new = allocArbitrateVecPooled(sizeof(struct URL_Prefs))) != NULL)
    {
      ULONG res;

      new->up_Version = PREFS_VERSION;
      new->up_Flags   = old->up_Flags;

      NewList((struct List *)&new->up_BrowserList);
      NewList((struct List *)&new->up_MailerList);
      NewList((struct List *)&new->up_FTPList);

      if((res = copyList((struct List *)&new->up_BrowserList,(struct List *)&old->up_BrowserList,sizeof(struct URL_BrowserNode))))
      {
        new->up_DefShow         = old->up_DefShow;
        new->up_DefBringToFront = old->up_DefBringToFront;
        new->up_DefNewWindow    = old->up_DefNewWindow;
        new->up_DefLaunch       = old->up_DefLaunch;

        if((res = copyList((struct List *)&new->up_MailerList,(struct List *)&old->up_MailerList,sizeof(struct URL_MailerNode))))
          res = copyList((struct List *)&new->up_FTPList,(struct List *)&old->up_FTPList,sizeof(struct URL_FTPNode));
      }

      if(res == FALSE)
      {
        URL_FreePrefsA(new, NULL);
        new = NULL;
      }
    }
  }

  return(new);
  return new;
}

/**************************************************************************/

void initPrefs(struct URL_Prefs *p)
{
  ENTER();

  memset(p,0,sizeof(*p));

  p->up_Version = PREFS_VERSION;

  NewList((struct List *)&p->up_BrowserList);
  NewList((struct List *)&p->up_MailerList);
  NewList((struct List *)&p->up_FTPList);

  LEAVE();
}

/**************************************************************************/

void setDefaultPrefs(struct URL_Prefs *p)
{
  struct URL_BrowserNode *bn;
  struct URL_MailerNode  *mn;
  struct URL_FTPNode     *fn;

  ENTER();

  initPrefs(p);

  p->up_Flags = DEF_FLAGS;

  p->up_DefShow         = DEF_DefShow;
  p->up_DefBringToFront = DEF_DefBringToFront;
  p->up_DefNewWindow    = DEF_DefNewWindow;
  p->up_DefLaunch       = DEF_DefLaunch;

  /* Browsers: OWB */
  if(!(bn = allocArbitrateVecPooled(sizeof(struct URL_BrowserNode))))
  {
    LEAVE();
    return;
  }

  bn->ubn_Flags = UNF_DISABLED;
  strlcpy(bn->ubn_Name,"OWB", sizeof(bn->ubn_Name));
  strlcpy(bn->ubn_Path,"OWB \"%u\"", sizeof(bn->ubn_Path));
  strlcpy(bn->ubn_Port,"OWB", sizeof(bn->ubn_Port));
  strlcpy(bn->ubn_ShowCmd,"", sizeof(bn->ubn_ShowCmd));
  strlcpy(bn->ubn_ToFrontCmd,"SCREENTOFRONT", sizeof(bn->ubn_ToFrontCmd));
  strlcpy(bn->ubn_OpenURLCmd,"OPENURL \"%u\"", sizeof(bn->ubn_OpenURLCmd));
  strlcpy(bn->ubn_OpenURLWCmd,"OPENURL \"%u\"", sizeof(bn->ubn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_BrowserList), (struct Node *)(bn));

  /* Browsers: NetSurf */
  if(!(bn = allocArbitrateVecPooled(sizeof(struct URL_BrowserNode))))
  {
    LEAVE();
    return;
  }

  bn->ubn_Flags = UNF_DISABLED;
  strlcpy(bn->ubn_Name, "NetSurf", sizeof(bn->ubn_Name));
  strlcpy(bn->ubn_Path, "NetSurf \"%u\"", sizeof(bn->ubn_Path));
  strlcpy(bn->ubn_Port, "NETSURF", sizeof(bn->ubn_Port));
  strlcpy(bn->ubn_ShowCmd, "", sizeof(bn->ubn_ShowCmd));
  strlcpy(bn->ubn_ToFrontCmd, "TOFRONT", sizeof(bn->ubn_ToFrontCmd));
  strlcpy(bn->ubn_OpenURLCmd, "OPEN \"%u\"", sizeof(bn->ubn_OpenURLCmd));
  strlcpy(bn->ubn_OpenURLWCmd, "OPEN \"%u\" NEW", sizeof(bn->ubn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_BrowserList), (struct Node *)(bn));

  /* Browsers: IBrowse */
  if(!(bn = allocArbitrateVecPooled(sizeof(struct URL_BrowserNode))))
  {
    LEAVE();
    return;
  }

  bn->ubn_Flags = UNF_DISABLED;
  strlcpy(bn->ubn_Name,"IBrowse", sizeof(bn->ubn_Name));
  strlcpy(bn->ubn_Path,"IBrowse \"%u\"", sizeof(bn->ubn_Path));
  strlcpy(bn->ubn_Port,"IBROWSE", sizeof(bn->ubn_Port));
  strlcpy(bn->ubn_ShowCmd,"SHOW", sizeof(bn->ubn_ShowCmd));
  strlcpy(bn->ubn_ToFrontCmd,"SCREENTOFRONT", sizeof(bn->ubn_ToFrontCmd));
  strlcpy(bn->ubn_OpenURLCmd,"GOTOURL \"%u\"", sizeof(bn->ubn_OpenURLCmd));
  strlcpy(bn->ubn_OpenURLWCmd,"NEWWINDOW \"%u\"", sizeof(bn->ubn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_BrowserList), (struct Node *)(bn));

  /* Browsers: AWeb */
  if(!(bn = allocArbitrateVecPooled(sizeof(struct URL_BrowserNode))))
  {
    LEAVE();
    return;
  }

  bn->ubn_Flags = UNF_DISABLED;
  strlcpy(bn->ubn_Name,"AWeb", sizeof(bn->ubn_Name));
  strlcpy(bn->ubn_Path,"AWeb \"%u\"", sizeof(bn->ubn_Path));
  strlcpy(bn->ubn_Port,"AWEB", sizeof(bn->ubn_Port));
  strlcpy(bn->ubn_ShowCmd,"ICONIFY SHOW", sizeof(bn->ubn_ShowCmd));
  strlcpy(bn->ubn_ToFrontCmd,"SCREENTOFRONT", sizeof(bn->ubn_ToFrontCmd));
  strlcpy(bn->ubn_OpenURLCmd,"OPEN \"%u\"", sizeof(bn->ubn_OpenURLCmd));
  strlcpy(bn->ubn_OpenURLWCmd,"NEW \"%u\"", sizeof(bn->ubn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_BrowserList),(struct Node *)(bn));

  /* Browsers: Voyager */
  if(!(bn = allocArbitrateVecPooled(sizeof(struct URL_BrowserNode))))
  {
    LEAVE();
    return;
  }

  bn->ubn_Flags = UNF_DISABLED;
  strcpy(bn->ubn_Name,"Voyager");
  if(GetVar("Vapor/Voyager_LASTUSEDDIR",bn->ubn_Path,sizeof(bn->ubn_Path),GVF_GLOBAL_ONLY)>0)
    AddPart(bn->ubn_Path,"V \"%u\"", sizeof(bn->ubn_Path));
  else
    strlcpy(bn->ubn_Path,"V \"%u\"", sizeof(bn->ubn_Path));
  strlcpy(bn->ubn_Port,"VOYAGER", sizeof(bn->ubn_Port));
  strlcpy(bn->ubn_ShowCmd,"SHOW", sizeof(bn->ubn_ShowCmd));
  strlcpy(bn->ubn_ToFrontCmd,"SCREENTOFRONT", sizeof(bn->ubn_ToFrontCmd));
  strlcpy(bn->ubn_OpenURLCmd,"OPENURL \"%u\"", sizeof(bn->ubn_OpenURLCmd));
  strlcpy(bn->ubn_OpenURLWCmd,"OPENURL \"%u\" NEWWIN", sizeof(bn->ubn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_BrowserList),(struct Node *)(bn));

  /* Mailers: YAM */
  if(!(mn = allocArbitrateVecPooled(sizeof(struct URL_MailerNode))))
  {
    LEAVE();
    return;
  }

  mn->umn_Flags = UNF_DISABLED;
  strlcpy(mn->umn_Name,"YAM", sizeof(mn->umn_Name));
  strlcpy(mn->umn_Path,"YAM MAILTO=\"%a\" SUBJECT=\"%s\" LETTER=\"%f\"", sizeof(mn->umn_Path));
  strlcpy(mn->umn_Port,"YAM", sizeof(mn->umn_Port));
  strlcpy(mn->umn_ShowCmd,"SHOW", sizeof(mn->umn_ShowCmd));
  strlcpy(mn->umn_ToFrontCmd,"SCREENTOFRONT", sizeof(mn->umn_ToFrontCmd));
  strlcpy(mn->umn_WriteMailCmd,"MAILWRITE;WRITETO \"%a\";WRITESUBJECT \"%s\";WRITEEDITOR \"CLEAR\";WRITEEDITOR \"TEXT %b\"", sizeof(mn->umn_WriteMailCmd));
  AddTail((struct List *)(&p->up_MailerList),(struct Node *)(mn));

  /* Mailers: SimpleMail */
  if(!(mn = allocArbitrateVecPooled(sizeof(struct URL_MailerNode))))
  {
    LEAVE();
    return;
  }

  mn->umn_Flags = UNF_DISABLED;
  strlcpy(mn->umn_Name,"SimpleMail", sizeof(mn->umn_Name));
  strlcpy(mn->umn_Path,"SimpleMail MAILTO=\"%a\" SUBJECT=\"%s\"", sizeof(mn->umn_Path));
  strlcpy(mn->umn_Port,"SIMPLEMAIL", sizeof(mn->umn_Port));
  strlcpy(mn->umn_ShowCmd,"SHOW", sizeof(mn->umn_ShowCmd));
  strlcpy(mn->umn_ToFrontCmd,"SCREENTOFRONT", sizeof(mn->umn_ToFrontCmd));
  strlcpy(mn->umn_WriteMailCmd,"MAILWRITE MAILTO=\"%a\" SUBJECT=\"%s\"", sizeof(mn->umn_WriteMailCmd));
  AddTail((struct List *)(&p->up_MailerList),(struct Node *)(mn));

  /* Mailers: MicroDot II */
  if(!(mn = allocArbitrateVecPooled(sizeof(struct URL_MailerNode))))
  {
    LEAVE();
    return;
  }

  mn->umn_Flags = UNF_DISABLED;
  strlcpy(mn->umn_Name,"MicroDot II", sizeof(mn->umn_Name));
  if(GetVar("Vapor/MD2_LASTUSEDDIR",mn->umn_Path, sizeof(mn->umn_Path),GVF_GLOBAL_ONLY)<0)
    *mn->umn_Path = 0;
  AddPart(mn->umn_Path,"MicroDot TO=\"%a\" SUBJECT=\"%s\" CONTENTS=\"%f\"", sizeof(mn->umn_Path));
  strlcpy(mn->umn_Port,"MD", sizeof(mn->umn_Port));
  strlcpy(mn->umn_ShowCmd,"SHOW", sizeof(mn->umn_ShowCmd));
  strlcpy(mn->umn_WriteMailCmd,"NEWMSGWINDOW TO=\"%a\" SUBJECT=\"%s\" CONTENTS=\"%f\"", sizeof(mn->umn_WriteMailCmd));
  AddTail((struct List *)(&p->up_MailerList),(struct Node *)(mn));

  /* Mailers: lola */
  if(!(mn = allocArbitrateVecPooled(sizeof(struct URL_MailerNode))))
  {
    LEAVE();
    return;
  }

  mn->umn_Flags = UNF_DISABLED;
  strlcpy(mn->umn_Name,"lola", sizeof(mn->umn_Name));
  strlcpy(mn->umn_Path,"lola TO=\"%a\" SUBJECT=\"%s\" TEXT=\"%b\" CX_POPUP CX_POPKEY=\"control alt l\"", sizeof(mn->umn_Path));
  strlcpy(mn->umn_Port,"LOLA", sizeof(mn->umn_Port));
  strlcpy(mn->umn_ShowCmd,"SHOW", sizeof(mn->umn_ShowCmd));
  strlcpy(mn->umn_WriteMailCmd,"FILL TO=\"%a\" SUBJECT=\"%s\" TEXT=\"%b\"", sizeof(mn->umn_WriteMailCmd));
  AddTail((struct List *)(&p->up_MailerList),(struct Node *)(mn));

  /* FTP: AmiFTP */
  if(!(fn = allocArbitrateVecPooled(sizeof(struct URL_FTPNode))))
  {
    LEAVE();
    return;
  }

  fn->ufn_Flags = UNF_DISABLED;
  strlcpy(fn->ufn_Name,"AmiFTP", sizeof(fn->ufn_Name));
  strlcpy(fn->ufn_Path,"AmiFTP \"%a\"", sizeof(fn->ufn_Path));
  strlcpy(fn->ufn_Port,"AMIFTP", sizeof(fn->ufn_Port));
  strlcpy(fn->ufn_ShowCmd,"", sizeof(fn->ufn_ShowCmd));
  strlcpy(fn->ufn_ToFrontCmd,"", sizeof(fn->ufn_ToFrontCmd));
  strlcpy(fn->ufn_OpenURLCmd,"", sizeof(fn->ufn_OpenURLCmd));
  strlcpy(fn->ufn_OpenURLWCmd,"", sizeof(fn->ufn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_FTPList),(struct Node *)(fn));

  /* FTP: Pete's FTP */
  if(!(fn = allocArbitrateVecPooled(sizeof(struct URL_FTPNode))))
  {
    LEAVE();
    return;
  }

  fn->ufn_Flags = UNF_DISABLED;
  strlcpy(fn->ufn_Name,"Pete's FTP", sizeof(fn->ufn_Name));
  strlcpy(fn->ufn_Path,"pftp \"%a\"", sizeof(fn->ufn_Path));
  strlcpy(fn->ufn_Port,"", sizeof(fn->ufn_Port));
  strlcpy(fn->ufn_ShowCmd,"", sizeof(fn->ufn_ShowCmd));
  strlcpy(fn->ufn_ToFrontCmd,"", sizeof(fn->ufn_ToFrontCmd));
  strlcpy(fn->ufn_OpenURLCmd,"", sizeof(fn->ufn_OpenURLCmd));
  strlcpy(fn->ufn_OpenURLWCmd,"", sizeof(fn->ufn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_FTPList),(struct Node *)(fn));

  /* FTP: AmiTradeCenter */
  if(!(fn = allocArbitrateVecPooled(sizeof(struct URL_FTPNode))))
  {
    LEAVE();
    return;
  }

  fn->ufn_Flags = UNF_DISABLED;
  strlcpy(fn->ufn_Name,"AmiTradeCenter", sizeof(fn->ufn_Name));
  strlcpy(fn->ufn_Path,"AmiTradeCenter \"%a\"", sizeof(fn->ufn_Path));
  strlcpy(fn->ufn_Port,"ATC_MAIN", sizeof(fn->ufn_Port));
  strlcpy(fn->ufn_ShowCmd,"", sizeof(fn->ufn_ShowCmd));
  strlcpy(fn->ufn_ToFrontCmd,"", sizeof(fn->ufn_ToFrontCmd));
  strlcpy(fn->ufn_OpenURLCmd,"", sizeof(fn->ufn_OpenURLCmd));
  strlcpy(fn->ufn_OpenURLWCmd,"", sizeof(fn->ufn_OpenURLWCmd));
  AddTail((struct List *)(&p->up_FTPList),(struct Node *)(fn));

  LEAVE();
}

/**************************************************************************/

BOOL savePrefs(CONST_STRPTR filename, struct URL_Prefs *p)
{
  struct IFFHandle *iffh;
  BOOL res = FALSE;

  ENTER();

  if((iffh = AllocIFF()) != NULL)
  {
    if((iffh->iff_Stream = (IPTR)Open(filename, MODE_NEWFILE)) != 0)
    {
      InitIFFasDOS(iffh);

      if(OpenIFF(iffh, IFFF_WRITE) == 0)
      {
        struct PrefHeader prhd;
        struct URL_BrowserNode *bn;
        struct URL_MailerNode *mn;
        struct URL_FTPNode *fn;

        D(DBF_ALWAYS, "saving prefs to '%s'", filename);

        if(PushChunk(iffh, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN) != 0)
          goto fail;
        if(PushChunk(iffh, ID_PREF, ID_PRHD, sizeof(struct PrefHeader)) != 0)
          goto fail;

        prhd.ph_Version = p->up_Version;
        prhd.ph_Type    = 0;
        prhd.ph_Flags   = 0;

        if(WriteChunkBytes(iffh, &prhd, sizeof(struct PrefHeader)) != sizeof(struct PrefHeader))
          goto fail;

        if(PopChunk(iffh) != 0)
          goto fail;

        /* write browser nodes */
        D(DBF_ALWAYS, "saving browsers");
        for(bn = (struct URL_BrowserNode *)p->up_BrowserList.mlh_Head;
            bn->ubn_Node.mln_Succ;
            bn = (struct URL_BrowserNode *)bn->ubn_Node.mln_Succ)
        {
            // mask out possibly invalid flags
            bn->ubn_Flags &= UNF_VALID_MASK;

            if(PushChunk(iffh, ID_PREF, ID_BRWS, BRWS_SIZE) != 0)
              goto fail;
            if(WriteChunkBytes(iffh, &bn->ubn_Flags, BRWS_SIZE) != BRWS_SIZE)
              goto fail;
            if(PopChunk(iffh) != 0)
              goto fail;
        }

        /* write mailer nodes */
        D(DBF_ALWAYS, "saving mailers");
        for(mn = (struct URL_MailerNode *)p->up_MailerList.mlh_Head;
            mn->umn_Node.mln_Succ;
            mn = (struct URL_MailerNode *)mn->umn_Node.mln_Succ)
        {
            // mask out possibly invalid flags
            mn->umn_Flags &= UNF_VALID_MASK;

            if(PushChunk(iffh, ID_PREF, ID_MLRS, MLRS_SIZE) != 0)
              goto fail;
            if(WriteChunkBytes(iffh, &mn->umn_Flags, MLRS_SIZE) != MLRS_SIZE)
              goto fail;
            if(PopChunk(iffh) != 0)
              goto fail;
        }

        /* write ftp nodes */
        D(DBF_ALWAYS, "saving ftps");
        for(fn = (struct URL_FTPNode *)p->up_FTPList.mlh_Head;
            fn->ufn_Node.mln_Succ;
            fn = (struct URL_FTPNode *)fn->ufn_Node.mln_Succ)
        {
            // mask out possibly invalid flags
            fn->ufn_Flags &= UNF_VALID_MASK;

            if(PushChunk(iffh, ID_PREF, ID_FTPS, FTPS_SIZE) != 0)
              goto fail;
            if(WriteChunkBytes(iffh, &fn->ufn_Flags, FTPS_SIZE) != FTPS_SIZE)
              goto fail;
            if(PopChunk(iffh) != 0)
              goto fail;
        }

        /* write flags */
        D(DBF_ALWAYS, "saving flags");
        // mask out possibly invalid flags
        p->up_Flags &= UPF_VALID_MASK;

        if(PushChunk(iffh, ID_PREF, ID_FLGS, FLGS_SIZE) != 0)
          goto fail;
        if(WriteChunkBytes(iffh, &p->up_Flags, FLGS_SIZE) != FLGS_SIZE)
          goto fail;
        if(PopChunk(iffh) != 0)
          goto fail;

        /* write defaults */
        D(DBF_ALWAYS, "saving defaults");
        if(PushChunk(iffh, ID_PREF, ID_DEFS, DEFS_SIZE) != 0)
          goto fail;
        if(WriteChunkBytes(iffh, &p->up_DefShow, DEFS_SIZE) != DEFS_SIZE)
          goto fail;
        if(PopChunk(iffh) != 0)
          goto fail;

        /* pop the IFF PREF FORM chunk */
        if(PopChunk(iffh) != 0)
          goto fail;

        res = TRUE;

      fail:
        CloseIFF(iffh);
      }

      Close((BPTR)iffh->iff_Stream);
    }

    FreeIFF(iffh);
  }

  if(res == FALSE)
    DeleteFile(filename);

  RETURN(res);
  return res;
}

/**************************************************************************/

BOOL loadPrefs(struct URL_Prefs *p,ULONG mode)
{
    struct IFFHandle *iffh;
    BOOL res = FALSE;

    ENTER();

    initPrefs(p);

    if((iffh = AllocIFF()) != NULL)
    {
        CONST_STRPTR fileName;
        BPTR  file;

        fileName = (mode==LOADPREFS_ENV) ? DEF_ENV : DEF_ENVARC;

        if (!(file = Open(fileName,MODE_OLDFILE)))
            if (mode==LOADPREFS_ENV) file = Open(DEF_ENVARC,MODE_OLDFILE);

        if((iffh->iff_Stream = (IPTR)file) != 0)
        {
            InitIFFasDOS(iffh);

            if (!OpenIFF(iffh,IFFF_READ))
            {
                struct PrefHeader  prhd;
                struct ContextNode *cn;

                if (StopChunk(iffh,ID_PREF,ID_PRHD)) goto fail;
                if (StopChunk(iffh,ID_PREF,ID_DEFS)) goto fail;
                if (StopChunk(iffh,ID_PREF,ID_FLGS)) goto fail;
                if (StopChunk(iffh,ID_PREF,ID_FTPS)) goto fail;
                if (StopChunk(iffh,ID_PREF,ID_MLRS)) goto fail;
                if (StopChunk(iffh,ID_PREF,ID_BRWS)) goto fail;

                if (ParseIFF(iffh,IFFPARSE_SCAN)) goto fail;

                if (!(cn = CurrentChunk(iffh))) goto fail;

                if ((cn->cn_Type!=ID_PREF) || (cn->cn_ID!=ID_PRHD) ||
                    (cn->cn_Size!=sizeof(struct PrefHeader))) goto fail;

                if (ReadChunkBytes(iffh,&prhd,cn->cn_Size)!=cn->cn_Size) goto fail;
                if (prhd.ph_Version>PREFS_VERSION) goto fail;

                for (;;)
                {
                    LONG error;

                    error = ParseIFF(iffh,IFFPARSE_SCAN);
                    if (error==IFFERR_EOF) break;
                    else if (error) goto fail;

                    if (!(cn = CurrentChunk(iffh))) goto fail;

                    if (cn->cn_Type!=ID_PREF) continue;

                    if ((cn->cn_ID==ID_BRWS) && (cn->cn_Size==BRWS_SIZE))
                    {
                        struct URL_BrowserNode *bn;

                        if(!(bn = allocArbitrateVecPooled(sizeof(struct URL_BrowserNode))))
                          goto fail;

                        if (ReadChunkBytes(iffh,&bn->ubn_Flags,cn->cn_Size)!=cn->cn_Size)
                        {
                          freeArbitrateVecPooled(bn);
                          goto fail;
                        }

                        // mask out possibly invalid flags
                        bn->ubn_Flags &= UNF_VALID_MASK;

                        AddTail((struct List *)(&p->up_BrowserList),(struct Node *)(bn));

                        continue;
                    }

                    if ((cn->cn_ID==ID_MLRS) && (cn->cn_Size==MLRS_SIZE))
                    {
                        struct URL_MailerNode *mn;

                        if(!(mn = allocArbitrateVecPooled(sizeof(struct URL_MailerNode))))
                          goto fail;

                        if (ReadChunkBytes(iffh,&mn->umn_Flags,cn->cn_Size)!=cn->cn_Size)
                        {
                          freeArbitrateVecPooled(mn);
                          goto fail;
                        }

                        // mask out possibly invalid flags
                        mn->umn_Flags &= UNF_VALID_MASK;

                        AddTail((struct List *)(&p->up_MailerList),(struct Node *)(mn));

                        continue;
                    }

                    if ((cn->cn_ID==ID_FTPS) && (cn->cn_Size==FTPS_SIZE))
                    {
                        struct URL_FTPNode *fn;

                        if(!(fn = allocArbitrateVecPooled(sizeof(struct URL_FTPNode))))
                          goto fail;

                        if (ReadChunkBytes(iffh,&fn->ufn_Flags,cn->cn_Size)!=cn->cn_Size)
                        {
                          freeArbitrateVecPooled(fn);
                          goto fail;
                        }

                        // mask out possibly invalid flags
                        fn->ufn_Flags &= UNF_VALID_MASK;

                        AddTail((struct List *)(&p->up_FTPList),(struct Node *)(fn));

                        continue;
                    }

                    if ((cn->cn_ID==ID_FLGS) && (cn->cn_Size==FLGS_SIZE))
                    {
                        if (ReadChunkBytes(iffh,&p->up_Flags,cn->cn_Size)!=cn->cn_Size)
                            goto fail;

                        continue;
                    }

                    if ((cn->cn_ID==ID_DEFS) && (cn->cn_Size==DEFS_SIZE))
                    {
                        if (ReadChunkBytes(iffh,&p->up_DefShow,cn->cn_Size)!=cn->cn_Size)
                            goto fail;

                        continue;
                    }
                }

                // the loaded prefs are not the default ones
                CLEAR_FLAG(p->up_Flags, UPF_ISDEFAULTS);

                // mask out possibly invalid flags
                p->up_Flags &= UPF_VALID_MASK;

                res = TRUE;

            fail:
                CloseIFF(iffh);
            }

            Close(file);
        }

        FreeIFF(iffh);
    }

    RETURN(res);
    return res;
}

/**************************************************************************/

struct URL_Prefs *loadPrefsNotFail(void)
{
  struct URL_Prefs *p;

  ENTER();

  if((p = allocArbitrateVecPooled(sizeof(*p))) != NULL)
  {
    if(loadPrefs(p, LOADPREFS_ENV) == FALSE)
      setDefaultPrefs(p);
  }

  RETURN(p);
  return p;
}

/**************************************************************************/

