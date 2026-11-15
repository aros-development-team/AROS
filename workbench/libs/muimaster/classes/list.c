/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <string.h>
#include <stdlib.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/view.h>
#include <devices/rawkeycodes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

/* #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"
#include "textengine.h"
#include "listimage.h"
#include "prefs.h"

/****** List.mui/MUIC_List ************
*
*   NAME
*       MUIC_List
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Display indexed entries in columns and manage the active cursor and
*       selection. List is a Group containing a drawing area and optional
*       vertical scrollbar; its Listview_List getter returns the List itself.
*
*       Entries are opaque pointers. By default List retains them without
*       copying or freeing them, but displays and compares them as terminated
*       strings. To own copied strings, pair MUIV_List_ConstructHook_String
*       with MUIV_List_DestructHook_String. Custom data needs compatible
*       display and comparison hooks.
*
*       List owns its entry records and internal pool, but borrows external
*       pools, hooks, format strings, titles and entry data. Keep borrowed
*       objects valid while in use; do not mutate or dispose the List during
*       its synchronous hooks.
*
*   NOTES
*       Active and selected are independent. Move and Exchange preserve the
*       active index; Sort follows the active entry. Direct Select calls do
*       not send Listview_SelectChange notifications. Quiet defers drawing but
*       not all notifications or callbacks.
*
*       CreateImage handles require explicit DeleteImage; List does not
*       dispose their Area objects or automatically release all handles at
*       cleanup.
*
*   BUGS
*       Insert may leave a partially changed list on allocation or
*       construction failure. Format replacement is also nontransactional; its
*       setter does not report allocation failure. Some setters and redraw
*       paths trust caller indices. Clear/Remove can access stale active-entry
*       records while updating Active. With external pools, the title record
*       is not explicitly freed. See the individual method contracts for
*       affected cases.
*
*/
/****** Scrmodelist.mui/MUIC_Scrmodelist ************
*
*   NAME
*       MUIC_Scrmodelist
*
*   SUPERCLASS
*       Not defined by the builtin implementation.
*
*   DESCRIPTION
*       The header declares Scrmodelist.mui and macros.h provides
*       ScrmodelistObject, but AROS does not register or implement a built-in
*       Scrmodelist class. No runtime availability, superclass or methods
*       follow from this declaration; external classes are outside this
*       contract.
*
*/
/****** List.mui/MUIA_List_Active ************
*
*   NAME
*       MUIA_List_Active
*
*   FUNCTION
*       Read or change the active entry index.
*
*   INPUTS
*       Initialize, set, get; LONG. Default MUIV_List_Active_Off (-1).
*
*   NOTES
*       Accepts an index or Off, Top, Bottom, Up, Down, PageUp or PageDown.
*       Relative movement clamps to the entry range; construction only
*       resolves Top/Bottom and clamps explicit values. Active does not change
*       selection. After setup it calls Jump even when AutoVisible is off.
*       Notifications can carry the supplied selector rather than the resolved
*       index; get the value again after setting. Read-only Listview hides the
*       cursor, not this setter.
*
*/
/****** List.mui/MUIA_List_AdjustWidth ************
*
*   NAME
*       MUIA_List_AdjustWidth
*
*   FUNCTION
*       Constrain the list width from its contents during size negotiation.
*
*   INPUTS
*       Initialize only; BOOL, default FALSE.
*
*   NOTES
*       For nonempty lists the minimum, default and maximum width use the sum
*       of measured column widths, gaps and bars. Empty lists retain
*       40/100/unlimited. There is no local setter/getter or automatic parent
*       relayout on entry changes.
*
*/
/****** List.mui/MUIA_List_AdjustHeight ************
*
*   NAME
*       MUIA_List_AdjustHeight
*
*   FUNCTION
*       Constrain the list height from its contents during size negotiation.
*
*   INPUTS
*       Initialize only; BOOL, default FALSE.
*
*   NOTES
*       For nonempty lists minimum, default and maximum height use measured
*       entry heights plus the title; empty lists retain 36/96/unlimited.
*       Drawing uses one maximum row height, so all rows need not fit. No
*       local setter/getter or automatic parent relayout is provided.
*
*/
/****** List.mui/MUIA_List_AutoVisible ************
*
*   NAME
*       MUIA_List_AutoVisible
*
*   FUNCTION
*       Enable the active-entry adjustment performed during layout.
*
*   INPUTS
*       Initialize, set, get; BOOL, default FALSE.
*
*   NOTES
*       Layout scrolls when Active crosses the lower visible boundary, but
*       does not symmetrically handle an Active above First. Changing Active
*       after setup calls Jump regardless of this flag; setting it does not
*       scroll immediately. Get returns a flag mask, not normalized TRUE.
*
*/
/****** List.mui/MUIA_List_CompareHook ************
*
*   NAME
*       MUIA_List_CompareHook
*
*   FUNCTION
*       Supply the comparator used by Compare and Sort.
*
*   INPUTS
*       Initialize, set; borrowed Hook pointer. NULL selects the builtin
*       comparator; no local getter.
*
*   NOTES
*       CallHookPkt receives entry2 as object and entry1 as message. Return a
*       negative, zero or positive LONG for the entry1/entry2 order. Default
*       is case-insensitive Stricmp on terminated strings. Changing the hook
*       does not sort; keep hook and data live during synchronous comparison.
*
*/
/****** List.mui/MUIA_List_ConstructHook ************
*
*   NAME
*       MUIA_List_ConstructHook
*
*   FUNCTION
*       Choose how incoming entry pointers become stored entries.
*
*   INPUTS
*       Initialize, set; borrowed Hook pointer, NULL, or
*       MUIV_List_ConstructHook_String. Default NULL; no local getter.
*
*   NOTES
*       NULL stores the input pointer unchanged. The String selector copies
*       text into the pool (NULL becomes empty). Custom hooks receive pool as
*       object and entry as message; zero means failure. Match owned data to a
*       Destruct hook before insertion. Changing hooks does not reconstruct
*       old entries.
*
*/
/****** List.mui/MUIA_List_DestructHook ************
*
*   NAME
*       MUIA_List_DestructHook
*
*   FUNCTION
*       Choose how stored entry data is released.
*
*   INPUTS
*       Initialize, set; borrowed Hook pointer, NULL, or
*       MUIV_List_DestructHook_String. Default NULL; no local getter.
*
*   NOTES
*       NULL does not free entry data. String frees only allocations from the
*       matching String constructor. Custom hooks receive pool and stored
*       entry; return values are ignored. Changing policy while entries exist
*       may free them incorrectly.
*
*/
/****** List.mui/MUIA_List_DisplayHook ************
*
*   NAME
*       MUIA_List_DisplayHook
*
*   FUNCTION
*       Provide display strings for the columns of an entry or title.
*
*   INPUTS
*       Initialize, set; borrowed Hook pointer or NULL. Default NULL; no local
*       getter.
*
*   NOTES
*       The hook receives the string-pointer array as object and entry as
*       message; NULL entry denotes a title. Set all displayed columns to live
*       strings or NULL. The slot preceding the array carries entry metadata,
*       with ABI limits noted in MUIM_List_Display. Without a hook only column
*       zero is assigned. Changing the hook does not remeasure; request
*       Redraw_All.
*
*/
/****** List.mui/MUIA_List_DragSortable ************
*
*   NAME
*       MUIA_List_DragSortable
*
*   FUNCTION
*       Accept a drag from this List and move its active entry on drop.
*
*   INPUTS
*       Initialize, set, get; BOOL, default FALSE.
*
*   NOTES
*       DragQuery accepts this List as source only when enabled. DragDrop
*       moves the active entry and sets Active, not a group of selected
*       entries. This flag does not set Draggable or the Listview drag type.
*       Get returns a mask.
*
*/
/****** List.mui/MUIA_List_DropMark ************
*
*   NAME
*       MUIA_List_DropMark
*
*   FUNCTION
*       Read the insertion boundary recorded by the last DragDrop call.
*
*   INPUTS
*       Get; LONG, initially zero.
*
*   NOTES
*       DragDrop stores the insertion boundary before moving the active entry;
*       Entries means after the last item. Hover drawing does not update this
*       attribute, and its value does not prove Move succeeded.
*
*/
/****** List.mui/MUIA_List_Entries ************
*
*   NAME
*       MUIA_List_Entries
*
*   FUNCTION
*       Read the current number of entries.
*
*   INPUTS
*       Get; LONG, initially zero.
*
*   NOTES
*       Mutators update the confirmed entry count and may notify through the
*       superclass. Applications must use the entry methods, not set the
*       count. Insertion failure can leave it inconsistent; see
*       MUIM_List_Insert.
*
*/
/****** List.mui/MUIA_List_First ************
*
*   NAME
*       MUIA_List_First
*
*   FUNCTION
*       Read or set the first displayed entry index.
*
*   INPUTS
*       Get and implemented Set despite the get-only header annotation; LONG,
*       initially zero. No constructor parsing.
*
*   NOTES
*       Set stores the index without validation, requests redraw and updates
*       the scrollbar unless Quiet. Prefer Jump after layout. Cleanup does not
*       reset First to -1; it does not indicate that a window is open. Set may
*       notify.
*
*/
/****** List.mui/MUIA_List_Format ************
*
*   NAME
*       MUIA_List_Format
*
*   FUNCTION
*       Describe columns using comma-separated ReadArgs option strings.
*
*   INPUTS
*       Initialize, set, get; borrowed terminated STRPTR or NULL. Default NULL
*       gives one default column.
*
*   NOTES
*       Comma-separated columns accept DELTA=D/N, PREPARSE=P/K, WEIGHT=W/N,
*       MINWIDTH=MIW/N, MAXWIDTH=MAW/N, COL=C/N and BAR/S. Only DELTA,
*       PREPARSE, BAR and measured text widths affect layout in this
*       implementation; WEIGHT, MINWIDTH, MAXWIDTH and COL are stored but not
*       applied. BAR adds 2 pixels; PREPARSE is copied, while the Format
*       string remains borrowed.
*
*       Set reparses and redraws without reporting allocation failure. Prior
*       format storage is released first; partial growth is not rolled back.
*       Column growth can leave a title record undersized when a title is
*       enabled later. Keep the supplied Format string alive.
*
*/
/****** List.mui/MUIA_List_InsertPosition ************
*
*   NAME
*       MUIA_List_InsertPosition
*
*   FUNCTION
*       Read the starting slot chosen by the latest insertion attempt.
*
*   INPUTS
*       Get; LONG, initially zero.
*
*   NOTES
*       Insert sets this before allocation, so failure can change it. With
*       sorted insertion it records the original append slot, not final sorted
*       positions; the method return is a different value.
*
*/
/****** List.mui/MUIA_List_MinLineHeight ************
*
*   NAME
*       MUIA_List_MinLineHeight
*
*   FUNCTION
*       Set the minimum measured entry height.
*
*   INPUTS
*       Initialize only; ULONG storage despite LONG header annotation. Default
*       zero.
*
*   NOTES
*       Measurement applies this to entries and title; all rows then use the
*       maximum height. There is no local validation or Set/Get: negative
*       signed values become large unsigned heights.
*
*/
/****** List.mui/MUIA_List_MultiTestHook ************
*
*   NAME
*       MUIA_List_MultiTestHook
*
*   FUNCTION
*       Decide whether an entry can share a selection with other entries.
*
*   INPUTS
*       Initialize, set; borrowed Hook pointer or NULL. Default NULL permits
*       multiselection. No local getter.
*
*   NOTES
*       The hook receives NULL as object and the entry as message. Zero
*       forbids multi-selection with that entry, but not selecting it alone.
*       Setting a non-NULL hook clears selection, even if unchanged; Ask does
*       not call it. Keep the hook valid and do not mutate List during the
*       call.
*
*/
/****** List.mui/MUIA_List_Pool ************
*
*   NAME
*       MUIA_List_Pool
*
*   FUNCTION
*       Supply the pool used for entry records, copied strings and image
*       handles.
*
*   INPUTS
*       Initialize only; valid Exec pool pointer or NULL. Default NULL creates
*       an internal pool.
*
*   NOTES
*       External pools are borrowed and must outlive List and its handles;
*       List deletes only its own pool. Pool size tags affect only pool
*       creation. The title record is not explicitly freed on disposal in
*       external pools.
*
*/
/****** List.mui/MUIA_List_PoolPuddleSize ************
*
*   NAME
*       MUIA_List_PoolPuddleSize
*
*   FUNCTION
*       Set a size passed to CreatePool when List creates its own pool.
*
*   INPUTS
*       Initialize only; ULONG tag stored in LONG. Default 2008 bytes.
*
*   NOTES
*       Use values valid for Exec CreatePool and representable by the stored
*       size. The class does not validate the size pair. Ignored for pool
*       creation when Pool supplies an external pool. No local setter or getter
*       exists; this does not resize a live pool.
*
*/
/****** List.mui/MUIA_List_PoolThreshSize ************
*
*   NAME
*       MUIA_List_PoolThreshSize
*
*   FUNCTION
*       Set a size passed to CreatePool when List creates its own pool.
*
*   INPUTS
*       Initialize only; ULONG tag stored in LONG. Default 1024 bytes.
*
*   NOTES
*       Use values valid for Exec CreatePool and representable by the stored
*       size. The class does not validate the size pair. Ignored for pool
*       creation when Pool supplies an external pool. No local setter or getter
*       exists; this does not resize a live pool.
*
*/
/****** List.mui/MUIA_List_Quiet ************
*
*   NAME
*       MUIA_List_Quiet
*
*   FUNCTION
*       Defer List rendering and selected scrollbar updates during a batch.
*
*   INPUTS
*       Set only; BOOL, initially FALSE. No local constructor parsing or getter.
*
*   NOTES
*       Quiet defers drawing and some scrollbar updates, but is not a nesting
*       counter and does not suppress all notifications or callbacks. Updates
*       may be lost in paths that do not record pending work; restore FALSE
*       and request Redraw_All if necessary.
*
*/
/****** List.mui/MUIA_List_ShowDropMarks ************
*
*   NAME
*       MUIA_List_ShowDropMarks
*
*   FUNCTION
*       Draw a provisional insertion line during drag reporting.
*
*   INPUTS
*       Initialize, set, get; BOOL, default TRUE.
*
*   NOTES
*       Only affects rendering in DragReport, not whether a drag is accepted or
*       entries move. Get returns a flag mask when enabled. The drawing position
*       is tracked separately from DropMark. DragFinish resets the internal y
*       position and delegates; it does not explicitly erase the line itself.
*
*/
/****** List.mui/MUIA_List_SourceArray ************
*
*   NAME
*       MUIA_List_SourceArray
*
*   FUNCTION
*       Insert an initial NULL-terminated array of entry pointers.
*
*   INPUTS
*       Initialize only; readable APTR array or NULL. Default NULL.
*
*   NOTES
*       The constructor consumes the NULL-terminated pointer array
*       synchronously and inserts according to ConstructHook. It does not
*       retain the array. Partial insertion failure is possible; see
*       MUIM_List_Insert.
*
*/
/****** List.mui/MUIA_List_Title ************
*
*   NAME
*       MUIA_List_Title
*
*   FUNCTION
*       Show a direct title or request column titles from the display callback.
*
*   INPUTS
*       Initialize, set, get; borrowed STRPTR, NULL for no title, or pointer
*       value 1 for callback titles. Default NULL.
*
*   NOTES
*       A one-column title other than pointer value 1 is displayed directly.
*       With multiple columns, or value 1, Display is called with NULL entry and
*       entry_pos -1; provide all title strings. The string is not copied or
*       freed. Set requests Redraw_All. Title is not included in Entries and
*       cannot be retrieved as an ordinary entry index; it has a separate
*       internal record.
*
*/
/****** List.mui/MUIA_List_Visible ************
*
*   NAME
*       MUIA_List_Visible
*
*   FUNCTION
*       Read the number of complete rows calculated for the drawing area.
*
*   INPUTS
*       Get; LONG, initially -1.
*
*   NOTES
*       Layout/drawing divide available height after title space by the maximum
*       row height. The result is capacity, not the number of existing entries,
*       and can exceed Entries. It can be zero for a small area. AROS emits
*       Visible and private scrollbar notifications when recalculated. Cleanup
*       does not reset it to -1. Internal setters synchronize scrollbar state;
*       applications should not set it to fabricate layout.
*
*/
/****** List.mui/MUIM_List_Clear ************
*
*   NAME
*       MUIM_List_Clear
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Clear);
*
*   FUNCTION
*       Remove all confirmed entries.
*
*   INPUTS
*       obj - Live List with matching destruction policy.
*
*   RESULT
*       Always zero, including an empty list.
*
*   NOTES
*       Calls Destruct in reverse order, frees records, resets Entries/First
*       and Active, and requests redraw. The title and capacity remain.
*       Updating Active after freeing its former entry can touch stale
*       storage.
*
*/
/****** List.mui/MUIM_List_Construct ************
*
*   NAME
*       MUIM_List_Construct
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Construct, (IPTR)entry,
*       (IPTR)pool);
*
*   FUNCTION
*       Create the data pointer to store for an entry.
*
*   INPUTS
*       entry - Input data interpreted by the construction policy.
*       pool - Live pool supplied to that policy.
*
*   RESULT
*       Stored pointer, or zero on failure.
*
*   NOTES
*       NULL hook returns the input pointer. String mode copies text into a
*       pool allocation with its size stored immediately before the string;
*       NULL input becomes empty text. Custom hooks receive pool and entry.
*       The returned pointer must be released with the matching Destruct
*       policy. This call does not insert an entry.
*
*/
/****** List.mui/MUIM_List_Destruct ************
*
*   NAME
*       MUIM_List_Destruct
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Destruct, (IPTR)entry,
*       (IPTR)pool);
*
*   FUNCTION
*       Release entry data using the current destruction policy.
*
*   INPUTS
*       entry - Data produced by the matching construction policy.
*       pool - Same live pool used for allocation.
*
*   RESULT
*       Always zero; custom-hook return is ignored.
*
*   NOTES
*       NULL hook does nothing. String mode frees only allocations made by its
*       matching String constructor; custom hooks receive pool and stored
*       entry. Direct calls do not remove List records: do not free data still
*       referenced.
*
*/
/****** List.mui/MUIM_List_Compare ************
*
*   NAME
*       MUIM_List_Compare
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Compare, (IPTR)entry1,
*       (IPTR)entry2, sort_type1, sort_type2);
*
*   FUNCTION
*       Compare entry1 with entry2.
*
*   INPUTS
*       entry1, entry2 - Live data suitable for the current comparator.
*       sort_type1, sort_type2 - Declared LONG fields; ignored locally.
*
*   RESULT
*       Hook result; interpret as signed LONG ordering, negative/zero/positive.
*
*   NOTES
*       Custom CompareHook receives entry2 as object and entry1 as message;
*       default is case-insensitive Stricmp. This method does not sort or
*       copy.
*
*/
/****** List.mui/MUIM_List_Display ************
*
*   NAME
*       MUIM_List_Display
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Display, (IPTR)entry, (IPTR)array,
*       entry_pos, (IPTR)preparses);
*
*   FUNCTION
*       Fill display-column string pointers.
*
*   INPUTS
*       entry - Stored data, or NULL for title.
*       array - Writable STRPTR array with a writable preceding pointer slot
*       when using a hook.
*       entry_pos - LONG index, or -1 for title.
*       preparses - Declared array; ignored by the base method.
*
*   RESULT
*       1 without a hook; otherwise the hook result. Rendering callers ignore
*       that result.
*
*   NOTES
*       Without a hook only array[0] is assigned. A hook receives the array as
*       object and entry as message. The preceding slot holds entry position,
*       written as ULONG here but pointer-sized on an internal row path; title
*       handling differs. Do not rely on consistent pointer-sized metadata.
*       All used column strings must survive measurement and drawing. Format
*       preparses are prepared internally; this base method does not pass its
*       preparses argument separately to the hook.
*
*/
/****** List.mui/MUIM_List_GetEntry ************
*
*   NAME
*       MUIM_List_GetEntry
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_GetEntry, pos, (IPTR)entry);
*
*   FUNCTION
*       Return the stored data pointer at an index.
*
*   INPUTS
*       pos - LONG index or MUIV_List_GetEntry_Active.
*       entry - Non-NULL writable APTR result location.
*
*   RESULT
*       Stored pointer as IPTR, also written to the entry output; zero with a
*       NULL output value for an invalid position.
*
*   NOTES
*       Returns borrowed data. Sorting/moving changes indices; removing or
*       clearing entries may destroy the pointer. No copy is made. The output
*       pointer must not be NULL.
*
*/
/****** List.mui/MUIM_List_Insert ************
*
*   NAME
*       MUIM_List_Insert
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Insert, (IPTR)entries, count,
*       pos);
*
*   FUNCTION
*       Construct and insert a sequence of entries.
*
*   INPUTS
*       entries - Readable APTR array for the call.
*       count - Positive LONG count, or -1 for a NULL-terminated array.
*       pos - LONG insertion slot or MUIV_List_Insert_Top, Active, Sorted or
*       Bottom.
*
*   RESULT
*       On completion, the pre-sort index of the last inserted slot, cast to
*       ULONG. On nonpositive count or failure, all-bits-one IPTR (~0). This is
*       not a Boolean or count.
*
*   NOTES
*       Top inserts at zero, Bottom appends, Active uses its index or zero,
*       and Sorted appends then sorts. Other positions are clamped.
*       ConstructHook supplies each stored pointer; the input array is not
*       retained. InsertPosition is the pre-sort slot; sorting can also affect
*       Active.
*
*       This operation is not atomic. An allocation or Construct failure can
*       leave partial entries and inconsistent counts; failure paths do not
*       roll back and may calculate invalid move lengths. The return value
*       cannot certify a recoverable partial state. Do not mutate List
*       reentrantly from its hooks.
*
*/
/****** List.mui/MUIM_List_InsertSingle ************
*
*   NAME
*       MUIM_List_InsertSingle
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_InsertSingle, (IPTR)entry, pos);
*
*   FUNCTION
*       Insert one entry through Insert.
*
*   INPUTS
*       entry - Input interpreted by ConstructHook; NULL is allowed only if that
*       policy produces a non-NULL stored value.
*       pos - Same index/selectors as Insert.
*
*   RESULT
*       Exactly the Insert result for a count of one: pre-sort slot or
*       all-bits-one failure.
*
*   NOTES
*       Passes the message entry to Insert synchronously. Construction,
*       sorting, error handling and ownership follow MUIM_List_Insert.
*
*/
/****** List.mui/MUIM_List_Move ************
*
*   NAME
*       MUIM_List_Move
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Move, from, to);
*
*   FUNCTION
*       Move one entry to another index.
*
*   INPUTS
*       from - LONG index or MUIV_List_Move_Top, Active or Bottom.
*       to - Same choices, plus MUIV_List_Move_Next or Previous relative to the
*       resolved first index.
*
*   RESULT
*       TRUE when distinct valid indices were changed; FALSE for invalid indices
*       or the same index.
*
*   NOTES
*       Move shifts records; selection bits follow the data. Active keeps its
*       index and may name different data afterwards. No Construct/Destruct or
*       Active notification occurs. Quiet defers the full redraw.
*
*/
/****** List.mui/MUIM_List_Exchange ************
*
*   NAME
*       MUIM_List_Exchange
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Exchange, pos1, pos2);
*
*   FUNCTION
*       Exchange two entry records.
*
*   INPUTS
*       pos1 - LONG index or MUIV_List_Exchange_Top, Active or Bottom.
*       pos2 - Same choices, plus MUIV_List_Exchange_Next or Previous relative
*       to the resolved first index.
*
*   RESULT
*       TRUE when distinct valid indices were changed; FALSE for invalid indices
*       or the same index.
*
*   NOTES
*       Exchange swaps two records, preserving their selection bits. Active
*       keeps its index and may name different data afterwards. No
*       Construct/Destruct or Active notification occurs. Unlike Move,
*       Exchange does not record the same pending full update under Quiet.
*
*/
/****** List.mui/MUIM_List_Remove ************
*
*   NAME
*       MUIM_List_Remove
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Remove, pos);
*
*   FUNCTION
*       Destroy and remove one entry or the selected entries.
*
*   INPUTS
*       pos - LONG index or MUIV_List_Remove_First, Active, Last or Selected.
*
*   RESULT
*       Always zero, including invalid index or empty list.
*
*   NOTES
*       Selected removes flagged entries, or Active if none are flagged. The
*       method destroys records, shifts survivors and adjusts Active. The
*       UWORD selected scan does not support unrestricted counts; updating
*       Active can reference stale records. Do not remove entries while
*       traversing them with NextSelected; use Remove_Selected instead.
*
*/
/****** List.mui/MUIM_List_Select ************
*
*   NAME
*       MUIM_List_Select
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Select, pos, seltype, (IPTR)info);
*
*   FUNCTION
*       Change selection bits or query their state.
*
*   INPUTS
*       pos - LONG index, MUIV_List_Select_Active or All.
*       seltype - MUIV_List_Select_Off deselects; On selects; Toggle reverses;
*       Ask only queries.
*       info - Optional writable LONG result pointer.
*
*   RESULT
*       Always zero. For All+Ask, the info output is the selected count.
*       Otherwise it is the last processed entry's previous selection bit; no
*       processed entry yields zero. An invalid explicit index returns before
*       writing info.
*
*   NOTES
*       Off deselects and On selects; MultiTestHook may refuse a combination.
*       Active is unchanged. Calls request redraw but do not emit
*       Listview_SelectChange. Quiet may suppress redraw without marking all
*       rows pending. Use Ask to verify the selection state.
*
*/
/****** List.mui/MUIM_List_NextSelected ************
*
*   NAME
*       MUIM_List_NextSelected
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_NextSelected, (IPTR)pos);
*
*   FUNCTION
*       Find the next selected entry index.
*
*   INPUTS
*       pos - Non-NULL writable LONG. Start with MUIV_List_NextSelected_Start;
*       otherwise supply a valid previous index.
*
*   RESULT
*       Always TRUE. The pos output receives the next index or
*       MUIV_List_NextSelected_End.
*
*   NOTES
*       Start and End are both -1. An initial call without selected entries
*       returns Active if set; later calls scan strictly after the supplied
*       index. End restarts on another call. Do not mutate the list or pass
*       invalid negative/overflowing indices while iterating.
*
*/
/****** List.mui/MUIM_List_Sort ************
*
*   NAME
*       MUIM_List_Sort
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Sort);
*
*   FUNCTION
*       Sort the entire list using Compare.
*
*   INPUTS
*       obj - Live List with a consistent comparator and valid entry data.
*
*   RESULT
*       Always zero, even when reordered.
*
*   NOTES
*       Uses unstable quadratic selection sort. Selection flags follow
*       entries; Active follows its entry without an Active notification.
*       Synchronous comparators must not mutate or dispose List. Quiet defers
*       redraw.
*
*/
/****** List.mui/MUIM_List_Jump ************
*
*   NAME
*       MUIM_List_Jump
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Jump, pos);
*
*   FUNCTION
*       Scroll until a requested index is in the visible range.
*
*   INPUTS
*       pos - LONG index or MUIV_List_Jump_Top, Active, Bottom, Up or Down.
*       obj - Laid-out List with a positive visible-row count for meaningful
*       scrolling.
*
*   RESULT
*       Always TRUE; not proof that an existing entry was displayed.
*
*   NOTES
*       Up targets First-1, Down First+Visible. The method clamps the target
*       and changes First only when needed; Active is unchanged. TRUE does not
*       confirm a visible entry when the list is empty or unlaid-out.
*
*/
/****** List.mui/MUIM_List_Redraw ************
*
*   NAME
*       MUIM_List_Redraw
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_Redraw, pos, (IPTR)entry);
*
*   FUNCTION
*       Recalculate measurements and request a row or full redraw.
*
*   INPUTS
*       pos - Valid LONG index, MUIV_List_Redraw_Active, All or Entry.
*       entry - Data pointer matched by identity for Entry; ignored otherwise.
*
*   RESULT
*       Always zero.
*
*   NOTES
*       Entry locates the first pointer-equal entry; All remeasures and
*       redraws. Cached maxima can grow without shrinking. Quiet defers
*       drawing. Numeric indices other than -1 are not fully validated and may
*       access outside the array; supply both method arguments.
*
*/
/****** List.mui/MUIM_List_TestPos ************
*
*   NAME
*       MUIM_List_TestPos
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_TestPos, x, y, (IPTR)res);
*
*   FUNCTION
*       Map window-relative coordinates to row and column information.
*
*   INPUTS
*       x, y - LONG coordinates. Both MUI_MAXMAX select the last stored
*       mouse-release position.
*       res - Non-NULL writable MUI_List_TestPos_Result.
*       obj - Laid-out List with valid measured row height.
*
*   RESULT
*       Always TRUE. Inspect entry, column and flags rather than treating TRUE
*       as a hit.
*
*   NOTES
*       Returns entry/column (-1 if absent) and hit flags; xoffset and yoffset
*       are written only on the corresponding hit paths, so initialize the
*       result before calling. Both coordinates MUI_MAXMAX use cached release
*       coordinates, not live mouse position. Visible-row bounds do not
*       subtract First; invalid scroll state can yield out-of-range entry
*       indices.
*
*/
/****** List.mui/MUIM_List_CreateImage ************
*
*   NAME
*       MUIM_List_CreateImage
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_CreateImage, (IPTR)area, flags);
*
*   FUNCTION
*       Create a list-image handle referring to an Area object.
*
*   INPUTS
*       area - Live Area-compatible object not simultaneously used in another
*       parent hierarchy.
*       flags - ULONG; ignored here, use zero.
*
*   RESULT
*       Opaque handle, or zero for NULL area or handle allocation failure.
*
*   NOTES
*       Allocates a pooled handle that borrows the Area. Setup is attempted
*       immediately when List is set up, without checking success. Embed it as
*       ESC followed by uppercase O and [hexadecimal handle] in display text.
*       Keep Area and handle valid until all references are removed; call
*       DeleteImage explicitly. List does not dispose the Area or walk handles
*       at Cleanup/Dispose.
*
*/
/****** List.mui/MUIM_List_DeleteImage ************
*
*   NAME
*       MUIM_List_DeleteImage
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_List_DeleteImage, (IPTR)listimg);
*
*   FUNCTION
*       Disconnect an image object and release its handle.
*
*   INPUTS
*       listimg - Live handle returned by this List's CreateImage, or NULL.
*
*   RESULT
*       Always zero. NULL is a no-op.
*
*   NOTES
*       Cleanup/DisconnectParent the Area, unlink the pooled handle and
*       release it; the Area is not disposed. Handles are not checked for
*       membership or setup state: remove text references and delete each once
*       during a matching setup lifecycle.
*
*/
/* Private MUIM_List_SelectChange
 * Private subclass callback. Active changes invoke it with position/state
 * and flags zero, but the base handler simply returns1 and changes nothing.
 * Public Select does not call it. The declared Multi flag does not imply
 * base behavior.
 */
