/*
 *----------------------------------------------------------------------------
 *                 bluetooth.library configuration (IFF prefs)
 *----------------------------------------------------------------------------
 *
 * Ported from the poseidon.library configuration code (Chris Hodges): the
 * config is a tree of IFF FORMs kept in memory (struct BtIFFContext), read
 * from and written to ENVARC:Sys/bluetooth.prefs. See bluetooth_intern.h
 * for the layout.
 */

#include "debug.h"
#include "bluetooth.library.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/btclass.h>

#include <string.h>

#define NewList(list) NEWLIST(list)
#define min(x,y) (((x) < (y)) ? (x) : (y))

#define BtClsBase bc->bc_ClassBase
#define DOSBase BluetoothBase->bt_DosBase

/* /// "bRdBE32()/bWrBE32()" */
/*
    IFF only pads chunks to even addresses, so a chunk or form header inside a
    config buffer can sit two bytes off a longword boundary. Access those
    headers bytewise: the compiler is otherwise free to merge neighbouring
    longword accesses into an LDRD/LDM (or STRD/STM) pair, which faults on ARM
    regardless of how the alignment check is configured.
*/
static ULONG bRdBE32(APTR ptr)
{
    UBYTE *b = ptr;

    return(((ULONG) b[0]<<24)|((ULONG) b[1]<<16)|((ULONG) b[2]<<8)|(ULONG) b[3]);
}

static void bWrBE32(APTR ptr, ULONG val)
{
    UBYTE *b = ptr;

    b[0] = val>>24;
    b[1] = val>>16;
    b[2] = val>>8;
    b[3] = val;
}
/* \\\ */

/* /// "btReadCfg()" */
AROS_LH2(BOOL, btReadCfg,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(APTR, formdata, A1),
         LIBBASETYPEPTR, BluetoothBase, 53, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *subpic;
    LONG len;
    ULONG chlen;
    ULONG *buf = formdata;
    BOOL res = TRUE;
    KPRINTF(10, ("btReadCfg(%p, %p)\n", pic, formdata));

    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(!pic) {
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(!(pic->bic_Node.ln_Succ)) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return(FALSE);
        }
    }
    if((bRdBE32(buf) != ID_FORM) || (bRdBE32(&buf[2]) != pic->bic_FormID)) {
        btAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Tried to replace a cfg form with a chunk or with an alien form!");
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        return(FALSE);
    }
    subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    while(subpic->bic_Node.ln_Succ) {
        bFreeForm(BluetoothBase, subpic);
        subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    }
    pic->bic_ChunksLen = 0;
    len = (bRdBE32(&buf[1]) - 3) & ~1UL;
    buf += 3;
    while(len >= 8) {
        if(!(bAddCfgChunk(BluetoothBase, pic, buf))) {
            break;
        }
        chlen = (bRdBE32(&buf[1]) + 9) & ~1UL;
        len -= chlen;
        buf = (ULONG *) (((UBYTE *) buf) + chlen);
    }
    if(len) {
        btAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Tried to add a nasty corrupted FORM chunk! Configuration is probably b0rken!");
        res = 0;
    }

    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    BluetoothBase->bt_CheckConfigReq = TRUE;
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btLoadCfgFromDisk()" */
AROS_LH1(BOOL, btLoadCfgFromDisk,
         AROS_LHA(STRPTR, filename, A1),
         LIBBASETYPEPTR, BluetoothBase, 72, bt)
{
    AROS_LIBFUNC_INIT
    ULONG *buf;
    BPTR filehandle;
    UWORD level;
    BOOL loaded = FALSE;

    XPRINTF(10, ("Loading config file: %s\n", filename));

    if(!filename) {
        loaded = btLoadCfgFromDisk("ENV:Sys/bluetooth.prefs");
        if(loaded) {
            return(TRUE);
        }

        loaded = btLoadCfgFromDisk("ENVARC:Sys/bluetooth.prefs");

        return(loaded);
    }

    if(!bOpenDOS(BluetoothBase)) {
        KPRINTF(1, ("dos.library not available yet\n"));
        return(FALSE);
    }

    filehandle = Open(filename, MODE_OLDFILE);
    KPRINTF(1, ("File handle 0x%p\n", filehandle));
    if(filehandle) {
        ULONG formhead[3];
        ULONG formlen;

        level = RETURN_ERROR;

        if(Read(filehandle, formhead, 12) == 12) {
            KPRINTF(1, ("Read header\n"));
            if((AROS_LONG2BE(formhead[0]) == ID_FORM) && (AROS_LONG2BE(formhead[2]) == IFFFORM_BTCFG)) {
                formlen = AROS_LONG2BE(formhead[1]);
                KPRINTF(1, ("Header OK, %lu bytes\n", formlen));

                buf = (ULONG *) btAllocVec(formlen + 8);
                if(buf) {
                    buf[0] = formhead[0];
                    buf[1] = formhead[1];
                    buf[2] = formhead[2];
                    if(Read(filehandle, &buf[3], formlen - 4) == formlen - 4) {
                        KPRINTF(1, ("Data read OK\n"));

                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Loaded config from '%s' (%ld bytes).", filename, formlen + 8);
                        btReadCfg(NULL, buf);
                        btParseCfg();

                        KPRINTF(1, ("All done\n"));
                        loaded = TRUE;
                        level = RETURN_OK;
                    }
                    btFreeVec(buf);
                }
            }
        }
        Close(filehandle);
    } else
        level = RETURN_WARN;

    if (level != RETURN_OK) {
        btAddErrorMsg(level, (STRPTR) GM_UNIQUENAME(libname),
                       "Failed to %s '%s'!", (level == RETURN_ERROR) ? "load config from" : "find config file",
                       filename);
    }
    if(loaded) {
        BluetoothBase->bt_SavedConfigHash = BluetoothBase->bt_ConfigHash;
    }
    return(loaded);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bSyncStackCfg()" */
/* Rewrites the radio (BHWD) and class (BCLS) entries of the stack config
   from what is running right now, so that a saved prefs file brings the same
   radios and classes back at the next boot. Trident does this in the prefs
   before saving; here it is part of every save so the automatic saves of
   pairings never produce a file that btParseCfg() would read as "no classes
   wanted". */
void bSyncStackCfg(LIBBASETYPEPTR BluetoothBase)
{
    struct BtIFFContext *pic;
    struct BtIFFContext *subpic;
    struct BtIFFContext *root;
    struct BtHardware *bth;
    struct BtClass *bc;
    ULONG unitchunk[3];

    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    pic = btFindCfgForm(NULL, IFFFORM_BTSTACKCFG);
    if(!pic) {
        root = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(root->bic_Node.ln_Succ) {
            pic = bAllocForm(BluetoothBase, root, IFFFORM_BTSTACKCFG);
        }
    }
    if(pic) {
        while((subpic = btFindCfgForm(pic, IFFFORM_BTHWDEVICE))) {
            bFreeForm(BluetoothBase, subpic);
        }
        while((subpic = btFindCfgForm(pic, IFFFORM_BTCLASS))) {
            bFreeForm(BluetoothBase, subpic);
        }
        btLockReadBase();
        ForeachNode(&BluetoothBase->bt_Hardware, bth) {
            if((subpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTHWDEVICE))) {
                bAddStringChunk(BluetoothBase, subpic, IFFCHNK_NAME, bth->bth_DevName);
                unitchunk[0] = AROS_LONG2BE(IFFCHNK_UNIT);
                unitchunk[1] = AROS_LONG2BE(4);
                unitchunk[2] = bth->bth_Unit;
                bAddCfgChunk(BluetoothBase, subpic, unitchunk);
            }
        }
        ForeachNode(&BluetoothBase->bt_Classes, bc) {
            if((subpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTCLASS))) {
                bAddStringChunk(BluetoothBase, subpic, IFFCHNK_NAME,
                                bc->bc_FullPath ? bc->bc_FullPath : bc->bc_ClassName);
            }
        }
        btUnlockBase();
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    bCheckCfgChanged(BluetoothBase);
}
/* \\\ */

