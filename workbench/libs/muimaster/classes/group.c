/*
    Copyright (C) 2002-2026, The AROS Development Team.
    Copyright (C) 1999, David Le Corfec.
    All rights reserved.

*/

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

extern struct Library *MUIMasterBase;

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"
#include "prefs.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

#define ROUND(x) ((int)(x + 0.5))
#define IS_HIDDEN(obj) (! (_flags(obj) & MADF_SHOWME) \
 || (_flags(obj) & MADF_BORDERGADGET))

/* Attributes filtered out in OM_SET, before OM_SET gets passed to children.
   Tested with MUI under UAE/AOS.

    notifyclass:
    
    MUIA_HelpLine
    MUIA_HelpNode
    MUIA_ObjectID
    MUIA_UserData
   
    areaclass:
     
    MUIA_ContextMenu
    MUIA_ContextMenuTrigger
    MUIA_ControlChar
    MUIA_CycleChain
    MUIA_Draggable
    MUIA_FillArea
    MUIA_Frame
    MUIA_FrameTitle
    MUIA_HorizWeight
    MUIA_Pressed
    MUIA_Selected is not explicitly filtered by Group.
    MUIA_ShortHelp
    MUIA_ShowMe
    MUIA_VertWeight
    MUIA_Weight
    
*/

/* Private attribute/method definitions */
#define MUIM_Group_Insert      (MUIB_MUI|0x00424d34)    /* MUI: V20 */
struct MUIP_Group_Insert
{
    STACKED ULONG MethodID;
    STACKED Object *obj;
    STACKED Object *pred;
};

#define MUIA_Group_ChildCount  0x80420322       /* MUI: V20 isg LONG */

struct layout2d_elem
{
    WORD min;
    WORD max;
    WORD dim;
    ULONG weight;
};


struct MUI_GroupData
{
    Object *family;
    struct Hook *layout_hook;
    ULONG flags;
    ULONG columns;
    ULONG rows;
    struct layout2d_elem *row_infos;
    struct layout2d_elem *col_infos;
    LONG active_page;
    ULONG horiz_spacing;
    ULONG vert_spacing;
    ULONG num_children;
    ULONG num_visible_children; /* for horiz/vert group only */
    ULONG horiz_weight_sum;
    ULONG vert_weight_sum;
    ULONG samesize_maxmin_horiz;
    ULONG samesize_maxmin_vert;
    ULONG update;               /* for MUI_Redraw() 1 - do not redraw the frame
                                 * 2 - the virtual pos has changed */
    struct MUI_EventHandlerNode ehn;
    LONG virt_offx, virt_offy;  /* diplay offsets */
    LONG old_virt_offx, old_virt_offy;  /* Saved virtual positions,
                                         * used for update == 2 */
    LONG virt_mwidth, virt_mheight;     /* The complete width */
    LONG saved_minwidth, saved_minheight;
    LONG dont_forward_get;      /* Set temporarily to 1 so that the get method
                                 * is not forwarded */
    LONG dont_forward_methods;  /* Set temporarily to 1, meaning that the
                                 * methods are not forwarded to the group's
                                 * children */
    /* MUI4 group with tabs */
    Object *titlegroup;
};

/* Note:
 * The MUI4 feature of group with tabs is implemented based on behaviour of
 * one application. What this application codes suggest it seems that passing
 * MUIV_Frame_Register together with MUIA_Group_PageMode, TRUE activates this
 * mode.
 * In such mode, the first passed group is used to register tab "titles" and
 * is always visible. The selection of object in this group selects the
 * matching (by position) group to be displayed
 */

#define GROUP_HORIZ       (1<<1)
#define GROUP_SAME_WIDTH  (1<<2)
#define GROUP_SAME_HEIGHT (1<<3)
#define GROUP_CHANGING    (1<<4)
#define GROUP_PAGEMODE    (1<<5)
#define GROUP_VIRTUAL     (1<<6)
#define GROUP_HSPACING    (1<<7)
#define GROUP_VSPACING    (1<<8)
#define GROUP_CHANGED     (1<<9)


/* During minmax calculations objects with a weight of 0 shall
   be treated like they had identical min/def/max size, ie. fixed size.
   
   During layout objects with 0 weight must be treated like fixed-sized
   too, but for hgroups only in x direction, and for vgroups only in
   y direction. I think ... */

#define w0_defwidth(x) (_hweight(x) ? _defwidth(x) : _minwidth(x))
#define w0_maxwidth(x) (_hweight(x) ? _maxwidth(x) : _minwidth(x))

#define w0_defheight(x) (_vweight(x) ? _defheight(x) : _minheight(x))
#define w0_maxheight(x) (_vweight(x) ? _maxheight(x) : _minheight(x))

/****** Group.mui/MUIC_Group ************
*
*   NAME
*       MUIC_Group
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Own an ordered set of Area-compatible children through an internal
*       Family. Arrange them horizontally, vertically, in a grid, as pages or
*       through a layout hook. Default orientation is vertical, with both stored
*       row and column counts initially one. Layout requires the connected
*       application's preferences and valid child min/max information.
*       Group owns the internal Family, its attached children and private grid
*       arrays. It borrows the layout Hook. Construction returns NULL on
*       superclass/Family creation failure or any explicit NULL Group_Child tag.
*       Disposal frees grid arrays and disposes the Family and remaining
*       children, then Area. Hide and Cleanup must already have ended active
*       display resources and handlers.
*       Only attach live compatible objects that are not already owned or linked
*       elsewhere. Adding adopts a child; removing unlinks without disposing it.
*       Serialize membership, layout and callbacks in the application task. The
*       implementation does not protect traversal against children destroying
*       themselves or changing the list.
*
*   METHODS
*       Setup calls Area then sets up every child, including children with
*       ShowMe FALSE. On child failure it cleans only the previously successful
*       children and returns FALSE; it does not clean the failing child or the
*       successful Area setup itself. Cleanup calls every child then Area.
*       Show/Hide select the active page or eligible visible children; child
*       results are ignored. Draw returns FALSE while InitChange is active.
*       AskMinMax asks visible children and finalizes their sizes, then uses
*       page layout, a custom hook or built-in layout in that priority order. It
*       returns zero regardless of results. Layout uses the same priority and
*       also returns zero, even after a hook failure or incomplete built-in
*       layout. The library MUI_Layout sets a child's box and returns TRUE
*       without propagating the child's Layout result.
*       FindUData/GetUData match Group first, then delegate through its internal
*       Family, whose own default UserData can also match. GetUData reports a
*       match without checking getter success. SetUData may set Group, whose
*       setter can already broadcast, and then visits the Family, so assignments
*       can repeat; SetUDataOnce stops at the first match. Export/Import visit
*       children only and return zero; they do not call Area's own
*       Selected-state serialization.
*       OM_GET tries Group, then Area/Notify, then children until one getter
*       succeeds. Unknown methods normally go to every child before Area; child
*       results are discarded. Explicit lifecycle/utility/drag cases have
*       different routes. DoMethodNoForward controls only the paths that inspect
*       its forwarding flag, not every explicit child traversal.
*
*   NOTES
*       MUIM_Notify first probes the group's trigger getter with child-get
*       forwarding disabled. If recognized locally, it registers through the
*       superclass. Otherwise it probes each child and registers on those
*       reporting support. This is a Group override of Notify, so an event-only
*       attribute without a getter is not automatically registered locally.
*       Probe storage is a fixed 30-IPTR stack array; getters must not write
*       beyond it. Return TRUE on the child path does not prove that any
*       registration succeeded.
*       Version and Revision getters both return 1. Family_GetChild is delegated
*       to the internal Family. Group also implements virtual offset/extent
*       attributes used by Virtgroup. The locally defined Group_Insert and
*       Group_ChildCount are internal source interfaces; the header's
*       ExitChange2 is explicitly private.
*
*   BUGS
*       Add/remove helpers ignore child and Family results and adjust a stored
*       count without validating membership. Setup has incomplete failure
*       unwinding. Page indices and grid dimensions are not generally
*       range-checked. Several layout intermediates are WORD values, and
*       allocation/arithmetic failures have no comprehensive status. Treat
*       documented results as implementation results, not stronger success
*       guarantees.
*
*/
/****** Group.mui/MUIA_Group_Child ************
*
*   NAME
*       MUIA_Group_Child
*
*   FUNCTION
*       Append a non-NULL Area-compatible child through OM_ADDMEMBER. The
*       internal Family owns attached children.
*
*   INPUTS
*       Object pointer; initialize only; repeatable.
*
*   NOTES
*       Runtime changes use AddHead/AddTail or OM_ADDMEMBER and Remove or
*       OM_REMMEMBER. Any explicit NULL Child tag makes construction fail after
*       tag processing and disposes attached children. Earlier superclass/Family
*       allocation failure can occur before child adoption. Do not dispose an
*       attached child separately.
*
*   BUGS
*       When the frame request is MUIV_Frame_Register, the first Child tag also
*       stores the internal titlegroup pointer, even without PageMode. Removing
*       or independently disposing this object does not clear that pointer.
*
*/
/****** Group.mui/MUIA_Group_ChildList ************
*
*   NAME
*       MUIA_Group_ChildList
*
*   FUNCTION
*       Return the borrowed child list owned by the internal Family.
*
*   INPUTS
*       Exec List pointer; get only.
*
*   NOTES
*       Use NextObject for traversal. Do not edit links, free the list or retain
*       it beyond Group disposal. Membership must remain stable during traversal
*       and callbacks. This is not a copied snapshot.
*
*/
/****** Group.mui/MUIA_Group_Horiz ************
*
*   NAME
*       MUIA_Group_Horiz
*
*   FUNCTION
*       Select horizontal one-dimensional layout when true; default FALSE
*       selects vertical.
*
*   INPUTS
*       BOOL; initialize; getter also implemented.
*
*   NOTES
*       This orientation is used when rows and columns both equal one and
*       neither PageMode nor LayoutHook takes precedence. There is no setter.
*       The getter returns the GROUP_HORIZ bit value (2 when set), not
*       normalized TRUE (1). Test it as nonzero.
*
*/
/****** Group.mui/MUIA_Group_PageMode ************
*
*   NAME
*       MUIA_Group_PageMode
*
*   FUNCTION
*       Enable page-group layout, default FALSE. One content child is shown at a
*       time; all pages contribute size constraints and are laid out.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       PageMode takes precedence over LayoutHook, Rows/Columns and Horiz. With
*       MUIV_Frame_Register, the first Child tag is treated as a title group and
*       excluded from content-page counting. Its minimum height is reserved
*       above the content; page changes set its Group_ActivePage. Group does not
*       create tab objects or install title-selection notifications itself.
*
*   BUGS
*       Show/Hide count all children except the title group, while Draw skips
*       ShowMe-FALSE children before incrementing the page count. Hidden pages
*       can therefore make these paths select different indices. There is no
*       PageMode setter or local getter.
*
*/
/****** Group.mui/MUIA_Group_ActivePage ************
*
*   NAME
*       MUIA_Group_ActivePage
*
*   FUNCTION
*       Choose a zero-based content-page index when PageMode is enabled. Default
*       zero. First selects zero, Last selects count minus one, Prev wraps from
*       zero, and Next/Advance both advance with modulo wrap. The title group,
*       if any, is excluded from that count.
*
*   INPUTS
*       LONG; initialize, set, get.
*
*   NOTES
*       A changed live page hides the previous selection, stores the new index,
*       lays out and shows the new selection, then redraws content. Before
*       PageMode is enabled this operation does nothing; initialization tag
*       order matters. Configure PageMode and children before using relative
*       selectors. The getter returns the stored index.
*
*   BUGS
*       Explicit indices are not clamped. Next/Advance divide by the content
*       count and are invalid for an empty page group. Last on an empty group
*       produces -1. Removing children does not repair an out-of-range active
*       index. See PageMode for hidden-child counting differences.
*
*/
/****** Group.mui/MUIA_Group_Columns ************
*
*   NAME
*       MUIA_Group_Columns
*
*   FUNCTION
*       Request a grid columns count, converted to at least one, and clear the
*       other dimension to zero so min/max calculation derives it from the total
*       child count.
*
*   INPUTS
*       LONG declaration, stored as ULONG; initialize and set.
*
*   NOTES
*       The last Rows/Columns tag wins. On min/max calculation, a nondivisible
*       request falls back to one row containing all children. Dimensions are
*       derived from total membership, while most cell passes skip hidden and
*       border-gadget children. A runtime setter requests Window recalculation
*       when RenderInfo exists. There is no local getter.
*
*   BUGS
*       Negative or oversized values are not rejected before unsigned
*       conversion/allocation arithmetic. There is no partial-last-row layout
*       guarantee. Grid-array allocation failure returns from an internal helper
*       without a public failure result; later Layout can do nothing.
*
*/
/****** Group.mui/MUIA_Group_Rows ************
*
*   NAME
*       MUIA_Group_Rows
*
*   FUNCTION
*       Request a grid rows count, converted to at least one, and clear the
*       other dimension to zero so min/max calculation derives it from the total
*       child count.
*
*   INPUTS
*       LONG declaration, stored as ULONG; initialize and set.
*
*   NOTES
*       The last Rows/Columns tag wins. On min/max calculation, a nondivisible
*       request falls back to one column containing all children. Dimensions are
*       derived from total membership, while most cell passes skip hidden and
*       border-gadget children. A runtime setter requests Window recalculation
*       when RenderInfo exists. There is no local getter.
*
*   BUGS
*       Negative or oversized values are not rejected before unsigned
*       conversion/allocation arithmetic. There is no partial-last-row layout
*       guarantee. Grid-array allocation failure returns from an internal helper
*       without a public failure result; later Layout can do nothing.
*
*/
/****** Group.mui/MUIA_Group_HorizSpacing ************
*
*   NAME
*       MUIA_Group_HorizSpacing
*
*   FUNCTION
*       Store the horizontal gap between children or grid cells. Before setup
*       the unspecified value is stored as ULONG -1; setup replaces it with the
*       corresponding application preference.
*
*   INPUTS
*       LONG declaration, stored as ULONG; initialize, set, get.
*
*   NOTES
*       Explicit initialization or setting marks the value as caller-specified
*       so setup no longer supplies that default. The getter returns the stored
*       value. A spacing setter alone does not request relayout; finish a
*       controlled change with ExitChange when live.
*
*   BUGS
*       No nonnegative/range validation is performed. Negative or very large
*       values can wrap in unsigned/WORD layout arithmetic. Do not interpret -1
*       as a documented runtime reset-to-default operation.
*
*/
/****** Group.mui/MUIA_Group_VertSpacing ************
*
*   NAME
*       MUIA_Group_VertSpacing
*
*   FUNCTION
*       Store the vertical gap between children or grid cells. Before setup the
*       unspecified value is stored as ULONG -1; setup replaces it with the
*       corresponding application preference.
*
*   INPUTS
*       LONG declaration, stored as ULONG; initialize, set, get.
*
*   NOTES
*       Explicit initialization or setting marks the value as caller-specified
*       so setup no longer supplies that default. The getter returns the stored
*       value. A spacing setter alone does not request relayout; finish a
*       controlled change with ExitChange when live.
*
*   BUGS
*       No nonnegative/range validation is performed. Negative or very large
*       values can wrap in unsigned/WORD layout arithmetic. Do not interpret -1
*       as a documented runtime reset-to-default operation.
*
*/
/****** Group.mui/MUIA_Group_Spacing ************
*
*   NAME
*       MUIA_Group_Spacing
*
*   FUNCTION
*       Set both horizontal and vertical gaps and mark both as explicit.
*
*   INPUTS
*       LONG declaration, stored as ULONG; initialize and set.
*
*   NOTES
*       Tag order determines the result when combined with the axis-specific
*       tags. There is no local combined getter or automatic relayout. Use axis
*       getters to inspect the stored values; spacing has the same unchecked
*       conversion limitations as those attributes.
*
*/
/****** Group.mui/MUIA_Group_SameWidth ************
*
*   NAME
*       MUIA_Group_SameWidth
*
*   FUNCTION
*       Enable same-width constraints in built-in min/max/layout calculations.
*       Default FALSE.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       The algorithms use the largest participating minimum and each child's
*       own limits; this is not an unconditional guarantee of identical final
*       dimensions. PageMode and custom hooks have their own behavior. There is
*       no runtime setter or local getter. SameSize sets or clears both flags;
*       later specific initialization tags can override one.
*
*/
/****** Group.mui/MUIA_Group_SameHeight ************
*
*   NAME
*       MUIA_Group_SameHeight
*
*   FUNCTION
*       Enable same-height constraints in built-in min/max/layout calculations.
*       Default FALSE.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       The algorithms use the largest participating minimum and each child's
*       own limits; this is not an unconditional guarantee of identical final
*       dimensions. PageMode and custom hooks have their own behavior. There is
*       no runtime setter or local getter. SameSize sets or clears both flags;
*       later specific initialization tags can override one.
*
*/
/****** Group.mui/MUIA_Group_SameSize ************
*
*   NAME
*       MUIA_Group_SameSize
*
*   FUNCTION
*       Enable both same-width and same-height constraints in built-in
*       min/max/layout calculations. Default FALSE.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       The algorithms use the largest participating minimum and each child's
*       own limits; this is not an unconditional guarantee of identical final
*       dimensions. PageMode and custom hooks have their own behavior. There is
*       no runtime setter or local getter. SameSize sets or clears both flags;
*       later specific initialization tags can override one.
*
*/
/****** Group.mui/MUIA_Group_LayoutHook ************
*
*   NAME
*       MUIA_Group_LayoutHook
*
*   FUNCTION
*       Borrow a custom layout hook used when PageMode is FALSE. NULL selects
*       built-in layout. Keep the Hook and callback code alive until replacement
*       or Group disposal.
*
*   INPUTS
*       Hook pointer; initialize; setter also implemented.
*
*   NOTES
*       CallHookPkt passes this Group as object and a temporary MUI_LayoutMsg as
*       message. lm_Children is the borrowed internal list; keep membership
*       stable. The hook return value is ignored for both message types;
*       returning MUILM_UNKNOWN does not request built-in fallback. There is no
*       local getter and replacing the hook does not itself relayout.
*       For MUILM_MINMAX, child limits have already been collected. Fill all six
*       lm_MinMax fields with content requirements excluding this group's
*       frame/spacing. This stack message is not initialized for you. Group
*       raises maxima and defaults below minima, caps aggregate maxima to
*       MUI_MAXMAX, then adds its Area contribution. It does not clamp a
*       too-large hook default back to the maximum.
*       For MUILM_LAYOUT, the message starts cleared; lm_Layout.Width/Height
*       contain available inner dimensions. Lay out children with MUI_Layout
*       using coordinates relative to the group inner origin. For virtual
*       groups, write the resulting virtual content Width/Height back to those
*       same fields. The private fields have no public contract. Do not retain
*       the message or list as owned storage.
*
*   BUGS
*       A hook that omits a min/max field leaves an uninitialized value in use.
*       Hook failure is not propagated; Group AskMinMax and Layout return zero
*       regardless. Group itself does not enforce the custom layout's child size
*       constraints.
*
*/
/****** Group.mui/MUIA_Group_Forward ************
*
*   NAME
*       MUIA_Group_Forward
*
*   FUNCTION
*       Control whether this OM_SET forwards its filtered tag list to children.
*       Default TRUE for each call; the last Forward tag controls that call.
*       FALSE still applies group/superclass changes.
*
*   INPUTS
*       BOOL; set only, per setter call.
*
*   NOTES
*       The flag is not stored and has no local getter. Suppression applies to
*       this setter's ordinary child broadcast, not to explicit child effects of
*       page switching or other operations. The setter mutates the caller's
*       writable tags to TAG_IGNORE for filtered attributes and does not restore
*       them.
*       The filter removes HelpLine, HelpNode, ObjectID, UserData, ContextMenu,
*       ContextMenuTrigger, ControlChar, CycleChain, Draggable, FillArea,
*       Group_ActivePage, Frame, FrameTitle, HorizWeight, Pressed, ShortHelp,
*       ShowMe, VertWeight, Weight, Virtgroup_Left/Top, AppMessage and Timer.
*       Selected is not explicitly filtered, but Area may already suppress an
*       unchanged Selected tag. Unfiltered group tags such as spacing can also
*       reach child groups.
*       Disabled is delayed: child forwarding prepends a NestedDisabled
*       operation and also passes Disabled, then the superclass applies the
*       group's Disabled tag. Repeated TRUE calls can accumulate child nesting
*       even though Disabled itself is Boolean. Forward FALSE omits that child
*       propagation.
*
*   BUGS
*       Multiple Disabled tags are initially changed to TAG_IGNORE, but only the
*       last one is restored and applied as the group's delayed value. Child
*       setters can mutate the shared tag list, affecting later children. There
*       is no tag-list copy per child.
*
*/
/****** Group.mui/MUIA_Group_Virtual ************
*
*   NAME
*       MUIA_Group_Virtual
*
*   FUNCTION
*       Enable virtual-content layout, clipping and edge autoscroll; default
*       FALSE. The constructor marks the object as a virtual group and
*       connection marks descendants as inside a virtual group.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       AskMinMax saves full content minima but exposes a viewport minimum of
*       Area's contribution plus two pixels per dimension. Layout computes
*       virtual extents and clamps offsets. Group implements Virtgroup_Left/Top
*       setters and getters plus Width/Height/MinWidth/MinHeight getters for
*       this machinery. It does not create scrollbar objects.
*       A click inside registers tick events; moving outside while tracking
*       scrolls four pixels per tick, clamped to extents. A later non-SELECTDOWN
*       button event stops tick tracking. There is no runtime Virtual setter or
*       local getter.
*
*   BUGS
*       Virtual offset setters can force Layout even without a live display;
*       callers must establish appropriate setup/layout state. Do not apply
*       virtual offsets to nonvirtual groups: the drawing cleanup path assumes
*       its scroll clip belongs to a virtual group.
*
*/

