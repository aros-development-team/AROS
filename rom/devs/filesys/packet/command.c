/*
 * packet.handler - Proxy filesystem for DOS packet handlers
 *
 * Copyright © 2007-2008 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include "packet.h"

#if defined(DEBUG) && DEBUG != 0
#define fsa_str(cmd) ( \
    cmd == FSA_OPEN            ? "FSA_OPEN"            : \
    cmd == FSA_CLOSE           ? "FSA_CLOSE"           : \
    cmd == FSA_READ            ? "FSA_READ"            : \
    cmd == FSA_WRITE           ? "FSA_WRITE"           : \
    cmd == FSA_SEEK            ? "FSA_SEEK"            : \
    cmd == FSA_SET_FILE_SIZE   ? "FSA_SET_FILE_SIZE"   : \
    cmd == FSA_WAIT_CHAR       ? "FSA_WAIT_CHAR"       : \
    cmd == FSA_FILE_MODE       ? "FSA_FILE_MODE"       : \
    cmd == FSA_IS_INTERACTIVE  ? "FSA_IS_INTERACTIVE"  : \
    cmd == FSA_SAME_LOCK       ? "FSA_SAME_LOCK"       : \
    cmd == FSA_EXAMINE         ? "FSA_EXAMINE"         : \
    cmd == FSA_EXAMINE_NEXT    ? "FSA_EXAMINE_NEXT"    : \
    cmd == FSA_EXAMINE_ALL     ? "FSA_EXAMINE_ALL"     : \
    cmd == FSA_EXAMINE_ALL_END ? "FSA_EXAMINE_ALL_END" : \
    cmd == FSA_OPEN_FILE       ? "FSA_OPEN_FILE"       : \
    cmd == FSA_CREATE_DIR      ? "FSA_CREATE_DIR"      : \
    cmd == FSA_CREATE_HARDLINK ? "FSA_CREATE_HARDLINK" : \
    cmd == FSA_CREATE_SOFTLINK ? "FSA_CREATE_SOFTLINK" : \
    cmd == FSA_RENAME          ? "FSA_RENAME"          : \
    cmd == FSA_READ_SOFTLINK   ? "FSA_READ_SOFTLINK"   : \
    cmd == FSA_DELETE_OBJECT   ? "FSA_DELETE_OBJECT"   : \
    cmd == FSA_SET_COMMENT     ? "FSA_SET_COMMENT"     : \
    cmd == FSA_SET_PROTECT     ? "FSA_SET_PROTECT"     : \
    cmd == FSA_SET_OWNER       ? "FSA_SET_OWNER"       : \
    cmd == FSA_SET_DATE        ? "FSA_SET_DATE"        : \
    cmd == FSA_IS_FILESYSTEM   ? "FSA_IS_FILESYSTEM"   : \
    cmd == FSA_MORE_CACHE      ? "FSA_MORE_CACHE"      : \
    cmd == FSA_FORMAT          ? "FSA_FORMAT"          : \
    cmd == FSA_MOUNT_MODE      ? "FSA_MOUNT_MODE"      : \
    cmd == FSA_INHIBIT         ? "FSA_INHIBIT"         : \
    cmd == FSA_ADD_NOTIFY      ? "FSA_ADD_NOTIFY"      : \
    cmd == FSA_REMOVE_NOTIFY   ? "FSA_REMOVE_NOTIFY"   : \
    cmd == FSA_DISK_INFO       ? "FSA_DISK_INFO"       : \
    cmd == FSA_CHANGE_SIGNAL   ? "FSA_CHANGE_SIGNAL"   : \
    cmd == FSA_LOCK_RECORD     ? "FSA_LOCK_RECORD"     : \
    cmd == FSA_UNLOCK_RECORD   ? "FSA_UNLOCK_RECORD"   : \
    cmd == FSA_PARENT_DIR      ? "FSA_PARENT_DIR"      : \
    cmd == FSA_PARENT_DIR_POST ? "FSA_PARENT_DIR_POST" : \
    cmd == FSA_CONSOLE_MODE    ? "FSA_CONSOLE_MODE"    : \
    cmd == FSA_RELABEL         ? "FSA_RELABEL"         : \
                                 "unknown")

#define act_str(cmd) ( \
    cmd == ACTION_NIL             ? "ACTION_NIL"             : \
    cmd == ACTION_STARTUP         ? "ACTION_STARTUP"         : \
    cmd == ACTION_GET_BLOCK       ? "ACTION_GET_BLOCK"       : \
    cmd == ACTION_SET_MAP         ? "ACTION_SET_MAP"         : \
    cmd == ACTION_DIE             ? "ACTION_DIE"             : \
    cmd == ACTION_EVENT           ? "ACTION_EVENT"           : \
    cmd == ACTION_CURRENT_VOLUME  ? "ACTION_CURRENT_VOLUME"  : \
    cmd == ACTION_LOCATE_OBJECT   ? "ACTION_LOCATE_OBJECT"   : \
    cmd == ACTION_RENAME_DISK     ? "ACTION_RENAME_DISK"     : \
    cmd == ACTION_FREE_LOCK       ? "ACTION_FREE_LOCK"       : \
    cmd == ACTION_DELETE_OBJECT   ? "ACTION_DELETE_OBJECT"   : \
    cmd == ACTION_RENAME_OBJECT   ? "ACTION_RENAME_OBJECT"   : \
    cmd == ACTION_MORE_CACHE      ? "ACTION_MORE_CACHE"      : \
    cmd == ACTION_COPY_DIR        ? "ACTION_COPY_DIR"        : \
    cmd == ACTION_WAIT_CHAR       ? "ACTION_WAIT_CHAR"       : \
    cmd == ACTION_SET_PROTECT     ? "ACTION_SET_PROTECT"     : \
    cmd == ACTION_CREATE_DIR      ? "ACTION_CREATE_DIR"      : \
    cmd == ACTION_EXAMINE_OBJECT  ? "ACTION_EXAMINE_OBJECT"  : \
    cmd == ACTION_EXAMINE_NEXT    ? "ACTION_EXAMINE_NEXT"    : \
    cmd == ACTION_DISK_INFO       ? "ACTION_DISK_INFO"       : \
    cmd == ACTION_INFO            ? "ACTION_INFO"            : \
    cmd == ACTION_FLUSH           ? "ACTION_FLUSH"           : \
    cmd == ACTION_SET_COMMENT     ? "ACTION_SET_COMMENT"     : \
    cmd == ACTION_PARENT          ? "ACTION_PARENT"          : \
    cmd == ACTION_TIMER           ? "ACTION_TIMER"           : \
    cmd == ACTION_INHIBIT         ? "ACTION_INHIBIT"         : \
    cmd == ACTION_DISK_TYPE       ? "ACTION_DISK_TYPE"       : \
    cmd == ACTION_DISK_CHANGE     ? "ACTION_DISK_CHANGE"     : \
    cmd == ACTION_SET_DATE        ? "ACTION_SET_DATE"        : \
    cmd == ACTION_SAME_LOCK       ? "ACTION_SAME_LOCK"       : \
    cmd == ACTION_WRITE           ? "ACTION_WRITE"           : \
    cmd == ACTION_READ            ? "ACTION_READ"            : \
    cmd == ACTION_SCREEN_MODE     ? "ACTION_SCREEN_MODE"     : \
    cmd == ACTION_CHANGE_SIGNAL   ? "ACTION_CHANGE_SIGNAL"   : \
    cmd == ACTION_READ_RETURN     ? "ACTION_READ_RETURN"     : \
    cmd == ACTION_WRITE_RETURN    ? "ACTION_WRITE_RETURN"    : \
    cmd == ACTION_FINDUPDATE      ? "ACTION_FINDUPDATE"      : \
    cmd == ACTION_FINDINPUT       ? "ACTION_FINDINPUT"       : \
    cmd == ACTION_FINDOUTPUT      ? "ACTION_FINDOUTPUT"      : \
    cmd == ACTION_END             ? "ACTION_END"             : \
    cmd == ACTION_SEEK            ? "ACTION_SEEK"            : \
    cmd == ACTION_FORMAT          ? "ACTION_FORMAT"          : \
    cmd == ACTION_MAKE_LINK       ? "ACTION_MAKE_LINK"       : \
    cmd == ACTION_SET_FILE_SIZE   ? "ACTION_SET_FILE_SIZE"   : \
    cmd == ACTION_WRITE_PROTECT   ? "ACTION_WRITE_PROTECT"   : \
    cmd == ACTION_READ_LINK       ? "ACTION_READ_LINK"       : \
    cmd == ACTION_FH_FROM_LOCK    ? "ACTION_FH_FROM_LOCK"    : \
    cmd == ACTION_IS_FILESYSTEM   ? "ACTION_IS_FILESYSTEM"   : \
    cmd == ACTION_CHANGE_MODE     ? "ACTION_CHANGE_MODE"     : \
    cmd == ACTION_COPY_DIR_FH     ? "ACTION_COPY_DIR_FH"     : \
    cmd == ACTION_PARENT_FH       ? "ACTION_PARENT_FH"       : \
    cmd == ACTION_EXAMINE_ALL     ? "ACTION_EXAMINE_ALL"     : \
    cmd == ACTION_EXAMINE_FH      ? "ACTION_EXAMINE_FH"      : \
    cmd == ACTION_EXAMINE_ALL_END ? "ACTION_EXAMINE_ALL_END" : \
    cmd == ACTION_SET_OWNER       ? "ACTION_SET_OWNER"       : \
    cmd == ACTION_LOCK_RECORD     ? "ACTION_LOCK_RECORD"     : \
    cmd == ACTION_FREE_RECORD     ? "ACTION_FREE_RECORD"     : \
    cmd == ACTION_ADD_NOTIFY      ? "ACTION_ADD_NOTIFY"      : \
    cmd == ACTION_REMOVE_NOTIFY   ? "ACTION_REMOVE_NOTIFY"   : \
    cmd == ACTION_SERIALIZE_DISK  ? "ACTION_SERIALIZE_DISK"  : \
                                    "unknown")
#endif

static BSTR mkbstr(APTR pool, STRPTR str) {
    UBYTE *buf;
    UBYTE len;

    len = strlen(str) & 0xff;

    buf = AllocPooled(pool, len + 1);
    CopyMem(str, &(buf[1]), len);
    buf[0] = len;

    return (BSTR) MKBADDR(buf);
}

static STRPTR mkcstr(APTR pool, BSTR bstr) {
    UBYTE *str = BADDR(bstr);
    UBYTE *buf;
    UBYTE len = str[0];

    buf = AllocPooled(pool, len + 1);
    CopyMem(&(str[1]), buf, len);
    buf[len] = 0;

    return buf;
}

static struct ph_packet *packet_alloc(void) {
    APTR pool;
    struct ph_packet *pkt;

    pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 1024, 256);
    if (pool == NULL) {
        SetIoErr(ERROR_NO_FREE_STORE);
        return NULL;
    }

    pkt = AllocPooled(pool, sizeof(struct ph_packet));

    pkt->dp.dp_Link = (struct Message *) pkt;
    pkt->msg.mn_Node.ln_Name = (char *) &(pkt->dp);

    pkt->pool = pool;

    return pkt;
}

void packet_handle_request(struct IOFileSys *iofs, struct PacketBase *PacketBase) {
    struct ph_handle *handle;
    struct ph_packet *pkt;
    struct DosPacket *dp;

    D(bug("[packet] got io request %d (%s)\n", iofs->IOFS.io_Command, fsa_str(iofs->IOFS.io_Command)));

    /* get our data back */
    handle = (struct ph_handle *) iofs->IOFS.io_Unit;

    /* make a fresh new packet */
    pkt = packet_alloc();
    dp = &(pkt->dp);

    /* hook the iofs up to the packet so we can find it on return
     * dp_Arg7 should be unused; DoPkt() doesn't touch it */
    dp->dp_Arg7 = (IPTR) iofs;

    /* our reply port will cause packet_reply() to be called when they reply */
    dp->dp_Port = &(handle->mount->reply_port);

    /* convert the command */
    switch (iofs->IOFS.io_Command) {

        case FSA_OPEN:
            D(bug("[packet] OPEN: lock 0x%08x (%s) name '%s' type %s\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_OPEN.io_Filename,
                (iofs->io_Union.io_OPEN.io_FileMode & FMF_LOCK) ? "EXCLUSIVE" : "SHARED"));

            /* 
             * NameFromLock() can call FSA_OPEN with a handle to a file rather
             * than a directory. That seems like a bug, but it doesn't affect
             * existing handlers because they naively concat the lock name and
             * the file name, then look backwards through the full name
             * looking for '/' and going up the tree based on that.
             * FATFileSystem instead checks a flag inside the lock structure
             * to see if the lock is a directory, and fails outright if it's
             * not.
             *
             * Here we intercept this special case and explicitly request the
             * current/parent directory. Unfortunately ACTION_PARENT can't
             * take a lock parameter - it always returns a shared lock. That's
             * sufficient for this case but is technically incorrect. The
             * real solution is for something other than FSA_OPEN to be used
             * to do this.
             */ 
            if (iofs->io_Union.io_OPEN.io_Filename[0] == '/' &&
                iofs->io_Union.io_OPEN.io_Filename[1] == '\0') {

                /* if they asked for the parent of the root, give it to them */
                if (handle == &(handle->mount->root_handle)) {
                    iofs->IOFS.io_Unit = (struct Unit *) &(handle->mount->root_handle);
                    goto reply;
                }

                dp->dp_Type = ACTION_PARENT;
                dp->dp_Arg1 = (IPTR) (handle->is_lock ? handle->actual : NULL);
            }

            else {
                dp->dp_Type = ACTION_LOCATE_OBJECT;
                dp->dp_Arg1 = (IPTR) (handle->is_lock ? handle->actual : NULL);
                dp->dp_Arg2 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_OPEN.io_Filename);
                dp->dp_Arg3 = (iofs->io_Union.io_OPEN.io_FileMode & FMF_LOCK) ? EXCLUSIVE_LOCK : SHARED_LOCK;
            }

            break;

        case FSA_OPEN_FILE: {
            ULONG mode = iofs->io_Union.io_OPEN_FILE.io_FileMode;
            struct FileHandle *fh;

            D(bug("[packet] OPEN_FILE: lock 0x%08x (%s) name '%s' mode 0x%x prot 0x%x\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_OPEN_FILE.io_Filename,
                mode,
                iofs->io_Union.io_OPEN_FILE.io_Protection));

            /* convert modes to the proper packet type (as best we can) */
            if ((mode & FMF_CLEAR) != 0)
                dp->dp_Type = ACTION_FINDOUTPUT;
            else if ((mode & FMF_CREATE) != 0)
                dp->dp_Type = ACTION_FINDUPDATE;
            else
                dp->dp_Type = ACTION_FINDINPUT;
            if ((mode & FMF_APPEND) != 0) {
                iofs->io_DosError = ERROR_BAD_NUMBER;
                goto reply;
            }

            /* make a new filehandle */
            fh = (struct FileHandle *) AllocMem(sizeof(struct FileHandle), MEMF_PUBLIC | MEMF_CLEAR);
            if (fh == NULL) {
                iofs->io_DosError = ERROR_NO_FREE_STORE;
                goto reply;
            }

            /* dos.lib buffer stuff, must be initialised this way */
            fh->fh_Pos = fh->fh_End = (UBYTE *) -1;

            dp->dp_Arg1 = (IPTR) MKBADDR(fh);
            dp->dp_Arg2 = (IPTR) (handle->is_lock ? handle->actual : NULL);
            dp->dp_Arg3 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_OPEN.io_Filename);

            break;
        }

        case FSA_CLOSE:
            D(bug("[packet] CLOSE: lock 0x%08x (%s)\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle")));

            /* if this is the root handle, then we previously intercepted a
             * call and returned it (eg FSA_OPEN/ACTION_PARENT), so we don't
             * want the handler to do anything */
            if (handle == &(handle->mount->root_handle)) {
                iofs->IOFS.io_Unit = NULL;
                goto reply;
            }

            dp->dp_Type = (handle->is_lock) ? ACTION_FREE_LOCK : ACTION_END;
            dp->dp_Arg1 = (IPTR) handle->actual;
            break;

        case FSA_READ:
            D(bug("[packet] READ: handle 0x%08x buf 0x%08x len %ld\n",
                handle->actual,
                iofs->io_Union.io_READ.io_Buffer,
                iofs->io_Union.io_READ.io_Length));

            dp->dp_Type = ACTION_READ;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) iofs->io_Union.io_READ.io_Buffer;
            dp->dp_Arg3 = (IPTR) iofs->io_Union.io_READ.io_Length;

            /* DOSFALSE == 0, so we can't distinguish between a zero-length
             * read and an actual error. So, we reset the length here. If the
             * returned packet is DOSFALSE, but no error, this will make sure
             * DOS gets the right length back */
            iofs->io_Union.io_READ.io_Length = 0;
            break;

        case FSA_WRITE:
            D(bug("[packet] WRITE: handle 0x%08x buf 0x%08x len %ld\n",
                handle->actual,
                iofs->io_Union.io_WRITE.io_Buffer,
                iofs->io_Union.io_WRITE.io_Length));

            dp->dp_Type = ACTION_WRITE;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) iofs->io_Union.io_WRITE.io_Buffer;
            dp->dp_Arg3 = (IPTR) iofs->io_Union.io_WRITE.io_Length;

            iofs->io_Union.io_WRITE.io_Length = 0;
            break;

        case FSA_SEEK:
#if defined(DEBUG) && DEBUG != 0
        {
            ULONG mode = iofs->io_Union.io_SEEK.io_SeekMode;

            bug("[packet] SEEK: handle 0x%08x offset %ld mode %ld (%s)\n",
                handle->actual,
                (LONG) iofs->io_Union.io_SEEK.io_Offset,
                mode,
                mode == OFFSET_BEGINNING ? "OFFSET_BEGINNING" :
                mode == OFFSET_CURRENT   ? "OFFSET_CURRENT"   :
                mode == OFFSET_END       ? "OFFSET_END"       :
                                           "[unknown]");
        }
#endif

            dp->dp_Type = ACTION_SEEK;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) iofs->io_Union.io_SEEK.io_Offset;
            dp->dp_Arg3 = (IPTR) iofs->io_Union.io_SEEK.io_SeekMode;
            break;

        case FSA_SET_FILE_SIZE:
#if defined(DEBUG) && DEBUG != 0
        {
            ULONG mode = iofs->io_Union.io_SET_FILE_SIZE.io_SeekMode;

            bug("[packet] SET_FILE_SIZE: handle 0x%08x offset %ld mode %ld (%s)\n",
                handle->actual,
                (LONG) iofs->io_Union.io_SET_FILE_SIZE.io_Offset,
                mode,
                mode == OFFSET_BEGINNING ? "OFFSET_BEGINNING" :
                mode == OFFSET_CURRENT   ? "OFFSET_CURRENT"   :
                mode == OFFSET_END       ? "OFFSET_END"       :
                                           "[unknown]");
        }
