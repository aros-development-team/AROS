/*
**  openurl.library-universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
** -Alfonso Ranieri <alforan@tin.it>
** -Stefan Kost <ensonic@sonicpulse.de>
*/


#include "lib.h"
#include <prefs/prefhdr.h>

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

struct URL_Prefs *
copyPrefs(struct URL_Prefs *old)
{
    struct URL_Prefs *new;
    LONG         ver;

    ver = old->up_Version;
    if (ver!=PREFS_VERSION) return FALSE;

    if (new = allocPooled(sizeof(struct URL_Prefs)))
    {
        ULONG res;

        new->up_Version = PREFS_VERSION;
        new->up_Flags   = old->up_Flags;

        NEWLIST(&new->up_BrowserList);
        NEWLIST(&new->up_MailerList);
        NEWLIST(&new->up_FTPList);

        if (res = copyList((struct List *)&new->up_BrowserList,(struct List *)&old->up_BrowserList,sizeof(struct URL_BrowserNode)))
        {
            new->up_DefShow         = old->up_DefShow;
            new->up_DefBringToFront = old->up_DefBringToFront;
            new->up_DefNewWindow    = old->up_DefNewWindow;
            new->up_DefLaunch       = old->up_DefLaunch;

            if (res = copyList((struct List *)&new->up_MailerList,(struct List *)&old->up_MailerList,sizeof(struct URL_MailerNode)))
                res = copyList((struct List *)&new->up_FTPList,(struct List *)&old->up_FTPList,sizeof(struct URL_FTPNode));

            if (res) return new;
        }

        URL_FreePrefsA(new,NULL);
    }

    return NULL;
}

/**************************************************************************/

void
initPrefs(struct URL_Prefs *p)
{
    memset(p,0,sizeof(*p));

    p->up_Version = PREFS_VERSION;

    QUICKNEWLIST(&p->up_BrowserList);
    QUICKNEWLIST(&p->up_MailerList);
    QUICKNEWLIST(&p->up_FTPList);
}

/**************************************************************************/