static const int __version = 1;
static const int __revision = 1;

/*
 * If message is non-NULL, show Area first and ignore its result. Page mode
 * shows title and selected page without ShowMe filtering; ordinary mode
 * checks ShowMe and virtual visibility. Ignore child results, return TRUE.
 * NULL message is an internal child-only path; requires valid drawable
 * geometry.
 */
IPTR Group__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg);
/*
 * Hide title/selected page in page mode or drawable children otherwise.
 * Call Area only for non-NULL message, returning its result, otherwise one.
 * Requires stable list and valid shown state. Does not cleanup or release
 * children; NULL message is the internal child-only path.
 */
IPTR Group__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg);

/*****************************************************************************/
/*****************************************************************************/


/*
 * Broadcast the borrowed message to every child unless dontforwardmethods
 * is set. Requires stable membership and writable message when recipients
 * mutate it. Discard every child result and return zero; no ownership
 * transfer. Explicit traversal methods do not use this suppression helper.
 */
static ULONG Group_DispatchMsg(struct IClass *cl, Object *obj, Msg msg);

/*
 * Change the stored page only in PageMode. Requires valid Group state and a
 * nonempty content set for Next/Advance modulo arithmetic.
 * Hide/layout/show/redraw the changed live selection and update the
 * borrowed title group. No bounds validation or error result; explicit
 * invalid indices can select no child.
 */
static void change_active_page(struct IClass *cl, Object *obj, LONG page)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    LONG newpage, num_children = data->num_children;

    if (!(data->flags & GROUP_PAGEMODE))
        return;

    if (data->titlegroup != NULL)
        num_children--;

    switch (page)
    {
    case MUIV_Group_ActivePage_First:
        newpage = 0;
        break;
    case MUIV_Group_ActivePage_Last:
        newpage = num_children - 1;
        break;
    case MUIV_Group_ActivePage_Prev:
        newpage = data->active_page - 1;
        if (newpage == -1)
            newpage = num_children - 1;
        break;
    case MUIV_Group_ActivePage_Next:
    case MUIV_Group_ActivePage_Advance:
        newpage = (data->active_page + 1) % num_children;
        break;
    default:
        newpage = page;
        break;
    }

    if (newpage != data->active_page)
    {
        if (_flags(obj) & MADF_CANDRAW)
            Group__MUIM_Hide(cl, obj, NULL);

        data->active_page = newpage;

        if (_flags(obj) & MADF_CANDRAW)
        {
            DoMethod(obj, MUIM_Layout);
            Group__MUIM_Show(cl, obj, NULL);
            data->update = 1;
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        }

        if (data->titlegroup)
            set(data->titlegroup, MUIA_Group_ActivePage, newpage);
    }
}

/**************************************************************************
 Returns the number of visible children. Visible children are all children
 that have MADF_SHOWME and not MADF_BORDERGADGET set.
**************************************************************************/
/*
 * Start with the stored child count and subtract hidden or border-gadget
 * children found in the borrowed list. Return the resulting integer; an
 * inconsistent stored count remains inconsistent. Requires stable
 * membership and valid child flags; no allocation or ownership transfer.
 */
static int Group_GetNumVisibleChildren(struct MUI_GroupData *data,
    struct MinList *children)
{
    int num_visible_children = data->num_children;
    APTR cstate;
    Object *child;

    /* As there can be invisible children we have to subtract those from
     * the total number of children */
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (IS_HIDDEN(child))
            num_visible_children--;
    }
    return num_visible_children;
}

/**************************************************************************
 Handles insertion of objects - works based on fact that IDs of all methods
 that use it are the same for Group and Family and that struct share obj at
 same offset.
**************************************************************************/
struct MUIP_StructWithObj
{
    STACKED ULONG MethodID;
    STACKED Object *obj;
};

/*
 * Shared private insertion path for AddHead/AddTail/Insert. Requires a
 * valid unowned Area child and valid predecessor for Insert. Family
 * insertion, connection and Setup results are ignored; count increases
 * unconditionally. Attach parent/virtual state and propagate disabled
 * nesting. Return TRUE, not verified insertion/setup success.
 */
static IPTR Group__MUIM_AddObject(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    struct MUIP_StructWithObj *msgint = (struct MUIP_StructWithObj *)msg;
    struct TagItem nestedDisableAttrs[4] = {
        { MUIA_NoNotify,        TRUE    },
        { MUIA_NestedDisabled,  TRUE    },
        { MUIA_Disabled,        FALSE   },
        { TAG_DONE                      }
    };

    DoMethodA(data->family, (Msg) msg);
    data->num_children++;
    if ((data->flags & GROUP_CHANGING) != 0)
        data->flags |= GROUP_CHANGED;

    /* if we are in an application tree, propagate pointers */
    if (muiNotifyData(obj)->mnd_GlobalInfo)
    {
        /* Only children of groups can have parents */

        if ((_flags(obj) & MADF_INVIRTUALGROUP)
            || (data->flags & GROUP_VIRTUAL))
        {
            _flags(msgint->obj) |= MADF_INVIRTUALGROUP;
        }

        DoMethod(msgint->obj, MUIM_ConnectParent, (IPTR) obj);
    }

    /* Ensure new children are disabled if their parent is */
    GET(obj, MUIA_Disabled, &nestedDisableAttrs[2].ti_Data);
    if (nestedDisableAttrs[2].ti_Data == (IPTR)TRUE)
    {
        SetAttrsA(msgint->obj, nestedDisableAttrs);
    }

    /* Some apps (Odyssey) expect _parent() will work before group tree is
     * added to application tree */
    muiNotifyData(msgint->obj)->mnd_ParentObject = obj;

    if (_flags(obj) & MADF_SETUP)
    {
        DoSetupMethod(msgint->obj, muiRenderInfo(obj));
    }
/*      if (_flags(obj) & MADF_CANDRAW) */
/*          DoShowMethod(msg->opam_Object); */

    return TRUE;
}

/**************************************************************************
 OM_NEW - Constructor
**************************************************************************/
/*
 * Construct Area, an owned Family and default Group state; consume
 * initialization tags in order. Adopt Child objects; remember first
 * register-frame child as borrowed titlegroup. Return object or NULL on
 * superclass/Family failure or explicit NULL child. Failure disposal
 * releases already adopted children. No grid arrays are allocated here.
 */
IPTR Group__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupData *data;
    struct TagItem *tags, *tag;
    BOOL bad_children = FALSE;
    IPTR disabled = FALSE;
    IPTR frame = MUIV_Frame_None;

    D(bug("[group.mui] OM_NEW, object 0x%p\n", obj));

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return 0;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    data->family = MUI_NewObjectA(MUIC_Family, NULL);
    if (!data->family)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->horiz_spacing = -1;
    data->vert_spacing = -1;
    data->columns = 1;
    data->rows = 1;
    data->active_page = 0;
    get(obj, MUIA_Frame, &frame);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Group_Child:
            D(bug("[group.mui] Adding child 0x%p\n", tag->ti_Data));
            if (tag->ti_Data)
            {
                DoMethod(obj, OM_ADDMEMBER, tag->ti_Data);
                /* Set first child as group title */
                if ((frame == MUIV_Frame_Register)
                    && (data->titlegroup == NULL))
                    data->titlegroup = (Object *) tag->ti_Data;
            }
            else
                bad_children = TRUE;
            break;

        case MUIA_Group_ActivePage:
            change_active_page(cl, obj, (LONG) tag->ti_Data);
            break;

        case MUIA_Group_Columns:
            data->columns = (tag->ti_Data) > 1 ? tag->ti_Data : 1;
            data->rows = 0;
            break;

        case MUIA_Group_Horiz:
            _handle_bool_tag(data->flags, tag->ti_Data, GROUP_HORIZ);
            break;

        case MUIA_Group_HorizSpacing:
            data->flags |= GROUP_HSPACING;
            data->horiz_spacing = tag->ti_Data;
            break;

        case MUIA_Group_LayoutHook:
            data->layout_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Group_PageMode:
            _handle_bool_tag(data->flags, tag->ti_Data, GROUP_PAGEMODE);
            break;

        case MUIA_Group_Rows:
            data->rows = MAX((ULONG) tag->ti_Data, 1);
            data->columns = 0;
            break;

        case MUIA_Group_SameHeight:
            _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_HEIGHT);
            break;

        case MUIA_Group_SameSize:
            _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_HEIGHT);
            _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_WIDTH);
            break;

        case MUIA_Group_SameWidth:
            _handle_bool_tag(data->flags, tag->ti_Data, GROUP_SAME_WIDTH);
            break;

        case MUIA_Group_Spacing:
            data->flags |= (GROUP_HSPACING | GROUP_VSPACING);
            data->horiz_spacing = tag->ti_Data;
            data->vert_spacing = tag->ti_Data;
            break;

        case MUIA_Group_VertSpacing:
            data->flags |= GROUP_VSPACING;
            data->vert_spacing = tag->ti_Data;
            break;

        case MUIA_Group_Virtual:
            _handle_bool_tag(data->flags, tag->ti_Data, GROUP_VIRTUAL);
            break;
        }
    }

    if (bad_children)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

/*      D(bug("Group_New(0x%p)\n",obj)); */

    if (data->flags & GROUP_VIRTUAL)
    {
        /* This is used by MUI_Render() to determine if group is virtual.
         * It then installs a clip region.
         * Also MUI_Layout() uses this. Probably for speed up reason  */
        _flags(obj) |= MADF_ISVIRTUALGROUP;
    }

#if (0)
    // kalamatee : disabled as unnecessary.
    // Object is created before children are added, so MUIA_Disabled is set
    // and OM_ADDMEMBER (called on the children) already does this.

    /* will forward MUIA_Disabled to children */
    get(obj, MUIA_Disabled, &disabled);
    if (disabled)
    {
        set(obj, MUIA_Disabled, TRUE);
    }
#endif

    /* This is only used for virtual groups */
    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;  /* Will be filled on demand */
    data->ehn.ehn_Priority = 10;        /* Will hear the click before all
                                         * other normal objects */
    data->ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;
    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/*
 * Free owned row/column arrays and dispose internal Family with remaining
 * children, then invoke Area disposal. Requires prior Hide/Cleanup of
 * active resources and no surviving borrowers. Does not dispose the
 * borrowed Hook separately; return superclass result.
 */
IPTR Group__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (data->row_infos != NULL)
        mui_free(data->row_infos);
    if (data->col_infos != NULL)
        mui_free(data->col_infos);
    if (data->family != NULL)
        MUI_DisposeObject(data->family);
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
/*
 * Apply Group tags, then Area, then optionally broadcast filtered writable
 * tags. Delay Disabled until after child propagation; only the last
 * Disabled tag is restored. Columns/Rows request Window recalculation;
 * spacing/hook alone do not. Virtual offsets may hide/layout/show/redraw
 * even outside normal setup. Return superclass setter result, ignoring
 * child/layout results; tag IDs can remain TAG_IGNORE.
 */