/* /// "btSaveCfgToDisk()" */
AROS_LH2(BOOL, btSaveCfgToDisk,
         AROS_LHA(STRPTR, filename, A1),
         AROS_LHA(BOOL, executable, D0),
         LIBBASETYPEPTR, BluetoothBase, 73, bt)
{
    AROS_LIBFUNC_INIT
    ULONG *buf;
    BOOL saved = FALSE;
    BPTR filehandle;

    if(!filename) {
        saved = btSaveCfgToDisk("ENVARC:Sys/bluetooth.prefs", FALSE);
        saved &= btSaveCfgToDisk("ENV:Sys/bluetooth.prefs", FALSE);
        if(saved) {
            /* the prefs file now mirrors memory: automatic saves of
               registrations/bonds (bStoreDevConfig) may go ahead */
            BluetoothBase->bt_ConfigRead = TRUE;
        }
        return(saved);
    }

    if(!bOpenDOS(BluetoothBase)) {
        return(FALSE);
    }
    /* the file describes the running stack: radios and classes included */
    bSyncStackCfg(BluetoothBase);
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);

    buf = (ULONG *) btWriteCfg(NULL);
    if(buf) {
        /* Write file */
        filehandle = Open(filename, MODE_NEWFILE);
        if(filehandle) {
            Write(filehandle, buf, (AROS_LONG2BE(buf[1])+9) & ~1UL);
            Close(filehandle);
            saved = TRUE;
        } else {
            btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Failed to write config to '%s'!",
                           filename);
        }
        btFreeVec(buf);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(saved) {
        BluetoothBase->bt_SavedConfigHash = BluetoothBase->bt_ConfigHash;
    }
    return(saved);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btWriteCfg()" */
