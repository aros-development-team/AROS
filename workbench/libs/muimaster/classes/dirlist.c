/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "dirlist_private.h"
#include "locale.h"

/****** Dirlist.mui/MUIC_Dirlist ************
*
*   NAME
*       MUIC_Dirlist
*
*   SUPERCLASS
*       List.mui
*
*   DESCRIPTION
*       Enumerate a DOS directory synchronously into List entries containing
*       copied FileInfoBlock records. Construction installs local construct,
*       destruct and display hooks, then applies Dirlist tags through its
*       setter. A non-NULL Directory tag can therefore perform filesystem I/O
*       before construction returns. Directory and pattern strings and
*       FilterHook are borrowed; list records and the generated Path string are
*       owned by the object.
*       The default display has six columns in this order: name, size or drawer
*       image, DOS date, time, protection flags and comment. The display hook
*       always writes six slots. A caller-supplied List_Format is accepted
*       without adapting the hook; retain a compatible six-column format. Its
*       temporary size/date/time/protection strings are instance buffers
*       overwritten by later display calls. DateToStr status is ignored.
*       Protection display shows script, pure, archive and owner
*       read/write/execute/delete flags, not an exhaustive protection-word
*       display.
*       The constructor replaces the List construction/destruction/display hooks
*       after superclass construction. Do not preload incompatible
*       List_SourceArray entries or replace these hooks while copied directory
*       records are present. Entries retrieved through List methods are borrowed
*       records with a FileInfoBlock as their first member; they become invalid
*       when removed, cleared or disposed.
*       A directory scan allocates one FileInfoBlock, takes a shared lock,
*       examines the target and iterates ExNext only for a positive directory
*       type. Entries with positive fib_DirEntryType are drawers; other types
*       follow the file path. There is no recursion, background worker, UI input
*       pumping, cancellation method or timeout. Filesystem operations and
*       filter callbacks can block the caller.
*
*   METHODS
*       Directory Set triggers a scan even when the pointer or string is
*       unchanged. ReRead scans the current Directory while forcing List_Quiet
*       TRUE and then FALSE. Changing other filters alone does not reread. If
*       the stored Status is Valid, both paths clear the old list and send
*       Invalid plus zero counter tags before reading. Those counter tags do not
*       reset the implementation's stored counters. If Status was not Valid,
*       existing list contents are not cleared.
*       The builtin filtering path applies FilesOnly/DrawersOnly, then
*       RejectIcons, then accept/reject patterns for files and optionally
*       drawers. Matching is case-insensitive and requires compiled DOS
*       patterns. Successful matches are appended in enumeration order. There is
*       no local compare hook or sorting operation implementing the advertised
*       sort attributes.
*       Getters return the stored options, counters and status, including
*       getters omitted by several header access annotations. Unknown queries
*       delegate to List. Disposal frees the last generated Path string, then
*       delegates record and child disposal to List; it never frees borrowed
*       Directory, patterns or FilterHook.
*
*   BUGS
*       With FilterHook non-NULL, its callback is called for each enumerated
*       entry, but insertion and counter updates exist only in the no-hook
*       branch. Returning either TRUE or FALSE from that hook therefore inserts
*       nothing in this implementation. The hook also bypasses all builtin
*       filters.
*       NumFiles and NumDrawers are incremented before insertion, without
*       checking insertion success, and are not reset by the zero tags emitted
*       on reread. They accumulate across scans. NumBytes is never assigned
*       after zero initialization; emitted NumBytes tag values describe
*       individual file sizes rather than a stored total.
*       After the ExNext loop, Status is set to Valid even if enumeration ended
*       with an error other than ERROR_NO_MORE_ENTRIES. Reading is never set
*       automatically. Valid does not prove a complete scan or correct totals.
*       Allocation, lock and Examine failures have no separate returned scan
*       status; a cleared old list is not restored.
*
*/
/****** Dirlist.mui/MUIM_Dirlist_ReRead ************
*
*   NAME
*       MUIM_Dirlist_ReRead
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Dirlist_ReRead);
*
*   FUNCTION
*       Synchronously scan the currently stored Directory again.
*
*   INPUTS
*       obj - Live Dirlist with valid borrowed directory/filter storage and no
*       concurrent mutation or disposal.
*
*   RESULT
*       Always zero. No scan success, completeness or filesystem error is
*       returned.
*
*   NOTES
*       Set List_Quiet TRUE; when Status is Valid, clear entries and send
*       Invalid plus zero counter tags; scan a non-NULL Directory; set Quiet
*       FALSE. The previous Quiet value is not restored. The counter-reset tags
*       are not stored, so counts accumulate and NumBytes remains zero. No
*       Directory means no filesystem scan. Callbacks and notifications can run
*       synchronously; they must not invalidate the instance or scan state. See
*       class filtering and Status limitations.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_Directory ************
*
*   NAME
*       MUIA_Dirlist_Directory
*
*   FUNCTION
*       Select the directory to enumerate or read its stored pointer.
*
*   INPUTS
*       Initialize, set and get, borrowed terminated STRPTR. Default NULL. No
*       copy, canonicalization or ownership transfer.
*
*   NOTES
*       Keep the string readable and unchanged for scans and Path queries until
*       replacement or disposal. Set stores the pointer and schedules one
*       synchronous scan after all tags in that call have been processed, using
*       their final option values. Repeating Directory still scans; multiple
*       Directory tags use the last value.
*       If previous Status is Valid, the old list is cleared first and Status
*       becomes Invalid. NULL skips reading after that conditional clear. Other
*       invalid states do not cause automatic clearing. Get returns the exact
*       borrowed pointer with TRUE, not a normalized filesystem path. Setter
*       return value is inherited from List and does not report scan success.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_AcceptPattern ************
*
*   NAME
*       MUIA_Dirlist_AcceptPattern
*
*   FUNCTION
*       Store a DOS pattern to accept only names matching it during builtin
*       filtering.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. Borrowed compiled pattern buffer from ParsePatternNoCase,
*       or NULL to disable. Default NULL.
*
*   NOTES
*       Dirlist passes the buffer directly to MatchPatternNoCase; it does not
*       parse a wildcard expression or copy its storage. Keep the compiled
*       buffer readable until replacement or disposal. Files are tested; drawers
*       are tested only with FilterDrawers TRUE. A non-NULL FilterHook bypasses
*       this test. Set does not trigger a new scan; use ReRead or Directory Set.
*       Get returns the stored pointer with TRUE.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_RejectPattern ************
*
*   NAME
*       MUIA_Dirlist_RejectPattern
*
*   FUNCTION
*       Store a DOS pattern to exclude names matching it during builtin
*       filtering.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. Borrowed compiled pattern buffer from ParsePatternNoCase,
*       or NULL to disable. Default NULL.
*
*   NOTES
*       Dirlist passes the buffer directly to MatchPatternNoCase; it does not
*       parse a wildcard expression or copy its storage. Keep the compiled
*       buffer readable until replacement or disposal. Files are tested; drawers
*       are tested only with FilterDrawers TRUE. A non-NULL FilterHook bypasses
*       this test. Set does not trigger a new scan; use ReRead or Directory Set.
*       Get returns the stored pointer with TRUE.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_DrawersOnly ************
*
*   NAME
*       MUIA_Dirlist_DrawersOnly
*
*   FUNCTION
*       Exclude entries whose fib_DirEntryType is not positive.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. BOOL, normalized to TRUE/FALSE. Default FALSE.
*
*   NOTES
*       FilesOnly and DrawersOnly both TRUE exclude all entries in the builtin
*       path. A non-NULL FilterHook bypasses the builtin filtering path. Set
*       stores the flag without rescanning; use ReRead or Directory Set. Get
*       returns the stored value with TRUE.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_FilesOnly ************
*
*   NAME
*       MUIA_Dirlist_FilesOnly
*
*   FUNCTION
*       Exclude entries whose fib_DirEntryType is positive.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. BOOL, normalized to TRUE/FALSE. Default FALSE.
*
*   NOTES
*       FilesOnly and DrawersOnly both TRUE exclude all entries in the builtin
*       path. A non-NULL FilterHook bypasses the builtin filtering path. Set
*       stores the flag without rescanning; use ReRead or Directory Set. Get
*       returns the stored value with TRUE.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_FilterDrawers ************
*
*   NAME
*       MUIA_Dirlist_FilterDrawers
*
*   FUNCTION
*       Apply accept/reject patterns to positive-type directory entries as well
*       as files.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. BOOL, normalized to TRUE/FALSE. Default FALSE.
*
*   NOTES
*       This flag affects pattern tests only; drawer/file filtering and
*       RejectIcons are independent. A non-NULL FilterHook bypasses the builtin
*       filtering path. Set stores the flag without rescanning; use ReRead or
*       Directory Set. Get returns the stored value with TRUE.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_RejectIcons ************
*
*   NAME
*       MUIA_Dirlist_RejectIcons
*
*   FUNCTION
*       Exclude names ending in .info, using a case-insensitive suffix
*       comparison.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. BOOL, normalized to TRUE/FALSE. Default FALSE.
*
*   NOTES
*       The suffix test also applies to drawers regardless of FilterDrawers. It
*       does not inspect file contents or require a valid icon datatype. A
*       non-NULL FilterHook bypasses the builtin filtering path. Set stores the
*       flag without rescanning; use ReRead or Directory Set. Get returns the
*       stored value with TRUE.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_FilterHook ************
*
*   NAME
*       MUIA_Dirlist_FilterHook
*
*   FUNCTION
*       Provide a callback for each enumerated directory entry.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. Borrowed struct Hook pointer; default NULL.
*
*   NOTES
*       The call is CallHookPkt(hook, dirlistObject, &exAllData). The temporary
*       zero-initialized ExAllData contains name, type, size, protection,
*       days/minutes/ticks and comment from the current FileInfoBlock. Other
*       fields remain zero. Name/comment pointers and the message are valid only
*       during the call; copy needed data before returning. Hook/object must
*       remain live and callbacks must not mutate or dispose the active scan.
*       A zero hook result skips the entry. A nonzero result also inserts
*       nothing because the insertion code is in the alternative no-hook branch.
*       Thus this implementation does not provide a functioning custom
*       acceptance hook. All builtin filters and counters are bypassed while the
*       hook is installed. Set alone does not rescan; Get returns the stored
*       hook with TRUE.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_MultiSelDirs ************
*
*   NAME
*       MUIA_Dirlist_MultiSelDirs
*
*   FUNCTION
*       Store a directory-multiselection option without changing selection
*       behavior.
*
*   INPUTS
*       Initialize, set and get in this implementation, despite the header
*       omitting get. BOOL normalized to TRUE/FALSE; default FALSE.
*
*   NOTES
*       The field is only stored and returned. No local multiselection test hook
*       uses it, and it does not enable or restrict drawer selection. Actual
*       List selection behavior remains governed by inherited List/Listview
*       options. Get returns TRUE with the stored flag.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_NumFiles ************
*
*   NAME
*       MUIA_Dirlist_NumFiles
*
*   FUNCTION
*       Read the accumulated count of accepted file records encountered by
*       builtin filtering.
*
*   INPUTS
*       Get, LONG in the header; stored as ULONG. Initially zero.
*
*   NOTES
*       The scan increments the field before attempting each matching insertion,
*       emits its value through Set and ignores insertion success. FilterHook
*       scans do not increment it. ReRead and Directory changes emit zero
*       counter tags but the setter has no case to store them; the count
*       therefore accumulates across scans rather than representing the current
*       list. Failed insertions and unsigned overflow are not accounted for. Get
*       returns TRUE. Do not use these values as verified current-directory
*       totals.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_NumDrawers ************
*
*   NAME
*       MUIA_Dirlist_NumDrawers
*
*   FUNCTION
*       Read the accumulated count of accepted drawer records encountered by
*       builtin filtering.
*
*   INPUTS
*       Get, LONG in the header; stored as ULONG. Initially zero.
*
*   NOTES
*       The scan increments the field before attempting each matching insertion,
*       emits its value through Set and ignores insertion success. FilterHook
*       scans do not increment it. ReRead and Directory changes emit zero
*       counter tags but the setter has no case to store them; the count
*       therefore accumulates across scans rather than representing the current
*       list. Failed insertions and unsigned overflow are not accounted for. Get
*       returns TRUE. Do not use these values as verified current-directory
*       totals.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_NumBytes ************
*
*   NAME
*       MUIA_Dirlist_NumBytes
*
*   FUNCTION
*       Read the stored byte-count field, which remains zero in this
*       implementation.
*
*   INPUTS
*       Get, LONG in the header; stored as IPTR. Initialized to zero and never
*       updated by the local setter or scan.
*
*   NOTES
*       For each accepted file the scan emits a NumBytes tag with stored
*       numbytes plus fib_Size. Since numbytes is not stored, these
*       notifications carry individual file sizes, not a cumulative total. Reset
*       tags also do not change storage. Get returns zero with TRUE regardless
*       of listed file sizes. The attribute cannot be used to calculate a
*       reliable directory total.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_Path ************
*
*   NAME
*       MUIA_Dirlist_Path
*
*   FUNCTION
*       Build a path for the active entry when Status is Valid.
*
*   INPUTS
*       Get, borrowed STRPTR result. The getter returns TRUE even when it stores
*       NULL.
*
*   NOTES
*       Every query first frees the previous generated Path. With Valid status
*       and an active List entry, it allocates a buffer, copies Directory and
*       appends fib_FileName with AddPart. No active entry, unsuitable status,
*       allocation failure or AddPart failure yields NULL. Directory and active
*       record must remain valid; the length calculation uses WORD without
*       overflow checking, so the combined string lengths plus three must fit a
*       positive WORD.
*       The returned path is owned by Dirlist. Do not modify or free it; copy it
*       before another Path query or disposal. Directory/selection changes do
*       not update an already returned string automatically. Path creates no
*       lock and does not prove that the named entry still exists.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_SortDirs ************
*
*   NAME
*       MUIA_Dirlist_SortDirs
*
*   FUNCTION
*       Store the requested drawer placement without sorting entries.
*
*   INPUTS
*       Initialize, set and get in this implementation. Declared LONG values:
*       First (0), Last (1), Mix (2). Default zero.
*
*   NOTES
*       The setter reduces every nonzero value to TRUE and stores it in a BYTE;
*       Last and Mix therefore both read back as one. The field is not used by
*       scanning or comparison. Entries are appended in ExNext order. Get
*       returns TRUE, but neither the stored value nor a notification proves
*       sorting occurred.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_SortHighLow ************
*
*   NAME
*       MUIA_Dirlist_SortHighLow
*
*   FUNCTION
*       Store a reverse-sort flag without reversing the list.
*
*   INPUTS
*       Initialize, set and get in this implementation. BOOL normalized to
*       TRUE/FALSE; default FALSE.
*
*   NOTES
*       The flag has no consumer in this class. No scan or setter invokes a
*       sorting operation based on it. Get returns TRUE with the stored flag;
*       inherited explicit List operations are separate.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_SortType ************
*
*   NAME
*       MUIA_Dirlist_SortType
*
*   FUNCTION
*       Store a sort-mode value without applying it.
*
*   INPUTS
*       Initialize, set and get in this implementation. LONG in the header,
*       stored as BYTE. Declared values Name (0), Date (1), Size (2); default
*       zero.
*
*   NOTES
*       No validation, comparison hook or sorting operation uses this value.
*       Large inputs truncate to BYTE. Scan insertion remains in ExNext order.
*       Get returns TRUE with the stored value; do not infer name/date/size
*       ordering.
*
*/
/****** Dirlist.mui/MUIA_Dirlist_Status ************
*
*   NAME
*       MUIA_Dirlist_Status
*
*   FUNCTION
*       Read the stored enumeration status.
*
*   INPUTS
*       Get, LONG in the header, stored as BYTE. Invalid (0) initially; Reading
*       (1) and Valid (2) are declared.
*
*   NOTES
*       This implementation never emits Reading automatically. It marks Invalid
*       when clearing a previously Valid list, and Valid after entering and
*       leaving the ExNext loop, including an error exit. Allocation, Lock and
*       Examine failures do not independently publish an error result. A partial
*       scan or a FilterHook scan that inserted nothing can still be Valid.
*       An internal setter stores this tag and ordinary Set can reach it despite
*       the get-only header. Do not set it to manufacture success: list clearing
*       and Path generation depend on this field. Get returns TRUE with the
*       stored value. Valid is not evidence of complete enumeration, correct
*       totals or an existing active path.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Copy one readable Dirlist_Entry/FileInfoBlock into the supplied live List
 * pool; return owned pooled pointer or NULL on allocation failure. Input is
 * borrowed for the call only; no NULL check or deep external-pointer copy.
 * Pair returned allocation with this class destructor and same pool.
 */