IPTR Group__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *disableTag = NULL;
    struct TagItem *tag;
    BOOL forward = TRUE;
    BOOL need_recalc = FALSE;
    IPTR retval;

    int virt_offx = data->virt_offx, virt_offy = data->virt_offy;

    /* There are many ways to find out what tag items provided by set()
     ** we do know. The best way should be using NextTagItem() and simply
     ** browsing through the list.
     */

    /* Parse group attributes before calling DoSuperMethodA(),
     ** otherwise if an app for example sets up a notification
     ** on MUIA_Group_ActivePage which calls a hook, and then
     ** the hook function calls get(obj, MUIA_Group_ActivePage),
     ** it would get returned the old active page instead of the new
     ** active page
     */

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Disabled:
            disableTag = tag;
            tag->ti_Tag = TAG_IGNORE;
            break;
        case MUIA_Group_Columns:
            data->columns = MAX((ULONG) tag->ti_Data, 1);
            data->rows = 0;
            need_recalc = TRUE;
            break;
        case MUIA_Group_ActivePage:
            change_active_page(cl, obj, (LONG) tag->ti_Data);
            break;
        case MUIA_Group_Forward:
            forward = tag->ti_Data;
            break;
        case MUIA_Group_HorizSpacing:
            data->flags |= GROUP_HSPACING;
            data->horiz_spacing = tag->ti_Data;
            break;
        case MUIA_Group_Rows:
            data->rows = MAX((ULONG) tag->ti_Data, 1);
            data->columns = 0;
            need_recalc = TRUE;
            break;
        case MUIA_Group_Spacing:
            data->flags |= (GROUP_HSPACING | GROUP_VSPACING);
            data->horiz_spacing = tag->ti_Data;
            data->vert_spacing = tag->ti_Data;
            break;
        case MUIA_Group_VertSpacing:
            data->flags |= GROUP_VSPACING;
            data->vert_spacing = tag->ti_Data;
            break;
        case MUIA_Virtgroup_Left:
            //kprintf("set virtgroup_left: %d\n", tag->ti_Data);
            virt_offx = tag->ti_Data;
            break;

        case MUIA_Group_LayoutHook:
            /*
               [ach] Seems like MUI supports setting this attribute after
               initialization, even though the documentation states
               otherwise. At least some programs use it...
             */
            data->layout_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Virtgroup_Top:
            //kprintf("set virtgroup_top: %d\n", tag->ti_Data);
            virt_offy = tag->ti_Data;
            break;
        }

    }

    if (muiRenderInfo(obj) && need_recalc)
        DoMethod(_win(obj), MUIM_Window_RecalcDisplay, (IPTR) obj);

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (disableTag)
        disableTag->ti_Tag = MUIA_Disabled;

    /* seems to be the documented behaviour, however it should be slow! */

    if (forward)
    {
        struct TagItem *attrsOrig = msg->ops_AttrList;
        struct TagItem nestedAttrs[2] = {
            { MUIA_NestedDisabled,  FALSE},
            { TAG_MORE,             (IPTR)msg->ops_AttrList}
        };

        /* Attributes which are to be filtered out, so that they are ignored
           when OM_SET is passed to group's children */

        tags = msg->ops_AttrList;
        while ((tag = NextTagItem(&tags)) != NULL)
        {
            switch (tag->ti_Tag)
            {
            case MUIA_Disabled:
                nestedAttrs[0].ti_Data = tag->ti_Data;
                msg->ops_AttrList = nestedAttrs;
                break;
            case MUIA_HelpLine:
            case MUIA_HelpNode:
            case MUIA_ObjectID:
            case MUIA_UserData:

            case MUIA_ContextMenu:
            case MUIA_ContextMenuTrigger:
            case MUIA_ControlChar:
            case MUIA_CycleChain:
            case MUIA_Draggable:
            case MUIA_FillArea:
            case MUIA_Group_ActivePage:
            case MUIA_Frame:
            case MUIA_FrameTitle:
            case MUIA_HorizWeight:
            case MUIA_Pressed:
            case MUIA_ShortHelp:
            case MUIA_ShowMe:
            case MUIA_VertWeight:
            case MUIA_Weight:
            case MUIA_Virtgroup_Left:
            case MUIA_Virtgroup_Top:
            case MUIA_AppMessage:
            case MUIA_Timer:
                tag->ti_Tag = TAG_IGNORE;
                break;
            case MUIA_Selected:
/*                D(bug("Group_Set(%p) MUIA_Selected forwarded\n", obj)); */
/*                          tag->ti_Tag = TAG_IGNORE; */
                break;
            }
        }

        Group_DispatchMsg(cl, obj, (Msg) msg);
        msg->ops_AttrList = attrsOrig;
    }

    if (disableTag)
    {
        struct TagItem disableAttrs[2] = {
            { MUIA_Disabled,  disableTag->ti_Data },
            { TAG_DONE, 		0}
        };
        struct TagItem *origTags = msg->ops_AttrList;
        msg->ops_AttrList = disableAttrs;
        DoSuperMethodA(cl, obj, (Msg) msg);
        msg->ops_AttrList = origTags;
    }

    if (virt_offx != data->virt_offx || virt_offy != data->virt_offy)
    {
        if (_flags(obj) & MADF_CANDRAW)
            Group__MUIM_Hide(cl, obj, NULL);
        data->virt_offx = virt_offx;
        data->virt_offy = virt_offy;
        /* Relayout ourselves. This will also relayout all the children */
        DoMethod(obj, MUIM_Layout);
        if (_flags(obj) & MADF_CANDRAW)
            Group__MUIM_Show(cl, obj, NULL);
        data->update = 2;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return retval;
}


/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Write recognized Group/virtual getters to caller storage, including
 * private MUIA_Group_ChildCount from the unchecked stored count. Otherwise
 * try Area then children until success unless forwarding is suppressed.
 * Return getter status. Borrowed ChildList stays Family-owned; Horiz
 * returns its raw flag bit.
 */
IPTR Group__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)

    struct MUI_GroupData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Version:
        STORE = __version;
        return 1;
    case MUIA_Revision:
        STORE = __revision;
        return 1;
    case MUIA_Group_ActivePage:
        STORE = data->active_page;
        return 1;
    case MUIA_Group_ChildList:
        return GetAttr(MUIA_Family_List, data->family, msg->opg_Storage);
    case MUIA_Group_Horiz:
        STORE = (data->flags & GROUP_HORIZ);
        return 1;
    case MUIA_Group_HorizSpacing:
        STORE = data->horiz_spacing;
        return 1;
    case MUIA_Group_VertSpacing:
        STORE = data->vert_spacing;
        return 1;
    case MUIA_Group_ChildCount:
        STORE = data->num_children;
        return 1;
    case MUIA_Virtgroup_Left:
        STORE = data->virt_offx;
        return 1;
    case MUIA_Virtgroup_Top:
        STORE = data->virt_offy;
        return 1;
    case MUIA_Virtgroup_Width:
        STORE = data->virt_mwidth;
        return 1;
    case MUIA_Virtgroup_Height:
        STORE = data->virt_mheight;
        return 1;
    case MUIA_Virtgroup_MinWidth:
        STORE = data->saved_minwidth;
        return 1;
    case MUIA_Virtgroup_MinHeight:
        STORE = data->saved_minheight;
        return 1;
    }

    /* our handler didn't understand the attribute, we simply pass
     ** it to our superclass now
     */
    if (DoSuperMethodA(cl, obj, (Msg) msg))
        return 1;

    /* seems to be the documented behaviour, however it should be slow! */
    if (!data->dont_forward_get && !data->dont_forward_methods)
    {
        APTR cstate;
        Object *child;
        struct MinList *ChildList = NULL;

        get(data->family, MUIA_Family_List, &(ChildList));
        cstate = ChildList->mlh_Head;
        while ((child = NextObject(&cstate)))
            if (DoMethodA(child, (Msg) msg))
                return 1;
    }
    return 0;
#undef STORE
}


/**************************************************************************
 MUIM_AddTail
**************************************************************************/
/****** Group.mui/MUIM_Group_AddTail ************
*
*   NAME
*       MUIM_Group_AddTail
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Group_AddTail, (IPTR)child);
*
*   FUNCTION
*       Insert a child at the end of the internal Family and increase the stored
*       count. Set its parent to this Group, connect it when application
*       information exists, and set it up immediately when the Group is set up.
*
*   INPUTS
*       child - Non-NULL live Area-compatible object, not already owned or
*       linked elsewhere. Ownership transfers through attachment.
*
*   RESULT
*       Always TRUE; Family, child connection and child Setup results are
*       ignored.
*
*   NOTES
*       For live structural changes, bracket the operations with
*       InitChange/ExitChange. Addition alone does not show or lay out the
*       child. If the group is disabled, the added child receives a no-notify
*       NestedDisabled increment and Disabled=TRUE. OM_ADDMEMBER uses the
*       AddTail path.
*
*   BUGS
*       No NULL/membership validation is provided by the Group helper; it
*       dereferences child even if Family rejects the insertion. A reported TRUE
*       does not establish successful setup. Subsequent removal does not undo
*       inherited disabled state.
*
*/
IPTR Group__MUIM_AddTail(struct IClass *cl, Object *obj,
    struct MUIP_Group_AddTail *msg)
{
    return Group__MUIM_AddObject(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AddHead
**************************************************************************/
/****** Group.mui/MUIM_Group_AddHead ************
*
*   NAME
*       MUIM_Group_AddHead
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Group_AddHead, (IPTR)child);
*
*   FUNCTION
*       Insert a child at the beginning of the internal Family and increase the
*       stored count. Set its parent to this Group, connect it when application
*       information exists, and set it up immediately when the Group is set up.
*
*   INPUTS
*       child - Non-NULL live Area-compatible object, not already owned or
*       linked elsewhere. Ownership transfers through attachment.
*
*   RESULT
*       Always TRUE; Family, child connection and child Setup results are
*       ignored.
*
*   NOTES
*       For live structural changes, bracket the operations with
*       InitChange/ExitChange. Addition alone does not show or lay out the
*       child. If the group is disabled, the added child receives a no-notify
*       NestedDisabled increment and Disabled=TRUE. OM_ADDMEMBER uses the
*       AddTail path.
*
*   BUGS
*       No NULL/membership validation is provided by the Group helper; it
*       dereferences child even if Family rejects the insertion. A reported TRUE
*       does not establish successful setup. Subsequent removal does not undo
*       inherited disabled state.
*
*/
IPTR Group__MUIM_AddHead(struct IClass *cl, Object *obj,
    struct MUIP_Group_AddHead *msg)
{
    return Group__MUIM_AddObject(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_Insert
**************************************************************************/
/*
 * Private source-local MUIM_Group_Insert forwards to the shared insertion
 * helper. Message carries child and predecessor; predecessor must be a
 * current child. Family converts it with _OBJECT without a NULL guard; do
 * not assume NULL safely requests head insertion. Adopts child and returns
 * TRUE without checking Family/setup results. This is not a public header
 * interface.
 */
IPTR Group__MUIM_Insert(struct IClass *cl, Object *obj,
    struct MUIP_Group_Insert *msg)
{
    return Group__MUIM_AddObject(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_Remove
**************************************************************************/
/****** Group.mui/MUIM_Group_Remove ************
*
*   NAME
*       MUIM_Group_Remove
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Group_Remove, (IPTR)child);
*
*   FUNCTION
*       Hide the child when the Group is drawable, clean it when the Group is
*       set up, disconnect it, decrement the stored count and remove it through
*       Family.
*
*   INPUTS
*       child - Non-NULL live child currently attached to this Group.
*
*   RESULT
*       Always TRUE; child and Family results are ignored.
*
*   NOTES
*       The child is not disposed; the caller resumes responsibility for its
*       lifetime. OM_REMMEMBER uses this path. Use InitChange/ExitChange for
*       live removal. Family removal also disconnects the child, so the
*       connected path can send DisconnectParent twice. The final Family removal
*       clears the parent pointer.
*
*   BUGS
*       No membership/count validation occurs. Passing an unrelated child can
*       corrupt ownership/counts. The titlegroup pointer and active page are not
*       repaired. Removal leaves any disabled state previously imposed on the
*       child intact.
*
*/
IPTR Group__MUIM_Remove(struct IClass *cl, Object *obj,
    struct MUIP_Group_Remove *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (_flags(obj) & MADF_CANDRAW)
        DoHideMethod(msg->obj);
    if (_flags(obj) & MADF_SETUP)
        DoMethod(msg->obj, MUIM_Cleanup);
    if (muiNotifyData(obj)->mnd_GlobalInfo)
    {
        DoMethod(msg->obj, MUIM_DisconnectParent);
        muiNotifyData(msg->obj)->mnd_ParentObject = NULL;

        _flags(msg->obj) &= ~MADF_INVIRTUALGROUP;
    }

    if ((data->flags & GROUP_CHANGING) != 0)
        data->flags |= GROUP_CHANGED;
    data->num_children--;
    DoMethodA(data->family, (Msg) msg);

    return TRUE;
}

/**************************************************************************
 MUIM_Family_GetChild
**************************************************************************/
/*
 * Delegate the message to the owned internal Family and return its result.
 * The returned child is borrowed; index/selectors and failure behavior
 * belong to Family_GetChild. Requires an intact Family and stable list.
 */
IPTR Group__MUIM_Family_GetChild(struct IClass *cl, Object *obj,
    struct MUIP_Group_Remove *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    return DoMethodA(data->family, (APTR) msg);
}

/**************************************************************************
 MUIM_ConnectParent
**************************************************************************/
/*
 * Connect Area first, then each child to this Group, assigning parent and
 * inherited virtual flags. Requires a valid connected parent and stable
 * child list. Ignore superclass/child results and return TRUE; the internal
 * Family is not itself connected here.
 */
IPTR Group__MUIM_ConnectParent(struct IClass *cl, Object *obj,
    struct MUIP_ConnectParent *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    DoSuperMethodA(cl, obj, (Msg) msg);

    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if ((_flags(obj) & MADF_INVIRTUALGROUP)
            || (data->flags & GROUP_VIRTUAL))
        {
            _flags(child) |= MADF_INVIRTUALGROUP;
        }

        /* Only children of groups can have parents */
        muiNotifyData(child)->mnd_ParentObject = obj;

        DoMethod(child, MUIM_ConnectParent, (IPTR) obj);
    }
    return TRUE;
}

/**************************************************************************
 MUIM_DisconnectParent
**************************************************************************/
/*
 * Disconnect every child, clear its parent and inherited virtual flag, then
 * disconnect Area. Requires stable live children and ended display
 * lifecycle. Return TRUE regardless of delegated results; do not dispose
 * children.
 */
IPTR Group__MUIM_DisconnectParent(struct IClass *cl, Object *obj,
    struct MUIP_ConnectParent *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethodA(child, (Msg) msg);
        muiNotifyData(child)->mnd_ParentObject = NULL;
        _flags(child) &= ~MADF_INVIRTUALGROUP;
    }
    DoSuperMethodA(cl, obj, (Msg) msg);
    return TRUE;
}

/*
 * Put group in exchange state
 */
/****** Group.mui/MUIM_Group_InitChange ************
*
*   NAME
*       MUIM_Group_InitChange
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Group_InitChange);
*
*   FUNCTION
*       Clear the changed marker and set the changing flag. Drawing returns
*       FALSE while that flag is set.
*
*   INPUTS
*       No arguments beyond MethodID. Keep the group and application alive
*       throughout the change sequence.
*
*   RESULT
*       Always TRUE.
*
*   NOTES
*       Pair with ExitChange around live add/remove or layout changes. This
*       begins neither a lock nor a rollback transaction. It does not hide
*       children or defer their connection/setup/cleanup.
*
*   BUGS
*       There is one flag, not a nesting counter. Calling InitChange twice on
*       the same object does not require two exits and clears the prior changed
*       marker.
*
*/
IPTR Group__MUIM_InitChange(struct IClass *cl, Object *obj,
    struct MUIP_Group_InitChange *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    data->flags &= ~GROUP_CHANGED;
    data->flags |= GROUP_CHANGING;
    return TRUE;
}


/*
 * Will recalculate display after dynamic adding/removing
 */
/****** Group.mui/MUIM_Group_ExitChange ************
*
*   NAME
*       MUIM_Group_ExitChange
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Group_ExitChange);
*
*   FUNCTION
*       Clear changing/changed flags and request Window recalculation when set
*       up, unless an ancestor group is still changing.
*
*   INPUTS
*       No arguments beyond MethodID. Requires a valid connected Group ancestor
*       chain for live recalculation.
*
*   RESULT
*       Always TRUE, including when recalculation is skipped or its result
*       ignored.
*
*   NOTES
*       Recalculation is attempted even without a recorded structural change;
*       the changed-only guard is disabled. An outer group's later exit is
*       responsible for updating a nested change. This is not a commit-success
*       report or a restoration of failed operations.
*
*/
IPTR Group__MUIM_ExitChange(struct IClass *cl, Object *obj,
    struct MUIP_Group_ExitChange *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    data->flags &= ~GROUP_CHANGING;

#if 0
    /* Code is invalid. ExitChange needs to re-layout each time (tested
     * with MUI 3.8 and MUI 4.0 on m68k).
     * This is temporary change until proper implementaion is in place. */
    if (data->flags & GROUP_CHANGED)
#endif
    {
        data->flags &= ~GROUP_CHANGED;

        if ((_flags(obj) & MADF_SETUP) && _win(obj))
        {
            Object *win = _win(obj);
            Object *parent = obj;

            /* CHECKME: Don't call RecalcDisplay if one of our parents is
               in GROUP_CHANGING state to prevent  crash with Zune prefs
               program NListtree page because NList/NListtree when
               killing tree images in MUIM_Cleanup uses InitChange/
               ExitChange. Zune prefs program uses InitChange/ExitChange
               when switching page -> nesting -> mess. */

            while ((parent = _parent(parent)))
            {
                struct MUI_GroupData *pdata = INST_DATA(cl, parent);

                if (parent == win)
                    break;

                if (pdata->flags & GROUP_CHANGING)
                {
                    return TRUE;
                }

            }

            DoMethod(win, MUIM_Window_RecalcDisplay, (IPTR) obj);
        }
    }

    return TRUE;
}


/*
 * Will recalculate display after dynamic adding/removing
 */
/*
 * Private header method: only leave change state when currently changing.
 * Ignore message flags, clear change markers and, when set up, request
 * Window recalculation unless an ancestor Group is changing. Requires valid
 * ancestor chain. Return TRUE regardless of skipped/failed recalculation;
 * not a nesting counter.
 */
IPTR Group__MUIM_ExitChange2(struct IClass *cl, Object *obj,
    struct MUIP_Group_ExitChange2 *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (data->flags & GROUP_CHANGING)
    {
        data->flags &= ~(GROUP_CHANGING | GROUP_CHANGED);

        if ((_flags(obj) & MADF_SETUP) && _win(obj))
        {
            Object *win = _win(obj);
            Object *parent = obj;

            /* CHECKME: Don't call RecalcDisplay if one of our parents is
               in GROUP_CHANGING state to prevent  crash with Zune prefs
               program NListtree page because NList/NListtree when
               killing tree images in MUIM_Cleanup uses InitChange/
               ExitChange. Zune prefs program uses InitChange/ExitChange
               when switching page -> nesting -> mess. */

            while ((parent = _parent(parent)))
            {
                struct MUI_GroupData *pdata = INST_DATA(cl, parent);

                if (parent == win)
                    break;

                if (pdata->flags & GROUP_CHANGING)
                {
                    return TRUE;
                }

            }

            DoMethod(win, MUIM_Window_RecalcDisplay, (IPTR) obj);
        }
    }

    return TRUE;
}


/*
 * Sort the family
 */
/****** Group.mui/MUIM_Group_Sort ************
*
*   NAME
*       MUIM_Group_Sort
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Group_Sort, (IPTR)first, ... ,
*       (IPTR)NULL);
*
*   FUNCTION
*       Replace the internal Family child order with the supplied
*       NULL-terminated sequence. The method temporarily changes MethodID to
*       Family_Sort, invokes Family, then restores Group_Sort.
*
*   INPUTS
*       first, ... - Every currently attached child exactly once in the desired
*       order, followed by NULL. A retained method message must be writable.
*
*   RESULT
*       Always TRUE; Family result is ignored.
*
*   NOTES
*       This is explicit ordering, not a comparison callback sort. It does not
*       update the stored child count, reconnect children, change active
*       page/titlegroup, mark a change or relayout. Use InitChange/ExitChange
*       for a live reorder.
*
*   BUGS
*       The implementation resets the list without validating completeness,
*       uniqueness or membership. Omitting a child detaches its list node
*       without cleanup/disposal/count repair; duplicates or foreign objects can
*       corrupt the list.
*
*/
IPTR Group__MUIM_Sort(struct IClass *cl, Object *obj,
    struct MUIP_Group_Sort *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    /* modify message */
    msg->MethodID = MUIM_Family_Sort;

    DoMethodA(data->family, (APTR) msg);

    /* restore original message */
    msg->MethodID = MUIM_Group_Sort;
    return TRUE;
}

/**************************************************************************
 MUIM_Group_DoMethodNoForward

 Executes the given method but does not forward it to the children
**************************************************************************/
/****** Group.mui/MUIM_Group_DoMethodNoForward ************
*
*   NAME
*       MUIM_Group_DoMethodNoForward
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Group_DoMethodNoForward, methodid,
*       ...);
*
*   FUNCTION
*       Set the private no-forward flag, dispatch the embedded method on this
*       object and clear the flag before returning.
*
*   INPUTS
*       methodid - ULONG MethodID beginning the embedded message.
*       ... - Its correctly laid-out argument fields. Keep the Group alive
*       through the call.
*
*   RESULT
*       The embedded method's result.
*
*   NOTES
*       This suppresses Group_DispatchMsg broadcasts and OM_GET fallback to
*       children. It does not suppress explicit loops in Setup/Cleanup,
*       Import/Export, user-data searches or the Group Notify override. It is
*       therefore not a universal self-only execution mechanism.
*
*   BUGS
*       The flag is set to one and later reset to zero rather than
*       saved/restored. Nested use can clear an outer suppression early. The
*       implementation passes the address of the declared ULONG DoMethodID field
*       as a message; use the ABI-defined structure layout for direct messages,
*       not an invented IPTR-array layout on every target.
*
*/
IPTR Group__MUIM_DoMethodNoForward(struct IClass *cl, Object *obj,
    struct MUIP_Group_DoMethodNoForward *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    IPTR rc;
    data->dont_forward_methods = 1;     /* disable forwarding */
    rc = DoMethodA(obj, (Msg) & msg->DoMethodID);
        /* Probably doesn't work correctly on AROS? */

    data->dont_forward_methods = 0;
    return rc;
}

/*
 * Propagate a method to group children.
 */
/*
 * Broadcast the borrowed message to every child unless dontforwardmethods
 * is set. Requires stable membership and writable message when recipients
 * mutate it. Discard every child result and return zero; no ownership
 * transfer. Explicit traversal methods do not use this suppression helper.
 */
static ULONG Group_DispatchMsg(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    if (data->dont_forward_methods)
        return TRUE;

    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethodA(child, (Msg) msg);
    }
    return TRUE;
}


/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Set up Area then all children including hidden children. Return FALSE on
 * superclass or child failure; on child failure clean only earlier
 * successful children, not Area or the failing child. Resolve unspecified
 * spacing from preferences. Register virtual mouse handler only after all
 * succeed. Return TRUE on completion.
 */
IPTR Group__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    Object *childFailed;
    struct MinList *ChildList = NULL;

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    ASSERT_VALID_PTR(muiGlobalInfo(obj));

    if (!(data->flags & GROUP_HSPACING))
        data->horiz_spacing = muiGlobalInfo(obj)->mgi_Prefs->group_hspacing;
    if (!(data->flags & GROUP_VSPACING))
        data->vert_spacing = muiGlobalInfo(obj)->mgi_Prefs->group_vspacing;
    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
#if 0                           /* SHOWME affects only show/hide */
        if (!(_flags(child) & MADF_SHOWME))
            continue;
#endif

        if (!DoSetupMethod(child, msg->RenderInfo))
        {
            /* Send MUIM_Cleanup to all objects that received MUIM_Setup.
             */
            childFailed = child;
            cstate = ChildList->mlh_Head;
            while ((child = NextObject(&cstate)) && (child != childFailed))
            {
#if 0                           /* SHOWME affects only show/hide */
                if (!(_flags(child) & MADF_SHOWME))
                    continue;
#endif
                DoMethod(child, MUIM_Cleanup);
            }
            return FALSE;
        }
    }

    if (data->flags & GROUP_VIRTUAL)
    {
        DoMethod(_win(obj), MUIM_Window_AddEventHandler,
            (IPTR) & data->ehn);
    }

    return TRUE;
}


/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Remove virtual event handler, clean every child, then clean Area and
 * return its result. Requires the matching live setup state. Child results
 * are ignored; no disposal or membership changes.
 */
IPTR Group__MUIM_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    if (data->flags & GROUP_VIRTUAL)
    {
        DoMethod(_win(obj), MUIM_Window_RemEventHandler,
            (IPTR) & data->ehn);
    }

    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
#if 0                           /* SHOWME affects only show/hide */
        if (!(_flags(child) & MADF_SHOWME))
            continue;
#endif
        DoMethodA(child, (Msg) msg);
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Allocate a Region covering the group minus drawable child rectangles
 * clipped to its inner box, selecting active page where needed. Return
 * owned Region or NULL on allocation failure. Caller must dispose it or
 * transfer it to MUI_AddClipRegion, which consumes it even on failure.
 * Requires live geometry; page counting includes hidden children.
 */
static struct Region *group_children_clip_region(struct IClass *cl,
    Object *obj)
{
    struct Region *region = NULL;

    region = NewRegion();
    if (region)
    {
        struct MUI_GroupData *data = INST_DATA(cl, obj);
        struct Rectangle rect;
        LONG page = -1;
        struct MinList *ChildList = NULL;
        APTR cstate;
        Object *child;

        rect.MinX = _left(obj);
        rect.MinY = _top(obj);
        rect.MaxX = _right(obj);
        rect.MaxY = _bottom(obj);

        OrRectRegion(region, &rect);
        get(data->family, MUIA_Family_List, &ChildList);
        cstate = ChildList->mlh_Head;
        while ((child = NextObject(&cstate)))
        {
            if (child != data->titlegroup)
                ++page;

            if ((data->flags & GROUP_PAGEMODE) && (page != data->active_page)
                && (child != data->titlegroup))
                continue;

            if ((muiAreaData(child)->mad_Flags & MADF_CANDRAW)
                && (_width(child) > 0) && (_height(child) > 0))
            {
                rect.MinX = MAX(_left(child), _mleft(obj));
                rect.MinY = MAX(_top(child), _mtop(obj));
                rect.MaxX = MIN(_right(child), _mright(obj));
                rect.MaxY = MIN(_bottom(child), _mbottom(obj));

                if ((rect.MaxX >= rect.MinX) && (rect.MaxY >= rect.MinY))
                {
                    ClearRectRegion(region, &rect);
                }
            }
        }
    }

    return region;
}

/**************************************************************************
 MUIM_Draw - draw the group
**************************************************************************/
/*
 * Draw Area and children under temporary regions, with separate page-switch
 * and virtual-scroll update paths. Return FALSE during InitChange,
 * otherwise normally TRUE; ignore child draw results. Requires drawable
 * render state and stable membership. Scroll path updates old offsets.
 * Nonvirtual use of virtual offset updates can leave a clip installed
 * because final removal is virtual-only.
 */
IPTR Group__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;
    struct Rectangle group_rect;        /* child_rect; */
    int page;
    struct Region *region = NULL;
    APTR clip = (APTR) - 1;

