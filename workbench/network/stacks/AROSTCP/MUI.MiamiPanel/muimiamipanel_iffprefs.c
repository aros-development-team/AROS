
#include <proto/dos.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_iffprefs.h"
#include "muimiamipanel_misc.h"

ULONG
saveIFFPrefs(UBYTE *file,struct MPS_Prefs *prefs, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register struct IFFHandle *iffh;
    register ULONG             res = FALSE;

    if (iffh = AllocIFF())
    {
        if (iffh->iff_Stream = Open(file,MODE_NEWFILE))
        {
            InitIFFasDOS(iffh);

            if (!OpenIFF(iffh,IFFF_WRITE))
            {
                struct PrefHeader      prhd;
                register struct ifnode *ifnode;

                if (PushChunk(iffh,ID_PREF,ID_FORM,IFFSIZE_UNKNOWN)) goto fail;
                if (PushChunk(iffh,ID_PREF,ID_PRHD,sizeof(struct PrefHeader))) goto fail;

                prhd.ph_Version = MPV_Prefs_Version;
                prhd.ph_Type    = 0;
                prhd.ph_Flags   = 0;

                if (WriteChunkBytes(iffh,&prhd,sizeof(struct PrefHeader))!=sizeof(struct PrefHeader))
                    goto fail;

                if (PopChunk(iffh)) goto fail;

                if (PushChunk(iffh,ID_PREF,ID_Prefs,SIZE_Prefs)) goto fail;
                if (WriteChunkBytes(iffh,prefs,SIZE_Prefs)!=SIZE_Prefs) goto fail;
                if (PopChunk(iffh)) goto fail;

                for (ifnode = (struct ifnode *)prefs->iflist.mlh_Head; ifnode->link.mln_Succ; ifnode = (struct ifnode *)ifnode->link.mln_Succ)
                {
                    if (PushChunk(iffh,ID_PREF,ID_IFNode,SIZE_IFNode)) goto fail;
                    if (WriteChunkBytes(iffh,&ifnode->name,SIZE_IFNode)!=SIZE_IFNode) goto fail;
                    if (PopChunk(iffh)) goto fail;
                }

                res = TRUE;

                fail: CloseIFF(iffh);
            }

            Close(iffh->iff_Stream);
        }

        FreeIFF(iffh);
    }

    if (!res) DeleteFile(file);

    return res;
}

/**************************************************************************/

ULONG
loadIFFPrefs(ULONG where,struct MPS_Prefs *prefs, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register struct IFFHandle *iffh;
    register ULONG            res = FALSE;

    NEWLIST(&prefs->iflist);

    if (iffh = AllocIFF())
    {
        register BPTR file;

        if (where & MPV_LoadPrefs_Env)
        {
            file = Open(DEF_ENVFILE,MODE_OLDFILE);
            if (!file && (where & MPV_LoadPrefs_FallBack))
                file = Open(DEF_ENVARCFILE,MODE_OLDFILE);
        }
        else file = Open(DEF_ENVARCFILE,MODE_OLDFILE);

        if (file)
        {
            iffh->iff_Stream = file;

            InitIFFasDOS(iffh);

            if (!OpenIFF(iffh,IFFF_READ))
            {
                struct PrefHeader           prhd;
                register struct ContextNode *cn;

                if (StopChunk(iffh,ID_PREF,ID_PRHD)) goto fail;
                if (StopChunk(iffh,ID_PREF,ID_Prefs)) goto fail;
                if (StopChunk(iffh,ID_PREF,ID_IFNode)) goto fail;

                if (ParseIFF(iffh,IFFPARSE_SCAN)) goto fail;

                if (!(cn = CurrentChunk(iffh))) goto fail;

                if ((cn->cn_Type!=ID_PREF) || (cn->cn_ID!=ID_PRHD) ||
                    (cn->cn_Size!=sizeof(struct PrefHeader))) goto fail;

                if (ReadChunkBytes(iffh,&prhd,cn->cn_Size)!=cn->cn_Size) goto fail;

                if (prhd.ph_Version>MPV_Prefs_Version) goto fail;

                for (;;)
                {
                    register ULONG error;

                    error = ParseIFF(iffh,IFFPARSE_SCAN);
                    if (error==IFFERR_EOF) break;
                    else if (error) goto fail;

                    if (!(cn = CurrentChunk(iffh))) goto fail;

                    if (cn->cn_Type!=ID_PREF) continue;

                    if ((cn->cn_ID==ID_Prefs) && (cn->cn_Size==SIZE_Prefs))
                    {
                        if (ReadChunkBytes(iffh,prefs,cn->cn_Size)!=cn->cn_Size)
                            goto fail;
                    }

                    if ((cn->cn_ID==ID_IFNode) && (cn->cn_Size==SIZE_IFNode))
                    {
                        struct ifnode ifnode;

                        if (ReadChunkBytes(iffh,&ifnode.name,cn->cn_Size)!=cn->cn_Size)
                            goto fail;

                        createIFNode(prefs, ifnode.name, ifnode.scale, MiamiPanelBaseIntern);
                    }
                }

                res = TRUE;

                fail: CloseIFF(iffh);
            }

            Close(file);
        }

        FreeIFF(iffh);
    }

    //if (!res) freeIFList(prefs);

    return res;
}

/**************************************************************************/
