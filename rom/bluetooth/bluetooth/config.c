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
/*****************************************************************************
    NAME
        BOOL btReadCfg(

    SYNOPSIS
        APTR bic, APTR formdata)

    FUNCTION
        Replace the contents of an existing configuration FORM. Its type
        and context remain; old child FORMs are freed and chunks replaced.

    INPUTS
        bic - Context to replace, or NULL for the global root.
        formdata - Serialized FORM, including its header and type. The type
            must match the destination context. The buffer is copied.

    RESULT
        TRUE if the input is consumed successfully. FALSE if the root is
        absent, the header or type differs, allocation fails, or parsing
        leaves unconsumed data.

    NOTES
        Supply a complete, valid IFF buffer with consistent nested lengths
        and a padding byte after each odd-length chunk. No buffer length
        is passed to this API; malformed input is not safely rejected.
        The buffer must not alias storage in the destination tree.

        Call from task context. A type mismatch leaves the tree unchanged.
        Once replacement starts, failure can leave partial contents; there
        is no rollback. Earlier child-context and string references become
        invalid. This call requests change detection but does not mount
        hardware or save a file.

    SEE ALSO
        btWriteCfg(), btParseCfg(), btAddCfgEntry()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        BOOL btLoadCfgFromDisk(

    SYNOPSIS
        STRPTR filename)

    FUNCTION
        Read an IFF preference file into the global configuration and apply
        it to the running stack.

    INPUTS
        filename - NUL-terminated filename, or NULL to try
            ENV:Sys/bluetooth.prefs first and ENVARC:Sys/bluetooth.prefs
            only if the first attempt returns FALSE.

    RESULT
        TRUE if a matching root header and its declared payload were read.
        FALSE for library, open, header, allocation or incomplete-read
        failures. TRUE does not establish successful configuration parsing.

    NOTES
        The root must use the current target's IFFFORM_BTCFG: BTBC on
        big-endian systems, BTLC on little-endian systems. Payloads contain
        native structures and are not converted between target formats.

        Call from process context with DOS available. Applying the file can
        remove hardware and release bindings. The filename is borrowed only
        during the call. Successful file reading updates the saved-state
        marker even when applying the configuration fails.

    BUGS
        The parser result is ignored; TRUE can describe a partially replaced
        or rejected configuration. Such a result prevents the default-path
        fallback. Declared lengths are not adequately validated before
        allocation and parsing. Load only complete, trusted preference files.

    SEE ALSO
        btReadCfg(), btParseCfg(), btSaveCfgToDisk()

*****************************************************************************/
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
/* Rewrites the radio (BHWD) entries of the stack config from what is
   running right now, so a saved prefs file mounts the same radios at the
   next boot (BTStackLoader runs before AddBTHardware on serial-attached
   radios). Classes are NOT recorded: they are installed by AddBTClasses,
   and the config only stores their configurations. */
void bSyncStackCfg(LIBBASETYPEPTR BluetoothBase)
{
    struct BtIFFContext *pic;
    struct BtIFFContext *subpic;
    struct BtIFFContext *root;
    struct BtHardware *bth;
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
        btUnlockBase();
    }
    bUnlockSem(BluetoothBase, &BluetoothBase->bt_ConfigLock);
    bCheckCfgChanged(BluetoothBase);
}
/* \\\ */