AROS_LH1(APTR, btWriteCfg,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         LIBBASETYPEPTR, BluetoothBase, 54, bt)
{
    AROS_LIBFUNC_INIT
    ULONG len;
    APTR buf = NULL;

    KPRINTF(10, ("btWriteCfg(%p)\n", pic));

    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(!pic) {
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(!(pic->bic_Node.ln_Succ)) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return(NULL);
        }
    }
    bUpdateGlobalCfg(BluetoothBase, pic);
    BluetoothBase->bt_CheckConfigReq = TRUE;
    len = bGetFormLength(pic);
    if((buf = btAllocVec(len))) {
        bInternalWriteForm(pic, buf);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(buf);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btFindCfgForm()" */
AROS_LH2(struct BtIFFContext *, btFindCfgForm,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(ULONG, formid, D0),
         LIBBASETYPEPTR, BluetoothBase, 55, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *subpic;

    KPRINTF(160, ("btFindCfgForm(0x%p, 0x%08lx)\n", pic, formid));
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(!pic) {
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(!(pic->bic_Node.ln_Succ)) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return(NULL);
        }
    }
    subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    while(subpic->bic_Node.ln_Succ) {
        if(subpic->bic_FormID == formid) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return(subpic);
        }
        subpic = (struct BtIFFContext *) subpic->bic_Node.ln_Succ;
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btNextCfgForm()" */
AROS_LH1(struct BtIFFContext *, btNextCfgForm,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         LIBBASETYPEPTR, BluetoothBase, 56, bt)
{
    AROS_LIBFUNC_INIT
    ULONG formid;
    KPRINTF(160, ("btNextCfgForm(%p)\n", pic));

    if(!pic) {
        return(NULL);
    }
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    formid = pic->bic_FormID;
    pic = (struct BtIFFContext *) pic->bic_Node.ln_Succ;
    while(pic->bic_Node.ln_Succ) {
        if(pic->bic_FormID == formid) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);

            KPRINTF(1, ("Found context 0x%p\n", pic));
            return(pic);
        }
        pic = (struct BtIFFContext *) pic->bic_Node.ln_Succ;
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btAllocCfgForm()" */
AROS_LH1(struct BtIFFContext *, btAllocCfgForm,
         AROS_LHA(ULONG, formid, D0),
         LIBBASETYPEPTR, BluetoothBase, 61, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;
    KPRINTF(10, ("btAllocCfgForm(%p)\n", formid));
    if((pic = btAllocVec(sizeof(struct BtIFFContext)))) {
        NewList(&pic->bic_SubForms);
        //pic->bic_Parent = parent;
        pic->bic_FormID = formid;
        pic->bic_FormLength = 4;
        pic->bic_Chunks = NULL;
        pic->bic_ChunksLen = 0;
        pic->bic_BufferLen = 0;
        Forbid();
        AddTail(&BluetoothBase->bt_AlienConfigs, &pic->bic_Node);
        Permit();
    }
    return(pic);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemCfgForm()" */
AROS_LH1(void, btRemCfgForm,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         LIBBASETYPEPTR, BluetoothBase, 57, bt)
{
    AROS_LIBFUNC_INIT
    KPRINTF(10, ("btRemCfgForm(%p)\n", pic));

    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(!pic) {
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(!(pic->bic_Node.ln_Succ)) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return;
        }
    }
    bFreeForm(BluetoothBase, pic);
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    BluetoothBase->bt_CheckConfigReq = TRUE;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btAddCfgEntry()" */
AROS_LH2(struct BtIFFContext *, btAddCfgEntry,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(APTR, formdata, A1),
         LIBBASETYPEPTR, BluetoothBase, 58, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *res;

    KPRINTF(10, ("btAddCfgEntry(%p, %p)\n", pic, formdata));
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(!pic) {
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(!(pic->bic_Node.ln_Succ)) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return(NULL);
        }
    }
    res = bAddCfgChunk(BluetoothBase, pic, formdata);
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    BluetoothBase->bt_CheckConfigReq = TRUE;
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemCfgChunk()" */
AROS_LH2(BOOL, btRemCfgChunk,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(ULONG, chnkid, D0),
         LIBBASETYPEPTR, BluetoothBase, 59, bt)
{
    AROS_LIBFUNC_INIT
    BOOL res = FALSE;

    KPRINTF(10, ("btRemCfgChunk(%p, %p)\n", pic, chnkid));
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(!pic) {
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(!(pic->bic_Node.ln_Succ)) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return(FALSE);
        }
    }
    if(chnkid) {
        res = bRemCfgChunk(BluetoothBase, pic, chnkid);
    } else {
        struct BtIFFContext *subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
        while(subpic->bic_Node.ln_Succ) {
            bFreeForm(BluetoothBase, subpic);
            res = TRUE;
            subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
        }
        if(pic->bic_ChunksLen) {
            res = TRUE;
        }
        pic->bic_ChunksLen = 0;
        pic->bic_FormLength = 4;
    }

    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    BluetoothBase->bt_CheckConfigReq = TRUE;
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetCfgChunk()" */
AROS_LH2(APTR, btGetCfgChunk,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(ULONG, chnkid, D0),
         LIBBASETYPEPTR, BluetoothBase, 60, bt)
{
    AROS_LIBFUNC_INIT
    ULONG *chnk;
    ULONG *res = NULL;

    KPRINTF(10, ("btGetCfgChunk(%p, 0x%08lx)\n", pic, chnkid));

    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    if(!pic) {
        pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
        if(!(pic->bic_Node.ln_Succ)) {
            bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
            return(NULL);
        }
    }
    bUpdateGlobalCfg(BluetoothBase, pic);
    chnk = bFindCfgChunk(BluetoothBase, pic, chnkid);
    if(chnk) {
        ULONG chnklen = bRdBE32(&chnk[1]) + 8;

        res = btAllocVec(chnklen);
        if(res) {
            memcpy(res, chnk, chnklen);
        }
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btParseCfg()" */
AROS_LH0(void, btParseCfg,
         LIBBASETYPEPTR, BluetoothBase, 65, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;
    struct BtIFFContext *subpic;
    ULONG *chnk;
    STRPTR name;
    ULONG unit;
    struct BtHardware *bth;
    struct BtClass *bc;
    BOOL removeall = TRUE;
    BOOL nodos = (FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS);
    IPTR restartme;

    XPRINTF(10, ("btParseCfg()\n"));

    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    bCheckCfgChanged(BluetoothBase);
    pic = btFindCfgForm(NULL, IFFFORM_BTSTACKCFG);
    if(!pic) {
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        return;
    }

    // if no config for hardware is found, we don't remove the devices,
    // because this could render the system useless (no USB mice or
    // keyboards to configure the hardware!)
    if(!btFindCfgForm(pic, IFFFORM_BTHWDEVICE)) {
        XPRINTF(10, ("No hardware data present\n"));
        removeall = FALSE;
    }

    btLockReadBase();

    /* select all hardware devices for removal */
    ForeachNode(&BluetoothBase->bt_Hardware, bth) {
        bth->bth_RemoveMe = removeall;
    }

    /* select all classes for removal - unless the config lists no classes
       at all (a prefs file from before the class list was saved with it):
       then, as with the hardware, keep what is loaded */
    ForeachNode(&BluetoothBase->bt_Classes, bc) {
        /*
         * For kickstart-resident classes we check usage count, and
         * remove them only if it's zero.
         * These classes can be responsible for devices which we can use
         * at boot time. If we happen to remove them, we can end up with
         * no input or storage devices at all.
         */
        if(!btFindCfgForm(pic, IFFFORM_BTCLASS))
            bc->bc_RemoveMe = FALSE;
        else if (FindResident(bc->bc_ClassName))
            bc->bc_RemoveMe = (bc->bc_UseCnt == 0);
        else
            bc->bc_RemoveMe = TRUE;
    }

    btUnlockBase();

    /* Get Hardware config */
    subpic = btFindCfgForm(pic, IFFFORM_BTHWDEVICE);
    while(subpic) {
        chnk = bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_NAME);
        if(chnk) {
            name = (STRPTR) &chnk[2];
            unit = 0;
            chnk = bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_UNIT);
            if(chnk) {
                unit = chnk[2];
            }
            if(!bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_OFFLINE)) {
                bth = bFindHardware(BluetoothBase, name, unit);
                XPRINTF(5, ("Have configuration for device 0x%p (%s unit %u)\n", bth, name, unit));
                if(bth) {
                    bth->bth_RemoveMe = FALSE;
                }
            }
        }
        subpic = btNextCfgForm(subpic);
    }

    /* Get Class config */
    subpic = btFindCfgForm(pic, IFFFORM_BTCLASS);
    while(subpic) {
        chnk = bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_NAME);
        if(chnk) {
            name = (STRPTR) &chnk[2];
            bc = (struct BtClass *) bFindName(BluetoothBase, &BluetoothBase->bt_Classes, name);
            XPRINTF(5, ("Have configuration for class 0x%p (%s)\n", bc, name));
            if(bc) {
                bc->bc_RemoveMe = FALSE;
            }
        }
        subpic = btNextCfgForm(subpic);
    }

    // unlock config while removing to avoid deadlocks.
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);

    struct Node *nodetmp;
    /* now remove remaining classes not found in the config */
    ForeachNodeSafe(&BluetoothBase->bt_Classes, bc, nodetmp) {
        if(bc->bc_RemoveMe) {
            XPRINTF(5, ("Removing class %s\n", bc->bc_ClassName));
            btRemClass(bc);
        }
    }

    /* now remove all remaining hardware not found in the config */
    ForeachNodeSafe(&BluetoothBase->bt_Hardware, bth, nodetmp) {
        if(bth->bth_RemoveMe) {
            XPRINTF(5, ("Removing device %s unit %u\n", bth->bth_DevName, bth->bth_Unit));
            btRemHardware(bth);
        }
    }

    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    pic = btFindCfgForm(NULL, IFFFORM_BTSTACKCFG);
    if(!pic) {
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        // oops!
        return;
    }

    /* Add missing Classes */
    subpic = btFindCfgForm(pic, IFFFORM_BTCLASS);
    while(subpic) {
        chnk = bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_NAME);
        if(chnk) {
            /* *** FIXME *** POSSIBLE DEADLOCK WHEN CLASS TRIES TO DO CONFIG STUFF IN
               AN EXTERNAL TASK INSIDE LIBOPEN CODE */
            name = (STRPTR) &chnk[2];
            bc = (struct BtClass *) bFindName(BluetoothBase, &BluetoothBase->bt_Classes, name);
            if(!bc) {
                btAddClass(name, 0);
            }
        }
        subpic = btNextCfgForm(subpic);
    }

    /* Now really mount Hardware found in config */
    subpic = btFindCfgForm(pic, IFFFORM_BTHWDEVICE);
    while(subpic) {
        chnk = bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_NAME);
        if(chnk) {
            name = (STRPTR) &chnk[2];
            unit = 0;
            chnk = bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_UNIT);
            if(chnk) {
                unit = chnk[2];
            }
            if(!bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_OFFLINE)) {
                bth = bFindHardware(BluetoothBase, name, unit);
                if(!bth) {
                    bth = btAddHardware(name, unit);
                    if(bth) {
                        btEnumerateHardware(bth);
                    }
                }
            }
        }
        subpic = btNextCfgForm(subpic);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);

    /* Radios that were already up before this config arrived (the USB class
       adds them early in the boot, BTStackLoader loads the config later)
       enumerated with an empty device list: restore their registered devices
       now. */
    /* Whatever state the radio is in (it may well be re-initialising on a
       late firmware load right now): the device objects do not need it. */
    ForeachNode(&BluetoothBase->bt_Hardware, bth) {
        ULONG count = bRestoreDevices(BluetoothBase, bth);
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: %ld registered device(s) restored from the config.",
                       bth->bth_DevName, bth->bth_Unit, count);
    }

    if(!nodos && BluetoothBase->bt_StartedAsTask) {
        // last time we were reading the config before DOS, so maybe we need to
        // unbind some classes that need to be overruled by newly available classes,
        // such as hid.class overruling bootmouse & bootkeyboard.
        // so unbind those classes that promote themselves as AfterDOS

        btLockReadBase();
        btAddErrorMsg0(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Checking AfterDOS...");
        bc = (struct BtClass *) BluetoothBase->bt_Classes.lh_Head;
        while(bc->bc_Node.ln_Succ) {
            restartme = FALSE;
            btcGetAttrs(BCGA_CLASS, NULL,
                        BCCA_AfterDOSRestart, &restartme,
                        TAG_END);

            if(restartme && bc->bc_UseCnt) {
                struct BtDevice *bd;
                struct BtService *bsv;

                /* Well, try to release the open bindings in a best effort attempt */
                bd = NULL;
                while((bd = btGetNextDevice(bd))) {
                    if(bd->bd_DevBinding && (bd->bd_ClsBinding == bc) && (!(bd->bd_Flags & BDFF_APPBINDING))) {
                        btUnlockBase();
                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "AfterDOS: Temporarily releasing %s %s binding to %s.",
                                       bc->bc_ClassName, "device", bd->bd_Name);
                        btReleaseDevBinding(bd);
                        btLockReadBase();
                        bd = NULL; /* restart */
                        continue;
                    }
                    ForeachNode(&bd->bd_Services, bsv) {
                        if(bsv->bsv_SvcBinding && (bsv->bsv_ClsBinding == bc)) {
                            btUnlockBase();
                            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                           "AfterDOS: Temporarily releasing %s %s binding to %s.",
                                           bc->bc_ClassName, "service", bd->bd_Name);
                            btReleaseSvcBinding(bsv);
                            btLockReadBase();
                            bd = NULL; /* restart */
                            continue;
                        }
                    }
                }
            }
            btcDoMethodA(BCM_DOSAvailableEvent, NULL);
            bc = (struct BtClass *) bc->bc_Node.ln_Succ;
        }
        BluetoothBase->bt_StartedAsTask = FALSE;
        btUnlockBase();
    }

    if(nodos && (!BluetoothBase->bt_ConfigRead)) {
        // it's the first time we were reading the config and DOS was not available
        BluetoothBase->bt_StartedAsTask = TRUE;
    }
    BluetoothBase->bt_ConfigRead = TRUE;
    BluetoothBase->bt_SavedConfigHash = BluetoothBase->bt_ConfigHash; // update saved hash

    /* do a class scan */
    btClassScan();

    if(nodos && BluetoothBase->bt_GlobalCfg->bgc_BootDelay) {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Delaying further execution by %ld second(s) (boot delay).",
                       BluetoothBase->bt_GlobalCfg->bgc_BootDelay);
        btDelayMS(BluetoothBase->bt_GlobalCfg->bgc_BootDelay*1000);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSetClsCfg()" */