AROS_UFH3S(APTR, construct_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Dirlist_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct Dirlist_Entry *new;

    if ((new = AllocPooled(pool, sizeof(*new))))
    {
        *new = *entry;
    }

    return new;

    AROS_USERFUNC_EXIT
}

/*
 * Free a previously constructed Dirlist_Entry from its matching live List
 * pool with exact structure size. Requires valid allocation pointer and no
 * retained users; no NULL guard, return value or ownership retained.
 */
AROS_UFH3S(void, destruct_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Dirlist_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct Dirlist_Entry));

    AROS_USERFUNC_EXIT
}

/*
 * Populate six writable List display slots from borrowed record or
 * localized headers for NULL entry. Use instance buffers for
 * size/date/time/protection, overwritten on next call; DateToStr status
 * ignored. Return zero. Requires live instance and compatible six-column
 * layout; retain no displayed pointers beyond their source lifetime.
 */
AROS_UFH3S(LONG, display_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct Dirlist_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct Dirlist_DATA *data = hook->h_Data;
    struct DateTime dt;

    /* MUI: name | size | Date  | Time  | Protection | Comment */
    if (entry)
    {
        *array++ = entry->fib.fib_FileName;

        if (entry->fib.fib_DirEntryType > 0)
        {
            *array++ = "\33I[6:22]";
        }
        else
        {
            snprintf(data->size_string, sizeof(data->size_string), "%ld",
                (long)entry->fib.fib_Size);
            *array++ = data->size_string;
        }

        dt.dat_Stamp = entry->fib.fib_Date;
        dt.dat_Format = FORMAT_DOS;
        dt.dat_Flags = 0;
        dt.dat_StrDay = NULL;
        dt.dat_StrDate = data->date_string;
        dt.dat_StrTime = data->time_string;

        DateToStr(&dt);

        *array++ = data->date_string;
        *array++ = data->time_string;

        data->prot_string[0] =
            (entry->fib.fib_Protection & FIBF_SCRIPT) ? 's' : '-';
        data->prot_string[1] =
            (entry->fib.fib_Protection & FIBF_PURE) ? 'p' : '-';
        data->prot_string[2] =
            (entry->fib.fib_Protection & FIBF_ARCHIVE) ? 'a' : '-';
        data->prot_string[3] =
            (entry->fib.fib_Protection & FIBF_READ) ? '-' : 'r';
        data->prot_string[4] =
            (entry->fib.fib_Protection & FIBF_WRITE) ? '-' : 'w';
        data->prot_string[5] =
            (entry->fib.fib_Protection & FIBF_EXECUTE) ? '-' : 'e';
        data->prot_string[6] =
            (entry->fib.fib_Protection & FIBF_DELETE) ? '-' : 'd';
        data->prot_string[7] = '\0';

        *array++ = data->prot_string;
        *array = entry->fib.fib_Comment;
    }
    else
    {
        *array++ = (char *)_(MSG_DIRLIST_NAME);
        *array++ = (char *)_(MSG_DIRLIST_SIZE);
        *array++ = (char *)_(MSG_DIRLIST_DATE);
        *array++ = (char *)_(MSG_DIRLIST_TIME);
        *array++ = (char *)_(MSG_DIRLIST_FLAGS);
        *array = (char *)_(MSG_DIRLIST_COMMENT);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

/*
 * Forward declaration. Store options in tag order; if Directory appeared,
 * conditionally clear a Valid list, emit ineffective counter resets, then
 * synchronously scan non-NULL directory. Return List setter result only for
 * OM_SET, otherwise zero. Does not own pointer inputs, validate patterns or
 * implement sorting; notifications can run during scan.
 */
IPTR Dirlist__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg);

/*
 * Create List with default six-column format unless supplied, install
 * owned-instance hooks, initialize Invalid and apply constructor tags
 * through local Set. Return object or zero on superclass failure; Directory
 * may synchronously scan before return without error propagation.
 * Incompatible preloaded entries or custom column counts violate hook
 * contracts.
 */
IPTR Dirlist__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    /* TODO: The format may specify different coluimn count or order
     * so it needs to be parses, and only set the requested columns in the
    * constructor/display hooks */
    STRPTR format =
        (STRPTR) GetTagData(MUIA_List_Format, 0, msg->ops_AttrList);

    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL,
        format ? TAG_IGNORE : MUIA_List_Format, (IPTR) ",P=\33r,,,,",
        TAG_MORE, (IPTR) msg->ops_AttrList);

    if (obj)
    {
        struct Dirlist_DATA *data = INST_DATA(cl, obj);

        data->status = MUIV_Dirlist_Status_Invalid;

        data->construct_hook.h_Entry = (HOOKFUNC) construct_func;
        data->destruct_hook.h_Entry = (HOOKFUNC) destruct_func;
        data->display_hook.h_Entry = (HOOKFUNC) display_func;
        data->display_hook.h_Data = data;

        SetAttrs(obj, MUIA_List_ConstructHook,
            (IPTR) & data->construct_hook, MUIA_List_DestructHook,
            (IPTR) & data->destruct_hook, MUIA_List_DisplayHook,
            (IPTR) & data->display_hook, TAG_DONE);

        Dirlist__OM_SET(cl, obj, msg);
    }

    return (IPTR) obj;
}