/* Private MUIA_List_Prop_Entries
 * Private scrollbar count storage: Set/Get LONG. Set does not allocate or
 * change confirmed entries; only internal synchronization may use it.
 * Default0, updated by Entries except while Quiet.
 */
/* Private MUIA_List_Prop_Visible
 * Private scrollbar visible count: Set/Get LONG, default-1. Setter also
 * changes entries_visible. Layout overwrites it; arbitrary external values
 * can invalidate scrolling calculations.
 */
/* Private MUIA_List_Prop_First
 * Private scrollbar first index: Set/Get LONG, default0. Setter
 * synchronizes List_First when different, without bounds checking.
 * Scrollbar hook uses NoNotifySet to avoid notification recursion.
 */
/* Private MUIA_List_VertProp_Entries
 * Private alias of MUIA_List_Prop_Entries with identical numeric ID,
 * storage and synchronization behavior; not a second independent count.
 */
/* Private MUIA_List_VertProp_Visible
 * Private alias of MUIA_List_Prop_Visible with identical numeric ID and
 * shared visible-row storage. No independent vertical sizing contract.
 */
/* Private MUIA_List_VertProp_First
 * Private alias of MUIA_List_Prop_First, sharing unvalidated first-row
 * storage and List_First synchronization. No separate ownership or state.
 */
/* Private MUIA_List_HorizProp_Entries
 * Private header identifier without a local constructor, Set or Get branch.
 * No horizontal scrollbar count implementation is established; unknown
 * operations delegate Group.
 */
/* Private MUIA_List_HorizProp_Visible
 * Private header identifier without local handling. No horizontal visible-
 * range implementation is established; do not infer parity with vertical
 * aliases.
 */
/* Private MUIA_List_HorizProp_First
 * Private header identifier without local handling. No horizontal scrolling
 * state is stored by List for this tag; unknown operations delegate Group.
 */
/* Private MUIA_List_ListArea
 * Private constructor-only replacement Area. Non-NULL object becomes a
 * Group member and suppresses normal List row drawing; List still uses its
 * geometry. No Set/Get branch. Follow Group child ownership and lifecycle;
 * allocation/member errors are not comprehensively checked.
 */

extern struct Library *MUIMasterBase;

#define ENTRY_TITLE (-1)

#define FORMAT_TEMPLATE "DELTA=D/N,PREPARSE=P/K,WEIGHT=W/N,MINWIDTH=MIW/N," \
    "MAXWIDTH=MAW/N,COL=C/N,BAR/S"

#define BAR_WIDTH 2

#define UPDATEMODE_ALL                  (1 << 0)
#define UPDATEMODE_ENTRY                (1 << 1)
#define UPDATEMODE_NEEDED               (1 << 2)

enum
{
    ARG_DELTA,
    ARG_PREPARSE,
    ARG_WEIGHT,
    ARG_MINWIDTH,
    ARG_MAXWIDTH,
    ARG_COL,
    ARG_BAR,
    ARG_CNT
};


struct ListEntry
{
    APTR data;
    LONG width;    /* Line width */
    LONG height;   /* Line height */
    WORD flags;    /* see below */
    LONG widths[]; /* Widths of the columns */
};

#define ENTRY_SELECTED   (1<<0)
#define ENTRY_RENDER     (1<<1)


struct ColumnInfo
{
    int colno;      /* Column number */
    int user_width; /* user set width; -1 if entry width */
    int min_width;  /* min width percentage */
    int max_width;  /* min width percentage */
    int weight;
    int delta;      /* ignored for the first and last column, defaults to 4 */
    int bar;
    STRPTR preparse;
    int entries_width; /* width of the entries (maximum of all widths) */
};

struct MUI_ImageSpec_intern;

struct MUI_ListData
{
    /* bool attrs */
    ULONG flags;

    APTR intern_pool;    /* The internal pool which the class has allocated */
    LONG intern_puddle_size;
    LONG intern_thresh_size;
    APTR pool;           /* the pool which is used to allocate list entries */

    struct Hook *construct_hook;
    struct Hook *compare_hook;
    struct Hook *destruct_hook;
    struct Hook *display_hook;
    struct Hook *multi_test_hook;

    struct Hook default_compare_hook;

    /* List management, currently we use a simple flat array, which is not
     * good if many entries are inserted/deleted */
    LONG entries_num;           /* Number of Entries in the list */
    LONG entries_allocated;
    struct ListEntry **entries;

    LONG entries_first;         /* first visible entry */
    LONG entries_visible;       /* number of visible entries,
                                 * determined at MUIM_Layout */
    LONG entries_active;
    LONG insert_position;       /* pos of the last insertion */

    LONG entry_maxheight;       /* Maximum height of an entry */
    ULONG entry_minheight;      /* from MUIA_List_MinLineHeight */

    LONG entries_totalheight;
    LONG entries_maxwidth;

    LONG vertprop_entries;
    LONG vertprop_visible;
    LONG vertprop_first;

    LONG confirm_entries_num;   /* These are the correct entries num, used
                                 * so you cannot set MUIA_List_Entries to
                                 * wrong values */

    LONG entries_top_pixel;     /* Where the entries start */