    if (data->flags & GROUP_CHANGING)
        return FALSE;

    if (muiGlobalInfo(obj)->mgi_Prefs->window_redraw
        == WINDOW_REDRAW_WITHOUT_CLEAR)
    {
        struct Region *r = group_children_clip_region(cl, obj);
        APTR c = (APTR)-1;
        if (r)
            c = MUI_AddClipRegion(muiRenderInfo(obj), r);

        DoSuperMethodA(cl, obj, (Msg) msg);

        if (r)
            MUI_RemoveClipRegion(muiRenderInfo(obj), c);
    }
    else
    {
        DoSuperMethodA(cl, obj, (Msg) msg);
    }

    if ((msg->flags & MADF_DRAWUPDATE) && data->update == 1)
    {
        struct Region *r = NULL;
        APTR c = (APTR)-1;

        if (muiGlobalInfo(obj)->mgi_Prefs->window_redraw
            == WINDOW_REDRAW_WITHOUT_CLEAR)
            r = group_children_clip_region(cl, obj);

        if (r)
            c = MUI_AddClipRegion(muiRenderInfo(obj), r);

        /*
         * update is set when changing active page of a page group
         * need to redraw background ourself
         */
        DoMethod(obj, MUIM_DrawBackground,
            _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj),
            _mleft(obj), _mtop(obj), 0);

        if (r)
            MUI_RemoveClipRegion(muiRenderInfo(obj), c);
    }
    else
    {
        if ((msg->flags & MADF_DRAWUPDATE) && data->update == 2)
        {
            LONG left, top, right, bottom;
            LONG diff_virt_offx = data->virt_offx - data->old_virt_offx;
            LONG diff_virt_offy = data->virt_offy - data->old_virt_offy;
            struct Rectangle rect;
            struct Rectangle *clip_rect = &muiRenderInfo(obj)->mri_ClipRect;

            data->update = 0;

            if (!diff_virt_offx && !diff_virt_offy)
            {
                return 1;
            }

            /* sba: I don't know how MUI handle this but ScrollRasterBF() made problems when scrolling
             ** a (partly visible) virtual groups in a virtual group, because e.g. _mtop() is then
             ** smaller than the region. ScrollRasterBF() on AmigaOS then marks the complete region
             ** as damaged. Using ScrollWindowRaster() solved that problem but it flickers then.
             ** To avoid this we prevent that the scroll area is out of the region bounds.
             ** The region bounds are setted in MUI_Redraw() but should probably should go in the
             ** MUI's clip functions
             */

            left = MAX(_mleft(obj), clip_rect->MinX);
            top = MAX(_mtop(obj), clip_rect->MinY);
            right = MIN(_mright(obj), clip_rect->MaxX);
            bottom = MIN(_mbottom(obj), clip_rect->MaxY);

            /* old code was
             ** ScrollRasterBF(_rp(obj), diff_virt_offx, diff_virt_offy, _mleft(obj), _mtop(obj), _mright(obj),_mbottom(obj));
             */

            ScrollWindowRaster(_window(obj), diff_virt_offx, diff_virt_offy,
                left, top, right, bottom);

            if ((region = NewRegion()))
            {
                if (diff_virt_offx)
                {
                    rect.MinY = top;
                    rect.MaxY = bottom;

                    if (diff_virt_offx > 0)
                    {
                        rect.MinX = right - diff_virt_offx + 1;
                        if (rect.MinX < left)
                            rect.MinX = left;
                        rect.MaxX = right;
                    }
                    else
                    {
                        rect.MinX = left;
                        rect.MaxX = left - diff_virt_offx - 1;
                        if (rect.MaxX > right)
                            rect.MaxX = right;
                    }

                    if (rect.MinX <= rect.MaxX)
                    {
                        DoMethod(obj, MUIM_DrawBackground,
                            rect.MinX, rect.MinY,
                            rect.MaxX - rect.MinX + 1,
                            rect.MaxY - rect.MinY + 1,
                            rect.MinX, rect.MinY, 0);

                        OrRectRegion(region, &rect);
                    }
                }

                if (diff_virt_offy)
                {
                    rect.MinX = left;
                    rect.MaxX = right;

                    if (diff_virt_offy > 0)
                    {
                        rect.MinY = bottom - diff_virt_offy + 1;
                        if (rect.MinY < top)
                            rect.MinY = top;
                        rect.MaxY = bottom;
                    }
                    else
                    {
                        rect.MinY = top;
                        rect.MaxY = top - diff_virt_offy - 1;
                        if (rect.MaxY > bottom)
                            rect.MaxY = bottom;
                    }
                    if (rect.MinY <= rect.MaxY)
                    {
                        DoMethod(obj, MUIM_DrawBackground,
                            rect.MinX, rect.MinY,
                            rect.MaxX - rect.MinX + 1,
                            rect.MaxY - rect.MinY + 1,
                            rect.MinX, rect.MinY, 0);

                        OrRectRegion(region, &rect);
                    }
                }
            }

        }
        else
        {
            if (!(msg->flags & MADF_DRAWOBJECT)
                && !(msg->flags & MADF_DRAWALL))
                return TRUE;
        }
    }

    if (data->flags & GROUP_VIRTUAL && !region)
    {
        /* Not really needed if MUI Draws all the objects, maybe that's
         * what DRAWALL is for??? */
        if ((region = NewRegion()))
        {
            struct Rectangle rect;
            rect.MinX = _mleft(obj);
            rect.MinY = _mtop(obj);
            rect.MaxX = _mright(obj);
            rect.MaxY = _mbottom(obj);
            OrRectRegion(region, &rect);
        }
    }

    /* Add clipping region if we have one */
    if (region)
        clip = MUI_AddClipRegion(muiRenderInfo(obj), region);

    group_rect = muiRenderInfo(obj)->mri_ClipRect;
    page = -1;
    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (!(_flags(child) & MADF_SHOWME))
            continue;

        if (child != data->titlegroup)
            ++page;

        if ((data->flags & GROUP_PAGEMODE) && ((page != data->active_page)
            && (child != data->titlegroup)))
        {
            continue;
        }

        if ((data->flags & GROUP_PAGEMODE) && (child == data->titlegroup)
            && (msg->flags & MADF_DRAWUPDATE) && (data->update == 1))
        {
            /* Do not issue a re-draw to title group during page switch.
             * The group will re-draw itself due to setting of
             * MUIA_Group_ActivePage attribute.
             */
            continue;
        }

        MUI_Redraw(child, MADF_DRAWOBJECT);
        muiRenderInfo(obj)->mri_ClipRect = group_rect;
    }

    if (data->flags & GROUP_VIRTUAL && region && clip != (APTR) - 1)
    {
        MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    }

    data->old_virt_offx = data->virt_offx;
    data->old_virt_offy = data->virt_offy;
    data->update = 0;

    return TRUE;
}


#define END_MINMAX() \
    tmp.MaxHeight = MAX(tmp.MaxHeight, tmp.MinHeight); \
    tmp.MaxWidth = MAX(tmp.MaxWidth, tmp.MinWidth); \
    tmp.DefHeight = CLAMP(tmp.DefHeight, tmp.MinHeight, tmp.MaxHeight); \
    tmp.DefWidth = CLAMP(tmp.DefWidth, tmp.MinWidth, tmp.MaxWidth); \
    msg->MinMaxInfo->MinWidth += tmp.MinWidth; \
    msg->MinMaxInfo->MinHeight += tmp.MinHeight; \
    msg->MinMaxInfo->MaxWidth += tmp.MaxWidth; \
    msg->MinMaxInfo->MaxHeight += tmp.MaxHeight; \
    msg->MinMaxInfo->DefWidth += tmp.DefWidth; \
    msg->MinMaxInfo->DefHeight += tmp.DefHeight;

/*
 * MinMax calculation function. When this is called,
 * the children of your group have already been asked
 * about their min/max dimension so you can use their
 * dimensions to calculate yours.
 *
 * Notes:
 * - Init minwidth and maxwidth with size needed for total child spacing.
 * - 1st pass to find maximum minimum width, to set minwidth of each child
 * if they should have the same width (for a row of buttons ...)
 * - Adjust minwidth w/o making object bigger than their max size.
 */
/*
 * Accumulate visible non-border children horizontally, using spacing, same-
 * width minimum and child limits/weights. Add normalized content limits to
 * caller MinMaxInfo and cache weight sums. Requires finalized child sizes;
 * no allocation or status. Zero weights constrain maxima to minima;
 * arithmetic is not overflow-checked.
 */