/*
 * Free last generated Path, then delegate List entry/child disposal and
 * return its result. Requires no active scan or display callbacks. Borrowed
 * directory/patterns/filterhook remain caller-owned.
 */
IPTR Dirlist__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Dirlist_DATA *data = INST_DATA(cl, obj);

    FreeVec(data->path);

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Synchronously allocate FIB, shared-lock and Examine directory, then
 * ExNext/filter/append copied entries. Release lock/FIB on completed local
 * paths; no timeout, UI pumping or returned status. Hook branch never
 * inserts; counters accumulate and byte field remains zero. Mark Valid even
 * after enumeration error; preserve borrowed storage and object through
 * callbacks.
 */
static void ReadDirectory(Object *obj, struct Dirlist_DATA *data)
{
    struct FileInfoBlock *fib;
    BPTR lock;

    if ((fib = AllocDosObject(DOS_FIB, NULL)))
    {
        if ((lock = Lock(data->directory, SHARED_LOCK)))
        {
            BOOL success;

            success = Examine(lock, fib);

            if (success && (fib->fib_DirEntryType > 0))
            {
                for (;;)
                {
                    BOOL isdir;

                    success = ExNext(lock, fib);

                    if (!success)
                    {
                        if (IoErr() == ERROR_NO_MORE_ENTRIES)
                            success = TRUE;

                        break;
                    }

                    isdir = (fib->fib_DirEntryType > 0);

                    if (data->filterhook)
                    {
                        struct ExAllData ead = { 0 };

                        ead.ed_Name = fib->fib_FileName;
                        ead.ed_Type = fib->fib_DirEntryType;
                        ead.ed_Size = fib->fib_Size;
                        ead.ed_Prot = fib->fib_Protection;
                        ead.ed_Days = fib->fib_Date.ds_Days;
                        ead.ed_Mins = fib->fib_Date.ds_Minute;
                        ead.ed_Ticks = fib->fib_Date.ds_Tick;
                        ead.ed_Comment = fib->fib_Comment;

                        if (!CallHookPkt(data->filterhook, obj, &ead))
                            continue;
                    }
                    else
                    {
                        if (isdir && data->filesonly)
                            continue;
                        if (!isdir && data->drawersonly)
                            continue;

                        if (data->rejecticons)
                        {
                            WORD len = strlen(fib->fib_FileName);

                            if (len >= 5)
                            {
                                if (stricmp(fib->fib_FileName + len - 5,
                                        ".info") == 0)
                                    continue;
                            }
                        }

                        if (!isdir || data->filterdrawers)
                        {
                            if (data->acceptpattern)
                            {
                                if (!MatchPatternNoCase(data->acceptpattern,
                                        fib->fib_FileName))
                                    continue;
                            }

                            if (data->rejectpattern)
                            {
                                if (MatchPatternNoCase(data->rejectpattern,
                                        fib->fib_FileName))
                                    continue;
                            }
                        }

                        if (isdir)
                        {
                            set(obj, MUIA_Dirlist_NumDrawers,
                                ++data->numdrawers);
                        }
                        else
                        {
                            set(obj, MUIA_Dirlist_NumFiles,
                                ++data->numfiles);
                            set(obj, MUIA_Dirlist_NumBytes,
                                data->numbytes + fib->fib_Size);
                        }

                        DoMethod(obj, MUIM_List_InsertSingle, (IPTR) fib,
                            MUIV_List_Insert_Bottom);

                    }           /* no filterhook */

                }               /* for(;;) */

                set(obj, MUIA_Dirlist_Status, MUIV_Dirlist_Status_Valid);


            }
            UnLock(lock);

        }
        FreeDosObject(DOS_FIB, fib);
    }
}