void
setDefaultPrefs(struct URL_Prefs *p)
{
    struct URL_BrowserNode *bn;
    struct URL_MailerNode  *mn;

    initPrefs(p);

    p->up_Flags = DEF_FLAGS;

    p->up_DefShow         = DEF_DefShow;
    p->up_DefBringToFront = DEF_DefBringToFront;
    p->up_DefNewWindow    = DEF_DefNewWindow;
    p->up_DefLaunch       = DEF_DefLaunch;

    /* Browsers: IBrowse */
    if (!(bn = allocPooled(sizeof(struct URL_BrowserNode)))) return;
    bn->ubn_Flags = UNF_DISABLED;
    strcpy(bn->ubn_Name,"IBrowse");
    strcpy(bn->ubn_Path,"IBrowse \"%u\"");
    strcpy(bn->ubn_Port,"IBROWSE");
    strcpy(bn->ubn_ShowCmd,"SHOW");
    strcpy(bn->ubn_ToFrontCmd,"SCREENTOFRONT");
    strcpy(bn->ubn_OpenURLCmd,"GOTOURL \"%u\"");
    strcpy(bn->ubn_OpenURLWCmd,"NEWWINDOW \"%u\"");
    AddTail(LIST(&p->up_BrowserList),NODE(bn));

    /* Browsers: AWeb */
    if (!(bn = allocPooled(sizeof(struct URL_BrowserNode)))) return;
    bn->ubn_Flags = UNF_DISABLED;
    strcpy(bn->ubn_Name,"AWeb");
    strcpy(bn->ubn_Path,"AWeb \"%u\"");
    strcpy(bn->ubn_Port,"AWEB");
    strcpy(bn->ubn_ShowCmd,"ICONIFY SHOW");
    strcpy(bn->ubn_ToFrontCmd,"SCREENTOFRONT");
    strcpy(bn->ubn_OpenURLCmd,"OPEN \"%u\"");
    strcpy(bn->ubn_OpenURLWCmd,"NEW \"%u\"");
    AddTail(LIST(&p->up_BrowserList),NODE(bn));

    /* Browsers: Voyager */
    if (!(bn = allocPooled(sizeof(struct URL_BrowserNode)))) return;
    strcpy(bn->ubn_Name,"Voyager");
    if (GetVar("Vapor/Voyager_LASTUSEDDIR",bn->ubn_Path,PATH_LEN,GVF_GLOBAL_ONLY)>0) AddPart(bn->ubn_Path,"V \"%u\"",PATH_LEN);
    else
    {
        strcpy(bn->ubn_Path,"V \"%u\"");
        bn->ubn_Flags = UNF_DISABLED;
    }
    strcpy(bn->ubn_Port,"VOYAGER");
    strcpy(bn->ubn_ShowCmd,"SHOW");
    strcpy(bn->ubn_ToFrontCmd,"SCREENTOFRONT");
    strcpy(bn->ubn_OpenURLCmd,"OPENURL \"%u\"");
    strcpy(bn->ubn_OpenURLWCmd,"OPENURL \"%u\" NEWWIN");
    AddTail(LIST(&p->up_BrowserList),NODE(bn));

    /* Mailers: SimpleMail */
    if (!(mn = allocPooled(sizeof(struct URL_MailerNode)))) return;
    mn->umn_Flags = UNF_DISABLED;
    strcpy(mn->umn_Name,"SimpleMail");
    strcpy(mn->umn_Path,"SimpleMail MAILTO=\"%a\" SUBJECT=\"%s\"");
    strcpy(mn->umn_Port,"SIMPLEMAIL");
    strcpy(mn->umn_ShowCmd,"SHOW");
    strcpy(mn->umn_ToFrontCmd,"SCREENTOFRONT");
    strcpy(mn->umn_WriteMailCmd,"MAILWRITE MAILTO=\"%a\" SUBJECT=\"%s\"");
    AddTail(LIST(&p->up_MailerList),NODE(mn));

    /* Mailers: YAM */
    if (!(mn = allocPooled(sizeof(struct URL_MailerNode)))) return;
    mn->umn_Flags = UNF_DISABLED;
    strcpy(mn->umn_Name,"YAM 2.3");
    strcpy(mn->umn_Path,"YAM:YAM MAILTO=\"%a\" SUBJECT=\"%s\" LETTER=\"%f\"");
    strcpy(mn->umn_Port,"YAM");
    strcpy(mn->umn_ShowCmd,"SHOW");
    strcpy(mn->umn_ToFrontCmd,"SCREENTOFRONT");
    strcpy(mn->umn_WriteMailCmd,"MAILWRITE;WRITETO \"%a\";WRITESUBJECT \"%s\";WRITEEDITOR \"CLEAR\";WRITEEDITOR \"TEXT %b\"");
    //strcpy(mn->umn_WriteMailCmd,"MAILWRITE;WRITETO '%a';WRITESUBJECT '%s';WRITELETTER '%f'\"");
    AddTail(LIST(&p->up_MailerList),NODE(mn));

    /* Mailers: MicroDot II */
    if (!(mn = allocPooled(sizeof(struct URL_MailerNode)))) return;
    mn->umn_Flags = UNF_DISABLED;
    strcpy(mn->umn_Name,"MicroDot II");
    if (GetVar("Vapor/MD2_LASTUSEDDIR",mn->umn_Path,PATH_LEN,GVF_GLOBAL_ONLY)<0)
        *mn->umn_Path = 0;
    AddPart(mn->umn_Path,"MicroDot TO=\"%a\" SUBJECT=\"%s\" CONTENTS=\"%f\"",PATH_LEN);
    strcpy(mn->umn_Port,"MD");
    strcpy(mn->umn_ShowCmd,"SHOW");
    strcpy(mn->umn_WriteMailCmd,"NEWMSGWINDOW TO=\"%a\" SUBJECT=\"%s\" CONTENTS=\"%f\"");
    AddTail(LIST(&p->up_MailerList),NODE(mn));

    /* Mailers: lola */
    if (!(mn = allocPooled(sizeof(struct URL_MailerNode)))) return;
    mn->umn_Flags = UNF_DISABLED;
    strcpy(mn->umn_Name,"lola");
    strcpy(mn->umn_Path,"lola TO=\"%a\" SUBJECT=\"%s\" TEXT=\"%b\" CX_POPUP CX_POPKEY=\"control alt l\"");
    strcpy(mn->umn_Port,"LOLA");
    strcpy(mn->umn_ShowCmd,"SHOW");
    strcpy(mn->umn_WriteMailCmd,"FILL TO=\"%a\" SUBJECT=\"%s\" TEXT=\"%b\"");
    AddTail(LIST(&p->up_MailerList),NODE(mn));
}