static void group_minmax_horiz(struct IClass *cl, Object *obj,
    struct MinList *children, struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MUI_MinMax tmp;
    WORD maxminwidth = 0;
    BOOL found_nonzero_vweight = FALSE;

    tmp.MinHeight = 0;
    tmp.DefHeight = 0;
    tmp.MaxHeight = MUI_MAXMAX;
    if (data->num_visible_children > 0)
    {
        tmp.MinWidth = tmp.DefWidth = tmp.MaxWidth =
            (data->num_visible_children - 1) * data->horiz_spacing;
    }
    else
    {
        tmp.MinWidth = tmp.DefWidth = 0;
        tmp.MaxWidth = MUI_MAXMAX;
    }

    if (data->flags & GROUP_SAME_WIDTH)
    {
        cstate = children->mlh_Head;
        while ((child = NextObject(&cstate)))
        {
            if (IS_HIDDEN(child))
                continue;
            maxminwidth = MAX(maxminwidth, _minwidth(child));
        }
    }

    data->samesize_maxmin_horiz = maxminwidth;
/*    D(bug("group_minmax_horiz(%p) : maxminwidth=%d\n", obj, maxminwidth)); */

    data->horiz_weight_sum = 0;
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        WORD minwidth;

        if (IS_HIDDEN(child))
            continue;
        if (data->flags & GROUP_SAME_WIDTH)
        {
            minwidth = MAX(maxminwidth, _minwidth(child));
            minwidth = MIN(minwidth, _maxwidth(child));
        }
        else
            minwidth = _minwidth(child);
        tmp.MinWidth += minwidth;
        tmp.DefWidth += w0_defwidth(child);
        tmp.MaxWidth += w0_maxwidth(child);
        tmp.MaxWidth = MIN(tmp.MaxWidth, MUI_MAXMAX);
        tmp.MinHeight = MAX(tmp.MinHeight, _minheight(child));
        tmp.DefHeight = MAX(tmp.DefHeight, _defheight(child));
        /*
           if all children have null weight then maxheight=minheight
           if all but some children have null weights, the maxheight
           is the min of all maxheights
         */
        tmp.MaxHeight = MIN(tmp.MaxHeight, _maxheight(child));
        data->horiz_weight_sum += _hweight(child);
        if (_vweight(child) > 0)
        {
            found_nonzero_vweight = TRUE;
        }
    }
    if (!found_nonzero_vweight)
    {
        tmp.MaxHeight = tmp.MinHeight;
    }

    //if (data->flags & GROUP_VIRTUAL)
    //{
    //kprintf("# min %d x %d  def %d x %d  max %d x %d\n",
    //                tmp.MinWidth, tmp.MinHeight,
    //                tmp.DefWidth, tmp.DefHeight,
    //                tmp.MaxWidth, tmp.MaxHeight);
    //}

    END_MINMAX();
}

/* minmax calculation for vertical groups (see group_minmax_horiz)
*/
/*
 * Accumulate visible non-border children vertically and cache weights.
 * SameHeight can mutate child minimum heights within their limits. Add
 * normalized content limits to caller MinMaxInfo. Requires finalized child
 * sizes and valid writable message; no status or overflow checking.
 */
static void group_minmax_vert(struct IClass *cl, Object *obj,
    struct MinList *children, struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MUI_MinMax tmp;
    WORD maxminheight = 0;
    BOOL found_nonzero_hweight = FALSE;

    tmp.MinWidth = 0;
    tmp.DefWidth = 0;
    tmp.MaxWidth = MUI_MAXMAX;
    if (data->num_visible_children > 0)
    {
        tmp.MinHeight = tmp.DefHeight = tmp.MaxHeight =
            (data->num_visible_children - 1) * data->vert_spacing;
    }
    else
    {
        tmp.MinHeight = tmp.DefHeight = 0;
        tmp.MaxHeight = MUI_MAXMAX;
    }

    if (data->flags & GROUP_SAME_HEIGHT)
    {
        cstate = children->mlh_Head;
        while ((child = NextObject(&cstate)))
        {
            if (IS_HIDDEN(child))
                continue;
            maxminheight = MAX(maxminheight, _minheight(child));
        }
    }

    data->samesize_maxmin_vert = maxminheight;
    data->vert_weight_sum = 0;
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (IS_HIDDEN(child))
            continue;

        if (data->flags & GROUP_SAME_HEIGHT)
            _minheight(child) = MIN(maxminheight, w0_maxheight(child));
        tmp.MinHeight += _minheight(child);
        tmp.DefHeight += w0_defheight(child);
        tmp.MaxHeight += w0_maxheight(child);
        tmp.MaxHeight = MIN(tmp.MaxHeight, MUI_MAXMAX);
        tmp.MinWidth = MAX(tmp.MinWidth, _minwidth(child));
        tmp.DefWidth = MAX(tmp.DefWidth, _defwidth(child));
        tmp.MaxWidth = MIN(tmp.MaxWidth, _maxwidth(child));
        data->vert_weight_sum += _vweight(child);
        if (_hweight(child) > 0)
        {
            found_nonzero_hweight = TRUE;
        }
    }
    if (!found_nonzero_hweight)
    {
        tmp.MaxWidth = tmp.MinWidth;
    }

    END_MINMAX();
}


/*
 * Update allocated row constraints from visible non-border children in row-
 * major order. SameHeight adjusts child minima/defaults subject to zero-
 * weight maxima. Requires arrays/dimensions established by grid
 * calculation. Mutates child size data and row records; no allocation or
 * result.
 */
static void
minmax_2d_rows_pass(struct MUI_GroupData *data, struct MinList *children,
    struct MUI_MinMax *req, WORD maxmin_height, WORD maxdef_height)
{
    int i, j;
    APTR cstate;
    Object *child;

    /* do not rewind after the while, to process line by line */
    cstate = children->mlh_Head;
    /* for each row */
    for (i = 0; i < data->rows; i++)
    {
        /* calculate min and max height of this row */
        int min_h = 0, def_h = 0, max_h = MUI_MAXMAX;
        BOOL found_nonzero_vweight = FALSE;

        data->row_infos[i].weight = 0;

        j = 0;
        while ((child = NextObject(&cstate)))
        {
            if (IS_HIDDEN(child))
                continue;
            if (data->flags & GROUP_SAME_HEIGHT)
            {
                _minheight(child) = MIN(maxmin_height, w0_maxheight(child));
                _defheight(child) = MIN(maxdef_height, w0_maxheight(child));
            }
            min_h = MAX(min_h, _minheight(child));
            def_h = MAX(def_h, w0_defheight(child));
            max_h = MIN(max_h, _maxheight(child));
            if (_vweight(child) > 0)
            {
                found_nonzero_vweight = TRUE;
                data->row_infos[i].weight += _vweight(child);
            }
            ++j;
            if ((j % data->columns) == 0)
                break;
        }
        if (!found_nonzero_vweight)
            max_h = min_h;
        else
            max_h = MAX(max_h, min_h);
/*          D(bug("row %d : min_h=%d max_h=%d\n", i, min_h, max_h)); */

        data->row_infos[i].min = min_h;
        data->row_infos[i].max = max_h;
        data->vert_weight_sum += data->row_infos[i].weight;

        req->MinHeight += min_h;
        req->DefHeight += def_h;
        req->MaxHeight += max_h;
        if (req->MaxHeight > MUI_MAXMAX)
            req->MaxHeight = MUI_MAXMAX;
    }
}


/*
 * Accumulate column constraints by child position modulo the column count,
 * skipping hidden/border children. SameWidth adjusts child minima/defaults
 * subject to zero-weight maxima. Requires valid nonzero dimensions and
 * allocated arrays. Mutates records and child limits; no result.
 */
static void
minmax_2d_columns_pass(struct MUI_GroupData *data, struct MinList *children,
    struct MUI_MinMax *req, WORD maxmin_width, WORD maxdef_width)
{
    int i, j;
    APTR cstate;
    Object *child;

    for (i = 0; i < data->columns; i++)
    {
        /* calculate min and max width of this column */
        int min_w = 0, def_w = 0, max_w = MUI_MAXMAX;
        BOOL found_nonzero_hweight = FALSE;

        data->col_infos[i].weight = 0;

        j = 0;
        /* process all children to get children on a column */
        cstate = children->mlh_Head;
        while ((child = NextObject(&cstate)))
        {
            if (IS_HIDDEN(child))
                continue;
            ++j;
            if (((j - 1) % data->columns) != i)
                continue;
            if (data->flags & GROUP_SAME_WIDTH)
            {
                _minwidth(child) = MIN(maxmin_width, w0_maxwidth(child));
                _defwidth(child) = MIN(maxdef_width, w0_maxwidth(child));
            }
            min_w = MAX(min_w, _minwidth(child));
            def_w = MAX(def_w, w0_defwidth(child));

            /* this handles the case of null weight children, which limit
             *  the max size if they're alone, but not if they are with
             *  non-null weight obj
             */
            max_w = MIN(max_w, _maxwidth(child));
            if (_hweight(child) > 0)
            {
                found_nonzero_hweight = TRUE;
                data->col_infos[i].weight += _hweight(child);
            }
        }
        if (!found_nonzero_hweight)
            max_w = min_w;
        else
            max_w = MAX(max_w, min_w);
/*          D(bug("col %d : min_w=%d max_w=%d\n", i, min_w, max_w)); */

        data->col_infos[i].min = min_w;
        data->col_infos[i].max = max_w;
        data->horiz_weight_sum += data->col_infos[i].weight;

        req->MinWidth += min_w;
        req->DefWidth += def_w;
        req->MaxWidth += max_w;
        if (req->MaxWidth > MUI_MAXMAX)
            req->MaxWidth = MUI_MAXMAX;
    }
}

/*
 * Derive grid dimensions from total child count; nondivisible requests fall
 * back to one full row/column. Replace owned constraint arrays, scan child
 * limits and perform row/column passes. Add spacing and aggregate
 * constraints to message. Allocation failure returns early with no status
 * and can leave only one newly allocated array; later Layout must validate
 * state.
 */
static void
group_minmax_2d(struct IClass *cl, Object *obj,
    struct MinList *children, struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MUI_MinMax tmp;
    WORD maxmin_width;
    WORD maxmin_height;
    WORD maxdef_width;
    WORD maxdef_height;

    if (!data->columns)
    {
        if (data->num_children % data->rows)
        {
            data->columns = 1;
            data->rows = data->num_children;
        }
        else
            data->columns = data->num_children / data->rows;
    }
    else
    {
        if (data->num_children % data->columns)
        {
            data->rows = 1;
            data->columns = data->num_children;
        }
        else
            data->rows = data->num_children / data->columns;
    }

    if (data->columns < 1)
        data->columns = 1;
    if (data->rows < 1)
        data->rows = 1;

    if (data->row_infos != NULL)
        mui_free(data->row_infos);

    data->row_infos = mui_alloc(data->rows * sizeof(struct layout2d_elem));
    if (NULL == data->row_infos)
        return;

    if (data->col_infos != NULL)
        mui_free(data->col_infos);

    data->col_infos =
        mui_alloc(data->columns * sizeof(struct layout2d_elem));
    if (NULL == data->col_infos)
        return;

    data->horiz_weight_sum = 0;
    data->vert_weight_sum = 0;

    tmp.MinHeight = tmp.DefHeight = tmp.MaxHeight =
        (data->rows - 1) * data->vert_spacing;
    tmp.MinWidth = tmp.DefWidth = tmp.MaxWidth =
        (data->columns - 1) * data->horiz_spacing;
    /* get minimum dims if same dims for all children are needed */
    maxmin_width = 0;
    maxmin_height = 0;
    maxdef_width = 0;
    maxdef_height = 0;

    if ((data->flags & GROUP_SAME_WIDTH)
        || (data->flags & GROUP_SAME_HEIGHT))
    {
        cstate = children->mlh_Head;
        while ((child = NextObject(&cstate)))
        {
            if (!(_flags(child) & MADF_SHOWME))
                continue;
            maxmin_width = MAX(maxmin_width, _minwidth(child));
            maxmin_height = MAX(maxmin_height, _minheight(child));
            maxdef_width = MAX(maxdef_width, w0_defwidth(child));
            maxdef_height = MAX(maxdef_height, w0_defheight(child));
        }
/*        g_print("2d group: mminw=%d mminh=%d\n", */
/*            maxmin_width, maxmin_height); */
    }
    if (data->flags & GROUP_SAME_HEIGHT)
        data->samesize_maxmin_vert = maxmin_height;
    else
        data->samesize_maxmin_vert = 0;

    if (data->flags & GROUP_SAME_WIDTH)
        data->samesize_maxmin_horiz = maxmin_width;
    else
        data->samesize_maxmin_horiz = 0;

    minmax_2d_rows_pass(data, children, &tmp, maxmin_height, maxdef_height);
    minmax_2d_columns_pass(data, children, &tmp, maxmin_width,
        maxdef_width);

    END_MINMAX();
}


/*
 * Combine ShowMe content-page limits, excluding titlegroup; add title
 * minimum height but not title width. Normalize and add content constraints
 * to message. Requires finalized children and valid borrowed title pointer.
 * Does not allocate or return status; hidden pages do not contribute.
 */
static void
group_minmax_pagemode(struct IClass *cl, Object *obj,
    struct MinList *children, struct MUIP_AskMinMax *msg)
{
    APTR cstate;
    Object *child;
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    struct MUI_MinMax tmp = { 0, 0, MUI_MAXMAX, MUI_MAXMAX, 0, 0 };

    cstate = children->mlh_Head;

    D(bug("minmax_pagemode(0x%p) defw = %ld\n", obj, tmp.DefWidth));

    while ((child = NextObject(&cstate)))
    {
        if (!(_flags(child) & MADF_SHOWME))
            continue;

        if (child == data->titlegroup)
            continue;

        tmp.MinHeight = MAX(tmp.MinHeight, _minheight(child));
        D(bug("minmax_pagemode(0x%p) minh child = %d tmpmin=%d\n", obj,
                _minheight(child), tmp.MinHeight));
        tmp.MinWidth = MAX(tmp.MinWidth, _minwidth(child));
        tmp.MaxHeight = MIN(tmp.MaxHeight, w0_maxheight(child));
        tmp.MaxWidth = MIN(tmp.MaxWidth, w0_maxwidth(child));
        tmp.DefHeight = MAX(tmp.DefHeight,
            ((w0_defheight(child) <
                    MUI_MAXMAX) ? w0_defheight(child) : tmp.DefHeight));
        tmp.DefWidth =
            MAX(tmp.DefWidth,
            ((w0_defwidth(child) <
                    MUI_MAXMAX) ? w0_defwidth(child) : tmp.DefWidth));
        D(bug("minmax_pagemode(0x%p) defw = %ld\n", obj, tmp.DefWidth));
    }

    if (data->titlegroup)
    {
        tmp.MinHeight += _minheight(data->titlegroup);
        tmp.MaxHeight += w0_maxheight(data->titlegroup);
        tmp.DefHeight += w0_defheight(data->titlegroup);
    }

    END_MINMAX();
}

/**************************************************************************
 MUIM_AskMinMax : ask children about min/max sizes, then
 either call a hook, or the builtin method, to calculate our minmax
**************************************************************************/
/*
 * Ask Area and ShowMe children, finalize child limits, then select page,
 * custom Hook or built-in constraints. Hook must fill all six fields of its
 * otherwise uninitialized min/max message; its return is ignored. Virtual
 * groups save full minima but expose Area minima plus two. Return zero even
 * on helper failure; needs valid preferences and stable membership.
 */