AROS_LH2(BOOL, btSetClsCfg,
         AROS_LHA(STRPTR, owner, A0),
         AROS_LHA(APTR, form, A1),
         LIBBASETYPEPTR, BluetoothBase, 66, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;
    BOOL result = FALSE;

    KPRINTF(10, ("btSetClsCfg(%s, %p)\n", owner, form));
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    pic = btFindCfgForm(NULL, IFFFORM_BTCLASSCFG);
    while(pic) {
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_OWNER, owner)) {
            pic = btFindCfgForm(pic, IFFFORM_BTCLASSDATA);
            if(pic) {
                if(form) {
                    result = btReadCfg(pic, form);
                } else {
                    btRemCfgChunk(pic, 0);
                    result = TRUE;
                }
                break;
            } else {
                break;
            }
        }
        pic = btNextCfgForm(pic);
    }
    if(result) {
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        bCheckCfgChanged(BluetoothBase);
        return(result);
    }
    pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
    if(pic->bic_Node.ln_Succ) {
        pic = bAllocForm(BluetoothBase, pic, IFFFORM_BTCLASSCFG);
        if(pic) {
            if(bAddStringChunk(BluetoothBase, pic, IFFCHNK_OWNER, owner)) {
                if(form) {
                    if(bAddCfgChunk(BluetoothBase, pic, form)) {
                        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
                        bCheckCfgChanged(BluetoothBase);
                        return(TRUE);
                    }
                } else {
                    ULONG buf[3];
                    buf[0] = AROS_LONG2BE(ID_FORM);
                    buf[1] = AROS_LONG2BE(4);
                    buf[2] = AROS_LONG2BE(IFFFORM_BTCLASSDATA);
                    if(bAddCfgChunk(BluetoothBase, pic, buf)) {
                        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
                        bCheckCfgChanged(BluetoothBase);
                        return(TRUE);
                    }
                }
            }
        }
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    bCheckCfgChanged(BluetoothBase);
    return(FALSE);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetClsCfg()" */
AROS_LH1(struct BtIFFContext *, btGetClsCfg,
         AROS_LHA(STRPTR, owner, A0),
         LIBBASETYPEPTR, BluetoothBase, 67, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;

    KPRINTF(10, ("btGetClsCfg(%s)\n", owner));
    pic = btFindCfgForm(NULL, IFFFORM_BTCLASSCFG);
    while(pic) {
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_OWNER, owner)) {
            return(btFindCfgForm(pic, IFFFORM_BTCLASSDATA));
        }
        pic = btNextCfgForm(pic);
    }
    return(NULL);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSetDevCfg()" */