#endif

            dp->dp_Type = ACTION_SET_FILE_SIZE;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) iofs->io_Union.io_SET_FILE_SIZE.io_Offset;
            dp->dp_Arg3 = (IPTR) iofs->io_Union.io_SET_FILE_SIZE.io_SeekMode;
            break;

        case FSA_FILE_MODE: /* XXX untested */
            D(bug("[packet] FILE_MODE: lock 0x%08x (%s) mode 0x%x\b\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_FILE_MODE.io_FileMode));

            dp->dp_Type = ACTION_CHANGE_MODE;

            if (handle->is_lock) {
                dp->dp_Arg1 = CHANGE_LOCK;
                dp->dp_Arg3 = (IPTR) iofs->io_Union.io_FILE_MODE.io_FileMode;
            }
            else {
                dp->dp_Arg1 = CHANGE_FH;
                dp->dp_Arg3 = iofs->io_Union.io_FILE_MODE.io_FileMode & FMF_LOCK ? EXCLUSIVE_LOCK : SHARED_LOCK;
            }

            break;

        case FSA_IS_INTERACTIVE:
            /* XXX is there some other way to query this? how does (eg) aos
             * console handler do it? */
            iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = FALSE;
            iofs->io_DosError = 0;
            goto reply;

        case FSA_SAME_LOCK: {
            struct ph_handle *h1, *h2;
            h1 = (struct ph_handle *) iofs->io_Union.io_SAME_LOCK.io_Lock[0];
            h2 = (struct ph_handle *) iofs->io_Union.io_SAME_LOCK.io_Lock[1];

            D(bug("[packet] SAME_LOCK: lock1 0x%08x (%s) lock2 0x%08x (%s)\n",
                h1->actual,
                h1 == &(h1->mount->root_handle) ? "root" : (h1->is_lock ? "lock" : "handle"),
                h2->actual,
                h2 == &(h2->mount->root_handle) ? "root" : (h2->is_lock ? "lock" : "handle")));

            dp->dp_Type = ACTION_SAME_LOCK;
            dp->dp_Arg1 = (IPTR) h1->actual;
            dp->dp_Arg2 = (IPTR) h2->actual;
            break;
        }

        case FSA_EXAMINE: {
            struct FileInfoBlock *fib;

            D(bug("[packet] EXAMINE: lock 0x%08x (%s)\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle")));

            fib = (struct FileInfoBlock *) AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);

            dp->dp_Type = (handle->is_lock) ? ACTION_EXAMINE_OBJECT : ACTION_EXAMINE_FH;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) MKBADDR(fib);
            break;
        }

        case FSA_EXAMINE_NEXT:
            D(bug("[packet] EXAMINE_NEXT: lock 0x%08x (%s) fib 0x%08x\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_EXAMINE_NEXT.io_fib));

            dp->dp_Type = ACTION_EXAMINE_NEXT;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) MKBADDR(iofs->io_Union.io_EXAMINE_NEXT.io_fib);
            break;

        case FSA_CREATE_DIR:
            D(bug("[packet] CREATE_DIR: lock 0x%08x (%s) name '%s'\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_CREATE_DIR.io_Filename));

            dp->dp_Type = ACTION_CREATE_DIR;
            dp->dp_Arg1 = (IPTR) (handle->is_lock ? handle->actual : NULL);
            dp->dp_Arg2 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_CREATE_DIR.io_Filename);
            break;

        case FSA_IS_FILESYSTEM:
            dp->dp_Type = ACTION_IS_FILESYSTEM;
            break;

        case FSA_DISK_INFO:
            dp->dp_Type = ACTION_DISK_INFO;
            dp->dp_Arg1 = (IPTR) MKBADDR(iofs->io_Union.io_INFO.io_Info);
            break;

        case FSA_CREATE_HARDLINK: /* XXX untested */
            D(bug("[packet] CREATE_HARDLINK: lock 0x%08x (%s) name '%s' target '%s'\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_CREATE_HARDLINK.io_Filename,
                iofs->io_Union.io_CREATE_HARDLINK.io_OldFile));

            dp->dp_Type = ACTION_MAKE_LINK;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_CREATE_HARDLINK.io_Filename);
            dp->dp_Arg3 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_CREATE_HARDLINK.io_OldFile);
            dp->dp_Arg4 = LINK_HARD;
            break;

        case FSA_CREATE_SOFTLINK: /* XXX untested */
            D(bug("[packet] CREATE_SOFTLINK: lock 0x%08x (%s) name '%s' target '%s'\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_CREATE_SOFTLINK.io_Filename,
                iofs->io_Union.io_CREATE_SOFTLINK.io_Reference));

            dp->dp_Type = ACTION_MAKE_LINK;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_CREATE_SOFTLINK.io_Filename);
            dp->dp_Arg3 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_CREATE_SOFTLINK.io_Reference);
            dp->dp_Arg4 = LINK_SOFT;
            break;

        case FSA_RENAME:
            D(bug("[packet] RENAME: lock 0x%08x (%s) name '%s' target '%s'\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_RENAME.io_Filename,
                iofs->io_Union.io_RENAME.io_NewName));

            /* XXX the two paths from FSA_RENAME are copied directly from the
             * arguments to rename with no changes, so they may contain volume
             * specifiers, path seperators, etc. both can be calculated
             * relative to the handle. here we just pass them through to the
             * handler as-is, but I'm not sure if that's right. fat.handler at
             * least will do the right thing. this probably needs to be
             * revisited if another packet-based handler is ported */

            dp->dp_Type = ACTION_RENAME_OBJECT;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_RENAME.io_Filename);
            dp->dp_Arg3 = (IPTR) handle->actual;
            dp->dp_Arg4 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_RENAME.io_NewName);
            break;

        case FSA_READ_SOFTLINK: /* XXX untested */
            D(bug("[packet] READ_SOFTLINK: lock 0x%08x (%s)\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle")));

            dp->dp_Type = ACTION_READ_LINK;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = 0; /* XXX we don't have this (path that caused ERROR_IS_SOFT_LINK) */
            dp->dp_Arg3 = (IPTR) iofs->io_Union.io_READ_SOFTLINK.io_Buffer;
            dp->dp_Arg4 = (IPTR) iofs->io_Union.io_READ_SOFTLINK.io_Size;
            break;

        case FSA_DELETE_OBJECT:
            D(bug("[packet] DELETE: lock 0x%08x (%s) name '%s'\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_DELETE_OBJECT.io_Filename));

            dp->dp_Type = ACTION_DELETE_OBJECT;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_DELETE_OBJECT.io_Filename);
            break;

        case FSA_SET_COMMENT:
            D(bug("[packet] SET_COMMENT: lock 0x%08x (%s) name '%s' comment '%s'\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_SET_COMMENT.io_Filename,
                iofs->io_Union.io_SET_COMMENT.io_Comment));

            dp->dp_Type = ACTION_SET_COMMENT;
            dp->dp_Arg1 = 0;
            dp->dp_Arg2 = (IPTR) handle->actual;
            dp->dp_Arg3 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_SET_COMMENT.io_Filename);
            dp->dp_Arg4 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_SET_COMMENT.io_Comment);
            break;
            
        case FSA_SET_PROTECT:
            D(bug("[packet] SET_PROTECT: lock 0x%08x (%s) name '%s' attrs 0x%x\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_SET_PROTECT.io_Filename,
                iofs->io_Union.io_SET_PROTECT.io_Protection));

            dp->dp_Type = ACTION_SET_PROTECT;
            dp->dp_Arg1 = 0;
            dp->dp_Arg2 = (IPTR) handle->actual;
            dp->dp_Arg3 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_SET_PROTECT.io_Filename);
            dp->dp_Arg4 = (IPTR) iofs->io_Union.io_SET_PROTECT.io_Protection;
            break;

        case FSA_SET_OWNER: /* XXX untested */
            D(bug("[packet] SET_OWNER: lock 0x%08x (%s) name '%s' uid 0x%x gid 0x%x\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_SET_OWNER.io_Filename,
                iofs->io_Union.io_SET_OWNER.io_UID,
                iofs->io_Union.io_SET_OWNER.io_GID));

            dp->dp_Type = ACTION_SET_OWNER;
            dp->dp_Arg1 = 0;
            dp->dp_Arg2 = (IPTR) handle->actual;
            dp->dp_Arg3 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_SET_OWNER.io_Filename);
            dp->dp_Arg4 = (IPTR) iofs->io_Union.io_SET_OWNER.io_GID << 16 |
                                 iofs->io_Union.io_SET_OWNER.io_UID;
            break;

        case FSA_SET_DATE: /* XXX untested */