IPTR Group__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    struct MUI_LayoutMsg lm;
    struct MUIP_AskMinMax childMsg;
    struct MUI_MinMax childMinMax;
    APTR cstate;
    Object *child;
    LONG super_minwidth, super_minheight;

    /*
     * let our superclass first fill in its size with frame, inner spc etc ...
     */
    DoSuperMethodA(cl, obj, (Msg) msg);
    super_minwidth = msg->MinMaxInfo->MinWidth;
    super_minheight = msg->MinMaxInfo->MinHeight;

    /*
     * Ask children
     */
    childMsg.MethodID = msg->MethodID;
    childMsg.MinMaxInfo = &childMinMax;
    get(data->family, MUIA_Family_List, &(lm.lm_Children));

    cstate = lm.lm_Children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (!(_flags(child) & MADF_SHOWME))
            /* BORDERGADGETs should handle this itself */
            continue;
        /*  Ask child  */
        DoMethodA(child, (Msg) & childMsg);
        /*  D(bug("*** group 0x%p, child 0x%p min=%ld,%ld\n", */
        /*      obj, child, childMinMax.MinWidth, childMinMax.MinHeight)); */
        __area_finish_minmax(child, childMsg.MinMaxInfo);
    }

    /*
     * Use children infos to calculate group size
     */
    if (data->flags & GROUP_PAGEMODE)
    {
        D(bug("minmax_pagemode(%p) minh initial = %d\n", obj,
                msg->MinMaxInfo->MinHeight));
        group_minmax_pagemode(cl, obj, lm.lm_Children, msg);
        D(bug("minmax_pagemode(%p) minh = %d\n", obj,
                msg->MinMaxInfo->MinHeight));
    }
    else if (data->layout_hook)
    {
        lm.lm_Type = MUILM_MINMAX;
        CallHookPkt(data->layout_hook, obj, &lm);

        if (lm.lm_MinMax.MaxHeight < lm.lm_MinMax.MinHeight)
            lm.lm_MinMax.MaxHeight = lm.lm_MinMax.MinHeight;
        if (lm.lm_MinMax.DefHeight < lm.lm_MinMax.MinHeight)
            lm.lm_MinMax.DefHeight = lm.lm_MinMax.MinHeight;
        if (lm.lm_MinMax.MaxWidth < lm.lm_MinMax.MinWidth)
            lm.lm_MinMax.MaxWidth = lm.lm_MinMax.MinWidth;
        if (lm.lm_MinMax.DefWidth < lm.lm_MinMax.MinWidth)
            lm.lm_MinMax.DefWidth = lm.lm_MinMax.MinWidth;

        //kprintf("### min %d x %d   def %d x %d  max %d x %d\n",
        //    msg->MinMaxInfo->MinWidth,
        //    msg->MinMaxInfo->MinHeight,
        //    msg->MinMaxInfo->DefWidth,
        //    msg->MinMaxInfo->DefHeight,
        //    msg->MinMaxInfo->MaxWidth,
        //    msg->MinMaxInfo->MaxHeight);

        msg->MinMaxInfo->MinWidth += lm.lm_MinMax.MinWidth;
        msg->MinMaxInfo->MinHeight += lm.lm_MinMax.MinHeight;
        msg->MinMaxInfo->MaxWidth += lm.lm_MinMax.MaxWidth;
        if (msg->MinMaxInfo->MaxWidth > MUI_MAXMAX)
            msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
        msg->MinMaxInfo->MaxHeight += lm.lm_MinMax.MaxHeight;
        if (msg->MinMaxInfo->MaxHeight > MUI_MAXMAX)
            msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
        msg->MinMaxInfo->DefWidth += lm.lm_MinMax.DefWidth;
        msg->MinMaxInfo->DefHeight += lm.lm_MinMax.DefHeight;

        //kprintf("#### min %d x %d   def %d x %d  max %d x %d\n",
        //    msg->MinMaxInfo->MinWidth,
        //    msg->MinMaxInfo->MinHeight,
        //    msg->MinMaxInfo->DefWidth,
        //    msg->MinMaxInfo->DefHeight,
        //    msg->MinMaxInfo->MaxWidth,
        //    msg->MinMaxInfo->MaxHeight);

    }
    else
    {
        if ((data->rows == 1) && (data->columns == 1))
        {
            data->num_visible_children =
                Group_GetNumVisibleChildren(data, lm.lm_Children);
            if (data->flags & GROUP_HORIZ)
                group_minmax_horiz(cl, obj, lm.lm_Children, msg);
            else
                group_minmax_vert(cl, obj, lm.lm_Children, msg);
        }
        else
        {
            group_minmax_2d(cl, obj, lm.lm_Children, msg);
        }
    }

    if (data->flags & GROUP_VIRTUAL)
    {
        data->saved_minwidth = msg->MinMaxInfo->MinWidth;
        data->saved_minheight = msg->MinMaxInfo->MinHeight;
        msg->MinMaxInfo->MinWidth = super_minwidth + 2;
        msg->MinMaxInfo->MinHeight = super_minheight + 2;

        //kprintf("## min %d x %d   def %d x %d  max %d x %d\n",
        //    msg->MinMaxInfo->MinWidth,
        //    msg->MinMaxInfo->MinHeight,
        //    msg->MinMaxInfo->DefWidth,
        //    msg->MinMaxInfo->DefHeight,
        //    msg->MinMaxInfo->MaxWidth,
        //    msg->MinMaxInfo->MaxHeight);

    }

    return 0;
}



// enforce minmax constraint, but also update total growable/shrinkable weights
// while we're at it
/*
 * Clamp one writable size to supplied bounds while adjusting remainder,
 * grow/shrink sizes and weight totals through required pointers. Same-size
 * zero-weight handling precedes clamping. Caller owns all storage and
 * consistent totals; WORD arithmetic can overflow. No allocation or result.
 */
static void Layout1D_minmax_constraint(WORD *sizep, WORD minsize,
    WORD maxsize, WORD *remainp, WORD *sizegrowp, WORD *sizeshrinkp,
    ULONG *weightgrowp, ULONG *weightshrinkp, UWORD weight, WORD samesize)
{
    WORD size = *sizep, remain = *remainp,
        sizegrow = *sizegrowp, sizeshrink = *sizeshrinkp;
    ULONG weightgrow = *weightgrowp, weightshrink = *weightshrinkp;

/*      D(bug("L1D_minmax_c size=%d min=%d max=%d w=%d ss=%d\n", */
/*          size, minsize, maxsize, */
/*          weight, samesize)); */

    if ((samesize > 0) && (weight == 0))
    {
        remain += size - samesize;
        size = samesize;
        sizeshrink -= size;
        sizegrow -= size;
    }

    if (size <= minsize)        // too little
    {
        remain += size - minsize;
        size = minsize;
        sizeshrink -= size;
        if (size == maxsize)
            sizegrow -= size;
    }
    else if (size >= maxsize)   // too big
    {
        remain += size - maxsize;
        size = maxsize;
        sizegrow -= size;
    }

    if (!((samesize > 0) && (weight == 0)))
    {
        if (size < maxsize)
            weightgrow += weight;
        if (size > minsize)
            weightshrink += weight;
    }

    *sizep = size;
    *remainp = remain;
    *sizegrowp = sizegrow;
    *sizeshrinkp = sizeshrink;
    *weightgrowp = weightgrow;
    *weightshrinkp = weightshrink;
}


// redistribute excess size to growable child, or reduce size of a shrinkable
// child
/*
 * Redistribute signed remaining space to one eligible weighted child,
 * updating size and all running totals through required pointers. Zero
 * weight returns without change. Caller must supply consistent nonzero
 * grow/shrink divisors when those branches execute. WORD arithmetic and
 * truncation are unchecked; no result.
 */
static void Layout1D_redistribution(WORD *sizep, WORD minsize,
    WORD maxsize, WORD *remainp, WORD *sizegrowp, WORD *sizeshrinkp,
    ULONG *weightgrowp, ULONG *weightshrinkp, UWORD weight)
{
    WORD size = *sizep, remain = *remainp,
        sizegrow = *sizegrowp, sizeshrink = *sizeshrinkp;
    ULONG weightgrow = *weightgrowp, weightshrink = *weightshrinkp;

    if (weight == 0)
        return;

    if ((remain > 0) && (size < maxsize))
    {
        LONG newsize;

        newsize = (sizegrow * weight + weightgrow / 2) / weightgrow;

/*        D(bug("newsize=%ld == size_growa=%ld * w=%ld / weight_grow=%d\n", */
/*                newsize, sizegrow, weight, weightgrow)); */

        /* take care of off-by-1 errors that may toggle remainder sign
         * by ensuring convergence to 0
         */
        if (remain - newsize + size < 0)
        {
/*              D(bug("adding remainder=%d => size = %d\n", */
/*                    remain, size + remain)); */
            size += remain;
            remain = 0;
        }
        else
        {
            remain -= newsize - size;
            size = newsize;
            sizegrow -= size;
            weightgrow -= weight;
        }
    }
    else if ((remain < 0) && (size > minsize))
    {
        LONG newsize;

        newsize = (sizeshrink * weight + weightshrink / 2) / weightshrink;

/*        D(bug("newsize=%ld == size_shrinkables=%ld * w=%ld " */
/*            "/ weight_shrinkables=%d\n", */
/*            newsize, sizeshrink, weight, weightshrink)); */

        if (remain - newsize + size > 0)
        {
/*              D(bug("adding remainder=%d => size = %d\n", */
/*                    remain, size + remain)); */
            size += remain;
            remain = 0;
        }
        else
        {
            remain -= newsize - size;
            size = newsize;
            sizeshrink -= size;
            weightshrink -= weight;
        }
    }

    *sizep = size;
    *remainp = remain;
    *sizegrowp = sizegrow;
    *sizeshrinkp = sizeshrink;
    *weightgrowp = weightgrow;
    *weightshrinkp = weightshrink;
}


/*
 * Run at most two clamp/redistribution passes over visible non-border
 * children using cached temporary sizes. Requires consistent weights, child
 * limits and stable list. Mutates child temporary dimensions; does not
 * report residual space or prove convergence for every input. No allocation
 * or result.
 */
static void Layout1D_minmax_constraints_and_redistrib(struct MinList
    *children, WORD total_size, WORD remainder, WORD samesize,
    BOOL group_horiz)
{
    APTR cstate;
    Object *child;
    int i;

    for (i = 0; i < 2; i++)
    {
        WORD size_growables = total_size;
        WORD size_shrinkables = total_size;
        ULONG weight_growables = 0;
        ULONG weight_shrinkables = 0;

/*          D(bug("start : rem=%ld, A=%ld, size_growables=%ld, " */
/*              "size_shrinkables=%ld\n", */
/*              remainder, total_size, size_growables, size_shrinkables)); */

        // minmax constraints
        cstate = children->mlh_Head;
        while ((child = NextObject(&cstate)))
        {
            /* WORD old_size; */

            if (IS_HIDDEN(child))
                continue;

            if (group_horiz)
            {
                /* old_size = _width(child); */

                Layout1D_minmax_constraint(&_width(child), _minwidth(child),
                    _maxwidth(child), &remainder, &size_growables,
                    &size_shrinkables, &weight_growables,
                    &weight_shrinkables, _hweight(child), samesize);

/*                D(bug("loop1 on %p : width=%d was %d, rem=%d, A=%d, " */
/*                    "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*                    child, _width(child), old_size, remainder, total_size, */
/*                        size_growables, size_shrinkables, _hweight(child), */
/*                        _minwidth(child), _maxwidth(child))); */
            }
            else                // ! group_horiz
            {
/*                old_size = _height(child); */

                Layout1D_minmax_constraint(&_height(child),
                    _minheight(child), _maxheight(child), &remainder,
                    &size_growables, &size_shrinkables, &weight_growables,
                    &weight_shrinkables, _vweight(child), samesize);

/*                D(bug("loop1 on %p : h=%ld was %d, rem=%d, A=%d, " */
/*                    "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*                    child, _height(child), old_size, remainder, total_size,*/
/*                    size_growables, size_shrinkables, _vweight(child), */
/*                    _minheight(child), _maxheight(child))); */
            }                   // if (group_horiz)
        }                       // while child, minmax constraints

        // mid-pass break
        if (remainder == 0)
            break;

/*          D(bug("mid : rem=%d, A=%d, size_grow=%d, size_shrink=%d, " */
/*                "wg=%ld, ws=%ld\n", remainder, total_size, size_growables, */
/*                size_shrinkables, weight_growables, weight_shrinkables)); */

        // distribute remaining space to possible candidates
        cstate = children->mlh_Head;
        while (((child = NextObject(&cstate)) != NULL) && (remainder != 0))
        {
/*            WORD old_size; */

            if (IS_HIDDEN(child))
                continue;

            if (group_horiz)
            {
/*                old_size = _width(child); */

                Layout1D_redistribution(&_width(child), _minwidth(child),
                    _maxwidth(child), &remainder, &size_growables,
                    &size_shrinkables, &weight_growables,
                    &weight_shrinkables, _hweight(child));

/*                  D(bug("loop2 on %p : h=%d was %d, rem=%d, A=%d, " */
/*                        "size_grow=%d, size_shrink=%d\n", child, */
/*                        _width(child), old_size, remainder, total_size, */
/*                        size_growables, size_shrinkables)); */
            }
            else                // ! group_horiz
            {
/*                old_size = _height(child); */

                Layout1D_redistribution(&_height(child), _minheight(child),
                    _maxheight(child), &remainder, &size_growables,
                    &size_shrinkables, &weight_growables,
                    &weight_shrinkables, _vweight(child));

/*                  D(bug("loop2 on %p : h=%d was %d, rem=%d, A=%d, " */
/*                        "size_grow=%d, size_shrink=%d\n", child, */
/*                        _height(child), old_size, remainder, total_size, */
/*                        size_growables, size_shrinkables)); */
            }                   // if (group_horiz)

        }                       // while child, redistribution

/*          if (remainder != 0) */
/*          { */
/*              D(bug("end : rem=%ld, A=%ld, size_grow=%ld, size_shrink=%ld\n", */
/*                    remainder, total_size, size_growables, size_shrinkables)); */
/*          } */
        // dont break here if remainder == 0, some minmax constraints
        // may not be respected
    }                           // end for

    // to easily spot layout bugs, nothing like a (division by zero) exception
/*      if (remainder != 0) */
/*      { */
/*          ASSERT(remainder != 0); */
/*          D(bug("gonna crash, remainder = %d\n", remainder)); */
/*          remainder /= 0; */
/*      } */

}

/*
 * Assign initial weighted size from remaining totals, or minimum when no
 * weight remains. Update caller size/space/weight pointers in place.
 * Requires valid consistent totals and representable WORD arithmetic. No
 * allocation, ownership change or result.
 */
static void Layout1D_weight_constraint(WORD *total_sizep,
    WORD *total_init_sizep,
    ULONG *total_weightp, WORD *sizep, UWORD weight, WORD minsize)
{
    if (*total_weightp > 0)
        *sizep = (*total_sizep * weight + *total_weightp / 2)
            / *total_weightp;
    else
        *sizep = minsize;

    *total_weightp -= weight;
    *total_sizep -= *sizep;
    *total_init_sizep += *sizep;
}


/*
 * Compute vertical content dimensions, weighted child heights and centered
 * cross-axis widths; clamp/redistribute then call MUI_Layout on visible
 * non-border children. Requires prior min/max and setup. Virtual mode
 * expands content bounds. Helper failure has no reported status; MUI_Layout
 * currently returns TRUE regardless of child Layout result.
 */
static void group_layout_vert(struct IClass *cl, Object *obj,
    struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    ULONG total_weight;
    WORD remainder;    /* must converge to 0 to successfully end layout */
    WORD total_size;
    WORD total_size_backup;
    WORD total_init_size;       /* total size of the ideally sized children */
    WORD width;
    WORD left = 0;
    WORD top = 0;
    WORD layout_width;
    WORD layout_height;

    //kprintf("group_layout_vert: virtoff = %d,%d\n",
    //    data->virt_offx, data->virt_offy);

    if (data->flags & GROUP_VIRTUAL)
    {
        data->virt_mwidth =
            CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj),
            _maxwidth(obj) - _subwidth(obj));
        data->virt_mheight =
            CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj),
            _maxheight(obj) - _subheight(obj));

        layout_width = data->virt_mwidth;
        layout_height = data->virt_mheight;
    }
    else
    {
        layout_width = _mwidth(obj);
        layout_height = _mheight(obj);
    }

    total_weight = data->vert_weight_sum;
    total_init_size = 0;
    total_size =
        layout_height - (data->num_visible_children -
        1) * data->vert_spacing;
    total_size_backup = total_size;

/*      D(bug("\nvert layout for %p, A=%d W=%ld\n", */
/*          obj, total_size, total_weight)); */

    // weight constraints
    // calculate ideal size for each object, and total ideal size
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (IS_HIDDEN(child))
            continue;

        Layout1D_weight_constraint(&total_size, &total_init_size,
            &total_weight, &_height(child), _vweight(child),
            _minheight(child));
/*          D(bug("child %p : ideal=%d w=%ld\n", */
/*                child, _height(child), _vweight(child))); */
    }                           // while child, weight constraints

    total_size = total_size_backup;
    remainder = total_size - total_init_size;

    if (data->flags & GROUP_VIRTUAL)
    {
        /* This is also true for non virtual groups, but if this would be the
         ** case then there is a bug in the layout function
         */
        if (total_size < 0)
            total_size = 0;
    }

    Layout1D_minmax_constraints_and_redistrib(children,
        total_size,
        remainder,
        (data->flags & GROUP_SAME_HEIGHT) ? data->samesize_maxmin_vert : 0,
        FALSE);

    // do the layout
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (IS_HIDDEN(child))
            continue;

        width = MIN(_maxwidth(child), layout_width);
        width = MAX(width, _minwidth(child));
        left = (layout_width - width) / 2;

/*          D(bug("child %p -> layout %d x %d\n", */
/*              child, width, _height(child))); */
        if (!MUI_Layout(child, left, top, width, _height(child), 0))
            return;
        top += data->vert_spacing + _height(child);
    }

}


/*
 * Compute horizontal weighted child widths and centered heights using prior
 * min/max data, then lay out visible non-border children. Virtual mode
 * clamps content dimensions to saved limits. Requires live geometry and
 * consistent cached weights. No result; child layout errors are not
 * propagated by MUI_Layout.
 */
static void group_layout_horiz(struct IClass *cl, Object *obj,
    struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    ULONG total_weight;
    WORD remainder;    /* must converge to 0 to successfully end layout */
    WORD total_size;
    WORD total_size_backup;
    WORD total_init_size;       /* total size of the ideally sized children */
    WORD height;
    WORD top = 0;
    WORD left = 0;
    WORD layout_width;
    WORD layout_height;

    //kprintf("group_layout_horiz: virtoff = %d,%d\n",
    //    data->virt_offx, data->virt_offy);

    if (data->flags & GROUP_VIRTUAL)
    {
        data->virt_mwidth =
            CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj),
            _maxwidth(obj) - _subwidth(obj));
        data->virt_mheight =
            CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj),
            _maxheight(obj) - _subheight(obj));

        layout_width = data->virt_mwidth;
        layout_height = data->virt_mheight;

        //kprintf("group_layout_horiz: layoutsize %d x %d "
        //    " virtsize %d x %d  msize %d x %d\n",
        //    layout_width, layout_height, data->virt_mwidth,
        //    data->virt_mheight,
        //    _mwidth(obj), _mheight(obj));
    }
    else
    {
        layout_width = _mwidth(obj);
        layout_height = _mheight(obj);
    }

    total_weight = data->horiz_weight_sum;
    total_init_size = 0;
    total_size =
        layout_width - (data->num_visible_children -
        1) * data->horiz_spacing;
    total_size_backup = total_size;