AROS_LH4(BOOL, btSetDevCfg,
         AROS_LHA(STRPTR, owner, A0),
         AROS_LHA(STRPTR, devid, A2),
         AROS_LHA(STRPTR, svcid, A3),
         AROS_LHA(APTR, form, A1),
         LIBBASETYPEPTR, BluetoothBase, 68, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;
    struct BtIFFContext *cpic = NULL;
    struct BtIFFContext *mpic = NULL;
    BOOL result = FALSE;

    KPRINTF(10, ("btSetDevCfg(%s, %s, %s, %p)\n", owner, devid, svcid, form));
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
    while(pic) {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, devid)) {
            cpic = NULL;
            /* We found the correct device. Now if we need to store service data, find the service first */
            if(svcid) {
                /* Search service config form */
                mpic = btFindCfgForm(pic, IFFFORM_BTSVCCFGDATA);
                while(mpic) {
                    /* Found the form. Find the the ID String for the service */
                    if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                        /* ID did match, now check for owner */
                        if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            /* found it! So there is already a config saved in there. Search for dev config data form */
                            cpic = btFindCfgForm(mpic, IFFFORM_BTSVCCLSDATA);
                            if(!cpic) {
                                /* not found, generate it */
                                cpic = bAllocForm(BluetoothBase, mpic, IFFFORM_BTSVCCLSDATA);
                            }
                            break;
                        }
                    }
                    mpic = btNextCfgForm(mpic);
                }
                if(!cpic) {
                    if((mpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTSVCCFGDATA))) {
                        if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                                cpic = bAllocForm(BluetoothBase, mpic, IFFFORM_BTSVCCLSDATA);
                            }
                        }
                    }
                }
            } else {
                /* Search for device config */
                mpic = btFindCfgForm(pic, IFFFORM_BTDEVCFGDATA);
                while(mpic) {
                    /* search for the right owner */
                    if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                        /* found it! So there is already a config saved in there. Search for dev config data form */
                        cpic = btFindCfgForm(mpic, IFFFORM_BTDEVCLSDATA);
                        if(!cpic) {
                            /* not found, generate it */
                            cpic = bAllocForm(BluetoothBase, mpic, IFFFORM_BTDEVCLSDATA);
                        }
                        break;
                    }
                    mpic = btNextCfgForm(mpic);
                }
                if(!cpic) { /* no device config form */
                    if((mpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTDEVCFGDATA))) {
                        if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            cpic = bAllocForm(BluetoothBase, mpic, IFFFORM_BTDEVCLSDATA);
                        }
                    }
                }
            }
            if(cpic) {
                if(form) {
                    result = btReadCfg(cpic, form);
                } else {
                    btRemCfgChunk(cpic, 0);
                    result = TRUE;
                }
                break;
            }
        }
        pic = btNextCfgForm(pic);
    }
    if(result) {
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        bCheckCfgChanged(BluetoothBase);
        return(result);
    }
    cpic = NULL;
    pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
    if(pic->bic_Node.ln_Succ) {
        pic = bAllocForm(BluetoothBase, pic, IFFFORM_BTDEVICECFG);
        if(pic) {
            if(bAddStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, devid)) {
                if(svcid) {
                    if((mpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTSVCCFGDATA))) {
                        if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                                cpic = bAllocForm(BluetoothBase, mpic, IFFFORM_BTSVCCLSDATA);
                            }
                        }
                    }
                } else {
                    if((mpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTDEVCFGDATA))) {
                        if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            cpic = bAllocForm(BluetoothBase, mpic, IFFFORM_BTDEVCLSDATA);
                        }
                    }
                }
                if(cpic) {
                    if(form) {
                        result = btReadCfg(cpic, form);
                    } else {
                        btRemCfgChunk(cpic, 0);
                        result = TRUE;
                    }
                }
            }
        }
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    bCheckCfgChanged(BluetoothBase);
    return(result);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetDevCfg()" */