/*
 * Store options in tag order; if Directory appeared, conditionally clear a
 * Valid list, emit ineffective counter resets, then synchronously scan non-
 * NULL directory. Return List setter result only for OM_SET, otherwise
 * zero. Does not own pointer inputs, validate patterns or implement
 * sorting; notifications can run during scan.
 */
IPTR Dirlist__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Dirlist_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tag, *tags;
    BOOL directory_changed = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        IPTR tidata = tag->ti_Data;

        switch (tag->ti_Tag)
        {

        case MUIA_Dirlist_AcceptPattern:
            data->acceptpattern = (STRPTR) tidata;
            break;

        case MUIA_Dirlist_Directory:
            data->directory = (STRPTR) tidata;
            directory_changed = TRUE;
            break;

        case MUIA_Dirlist_DrawersOnly:
            data->drawersonly = tidata ? TRUE : FALSE;
            break;

        case MUIA_Dirlist_FilesOnly:
            data->filesonly = tidata ? TRUE : FALSE;
            break;

        case MUIA_Dirlist_FilterDrawers:
            data->filterdrawers = tidata ? TRUE : FALSE;
            break;

        case MUIA_Dirlist_FilterHook:
            data->filterhook = (struct Hook *)tidata;
            break;

        case MUIA_Dirlist_MultiSelDirs:
            data->multiseldirs = tidata ? TRUE : FALSE;
            break;

        case MUIA_Dirlist_RejectIcons:
            data->rejecticons = tidata ? TRUE : FALSE;
            break;

        case MUIA_Dirlist_RejectPattern:
            data->rejectpattern = (STRPTR) tidata;
            break;

        case MUIA_Dirlist_SortDirs:
            data->sortdirs = tidata ? TRUE : FALSE;
            break;

        case MUIA_Dirlist_SortHighLow:
            data->sorthighlow = tidata ? TRUE : FALSE;
            break;

        case MUIA_Dirlist_SortType:
            data->sorttype = tidata;
            break;

        case MUIA_Dirlist_Status:
            data->status = tidata;
            break;

        }
    }

    if (directory_changed)
    {
        if (data->status == MUIV_Dirlist_Status_Valid)
        {
            DoMethod(obj, MUIM_List_Clear);

            SetAttrs(obj, MUIA_Dirlist_Status, MUIV_Dirlist_Status_Invalid,
                MUIA_Dirlist_NumBytes, 0,
                MUIA_Dirlist_NumFiles, 0,
                MUIA_Dirlist_NumDrawers, 0, TAG_DONE);

        }

        if (data->directory)
        {
            ReadDirectory(obj, data);
        }

    }

    return (msg->MethodID == OM_SET) ? DoSuperMethodA(cl, obj,
        (Msg) msg) : 0;

}