/*      D(bug("\nhoriz layout for %p, A=%d W=%ld\n", */
/*          obj, total_size, total_weight)); */

    // weight constraints
    // calculate ideal size for each object, and total ideal size
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (IS_HIDDEN(child))
            continue;

        Layout1D_weight_constraint(&total_size, &total_init_size,
            &total_weight, &_width(child), _hweight(child),
            _minwidth(child));
/*          D(bug("child %p : ideal=%d w=%ld\n", */
/*                child, _width(child), _hweight(child))); */
    }                           // while child, weight constraints

    total_size = total_size_backup;
    if (data->horiz_weight_sum > 0)
        remainder = total_size - total_init_size;
    else
        remainder = 0;

    if (data->flags & GROUP_VIRTUAL)
    {
        /* This is also true for non virtual groups, but if this would be the
         ** case then there is a bug in the layout function
         */
        if (total_size < 0)
            total_size = 0;
    }

    Layout1D_minmax_constraints_and_redistrib(children,
        total_size,
        remainder,
        (data->flags & GROUP_SAME_WIDTH) ? data->samesize_maxmin_horiz : 0,
        TRUE);

    // do the layout
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (IS_HIDDEN(child))
            continue;

        height = MIN(_maxheight(child), layout_height);
        height = MAX(height, _minheight(child));
        top = (layout_height - height) / 2;

/*          D(bug("child %p -> layout %d x %d\n", */
/*              child, _width(child), height)); */
        if (!MUI_Layout(child, left, top, _width(child), height, 0))
            return;
        left += data->horiz_spacing + _width(child);
    }

}


/*
 * Initialize row and column dimensions proportionally to cached weights and
 * available content space. Mutate allocated grid records. Requires nonzero
 * established dimensions, arrays and consistent weight totals; WORD
 * arithmetic is unchecked. No allocation or status.
 */
static void Layout2D_weight_constraint(struct MUI_GroupData *data,
    struct layout2d_elem *row_infos,
    struct layout2d_elem *col_infos,
    WORD total_size_height, WORD total_size_width,
    WORD *total_init_height, WORD *total_init_width)
{
    int i;
    ULONG total_weight_vert = data->vert_weight_sum;
    ULONG total_weight_horiz = data->horiz_weight_sum;

    *total_init_height = 0;
    *total_init_width = 0;

    /* calc row heights */
    for (i = 0; i < data->rows; i++)
    {
        if (total_weight_vert > 0)
            row_infos[i].dim =
                (total_size_height * row_infos[i].weight +
                total_weight_vert / 2) / total_weight_vert;
        else
            row_infos[i].dim = row_infos[i].min;

/*          D(bug("l2 row %d : ideal = %d with w=%d, A=%d, W=%d\n", */
/*              i, row_infos[i].dim, */
/*              row_infos[i].weight, total_size_height, total_weight_vert)); */

        total_weight_vert -= row_infos[i].weight;
        total_size_height -= row_infos[i].dim;
        *total_init_height += row_infos[i].dim;
    }

    /* calc columns widths */
    for (i = 0; i < data->columns; i++)
    {
        if (total_weight_horiz)
            col_infos[i].dim =
                (total_size_width * col_infos[i].weight +
                total_weight_horiz / 2) / total_weight_horiz;
        else
            col_infos[i].dim = col_infos[i].min;

/*          D(bug("l2 col %d : ideal = %d with w=%d, A=%d, W=%d\n", */
/*              i, col_infos[i].dim, */
/*              col_infos[i].weight, total_size_width, total_weight_horiz)); */

        total_weight_horiz -= col_infos[i].weight;
        total_size_width -= col_infos[i].dim;
        *total_init_width += col_infos[i].dim;
    }
}



/*
 * Run two passes over one allocated axis array, clamping dimensions and
 * redistributing remainder with shared 1D helpers. Requires valid count,
 * constraints and consistent totals. Mutates records and does not report
 * unresolved remainder or guarantee arithmetic safety.
 */
static void Layout2D_minmax_constraints_and_redistrib(struct layout2d_elem
    *infos, WORD nitems, WORD total_size, WORD samesize, WORD remainder)
{
    int j;

/*      D(bug("L2D mc&r n=%d A=%d ss=%d rem=%d\n", */
/*          nitems, total_size, samesize, remainder)); */

    for (j = 0; j < 2; j++)
    {
        WORD size_growables = total_size;
        WORD size_shrinkables = total_size;
        ULONG weight_growables = 0;
        ULONG weight_shrinkables = 0;
/*        WORD old_size; */
        int i;

        // minmax constraints
        for (i = 0; i < nitems; i++)
        {
/*                old_size = infos[i].dim; */

/*              D(bug("bef loop1 on %d : size=%d, rem=%d, A=%d, " */
/*                        "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*                        i, infos[i].dim, remainder, total_size, */
/*                        size_growables, size_shrinkables, infos[i].weight, */
/*                        infos[i].min, infos[i].max)); */

            Layout1D_minmax_constraint(&infos[i].dim, infos[i].min,
                infos[i].max, &remainder, &size_growables,
                &size_shrinkables, &weight_growables, &weight_shrinkables,
                infos[i].weight, samesize);

/*              D(bug("loop1 on %d : size=%d was %d, rem=%d, A=%d, " */
/*                        "sizegrow=%d, sizeshrink=%d w=%d min=%d max=%d\n", */
/*                        i, infos[i].dim, old_size, remainder, total_size, */
/*                        size_growables, size_shrinkables, infos[i].weight, */
/*                        infos[i].min, infos[i].max)); */
        }

        if (remainder == 0)
            break;

        for (i = 0; i < nitems; i++)
        {
/*            old_size = infos[i].dim; */

/*              D(bug("bef loop2 on %d : size=%d, rem=%d, A=%d, " */
/*                        "size_grow=%d, size_shrink=%d\n", i, */
/*                        infos[i].dim, remainder, total_size, */
/*                        size_growables, size_shrinkables)); */

            Layout1D_redistribution(&infos[i].dim, infos[i].min,
                infos[i].max, &remainder, &size_growables,
                &size_shrinkables, &weight_growables, &weight_shrinkables,
                infos[i].weight);

/*              D(bug("loop2 on %d : size=%d was %d, rem=%d, A=%d, " */
/*                        "size_grow=%d, size_shrink=%d\n", i, */
/*                        infos[i].dim, old_size, remainder, total_size, */
/*                        size_growables, size_shrinkables)); */
        }
    }
}

/*
 * Place visible non-border children row-major using prepared row/column
 * dimensions and supplied starting coordinates. Clamp each child to its
 * min/max and center within its cell; advance by cell size plus spacing.
 * Requires valid arrays, nonzero columns and stable membership. Return
 * early if MUI_Layout reports FALSE, without an error result; that wrapper
 * currently always returns TRUE.
 */
static void
layout_2d_distribute_space(struct MUI_GroupData *data,
    struct layout2d_elem *row_infos,
    struct layout2d_elem *col_infos,
    struct MinList *children, LONG left_start, LONG top_start)
{
    APTR cstate;
    Object *child;
    LONG left;
    LONG top;
    LONG col_width;
    LONG row_height;
    int i, j;

    /*
     * pass 2 : distribute space
     */
    cstate = children->mlh_Head;
    top = top_start;
    /* for each row */
    for (i = 0; i < data->rows; i++)
    {
        /* left start for child layout in this row */
        left = left_start;

        /* max height for children in this row */
        row_height = row_infos[i].dim;
        j = 0;
        /* for each column */
        while ((child = NextObject(&cstate)))
        {
            LONG cleft;
            LONG ctop;
            LONG cwidth;
            LONG cheight;

            if (IS_HIDDEN(child))
                continue;
            /* max width for children in this column */
            col_width = col_infos[j].dim;

            /* center child if col width is bigger than child maxwidth */
            cwidth = MIN(_maxwidth(child), col_width);
            cwidth = MAX(cwidth, _minwidth(child));
            cleft = left + (col_width - cwidth) / 2;

            /* center child if row height is bigger than child maxheight */
            cheight = MIN(_maxheight(child), row_height);
            cheight = MAX(cheight, _minheight(child));
            ctop = top + (row_height - cheight) / 2;

/*          g_print("layout %d %d  %d %d\n", cleft, ctop, cwidth, cheight); */
/*          D(bug("2DL/child %p -> layout %d x %d\n", */
/*              child, cwidth, cheight)); */
            if (!MUI_Layout(child, cleft, ctop, cwidth, cheight, 0))
                return;

            left += data->horiz_spacing + col_width;

            ++j;
            if ((j % data->columns) == 0)
                break;
        }

        top += data->vert_spacing + row_height;
    }

}


/*
 * all children in the same row have the same maximum height
 * all children in the same column have the same maximum height
 * if a child maximum size is smaller than the biggest minimum size,
 * the chid will be centered in the remaining space.
 *
 * for each row, determine its height allocation
 * weight ? the vertical weight of a row, if no fixed-height child
 *  in the row, is the sum of all vertical weights of children
 * row height is shared; individual child limits still apply
 *
 * for each column, determine its width allocation
 * column width is shared; individual child limits still apply
 */
/* Write a proper hook function */
/*
 * Validate nonzero compatible dimensions and both arrays; otherwise return
 * without layout or error status. Distribute space then place visible non-
 * border children row-major, centered within cell limits. Requires prior
 * min/max and render geometry. Calls MUI_Layout with no propagated child
 * result.
 */
static void
group_layout_2d(struct IClass *cl, Object *obj, struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    WORD left_start = 0;
    WORD top_start = 0;
    WORD total_size_height =
        _mheight(obj) - (data->rows - 1) * data->vert_spacing;
    WORD total_size_width =
        _mwidth(obj) - (data->columns - 1) * data->horiz_spacing;
    WORD total_init_height;
    WORD total_init_width;
    WORD remainder_height;
    WORD remainder_width;
    WORD layout_width;
    WORD layout_height;

    if (data->rows == 0 || data->columns == 0)
        return;
    if (data->num_children % data->rows
        || data->num_children % data->columns)
        return;
    if (data->row_infos == NULL || data->col_infos == NULL)
        return;

    //kprintf("group_layout_horiz: virtoff = %d,%d\n",
    //    data->virt_offx, data->virt_offy);

    if (data->flags & GROUP_VIRTUAL)
    {
        data->virt_mwidth =
            CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj),
            _maxwidth(obj) - _subwidth(obj));
        data->virt_mheight =
            CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj),
            _maxheight(obj) - _subheight(obj));

        layout_width = data->virt_mwidth;
        layout_height = data->virt_mheight;
    }
    else
    {
        layout_width = _mwidth(obj);
        layout_height = _mheight(obj);
    }

    total_size_height =
        layout_height - (data->rows - 1) * data->vert_spacing;
    total_size_width =
        layout_width - (data->columns - 1) * data->horiz_spacing;

    // fix left/top ?

    // weight constraints
    Layout2D_weight_constraint(data, data->row_infos, data->col_infos,
        total_size_height, total_size_width,
        &total_init_height, &total_init_width);

    remainder_height = total_size_height - total_init_height;
    remainder_width = total_size_width - total_init_width;

    Layout2D_minmax_constraints_and_redistrib(data->row_infos,
        data->rows, total_size_height,
/*      (data->flags & GROUP_SAME_HEIGHT) ? data->samesize_maxmin_vert : 0, */
        0, remainder_height);

    Layout2D_minmax_constraints_and_redistrib(data->col_infos,
        data->columns, total_size_width,
/*      (data->flags & GROUP_SAME_WIDTH) ? data->samesize_maxmin_horiz : 0, */
        0, remainder_width);

    layout_2d_distribute_space(data, data->row_infos, data->col_infos,
        children, left_start, top_start);
}


/* Write a proper hook function */
/*
 * Lay out all pages, including hidden/inactive children, centered in the
 * available width with title minimum height reserved above content. Width
 * uses available space and maximum, not a minimum clamp. Requires valid
 * child limits/title pointer; ignores child layout results and returns no
 * status.
 */
static void group_layout_pagemode(struct IClass *cl, Object *obj,
    struct MinList *children)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    WORD layout_width;
    WORD layout_height;
    int w, h, yoffset = 0;

    if (data->flags & GROUP_VIRTUAL)
    {
        data->virt_mwidth =
            CLAMP(_mwidth(obj), data->saved_minwidth - _subwidth(obj),
            _maxwidth(obj) - _subwidth(obj));
        data->virt_mheight =
            CLAMP(_mheight(obj), data->saved_minheight - _subheight(obj),
            _maxheight(obj) - _subheight(obj));

        layout_width = data->virt_mwidth;
        layout_height = data->virt_mheight;
    }
    else
    {
        layout_width = _mwidth(obj);
        layout_height = _mheight(obj);
    }

    if (data->titlegroup)
    {
        yoffset = _minheight(data->titlegroup);
        layout_height -= yoffset;
    }

    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        w = MIN(layout_width, _maxwidth(child));
        h = MIN(layout_height, _maxheight(child));

        if (child == data->titlegroup)
        {
            MUI_Layout(child, (layout_width - w) / 2, 0, w, yoffset, 0);
        }
        else
        {
            D(bug("PM/child %p -> layout %d x %d\n", child, w, h));
            MUI_Layout(child, (layout_width - w) / 2,
                yoffset + (layout_height - h) / 2, w, h, 0);
        }
    }
}


/**************************************************************************
 MUIM_Layout
 Either use a given layout hook, or the builtin method.
**************************************************************************/
/*
 * Select page, custom Hook or built-in layout. Hook receives a zeroed
 * temporary message with inner dimensions and borrowed list; return
 * ignored. Copy virtual dimensions back, clamp WORD offsets and set them
 * with child forwarding disabled, potentially reentering Layout. Return zero
 * regardless of outcome. Requires established child min/max and valid
 * lifecycle.
 */
IPTR Group__MUIM_Layout(struct IClass *cl, Object *obj,
    struct MUIP_Layout *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    struct MUI_LayoutMsg lm = { 0 };

    get(data->family, MUIA_Family_List, &(lm.lm_Children));
    if (data->flags & GROUP_PAGEMODE)
    {
        group_layout_pagemode(cl, obj, lm.lm_Children);
    }
    else if (data->layout_hook)
    {
        lm.lm_Type = MUILM_LAYOUT;
        lm.lm_Layout.Width = _mwidth(obj);
        lm.lm_Layout.Height = _mheight(obj);

        CallHookPkt(data->layout_hook, obj, &lm);

        if (data->flags & GROUP_VIRTUAL)
        {
            data->virt_mwidth = lm.lm_Layout.Width;
            data->virt_mheight = lm.lm_Layout.Height;
        }
    }
    else
    {
        if ((data->rows == 1) && (data->columns == 1))
        {
            if (data->flags & GROUP_HORIZ)
                group_layout_horiz(cl, obj, lm.lm_Children);
            else
                group_layout_vert(cl, obj, lm.lm_Children);
        }
        else
            group_layout_2d(cl, obj, lm.lm_Children);
    }

    if (data->flags & GROUP_VIRTUAL)
    {
        WORD new_virt_offx, new_virt_offy;

        new_virt_offx = data->virt_offx;
        new_virt_offy = data->virt_offy;

        if (new_virt_offx + _mwidth(obj) > data->virt_mwidth)
        {
            new_virt_offx = data->virt_mwidth - _mwidth(obj);
        }
        if (new_virt_offx < 0)
            new_virt_offx = 0;

        if (new_virt_offy + _mheight(obj) > data->virt_mheight)
        {
            new_virt_offy = data->virt_mheight - _mheight(obj);
        }
        if (new_virt_offy < 0)
            new_virt_offy = 0;

        if (new_virt_offx != data->virt_offx)
        {
            nfset(obj, MUIA_Virtgroup_Left, new_virt_offx);
        }

        if (new_virt_offy != data->virt_offy)
        {
            nfset(obj, MUIA_Virtgroup_Top, new_virt_offy);
        }

    }

    return 0;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
/*
 * If message is non-NULL, show Area first and ignore its result. Page mode
 * shows title and selected page without ShowMe filtering; ordinary mode
 * checks ShowMe and virtual visibility. Ignore child results, return TRUE.
 * NULL message is an internal child-only path; requires valid drawable
 * geometry.
 */
IPTR Group__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    /* If msg is NULL, we won't want that the super method actually gets
     * this call */
    if (msg)
        DoSuperMethodA(cl, obj, (Msg) msg);

    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;

    if (data->flags & GROUP_PAGEMODE)
    {
        int page = 0;
        while ((child = NextObject(&cstate)))
        {
            if (child == data->titlegroup)
            {
                DoShowMethod(child);
                continue;       /* Title group is not counted as page */
            }

            if (page == data->active_page)
            {
                DoShowMethod(child);
                break;
            }
            page++;
        }
    }
    else
    {
        while ((child = NextObject(&cstate)))
        {
            if (!(data->flags & GROUP_VIRTUAL) ||
                IsObjectVisible(child, MUIMasterBase))
            {
                if (_flags(child) & MADF_SHOWME)
                    DoShowMethod(child);
            }
        }
    }
    return TRUE;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
/*
 * Hide title/selected page in page mode or drawable children otherwise.
 * Call Area only for non-NULL message, returning its result, otherwise one.
 * Requires stable list and valid shown state. Does not cleanup or release
 * children; NULL message is the internal child-only path.
 */
IPTR Group__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;

    if (data->flags & GROUP_PAGEMODE)
    {
        int page = 0;
        while ((child = NextObject(&cstate)))
        {
            if (child == data->titlegroup)
            {
                DoHideMethod(child);
                continue;       /* Title group is not counted as page */
            }

            if (page == data->active_page)
            {
                DoHideMethod(child);
                break;
            }
            page++;
        }
    }
    else
    {
        while ((child = NextObject(&cstate)))
        {
            if (_flags(child) & MADF_CANDRAW)
                DoHideMethod(child);
        }
    }

    /* If msg is NULL, we won't want that the super method actually gets
     * this call */
    if (msg)
        return DoSuperMethodA(cl, obj, (Msg) msg);
    return 1;
}