AROS_LH3(struct BtIFFContext *, btGetDevCfg,
         AROS_LHA(STRPTR, owner, A0),
         AROS_LHA(STRPTR, devid, A2),
         AROS_LHA(STRPTR, svcid, A3),
         LIBBASETYPEPTR, BluetoothBase, 69, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;
    struct BtIFFContext *cpic = NULL;
    struct BtIFFContext *mpic = NULL;

    KPRINTF(10, ("btGetDevCfg(%s, %s, %s)\n", owner, devid, svcid));
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
    while(pic) {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, devid)) {
            cpic = NULL;
            /* We found the correct device. Now if we need to store service data, find the service first */
            if(svcid) {
                /* Search service config form */
                mpic = btFindCfgForm(pic, IFFFORM_BTSVCCFGDATA);
                while(mpic) {
                    /* Found the form. Find the the ID String for the service */
                    if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                        /* ID did match, now check for owner */
                        if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            /* found it! So there is already a config saved in there. Search for dev config data form */
                            cpic = btFindCfgForm(mpic, IFFFORM_BTSVCCLSDATA);
                            break;
                        }
                    }
                    mpic = btNextCfgForm(mpic);
                }
            } else {
                /* Search for device config */
                mpic = btFindCfgForm(pic, IFFFORM_BTDEVCFGDATA);
                while(mpic) {
                    /* search for the right owner */
                    if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                        /* found it! So there is already a config saved in there. Search for dev config data form */
                        cpic = btFindCfgForm(mpic, IFFFORM_BTDEVCLSDATA);
                        break;
                    }
                    mpic = btNextCfgForm(mpic);
                }
            }
            break;
        }
        pic = btNextCfgForm(pic);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    KPRINTF(1, ("Result %p\n", cpic));
    return(cpic);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btSetForcedBinding()" */
AROS_LH3(BOOL, btSetForcedBinding,
         AROS_LHA(STRPTR, owner, A2),
         AROS_LHA(STRPTR, devid, A0),
         AROS_LHA(STRPTR, svcid, A1),
         LIBBASETYPEPTR, BluetoothBase, 70, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;
    struct BtIFFContext *mpic = NULL;
    ULONG olen = 0;
    BOOL result = FALSE;

    if(owner) {
        olen = strlen(owner);
    }
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
    while(pic) {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, devid)) {
            /* We found the correct device. Now if we need to store service data, find the service first */
            if(svcid) {
                /* Search service config form */
                mpic = btFindCfgForm(pic, IFFFORM_BTSVCCFGDATA);
                while(mpic) {
                    /* Found the form. Find the the ID String for the service */
                    if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                        /* ID did match, insert/replace forced binding */
                        if(olen) {
                            if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_FORCEDBIND, owner)) {
                                result = TRUE;
                            }
                        } else {
                            bRemCfgChunk(BluetoothBase, mpic, IFFCHNK_FORCEDBIND);
                            result = TRUE;
                        }
                    }
                    mpic = btNextCfgForm(mpic);
                }
                if(!olen) {
                    result = TRUE;
                }
                if((!result) && olen) {
                    if((mpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTSVCCFGDATA))) {
                        if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_FORCEDBIND, owner)) {
                                if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                                    result = TRUE;
                                }
                            }
                        }
                    }
                }
            } else {
                /* Add FBND chunk */
                if(olen) {
                    if(bAddStringChunk(BluetoothBase, pic, IFFCHNK_FORCEDBIND, owner)) {
                        result = TRUE;
                    }
                } else {
                    bRemCfgChunk(BluetoothBase, pic, IFFCHNK_FORCEDBIND);
                    result = TRUE;
                }
            }
            break;
        }
        pic = btNextCfgForm(pic);
    }
    if(!olen) {
        result = TRUE;
    }
    if(result) {
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        bCheckCfgChanged(BluetoothBase);
        return(result);
    }
    pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
    if(pic->bic_Node.ln_Succ) {
        pic = bAllocForm(BluetoothBase, pic, IFFFORM_BTDEVICECFG);
        if(pic) {
            if(bAddStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, devid)) {
                if(svcid) {
                    if((mpic = bAllocForm(BluetoothBase, pic, IFFFORM_BTSVCCFGDATA))) {
                        if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_OWNER, owner)) {
                            if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_FORCEDBIND, owner)) {
                                if(bAddStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                                    result = TRUE;
                                }
                            }
                        }
                    }
                } else {
                    /* Add FBND chunk */
                    if(bAddStringChunk(BluetoothBase, pic, IFFCHNK_FORCEDBIND, owner)) {
                        result = TRUE;
                    }
                }
            }
        }
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    bCheckCfgChanged(BluetoothBase);
    return(result);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetForcedBinding()" */