#if defined(DEBUG) && DEBUG != 0
        {
            struct DateTime dt;
            char datestr[LEN_DATSTRING];

            dt.dat_Stamp = iofs->io_Union.io_SET_DATE.io_Date;
            dt.dat_Format = FORMAT_DOS;
            dt.dat_Flags = 0;
            dt.dat_StrDay = NULL;
            dt.dat_StrDate = datestr;
            dt.dat_StrTime = NULL;
            DateToStr(&dt);

            bug("[packet] SET_DATE: lock 0x%08x (%s) name '%s' date '%s'\n",
                handle->actual,
                handle == &(handle->mount->root_handle) ? "root" : (handle->is_lock ? "lock" : "handle"),
                iofs->io_Union.io_SET_DATE.io_Filename,
                datestr);
        }
#endif

            dp->dp_Type = ACTION_SET_DATE;
            dp->dp_Arg1 = 0;
            dp->dp_Arg2 = (IPTR) handle->actual;
            dp->dp_Arg3 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_SET_DATE.io_Filename);
            dp->dp_Arg4 = (IPTR) &iofs->io_Union.io_SET_DATE.io_Date;
            break;

        case FSA_MORE_CACHE: /* XXX untested */
            D(bug("[packet] MORE_CACHE: buffers '0x%x'\n", iofs->io_Union.io_MORE_CACHE.io_NumBuffers));

            dp->dp_Type = ACTION_MORE_CACHE;
            dp->dp_Arg1 = (IPTR) iofs->io_Union.io_MORE_CACHE.io_NumBuffers;
            break;

        case FSA_FORMAT: /* XXX untested */
            D(bug("[packet] FSA_FORMAT: name '%s' type 0x%x\n",
                  iofs->io_Union.io_FORMAT.io_VolumeName,
                  iofs->io_Union.io_FORMAT.io_DosType));

            dp->dp_Type = ACTION_FORMAT;
            dp->dp_Arg1 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_FORMAT.io_VolumeName);
            dp->dp_Arg2 = (IPTR) iofs->io_Union.io_FORMAT.io_DosType;
            break;

        case FSA_INHIBIT: /* XXX untested */
            D(bug("[packet] FSA_INHIBIT: %sinhibit\n", iofs->io_Union.io_INHIBIT.io_Inhibit == 0 ? "un" : ""));

            dp->dp_Type = ACTION_INHIBIT;
            dp->dp_Arg1 = (IPTR) iofs->io_Union.io_INHIBIT.io_Inhibit;
            break;

        case FSA_RELABEL:
            D(bug("[packet] FSA_RELABEL: name '%s'\n", iofs->io_Union.io_RELABEL.io_NewName));

            dp->dp_Type = ACTION_RENAME_DISK;
            dp->dp_Arg1 = (IPTR) mkbstr(pkt->pool, iofs->io_Union.io_RELABEL.io_NewName);
            break;

        case FSA_LOCK_RECORD: /* XXX untested */