    /* Column managment, is allocated by ParseListFormat() and freed
     * by CleanListFormat() */
    STRPTR format;
    LONG columns;               /* Number of columns the list has */
    LONG columns_allocated;     /* List has space for columns_allocated columns */
    struct ColumnInfo *ci;
    STRPTR *preparses;
    STRPTR *strings_mem;        /* safe pointer to allocated memory for strings[] */
    STRPTR *strings;            /* the strings for the display function, one
                                 * more than needed (for the entry position) */

    /* Titlestuff */
    int title_height;           /* The complete height of the title */
    STRPTR title;               /* On single column lists this is the title,
                                 * otherwise 1. NULL for no title(s) */

    /* Cursor images */
    struct MUI_ImageSpec_intern *list_cursor;
    struct MUI_ImageSpec_intern *list_select;
    struct MUI_ImageSpec_intern *list_selcur;

    /* Render optimization */
    int update; /* 1 - update everything, 2 - redraw entry at update_pos,
                 * 3 - scroll to current entries_first (old value is in
                 * update_pos) */
    int update_pos;

    LONG drop_mark;
    LONG drop_mark_y;

    /* list images */
    struct MinList images;

    /* user prefs */
    ListviewRefresh prefs_refresh;
    UWORD prefs_linespacing;
    BOOL prefs_smoothed;
    UWORD prefs_smoothval;

    /* render space handling */
    Object *area;
    BOOL area_replaced;
    BOOL area_connected;

    /***************************/
    /* Former Listview members */
    /***************************/

    Object *vert;
    BOOL vert_connected;
    IPTR scroller_pos;
    BOOL read_only;
    IPTR multiselect;
    LONG drag_type;

    /* clicked column */
    LONG click_column;
    LONG def_click_column;

    LONG mouse_click;            /* see below if mouse is held down */
    LONG mouse_x;
    LONG mouse_y;

    /* double click */
    ULONG last_secs;
    ULONG last_mics;
    ULONG last_active;

    struct MUI_EventHandlerNode ehn;

    /* user prefs */
    ListviewMulti prefs_multi;

    BOOL doubleclick;
    LONG seltype;

    struct Hook hook;
};

#define MOUSE_CLICK_ENTRY 1     /* on entry clicked */
#define MOUSE_CLICK_TITLE 2     /* on title clicked */

#define LIST_ADJUSTWIDTH   (1<<0)
#define LIST_ADJUSTHEIGHT  (1<<1)
#define LIST_AUTOVISIBLE   (1<<2)
#define LIST_DRAGSORTABLE  (1<<3)
#define LIST_SHOWDROPMARKS (1<<4)
#define LIST_QUIET         (1<<5)
#define LIST_CHANGED    (1<<6)

/* Forward declaration: grow pooled per-entry width records for a larger
 * column count. Requires live pool and consistent records; failure can
 * leave earlier records grown. No rollback or ownership transfer.
 */
static BOOL IncreaseColumns(struct MUI_ListData *data, int new_columns);












/**************************************************************************
 Allocate and zero a single list entry, including its width slots
**************************************************************************/
/*
 * Allocate and zero a pooled record with widths for columns+1. Requires
 * live pool and representable allocation arithmetic. Return owned record or
 * NULL; data ownership is established separately by Construct.
 */
static struct ListEntry *AllocListEntry(struct MUI_ListData *data)
{
    struct ListEntry *le;
    /* use IncreaseColumns() to enlarge column entry array */
    IPTR size = sizeof(struct ListEntry) + sizeof(LONG) * (data->columns + 1);

    le = (struct ListEntry *) AllocVecPooled(data->pool, size);
    D(bug("List AllocListEntry %p, %ld bytes\n", le, size));
    if (le)
    {
        /* possible, that we have an external pool, which does not have
           MEMF_CLEAR set.. */
        memset(le, 0, size);
    }
    return le;
}

/**************************************************************************
 Deallocate a single list entry, does not deinitialize it
**************************************************************************/
/*
 * Free a record through the same live pool used by AllocListEntry. Requires
 * no remaining record users and prior destruction of owned entry data.
 * Return void; does not call Destruct itself.
 */
static void FreeListEntry(struct MUI_ListData *data,
    struct ListEntry *entry)
{
    D(bug("FreeListEntry %p\n", entry));
    FreeVecPooled(data->pool, entry);
}

/**************************************************************************
 Ensures that there can be at least the given amount of entries within
 the list. Returns 0 if not. It also allocates the space for the title.
 It can be accessed with data->entries[ENTRY_TITLE]
**************************************************************************/
/*
 * Grow the pointer array geometrically, preserving entries and the
 * preceding title slot. Requires consistent counts and nonoverflowing
 * sizes. Return1 for sufficient/new storage,0 on allocation failure; never
 * shrink. Free old array only after successful allocation.
 */
static int SetListSize(struct MUI_ListData *data, LONG size)
{
    struct ListEntry **new_entries;
    int new_entries_allocated;

    if (size + 1 <= data->entries_allocated)
        return 1;

    new_entries_allocated = data->entries_allocated * 2 + 4;
    if (new_entries_allocated < size + 1)
        new_entries_allocated = size + 1 + 10;  /* 10 is just random */

    D(bug("List %p : SetListSize allocating %ld bytes\n", data,
            new_entries_allocated * sizeof(struct ListEntry *)));
    new_entries =
        AllocVec(new_entries_allocated * sizeof(struct ListEntry *), 0);
    if (NULL == new_entries)
        return 0;
    if (data->entries)
    {
        CopyMem(data->entries - 1, new_entries,
            (data->entries_num + 1) * sizeof(struct ListEntry *));
        FreeVec(data->entries - 1);
    }
    data->entries = new_entries + 1;
    data->entries_allocated = new_entries_allocated;
    return 1;
}

/**************************************************************************
 Prepares the insertion of count entries at pos.
 This function doesn't care if there is enough space in the datastructure.
 SetListSize() must be used first.
 With current implementation, this call will never fail
**************************************************************************/
/*
 * Move existing pointer suffix upward to reserve count slots at pos.
 * Requires sufficient allocated capacity and valid nonnegative
 * position/count. Always1; does not construct entries or update counts and
 * performs no bounds checks.
 */
static int PrepareInsertListEntries(struct MUI_ListData *data, int pos,
    int count)
{
    memmove(&data->entries[pos + count], &data->entries[pos],
        (data->entries_num - pos) * sizeof(struct ListEntry *));
    return 1;
}

/**************************************************************************
 Removes count (already deinitalized) list entries starting az pos.
**************************************************************************/
/*
 * Move pointer suffix down over count reserved/removed slots. Requires
 * pos+count within the count used in the subtraction. Return void, no
 * destruction/count update. Partial insertion callers can violate this
 * length precondition.
 */
static void RemoveListEntries(struct MUI_ListData *data, int pos, int count)
{
    // FIXME: segfault if entries_num = pos = count = 1
    memmove(&data->entries[pos], &data->entries[pos + count],
        (data->entries_num - (pos + count)) * sizeof(struct ListEntry *));
}

/**************************************************************************
 Frees all memory allocated by ParseListFormat()
**************************************************************************/
/*
 * Free copied preparses, column metadata and display pointer arrays; reset
 * columns and pointers. Requires no active display callback or renderer
 * using them. Return void; borrowed Format itself is not freed.
 */
static void FreeListFormat(struct MUI_ListData *data)
{
    int i;

    if (data->ci)
    {
        for (i = 0; i < data->columns; i++)
        {
            FreeVec(data->ci[i].preparse);
            data->ci[i].preparse = NULL;
        }
        FreeVec(data->ci);
        data->ci = NULL;
    }
    FreeVec(data->preparses);
    data->preparses = NULL;
    if (data->strings_mem)
    {
        FreeVec(data->strings_mem);
        data->strings_mem = NULL;
        data->strings = NULL;
    }
    data->columns = 0;
}

/**************************************************************************
 Parses the given format string (also frees a previously parsed format).
 Use initial=FALSE for format changes outside OM_NEW.
 Return FALSE on failure.
**************************************************************************/
/*
 * Discard old format storage, allocate column/display arrays and parse
 * comma-separated ReadArgs options. Requires live pool/consistent entry
 * records and readable input or NULL. Return FALSE on checked
 * allocation/growth failures without rollback; parse/duplicate failures can
 * leave defaults. Store some options without applying them.
 */
static BOOL ParseListFormat(struct MUI_ListData *data, STRPTR format,
    BOOL initial)
{
    int new_columns, i;
    STRPTR ptr;
    STRPTR format_sep;
    char c;

    IPTR args[ARG_CNT];
    struct RDArgs *rdargs;

    if (!format)
        format = (STRPTR) "";

    ptr = format;

    FreeListFormat(data);

    new_columns = 1;

    /* Count the number of columns first */
    while ((c = *ptr++))
        if (c == ',')
            new_columns++;

    if (!(data->preparses =
            AllocVec((new_columns + 10) * sizeof(STRPTR), MEMF_CLEAR)))
        return FALSE;

    if (!(data->strings_mem = AllocVec((new_columns + 1 + 10)
        * sizeof(STRPTR), MEMF_CLEAR)))
                                  /* hold enough space also for the entry pos,
                                   * used by orginal MUI and also some
                                   * security space */
        return FALSE;
    data->strings=data->strings_mem;

    if (!(data->ci = AllocVec(new_columns * sizeof(struct ColumnInfo), MEMF_CLEAR)))
        return FALSE;

    // set defaults
    for (i = 0; i < new_columns; i++)
    {
        data->ci[i].colno = -1; // -1 means: use unassigned column
        data->ci[i].weight = 100;
        data->ci[i].delta = 4;
        data->ci[i].min_width = -1;
        data->ci[i].max_width = -1;
        data->ci[i].user_width = -1;
        data->ci[i].bar = FALSE;
        data->ci[i].preparse = NULL;
    }

    if ((format_sep = StrDup(format)) != 0)
    {
        for (i = 0; format_sep[i] != '\0'; i++)
        {
            if (format_sep[i] == ',')
                format_sep[i] = '\0';
        }

        if ((rdargs = AllocDosObject(DOS_RDARGS, NULL)) != 0)
        {
            ptr = format_sep;
            i = 0;
            do
            {
                rdargs->RDA_Source.CS_Buffer = ptr;
                rdargs->RDA_Source.CS_Length = strlen(ptr);
                rdargs->RDA_Source.CS_CurChr = 0;
                rdargs->RDA_DAList = 0;
                rdargs->RDA_Buffer = NULL;
                rdargs->RDA_BufSiz = 0;
                rdargs->RDA_ExtHelp = NULL;
                rdargs->RDA_Flags = 0;

                memset(args, 0, sizeof args);
                if (ReadArgs(FORMAT_TEMPLATE, args, rdargs))
                {
                    if (args[ARG_COL])
                        data->ci[i].colno = *(LONG *) args[ARG_COL];
                    if (args[ARG_WEIGHT])
                        data->ci[i].weight = *(LONG *) args[ARG_WEIGHT];
                    if (args[ARG_DELTA])
                        data->ci[i].delta = *(LONG *) args[ARG_DELTA];
                    if (args[ARG_MINWIDTH])
                        data->ci[i].min_width =
                            *(LONG *) args[ARG_MINWIDTH];
                    if (args[ARG_MAXWIDTH])
                        data->ci[i].max_width =
                            *(LONG *) args[ARG_MAXWIDTH];
                    data->ci[i].bar = args[ARG_BAR];
                    if (args[ARG_PREPARSE])
                        data->ci[i].preparse =
                            StrDup((STRPTR) args[ARG_PREPARSE]);

                    FreeArgs(rdargs);
                }
                ptr += strlen(ptr) + 1;
                i++;
            }
            while (i < new_columns);
            FreeDosObject(DOS_RDARGS, rdargs);
        }
        FreeVec(format_sep);
    }

    for (i = 0; i < new_columns; i++)
    {
        D(bug("colno %d weight %d delta %d preparse %s\n",
                data->ci[i].colno, data->ci[i].weight, data->ci[i].delta,
                data->ci[i].preparse));
    }

    if (initial)
    {
        /* called from OM_NEW */
        data->columns_allocated = new_columns;
    }
    else if (data->columns_allocated < new_columns)
    {
        /* called by MUIA_List_Format */
        if (!IncreaseColumns(data, new_columns))
        {
            bug("[Zune:List] not enough memory for new columns!!\n");
            /* FIXME: failure can leave list entries with mixed allocation sizes. */
            return FALSE;
        }
        data->columns_allocated = new_columns;
    }
    data->columns = new_columns;
    /* Skip entry pos (-1) and enough indexes to get (-9). Based on MPlayer source codes. */
    data->strings += 10;

    return TRUE;
}

/**************************************************************************
 Call the MUIM_List_Display for the given entry. It fills out
 data->string and data->preparses
**************************************************************************/
/*
 * Prepare column preparses and dispatch Display for a valid row or title.
 * Direct one-column title bypasses callback; ordinary rows set preceding
 * pointer metadata. Requires allocated arrays and live borrowed text.
 * Return void, no result check or ownership transfer.
 */
static void DisplayEntry(struct IClass *cl, Object *obj, int entry_pos)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    APTR entry_data;
    int col;

    for (col = 0; col < data->columns; col++)
        data->preparses[col] = data->ci[col].preparse;

    if (entry_pos == ENTRY_TITLE)
    {
        if ((data->columns == 1) && (data->title != (STRPTR) 1))
        {
            *data->strings = data->title;
            return;
        }
        entry_data = NULL;      /* it's a title request */
    }
    else
    {
        data->strings[-1] = (STRPTR)(IPTR)entry_pos;
        entry_data = data->entries[entry_pos]->data;
    }

    /* Get the display formation */
    DoMethod(obj, MUIM_List_Display, (IPTR) entry_data,
        (IPTR) data->strings, entry_pos, (IPTR) data->preparses);
}

/**************************************************************************
 Determine the dims of a single entry and adapt the columninfo according
 to it. pos might be ENTRY_TITLE. Returns 0 if pos entry needs to
 be redrawn after this operation, 1 if all entries need to be redrawn.
**************************************************************************/
/*
 * Measure each displayed column and grow entry/shared height/width maxima
 * after setup. Requires valid row/title record and callbacks that preserve
 * list state. Return1 if shared dimensions grew,0 otherwise or before
 * setup. Text allocation failures are skipped; cached entry height is not
 * reset.
 */
static int CalcDimsOfEntry(struct IClass *cl, Object *obj, int pos)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListEntry *entry = data->entries[pos];
    int j;
    int ret = 0;

    if (!entry)
        return ret;

    if (!(_flags(obj) & MADF_SETUP))
        return ret;

    DisplayEntry(cl, obj, pos);

    /* Set height to at least minheight */
    if (data->entries[pos]->height < data->entry_minheight)
        data->entries[pos]->height = data->entry_minheight;

    for (j = 0; j < data->columns; j++)
    {
        ZText *text =
            zune_text_new(data->preparses[j], data->strings[j],
            ZTEXT_ARG_NONE, 0);
        if (text != NULL)
        {
            zune_text_get_bounds(text, obj);

            if (text->height > data->entries[pos]->height)
            {
                data->entries[pos]->height = text->height;
                /* entry height changed, redraw all entries later */
                ret = 1;
            }
            data->entries[pos]->widths[j] = text->width;

            if (text->width > data->ci[j].entries_width)
            {
                /* This entry has a greater width for this column than any
                 * other entry, so we store this value
                 */
                data->ci[j].entries_width = text->width;
                /* column width changed, redraw all entries later */
                ret = 1;
            }

            zune_text_destroy(text);
        }
    }
    if (data->entries[pos]->height > data->entry_maxheight)
    {
        data->entry_maxheight = data->entries[pos]->height;
        /* maximum entry height changed, redraw all entries later */
        ret = 1;
    }

    return ret;
}

/**************************************************************************
 Determine the widths of the entries
**************************************************************************/
/*
 * Reset column/global maxima and measure title plus entries after setup,
 * accumulating heights and widths. Requires valid records and format.
 * Return void; cached per-entry heights can prevent shrinkage, and row
 * maximum is forced to at least1.
 */
static void CalcWidths(struct IClass *cl, Object *obj)
{
    int i, j;
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (!(_flags(obj) & MADF_SETUP))
        return;

    for (j = 0; j < data->columns; j++)
        data->ci[j].entries_width = 0;

    data->entry_maxheight = 0;
    data->entries_totalheight = 0;
    data->entries_maxwidth = 0;

    for (i = (data->title ? ENTRY_TITLE : 0); i < data->entries_num; i++)
    {
        CalcDimsOfEntry(cl, obj, i);
        data->entries_totalheight += data->entries[i]->height;
    }

    for (j = 0; j < data->columns; j++)
        data->entries_maxwidth += data->ci[j].entries_width
            + data->ci[j].delta + (data->ci[j].bar ? BAR_WIDTH : 0);

    if (!data->entry_maxheight)
        data->entry_maxheight = 1;
}

/**************************************************************************
 Calculates the number of visible entry lines. Returns 1 if it has
 changed
**************************************************************************/
/*
 * Calculate full visible rows and centered row origin from area
 * height/title/maxheight. Requires layout geometry and positive maxheight.
 * Notify changed visible count through superclass. Return whether
 * count/origin changed; no count-range validation.
 */
static int CalcVertVisible(struct IClass *cl, Object *obj)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int old_entries_visible = data->entries_visible;
    int old_entries_top_pixel = data->entries_top_pixel;

    data->vertprop_visible = data->entries_visible =
        (_mheight(data->area) - data->title_height)
        / (data->entry_maxheight /* + data->prefs_linespacing */ );

    /* Distribute extra vertical space evenly between top and bottom of
     * list */

    data->entries_top_pixel = _mtop(data->area) + data->title_height
        + (_mheight(data->area) - data->title_height
        -
        data->entries_visible *
        (data->entry_maxheight /* + data->prefs_linespacing */ )) / 2;

    if (data->entries_visible != old_entries_visible)
    {
        superset(cl, obj, MUIA_List_Visible, data->entries_visible);
        superset(cl, obj, MUIA_List_VertProp_Visible, data->entries_visible);
    }

    return (old_entries_visible != data->entries_visible)
        || (old_entries_top_pixel != data->entries_top_pixel);
}