/*
 * MUIM_FindUData : tests if the MUIA_UserData of the object
 * contains the given <udata> and returns the object pointer in this case.
 */
/*
 * Return this borrowed Group if UserData matches; otherwise delegate to
 * internal Family, which can itself match. Requires live objects and stable
 * traversal. Return zero when Family finds no match; do not dispose
 * returned objects.
 */
IPTR Group__MUIM_FindUData(struct IClass *cl, Object *obj,
    struct MUIP_FindUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        return (IPTR) obj;

    return DoMethodA(data->family, (Msg) msg);
}


/*
 * MUIM_GetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and gets <attr> to <storage> for itself
 * in this case.
 */
/*
 * Match Group first and invoke its getter into caller storage without
 * checking getter success; otherwise delegate to Family. Return match
 * status, not getter success. Requires storage suitable for requested
 * attribute and stable traversal.
 */
IPTR Group__MUIM_GetUData(struct IClass *cl, Object *obj,
    struct MUIP_GetUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        get(obj, msg->attr, msg->storage);
        return TRUE;
    }

    return DoMethodA(data->family, (Msg) msg);
}


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
/*
 * Set matching Group then traverse Family for all matches. Group setter may
 * already broadcast to children before explicit Family traversal, so
 * assignments can repeat. Requires writable valid setter arguments and
 * stable membership. Return TRUE; delegated results are ignored.
 */
IPTR Group__MUIM_SetUData(struct IClass *cl, Object *obj,
    struct MUIP_SetUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        set(obj, msg->attr, msg->val);

    DoMethodA(data->family, (Msg) msg);
    return TRUE;
}


/*
 * MUIM_SetUDataOnce : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 * Stop after the first udata found.
 */
/*
 * Set Group and return TRUE when its UserData matches; otherwise return
 * Family search result. A Group match can still propagate its setter to
 * children. Requires live compatible objects and valid setter arguments; no
 * ownership change.
 */
IPTR Group__MUIM_SetUDataOnce(struct IClass *cl, Object *obj,
    struct MUIP_SetUData *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        set(obj, msg->attr, msg->val);
        return TRUE;
    }
    return DoMethodA(data->family, (Msg) msg);
}

/**************************************************************************
 MUIM_DragQueryExtented
**************************************************************************/
/*
 * Ask drawable children in list order and return the first accepting
 * object/result; otherwise ask Area. Requires valid borrowed drag message
 * and live geometry. Does not transfer ownership or validate child callback
 * lifetime.
 */
IPTR Group__MUIM_DragQueryExtended(struct IClass *cl, Object *obj,
    struct MUIP_DragQueryExtended *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    Object *found_obj;
    struct MinList *ChildList = NULL;

    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (!(_flags(child) & MADF_CANDRAW))
            continue;

        if ((found_obj = (Object *) DoMethodA(child, (Msg) msg)))
            return (IPTR) found_obj;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
/*
 * Handle virtual-group mouse/tick autoscroll only; never delegate to Area
 * here. SELECTDOWN inside starts ticks; another button event removes them.
 * Ticks outside adjust/clamp offsets by four pixels and set them with
 * Forward FALSE. Requires valid IntuiMessage/window/setup. Return zero,
 * including unhandled events.
 */
IPTR Group__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    /* check this, otherwise a superclass who has IDCMP_MOUSEBUTTONS
       eventhandler might call DoSuperMethod, and this function gets
       called even when he have not added any eventhandler */

    if ((data->flags & GROUP_VIRTUAL) && msg->imsg)
    {
        switch (msg->imsg->Class)
        {
        case IDCMP_MOUSEBUTTONS:
            /* For virtual groups */
            if (msg->imsg->Code == SELECTDOWN)
            {
                if (_between(_mleft(obj), msg->imsg->MouseX, _mright(obj))
                    && _between(_mtop(obj), msg->imsg->MouseY,
                        _mbottom(obj)))
                {
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                        (IPTR) & data->ehn);
                    data->ehn.ehn_Events |= IDCMP_INTUITICKS;
                    DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                        (IPTR) & data->ehn);
                }
            }
            else
            {
                if (data->ehn.ehn_Events & IDCMP_INTUITICKS)
                {
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                        (IPTR) & data->ehn);
                    data->ehn.ehn_Events &= ~IDCMP_INTUITICKS;
                    DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                        (IPTR) & data->ehn);
                }
            }
            break;

        case IDCMP_INTUITICKS:
            if (!(data->ehn.ehn_Events & IDCMP_INTUITICKS))
                break;

            if (!(_between(_mleft(obj), msg->imsg->MouseX, _mright(obj))
                    && _between(_mtop(obj), msg->imsg->MouseY,
                        _mbottom(obj))))
            {
                LONG new_virt_offx = data->virt_offx;
                LONG new_virt_offy = data->virt_offy;

                if (msg->imsg->MouseX < _mleft(obj))
                {
                    /* scroll left */
                    if (new_virt_offx >= 4)
                        new_virt_offx -= 4;
                    else
                        new_virt_offx = 0;
                }
                else if (msg->imsg->MouseX > _mright(obj))
                {
                    /* scroll right */
                    new_virt_offx += 4;
                    if (new_virt_offx > data->virt_mwidth - _mwidth(obj))
                        new_virt_offx = data->virt_mwidth - _mwidth(obj);
                    if (new_virt_offx < 0)
                        new_virt_offx = 0;
                }

                if (msg->imsg->MouseY < _mtop(obj))
                {
                    /* scroll top */
                    if (new_virt_offy >= 4)
                        new_virt_offy -= 4;
                    else
                        new_virt_offy = 0;
                }
                else if (msg->imsg->MouseY > _mbottom(obj))
                {
                    /* scroll bottom */
                    new_virt_offy += 4;
                    if (new_virt_offy > data->virt_mheight - _mheight(obj))
                        new_virt_offy = data->virt_mheight - _mheight(obj);
                    if (new_virt_offy < 0)
                        new_virt_offy = 0;
                }

                if (new_virt_offx != data->virt_offx
                    || new_virt_offy != data->virt_offy)
                {
                    SetAttrs(obj,
                        MUIA_Virtgroup_Left, new_virt_offx,
                        MUIA_Virtgroup_Top, new_virt_offy,
                        MUIA_Group_Forward, FALSE, TAG_DONE);
                }
            }
            break;
        }
    }

    return 0;
}

/**************************************************************************
 MUIM_DrawBackground
**************************************************************************/
/*
 * For virtual groups copy the message and add virtual offsets to background
 * offsets before Area dispatch; otherwise forward original. Requires live
 * drawing state and valid message. Return superclass result; temporary copy
 * must not escape.
 */
IPTR Group__MUIM_DrawBackground(struct IClass *cl, Object *obj,
    struct MUIP_DrawBackground *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    if (data->flags & GROUP_VIRTUAL)
    {
        struct MUIP_DrawBackground msg2 = *msg;

        msg2.xoffset += data->virt_offx;
        msg2.yoffset += data->virt_offy;

        return DoSuperMethodA(cl, obj, (Msg) & msg2);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_FindAreaObject
 Find the given object or return NULL
**************************************************************************/
/*
 * Find this Group, then immediate child identities, then recursive child
 * matches. Return a borrowed object or zero. Requires stable live tree and
 * valid sought object identity; no allocation or ownership change.
 */
IPTR Group__MUIM_FindAreaObject(struct IClass *cl, Object *obj,
    struct MUIP_FindAreaObject *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    // it's me ?
    if (msg->obj == obj)
        return (IPTR) obj;

    // it's one of my children ?
    get(data->family, MUIA_Family_List, &(ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (msg->obj == child)
            return (IPTR) child;
    }

    // let the children find it
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        Object *res = (Object *) DoMethodA(child, (Msg) msg);
        if (res != NULL)
            return (IPTR) res;
    }

    return (IPTR) NULL;
}

/**************************************************************************
MUIM_Export : to export an object's "contents" to a dataspace object.
**************************************************************************/
/*
 * Forward export to every child, ignoring results, then return zero.
 * Requires stable membership and valid borrowed Dataspace. Does not export
 * this Group through Area and does not transfer ownership.
 */
static IPTR Group__MUIM_Export(struct IClass *cl, Object *obj,
    struct MUIP_Export *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    get(data->family, MUIA_Family_List, &(ChildList));
    if (!ChildList)
        return 0;

    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethodA(child, (Msg) msg);
    }

    return 0;
}


/**************************************************************************
MUIM_Import : to import an object's "contents" from a dataspace object.
**************************************************************************/
/*
 * Forward import to every child, ignoring results, then return zero.
 * Requires stable membership and valid borrowed Dataspace. Does not import
 * this Group through Area; callbacks can change child state.
 */
static IPTR Group__MUIM_Import(struct IClass *cl, Object *obj,
    struct MUIP_Import *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    get(data->family, MUIA_Family_List, &(ChildList));
    if (!ChildList)
        return 0;

    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethodA(child, (Msg) msg);
    }

    return 0;
}

/**************************************************************************
 MUIM_Notify - disabled now because previous Zune versions had a OM_GET
 check in MUIM_Notify which is no longer the case
**************************************************************************/
#if 0
/*
 * Inactive #if 0 alternative; not compiled. Probes local Notify then
 * children and returns registration result. Its suppression flag is not
 * nested-safe. Retained for source history, not a runtime contract.
 */
STATIC IPTR Group_Notify(struct IClass *cl, Object *obj,
    struct MUIP_Notify *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList;

    /* Try at first if understand the message our self
     ** We disable the forwarding of the OM_GET message
     ** as the MUIM_Notify otherwise would "think" that
     ** the group class actually understands the attribute
     ** although a child does this only
     */
    data->dont_forward_get = 1;
    if (DoSuperMethodA(cl, obj, (Msg) msg))
    {
        data->dont_forward_get = 0;
        return 1;
    }

    /* We ourselves didn't understand the notify tag so we try the
     * children now */
    data->dont_forward_get = 0;

    get(data->family, MUIA_Family_List, (IPTR *) & (ChildList));
    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (DoMethodA(child, (Msg) msg))
            return 1;
    }
    return 0;
}
#endif

#if 1
/* Notes about Group_Notify() and echo notification problem:
It was discovered that MUI seems to have some special handling for group class
which will drop notifications on the children which are found to not
understand the attribute.

This is done by checking if an OM_GET on the child returns TRUE.
There's a little problem here because it is not known how big the storage
needed for the attribute in question will be. Almost no class uses anything
bigger than one IPTR. For "big" attributes those return a pointer to the data,
not the data itself. Unfortuntely there are some exceptions like colorwheel
class which does not return a pointer, but the data itself. So it's not
enough to use one single IPTR variable (4 Bytes on 32bit machines, 8 bytes
on 64 bit machines) to store the result of the test-OM_Get.

There is no general way to query the size needed so if one wants to change
Zune to work like MUI one needs to choose a size which one hopes will be
big enough to hold all possible attributes of all classes, old, present
and future ones.
*/
/*
 * Probe trigger getter on Group with child-get forwarding disabled, then
 * register locally through Area if recognized. Otherwise probe each child
 * and register on supported getters, ignoring registration results and
 * returning TRUE even if none match. Requires stable children and getters
 * writing at most 30 IPTRs. Borrowed message; flag restoration is not
 * nested-safe.
 */
STATIC IPTR Group_Notify(struct IClass *cl, Object *obj,
    struct MUIP_Notify *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);
    APTR cstate;
    Object *child;
    struct MinList *ChildList = NULL;
    IPTR attr[30];

    data->dont_forward_get = 1;

    if (GetAttr(msg->TrigAttr, obj, attr))
    {
        data->dont_forward_get = 0;
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }
    data->dont_forward_get = 0;

    get(data->family, MUIA_Family_List, &(ChildList));
    if (!ChildList)
        return TRUE;

    cstate = ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {

        if (GetAttr(msg->TrigAttr, child, attr))
        {
            DoMethodA(child, (Msg) msg);
            /* No return here! */
        }
    }
    return TRUE;
}
#endif

/*
 * Route explicit Group/Area overrides and utility exceptions; unknown
 * messages broadcast to children then reach Area. Return selected
 * handler/superclass result, not aggregate child success. Requires live
 * class/object and ABI-correct writable message where handlers mutate it.
 * Explicit routes can bypass no-forward suppression.
 */
BOOPSI_DISPATCHER(IPTR, Group_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Group__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Group__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Group__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Group__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_ADDMEMBER:         /* Fall through */
    case MUIM_Group_AddTail:
        return Group__MUIM_AddTail(cl, obj, (APTR) msg);
    case MUIM_Group_AddHead:
        return Group__MUIM_AddHead(cl, obj, (APTR) msg);
    case MUIM_Group_Insert:
        return Group__MUIM_Insert(cl, obj, (APTR) msg);
    case OM_REMMEMBER:         /* Fall through */
    case MUIM_Group_Remove:
        return Group__MUIM_Remove(cl, obj, (APTR) msg);
    case MUIM_Family_GetChild:
        return Group__MUIM_Family_GetChild(cl, obj, (APTR) msg);
    case MUIM_AskMinMax:
        return Group__MUIM_AskMinMax(cl, obj, (APTR) msg);
    case MUIM_Group_ExitChange:
        return Group__MUIM_ExitChange(cl, obj, (APTR) msg);
    case MUIM_Group_ExitChange2:
        return Group__MUIM_ExitChange2(cl, obj, (APTR) msg);
    case MUIM_Group_InitChange:
        return Group__MUIM_InitChange(cl, obj, (APTR) msg);
    case MUIM_Group_Sort:
        return Group__MUIM_Sort(cl, obj, (APTR) msg);
    case MUIM_Group_DoMethodNoForward:
        return Group__MUIM_DoMethodNoForward(cl, obj, (APTR) msg);
    case MUIM_ConnectParent:
        return Group__MUIM_ConnectParent(cl, obj, (APTR) msg);
    case MUIM_DisconnectParent:
        return Group__MUIM_DisconnectParent(cl, obj, (APTR) msg);
    case MUIM_Layout:
        return Group__MUIM_Layout(cl, obj, (APTR) msg);
    case MUIM_Setup:
        return Group__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Group__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Draw:
        return Group__MUIM_Draw(cl, obj, (APTR) msg);

    case MUIM_FindUData:
        return Group__MUIM_FindUData(cl, obj, (APTR) msg);
    case MUIM_GetUData:
        return Group__MUIM_GetUData(cl, obj, (APTR) msg);
    case MUIM_SetUData:
        return Group__MUIM_SetUData(cl, obj, (APTR) msg);
    case MUIM_SetUDataOnce:
        return Group__MUIM_SetUDataOnce(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Group__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Group__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Group__MUIM_HandleEvent(cl, obj, (APTR) msg);
    case MUIM_DrawBackground:
        return Group__MUIM_DrawBackground(cl, obj, (APTR) msg);
    case MUIM_DragQueryExtended:
        return Group__MUIM_DragQueryExtended(cl, obj, (APTR) msg);
    case MUIM_FindAreaObject:
        return Group__MUIM_FindAreaObject(cl, obj, (APTR) msg);
    case MUIM_Export:
        return Group__MUIM_Export(cl, obj, (APTR) msg);
    case MUIM_Import:
        return Group__MUIM_Import(cl, obj, (APTR) msg);

//#if 0
#if 1
        /* Active getter-probing notification override. */
    case MUIM_Notify:
        return Group_Notify(cl, obj, (APTR) msg);
#endif
    case MUIM_Set:
    case MUIM_MultiSet:
    case MUIM_CallHook:
    case MUIM_DrawParentBackground:
    case MUIM_DragBegin:
    case MUIM_DragDrop:
    case MUIM_DragQuery:
    case MUIM_DragFinish:
    case MUIM_DoDrag:
    case MUIM_CreateDragImage:
    case MUIM_DeleteDragImage:
    case MUIM_GoActive:
    case MUIM_GoInactive:
    case MUIM_CreateBubble:
    case MUIM_DeleteBubble:
    case MUIM_CreateShortHelp:
    case MUIM_DeleteShortHelp:
    case OM_ADDTAIL:
    case OM_REMOVE:
        return DoSuperMethodA(cl, obj, (APTR) msg);
            /* Needs not to be forwarded? */
    }

    /* sometimes you want to call a superclass method,
     * but not dispatching to child.
     * But what to do with list methods in a listview ?
     */
    Group_DispatchMsg(cl, obj, (APTR) msg);

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Group_desc =
{
    MUIC_Group,
    MUIC_Area,
    sizeof(struct MUI_GroupData),
    (void *) Group_Dispatcher
};