#if defined(DEBUG) && DEBUG != 0
        {
            ULONG mode = iofs->io_Union.io_RECORD.io_RecordMode;

            bug("[packet] FSA_LOCK_RECORD: handle 0x%08x offset %ld size %ld mode %d (%s) timeout %d\n",
                handle->actual,
                (LONG) iofs->io_Union.io_RECORD.io_Offset,
                iofs->io_Union.io_RECORD.io_Size,
                mode,
                mode == REC_EXCLUSIVE       ? "REC_EXCLUSIVE"       :
                mode == REC_EXCLUSIVE_IMMED ? "REC_EXCLUSIVE_IMMED" :
                mode == REC_SHARED          ? "REC_SHARED"          :
                mode == REC_SHARED_IMMED    ? "REC_SHARED_IMMED"    :
                                              "[unknown]",
                iofs->io_Union.io_RECORD.io_Timeout);
        }
#endif

            dp->dp_Type = ACTION_LOCK_RECORD;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) iofs->io_Union.io_RECORD.io_Offset;
            dp->dp_Arg3 = (IPTR) iofs->io_Union.io_RECORD.io_Size;
            dp->dp_Arg4 = (IPTR) iofs->io_Union.io_RECORD.io_RecordMode;
            dp->dp_Arg5 = (IPTR) iofs->io_Union.io_RECORD.io_Timeout;
            break;

        case FSA_UNLOCK_RECORD: /* XXX untested */
            D(bug("[packet] FSA_UNLOCK_RECORD: handle 0x%08x offset %ld size %ld\n",
                  handle->actual,
                  (LONG) iofs->io_Union.io_RECORD.io_Offset,
                  iofs->io_Union.io_RECORD.io_Size));

            dp->dp_Type = ACTION_FREE_RECORD;
            dp->dp_Arg1 = (IPTR) handle->actual;
            dp->dp_Arg2 = (IPTR) iofs->io_Union.io_RECORD.io_Offset;
            dp->dp_Arg3 = (IPTR) iofs->io_Union.io_RECORD.io_Size;
            break;

        case FSA_ADD_NOTIFY:
            D(bug("[packet] FSA_ADD_NOTIFY: nr 0x%08x name '%s'\n", 
                  iofs->io_Union.io_NOTIFY.io_NotificationRequest,
                  iofs->io_Union.io_NOTIFY.io_NotificationRequest->nr_FullName));

            dp->dp_Type = ACTION_ADD_NOTIFY;
            dp->dp_Arg1 =
                (SIPTR) iofs->io_Union.io_NOTIFY.io_NotificationRequest;
            break;

        case FSA_REMOVE_NOTIFY:
            D(bug("[packet] FSA_REMOVE_NOTIFY: nr 0x%08x name '%s'\n", 
                  iofs->io_Union.io_NOTIFY.io_NotificationRequest,
                  iofs->io_Union.io_NOTIFY.io_NotificationRequest->nr_FullName));

            dp->dp_Type = ACTION_REMOVE_NOTIFY;
            dp->dp_Arg1 =
                (SIPTR) iofs->io_Union.io_NOTIFY.io_NotificationRequest;
            break;

        /* XXX implement */
        case FSA_EXAMINE_ALL:
        case FSA_EXAMINE_ALL_END:
        case FSA_MOUNT_MODE:
        case FSA_CHANGE_SIGNAL:
        case FSA_PARENT_DIR:
        case FSA_PARENT_DIR_POST:
        case FSA_CONSOLE_MODE:
            D(bug("[packet] command not implemented\n"));
            iofs->io_DosError = ERROR_NOT_IMPLEMENTED;
            goto reply;

        default:
            D(bug("[packet] unknown command\n"));
            iofs->io_DosError = ERROR_ACTION_NOT_KNOWN;
            goto reply;
    }

    D(bug("[packet] converted to %s packet\n", act_str(dp->dp_Type)));

    /* WaitIO() will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;

    /* since these all go to the packet handler process, they can't be done now */
    iofs->IOFS.io_Flags &= ~IOF_QUICK;

    /* send the packet */
    PutMsg(&(handle->mount->process->pr_MsgPort), dp->dp_Link);

    return;

    /* jump here to reply to the packet now, handling IOF_QUICK appropriately */