/**************************************************************************
 Resize arrays, so that there is enough space for new columns.
 The widths[] array must grow with increasing columns.
 columns_allocated must be checked, before calling this function.
 Space can only grow, not shrink.
 Return FALSE on error (no memory).
**************************************************************************/
/*
 * Grow pooled width-bearing records one by one, including title only when
 * enabled. Requires larger requested capacity and consistent records/pool.
 * Return FALSE on allocation failure after possible partial replacement,
 * TRUE otherwise; no rollback. Hidden title record can remain at older
 * capacity.
 */
static BOOL IncreaseColumns(struct MUI_ListData *data, int new_columns)
{
    int i = 0;
    IPTR newsize, oldsize;
    struct ListEntry *le;

    D(bug("IncreaseColumns: %d => %d columns\n", data->columns_allocated, new_columns));

    newsize = sizeof(struct ListEntry) + sizeof(LONG) * (new_columns             + 1);
    oldsize = sizeof(struct ListEntry) + sizeof(LONG) * (data->columns_allocated + 1);

    if (data->title)
    {
        i = -1;
    }

    for (; i < data->entries_num; i++)
    {
        D(bug("IncreaseColumns: i: %d, size: %d => %d\n", i, oldsize, newsize));
        le = (struct ListEntry *) AllocVecPooled(data->pool, newsize);
        if (!le)
        {
            return FALSE;
        }
        memset(le, 0, newsize);
        D(bug("IncreaseColumns: CopyMem(%p, %p, %d)\n", data->entries[i],
            le, oldsize));
        CopyMem(data->entries[i], le, oldsize);
        FreeVecPooled(data->pool, data->entries[i]);
        data->entries[i]=le;
    }

    return TRUE;
}

/**************************************************************************
 Default hook to compare two list entries. Works for strings only.
**************************************************************************/
/*
 * Compare message entry1 against object entry2 with Stricmp. Requires both
 * readable terminated strings and hook ABI. Return signed ordering; no
 * allocation, mutation or lifetime extension.
 */
AROS_UFH3S(int, default_compare_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char *, s2, A2),
    AROS_UFHA(char *, s1, A1))
{
    AROS_USERFUNC_INIT

    return Stricmp(s1, s2);

    AROS_USERFUNC_EXIT
}

#define PROP_VERT_FIRST   1

/*
 * Handle vertical-scrollbar callback by reading its current First and
 * applying List_VertProp_First with NoNotifySet. Requires live hook
 * data/List/scrollbar and two-word message. Always0; other callback types
 * ignored.
 */
static ULONG List_Function(struct Hook *hook, Object * obj, void **msg)
{
    struct MUI_ListData *data = (struct MUI_ListData *)hook->h_Data;
    SIPTR type = (SIPTR) msg[0];
    SIPTR val = (SIPTR) msg[1];

    switch (type)
    {
    case PROP_VERT_FIRST:
        get(data->vert, MUIA_Prop_First, &val);
        nnset(obj, MUIA_List_VertProp_First, val);
        break;
    }
    return 0;
}

/* At entry to this function, data->area is always set, but data->vert may
 * or may not be set */
/*
 * Before setup, remove/reinsert drawing area and optional scrollbar in
 * requested order and install reciprocal notifications. Requires live Group
 * children; creates/disposes scrollbar as needed. Return void; setup state
 * causes immediate no-op, allocation/member results not comprehensively
 * checked.
 */
static void List_HandleScrollerPos(struct IClass *cl, Object *obj)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    BOOL vert_not_used = FALSE;

    /* Disallow any changes after setup. This function should basically be
     * creation-time only */
    if (_flags(obj) & MADF_SETUP)
        return;

    /* Remove both objects */
    if (data->area_connected)
        DoMethod(obj, OM_REMMEMBER, data->area);
    if (data->vert_connected)
        DoMethod(obj, OM_REMMEMBER, data->vert);

    /* Add list and/or scroller */
    switch (data->scroller_pos)
    {
    case MUIV_Listview_ScrollerPos_None:
        vert_not_used = TRUE;
        DoMethod(obj, OM_ADDMEMBER, data->area);
        break;
    case MUIV_Listview_ScrollerPos_Left:
        if (!data->vert)
            data->vert =ScrollbarObject, MUIA_Group_Horiz, FALSE, End;
        DoMethod(obj, OM_ADDMEMBER, data->vert);
        DoMethod(obj, OM_ADDMEMBER, data->area);
        break;
    default:
        if (!data->vert)
            data->vert = ScrollbarObject, MUIA_Group_Horiz, FALSE, End;
        DoMethod(obj, OM_ADDMEMBER, data->area);
        DoMethod(obj, OM_ADDMEMBER, data->vert);
        break;
    }

    data->area_connected = TRUE;

    /* Handle case where it was decided that vert will not be used */
    if (vert_not_used)
    {
        if (data->vert)
        {
            if (data->vert_connected)
            {
                DoMethod(obj, MUIM_KillNotifyObj, MUIA_List_VertProp_First,
                    (IPTR) data->vert);
                DoMethod(obj, MUIM_KillNotifyObj, MUIA_List_VertProp_Visible,
                    (IPTR) data->vert);
                DoMethod(obj, MUIM_KillNotifyObj, MUIA_List_VertProp_Entries,
                    (IPTR) data->vert);
                data->vert_connected = FALSE;
            }

            MUI_DisposeObject(data->vert);
            data->vert = NULL;
        }
    }

    /* If at this point data->vert is not null, it means vert is to be
     * connected */
    if (data->vert && !data->vert_connected)
    {
        LONG entries = 0, first = 0, visible = 0;

        get(obj, MUIA_List_VertProp_First, &first);
        get(obj, MUIA_List_VertProp_Visible, &visible);
        get(obj, MUIA_List_VertProp_Entries, &entries);

        SetAttrs(data->vert, MUIA_Prop_First, first,
            MUIA_Prop_Visible, visible, MUIA_Prop_Entries, entries, TAG_DONE);

        DoMethod(data->vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime,
            (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->hook, PROP_VERT_FIRST,
            MUIV_TriggerValue);

        /* Pass prop object as DestObj (based on code in NList) */
        DoMethod(obj, MUIM_Notify, MUIA_List_VertProp_First, MUIV_EveryTime,
            (IPTR) data->vert, 3, MUIM_NoNotifySet,
            MUIA_Prop_First, MUIV_TriggerValue);
        DoMethod(obj, MUIM_Notify, MUIA_List_VertProp_Visible, MUIV_EveryTime,
            (IPTR) data->vert, 3, MUIM_NoNotifySet,
            MUIA_Prop_Visible, MUIV_TriggerValue);
        DoMethod(obj, MUIM_Notify, MUIA_List_VertProp_Entries, MUIV_EveryTime,
            (IPTR) data->vert, 3, MUIM_NoNotifySet,
            MUIA_Prop_Entries, MUIV_TriggerValue);

        data->vert_connected = TRUE;
    }
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Create horizontal Group, transfer Frame tag to drawing area, parse
 * borrowed policies and create pool/format/title storage. Insert
 * SourceArray then initialize Active and image list. Return object or0 for
 * checked failures; subordinate and insertion failures are not fully
 * propagated. Mutates original Frame tag to TAG_IGNORE.
 */
IPTR List__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListData *data;
    struct TagItem *tag;
    struct TagItem *tags;
    APTR *array = NULL;
    LONG new_entries_active = MUIV_List_Active_Off;
    struct TagItem rectattrs[2] =
        {{TAG_IGNORE, TAG_IGNORE }, {TAG_DONE, TAG_DONE}};
    Object *area;

    /* search for MUIA_Frame as it has to be passed to rectangle object */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        if (tag->ti_Tag == MUIA_Frame)
        {
            rectattrs[0].ti_Tag = MUIA_Frame;
            rectattrs[0].ti_Data = tag->ti_Data;
            tag->ti_Tag = TAG_IGNORE;
            break;
        }
    }

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Horiz, TRUE,
        MUIA_InnerLeft, 0,
        MUIA_InnerRight, 0,
        MUIA_Group_Spacing, 0,
        MUIA_Font, MUIV_Font_List,
        MUIA_ShowSelState, FALSE,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Background, MUII_ListBack,
        TAG_MORE, (IPTR) msg->ops_AttrList,
        TAG_DONE);

    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->columns = 1;
    data->entries_active = MUIV_List_Active_Off;
    data->intern_puddle_size = 2008;
    data->intern_thresh_size = 1024;
    data->default_compare_hook.h_Entry = (HOOKFUNC) default_compare_func;
    data->default_compare_hook.h_SubEntry = 0;
    data->compare_hook = &(data->default_compare_hook);
    data->flags = LIST_SHOWDROPMARKS;
    data->area_replaced = FALSE;
    data->area_connected = FALSE;
    data->vert_connected = FALSE;

    data->entries_visible = data->vertprop_visible = -1;
    data->last_active = -1;
    data->drop_mark = 0;

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    data->mouse_click = 0;
    data->mouse_x = MUI_MAXMAX;
    data->mouse_y = MUI_MAXMAX;

    /* HACK:
     * List is a group where part of area is rendered and part is filled
     * with other objects (inside of List dimensions). One such object is
     * up/down arrow. This object depends on RelVerify mode to control
     * behaviour. List also has the RelVerify mode. Area super class in case
     * of both of those objects adds an event handler node with the same
     * priority. Depending on the sort order, the event handler node of
     * "list" can eat a click event and up/down arrows stop working. The
     * hack is to decrease the priority of Area event handler for list to
     * always favor up/down arrow. There are other hacky ways of solving
     * this, but this seems least evil approach, as this hack is
     * encapsulated in the List class itself.
     */
    muiAreaData(obj)->mad_ehn.ehn_Priority--;

    data->hook.h_Entry = HookEntry;
    data->hook.h_SubEntry = (HOOKFUNC) List_Function;
    data->hook.h_Data = data;

    area = (Object *)GetTagData(MUIA_List_ListArea, (IPTR) 0,
        msg->ops_AttrList);

    if (!area)
        area = RectangleObject, MUIA_FillArea, FALSE, TAG_MORE,
            (IPTR) rectattrs, End;
    else
        data->area_replaced = TRUE;
    data->area = area;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_List_Active:
            new_entries_active = tag->ti_Data;
            break;

        case MUIA_List_Pool:
            data->pool = (APTR) tag->ti_Data;
            break;

        case MUIA_List_PoolPuddleSize:
            data->intern_puddle_size = tag->ti_Data;
            break;

        case MUIA_List_PoolThreshSize:
            data->intern_thresh_size = tag->ti_Data;
            break;

        case MUIA_List_CompareHook:
            data->compare_hook = (struct Hook *)tag->ti_Data;
            if (data->compare_hook == NULL)
                data->compare_hook = &data->default_compare_hook;
            break;

        case MUIA_List_ConstructHook:
            data->construct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DestructHook:
            data->destruct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DisplayHook:
            data->display_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_MultiTestHook:
            data->multi_test_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_SourceArray:
            array = (APTR *) tag->ti_Data;
            break;

        case MUIA_List_Format:
            data->format = (STRPTR) tag->ti_Data;
            break;

        case MUIA_List_Title:
            data->title = (STRPTR) tag->ti_Data;
            break;

        case MUIA_List_MinLineHeight:
            data->entry_minheight = tag->ti_Data;
            break;

        case MUIA_List_AdjustHeight:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_ADJUSTHEIGHT);
            break;

        case MUIA_List_AdjustWidth:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_ADJUSTWIDTH);
            break;

        case MUIA_List_AutoVisible:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_AUTOVISIBLE);
            break;

        case MUIA_List_ShowDropMarks:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_SHOWDROPMARKS);
            break;

        case MUIA_List_DragSortable:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_DRAGSORTABLE);
            break;

        case MUIA_Listview_ScrollerPos:
            data->scroller_pos = tag->ti_Data;
            break;

        case MUIA_Listview_Input:
            data->read_only = !tag->ti_Data;
            break;

        case MUIA_Listview_MultiSelect:
            data->multiselect = tag->ti_Data;
            break;

        case MUIA_Listview_DefClickColumn:
            data->def_click_column = tag->ti_Data;
            break;

        case MUIA_Listview_DragType:
            data->drag_type = tag->ti_Data;
            if (data->drag_type != MUIV_Listview_DragType_None)
                set(obj, MUIA_Draggable, TRUE);
            break;
        }
    }

    List_HandleScrollerPos(cl, obj);

    if (!data->pool)
    {
        /* No memory pool given, so we create our own */
        data->pool = data->intern_pool =
            CreatePool(0, data->intern_puddle_size,
            data->intern_thresh_size);
        if (!data->pool)
        {
            CoerceMethod(cl, obj, OM_DISPOSE);
            return 0;
        }
    }

    /* parse the list format */
    if (!ParseListFormat(data, data->format, TRUE))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    /* This is necessary for at least the title */
    if (!SetListSize(data, 0))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (!(data->entries[ENTRY_TITLE] = AllocListEntry(data)))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (array)
    {
        int i;
        /* Count the number of elements */
        for (i = 0; array[i] != NULL; i++)
            ;
        /* Insert them */
        DoMethod(obj, MUIM_List_Insert, (IPTR) array, i,
            MUIV_List_Insert_Top);
    }

    if ((data->entries_num) && (new_entries_active != MUIV_List_Active_Off))
    {
        switch (new_entries_active)
        {
        case MUIV_List_Active_Top:
            new_entries_active = 0;
            break;

        case MUIV_List_Active_Bottom:
            new_entries_active = data->entries_num - 1;
            break;
        }

        if (new_entries_active < 0)
            new_entries_active = 0;
        else if (new_entries_active >= data->entries_num)
            new_entries_active = data->entries_num - 1;

        data->entries_active = new_entries_active;
        /* Selected entry will be moved into visible area */
    }

    NewList((struct List *)&data->images);

    D(bug("List_New(0x%p)\n", obj));

    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/*
 * Destruct confirmed entries in reverse order, free records/pointer array
 * and format, delete only internal pool, then delegate Group. Requires
 * callbacks/lifetimes valid and images explicitly removed. Return
 * superclass result; external pool retains title record and image objects
 * are not disposed here.
 */
IPTR List__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    D(bug("List Dispose\n"));

    /* Call destruct method for every entry and free the entries manually
     * to avoid notification */
    while (data->confirm_entries_num)
    {
        struct ListEntry *lentry =
            data->entries[--data->confirm_entries_num];
        DoMethod(obj, MUIM_List_Destruct, (IPTR) lentry->data,
            (IPTR) data->pool);
        FreeListEntry(data, lentry);
    }

    if (data->intern_pool)
        DeletePool(data->intern_pool);
    if (data->entries)
        FreeVec(data->entries - 1);
            /* title is currently before all other elements */

    FreeListFormat(data);

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 OM_SET
**************************************************************************/
/*
 * Apply local policies/state, redraw/synchronize scrollbars and delegate
 * Group with forwarding semantics. Requires valid live records and mutable
 * tags; Selected is ignored. Some public read-only attributes have internal
 * setters; Format failure ignored. Active can touch stale old records after
 * removal; return superclass result.
 */