/**************************************************************************/

ULONG
savePrefs(UBYTE *filename,struct URL_Prefs *p)
{
    struct IFFHandle *iffh;
    ULONG            res = FALSE;

    if (iffh = AllocIFF())
    {
        if (iffh->iff_Stream = Open(filename,MODE_NEWFILE))
        {
            InitIFFasDOS(iffh);

            if (!OpenIFF(iffh,IFFF_WRITE))
            {
                struct PrefHeader      prhd;
                struct URL_BrowserNode *bn;
                struct URL_MailerNode  *mn;
                struct URL_FTPNode     *fn;

                if (PushChunk(iffh,ID_PREF,ID_FORM,IFFSIZE_UNKNOWN)) goto fail;
                if (PushChunk(iffh,ID_PREF,ID_PRHD,sizeof(struct PrefHeader))) goto fail;

                prhd.ph_Version = p->up_Version;
                prhd.ph_Type    = 0;
                prhd.ph_Flags   = 0;

                if (WriteChunkBytes(iffh,&prhd,sizeof(struct PrefHeader))!=sizeof(struct PrefHeader))
                    goto fail;

                if (PopChunk(iffh)) goto fail;

                /* write browser nodes */
                for (bn = (struct URL_BrowserNode *)p->up_BrowserList.mlh_Head;
                     bn->ubn_Node.mln_Succ;
                     bn = (struct URL_BrowserNode *)bn->ubn_Node.mln_Succ)
                {
                    if (PushChunk(iffh,ID_PREF,ID_BRWS,BRWS_SIZE)) goto fail;
                    if (WriteChunkBytes(iffh,&bn->ubn_Flags,BRWS_SIZE)!=BRWS_SIZE) goto fail;
                    if (PopChunk(iffh)) goto fail;
                }

                /* write mailer nodes */
                for (mn = (struct URL_MailerNode *)p->up_MailerList.mlh_Head;
                     mn->umn_Node.mln_Succ;
                     mn = (struct URL_MailerNode *)mn->umn_Node.mln_Succ)
                {
                    if (PushChunk(iffh,ID_PREF,ID_MLRS,MLRS_SIZE)) goto fail;
                    if (WriteChunkBytes(iffh,&mn->umn_Flags,MLRS_SIZE)!=MLRS_SIZE) goto fail;
                    if (PopChunk(iffh)) goto fail;
                }

                /* write ftp nodes */
                for (fn = (struct URL_FTPNode *)p->up_FTPList.mlh_Head;
                     fn->ufn_Node.mln_Succ;
                     fn = (struct URL_FTPNode *)fn->ufn_Node.mln_Succ)
                {
                    if (PushChunk(iffh,ID_PREF,ID_FTPS,FTPS_SIZE)) goto fail;
                    if (WriteChunkBytes(iffh,&fn->ufn_Flags,FTPS_SIZE)!=FTPS_SIZE) goto fail;
                    if (PopChunk(iffh)) goto fail;
                }

                /* write flags */
                if (PushChunk(iffh,ID_PREF,ID_FLGS,FLGS_SIZE)) goto fail;
                if (WriteChunkBytes(iffh,&p->up_Flags,FLGS_SIZE)!=FLGS_SIZE) goto fail;
                if (PopChunk(iffh)) goto fail;

                /* write defaults */
                if (PushChunk(iffh,ID_PREF,ID_DEFS,DEFS_SIZE)) goto fail;
                if (WriteChunkBytes(iffh,&p->up_DefShow,DEFS_SIZE)!=DEFS_SIZE) goto fail;
                if (PopChunk(iffh)) goto fail;

                /* pop the IFF PREF FORM chunk */
                if (PopChunk(iffh)) goto fail;

                res = TRUE;

            fail:
                CloseIFF(iffh);
            }

            Close(iffh->iff_Stream);
        }

        FreeIFF(iffh);
    }

    if (!res) DeleteFile(filename);

    return res;
}