reply:
    D(bug("[packet] replying directly with error %d\n", iofs->io_DosError));
    
    /* kill the packet */
    DeletePool(pkt->pool);

    /* if they can handle quick replies, just bail out */
    if (iofs->IOFS.io_Flags & IOF_QUICK)
        return;

    /* otherwise tell them properly */
    ReplyMsg((APTR) iofs);
}

AROS_UFH3(void, packet_reply,
          AROS_UFHA(struct ph_mount *, mount,     A1),
          AROS_UFHA(APTR,              dummy,   A5),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {   

    AROS_USERFUNC_INIT

    struct DosPacket *dp;
    struct ph_packet *pkt;
    struct IOFileSys *iofs;
    struct ph_handle *handle;

    /* retrieve the message and fish the packet out */
    dp = (struct DosPacket *) GetMsg(&(mount->reply_port))->mn_Node.ln_Name;
    pkt = (struct ph_packet *) dp->dp_Link;

    D(bug("[packet] got reply packet %d (%s)\n", dp->dp_Type, act_str(dp->dp_Type)));

    /* get the iofs back */
    iofs = (struct IOFileSys *) dp->dp_Arg7;

    /* dos error code comes back in Res2 */
    if (dp->dp_Res1 == DOSFALSE) {
        iofs->io_DosError = dp->dp_Res2;

        /* do any cleanup from the request (eg freeing memory) */
        switch (dp->dp_Type) {
            case ACTION_FINDINPUT:
            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE:
                FreeMem((APTR) BADDR(dp->dp_Arg1), sizeof(struct FileHandle));
                break;

            case ACTION_SAME_LOCK:
                /* DOSFALSE & no error means the locks are different */
                if (iofs->io_DosError == 0)
                    iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_DIFFERENT;
                break;

            case ACTION_PARENT:
                /* no error means they tried to go up past the root. The
                 * packet system allows this, IOFS does not */
                if (iofs->io_DosError == 0)
                    iofs->io_DosError = ERROR_OBJECT_NOT_FOUND;
                break;

            /* a zero result is not an error for the following two packet
             * types. We shouldn't really be here */
            case ACTION_SEEK:
                iofs->io_Union.io_SEEK.io_Offset = dp->dp_Res1;
            case ACTION_SET_FILE_SIZE:
                iofs->io_DosError = 0;
                break;
        }

        /* kill the packet */
        DeletePool(pkt->pool);

        D(bug("[packet] returning error %ld\n", iofs->io_DosError));

        /* and tell them */
        ReplyMsg((APTR) iofs);

        return;
    }

    /* no error */
    iofs->io_DosError = 0;
    
    /* populate the iofs with the results. note that for packets that only
     * return success/failure we have nothing to do, so they're not listed here */
    switch (dp->dp_Type) {

        case ACTION_LOCATE_OBJECT:
        case ACTION_PARENT:
            handle = (struct ph_handle *) AllocMem(sizeof(struct ph_handle), MEMF_PUBLIC | MEMF_CLEAR);
            if (handle == NULL) {
                iofs->io_DosError = ERROR_NO_FREE_STORE;
                break;
            }

            /* we'll need the lock they gave us for future operations */
            handle->actual = (void *) dp->dp_Res1;
            handle->is_lock = TRUE;
            handle->mount = mount;

            iofs->IOFS.io_Unit = (struct Unit *) handle;

            break;

        case ACTION_FINDINPUT:
        case ACTION_FINDOUTPUT:
        case ACTION_FINDUPDATE: {
            struct FileHandle *fh = (struct FileHandle *) BADDR(dp->dp_Arg1);

            /* XXX this is wrong. if we can't get the memory, we still have an
             * open file which gets leaked. this handle needs to be allocated
             * before the call goes out to the handler, or we need to schedule
             * ACTION_END to clean up the file */
            handle = (struct ph_handle *) AllocMem(sizeof(struct ph_handle), MEMF_PUBLIC | MEMF_CLEAR);
            if (handle == NULL) {
                iofs->io_DosError = ERROR_NO_FREE_STORE;
                FreeMem((APTR) fh, sizeof(struct FileHandle));
                break;
            }

            /* handlers return "internal data" (typically a lock, though we
             * can't assume that) in fh_Arg1. we need to keep it for later
             * filehandle operations. the filehandle itself is expendable -
             * the calls that need this data (eg ACTION_READ/WRITE/SEEK) take
             * it directly in dp_Arg1 */
            handle->actual = (void *) fh->fh_Arg1;
            handle->is_lock = FALSE;
            handle->mount = mount;

            iofs->IOFS.io_Unit = (struct Unit *) handle;

            FreeMem((APTR) fh, sizeof(struct FileHandle));

            break;
        }

        case ACTION_FREE_LOCK:
        case ACTION_END:
            /* free up our data */
            handle = (struct ph_handle *) iofs->IOFS.io_Unit;
            FreeMem((APTR) handle, sizeof(struct ph_handle));
            iofs->IOFS.io_Unit = NULL;
            break;

        case ACTION_READ:
            iofs->io_Union.io_READ.io_Length = dp->dp_Res1;
            break;

        case ACTION_WRITE:
            iofs->io_Union.io_WRITE.io_Length = dp->dp_Res1;
            break;

        case ACTION_SEEK:
            if (dp->dp_Res1 == -1)
                iofs->io_DosError = dp->dp_Res2;
            else
                iofs->io_Union.io_SEEK.io_Offset = dp->dp_Res1;
            break;

        case ACTION_SET_FILE_SIZE:
            if (dp->dp_Res1 == -1)
                iofs->io_DosError = dp->dp_Res2;
            break;

        case ACTION_SAME_LOCK:
            iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_SAME;
            break;

        case ACTION_EXAMINE_OBJECT:
        case ACTION_EXAMINE_FH: {
            struct FileInfoBlock *fib = (struct FileInfoBlock *) BADDR(dp->dp_Arg2);
            struct ExAllData *ead = iofs->io_Union.io_EXAMINE.io_ead;
            ULONG size = iofs->io_Union.io_EXAMINE.io_Size;
            ULONG mode = iofs->io_Union.io_EXAMINE.io_Mode;
            ULONG comment_len = 0, filename_len = 0;

            iofs->io_DirPos = fib->fib_DiskKey;

            /* make sure we have enough room for everything that came back */
            if (size < sizeof(struct ExAllData) +
                       (mode >= ED_COMMENT ? (comment_len = fib->fib_Comment[0]) : 0) +
                       (mode >= ED_NAME    ? (filename_len = fib->fib_FileName[0]) : 0)) {
                iofs->io_DosError = ERROR_BUFFER_OVERFLOW;
                FreeMem(fib, sizeof(struct FileInfoBlock));
                break;
            }

            /* copy stuff from the fib to the ead */
            switch (mode) {
                case ED_OWNER:
                    ead->ed_OwnerUID = fib->fib_OwnerUID;
                    ead->ed_OwnerGID = fib->fib_OwnerGID;

                case ED_COMMENT:
                    /* store the comment in the spare space after the ead and
                     * the filename */
                    ead->ed_Comment = (UBYTE *) ead + sizeof(struct ExAllData) + filename_len + 1;
                    strcpy(ead->ed_Comment,
                        mkcstr(pkt->pool, fib->fib_Comment));

                case ED_DATE:
                    ead->ed_Days = fib->fib_Date.ds_Days;
                    ead->ed_Mins = fib->fib_Date.ds_Minute;
                    ead->ed_Ticks = fib->fib_Date.ds_Tick;

                case ED_PROTECTION:
                    ead->ed_Prot = fib->fib_Protection;
                    
                case ED_SIZE:
                    ead->ed_Size = fib->fib_Size;

                case ED_TYPE:
                    ead->ed_Type = fib->fib_EntryType;

                case ED_NAME:
                    /* store the name in the spare space after the ead */
                    ead->ed_Name = (UBYTE *) ead + sizeof(struct ExAllData);
                    strcpy(ead->ed_Name, mkcstr(pkt->pool, fib->fib_FileName));
               
                case 0:
                    ead->ed_Next = NULL;
                    break;

                default:
                    iofs->io_DosError = ERROR_BAD_NUMBER;
                    break;
            }

            FreeMem(fib, sizeof(struct FileInfoBlock));

            break;
        }

        case ACTION_EXAMINE_NEXT: {
            struct FileInfoBlock *fib = iofs->io_Union.io_EXAMINE_NEXT.io_fib;
            strcpy(fib->fib_FileName, mkcstr(pkt->pool, fib->fib_FileName));
            strcpy(fib->fib_Comment, mkcstr(pkt->pool, fib->fib_Comment));
            break;
        }

        case ACTION_IS_FILESYSTEM:
            iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem = TRUE;
            break;

        case ACTION_CREATE_DIR:
            handle = (struct ph_handle *) AllocMem(sizeof(struct ph_handle), MEMF_PUBLIC | MEMF_CLEAR);
            if (handle == NULL) {
                iofs->io_DosError = ERROR_NO_FREE_STORE;
                break;
            }

            /* we'll need the lock they gave us for future operations */
            handle->actual = (void *) dp->dp_Res1;
            handle->is_lock = TRUE;
            handle->mount = mount;

            iofs->IOFS.io_Unit = (struct Unit *) handle;

            break;

        case ACTION_READ_LINK:
            iofs->io_Union.io_READ_SOFTLINK.io_Size = dp->dp_Res1;
            break;

        case ACTION_MORE_CACHE:
            iofs->io_Union.io_MORE_CACHE.io_NumBuffers = dp->dp_Res1;
    }

    /* done with the packet */
    DeletePool(pkt->pool);

    /* send it back */
    ReplyMsg((APTR) iofs);

    AROS_USERFUNC_EXIT
}