/*
 * Return stored options/counters with TRUE; Path first frees old result and
 * creates borrowed owned-buffer result or NULL while still returning TRUE.
 * Requires writable storage and valid Directory/active record; WORD path
 * length must not overflow. Delegate unknown queries to List.
 */
IPTR Dirlist__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Dirlist_DATA *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)

    switch (msg->opg_AttrID)
    {
    case MUIA_Dirlist_AcceptPattern:
        STORE = (IPTR) data->acceptpattern;
        return 1;

    case MUIA_Dirlist_Directory:
        STORE = (IPTR) data->directory;
        return 1;

    case MUIA_Dirlist_DrawersOnly:
        STORE = data->drawersonly;
        return 1;

    case MUIA_Dirlist_FilesOnly:
        STORE = data->filesonly;
        return 1;

    case MUIA_Dirlist_FilterDrawers:
        STORE = data->filterdrawers;
        return 1;

    case MUIA_Dirlist_FilterHook:
        STORE = (IPTR) data->filterhook;
        return 1;

    case MUIA_Dirlist_MultiSelDirs:
        STORE = data->multiseldirs;
        return 1;

    case MUIA_Dirlist_NumBytes:
        STORE = data->numbytes;
        return 1;

    case MUIA_Dirlist_NumFiles:
        STORE = data->numfiles;
        return 1;

    case MUIA_Dirlist_NumDrawers:
        STORE = data->numdrawers;
        return 1;

    case MUIA_Dirlist_Path:
        if (data->path)
        {
            FreeVec(data->path);
            data->path = NULL;
        }

        STORE = 0;

        if (data->status == MUIV_Dirlist_Status_Valid)
        {
            struct FileInfoBlock *fib;

            DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active,
                &fib);

            if (fib)
            {
                WORD len =
                    strlen(fib->fib_FileName) + strlen(data->directory) + 3;

                data->path = AllocVec(len, MEMF_ANY);
                if (data->path)
                {
                    strcpy(data->path, data->directory);
                    if (AddPart(data->path, fib->fib_FileName, len))
                    {
                        STORE = (IPTR) data->path;
                    }
                }
            }
        }
        return 1;

    case MUIA_Dirlist_RejectIcons:
        STORE = data->rejecticons;
        return 1;

    case MUIA_Dirlist_RejectPattern:
        STORE = (IPTR) data->rejectpattern;
        return 1;

    case MUIA_Dirlist_SortDirs:
        STORE = data->sortdirs;
        return 1;

    case MUIA_Dirlist_SortHighLow:
        STORE = data->sorthighlow;
        return 1;

    case MUIA_Dirlist_SortType:
        STORE = data->sorttype;
        return 1;

    case MUIA_Dirlist_Status:
        STORE = data->status;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/*
 * Force Quiet TRUE, clear only when Status is Valid, emit Invalid/zero
 * counter tags, scan current non-NULL Directory, force Quiet FALSE and
 * return zero. Does not restore prior Quiet, reset stored counts or report
 * errors. Requires live object and borrowed storage through blocking scan
 * and callbacks.
 */