AROS_LH2(STRPTR, btGetForcedBinding,
         AROS_LHA(STRPTR, devid, A0),
         AROS_LHA(STRPTR, svcid, A1),
         LIBBASETYPEPTR, BluetoothBase, 71, bt)
{
    AROS_LIBFUNC_INIT
    struct BtIFFContext *pic;
    struct BtIFFContext *mpic = NULL;
    ULONG *chunk;
    STRPTR owner = NULL;

    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    /* Find device config form. It contains all device config data */
    pic = btFindCfgForm(NULL, IFFFORM_BTDEVICECFG);
    while(pic) {
        /* Find DEVID-Chunk. Check if it matches our device id */
        if(bMatchStringChunk(BluetoothBase, pic, IFFCHNK_DEVID, devid)) {
            /* We found the correct device. Now if we need to store service data, find the service first */
            if(svcid) {
                /* Search service config form */
                mpic = btFindCfgForm(pic, IFFFORM_BTSVCCFGDATA);
                while(mpic) {
                    /* Found the form. Find the the ID String for the service */
                    if(bMatchStringChunk(BluetoothBase, mpic, IFFCHNK_SVCID, svcid)) {
                        /* ID did match, now check for forced binding */
                        chunk = bFindCfgChunk(BluetoothBase, mpic, IFFCHNK_FORCEDBIND);
                        if(chunk) {
                            owner = (STRPTR) &chunk[2];
                            break;
                        }
                    }
                    mpic = btNextCfgForm(mpic);
                }
            } else {
                /* Search for device forced binding */
                chunk = bFindCfgChunk(BluetoothBase, pic, IFFCHNK_FORCEDBIND);
                if(chunk) {
                    owner = (STRPTR) &chunk[2];
                    break;
                }
            }
            break;
        }
        pic = btNextCfgForm(pic);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(owner);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btAddStringChunk()" */
AROS_LH3(BOOL, btAddStringChunk,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(ULONG, chunkid, D0),
         AROS_LHA(CONST_STRPTR, str, A1),
         LIBBASETYPEPTR, BluetoothBase, 62, bt)
{
    AROS_LIBFUNC_INIT
    BOOL res;
    KPRINTF(10, ("btAddStringChunk(%p, %p, %s)\n", pic, chunkid, str));
    bLockSemExcl(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    res = bAddStringChunk(BluetoothBase, pic, chunkid, str);
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btMatchStringChunk()" */
AROS_LH3(BOOL, btMatchStringChunk,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(ULONG, chunkid, D0),
         AROS_LHA(CONST_STRPTR, str, A1),
         LIBBASETYPEPTR, BluetoothBase, 63, bt)
{
    AROS_LIBFUNC_INIT
    BOOL res;
    KPRINTF(10, ("btMatchStringChunk(%p, %p, %s)\n", pic, chunkid, str));
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    res = bMatchStringChunk(BluetoothBase, pic, chunkid, str);
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(res);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btGetStringChunk()" */
AROS_LH2(STRPTR, btGetStringChunk,
         AROS_LHA(struct BtIFFContext *, pic, A0),
         AROS_LHA(ULONG, chunkid, D0),
         LIBBASETYPEPTR, BluetoothBase, 64, bt)
{
    AROS_LIBFUNC_INIT
    STRPTR str;
    KPRINTF(10, ("btGetStringChunk(%p, %p)\n", pic, chunkid));
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    str = bGetStringChunk(BluetoothBase, pic, chunkid);
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(str);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* *** (non-library subroutines) *** */

/* /// "bAllocForm()" */
struct BtIFFContext * bAllocForm(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *parent, ULONG formid)
{
    struct BtIFFContext *pic;
    KPRINTF(10, ("bAllocForm(%p, %p)\n", parent, formid));
    if((pic = btAllocVec(sizeof(struct BtIFFContext)))) {
        NewList(&pic->bic_SubForms);
        //pic->bic_Parent = parent;
        pic->bic_FormID = formid;
        pic->bic_FormLength = 4;
        pic->bic_Chunks = NULL;
        pic->bic_ChunksLen = 0;
        pic->bic_BufferLen = 0;
        Forbid();
        if(parent) {
            AddTail(&parent->bic_SubForms, &pic->bic_Node);
        } else {
            AddTail(&BluetoothBase->bt_ConfigRoot, &pic->bic_Node);
        }
        Permit();
    }
    return(pic);
}
/* \\\ */

/* /// "bFreeForm()" */
void bFreeForm(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic)
{
    struct BtIFFContext *subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    KPRINTF(10, ("bFreeForm(%p)\n", pic));
    Remove(&pic->bic_Node);
    while(subpic->bic_Node.ln_Succ) {
        bFreeForm(BluetoothBase, subpic);
        subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    }
    btFreeVec(pic->bic_Chunks);
    btFreeVec(pic);
}
/* \\\ */

/* /// "bGetFormLength()" */
ULONG bGetFormLength(struct BtIFFContext *pic)
{
    ULONG len = (5 + pic->bic_ChunksLen) & ~1UL;
    struct BtIFFContext *subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    //KPRINTF(10, ("bGetFormLength(%p)\n", pic));
    while(subpic->bic_Node.ln_Succ) {
        len += bGetFormLength(subpic);
        subpic = (struct BtIFFContext *) subpic->bic_Node.ln_Succ;
    }
    pic->bic_FormLength = len;
    //KPRINTF(10, ("FormLen=%ld\n", len+8));
    return(len + 8);
}
/* \\\ */

/* /// "bFindCfgChunk()" */
APTR bFindCfgChunk(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic, ULONG chnkid)
{
    ULONG *buf = pic->bic_Chunks;
    ULONG len = pic->bic_ChunksLen;
    ULONG chlen;
    KPRINTF(10, ("bFindCfgChunk(%p, %p)\n", pic, chnkid));

    while(len) {
        if(bRdBE32(buf) == chnkid) {
            KPRINTF(10, ("Found at %p\n", buf));
            return(buf);
        }
        chlen = (bRdBE32(&buf[1]) + 9) & ~1UL;
        len -= chlen;
        buf = (ULONG *) (((UBYTE *) buf) + chlen);
    }
    KPRINTF(10, ("Not found!\n"));
    return(NULL);
}
/* \\\ */

/* /// "bRemCfgChunk()" */
BOOL bRemCfgChunk(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic, ULONG chnkid)
{
    ULONG *buf = pic->bic_Chunks;
    ULONG len = pic->bic_ChunksLen;
    ULONG chlen;
    KPRINTF(10, ("bRemCfgChunk(%p, %p)\n", pic, chnkid));

    while(len) {
        chlen = (bRdBE32(&buf[1]) + 9) & ~1UL;
        if(bRdBE32(buf) == chnkid) {
            len -= chlen;
            if(len) {
                memcpy(buf, &((UBYTE *) buf)[chlen], (size_t) len);
            }
            pic->bic_ChunksLen -= chlen;
            KPRINTF(10, ("Deleted %ld bytes to %ld chunk len\n", chlen, pic->bic_ChunksLen));
            return(TRUE);
        }
        len -= chlen;
        buf = (ULONG *) (((UBYTE *) buf) + chlen);
    }
    KPRINTF(10, ("Not found!\n"));
    return(FALSE);
}
/* \\\ */

/* /// "bAddCfgChunk()" */
struct BtIFFContext * bAddCfgChunk(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic, APTR chunk)
{
    LONG len;
    LONG chlen;
    ULONG *buf = chunk;
    ULONG *newbuf;
    struct BtIFFContext *subpic;
    KPRINTF(10, ("bAddCfgChunk(%p, %p)\n", pic, chunk));
    if(bRdBE32(buf) == ID_FORM) {
        buf++;
        len = (bRdBE32(buf) - 3) & ~1UL;
        buf++;
        if((subpic = bAllocForm(BluetoothBase, pic, bRdBE32(buf)))) {
            buf++;
            while(len >= 8) {
                if(!(bAddCfgChunk(BluetoothBase, subpic, buf))) {
                    break;
                }
                chlen = (bRdBE32(&buf[1]) + 9) & ~1UL;
                len -= chlen;
                buf = (ULONG *) (((UBYTE *) buf) + chlen);
            }
            if(len) {
                btAddErrorMsg0(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Tried to add a nasty corrupted FORM chunk! Configuration is probably b0rken!");
                return(NULL);
            }
        } else {
            return(NULL);
        }
        return(subpic);
    } else {
        bRemCfgChunk(BluetoothBase, pic, bRdBE32(buf));
        len = (bRdBE32(&buf[1]) + 9) & ~1UL;
        if(pic->bic_ChunksLen+len > pic->bic_BufferLen) {
            KPRINTF(10, ("expanding buffer from %ld to %ld to fit %ld bytes\n", pic->bic_BufferLen, (pic->bic_ChunksLen+len)<<1, pic->bic_ChunksLen+len));

            /* Expand buffer */
            if((newbuf = btAllocVec((pic->bic_ChunksLen+len)<<1))) {
                if(pic->bic_ChunksLen) {
                    memcpy(newbuf, pic->bic_Chunks, (size_t) pic->bic_ChunksLen);
                    btFreeVec(pic->bic_Chunks);
                }
                pic->bic_Chunks = newbuf;
                pic->bic_BufferLen = (pic->bic_ChunksLen+len)<<1;
            } else {
                return(NULL);
            }
        }
        memcpy(&(((UBYTE *) pic->bic_Chunks)[pic->bic_ChunksLen]), chunk, (size_t) len);
        pic->bic_ChunksLen += len;
        return(pic);
    }
}
/* \\\ */

/* /// "bInternalWriteForm()" */
ULONG * bInternalWriteForm(struct BtIFFContext *pic, ULONG *buf)
{
    struct BtIFFContext *subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    //KPRINTF(10, ("bInternalWriteForm(%p, %p)", pic, buf));
    bWrBE32(buf++, ID_FORM);
    bWrBE32(buf++, pic->bic_FormLength);
    bWrBE32(buf++, pic->bic_FormID);
    if(pic->bic_ChunksLen) {
        memcpy(buf, pic->bic_Chunks, (size_t) pic->bic_ChunksLen);
        buf = (ULONG *) (((UBYTE *) buf) + pic->bic_ChunksLen);
    }
    while(subpic->bic_Node.ln_Succ) {
        buf = bInternalWriteForm(subpic, buf);
        subpic = (struct BtIFFContext *) subpic->bic_Node.ln_Succ;
    }
    return(buf);
}
/* \\\ */

/* /// "bCalcCfgCRC()" */
ULONG bCalcCfgCRC(struct BtIFFContext *pic)
{
    struct BtIFFContext *subpic = (struct BtIFFContext *) pic->bic_SubForms.lh_Head;
    ULONG len;
    ULONG crc = pic->bic_FormID;
    UWORD *ptr;

    //KPRINTF(10, ("bInternalWriteForm(%p, %p)", pic, buf));
    if(pic->bic_ChunksLen) {
        len = pic->bic_ChunksLen>>1;
        if(len) {
            ptr = (UWORD *) pic->bic_Chunks;
            do {
                crc = ((crc<<1)|(crc>>31))^(*ptr++);
            } while(--len);
        }
    }
    while(subpic->bic_Node.ln_Succ) {
        crc ^= bCalcCfgCRC(subpic);
        subpic = (struct BtIFFContext *) subpic->bic_Node.ln_Succ;
    }
    return(crc);
}
/* \\\ */

/* /// "bCheckCfgChanged()" */
BOOL bCheckCfgChanged(LIBBASETYPEPTR BluetoothBase)
{
    ULONG crc;
    struct BtIFFContext *pic;
    struct BtIFFContext *subpic;
    bLockSemShared(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    BluetoothBase->bt_CheckConfigReq = FALSE;
    pic = (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head;
    if(!(pic->bic_Node.ln_Succ)) {
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        return(FALSE);
    }
    crc = bCalcCfgCRC(pic);
    if(crc != BluetoothBase->bt_ConfigHash) {
        ULONG *chnk;
        BluetoothBase->bt_ConfigHash = crc;
        /* Get Global config */
        if((subpic = btFindCfgForm(pic, IFFFORM_BTSTACKCFG))) {
            if((chnk = bFindCfgChunk(BluetoothBase, subpic, IFFCHNK_GLOBALCFG))) {
                CopyMem(&chnk[2], ((UBYTE *) BluetoothBase->bt_GlobalCfg) + 8, min(bRdBE32(&chnk[1]), AROS_LONG2BE(BluetoothBase->bt_GlobalCfg->bgc_Length)));
            }
        }
        bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
        btSendEvent(BEHMB_CONFIGCHG, NULL, NULL);
        return(TRUE);
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    return(FALSE);
}
/* \\\ */

/* /// "bAddStringChunk()" */
BOOL bAddStringChunk(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic, ULONG chunkid, CONST_STRPTR str)
{
    BOOL res = FALSE;
    ULONG len = strlen(str);
    ULONG *chnk = (ULONG *) btAllocVec((ULONG) len+8+2);
    if(chnk) {
        chnk[0] = AROS_LONG2BE(chunkid);
        chnk[1] = AROS_LONG2BE(len+1);
        strcpy((STRPTR) &chnk[2], str);
        if(bAddCfgChunk(BluetoothBase, pic, chnk)) {
            res = TRUE;
        }
        btFreeVec(chnk);
    }
    return(res);
}
/* \\\ */

/* /// "bMatchStringChunk()" */
BOOL bMatchStringChunk(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic, ULONG chunkid, CONST_STRPTR str)
{
    ULONG *chunk;
    ULONG len;
    STRPTR srcptr;
    if((chunk = bFindCfgChunk(BluetoothBase, pic, chunkid))) {
        srcptr = (STRPTR) &chunk[2];
        len = bRdBE32(&chunk[1]);
        while(len-- && *srcptr) {
            if(*str++ != *srcptr++) {
                return(FALSE);
            }
        }
        if(!*str) {
            return(TRUE);
        }
    }
    return(FALSE);
}
/* \\\ */

/* /// "bGetStringChunk()" */
STRPTR bGetStringChunk(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic, ULONG chunkid)
{
    ULONG *chunk;
    STRPTR str;
    if((chunk = bFindCfgChunk(BluetoothBase, pic, chunkid))) {
        ULONG len = bRdBE32(&chunk[1]);

        if((str = (STRPTR) btAllocVec(len + 1))) {
            memcpy(str, &chunk[2], (size_t) len);
            return(str);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "bUpdateGlobalCfg()" */
void bUpdateGlobalCfg(LIBBASETYPEPTR BluetoothBase, struct BtIFFContext *pic)
{
    struct BtIFFContext *tmppic;
    /* Set Global config */
    if(pic == (struct BtIFFContext *) BluetoothBase->bt_ConfigRoot.lh_Head) {
        if((tmppic = btFindCfgForm(NULL, IFFFORM_BTSTACKCFG))) {
            bAddCfgChunk(BluetoothBase, tmppic, BluetoothBase->bt_GlobalCfg);
        }
    }
}
/* \\\ */

/* *** Misc (non library functions) ***/