/**************************************************************************/

ULONG
loadPrefs(struct URL_Prefs *p,ULONG mode)
{
    struct IFFHandle *iffh;
    ULONG            res = FALSE;

    initPrefs(p);

    if (iffh = AllocIFF())
    {
        UBYTE *fileName;
        BPTR  file;

        fileName = (mode==LOADPREFS_ENV) ? DEF_ENV : DEF_ENVARC;

        if (!(file = Open(fileName,MODE_OLDFILE)))
            if (mode==LOADPREFS_ENV) file = Open(DEF_ENVARC,MODE_OLDFILE);

        if (iffh->iff_Stream = file)
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
                    ULONG error;

                    error = ParseIFF(iffh,IFFPARSE_SCAN);
                    if (error==IFFERR_EOF) break;
                    else if (error) goto fail;

                    if (!(cn = CurrentChunk(iffh))) goto fail;

                    if (cn->cn_Type!=ID_PREF) continue;

                    if ((cn->cn_ID==ID_BRWS) && (cn->cn_Size==BRWS_SIZE))
                    {
                        struct URL_BrowserNode *bn;

                        if (!(bn = allocPooled(sizeof(struct URL_BrowserNode))))
                            goto fail;

                        if (ReadChunkBytes(iffh,&bn->ubn_Flags,cn->cn_Size)!=cn->cn_Size)
                        {
                            freePooled(bn,sizeof(struct URL_BrowserNode));
                            goto fail;
                        }

                        AddTail(LIST(&p->up_BrowserList),NODE(bn));

                        continue;
                    }

                    if ((cn->cn_ID==ID_MLRS) && (cn->cn_Size==MLRS_SIZE))
                    {
                        struct URL_MailerNode *mn;

                        if (!(mn = allocPooled(sizeof(struct URL_MailerNode))))
                            goto fail;

                        if (ReadChunkBytes(iffh,&mn->umn_Flags,cn->cn_Size)!=cn->cn_Size)
                        {
                            freePooled(mn,sizeof(struct URL_MailerNode));
                            goto fail;
                        }

                        AddTail(LIST(&p->up_MailerList),NODE(mn));

                        continue;
                    }

                    if ((cn->cn_ID==ID_FTPS) && (cn->cn_Size==FTPS_SIZE))
                    {
                        struct URL_FTPNode *fn;

                        if (!(fn = allocPooled(sizeof(struct URL_FTPNode))))
                            goto fail;

                        if (ReadChunkBytes(iffh,&fn->ufn_Flags,cn->cn_Size)!=cn->cn_Size)
                        {
                            freePooled(fn,sizeof(struct URL_FTPNode));
                            goto fail;
                        }

                        AddTail(LIST(&p->up_FTPList),NODE(fn));

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

                p->up_Flags &= ~UPF_ISDEFAULTS;

                res = TRUE;

            fail:
                CloseIFF(iffh);
            }

            Close(file);
        }

        FreeIFF(iffh);
    }

    return res;
}

/**************************************************************************/

struct URL_Prefs *
loadPrefsNotFail(void)
{
    struct URL_Prefs *p;

    if (p = allocPooled(sizeof(struct URL_Prefs)))
    {
        if (!loadPrefs(p,LOADPREFS_ENV))
            setDefaultPrefs(p);
    }

    return p;
}

/**************************************************************************/