IPTR List__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct TagItem *tag;
    struct TagItem *tags;

    /* parse taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_List_CompareHook:
            data->compare_hook = (struct Hook *)tag->ti_Data;
            if (data->compare_hook == NULL)
                data->compare_hook = &data->default_compare_hook;
            break;

        case MUIA_List_ConstructHook:
            data->construct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DestructHook:
            data->destruct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DisplayHook:
            data->display_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_MultiTestHook:
            data->multi_test_hook = (struct Hook *)tag->ti_Data;
            if (data->multi_test_hook != NULL)
            {
                /* Clearing current selections is the easiest way to keep
                 * selections consistent with the new hook */
                DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All,
                    MUIV_List_Select_Off, NULL);
            }
            break;

        case MUIA_List_Title:
            data->title = (STRPTR) tag->ti_Data;
            DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
            break;

        case MUIA_List_VertProp_First:
            data->vertprop_first = tag->ti_Data;
            if (data->entries_first != tag->ti_Data)
            {
                set(obj, MUIA_List_First, tag->ti_Data);
            }
            break;

        case MUIA_List_Format:
            data->format = (STRPTR) tag->ti_Data;
            ParseListFormat(data, data->format, FALSE);
            // FIXME: ParseListFormat() failure is ignored.
            DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
            break;

        case MUIA_List_VertProp_Entries:
            data->vertprop_entries = tag->ti_Data;
            break;

        case MUIA_List_VertProp_Visible:
            data->vertprop_visible = tag->ti_Data;
            data->entries_visible = tag->ti_Data;
            break;

        case MUIA_List_Active:
            {
                LONG new_entries_active = tag->ti_Data;

                if ((data->entries_num)
                    && (new_entries_active != MUIV_List_Active_Off))
                {
                    switch (new_entries_active)
                    {
                    case MUIV_List_Active_Top:
                        new_entries_active = 0;
                        break;

                    case MUIV_List_Active_Bottom:
                        new_entries_active = data->entries_num - 1;
                        break;

                    case MUIV_List_Active_Up:
                        new_entries_active = data->entries_active - 1;
                        break;

                    case MUIV_List_Active_Down:
                        new_entries_active = data->entries_active + 1;
                        break;

                    case MUIV_List_Active_PageUp:
                        new_entries_active =
                            data->entries_active - data->entries_visible;
                        break;

                    case MUIV_List_Active_PageDown:
                        new_entries_active =
                            data->entries_active + data->entries_visible;
                        break;
                    }

                    if (new_entries_active < 0)
                        new_entries_active = 0;
                    else if (new_entries_active >= data->entries_num)
                        new_entries_active = data->entries_num - 1;
                }
                else
                    new_entries_active = -1;

                if (data->entries_active != new_entries_active)
                {
                    LONG old = data->entries_active;
                    data->entries_active = new_entries_active;

                    /* SelectChange stuff */
                    if (new_entries_active != -1)
                    {
                        DoMethod(obj, MUIM_List_SelectChange,
                            new_entries_active, MUIV_List_Select_On, 0);
                        DoMethod(obj, MUIM_List_SelectChange,
                            new_entries_active, MUIV_List_Select_Active, 0);
                    }
                    else
                        DoMethod(obj, MUIM_List_SelectChange,
                            MUIV_List_Active_Off, MUIV_List_Select_Off, 0);

                    if (!data->read_only)
                    {
                        data->entries[old]->flags |= ENTRY_RENDER;
                        if (!(data->flags & LIST_QUIET))
                        {
                            data->update = UPDATEMODE_ENTRY;
                            data->update_pos = old;
                            MUI_Redraw(obj, MADF_DRAWUPDATE);
                        }
                        data->entries[data->entries_active]->flags |= ENTRY_RENDER;
                        if (!(data->flags & LIST_QUIET))
                        {
                            data->update = UPDATEMODE_ENTRY;
                            data->update_pos = data->entries_active;
                            MUI_Redraw(obj, MADF_DRAWUPDATE);
                        }
                        else
                        {
                            data->update = UPDATEMODE_NEEDED;
                            data->flags |= LIST_CHANGED;
                        }
                    }

                    /* Make new active entry visible (if there is one and
                       list is visible) */
                    if (new_entries_active != -1
                        && (_flags(obj) & MADF_SETUP))
                    {
                        DoMethod(obj, MUIM_List_Jump,
                            MUIV_List_Jump_Active);
                    }
                }
            }
            break;

        case MUIA_List_First:
            data->update_pos = data->entries_first;
            data->entries_first = tag->ti_Data;
            if (!(data->flags & LIST_QUIET))
            {
                data->update = (UPDATEMODE_ENTRY|UPDATEMODE_ALL);
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                if (data->vertprop_first != tag->ti_Data)
                {
                    set(obj, MUIA_List_VertProp_First, tag->ti_Data);
                }
            }
            else
            {
                data->update = UPDATEMODE_ALL;
                data->flags |= LIST_CHANGED;
            }
            break;

        case MUIA_List_Visible:    /* Shouldn't be settable? */
            if (data->vertprop_visible != tag->ti_Data)
                set(obj, MUIA_List_VertProp_Visible, tag->ti_Data);
            break;

        case MUIA_List_Entries:
            if (data->confirm_entries_num == tag->ti_Data)
            {
                data->entries_num = tag->ti_Data;
                if (!(data->flags & LIST_QUIET))
                {
                    set(obj, MUIA_List_VertProp_Entries, data->entries_num);
                }
            }
            else
            {
                D(bug("Bug: confirm_entries != MUIA_List_Entries!\n"));
            }
            break;

        case MUIA_List_Quiet:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_QUIET);
            if (!(data->flags & LIST_QUIET))
            {
                if (data->flags & LIST_CHANGED)
                {
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                    if (data->entries_num != XGET(obj, MUIA_List_VertProp_Entries))
                        set(obj, MUIA_List_VertProp_Entries, data->entries_num);
                    if (data->entries_first != XGET(obj, MUIA_List_VertProp_First))
                        set(obj, MUIA_List_VertProp_First, data->entries_first);
                }
                data->flags &= ~LIST_CHANGED;
            }
            break;

        case MUIA_List_AutoVisible:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_AUTOVISIBLE);
            break;

        case MUIA_List_ShowDropMarks:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_SHOWDROPMARKS);
            break;

        case MUIA_List_DragSortable:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_DRAGSORTABLE);
            break;

        case MUIA_Selected:
            /* Swallow this so the Area class doesn't redraw us */
            tag->ti_Tag = TAG_IGNORE;
            break;

        case MUIA_Disabled:
            if (_flags(obj) & MADF_SETUP)
            {
                /* Stop listening for events we only listen to when mouse
                   button is down: we will not be informed of the button
                   being released */
                DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                    (IPTR) &data->ehn);
                data->ehn.ehn_Events &= ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS
                    | IDCMP_INACTIVEWINDOW);
                DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                    (IPTR) &data->ehn);
            }
            break;

        case MUIA_Listview_DoubleClick: /* private set */
            data->doubleclick = tag->ti_Data != 0;
            break;

        case MUIA_Listview_ScrollerPos: /* private set */
            data->scroller_pos = tag->ti_Data;
            List_HandleScrollerPos(cl, obj);
            break;

        case MUIA_Listview_Input: /* private set */
            data->read_only = !tag->ti_Data;
            break;

        case MUIA_Listview_MultiSelect: /* private set */
            data->multiselect = tag->ti_Data;
            break;

        case MUIA_Listview_DefClickColumn:
            data->def_click_column = tag->ti_Data;
            break;

        case MUIA_Listview_DragType:
            data->drag_type = tag->ti_Data;
            set(obj, MUIA_Draggable,
                tag->ti_Data != MUIV_Listview_DragType_None);
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Write local counts, indices, borrowed pointers and Listview compatibility
 * values to writable IPTR storage. Return1 for handled/superclass success,0
 * otherwise. Boolean flags are masks; Listview_List returns self and
 * SelectChange returnsFALSE. Does not transfer ownership.
 */
IPTR List__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_ListData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_List_Entries:
        STORE = data->entries_num;
        return 1;
    case MUIA_List_First:
        STORE = data->entries_first;
        return 1;
    case MUIA_List_Active:
        STORE = data->entries_active;
        return 1;
    case MUIA_List_InsertPosition:
        STORE = data->insert_position;
        return 1;
    case MUIA_List_Title:
        STORE = (IPTR) data->title;
        return 1;
    case MUIA_List_VertProp_Entries:
        STORE = data->vertprop_entries;
        return 1;
    case MUIA_List_VertProp_Visible:
    case MUIA_List_Visible:
        STORE = data->vertprop_visible;
        return 1;
    case MUIA_List_VertProp_First:
        STORE = data->vertprop_first;
        return 1;
    case MUIA_List_Format:
        STORE = (IPTR) data->format;
        return 1;
    case MUIA_List_AutoVisible:
        STORE = data->flags & LIST_AUTOVISIBLE;
        return 1;
    case MUIA_List_ShowDropMarks:
        STORE = data->flags & LIST_SHOWDROPMARKS;
        return 1;
    case MUIA_List_DragSortable:
        STORE = data->flags & LIST_DRAGSORTABLE;
        return 1;
    case MUIA_List_DropMark:
        STORE = data->drop_mark;
        return 1;
    case MUIA_Listview_ClickColumn:
        STORE = data->click_column;
        return 1;
    case MUIA_Listview_DoubleClick:
        STORE = data->doubleclick;
        return 1;
    case MUIA_Listview_SelectChange:
        STORE = FALSE;
        return 1;
    case MUIA_Listview_List:
        STORE = (IPTR)obj;
        return 1;
    case MUIA_Listview_DefClickColumn:
        STORE = data->def_click_column;
        return 1;
    case MUIA_Listview_DragType:
        STORE = data->drag_type;
        return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg))
        return 1;
    return 0;
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Require superclass setup, acquire highlight image specs/preferences,
 * connect/setup registered images and add window handler. Return0 only for
 * superclass failure,1 otherwise; image/setup outcomes ignored. Requires
 * live Window and registered Area objects.
 */
IPTR List__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return 0;

    data->prefs_refresh = muiGlobalInfo(obj)->mgi_Prefs->list_refresh;
    data->prefs_linespacing =
        muiGlobalInfo(obj)->mgi_Prefs->list_linespacing;
    data->prefs_smoothed = muiGlobalInfo(obj)->mgi_Prefs->list_smoothed;
    data->prefs_smoothval = muiGlobalInfo(obj)->mgi_Prefs->list_smoothval;

    data->list_cursor =
        zune_imspec_setup(MUII_ListCursor, muiRenderInfo(obj));
    data->list_select =
        zune_imspec_setup(MUII_ListSelect, muiRenderInfo(obj));
    data->list_selcur =
        zune_imspec_setup(MUII_ListSelCur, muiRenderInfo(obj));

    data->prefs_multi = muiGlobalInfo(obj)->mgi_Prefs->list_multi;
    if (data->multiselect == MUIV_Listview_MultiSelect_Default)
    {
        if (data->prefs_multi == LISTVIEW_MULTI_SHIFTED)
            data->multiselect = MUIV_Listview_MultiSelect_Shifted;
        else
            data->multiselect = MUIV_Listview_MultiSelect_Always;
    }

    struct ListImage *li;
    ForeachNode((struct List *)&data->images, li) {
        DoMethod(li->obj, MUIM_ConnectParent, (IPTR) obj);
        DoSetupMethod(li->obj, muiRenderInfo(obj));
    }

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->ehn);

    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Release highlight specs, remove event handler, reset mouse click and
 * delegate Group cleanup. Requires matching setup. Return superclass
 * result; does not iterate registered image handles for
 * cleanup/disconnection.
 */
IPTR List__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    zune_imspec_cleanup(data->list_cursor);
    zune_imspec_cleanup(data->list_select);
    zune_imspec_cleanup(data->list_selcur);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->ehn);
    data->mouse_click = 0;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
/*
 * Delegate Group and measure contents, then add adjusted or default
 * width/height contributions. Requires setup font/valid limits and
 * callbacks. ReturnTRUE; no general parent relayout or overflow protection.
 */
IPTR List__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    CalcWidths(cl, obj);

    if ((data->flags & LIST_ADJUSTWIDTH) && (data->entries_num > 0))
    {
        msg->MinMaxInfo->MinWidth += data->entries_maxwidth;
        msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
        msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;
    }
    else
    {
        msg->MinMaxInfo->MinWidth += 40;
        msg->MinMaxInfo->DefWidth += 100;
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    }

    if (data->entries_num > 0)
    {
        if (data->flags & LIST_ADJUSTHEIGHT)
        {
            msg->MinMaxInfo->MinHeight += data->entries_totalheight;
            msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
            msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
        }
        else
        {
            ULONG h = data->entry_maxheight + data->prefs_linespacing;
            msg->MinMaxInfo->MinHeight += 2 * h + data->prefs_linespacing;
            msg->MinMaxInfo->DefHeight += 8 * h + data->prefs_linespacing;
            msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
        }
    }
    else
    {
        msg->MinMaxInfo->MinHeight += 36;
        msg->MinMaxInfo->DefHeight += 96;
        msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    D(bug("List %p minheight=%d, line maxh=%d\n",
            obj, msg->MinMaxInfo->MinHeight, data->entry_maxheight));

    return TRUE;
}


/*
 * Delegate Group, calculate visible rows, optionally adjust active bottom
 * visibility and bound First, then synchronize scrollbar Visible. Requires
 * positive measured row height. Return superclass result; does not
 * symmetrically enforce active-above-first visibility.
 */
IPTR List__MUIM_Layout(struct IClass *cl, Object *obj,
    struct MUIP_Layout *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);
    LONG new_entries_first = data->entries_first;

    /* Calc the numbers of entries visible */
    CalcVertVisible(cl, obj);

    /* Ensure active entry is visible if requested */
    if (data->entries_active + 1 >=
        (data->entries_first + data->entries_visible)
        && (data->flags & LIST_AUTOVISIBLE) != 0)
        new_entries_first =
            data->entries_active - data->entries_visible + 1;

    /* Ensure there are no unnecessary empty lines */
    if ((new_entries_first + data->entries_visible >=
            data->entries_num)
        && (data->entries_visible <= data->entries_num))
        new_entries_first = data->entries_num - data->entries_visible;

    /* Always show the start of the list if it isn't long enough to fill the
       view */
    if (data->entries_num <= data->entries_visible)
        new_entries_first = 0;

    if (new_entries_first < 0)
        new_entries_first = 0;

    set(obj, new_entries_first != data->entries_first ?
        MUIA_List_First : TAG_IGNORE, new_entries_first);

    /* So the notify happens */
    set(obj, MUIA_List_VertProp_Visible, data->entries_visible);

    return rc;
}


/**************************************************************************
 MUIM_Show
**************************************************************************/
/*
 * Delegate Group then show three highlight image specs. Requires successful
 * setup and valid render state. Return superclass result; no entry-data
 * ownership change or separate image-handle traversal.
 */
IPTR List__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);

    zune_imspec_show(data->list_cursor, obj);
    zune_imspec_show(data->list_select, obj);
    zune_imspec_show(data->list_selcur, obj);
    return rc;
}


/**************************************************************************
 MUIM_Hide
**************************************************************************/
/*
 * Hide three highlight specs then delegate Group. Requires matched
 * setup/show lifecycle. Return superclass result; entries and registered
 * image handles remain retained.
 */
IPTR List__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    zune_imspec_hide(data->list_cursor);
    zune_imspec_hide(data->list_select);
    zune_imspec_hide(data->list_selcur);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/**************************************************************************
 Draw an entry at entry_pos at the given row. To draw the title, set pos to
 ENTRY_TITLE
**************************************************************************/
/*
 * Dispatch display and draw each column at measured widths with centered
 * row text. Requires setup graphics, valid index/title and stable callback
 * strings. Return void; text chunks are temporary and freed, failed text
 * allocation skipped.
 */
static VOID List_DrawEntry(struct IClass *cl, Object *obj, int entry_pos,
    int y)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListEntry *entry = data->entries[entry_pos];
    int row_height = data->entry_maxheight;
    int y_offset = y;
    int col, x1, x2;

    /* To be sure we don't draw anything if there is no title */
    if (entry_pos == ENTRY_TITLE) {
        if (!data->title)
            return;
        row_height = entry ? entry->height : 0;
    }

    DisplayEntry(cl, obj, entry_pos);
    x1 = _mleft(data->area);

    for (col = 0; col < data->columns; col++) {
        ZText *text;
        x2 = x1 + data->ci[col].entries_width;

        if ((text =
                zune_text_new(data->preparses[col], data->strings[col],
                    ZTEXT_ARG_NONE, 0))) {
            /*
             * We need the bounds so that embedded
             * images are (correctly) rendered and to
             * vertically align the entry.
             */
            zune_text_get_bounds(text, obj);
            y_offset = y + (row_height - text->height) / 2;
            if (y_offset < y)
                y_offset = y;
            /* Note, this was MPEN_SHADOW before */
            SetAPen(_rp(obj), muiRenderInfo(obj)->mri_Pens[MPEN_TEXT]);

            zune_text_draw(text, obj, x1, x2, y_offset);
            zune_text_destroy(text);
        }
        x1 = x2 + data->ci[col].delta + (data->ci[col].bar ? BAR_WIDTH : 0);
    }
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
/*
 * Skip all drawing when Quiet; otherwise delegate Group and draw measured
 * rows/title/highlights unless area replaced. Requires valid scroll indices
 * and setup graphics. Maintain clipping/scroll damage refresh and clear
 * pending state. Return0 after local drawing or superclass result on
 * replacement path.
 */
IPTR List__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int entry_pos, y;
    APTR clip;
    int start, end;
    BOOL scroll_caused_damage = FALSE;
    struct MUI_ImageSpec_intern *highlight;
    IPTR ret = (IPTR)0;

    D(bug("[Zune:List] %s()\n", __func__);)

    if (data->flags & LIST_QUIET)
        return ret;

    D(
        bug("[Zune:List] %s: Rendering...\n", __func__);
        bug("[Zune:List] %s: update = %d\n", __func__, data->update);
    )

    ret = DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->area_replaced)
        return ret;

    /* Calculate the title height */
    if (data->title)
    {
        data->title_height = data->entries[ENTRY_TITLE]->height + 2;
    }
    else
    {
        data->title_height = 0;
    }

    /* Calc the numbers of entries visible */
    CalcVertVisible(cl, obj);

    if ((msg->flags & MADF_DRAWUPDATE) == 0 || data->update == UPDATEMODE_ALL)
    {
        DoMethod(obj, MUIM_DrawBackground, _mleft(data->area),
            _mtop(data->area), _mwidth(data->area), _mheight(data->area),
            0, data->entries_first * data->entry_maxheight, 0);
    }

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(data->area),
        _mtop(data->area), _mwidth(data->area), _mheight(data->area));

    if ((msg->flags & MADF_DRAWUPDATE) == 0 || data->update == UPDATEMODE_ALL)
    {
        y = _mtop(data->area);
        /* Draw Title
         */
        if (data->title_height && data->title)
        {
            List_DrawEntry(cl, obj, ENTRY_TITLE, y);
            y += data->entries[ENTRY_TITLE]->height;
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
            Move(_rp(obj), _mleft(data->area), y);
            Draw(_rp(obj), _mright(data->area), y);
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
            y++;
            Move(_rp(obj), _mleft(data->area), y);
            Draw(_rp(obj), _mright(data->area), y);
        }
    }

    y = data->entries_top_pixel;

    start = data->entries_first;
    end = data->entries_first + data->entries_visible;

    if ((msg->flags & MADF_DRAWUPDATE) && data->update == (UPDATEMODE_ENTRY|UPDATEMODE_ALL))
    {
        int diffy = data->entries_first - data->update_pos;
        int top, bottom;
        if (abs(diffy) < data->entries_visible)
        {
            scroll_caused_damage =
                (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;

            ScrollRaster(_rp(obj), 0, diffy * data->entry_maxheight,
                _mleft(data->area), y,
                _mright(data->area),
                y + data->entry_maxheight * data->entries_visible);

            scroll_caused_damage =
                scroll_caused_damage
                && (_rp(obj)->Layer->Flags & LAYERREFRESH);

            if (diffy > 0)
            {
                start = end - diffy;
                y += data->entry_maxheight * (data->entries_visible -
                    diffy);
            }
            else
                end = start - diffy;
        }

        top = y;
        bottom = y + (end - start) * data->entry_maxheight;

        DoMethod(obj, MUIM_DrawBackground, _mleft(data->area), top,
            _mwidth(data->area), bottom - top + 1, 0,
            top - _mtop(data->area) + data->entries_first
            * data->entry_maxheight, 0);
    }

    for (entry_pos = start;
        entry_pos < end && entry_pos < data->entries_num; entry_pos++)
    {
        struct ListEntry *entry = data->entries[entry_pos];

        if (!(msg->flags & MADF_DRAWUPDATE) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_ALL) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == (UPDATEMODE_ENTRY|UPDATEMODE_ALL)) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_ENTRY
                && data->update_pos == entry_pos) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_NEEDED
                && (entry->flags & ENTRY_RENDER)))
        {
            /* Choose appropriate highlight image */

            if (entry_pos == data->entries_active
                && (entry->flags & ENTRY_SELECTED) && !data->read_only)
                highlight = data->list_selcur;
            else if (entry_pos == data->entries_active && !data->read_only)
                highlight = data->list_cursor;
            else if (entry->flags & ENTRY_SELECTED)
                highlight = data->list_select;
            else
                highlight = NULL;

            /* Draw highlight or background */

            if (highlight != NULL)
            {
                zune_imspec_draw(highlight, muiRenderInfo(obj),
                    _mleft(data->area), y, _mwidth(data->area),
                    data->entry_maxheight,
                    0, y - data->entries_top_pixel, 0);
            }
            else if (((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_ENTRY
                && data->update_pos == entry_pos) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_NEEDED
                && (entry->flags & ENTRY_RENDER)))
            {
                DoMethod(obj, MUIM_DrawBackground, _mleft(data->area), y,
                    _mwidth(data->area), data->entry_maxheight, 0,
                    y - _mtop(data->area) +
                    data->entries_first * data->entry_maxheight, 0);
            }

            List_DrawEntry(cl, obj, entry_pos, y);
            entry->flags &= ~ENTRY_RENDER;
        }
        y += data->entry_maxheight;
    }

    MUI_RemoveClipping(muiRenderInfo(obj), clip);

    data->update = 0;

    if (scroll_caused_damage)
    {
        if (MUI_BeginRefresh(muiRenderInfo(obj), 0))
        {
            /* Theoretically it might happen that more damage is caused
               after ScrollRaster. By something else, like window movement
               in front of our window. Therefore refresh root object of
               window, not just this object */

            Object *o = NULL;

            get(_win(obj), MUIA_Window_RootObject, &o);
            MUI_Redraw(o, MADF_DRAWOBJECT);

            MUI_EndRefresh(muiRenderInfo(obj), 0);
        }
    }

    ULONG x1 = _mleft(data->area);
    ULONG col;
    y = _mtop(data->area);

    if (data->title_height && data->title)
    {
        for (col = 0; col < data->columns; col++)
        {
            ULONG halfdelta = data->ci[col].delta / 2;
            x1 += data->ci[col].entries_width + halfdelta;

            if (x1 + (data->ci[col].bar ? BAR_WIDTH : 0) > _mright(data->area))
                break;

            if (data->ci[col].bar)
            {
                SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
                Move(_rp(obj), x1, y);
                Draw(_rp(obj), x1,
                    y + data->entries[ENTRY_TITLE]->height - 1);
                SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
                Move(_rp(obj), x1 + 1, y);
                Draw(_rp(obj), x1 + 1,
                    y + data->entries[ENTRY_TITLE]->height - 1);

                x1 += BAR_WIDTH;
            }
            x1 += data->ci[col].delta - halfdelta;
        }
        y += data->entries[ENTRY_TITLE]->height + 1;
    }

    x1 = _mleft(data->area);

    for (col = 0; col < data->columns; col++)
    {
        ULONG halfdelta = data->ci[col].delta / 2;
        x1 += data->ci[col].entries_width + halfdelta;

        if (x1 + (data->ci[col].bar ? BAR_WIDTH : 0) > _mright(data->area))
            break;

        if (data->ci[col].bar)
        {
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
            Move(_rp(obj), x1, y);
            Draw(_rp(obj), x1, _mbottom(data->area));
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
            Move(_rp(obj), x1 + 1, y);
            Draw(_rp(obj), x1 + 1, _mbottom(data->area));

            x1 += BAR_WIDTH;
        }

        x1 += data->ci[col].delta - halfdelta;
    }

    data->flags &= ~LIST_CHANGED;

    return 0;
}