/* /// "btSaveCfgToDisk()" */
/*****************************************************************************
    NAME
        BOOL btSaveCfgToDisk(

    SYNOPSIS
        STRPTR filename, BOOL mode)

    FUNCTION
        Serialize the running configuration to a preference file. First
        replace stored radio entries with the currently registered radios;
        class configurations remain, but no loaded-class list is generated.

    INPUTS
        filename - NUL-terminated filename, or NULL to attempt both
            ENVARC:Sys/bluetooth.prefs and ENV:Sys/bluetooth.prefs, in that
            order, even if the first attempt fails.
        mode - Ignored. Only a data file is written.

    RESULT
        TRUE if serialization succeeds and the destination opens. For NULL,
        both destinations must meet those conditions. FALSE otherwise.
        Write and close failures are not reflected in this result.

    NOTES
        Call from process context with DOS available. An existing file is
        truncated when opened; no temporary-file replacement or rollback is
        provided. The in-memory radio list is rewritten even if opening the
        file later fails. The filename is not retained. A NULL-path success
        also permits subsequent automatic registration and bond saves.
        Files use the current target's IFFFORM_BTCFG and native payloads.

    BUGS
        A short write or close failure can leave a damaged file while the
        function returns TRUE and marks the configuration saved. Failure
        to rebuild every in-memory radio entry is also not reported.

    SEE ALSO
        btWriteCfg(), btLoadCfgFromDisk()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btWriteCfg(

    SYNOPSIS
        APTR bic)

    FUNCTION
        Serialize a configuration FORM and its descendants. Plain chunks
        precede child FORMs in the output. For the global root, refresh the
        stored global settings from the running stack first.

    INPUTS
        bic - Live context, or NULL for the global root.

    RESULT
        Newly allocated FORM buffer, or NULL if the root is absent or the
        buffer cannot be allocated. Release the buffer with btFreeVec().

    NOTES
        The buffer includes the eight-byte IFF header; decode its big-endian
        length and add eight to obtain the total byte count. It remains
        independent of later tree changes. Chunk payloads can contain native
        structures: the root uses IFFFORM_BTCFG for the current byte order,
        not a portable interchange format. Keep bluetooth.library open
        until the buffer is freed. Call from task context.

        Serialize this call with other configuration access. Root export
        can replace the stored global-settings chunk. An allocation failure
        during that refresh is not reported separately; a non-NULL result
        does not guarantee that every global setting was refreshed.

    SEE ALSO
        btReadCfg(), btSaveCfgToDisk(), btFreeVec()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btFindCfgForm(

    SYNOPSIS
        APTR bic, ULONG formid)

    FUNCTION
        Find the first immediate child FORM of the requested type. The
        search does not descend into grandchildren or test the parent itself.

    INPUTS
        bic - Parent context, or NULL for the global root.
        formid - FORM type to find, in host byte order.

    RESULT
        Borrowed child context, or NULL if no match or no root exists.

    NOTES
        The context is borrowed, not a snapshot. Do not free it directly.
        Prevent concurrent removal or replacement while using it; internal
        locking does not extend its lifetime after this call returns.
        Call from task context. Use btNextCfgForm() to find later siblings
        of the same type.

    SEE ALSO
        btNextCfgForm(), btAllocCfgForm()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btNextCfgForm(

    SYNOPSIS
        APTR bic)

    FUNCTION
        Find the next sibling with the same FORM type as the supplied
        context, skipping siblings of other types.

    INPUTS
        bic - Live context in a context list, or NULL.

    RESULT
        Borrowed matching sibling, or NULL at the end of the list or for
        a NULL input.

    NOTES
        The context is borrowed, not a snapshot. Do not free it directly.
        Prevent concurrent removal or replacement while using it; internal
        locking does not extend its lifetime after this call returns.
        Call from task context. Do not remove the current context before
        using it to locate its successor.

    SEE ALSO
        btFindCfgForm(), btRemCfgForm()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btAllocCfgForm(

    SYNOPSIS
        ULONG formid)

    FUNCTION
        Create an empty, independent configuration FORM. It is not attached
        to the global configuration tree.

    INPUTS
        formid - FORM type in host byte order.

    RESULT
        New context, or NULL on allocation failure.

    NOTES
        Release the context and its descendants with btRemCfgForm(), not
        btFreeVec(). Keep bluetooth.library open until it is released.
        Populate it with btAddCfgEntry() or btReadCfg(). To add its contents
        to another tree, serialize it with btWriteCfg() and add that buffer;
        a context pointer is not a serialized FORM. Call from task context.

    SEE ALSO
        btRemCfgForm(), btWriteCfg(), btAddCfgEntry()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        BOOL btRemCfgForm(

    SYNOPSIS
        APTR bic)

    FUNCTION
        Unlink and free a configuration FORM, all descendant FORMs and
        their stored chunks.

    INPUTS
        bic - Live context to remove, or NULL to remove the global root.
            If no global root exists, NULL has no effect.

    RESULT
        No usable result. Although the public declaration returns BOOL,
        the implementation returns no value; do not test the result.

    NOTES
        All references to the removed tree become invalid. Removing the
        global root does not create a replacement root or stop running
        hardware. Call from task context and prevent concurrent users from
        accessing the context. Change detection is requested after removal.

    SEE ALSO
        btAllocCfgForm(), btRemCfgChunk()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btAddCfgEntry(

    SYNOPSIS
        APTR bic, APTR formdata)

    FUNCTION
        Copy an IFF chunk or FORM into a configuration context. A plain
        chunk replaces the first chunk with the same ID, then appends the
        new data. A FORM is appended as a new child; existing FORMs of that
        type are retained.

    INPUTS
        bic - Destination context, or NULL for the global root.
        formdata - Complete serialized chunk or FORM, including its header
            and even-byte padding.

    RESULT
        For a plain chunk, the destination context. For a FORM, the new
        child context. NULL if the root is absent, allocation fails or a
        FORM cannot be consumed completely.

    NOTES
        Supply a complete, valid IFF buffer with consistent nested lengths
        and a padding byte after each odd-length chunk. No buffer length
        is passed to this API; malformed input is not safely rejected.
        The buffer must not alias storage in the destination tree.

        The input is not retained. Returned contexts belong to the tree.
        Failure is not atomic: replacement can remove the old chunk before
        allocation fails, and failed FORM import can leave a partial child.
        Call from task context. Change detection is requested; no file is
        saved and no hardware configuration is applied.

    BUGS
        Replacing a chunk can reach the overlapping-copy defect described
        under btRemCfgChunk().

    SEE ALSO
        btReadCfg(), btRemCfgChunk(), btWriteCfg()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        BOOL btRemCfgChunk(

    SYNOPSIS
        APTR bic, ULONG chnkid)

    FUNCTION
        Remove the first plain chunk with a matching ID. If the ID is zero,
        clear all chunks and child FORMs while retaining the context itself.

    INPUTS
        bic - Context to modify, or NULL for the global root.
        chnkid - Chunk ID in host byte order; zero clears the contents.

    RESULT
        TRUE if a chunk or child FORM was removed; FALSE if nothing matched
        or the global root is absent.

    NOTES
        A nonzero ID does not remove child FORMs; use btRemCfgForm() for
        those. Clearing the context invalidates all descendant references.
        Chunk removal can invalidate borrowed strings within the context.
        Call from task context. Change detection is requested.

    BUGS
        Removing a chunk before later chunks uses memcpy() on potentially
        overlapping storage. The remaining data is not reliably preserved
        on that path.

    SEE ALSO
        btRemCfgForm(), btAddCfgEntry()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btGetCfgChunk(

    SYNOPSIS
        APTR bic, ULONG chnkid)

    FUNCTION
        Copy the first plain chunk with a matching ID. Only the supplied
        context is searched. A root query also refreshes its stored global
        settings from the running stack.

    INPUTS
        bic - Live context, or NULL for the global root.
        chnkid - Chunk ID in host byte order.

    RESULT
        Allocated chunk including its eight-byte header, or NULL if no
        match or root exists, or allocation fails. Release with btFreeVec().

    NOTES
        The returned size is eight plus the big-endian payload length in
        the header. An odd-length payload has no extra padding byte in this
        copy. Add padding before passing such a copy to btAddCfgEntry().

        The copy is independent of later configuration changes. Its payload
        is opaque and may contain native structures. Keep bluetooth.library
        open until it is freed. Call from task context and serialize root
        queries with other configuration access.

    SEE ALSO
        btAddCfgEntry(), btGetStringChunk(), btFreeVec()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        VOID btParseCfg(

    SYNOPSIS
        )

    FUNCTION
        Apply the in-memory stack configuration to running hardware and
        classes, restore registered devices and scan for class bindings.

    INPUTS
        None.

    RESULT
        None. Individual hardware, class and binding failures are not
        returned to the caller.

    NOTES
        Call from task context; hardware changes and class initialization
        can block. If STKC is absent, no hardware reconciliation occurs.
        If STKC contains BHWD entries, remove radios that have no matching
        online entry, then add and enumerate missing online radios. Without
        any BHWD entries, retain existing radios. Legacy BCLS entries add
        missing classes; omission from that list does not remove a class.

        This call can disconnect devices and release bindings. On the first
        process-context application after an earlier task-context load, it
        performs AfterDOS class handling. Before DOS, the configured boot
        delay can postpone return. It marks the resulting configuration as
        read and updates its saved-state marker without writing a file.
        Serialize configuration replacement with this operation.

    BUGS
        Opening a class while holding the configuration lock can deadlock
        if its initialization waits for another task that needs that lock.

    SEE ALSO
        btReadCfg(), btLoadCfgFromDisk(), btSaveCfgToDisk()

*****************************************************************************/
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

    /* Unlike Poseidon (where Trident is the class registry and the saved
       list decides what exists), classes here are installed by AddBTClasses
       from SYS:Classes/Bluetooth. The config only carries per-class
       CONFIGURATIONS (CLSC forms); a class list in an old prefs file is
       honoured additively below but never removes a loaded class. */
    ForeachNode(&BluetoothBase->bt_Classes, bc) {
        bc->bc_RemoveMe = FALSE;
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
        if(count) {
            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "%s/%ld: %ld registered device(s) restored from the config.",
                           bth->bth_DevName, bth->bth_Unit, count);
        }
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
/*****************************************************************************
    NAME
        BOOL btSetClsCfg(

    SYNOPSIS
        STRPTR owner, APTR form)

    FUNCTION
        Store private class configuration under the specified owner.
        Replace existing class-data contents, or create a class entry.

    INPUTS
        owner - Non-NULL, NUL-terminated class owner name. Case-sensitive.
        form - Complete IFFFORM_BTCLASSDATA (GCPD) FORM, or NULL to store
            empty class data.

    RESULT
        TRUE if the requested update is accepted; FALSE on failure.

    NOTES
        For a non-NULL form, supply a complete, valid IFF buffer with consistent nested lengths
        and a padding byte after each odd-length chunk. No buffer length
        is passed to this API; malformed input is not safely rejected.
        The buffer must not alias storage in the destination tree.

        Owner and FORM data are copied. NULL clears data rather than removing
        the owner entry. Updates are not transactional: a failed replacement
        can leave partial data and fall back to creating another owner
        entry. Call from task context. Change detection runs before return;
        no preference file is written.

    SEE ALSO
        btGetClsCfg(), btReadCfg(), btSaveCfgToDisk()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btGetClsCfg(

    SYNOPSIS
        STRPTR owner)

    FUNCTION
        Find private class data for the first matching owner entry.

    INPUTS
        owner - Non-NULL, NUL-terminated class owner name. Case-sensitive.

    RESULT
        Borrowed GCPD context, or NULL if the owner or its data is absent.

    NOTES
        The context is borrowed, not a snapshot. Do not free it directly.
        Prevent concurrent removal or replacement while using it; internal
        locking does not extend its lifetime after this call returns.
        The search does not continue past a matching owner whose data FORM
        is missing. Call from task context.

    SEE ALSO
        btSetClsCfg(), btWriteCfg(), btGetCfgChunk()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        BOOL btSetDevCfg(

    SYNOPSIS
        STRPTR owner, STRPTR devid, STRPTR svcid, APTR form)

    FUNCTION
        Store owner-specific configuration for a device or one of its
        services, creating missing containers as needed.

    INPUTS
        owner - Non-NULL, NUL-terminated class owner name.
        devid - Non-NULL, NUL-terminated device ID.
        svcid - Service ID, or NULL for device-wide configuration.
        form - Complete DCPD FORM for device-wide data or SCPD FORM for
            service data; NULL stores empty data. Names and IDs are
            case-sensitive.

    RESULT
        TRUE if the selected data was replaced or cleared; FALSE on failure.

    NOTES
        For a non-NULL form, supply a complete, valid IFF buffer with consistent nested lengths
        and a padding byte after each odd-length chunk. No buffer length
        is passed to this API; malformed input is not safely rejected.
        The buffer must not alias storage in the destination tree.

        Strings and FORM data are copied. NULL clears the data, retaining
        its containers. Failures can leave partial containers or duplicate
        device entries; prior data is not restored. Call from task context.
        Change detection runs before return. This does not bind a class or
        write a preference file.

    SEE ALSO
        btGetDevCfg(), btReadCfg(), btSetForcedBinding()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        APTR btGetDevCfg(

    SYNOPSIS
        STRPTR owner, STRPTR devid, STRPTR svcid)

    FUNCTION
        Find an owner-specific data FORM under the first matching device
        entry, optionally selecting a service within that device.

    INPUTS
        owner - Non-NULL, NUL-terminated class owner name.
        devid - Non-NULL, NUL-terminated device ID.
        svcid - Non-NULL service ID, or NULL for device-wide data.
            Comparisons are case-sensitive.

    RESULT
        Borrowed DCPD or SCPD context, or NULL if the selected data is absent.

    NOTES
        The context is borrowed, not a snapshot. Do not free it directly.
        Prevent concurrent removal or replacement while using it; internal
        locking does not extend its lifetime after this call returns.
        Later duplicate device entries are not searched. Call from task
        context; no context is created by this query.

    SEE ALSO
        btSetDevCfg(), btGetCfgChunk(), btWriteCfg()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        BOOL btSetForcedBinding(

    SYNOPSIS
        STRPTR owner, STRPTR devid, STRPTR svcid)

    FUNCTION
        Set or clear the preferred class owner recorded for a device or
        service. This changes configuration, not an active binding.

    INPUTS
        owner - Class owner name; NULL or an empty string clears the choice.
        devid - Non-NULL, NUL-terminated device ID.
        svcid - Service ID, or NULL for a device-wide choice.
            Non-NULL strings must be terminated; matching is case-sensitive.

    RESULT
        TRUE if a choice was stored, or for clearing even if none existed.
        FALSE if storing a choice fails.

    NOTES
        Strings are copied. For a service, all matching service containers
        under the first matching device are visited. A successful update
        of one container can mask a failure in another. Creation failures
        can leave partial containers or duplicate device entries.

        Call from task context. Change detection runs before return; no
        file is saved and no class is immediately rebound.

    SEE ALSO
        btGetForcedBinding(), btClassScan(), btSaveCfgToDisk()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        STRPTR btGetForcedBinding(

    SYNOPSIS
        STRPTR devid, STRPTR svcid)

    FUNCTION
        Find the recorded class owner for a device or service. Search only
        the first matching device entry.

    INPUTS
        devid - Non-NULL, NUL-terminated device ID.
        svcid - Service ID, or NULL for the device-wide choice.
            Comparisons are case-sensitive.

    RESULT
        Borrowed owner string, or NULL if no matching choice exists.

    NOTES
        Do not modify or free the string. Copy it while configuration
        changes are excluded if it must survive another configuration call.
        Chunk replacement, buffer growth or form removal can invalidate it.
        Internal locking ends before return. A service query does not fall
        back to the device-wide choice. Call from task context.

    SEE ALSO
        btSetForcedBinding(), btCopyStr()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        BOOL btAddStringChunk(

    SYNOPSIS
        APTR bic, ULONG chunkid, CONST_STRPTR contents)

    FUNCTION
        Copy a terminated string into a plain chunk, replacing the first
        chunk with the same ID. The stored length includes the terminator.

    INPUTS
        bic - Live context; NULL is not a root shorthand here.
        chunkid - Plain-chunk ID in host byte order; do not use ID_FORM.
        contents - Non-NULL, NUL-terminated string.

    RESULT
        TRUE if stored; FALSE if allocation fails.

    NOTES
        The string is copied before replacement and is not retained.
        Failure after replacement begins can remove the old chunk. Call
        from task context. This function does not request change detection,
        apply the configuration or save it to disk.

    BUGS
        Replacing an existing chunk can reach the overlapping-copy defect
        described under btRemCfgChunk().

    SEE ALSO
        btGetStringChunk(), btMatchStringChunk(), btAddCfgEntry()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        BOOL btMatchStringChunk(

    SYNOPSIS
        APTR bic, ULONG chunkid, CONST_STRPTR contents)

    FUNCTION
        Compare a string with the first matching plain chunk in a context.
        Comparison is case-sensitive and bounded by the stored chunk length.

    INPUTS
        bic - Live context; NULL is invalid.
        chunkid - Chunk ID in host byte order.
        contents - Non-NULL, NUL-terminated comparison string.

    RESULT
        TRUE if the strings match; FALSE for a missing chunk or a mismatch.

    NOTES
        The comparison stops at the stored NUL or the end of the payload.
        It also accepts an unterminated payload when its full byte sequence
        matches the comparison string. No data is retained or allocated.
        Call from task context.

    SEE ALSO
        btAddStringChunk(), btGetStringChunk()

*****************************************************************************/
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
/*****************************************************************************
    NAME
        STRPTR btGetStringChunk(

    SYNOPSIS
        APTR bic, ULONG chunkid)

    FUNCTION
        Copy the payload of the first matching plain chunk into a new
        string buffer, adding a trailing NUL byte.

    INPUTS
        bic - Live context; NULL is invalid.
        chunkid - Chunk ID in host byte order.

    RESULT
        Allocated string, or NULL if the chunk is absent or allocation
        fails. Release it with btFreeVec().

    NOTES
        Copies the complete payload, including bytes after an embedded NUL.
        An ordinary string consumer sees only the prefix before that NUL.
        The copy is independent of the configuration tree. Keep the library
        open until the buffer is freed. Call from task context.

    SEE ALSO
        btGetCfgChunk(), btMatchStringChunk(), btFreeVec()

*****************************************************************************/
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