IPTR Dirlist__MUIM_Dirlist_ReRead(struct IClass *cl, Object *obj,
    struct MUIP_Dirlist_ReRead *msg)
{
    struct Dirlist_DATA *data = INST_DATA(cl, obj);

    set(obj, MUIA_List_Quiet, TRUE);
    if (data->status == MUIV_Dirlist_Status_Valid)
    {
        DoMethod(obj, MUIM_List_Clear);

        SetAttrs(obj, MUIA_Dirlist_Status, MUIV_Dirlist_Status_Invalid,
            MUIA_Dirlist_NumBytes, 0,
            MUIA_Dirlist_NumFiles, 0,
            MUIA_Dirlist_NumDrawers, 0, TAG_DONE);
    }

    if (data->directory)
    {
        ReadDirectory(obj, data);
    }
    set(obj, MUIA_List_Quiet, FALSE);

    return 0;
}


#if ZUNE_BUILTIN_DIRLIST
/*
 * Dispatch local construction/disposal/set/get/ReRead and return handler
 * results, delegating other messages to List. Requires ABI-correct message
 * and live object except construction. No additional synchronization or
 * filesystem status conversion.
 */
BOOPSI_DISPATCHER(IPTR, Dirlist_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Dirlist__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Dirlist__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Dirlist__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Dirlist__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Dirlist_ReRead:
        return Dirlist__MUIM_Dirlist_ReRead(cl, obj, (struct opGet *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Dirlist_desc =
{
    MUIC_Dirlist,
    MUIC_List,
    sizeof(struct Dirlist_DATA),
    (void *) Dirlist_Dispatcher
};
#endif /* ZUNE_BUILTIN_DIRLIST */