/*
 * Destruct/free confirmed entries then publish zero count/first and
 * inactive cursor, request redraw. Requires consistent callbacks; always0,
 * retains capacity/title. Active update can use freed old record storage;
 * no transactional lifetime guard.
 */
IPTR List__MUIM_Clear(struct IClass *cl, Object *obj,
    struct MUIP_List_Clear *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    while (data->confirm_entries_num)
    {
        struct ListEntry *lentry =
            data->entries[--data->confirm_entries_num];
        DoMethod(obj, MUIM_List_Destruct, (IPTR) lentry->data,
            (IPTR) data->pool);
        FreeListEntry(data, lentry);

        data->flags |= LIST_CHANGED;
    }
    /* Should never fail when shrinking */
    SetListSize(data, 0);

    if (data->confirm_entries_num != data->entries_num)
    {
        SetAttrs(obj, MUIA_List_Entries, 0, MUIA_List_First, 0,
            /* Notify only when no entry was active */
            data->entries_active !=
            MUIV_List_Active_Off ? MUIA_List_Active : TAG_DONE,
            MUIV_List_Active_Off, TAG_DONE);

        data->update = UPDATEMODE_ALL;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return 0;
}


/*
 * Resolve selectors and swap distinct valid records, requesting two row
 * redraws. Requires consistent list; TRUE changed,FALSE invalid/equal.
 * Active index unchanged, no hooks, and Quiet pending-state bookkeeping is
 * incomplete.
 */
IPTR List__MUIM_Exchange(struct IClass *cl, Object *obj,
    struct MUIP_List_Exchange *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos1, pos2;

    switch (msg->pos1)
    {
    case MUIV_List_Exchange_Top:
        pos1 = 0;
        break;
    case MUIV_List_Exchange_Active:
        pos1 = data->entries_active;
        break;
    case MUIV_List_Exchange_Bottom:
        pos1 = data->entries_num - 1;
        break;
    default:
        pos1 = msg->pos1;
    }

    switch (msg->pos2)
    {
    case MUIV_List_Exchange_Top:
        pos2 = 0;
        break;
    case MUIV_List_Exchange_Active:
        pos2 = data->entries_active;
        break;
    case MUIV_List_Exchange_Bottom:
        pos2 = data->entries_num - 1;
        break;
    case MUIV_List_Exchange_Next:
        pos2 = pos1 + 1;
        break;
    case MUIV_List_Exchange_Previous:
        pos2 = pos1 - 1;
        break;
    default:
        pos2 = msg->pos2;
    }

    if (pos1 >= 0 && pos1 < data->entries_num && pos2 >= 0
        && pos2 < data->entries_num && pos1 != pos2)
    {
        struct ListEntry *save = data->entries[pos1];
        data->entries[pos1] = data->entries[pos2];
        data->entries[pos2] = save;

        data->update = UPDATEMODE_ENTRY;
        data->update_pos = pos1;
        MUI_Redraw(obj, MADF_DRAWUPDATE);

        data->update = UPDATEMODE_ENTRY;
        data->update_pos = pos2;
        MUI_Redraw(obj, MADF_DRAWUPDATE);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**************************************************************************
 MUIM_List_Redraw
**************************************************************************/
/*
 * Remeasure all or resolved row, mark updates and honor Quiet pending
 * state. Requires valid numeric index; only -1 is rejected before record
 * access. Always0, first pointer match used for Entry; no ownership change.
 */
IPTR List__MUIM_Redraw(struct IClass *cl, Object *obj,
    struct MUIP_List_Redraw *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    D(bug("[Zune:List] %s()\n", __func__);)

    if (msg->pos == MUIV_List_Redraw_All)
    {
        CalcWidths(cl, obj);
        data->update = UPDATEMODE_ALL;
        if (!(data->flags & LIST_QUIET))
        {
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        }
        else
            data->flags |= LIST_CHANGED;
    }
    else
    {
        LONG pos = -1;
        if (msg->pos == MUIV_List_Redraw_Active)
            pos = data->entries_active;
        else if (msg->pos == MUIV_List_Redraw_Entry)
        {
            LONG i;
            for (i = 0; i < data->entries_num; i++)
                if (data->entries[i]->data == msg->entry)
                {
                    pos = i;
                    break;
                }
        }
        else
            pos = msg->pos;

        if (pos != -1)
        {
            data->entries[pos]->flags |= ENTRY_RENDER;
            if (!(data->flags & LIST_QUIET))
            {
                if (CalcDimsOfEntry(cl, obj, pos))
                    data->update = UPDATEMODE_ALL;
                else
                {
                    data->update = UPDATEMODE_ENTRY;
                    data->update_pos = pos;
                }

                MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
            else
            {
                if (CalcDimsOfEntry(cl, obj, pos))
                    data->update = UPDATEMODE_ALL;
                else if (!(data->update & UPDATEMODE_ALL))
                    data->update = UPDATEMODE_NEEDED;
                data->flags |= LIST_CHANGED;
            }
        }
    }

    return 0;
}


/*
 * Destruct/free matching rows and compact pointers, adjust confirmed/public
 * counts and Active then redraw. Requires valid state and callbacks;
 * always0. Selected falls back to active if none selected; UWORD scan and
 * stale-active lifetime limits remain.
 */
IPTR List__MUIM_Remove(struct IClass *cl, Object *obj,
    struct MUIP_List_Remove *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos;
    LONG new_act;
    UWORD i;
    BOOL found, done = FALSE;
    struct ListEntry *lentry;
    Tag active_tag = TAG_DONE;

    if (!data->entries_num)
        return 0;

    switch (msg->pos)
    {
    case MUIV_List_Remove_First:
        pos = 0;
        break;

    case MUIV_List_Remove_Active:
        pos = data->entries_active;
        break;

    case MUIV_List_Remove_Last:
        pos = data->entries_num - 1;
        break;

    case MUIV_List_Remove_Selected:
        pos = 0;
        break;

    default:
        pos = msg->pos;
        break;
    }

    if (pos < 0 || pos >= data->entries_num)
        return 0;

    new_act = data->entries_active;

    while (!done)
    {
        if (msg->pos == MUIV_List_Remove_Selected)
        {
            /* Find the next selected entry */
            for (found = FALSE, i = pos;
                i < data->confirm_entries_num && !found; i++)
            {
                if (data->entries[i]->flags & ENTRY_SELECTED)
                {
                    pos = i;
                    found = TRUE;
                }
            }

            if (!found)
            {
                done = TRUE;

                /* If there were no selected entries, remove the active one */
                if (data->confirm_entries_num == data->entries_num
                    && data->entries_active != MUIV_List_Active_Off)
                {
                    pos = data->entries_active;
                    found = TRUE;
                }
            }
        }
        else
        {
            done = TRUE;
            found = TRUE;
        }

        if (found)
        {
            lentry = data->entries[pos];
            DoMethod(obj, MUIM_List_Destruct, (IPTR) lentry->data,
                (IPTR) data->pool);
            FreeListEntry(data, lentry);
            RemoveListEntries(data, pos, 1);
            data->confirm_entries_num--;

            if (pos < new_act)
            {
                new_act--;
                active_tag = MUIA_List_Active;
            }
            else if (pos == new_act)
                active_tag = MUIA_List_Active;
        }
    }

    /* Update entries count prior to range check */
    SetAttrs(obj, MUIA_List_Entries, data->confirm_entries_num, TAG_DONE);

    /* Ensure that the active element is in a valid range (it might become
     * MUIV_List_Active_Off (-1), but that's OK) */
    if (new_act >= data->entries_num)
        new_act = data->entries_num - 1;

    SetAttrs(obj,
        active_tag, new_act,   /* Inform only if necessary (for notify) */
        TAG_DONE);

    data->flags |= LIST_CHANGED;
    data->update = UPDATEMODE_ALL;
    if (!(data->flags & LIST_QUIET))
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return 0;
}


/*
 * Resolve range and query/change selection bits, applying MultiTestHook to
 * coexistence. Requires declared mode and stable callbacks; always0.
 * Optional info is selected count only for All+Ask, otherwise last previous
 * state. Direct method emits no SelectChange notification.
 */
IPTR List__MUIM_Select(struct IClass *cl, Object *obj,
    struct MUIP_List_Select *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, i, count, selcount = 0, state = 0;
    BOOL multi_allowed = TRUE, new_multi_allowed, new_select_state = FALSE;

    /* Establish the range of entries affected */
    switch (msg->pos)
    {
    case MUIV_List_Select_Active:
        pos = data->entries_active;
        if (pos == MUIV_List_Active_Off)
            count = 0;
        else
            count = 1;
        break;

    case MUIV_List_Select_All:
        pos = 0;
        count = data->entries_num;
        break;

    default:
        pos = msg->pos;
        count = 1;
        if (pos < 0 || pos >= data->entries_num)
            return 0;
        break;
    }

    if (msg->seltype != MUIV_List_Select_Ask && data->multi_test_hook != NULL)
    {
        /* Count selected entries and disallow selection of additional
           entries if there is a currently selected entry that is not
           multi-selectable */
        for (i = 0; i < data->entries_num && multi_allowed; i++)
        {
            if (data->entries[i]->flags & ENTRY_SELECTED)
            {
                selcount++;
                if (data->multi_test_hook != NULL && selcount == 1)
                    multi_allowed = CallHookPkt(data->multi_test_hook, NULL,
                        data->entries[i]->data);
            }
        }
    }

    /* Change or check state of each entry in the range */
    for (i = pos; i < pos + count; i++)
    {
        state = data->entries[i]->flags & ENTRY_SELECTED;
        switch (msg->seltype)
        {
        case MUIV_List_Select_Off:
            new_select_state = FALSE;
            break;

        case MUIV_List_Select_On:
            new_select_state = TRUE;
            break;

        case MUIV_List_Select_Toggle:
            new_select_state = !state;
            break;

        default:
            if (data->entries[i]->flags & ENTRY_SELECTED)
                selcount++;
            break;
        }

        if (msg->seltype != MUIV_List_Select_Ask)
        {
            if (new_select_state && !state)
            {
                /* Check if there is potential to select an additional entry */
                if (multi_allowed || selcount == 0)
                {
                    /* Check if the entry to be selected is multi-selectable */
                    if (data->multi_test_hook != NULL)
                        new_multi_allowed = CallHookPkt(data->multi_test_hook,
                            NULL, data->entries[i]->data);
                    else
                        new_multi_allowed = TRUE;

                    /* Check if the entry to be selected can be selected at
                       the same time as the already selected entries */
                    if (new_multi_allowed || selcount == 0)
                    {
                        /* Select the entry and update the selection count
                           and flag */
                        data->entries[i]->flags |= ENTRY_SELECTED;
                        selcount++;

                        multi_allowed = new_multi_allowed;
                    }
                }
            }
            else if (!new_select_state && state)
            {
                data->entries[i]->flags &= ~ENTRY_SELECTED;
                    selcount--;
            }
        }
    }

    /* Report old state or number of selected entries */
    if (msg->info)
    {
        if (msg->pos == MUIV_List_Select_All
            && msg->seltype == MUIV_List_Select_Ask)
            *msg->info = selcount;
        else
            *msg->info = state;
    }

    /* Redraw unless it was just an enquiry */
    if (msg->seltype != MUIV_List_Select_Ask)
    {
        if (count > 1)
            data->update = UPDATEMODE_ALL;
        else
        {
            data->update = UPDATEMODE_ENTRY;
            data->update_pos = pos;
        }
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
   
    return 0;
}


/*
 * Reserve pointer gap, construct entries and publish count/update state on
 * completion. Requires valid array, pool, callbacks and arithmetic. Return
 * pre-sort last slot or all-bits-one failure; partial failures lack
 * rollback and can violate gap-compaction preconditions. Sorted active
 * adjustment occurs after sorting.
 */
IPTR List__MUIM_Insert(struct IClass *cl, Object *obj,
    struct MUIP_List_Insert *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, count, sort, active;
    BOOL adjusted = FALSE;

    count = msg->count;
    sort = 0;

    if (count == -1)
    {
        /* Count the number of entries */
        for (count = 0; msg->entries[count] != NULL; count++)
            ;
    }

    if (count <= 0)
        return ~0;

    switch (msg->pos)
    {
    case MUIV_List_Insert_Top:
        pos = 0;
        break;

    case MUIV_List_Insert_Active:
        if (data->entries_active != -1)
            pos = data->entries_active;
        else
            pos = 0;
        break;

    case MUIV_List_Insert_Sorted:
        pos = data->entries_num;
        sort = 1;               /* we sort'em later */
        break;

    case MUIV_List_Insert_Bottom:
        pos = data->entries_num;
        break;

    default:
        if (msg->pos > data->entries_num)
            pos = data->entries_num;
        else if (msg->pos < 0)
            pos = 0;
        else
            pos = msg->pos;
        break;
    }
    data->insert_position = pos;

    if (!(SetListSize(data, data->entries_num + count)))
        return ~0;

    LONG until = pos + count;
    APTR *toinsert = msg->entries;

    if (!(PrepareInsertListEntries(data, pos, count)))
        return ~0;

    while (pos < until)
    {
        struct ListEntry *lentry;

        if (!(lentry = AllocListEntry(data)))
        {
            /* Panic, but we must be in a consistent state, so remove
             * the space where the following list entries should have gone
             */
            RemoveListEntries(data, pos, until - pos);
            return ~0;
        }

        /* now call the construct method which returns us a pointer which
           we need to store */
        lentry->data = (APTR) DoMethod(obj, MUIM_List_Construct,
            (IPTR) * toinsert, (IPTR) data->pool);
        if (!lentry->data)
        {
            FreeListEntry(data, lentry);
            RemoveListEntries(data, pos, until - pos);

            /* TODO: Update visible-entry state before returning on
             * MUIM_List_Construct failure. */
            if (data->entries_num != data->confirm_entries_num)
                set(obj, MUIA_List_Entries, data->confirm_entries_num);
            return ~0;
        }

        lentry->flags |= ENTRY_RENDER;
        data->entries[pos] = lentry;
        data->confirm_entries_num++;

        data->flags |= LIST_CHANGED;

        if (_flags(obj) & MADF_SETUP)
        {
            /* We have to calculate the width and height of the newly
             * inserted entry. This has to be done after inserting the
             * element into the list */
            if (CalcDimsOfEntry(cl, obj, pos))
                adjusted = TRUE;
        }

        toinsert++;
        pos++;
    }
    pos--;

    /* Recalculate the number of visible entries */
    if (_flags(obj) & MADF_SETUP)
        CalcVertVisible(cl, obj);

    if (data->entries_num != data->confirm_entries_num)
    {
        SetAttrs(obj,
            MUIA_List_Entries, data->confirm_entries_num,
            MUIA_List_Visible, data->entries_visible, TAG_DONE);
    }

    /* If the array is already sorted, we could do a simple insert
     * sort and would be much faster than with qsort.
     * If an array is not yet sorted, does a MUIV_List_Insert_Sorted
     * sort the whole array?
     *
     * I think, we better sort the whole array:
     */
    if (sort)
    {
        /* TODO: define which position to return after
           MUIV_List_Insert_Sorted reorders the list. */
        DoMethod(obj, MUIM_List_Sort);

        if ((adjusted) && (data->flags & LIST_QUIET))
            data->update = UPDATEMODE_ALL;
    }
    else
    {
        data->update = UPDATEMODE_ALL;
        if (!(data->flags & LIST_QUIET))
            MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    superset(cl, obj, MUIA_List_InsertPosition, data->insert_position);

    /* Update index of active entry */
    if (data->entries_active >= data->insert_position)
    {
        active = data->entries_active + count;
        SET(obj, MUIA_List_Active, active);
    }

    return (ULONG) pos;
}


/*
 * Dispatch Insert synchronously with address of message entry and count1.
 * Requires correct message and construction-compatible data. Return Insert
 * result unchanged, including failure/state limits; no separate allocation
 * policy.
 */
IPTR List__MUIM_InsertSingle(struct IClass *cl, Object *obj,
    struct MUIP_List_InsertSingle *msg)
{
    return DoMethod(obj, MUIM_List_Insert, (IPTR) & msg->entry, 1,
        msg->pos);
}


/*
 * Resolve Active or numeric index and write borrowed stored pointer to
 * required APTR output. Return same pointer or0 with NULL output for
 * invalid index. Requires writable output; no copy or lifetime extension.
 */
IPTR List__MUIM_GetEntry(struct IClass *cl, Object *obj,
    struct MUIP_List_GetEntry *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int pos = msg->pos;

    if (pos == MUIV_List_GetEntry_Active)
        pos = data->entries_active;

    if (pos < 0 || pos >= data->entries_num)
    {
        *msg->entry = NULL;
        return 0;
    }
    *msg->entry = data->entries[pos]->data;
    return (IPTR) *msg->entry;
}

/**************************************************************************
 MUIM_List_Construct
**************************************************************************/
/*
 * Retain input, allocate builtin pooled string with size prefix, or invoke
 * custom hook with pool/object and entry/message. Return stored pointer
 * or0; caller owns matching destruction duty. Does not insert a record;
 * require valid pool and source representation.
 */
IPTR List__MUIM_Construct(struct IClass *cl, Object *obj,
    struct MUIP_List_Construct *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (NULL == data->construct_hook)
        return (IPTR) msg->entry;
    if ((IPTR) data->construct_hook == MUIV_List_ConstructHook_String)
    {
        int len = msg->entry ? strlen((STRPTR) msg->entry) : 0;
        ULONG *mem = AllocPooled(msg->pool, len + 5);

        if (NULL == mem)
            return 0;
        mem[0] = len + 5;
        if (msg->entry != NULL)
            strcpy((STRPTR) (mem + 1), (STRPTR) msg->entry);
        else
            *(STRPTR) (mem + 1) = 0;
        return (IPTR) (mem + 1);
    }
    return CallHookPkt(data->construct_hook, msg->pool, msg->entry);
}

/**************************************************************************
 MUIM_List_Destruct
**************************************************************************/
/*
 * No-op for NULL policy, free builtin prefixed allocation or invoke custom
 * destruction hook. Require matching policy/pool and no remaining entry
 * users. Always0; direct invocation does not remove list record.
 */
IPTR List__MUIM_Destruct(struct IClass *cl, Object *obj,
    struct MUIP_List_Destruct *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (NULL == data->destruct_hook)
        return 0;

    if ((IPTR) data->destruct_hook == MUIV_List_DestructHook_String)
    {
        ULONG *mem = ((ULONG *) msg->entry) - 1;
        FreePooled(msg->pool, mem, mem[0]);
    }
    else
    {
        CallHookPkt(data->destruct_hook, msg->pool, msg->entry);
    }
    return 0;
}


/*
 * Invoke comparator with entry2/object and entry1/message, ignoring both
 * sort fields. Requires valid live comparator and data. Return hook result
 * interpreted as signedLONG by Sort; no mutation or sorting here.
 */
IPTR List__MUIM_Compare(struct IClass *cl, Object *obj,
    struct MUIP_List_Compare *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    return CallHookPkt(data->compare_hook, msg->entry2, msg->entry1);
}

/**************************************************************************
 MUIM_List_Display
**************************************************************************/
/*
 * Without hook set first string only and return1; otherwise write ULONG
 * metadata before array and call hook with array/object and entry/message.
 * Requires writable prefix and all used column pointers valid afterwards.
 * Return hook result; preparses argument unused and metadata width is ABI-
 * sensitive.
 */
IPTR List__MUIM_Display(struct IClass *cl, Object *obj,
    struct MUIP_List_Display *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (NULL == data->display_hook)
    {
        if (msg->entry)
            *msg->array = msg->entry;
        else
            *msg->array = 0;
        return 1;
    }

    *((ULONG *) (msg->array - 1)) = msg->entry_pos;
    return CallHookPkt(data->display_hook, msg->array, msg->entry);
}

/**************************************************************************
 MUIM_List_SelectChange
**************************************************************************/
/*
 * Private MUIM_List_SelectChange callback is a no-op returning1. Position,
 * state and flags are ignored. Active setter invokes it; public Select does
 * not. Subclass behavior must not invalidate active traversal.
 */
IPTR List__MUIM_SelectChange(struct IClass *cl, Object *obj,
    struct MUIP_List_SelectChange *msg)
{
    return 1;
}


/*
 * Allocate pooled handle retaining Area object, append to image list and
 * connect/setup immediately when List setup. Return handle or0 for
 * NULL/allocation failure; flags ignored and setup success unchecked.
 * Caller must delete handle explicitly and owns Area disposal.
 */
IPTR List__MUIM_CreateImage(struct IClass *cl, Object *obj,
    struct MUIP_List_CreateImage *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListImage *li;

    if (!msg->obj)
        return 0;

    li = AllocPooled(data->pool, sizeof(struct ListImage));
    if (!li)
        return 0;
    li->obj = msg->obj;

    AddTail((struct List *)&data->images, (struct Node *)li);

    /* If the list is already setup, connect the object */
    if ((_flags(obj) & MADF_SETUP)) {
        DoMethod(li->obj, MUIM_ConnectParent, (IPTR) obj);
        DoSetupMethod(li->obj, muiRenderInfo(obj));
    }
    return (IPTR) li;
}


/*
 * For non-NULL live same-list handle call Area Cleanup/DisconnectParent,
 * unlink and free pooled handle. Requires matched setup and removed display
 * references. Always0; does not dispose Area or validate ownership/double
 * deletion.
 */
IPTR List__MUIM_DeleteImage(struct IClass *cl, Object *obj,
    struct MUIP_List_DeleteImage *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListImage *li = (struct ListImage *)msg->listimg;

    if (li)
    {
        DoMethod(li->obj, MUIM_Cleanup);
        DoMethod(li->obj, MUIM_DisconnectParent);
        Remove((struct Node *)li);
        FreePooled(data->pool, li, sizeof(struct ListImage));
    }

    return 0;
}


/*
 * Resolve/clamp target and set First only when outside current visible
 * span. Requires valid positive visible count for meaningful output.
 * AlwaysTRUE even empty/unlaid-out, does not change Active or validate
 * display success.
 */
IPTR List__MUIM_Jump(struct IClass *cl, Object *obj,
    struct MUIP_List_Jump *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos = msg->pos;

    switch (pos)
    {
    case MUIV_List_Jump_Top:
        pos = 0;
        break;

    case MUIV_List_Jump_Active:
        pos = data->entries_active;
        break;

    case MUIV_List_Jump_Bottom:
        pos = data->entries_num - 1;
        break;

    case MUIV_List_Jump_Down:
        pos = data->entries_first + data->entries_visible;
        break;

    case MUIV_List_Jump_Up:
        pos = data->entries_first - 1;
        break;
    }

    if (pos >= data->entries_num)
    {
        pos = data->entries_num - 1;
    }
    if (pos < 0)
        pos = 0;

    if (pos < data->entries_first)
    {
        set(obj, MUIA_List_First, pos);
    }
    else if (pos >= data->entries_first + data->entries_visible)
    {
        pos -= (data->entries_visible - 1);
        if (pos < 0)
            pos = 0;
        if (pos != data->entries_first)
        {
            set(obj, MUIA_List_First, pos);
        }
    }

    return TRUE;
}


/*
 * Selection-sort records using Compare with zero sort fields, preserving
 * active data by direct index adjustment. Requires stable
 * comparator/callback lifetimes. Always0, quadratic and not stable; marks
 * reordered rows and defers redraw under Quiet, no Active notification.
 */
IPTR List__MUIM_Sort(struct IClass *cl, Object *obj,
    struct MUIP_List_Sort *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    int i, j, max;
    struct MUIP_List_Compare cmpmsg =
        { MUIM_List_Compare, NULL, NULL, 0, 0 };
    BOOL changed = FALSE;

    D(bug("[Zune:List] %s()\n", __func__);)

    if (data->entries_num > 1)
    {
        /*
           Simple sort algorithm. Feel free to improve it.
         */
        for (i = 0; i < data->entries_num - 1; i++)
        {
            max = i;
            for (j = i + 1; j < data->entries_num; j++)
            {
                cmpmsg.entry1 = data->entries[max]->data;
                cmpmsg.entry2 = data->entries[j]->data;
                if ((LONG) DoMethodA(obj, (Msg) & cmpmsg) > 0)
                {
                    max = j;
                }
            }
            if (i != max)
            {
                APTR tmp = data->entries[i];
                data->entries[i] = data->entries[max];
                data->entries[i]->flags |= ENTRY_RENDER;
                data->entries[max] = tmp;
                data->entries[max]->flags |= ENTRY_RENDER;
                if (data->entries_active == i)
                    data->entries_active = max;
                else if (data->entries_active == max)
                    data->entries_active = i;
                changed = TRUE;
            }
        }
    }

    if (changed)
    {
        data->flags |= LIST_CHANGED;
        if (!(data->update & UPDATEMODE_ALL))
            data->update = UPDATEMODE_NEEDED;
        if (!(data->flags & LIST_QUIET))
            MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return 0;
}


/*
 * Resolve selectors, validate distinct indices and shift records to
 * destination. TRUE changed,FALSE invalid/equal. Active adjustment code is
 * disabled, so index remains unchanged; selection flags move with record.
 * Mark full pending redraw; no construction/destruction.
 */
IPTR List__MUIM_Move(struct IClass *cl, Object *obj,
    struct MUIP_List_Move *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    LONG from, to;
    int i;

    /* Normalise special 'from' values */
    switch (msg->from)
    {
    case MUIV_List_Move_Top:
        from = 0;
        break;
    case MUIV_List_Move_Active:
        from = data->entries_active;
        break;
    case MUIV_List_Move_Bottom:
        from = data->entries_num - 1;
        break;
    default:
        from = msg->from;
    }

    /* Normalise special 'to' values */
    switch (msg->to)
    {
    case MUIV_List_Move_Top:
        to = 0;
        break;
    case MUIV_List_Move_Active:
        to = data->entries_active;
        break;
    case MUIV_List_Move_Bottom:
        to = data->entries_num - 1;
        break;
    case MUIV_List_Move_Next:
        to = from + 1;
        break;
    case MUIV_List_Move_Previous:
        to = from - 1;
        break;
    default:
        to = msg->to;
    }

    /* Check that values are within valid bounds */
    if (from > data->entries_num - 1 || from < 0
        || to > data->entries_num - 1 || to < 0 || from == to)
        return (IPTR) FALSE;

    /* Shift all entries in the range between the 'from' and 'to' positions */
    if (from < to)
    {
        struct ListEntry *backup = data->entries[from];
        for (i = from; i < to; i++)
            data->entries[i] = data->entries[i + 1];
        data->entries[to] = backup;
    }
    else
    {
        struct ListEntry *backup = data->entries[from];
        for (i = from; i > to; i--)
            data->entries[i] = data->entries[i - 1];
        data->entries[to] = backup;
    }

#if 0    /* Not done in MUI 3 */
    /* Update index of active entry */
    if (from == data->entries_active)
        data->entries_active = to;
    else if (data->entries_active > from && data->entries_active < to)
        data->entries_active--;
    else if (data->entries_active < from && data->entries_active >= to)
        data->entries_active++;
#endif

    /* Reflect list changes visually */
    data->flags |= LIST_CHANGED;
    data->update = UPDATEMODE_ALL;
    if (!(data->flags & LIST_QUIET))
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return TRUE;
}


/*
 * Read required LONG cursor, scan after it or from Start, with active
 * fallback only at initial empty-selection scan. AlwaysTRUE, write next
 * index or End. Require valid prior index; other negatives/overflow not
 * checked. Mutation during iteration can skip entries.
 */
IPTR List__MUIM_NextSelected(struct IClass *cl, Object *obj,
    struct MUIP_List_NextSelected *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, i;
    BOOL found = FALSE;

    /* Get the first entry to check */
    pos = *msg->pos;
    if (pos == MUIV_List_NextSelected_Start)
        pos = 0;
    else
        pos++;

    /* Find the next selected entry */
    for (i = pos; i < data->entries_num && !found; i++)
    {
        if (data->entries[i]->flags & ENTRY_SELECTED)
        {
            pos = i;
            found = TRUE;
        }
    }

    /* Return index of selected or active entry, or indicate there are no
       more */
    if (!found)
    {
        if (*msg->pos == MUIV_List_NextSelected_Start
            && data->entries_active != MUIV_List_Active_Off)
            pos = data->entries_active;
        else
            pos = MUIV_List_NextSelected_End;
    }
    *msg->pos = pos;

    return TRUE;
}

/**************************************************************************
 MUIM_List_TestPos
**************************************************************************/
/*
 * Map window coordinates or cached release position to row/column/flags.
 * Requires layout and writable result. AlwaysTRUE; offsets only
 * conditionally written and row bounds assume valid scroll state.
 * Initialize result when unused offsets matter.
 */
IPTR List__MUIM_TestPos(struct IClass *cl, Object *obj,
    struct MUIP_List_TestPos *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result *result = msg->res;
    LONG col = -1, row = -1;
    UWORD flags = 0, i;
    LONG mx;
    LONG ey;
    LONG entries_visible;

    if (data->entries_visible <= data->entries_num)
        entries_visible = data->entries_visible;
    else
        entries_visible = data->entries_num;

    if ((msg->x == MUI_MAXMAX) && (msg->y == MUI_MAXMAX))
    {
        mx = data->mouse_x;
        ey = data->mouse_y;
    }
    else
    {
        mx = msg->x;
        ey = msg->y;
    }

    if ((mx != MUI_MAXMAX) && (ey != MUI_MAXMAX))
    {
        mx -= _left(data->area);
        /* y coordinates transformed to the entries */
        ey -= data->entries_top_pixel;

        /* Now check if it was clicked on a title or on entries */
        if (ey < 0)
            flags |= MUI_LPR_ABOVE;
        else if (ey >= entries_visible * data->entry_maxheight)
            flags |= MUI_LPR_BELOW;
        else
        {
            /* Identify row */
            row = ey / data->entry_maxheight + data->entries_first;
            result->yoffset =
                ey % data->entry_maxheight - data->entry_maxheight / 2;
        }

        if (mx < 0)
            flags |= MUI_LPR_LEFT;
        else if (mx >= _width(data->area))
            flags |= MUI_LPR_RIGHT;
        else
        {
            /* Identify column */
            if (data->entries_num > 0 && data->columns > 0)
            {
                LONG width_sum = 0;
                col = data->columns - 1;
                for (i = 0; i < data->columns; i++)
                {
                    result->xoffset = mx - width_sum;
                    width_sum +=
                        data->ci[i].entries_width +
                        data->ci[i].delta +
                        (data->ci[i].bar ? BAR_WIDTH : 0);
                    D(bug("[List/MUIM_TestPos] i %d "
                        "width %d width_sum %d mx %d\n",
                        i, data->ci[i].entries_width, width_sum, mx));
                    if (mx < width_sum)
                    {
                        col = i;
                        D(bug("[List/MUIM_TestPos] Column hit %d\n", col));
                        break;
                    }
                }
            }
        }
    }

    result->entry = row;
    result->column = col;
    result->flags = flags;

    return TRUE;
}


/*
 * Accept only self-originating drag when DragSortable set, otherwise
 * refuse. Requires valid method object pointer comparison. No reordering,
 * allocation or cross-list transfer is performed.
 */
IPTR List__MUIM_DragQuery(struct IClass *cl, Object *obj,
    struct MUIP_DragQuery *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (msg->obj == obj && (data->flags & LIST_DRAGSORTABLE))
        return MUIV_DragQuery_Accept;
    else
        return MUIV_DragQuery_Refuse;
}



/*
 * Reset internal drop-marker y to-1 and delegate superclass finish.
 * Requires valid drag lifecycle; return superclass result. Does not
 * explicitly erase marker or reset public DropMark.
 */
IPTR List__MUIM_DragFinish(struct IClass *cl, Object *obj,
    struct MUIP_DragFinish *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    data->drop_mark_y = -1;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/*
 * Test coordinates and optionally erase/draw provisional line using setup
 * RastPort. ReturnTRUE; does not update DropMark. Below-row calculation
 * subtracts First from min(Visible,Entries), so scrolled positions can
 * produce unexpected marker placement.
 */
IPTR List__MUIM_DragReport(struct IClass *cl, Object *obj,
    struct MUIP_DragReport *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result pos;
    struct RastPort *rp = _rp(obj);
    LONG n, y;
    UWORD old_pattern;

    /* Choose new drop mark position */

    DoMethod(obj, MUIM_List_TestPos, msg->x, msg->y, (IPTR) &pos);
    if (pos.entry != -1)
    {
        n = pos.entry;
        if (pos.yoffset > 0)
            n++;
    }
    else if ((pos.flags & MUI_LPR_ABOVE) != 0)
        n = data->entries_first;
    else
    {
        n = MIN(data->entries_visible, data->entries_num)
            - data->entries_first;
    }

    /* Clear old drop mark */

    if ((data->flags & LIST_SHOWDROPMARKS) != 0)
    {
        y = data->entries_top_pixel + (n - data->entries_first)
            * data->entry_maxheight;
        if (y != data->drop_mark_y)
        {
            DoMethod(obj, MUIM_DrawBackground, _mleft(data->area),
                data->drop_mark_y, _mwidth(data->area), 1, 0, 0, 0);

            /* Draw new drop mark and store its position */

            SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], _pens(obj)[MPEN_SHADOW],
                JAM2);
            old_pattern = rp->LinePtrn;
            SetDrPt(rp, 0xF0F0);
            Move(rp, _mleft(data->area), y);
            Draw(rp, _mright(data->area), y);
            SetDrPt(rp, old_pattern);
            data->drop_mark_y = y;
        }
    }

    return TRUE;
}



/*
 * Test position, record insertion boundary in DropMark and optionally move
 * active record/SetActive. Requires valid drag/list state; alwaysTRUE,
 * ignores Move result. Does not move a selected group or notify DropMark by
 * its direct assignment.
 */
IPTR List__MUIM_DragDrop(struct IClass *cl, Object *obj,
    struct MUIP_DragDrop *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result pos;
    LONG n;

    /* Find drop position */

    DoMethod(obj, MUIM_List_TestPos, msg->x, msg->y, (IPTR) &pos);
    if (pos.entry != -1)
    {
        /* Change drop position when coords move past centre of entry, not
         * entry boundary */

        n = pos.entry;
        if (pos.yoffset > 0)
            n++;
    }
    else if ((pos.flags & MUI_LPR_ABOVE) != 0)
        n = 0;
    else
        n = data->entries_num;

    data->drop_mark = n;

    if (data->flags & LIST_DRAGSORTABLE)
    {
        /* Ensure that dropped entry will be positioned between the two
         * entries that are above and below the drop mark, rather than
         * strictly at the numeric index shown */

        if (n > data->entries_active)
            n--;

        DoMethod(obj, MUIM_List_Move, MUIV_List_Move_Active, n);
        SET(obj, MUIA_List_Active, n);
    }

    return TRUE;
}



/*
 * Delegate when drag type None; otherwise snapshot one hit row with frame
 * into allocated drag image. Requires setup graphics/valid coordinates.
 * Return pointer orNULL; bitmap allocation failure can still return a
 * structure with NULL bitmap. Caller follows inherited drag-image deletion
 * lifecycle.
 */
static IPTR List__MUIM_CreateDragImage(struct IClass *cl, Object *obj,
    struct MUIP_CreateDragImage *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    BOOL success = TRUE;
    struct MUI_List_TestPos_Result pos;
    WORD width, height, left, top;
    struct MUI_DragImage *img = NULL;
    const struct ZuneFrameGfx *zframe;
    LONG depth;

    /* If entries aren't draggable, allow the list as a whole to be */
    if (data->drag_type == MUIV_Listview_DragType_None)
        return DoSuperMethodA(cl, obj, msg);

    /* Get info on dragged entry */
    DoMethod(obj, MUIM_List_TestPos, _left(data->area) + msg->touchx,
        _top(data->area) + msg->touchy, (IPTR) &pos);
    if (pos.entry == -1)
        success = FALSE;

    if (success)
    {
        /* Get boundaries of entry */
        width = _mwidth(data->area);
        height = data->entry_maxheight;
        left = _mleft(data->area);
        top = _top(data->area) + msg->touchy
            - (pos.yoffset + data->entry_maxheight / 2);

        /* Allocate drag image structure */
        img = (struct MUI_DragImage *)
            AllocVec(sizeof(struct MUI_DragImage), MEMF_CLEAR);
        if (img == NULL)
            success = FALSE;
    }

    if (success)
    {
        /* Get drag frame */
        zframe = zune_zframe_get(obj,
            &muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Drag]);

        /* Allocate drag image buffer */
        img->width = width + zframe->ileft + zframe->iright;
        img->height = height + zframe->itop + zframe->ibottom;
        depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);
        img->bm = AllocBitMap(img->width, img->height, depth, BMF_MINPLANES,
            _screen(obj)->RastPort.BitMap);

        if (img->bm != NULL)
        {
            /* Render entry */
            struct RastPort temprp;
            InitRastPort(&temprp);
            temprp.BitMap = img->bm;
            ClipBlit(_rp(obj), left, top, &temprp,
                zframe->ileft, zframe->itop, width, height,
                0xc0);

            /* Render frame */
            struct RastPort *rp_save = muiRenderInfo(obj)->mri_RastPort;
            muiRenderInfo(obj)->mri_RastPort = &temprp;
            zframe->draw(zframe->customframe, muiRenderInfo(obj), 0, 0,
                img->width, img->height, 0, 0, img->width, img->height);
            muiRenderInfo(obj)->mri_RastPort = rp_save;
        }

        /* Ensure drag point matches where user clicked */
        img->touchx = msg->touchx - zframe->ileft + _addleft(obj);
        img->touchy = -(pos.yoffset + data->entry_maxheight / 2)
            - zframe->itop;
        img->flags = 0;
    }

    return (IPTR) img;
}

/*
 * Read First/Entries/Visible, offset and clamp to nonnegative last page,
 * then SetFirst when changed. Requires consistent laid-out list and
 * representable delta. Return void, no active/selection change.
 */
static void DoWheelMove(struct IClass *cl, Object *obj, LONG wheely)
{
    LONG new, first, entries, visible;

    new = first = XGET(obj, MUIA_List_First);
    entries = XGET(obj, MUIA_List_Entries);
    visible = XGET(obj, MUIA_List_Visible);

    new += wheely;

    if (new > entries - visible)
    {
        new = entries - visible;
    }

    if (new < 0)
    {
        new = 0;
    }

    if (new != first)
    {
        set(obj, MUIA_List_First, new);
    }
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
/*
 * Handle keyboard, wheel, mouse selection/range tracking and double-click
 * notifications. Requires setup Window, stable list and ABI event valid for
 * call. Return consumption flags; emit Listview_SelectChange for attempted
 * input changes, not a success count. Cached mouse position updates on
 * release.
 */
IPTR List__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result pos;
    LONG seltype = MUIV_List_Select_On, old_active, new_active, visible,
        first, last, i;
    IPTR result = 0;
    BOOL select = FALSE, clear = FALSE, range_select = FALSE, changing;
    WORD delta;
    typeof(msg->muikey) muikey = msg->muikey;

    new_active = old_active = XGET(obj, MUIA_List_Active);
    visible = XGET(obj, MUIA_List_Visible);

    if (muikey != MUIKEY_NONE)
    {
        result = MUI_EventHandlerRC_Eat;

        /* Make keys behave differently in read-only mode */
        if (data->read_only)
        {
            switch (muikey)
            {
            case MUIKEY_TOP:
                muikey = MUIKEY_LINESTART;
                break;

            case MUIKEY_BOTTOM:
                muikey = MUIKEY_LINEEND;
                break;

            case MUIKEY_UP:
                muikey = MUIKEY_LEFT;
                break;

            case MUIKEY_DOWN:
            case MUIKEY_PRESS:
                muikey = MUIKEY_RIGHT;
                break;
            }
        }

        switch (muikey)
        {
        case MUIKEY_TOGGLE:
            if (data->multiselect != MUIV_Listview_MultiSelect_None
                && !data->read_only)
            {
                select = TRUE;
                data->click_column = data->def_click_column;
                new_active = MUIV_List_Active_Down;
            }
            else
            {
                DoMethod(obj, MUIM_List_Jump, 0);
                muikey = MUIKEY_NONE;
            }
            break;

        case MUIKEY_TOP:
            new_active = MUIV_List_Active_Top;
            break;

        case MUIKEY_BOTTOM:
            new_active = MUIV_List_Active_Bottom;
            break;

        case MUIKEY_LEFT:
        case MUIKEY_WORDLEFT:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Up);
            break;

        case MUIKEY_RIGHT:
        case MUIKEY_WORDRIGHT:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Down);
            break;

        case MUIKEY_LINESTART:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Top);
            break;

        case MUIKEY_LINEEND:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Bottom);
            break;

        case MUIKEY_UP:
            new_active = MUIV_List_Active_Up;
            break;

        case MUIKEY_DOWN:
            new_active = MUIV_List_Active_Down;
            break;

        case MUIKEY_PRESS:
            data->click_column = data->def_click_column;
            superset(cl, obj, MUIA_Listview_ClickColumn,
                data->click_column);
            set(obj, MUIA_Listview_DoubleClick, TRUE);
            break;

        case MUIKEY_PAGEUP:
            if (data->read_only)
                DoWheelMove(cl, obj, -visible);
            else
                new_active = MUIV_List_Active_PageUp;
            break;

        case MUIKEY_PAGEDOWN:
            if (data->read_only)
                DoWheelMove(cl, obj, visible);
            else
                new_active = MUIV_List_Active_PageDown;
            break;

        default:
            result = 0;
        }
    }
    else if (msg->imsg)
    {
        DoMethod(obj, MUIM_List_TestPos, msg->imsg->MouseX, msg->imsg->MouseY,
            (IPTR) &pos);

        switch (msg->imsg->Class)
        {
        case IDCMP_MOUSEBUTTONS:
            if (msg->imsg->Code == SELECTDOWN)
            {
                if (_isinobject(data->area, msg->imsg->MouseX,
                    msg->imsg->MouseY))
                {
                    data->mouse_click = MOUSE_CLICK_ENTRY;

                    if (!data->read_only && pos.entry != -1)
                    {
                        new_active = pos.entry;

                        clear = (data->multiselect
                            == MUIV_Listview_MultiSelect_Shifted
                            && (msg->imsg->Qualifier
                            & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) == 0);
                        seltype = clear ? MUIV_List_Select_On
                            : MUIV_List_Select_Toggle;
                        select = data->multiselect
                            != MUIV_Listview_MultiSelect_None;

                        /* Handle MUIA_Listview_ClickColumn */
                        data->click_column = pos.column;
                        superset(cl, obj, MUIA_Listview_ClickColumn,
                            data->click_column);

                        /* Handle double clicking */
                        if (data->last_active == pos.entry
                            && DoubleClick(data->last_secs, data->last_mics,
                            msg->imsg->Seconds, msg->imsg->Micros))
                        {
                            set(obj, MUIA_Listview_DoubleClick, TRUE);
                            data->last_active = -1;
                            data->last_secs = data->last_mics = 0;
                        }
                        else
                        {
                            data->last_active = pos.entry;
                            data->last_secs = msg->imsg->Seconds;
                            data->last_mics = msg->imsg->Micros;

                            /* Look out for mouse movement, timer and inactive-window events while mouse button is down */
                            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->ehn);
                            data->ehn.ehn_Events |= (IDCMP_MOUSEMOVE | IDCMP_INTUITICKS |IDCMP_INACTIVEWINDOW);
                            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->ehn);
                        }
                    }
                    result = MUI_EventHandlerRC_Eat;
                }
            }
            else
            {
                /* Do not check for _isinobject, so that button releases once mouse leaves the object are still
                   registered and the drag-selection is turned off */
                if (msg->imsg->Code == SELECTUP && data->mouse_click)
                {
                    /* cache click position ... */
                    data->mouse_x = msg->imsg->MouseX;
                    data->mouse_y = msg->imsg->MouseY;

                    /* ... and activate the object */
                    set(_win(obj), MUIA_Window_ActiveObject, (IPTR)obj);
                    data->mouse_click = 0;
                }

                /* Restore normal event mask */
                DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                    (IPTR) &data->ehn);
                data->ehn.ehn_Events &= ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS
                    | IDCMP_INACTIVEWINDOW);
                DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                    (IPTR) &data->ehn);
            }
            break;

        case IDCMP_MOUSEMOVE:
        case IDCMP_INTUITICKS:
            if (pos.flags & MUI_LPR_ABOVE)
                new_active = MUIV_List_Active_Up;
            else if (pos.flags & MUI_LPR_BELOW)
                new_active = MUIV_List_Active_Down;
            else
                new_active = pos.entry;

            select = new_active != old_active
                && data->multiselect != MUIV_Listview_MultiSelect_None;
            if (select)
            {
                seltype = data->seltype;
                range_select = new_active >= 0;
            }

            break;

        case IDCMP_INACTIVEWINDOW:
            /* Stop listening for events we only listen to when mouse button is
               down: we will not be informed of the button being released */
            DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                (IPTR) &data->ehn);
            data->ehn.ehn_Events &=
                ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS | IDCMP_INACTIVEWINDOW);
            DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                (IPTR) &data->ehn);
            break;

        case IDCMP_RAWKEY:
            /* Scroll wheel */
            if (data->vert && _isinobject(data->vert, msg->imsg->MouseX,
                msg->imsg->MouseY))
                delta = 1;
            else if (_isinobject(data->area, msg->imsg->MouseX,
                msg->imsg->MouseY))
                delta = 4;
            else
                delta = 0;

            if (delta != 0)
            {
                switch (msg->imsg->Code)
                {
                case RAWKEY_NM_WHEEL_UP:
                    DoWheelMove(cl, obj, -delta);
                    break;

                case RAWKEY_NM_WHEEL_DOWN:
                    DoWheelMove(cl, obj, delta);
                    break;
                }
                result = MUI_EventHandlerRC_Eat;
            }
            break;
        }
    }

    /* Decide in advance if any selections may change */
    changing = clear || muikey == MUIKEY_TOGGLE || select;

    /* Change selected and active entries */
    if (clear)
    {
        DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All,
            MUIV_List_Select_Off, NULL);
    }

    if (muikey == MUIKEY_TOGGLE)
    {
        DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active,
            MUIV_List_Select_Toggle, NULL);
        select = FALSE;
    }

    if (new_active != old_active)
        set(obj, MUIA_List_Active, new_active);

    if (select)
    {
        if (range_select)
        {
            if (old_active < new_active)
                first = old_active + 1, last = new_active;
            else
                first = new_active, last = old_active - 1;
            for (i = first; i <= last; i++)
                DoMethod(obj, MUIM_List_Select, i, seltype, NULL);
        }
        else
        {
            DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active,
                seltype, NULL);
            DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active,
                MUIV_List_Select_Ask, &data->seltype);
        }
    }

    if (changing)
        superset(cl, obj, MUIA_Listview_SelectChange, TRUE);

    return result;
}

/**************************************************************************
 Dispatcher
**************************************************************************/
/*
 * Dispatch local List and lifecycle handlers with ABI-correct message or
 * delegate Group. Requires appropriate live object/setup state except
 * construction. Return handler result without extra bounds, ownership or
 * callback guards.
 */
BOOPSI_DISPATCHER(IPTR, List_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return List__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return List__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return List__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return List__OM_GET(cl, obj, (struct opGet *)msg);

    case MUIM_Setup:
        return List__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return List__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
    case MUIM_HandleEvent:
        return List__MUIM_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    case MUIM_AskMinMax:
        return List__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    case MUIM_Show:
        return List__MUIM_Show(cl, obj, (struct MUIP_Show *)msg);
    case MUIM_Hide:
        return List__MUIM_Hide(cl, obj, (struct MUIP_Hide *)msg);
    case MUIM_Draw:
        return List__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    case MUIM_Layout:
        return List__MUIM_Layout(cl, obj, (struct MUIP_Layout *)msg);
    case MUIM_List_Clear:
        return List__MUIM_Clear(cl, obj, (struct MUIP_List_Clear *)msg);
    case MUIM_List_Sort:
        return List__MUIM_Sort(cl, obj, (struct MUIP_List_Sort *)msg);
    case MUIM_List_Exchange:
        return List__MUIM_Exchange(cl, obj,
            (struct MUIP_List_Exchange *)msg);
    case MUIM_List_Insert:
        return List__MUIM_Insert(cl, obj, (APTR) msg);
    case MUIM_List_InsertSingle:
        return List__MUIM_InsertSingle(cl, obj, (APTR) msg);
    case MUIM_List_GetEntry:
        return List__MUIM_GetEntry(cl, obj, (APTR) msg);
    case MUIM_List_Redraw:
        return List__MUIM_Redraw(cl, obj, (APTR) msg);
    case MUIM_List_Remove:
        return List__MUIM_Remove(cl, obj, (APTR) msg);
    case MUIM_List_Select:
        return List__MUIM_Select(cl, obj, (APTR) msg);
    case MUIM_List_Construct:
        return List__MUIM_Construct(cl, obj, (APTR) msg);
    case MUIM_List_Destruct:
        return List__MUIM_Destruct(cl, obj, (APTR) msg);
    case MUIM_List_Compare:
        return List__MUIM_Compare(cl, obj, (APTR) msg);
    case MUIM_List_Display:
        return List__MUIM_Display(cl, obj, (APTR) msg);
    case MUIM_List_SelectChange:
        return List__MUIM_SelectChange(cl, obj, (APTR) msg);
    case MUIM_List_CreateImage:
        return List__MUIM_CreateImage(cl, obj, (APTR) msg);
    case MUIM_List_DeleteImage:
        return List__MUIM_DeleteImage(cl, obj, (APTR) msg);
    case MUIM_List_Jump:
        return List__MUIM_Jump(cl, obj, (APTR) msg);
    case MUIM_List_Move:
        return List__MUIM_Move(cl, obj, (struct MUIP_List_Move *)msg);
    case MUIM_List_NextSelected:
        return List__MUIM_NextSelected(cl, obj,
           (struct MUIP_List_NextSelected *)msg);
    case MUIM_List_TestPos:
        return List__MUIM_TestPos(cl, obj, (APTR) msg);
    case MUIM_DragQuery:
        return List__MUIM_DragQuery(cl, obj, (APTR) msg);
    case MUIM_DragFinish:
        return List__MUIM_DragFinish(cl, obj, (APTR) msg);
    case MUIM_DragReport:
        return List__MUIM_DragReport(cl, obj, (APTR) msg);
    case MUIM_DragDrop:
        return List__MUIM_DragDrop(cl, obj, (APTR) msg);
    case MUIM_CreateDragImage:
        return List__MUIM_CreateDragImage(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_List_desc =
{
    MUIC_List,
    MUIC_Group,
    sizeof(struct MUI_ListData),
    (void *) List_Dispatcher
};
