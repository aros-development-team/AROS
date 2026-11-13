/*
    Copyright (C) 2002-2026, The AROS Development Team.
    Copyright (C) 1999, David Le Corfec.
    All rights reserved.

*/

#include "frame.h"
#include <devices/rawkeycodes.h>
#include <exec/types.h>
#include <graphics/gfxmacros.h>
#include <intuition/imageclass.h>
#include <libraries/gadtools.h>
#include <stdlib.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <proto/muimaster.h>
#include <proto/utility.h>

extern struct Library *MUIMasterBase;
#include "muimaster_intern.h"

#include "bubbleengine.h"
#include "datatypescache.h"
#include "font.h"
#include "imspec.h"
#include "menu.h"
#include "mui.h"
#include "prefs.h"
#include "support.h"
#include "textengine.h"

//  #define MYDEBUG 1
#include "debug.h"

/****** Area.mui/MUIC_Area ************
*
*   NAME
*       MUIC_Area
*
*   SUPERCLASS
*       Notify.mui
*
*   DESCRIPTION
*       Provide the base visual-object implementation for layout limits,
*       frame/background drawing, input state, context menus, help and
*       drag-and-drop. Area itself does not own or traverse child objects and
*       contributes only frame/spacing dimensions. Subclasses add content and
*       container behavior.
*       Construction calls Notify, initializes weights to 100 and enables
*       FillArea, ShowMe, ShowSelState and Dropable. InputMode defaults to None.
*       A superclass construction failure returns NULL; local background-copy
*       failure does not fail construction.
*       The object owns its duplicated background specification and setup image
*       resources. It borrows frame/help/size strings, font pointers,
*       context-menu objects and Window RenderInfo. Keep borrowed resources
*       alive while used. Dispose only after hiding and cleanup have ended
*       drawing, timers and event registration. OM_DISPOSE frees the background
*       specification but does not perform Hide or Cleanup.
*
*   METHODS
*       Setup establishes environment resources; AskMinMax establishes size
*       limits; layout precedes Show and drawing. Hide precedes Cleanup. Zune's
*       support wrappers install RenderInfo before Setup, set CANDRAW after
*       successful Show and clear CANDRAW before Hide. Calling Show or Hide
*       directly does not reproduce those wrapper state changes.
*       For custom subclasses, preserve superclass lifecycle calls and unwind
*       resources on failure. Process ordinary object operations in the
*       application's task. Notifications and methods are synchronous; keep the
*       object and callback targets alive throughout calls.
*       Export stores one byte for Selected under a nonzero ObjectID. Import
*       reads the first byte of the corresponding Dataspace entry and sets
*       Selected. Both return zero and ignore underlying failure; Import does
*       not check the entry length. No other Area state is exported.
*
*   NOTES
*       Attribute access below describes actual implementation, including
*       implemented accesses absent from header comments. A setter may rewrite
*       its writable TagItems to TAG_IGNORE before forwarding to Notify. A
*       notification or setter return does not prove resource allocation or
*       layout success.
*       Five methods marked PRIV in area.h remain internal source contracts and
*       are excluded from the public index. Layout, DragQuery, DragDrop and
*       DragReport have explicit dispatcher defaults, not missing
*       implementations. CustomBackfill has no dispatcher case.
*
*   BUGS
*       InputMode and ContextMenu setters do not rebuild setup-time event
*       registration or selected-background resources. Do not assume that
*       changing them while set up produces the same state as constructing with
*       those values. QueryFrameCharacteristics has no defined success return;
*       CreateFrameClippingRegion writes its output into the method message
*       itself. See their individual contracts.
*
*/
/****** Area.mui/MUIA_Background ************
*
*   NAME
*       MUIA_Background
*
*   FUNCTION
*       Duplicate a supported MUII background/pattern identifier or a readable
*       NUL-terminated image-specification string into owned storage. Omission
*       inherits background drawing through the parent or Window.
*
*   INPUTS
*       IPTR image specification; initialize and set. An internal-pointer getter
*       is also implemented.
*
*   NOTES
*       The setter copies before freeing the previous specification. Copy
*       failure preserves the old value. An equal copied specification
*       suppresses its notification by changing the tag to TAG_IGNORE. During
*       setup/show, replacement cleans up and recreates the display
*       specification and requests redraw. The getter returns the private
*       prepared image-spec pointer, not the original identifier or string; do
*       not free it or pass it as an input specification.
*
*   BUGS
*       Prepared-image allocation is not reported by the setter. Constructor
*       copy failure is tolerated. Getter storage is meaningful only within the
*       setup lifetime and is invalidated by replacement or cleanup.
*
*/
/****** Area.mui/MUIA_LeftEdge ************
*
*   NAME
*       MUIA_LeftEdge
*
*   FUNCTION
*       Return left from the object layout box. Coordinates are relative to the
*       native window origin, including border offsets established by layout,
*       not to the parent object.
*
*   INPUTS
*       LONG geometry value; get only.
*
*   NOTES
*       Values describe the full object rectangle including frame and inner
*       spacing. Right and bottom are inclusive. Meaningful geometry requires
*       completed layout; the getter does not force layout or test visibility.
*
*/
/****** Area.mui/MUIA_TopEdge ************
*
*   NAME
*       MUIA_TopEdge
*
*   FUNCTION
*       Return top from the object layout box. Coordinates are relative to the
*       native window origin, including border offsets established by layout,
*       not to the parent object.
*
*   INPUTS
*       LONG geometry value; get only.
*
*   NOTES
*       Values describe the full object rectangle including frame and inner
*       spacing. Right and bottom are inclusive. Meaningful geometry requires
*       completed layout; the getter does not force layout or test visibility.
*
*/
/****** Area.mui/MUIA_RightEdge ************
*
*   NAME
*       MUIA_RightEdge
*
*   FUNCTION
*       Return left + width - 1 from the object layout box. Coordinates are
*       relative to the native window origin, including border offsets
*       established by layout, not to the parent object.
*
*   INPUTS
*       LONG geometry value; get only.
*
*   NOTES
*       Values describe the full object rectangle including frame and inner
*       spacing. Right and bottom are inclusive. Meaningful geometry requires
*       completed layout; the getter does not force layout or test visibility.
*
*/
/****** Area.mui/MUIA_BottomEdge ************
*
*   NAME
*       MUIA_BottomEdge
*
*   FUNCTION
*       Return top + height - 1 from the object layout box. Coordinates are
*       relative to the native window origin, including border offsets
*       established by layout, not to the parent object.
*
*   INPUTS
*       LONG geometry value; get only.
*
*   NOTES
*       Values describe the full object rectangle including frame and inner
*       spacing. Right and bottom are inclusive. Meaningful geometry requires
*       completed layout; the getter does not force layout or test visibility.
*
*/
/****** Area.mui/MUIA_Width ************
*
*   NAME
*       MUIA_Width
*
*   FUNCTION
*       Return width from the object layout box. Coordinates are relative to the
*       native window origin, including border offsets established by layout,
*       not to the parent object.
*
*   INPUTS
*       LONG geometry value; get only.
*
*   NOTES
*       Values describe the full object rectangle including frame and inner
*       spacing. Right and bottom are inclusive. Meaningful geometry requires
*       completed layout; the getter does not force layout or test visibility.
*
*/
/****** Area.mui/MUIA_Height ************
*
*   NAME
*       MUIA_Height
*
*   FUNCTION
*       Return height from the object layout box. Coordinates are relative to
*       the native window origin, including border offsets established by
*       layout, not to the parent object.
*
*   INPUTS
*       LONG geometry value; get only.
*
*   NOTES
*       Values describe the full object rectangle including frame and inner
*       spacing. Right and bottom are inclusive. Meaningful geometry requires
*       completed layout; the getter does not force layout or test visibility.
*
*/
/****** Area.mui/MUIA_ContextMenu ************
*
*   NAME
*       MUIA_ContextMenu
*
*   FUNCTION
*       Store a borrowed context-menu object, initially NULL. During setup, a
*       non-NULL pointer requests mouse-button handling. A right-button press in
*       the object calls ContextMenuBuild; the returned menu is connected to
*       this object and used to open a temporary native menu.
*
*   INPUTS
*       Object pointer; initialize, set, get.
*
*   NOTES
*       The Area does not dispose the menu object. Keep it and its items alive
*       while used. Hide closes a live native context menu. Replacing the
*       pointer does not close an existing menu or update event registration. A
*       dynamic ContextMenuBuild override still needs a non-NULL ContextMenu
*       value to enter the default event path.
*
*/
/****** Area.mui/MUIA_ContextMenuTrigger ************
*
*   NAME
*       MUIA_ContextMenuTrigger
*
*   FUNCTION
*       ContextMenuChoice sets this tag to the chosen menu item for Notify
*       delivery.
*
*   INPUTS
*       Object pointer notification event; no Area getter or storage.
*
*   NOTES
*       Use the notification trigger value; the pointer is borrowed. Selection
*       also updates Menuitem_Checked when applicable and raises
*       Menuitem_Trigger before ContextMenuChoice. The native menu closes
*       afterward. The header get flag does not correspond to an Area getter.
*
*/
/****** Area.mui/MUIA_ControlChar ************
*
*   NAME
*       MUIA_ControlChar
*
*   FUNCTION
*       Store a keyboard shortcut, initially zero. Setup registers the character
*       and input-mode action with the owning Window. Setting it during setup
*       removes and rebuilds that registration.
*
*   INPUTS
*       Character value stored in TEXT; initialize, set, get.
*
*   NOTES
*       The getter returns the stored character. RelVerify and Immediate
*       register MUIKEY_PRESS; Toggle registers MUIKEY_TOGGLE. None has no
*       action assignment in the helper. Character matching follows the Window
*       key-conversion path; this attribute does not define a new key map.
*
*/
/****** Area.mui/MUIA_CycleChain ************
*
*   NAME
*       MUIA_CycleChain
*
*   FUNCTION
*       Enable membership in the Window focus cycle chain, initially FALSE. The
*       getter returns a Boolean rather than an ordering value.
*
*   INPUTS
*       LONG treated as Boolean; initialize, set, get.
*
*   NOTES
*       Changes during setup remove and recreate the combined
*       control-character/cycle registration. Window appends a private cycle
*       node; nonzero values do not select a numeric position. InputMode None is
*       not rejected. Window cycle-node allocation failure is not reported to
*       this setter.
*
*/
/****** Area.mui/MUIA_Disabled ************
*
*   NAME
*       MUIA_Disabled
*
*   FUNCTION
*       Set or clear the independent high-bit disabled flag. The getter returns
*       TRUE when either that flag or the NestedDisabled count is nonzero.
*       Default FALSE.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       Clearing Disabled does not cancel nested disables. A change between
*       enabled and disabled requests redraw and, if set up, calls the release
*       helper with cancellation. Notify receives the requested tag value, which
*       need not equal the combined getter result. Area does not itself
*       propagate this state to children.
*
*/
/****** Area.mui/MUIA_NestedDisabled ************
*
*   NAME
*       MUIA_NestedDisabled
*
*   FUNCTION
*       A nonzero setter increments the low 31-bit disable count; zero
*       decrements a positive count. The getter reports whether this count is
*       nonzero, excluding the independent Disabled flag.
*
*   INPUTS
*       BOOL operation; initialize, set, get.
*
*   NOTES
*       Initially zero. Construction treats any true initialization tag as
*       setting count bit zero, not as repeated increments. Balance runtime
*       increments and decrements. Combined zero/nonzero transitions release
*       input with cancellation and request redraw.
*
*   BUGS
*       The count wraps through masking at 31 bits; there is no overflow error.
*       A notification carries the requested Boolean, not the count or combined
*       Disabled state.
*
*/
/****** Area.mui/MUIA_Draggable ************
*
*   NAME
*       MUIA_Draggable
*
*   FUNCTION
*       Enable the default motion handler to start dragging; default FALSE.
*       While the pointer remains inside and the left button is held, movement
*       of at least three pixels on either axis from the initial click starts
*       DoDrag.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       The flag alone does not register motion events. The default RelVerify
*       press path adds them; other input arrangements must provide appropriate
*       delivery. Starting a drag clears pressed state, stops the repeat timer
*       and restores the mouse-button event mask. Direct DoDrag does not check
*       this flag.
*
*/
/****** Area.mui/MUIA_Dropable ************
*
*   NAME
*       MUIA_Dropable
*
*   FUNCTION
*       Allow this object to be considered by the private drop-target query;
*       default TRUE.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       The query also requires coordinates inside the object and a DragQuery
*       result equal to Accept. Area's DragQuery default refuses every source,
*       so Dropable alone does not make a working drop target. The private query
*       does not independently test Disabled or ShowMe.
*
*/
/****** Area.mui/MUIA_FillArea ************
*
*   NAME
*       MUIA_FillArea
*
*   FUNCTION
*       Choose whether Area drawing fills the content region as well as the
*       frame surroundings. Default TRUE. FALSE limits ordinary background
*       rectangles to the area outside the inner box.
*
*   INPUTS
*       BOOL; initialize and set.
*
*   NOTES
*       There is no getter and the setter does not itself request redraw.
*       Rounded-frame preparation can first paint the parent background over the
*       frame area even when FillArea is FALSE. This flag does not suppress the
*       frame/title.
*
*/
/****** Area.mui/MUIA_FixWidth ************
*
*   NAME
*       MUIA_FixWidth
*
*   FUNCTION
*       Request a fixed content width. During final min/max processing, a
*       positive stored value plus frame/spacing is clamped to the limits
*       already computed and assigned to minimum, default and maximum.
*
*   INPUTS
*       LONG stored in WORD; initialize only.
*
*   NOTES
*       Nonpositive stored values do not take the fixed-size branch. The field
*       is shared with MaxWidth; supplying both tags leaves both flags set and
*       the last numeric value stored. Positive fixed size takes precedence over
*       text sizing, then maximum size. No setter or getter is implemented.
*
*/
/****** Area.mui/MUIA_FixWidthTxt ************
*
*   NAME
*       MUIA_FixWidthTxt
*
*   FUNCTION
*       Borrow a NUL-terminated text string whose measured width plus
*       frame/spacing is used as a fixed dimension, clamped to existing limits.
*
*   INPUTS
*       STRPTR; initialize only.
*
*   NOTES
*       The string is parsed and measured during final min/max processing. Keep
*       it alive while the object can be laid out. A positive FixWidth takes
*       precedence. Failure to create the temporary text object leaves that
*       dimension unchanged and is not reported; the MaxWidth branch is not then
*       retried.
*
*/
/****** Area.mui/MUIA_MaxWidth ************
*
*   NAME
*       MUIA_MaxWidth
*
*   FUNCTION
*       Limit maximum content width by adding frame/spacing and clamping to the
*       previously computed min/max range.
*
*   INPUTS
*       LONG stored in WORD; initialize only.
*
*   NOTES
*       The fixed positive and text-size branches take precedence. The numeric
*       field is shared with FixWidth. No getter, setter or immediate resize is
*       implemented. This branch changes only the maximum, not the default
*       value.
*
*/
/****** Area.mui/MUIA_FixHeight ************
*
*   NAME
*       MUIA_FixHeight
*
*   FUNCTION
*       Request a fixed content height. During final min/max processing, a
*       positive stored value plus frame/spacing is clamped to the limits
*       already computed and assigned to minimum, default and maximum.
*
*   INPUTS
*       LONG stored in WORD; initialize only.
*
*   NOTES
*       Nonpositive stored values do not take the fixed-size branch. The field
*       is shared with MaxHeight; supplying both tags leaves both flags set and
*       the last numeric value stored. Positive fixed size takes precedence over
*       text sizing, then maximum size. No setter or getter is implemented.
*
*/
/****** Area.mui/MUIA_FixHeightTxt ************
*
*   NAME
*       MUIA_FixHeightTxt
*
*   FUNCTION
*       Borrow a NUL-terminated text string whose measured height plus
*       frame/spacing is used as a fixed dimension, clamped to existing limits.
*
*   INPUTS
*       STRPTR; initialize only.
*
*   NOTES
*       The string is parsed and measured during final min/max processing. Keep
*       it alive while the object can be laid out. A positive FixHeight takes
*       precedence. Failure to create the temporary text object leaves that
*       dimension unchanged and is not reported; the MaxHeight branch is not
*       then retried.
*
*/
/****** Area.mui/MUIA_MaxHeight ************
*
*   NAME
*       MUIA_MaxHeight
*
*   FUNCTION
*       Limit maximum content height by adding frame/spacing and clamping to the
*       previously computed min/max range.
*
*   INPUTS
*       LONG stored in WORD; initialize only.
*
*   NOTES
*       The fixed positive and text-size branches take precedence. The numeric
*       field is shared with FixHeight. No getter, setter or immediate resize is
*       implemented. This branch changes only the maximum, not the default
*       value.
*
*/
/****** Area.mui/MUIA_Font ************
*
*   NAME
*       MUIA_Font
*
*   FUNCTION
*       Store the font request, default MUIV_Font_Inherit (zero). Setup resolves
*       inherited parent font, or the normal window font for a root, or the
*       supplied preset/pointer.
*
*   INPUTS
*       IPTR font preset or borrowed TextFont pointer; initialize, get; setter
*       also implemented.
*
*   NOTES
*       The getter returns the request, not the resolved TextFont. Cached preset
*       fonts belong to Window RenderInfo. Supplied font pointers remain
*       borrowed. The setter only updates the request and does not re-resolve
*       the live drawing font or relayout.
*
*/
/****** Area.mui/MUIA_Frame ************
*
*   NAME
*       MUIA_Frame
*
*   FUNCTION
*       Store the requested frame, initially MUIV_Frame_None. Values greater
*       than 100 are treated as pointers to frame-specification strings; other
*       values index the preference frame array. Failed string parsing falls
*       back to the None frame.
*
*   INPUTS
*       IPTR frame preset or borrowed specification string; initialize; setter
*       and getter also implemented.
*
*   NOTES
*       Keep a string alive while the object uses it. Setup removes a nonzero
*       frame and its title from the Window root object. The getter returns the
*       stored request. The setter recomputes inner/title sizes when global
*       information exists, but does not request layout or redraw.
*
*   BUGS
*       There is no numeric bounds check: use declared valid frame presets, not
*       arbitrary values up to 100. With a title, size recomputation uses
*       RenderInfo-dependent font lookup, so connection alone is not a
*       sufficient lifecycle guarantee for a runtime setter.
*
*/
/****** Area.mui/MUIA_FramePhantomHoriz ************
*
*   NAME
*       MUIA_FramePhantomHoriz
*
*   FUNCTION
*       The presence of this tag enables phantom-frame handling, even when its
*       value is FALSE. The flag is initially clear.
*
*   INPUTS
*       BOOL tag; initialize only.
*
*   NOTES
*       Horizontal frame/spacing contributions are forced to zero, and drawing
*       suppresses the entire frame and title. Vertical contributions remain.
*       There is no setter or getter; this implementation is broader than merely
*       hiding horizontal borders.
*
*/
/****** Area.mui/MUIA_FrameTitle ************
*
*   NAME
*       MUIA_FrameTitle
*
*   FUNCTION
*       Borrow a NUL-terminated frame-title string, initially NULL. Setup
*       computes title spacing from the title font and preferences; drawing
*       centers and fits the title.
*
*   INPUTS
*       STRPTR; initialize only.
*
*   NOTES
*       Keep it alive through setup and drawing. Setup clears the title when
*       removing a nonzero root-object frame. There is no setter or getter.
*       Font-derived title dimensions are stored in BYTE fields without general
*       overflow validation.
*
*/
/****** Area.mui/MUIA_InnerBottom ************
*
*   NAME
*       MUIA_InnerBottom
*
*   FUNCTION
*       Set explicit bottom inner spacing, clamped to 0 through 32 during
*       construction. Otherwise setup obtains the value from the frame
*       preferences; Window can supply root spacing.
*
*   INPUTS
*       LONG; initialize and get.
*
*   NOTES
*       The getter returns the stored spacing, excluding the frame border and
*       title additions. There is no setter. Explicit zero differs from omitting
*       the tag because it prevents default spacing replacement.
*
*/
/****** Area.mui/MUIA_InnerLeft ************
*
*   NAME
*       MUIA_InnerLeft
*
*   FUNCTION
*       Set explicit left inner spacing, clamped to 0 through 32 during
*       construction. Otherwise setup obtains the value from the frame
*       preferences; Window can supply root spacing.
*
*   INPUTS
*       LONG; initialize and get.
*
*   NOTES
*       The getter returns the stored spacing, excluding the frame border and
*       title additions. There is no setter. Explicit zero differs from omitting
*       the tag because it prevents default spacing replacement.
*
*/
/****** Area.mui/MUIA_InnerRight ************
*
*   NAME
*       MUIA_InnerRight
*
*   FUNCTION
*       Set explicit right inner spacing, clamped to 0 through 32 during
*       construction. Otherwise setup obtains the value from the frame
*       preferences; Window can supply root spacing.
*
*   INPUTS
*       LONG; initialize and get.
*
*   NOTES
*       The getter returns the stored spacing, excluding the frame border and
*       title additions. There is no setter. Explicit zero differs from omitting
*       the tag because it prevents default spacing replacement.
*
*/
/****** Area.mui/MUIA_InnerTop ************
*
*   NAME
*       MUIA_InnerTop
*
*   FUNCTION
*       Set explicit top inner spacing, clamped to 0 through 32 during
*       construction. Otherwise setup obtains the value from the frame
*       preferences; Window can supply root spacing.
*
*   INPUTS
*       LONG; initialize and get.
*
*   NOTES
*       The getter returns the stored spacing, excluding the frame border and
*       title additions. There is no setter. Explicit zero differs from omitting
*       the tag because it prevents default spacing replacement.
*
*/
/****** Area.mui/MUIA_HorizWeight ************
*
*   NAME
*       MUIA_HorizWeight
*
*   FUNCTION
*       Store relative horizontal layout weight, default 100.
*
*   INPUTS
*       WORD declaration, stored as UWORD; initialize, set, get.
*
*   NOTES
*       Area stores the value without range validation or immediate relayout.
*       The parent layout implementation determines how it distributes space.
*       Conversion to UWORD can wrap negative or oversized values.
*
*/
/****** Area.mui/MUIA_VertWeight ************
*
*   NAME
*       MUIA_VertWeight
*
*   FUNCTION
*       Store relative vertical layout weight, default 100.
*
*   INPUTS
*       WORD declaration, stored as UWORD; initialize, set, get.
*
*   NOTES
*       Area stores the value without range validation or immediate relayout.
*       The parent layout implementation determines how it distributes space.
*       Conversion to UWORD can wrap negative or oversized values.
*
*/
/****** Area.mui/MUIA_Weight ************
*
*   NAME
*       MUIA_Weight
*
*   FUNCTION
*       Set both horizontal and vertical weights to the same value. Each
*       defaults to 100.
*
*   INPUTS
*       WORD declaration, stored as UWORD; initialize only.
*
*   NOTES
*       There is no combined getter or setter. Use HorizWeight and VertWeight
*       for runtime access. Initialization tag order determines the final values
*       when combined with those tags.
*
*/
/****** Area.mui/MUIA_InputMode ************
*
*   NAME
*       MUIA_InputMode
*
*   FUNCTION
*       Select None, RelVerify, Immediate or Toggle; default None. RelVerify
*       sets Selected and Pressed on press, repeats Timer, and verifies release
*       while selected. Immediate forces a Selected notification on press.
*       Toggle flips Selected and Pressed on the default mouse-press path.
*
*   INPUTS
*       LONG stored in BYTE; initialize; setter also implemented.
*
*   NOTES
*       The MUIKEY_TOGGLE path changes only Selected. RelVerify construction
*       clears an initial Selected value. There is no getter. Changing mode
*       after setup does not rebuild handlers, shortcut action or
*       selected-background resources; configure the mode before setup.
*
*   BUGS
*       Cleanup tests the current mode when freeing selected-background
*       resources, so changing it after setup can bypass a required cleanup. The
*       BYTE value is not validated against the defined modes.
*
*/
/****** Area.mui/MUIA_Pressed ************
*
*   NAME
*       MUIA_Pressed
*
*   FUNCTION
*       Return or set the pressed flag, initially FALSE. The input handlers use
*       it for press/release notifications.
*
*   INPUTS
*       BOOL; get; setter also implemented.
*
*   NOTES
*       Construction ignores this tag. In RelVerify, a verified release clears
*       Pressed with notification only if Selected is still true. Cancellation
*       clears it without notification in that selected branch. If Selected was
*       cleared when the pointer left, the release helper can leave Pressed
*       true. Do not equate this field unconditionally with the physical button
*       state.
*
*/
/****** Area.mui/MUIA_Selected ************
*
*   NAME
*       MUIA_Selected
*
*   FUNCTION
*       Store selection state, initially FALSE. A changed setter requests
*       complete object redraw. An unchanged setter changes its TagItem to
*       TAG_IGNORE, suppressing that notification.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       RelVerify construction forces FALSE despite an initialization tag. A
*       runtime setter is not restricted by InputMode. Immediate input silently
*       clears an already selected state before setting TRUE so a new press can
*       notify. Export/Import serializes this state under ObjectID.
*
*/
/****** Area.mui/MUIA_ShortHelp ************
*
*   NAME
*       MUIA_ShortHelp
*
*   FUNCTION
*       Store a borrowed NUL-terminated help string, initially NULL.
*       CreateShortHelp returns it unchanged.
*
*   INPUTS
*       STRPTR; initialize, set, get.
*
*   NOTES
*       Keep it alive until any bubble using it is deleted. The default Window
*       help search requires a non-NULL stored ShortHelp before selecting this
*       object, even with an overridden CreateShortHelp. Setting a new pointer
*       does not close an existing bubble.
*
*/
/****** Area.mui/MUIA_ShowMe ************
*
*   NAME
*       MUIA_ShowMe
*
*   FUNCTION
*       Store whether the object should be shown; default TRUE. A changed FALSE
*       setter hides a drawable object. A changed TRUE setter shows it when its
*       parent is drawable. When this changes live display, request Window
*       relayout.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       This setter changes show/hide state, not setup/cleanup state: the
*       cleanup/setup code in that branch is disabled. Parent container layout
*       determines participation. An object without a parent has no implemented
*       immediate show branch. The getter is the requested flag, not proof of
*       CANDRAW or an open window.
*
*/
/****** Area.mui/MUIA_ShowSelState ************
*
*   NAME
*       MUIA_ShowSelState
*
*   FUNCTION
*       Enable selected background and frame-state inversion; default TRUE.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       Selected background is prepared during setup only when InputMode is not
*       None. This flag changes presentation, not Selected or Pressed semantics.
*       No setter or getter is implemented.
*
*/
/****** Area.mui/MUIA_Timer ************
*
*   NAME
*       MUIA_Timer
*
*   FUNCTION
*       Return the repeat counter used by RelVerify input. The press path
*       increments it immediately, requests a 300 ms first timer, then the timer
*       method re-registers at 50 ms and increments only while Selected.
*
*   INPUTS
*       ULONG stored notification counter; get; setter also implemented.
*
*   NOTES
*       This is not a configurable timer interval. A mouse press first resets
*       the counter without notification; the keyboard path does not perform
*       that reset. Release, drag start and cleanup remove an active timer.
*       Setting Timer only changes the counter and passes the tag to Notify;
*       construction ignores it. Timer registration failure is not reported
*       here.
*
*/
/****** Area.mui/MUIA_Window ************
*
*   NAME
*       MUIA_Window
*
*   FUNCTION
*       Return the native window from RenderInfo, or NULL when RenderInfo is
*       absent.
*
*   INPUTS
*       Intuition Window pointer; get only.
*
*   NOTES
*       The pointer is borrowed. Setup establishes RenderInfo and Cleanup clears
*       it. The native window may still be NULL during setup and becomes invalid
*       at close; do not close it yourself.
*
*/
/****** Area.mui/MUIA_WindowObject ************
*
*   NAME
*       MUIA_WindowObject
*
*   FUNCTION
*       Return the owning Window object from RenderInfo, or NULL when RenderInfo
*       is absent.
*
*   INPUTS
*       Zune Window object pointer; get only.
*
*   NOTES
*       The pointer is borrowed. Setup establishes RenderInfo and Cleanup clears
*       it. This does not transfer ownership or prove the native window is open.
*
*/
/****** Area.mui/MUIA_CustomBackfill ************
*
*   NAME
*       MUIA_CustomBackfill
*
*   FUNCTION
*       Declared in area.h but not implemented by Area.
*
*   INPUTS
*       Declared initialize only.
*
*   NOTES
*       There is no corresponding constructor, setter or getter case. Tags can
*       still participate in Notify delivery. The declaration does not establish
*       this capability.
*
*/
/****** Area.mui/MUIA_DoubleBuffer ************
*
*   NAME
*       MUIA_DoubleBuffer
*
*   FUNCTION
*       Declared in area.h but not implemented by Area.
*
*   INPUTS
*       BOOL; declared initialize, set, get.
*
*   NOTES
*       There is no corresponding constructor, setter or getter case. Tags can
*       still participate in Notify delivery. The declaration does not establish
*       this capability.
*
*/
/****** Area.mui/MUIA_HorizDisappear ************
*
*   NAME
*       MUIA_HorizDisappear
*
*   FUNCTION
*       Declared in area.h but not implemented by Area.
*
*   INPUTS
*       LONG; declared initialize, set, get.
*
*   NOTES
*       There is no corresponding constructor, setter or getter case. Tags can
*       still participate in Notify delivery. The declaration does not establish
*       this capability.
*
*/
/****** Area.mui/MUIA_VertDisappear ************
*
*   NAME
*       MUIA_VertDisappear
*
*   FUNCTION
*       Declared in area.h but not implemented by Area.
*
*   INPUTS
*       LONG; declared initialize, set, get.
*
*   NOTES
*       There is no corresponding constructor, setter or getter case. Tags can
*       still participate in Notify delivery. The declaration does not establish
*       this capability.
*
*/
/****** Area.mui/MUIA_ExportID ************
*
*   NAME
*       MUIA_ExportID
*
*   FUNCTION
*       Declared in area.h but not implemented by Area.
*
*   INPUTS
*       ULONG; declared under MUI_OBSOLETE.
*
*   NOTES
*       There is no corresponding constructor, setter or getter case. Tags can
*       still participate in Notify delivery. Export and Import use the
*       inherited ObjectID attribute instead; ExportID is not an alias here.
*
*/

#if (0)
static const int __version = 1;
static const int __revision = 1;

#ifdef DEBUG
/* Inactive debug-only declaration inside #if (0). No definition or callable
 * implementation exists in this file. It supplies no runtime string-
 * conversion contract and must not be treated as an available API.
 */
static STRPTR zune_area_to_string(Object *area);
#endif
#endif

/* Resolve the live object frame request using application preferences.
 * Values above 100 are parsed as a borrowed string into writable tempstore;
 * parse failure returns the None preference. Other values directly index
 * the preference array without bounds checking. Return a borrowed
 * preference pointer or tempstore; neither outlives its backing storage.
 */
static const struct MUI_FrameSpec_intern *
get_intframe(Object *obj, struct MUI_AreaData *data,
             struct MUI_FrameSpec_intern *tempstore);
/* Resolve the frame and copy its inner spacing into components not
 * explicitly initialized by the caller. Requires connected preferences and
 * a valid frame request. Mutate Area instance data without layout, drawing
 * or allocation ownership transfer; return no status.
 */
static void set_inner_sizes(Object *obj, struct MUI_AreaData *data);
/* If a borrowed frame-title string exists, resolve frame and title font and
 * store title offsets in BYTE fields from preferences. Requires live
 * RenderInfo and font resources. Sets the object font to the title font and
 * does not restore it here. No status or overflow guarantee is provided.
 */
static void set_title_sizes(Object *obj, struct MUI_AreaData *data);
/* Restore the embedded event registration to mouse buttons, then for
 * selected RelVerify objects clear Pressed (silently on cancel) and
 * Selected. Stop an active repeat timer. Requires live Window/application;
 * it may register a mouse handler even if none existed. If not selected,
 * Pressed is not cleared. Return no status and keep state alive through
 * notifications.
 */
static void handle_release(struct IClass *cl, Object *obj, int cancel);

/* Compute BYTE add-left/top and total sub-width/height from current inner
 * spacing, title height and frame borders. Phantom mode zeros both
 * horizontal contributions. Borrow live inputs and mutate instance geometry
 * offsets; frame parameter is unused. No clamping/overflow check or status
 * is provided.
 */
static void area_update_msizes(Object *obj, struct MUI_AreaData *data,
                               const struct MUI_FrameSpec_intern *frame,
                               const struct ZuneFrameGfx *zframe);
/* Register the embedded borrowed control-character node with Window when a
 * character or cycle flag exists. Map RelVerify/Immediate to PRESS and
 * Toggle to TOGGLE; None leaves the action field unchanged. Requires live
 * Window and stable registration state. Window may allocate a cycle node;
 * failure is ignored. Return no status.
 */
static void setup_control_char(struct MUI_AreaData *data, Object *obj,
                               struct IClass *cl);
/* Remove the embedded control-character/cycle registration when current
 * character or cycle flags request it. Requires the corresponding live
 * Window and existing registration. Do not free the embedded node. No
 * membership validation or result is provided.
 */
static void cleanup_control_char(struct MUI_AreaData *data, Object *obj);

// static void setup_cycle_chain (struct MUI_AreaData *data, Object *obj);
// static void cleanup_cycle_chain (struct MUI_AreaData *data, Object *obj);

#define ZUNE_FOCUS_TYPE_ACTIVE_OBJ 0
#define ZUNE_FOCUS_TYPE_DROP_OBJ 1

/* Draw a keyboard or drop outline for a drawable live Area with a non-
 * Window parent; skip NULL/undrawable/root targets. Borrow the RastPort and
 * pens, restoring only the line pattern after drawing. Keyboard outline
 * extends one pixel outside bounds; drop outline uses the bounds. Return no
 * status.
 */
static void _zune_focus_new(Object *obj, int type)
{
    Object *parent;
    struct RastPort *rp;
    UWORD oldDrPt;

    // bug("_zune_focus_new 1 %p\n", obj);

    if (NULL == obj || !(_flags(obj) & MADF_CANDRAW))
        return;

    parent = _parent(obj);
    rp = _rp(obj);
    oldDrPt = rp->LinePtrn;

    int x1 = _left(obj);
    int y1 = _top(obj);
    int x2 = _left(obj) + _width(obj) - 1;
    int y2 = _top(obj) + _height(obj) - 1;

    if (!parent || parent == _win(obj))
        return;

    SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], _pens(obj)[MPEN_SHADOW], JAM2);

    if (type == ZUNE_FOCUS_TYPE_ACTIVE_OBJ) {
        SetDrPt(rp, 0xCCCC);
        x1--;
        y1--;
        x2++;
        y2++;
    } else {
        SetDrPt(rp, 0xF0F0);
    }

    Move(rp, x1, y1);
    Draw(rp, x2, y1);
    Draw(rp, x2, y2);
    Draw(rp, x1, y2);
    Draw(rp, x1, y1);
    SetDrPt(rp, oldDrPt);
}

/* Erase a drawable object outline. Keyboard focus repaints four strips
 * through the parent background. Drop focus redraws the object under an
 * edge clip region when available. Borrow live object/display resources;
 * release an installed clip through MUI_RemoveClipRegion. Clip
 * allocation/install failures are not fully reported and redraw may be
 * broader. Return no status.
 */
static void _zune_focus_destroy(Object *obj, int type)
{
    Object *parent;

    // bug("_zune_focus_destroy 1 %p\n", obj);

    if (NULL == obj || !(_flags(obj) & MADF_CANDRAW))
        return;

    parent = _parent(obj);
    int x1 = _left(obj);
    int y1 = _top(obj);
    int x2 = _left(obj) + _width(obj) - 1;
    int y2 = _top(obj) + _height(obj) - 1;
    int width;
    int height;

    if (type == ZUNE_FOCUS_TYPE_ACTIVE_OBJ) {
        if (!parent || parent == _win(obj))
            return;

        x1--;
        y1--;
        x2++;
        y2++;
        width = x2 - x1 + 1;
        height = y2 - y1 + 1;

        DoMethod(parent, MUIM_DrawBackground, x1, y1, width, 1, x1, y1, 0);
        DoMethod(parent, MUIM_DrawBackground, x2, y1, 1, height, x2, y1, 0);
        DoMethod(parent, MUIM_DrawBackground, x1, y2, width, 1, x1, y2, 0);
        DoMethod(parent, MUIM_DrawBackground, x1, y1, 1, height, x1, y1, 0);
    } else {
        struct Region *region;
        struct Rectangle rect;
        APTR clip = NULL;

        region = NewRegion();
        if (region) {
            rect.MinX = _left(obj);
            rect.MinY = _top(obj);
            rect.MaxX = _right(obj);
            rect.MaxY = _top(obj);

            OrRectRegion(region, &rect);

            rect.MinX = _right(obj);
            rect.MinY = _top(obj);
            rect.MaxX = _right(obj);
            rect.MaxY = _bottom(obj);

            OrRectRegion(region, &rect);

            rect.MinX = _left(obj);
            rect.MinY = _bottom(obj);
            rect.MaxX = _right(obj);
            rect.MaxY = _bottom(obj);

            OrRectRegion(region, &rect);

            rect.MinX = _left(obj);
            rect.MinY = _top(obj);
            rect.MaxX = _left(obj);
            rect.MaxY = _bottom(obj);

            OrRectRegion(region, &rect);

            clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
        }

        MUI_Redraw(obj, MADF_DRAWOBJECT);

        if (region) {
            MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
        }
    }
}

/* Construct Notify, establish visual/input defaults and parse
 * initialization tags. Adopt a duplicated background specification but
 * borrow supplied frame/help/text/font/menu data. Return the object or NULL
 * on superclass failure; local duplicate failure is tolerated. RelVerify
 * clears initial Selected; a FramePhantomHoriz tag sets its flag regardless
 * of value.
 */
static IPTR Area__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_AreaData *data;
    struct TagItem *tags, *tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
        return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    data->mad_Flags =
        MADF_FILLAREA | MADF_SHOWME | MADF_SHOWSELSTATE | MADF_DROPABLE;
    data->mad_HorizWeight = data->mad_VertWeight = 100;
    data->mad_InputMode = MUIV_InputMode_None;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_Background:

            data->mad_Flags |= MADF_OWNBG;
            if (data->mad_BackgroundSpec) {
                zune_image_spec_free(data->mad_BackgroundSpec);
            }
            data->mad_BackgroundSpec = zune_image_spec_duplicate(tag->ti_Data);

            break;

        case MUIA_ControlChar:
            data->mad_ControlChar = tag->ti_Data;
            break;

        case MUIA_CycleChain:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
            break;

        case MUIA_NestedDisabled:
            if (tag->ti_Data)
                data->mad_DisableCount |= 1;
            break;

        case MUIA_Disabled:
            if (tag->ti_Data)
                data->mad_DisableCount |= (1 << 31);
            break;

        case MUIA_FillArea:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_FILLAREA);
            break;

        case MUIA_Draggable:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DRAGGABLE);
            break;

        case MUIA_Dropable:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DROPABLE);
            break;

        case MUIA_FixHeight:
            data->mad_Flags |= MADF_FIXHEIGHT;
            data->mad_HardHeight = tag->ti_Data;
            break;

        case MUIA_FixHeightTxt:
            data->mad_HardHeightTxt = (STRPTR)tag->ti_Data;
            break;

        case MUIA_FixWidth:
            data->mad_Flags |= MADF_FIXWIDTH;
            data->mad_HardWidth = tag->ti_Data;
            break;

        case MUIA_FixWidthTxt:
            data->mad_HardWidthTxt = (STRPTR)tag->ti_Data;
            break;

        case MUIA_Font:
            data->mad_FontPreset = tag->ti_Data;
            break;

        case MUIA_Frame:
            data->mad_Frame = tag->ti_Data;
            break;

        case MUIA_FramePhantomHoriz:
            data->mad_Flags |= MADF_FRAMEPHANTOM;
            break;

        case MUIA_FrameTitle:
            data->mad_FrameTitle = (STRPTR)tag->ti_Data;
            break;

        case MUIA_HorizWeight:
            data->mad_HorizWeight = tag->ti_Data;
            break;

        case MUIA_InnerBottom:
            data->mad_Flags |= MADF_INNERBOTTOM;
            data->mad_InnerBottom = CLAMP((IPTR)tag->ti_Data, 0, 32);
            break;

        case MUIA_InnerLeft:
            data->mad_Flags |= MADF_INNERLEFT;
            data->mad_InnerLeft = CLAMP((IPTR)tag->ti_Data, 0, 32);
            break;

        case MUIA_InnerRight:
            data->mad_Flags |= MADF_INNERRIGHT;
            data->mad_InnerRight = CLAMP((IPTR)tag->ti_Data, 0, 32);
            break;

        case MUIA_InnerTop:
            data->mad_Flags |= MADF_INNERTOP;
            data->mad_InnerTop = CLAMP((IPTR)tag->ti_Data, 0, 32);
            break;

        case MUIA_InputMode:
            data->mad_InputMode = tag->ti_Data;
            break;

        case MUIA_MaxHeight:
            data->mad_Flags |= MADF_MAXHEIGHT;
            data->mad_HardHeight = tag->ti_Data;
            break;

        case MUIA_MaxWidth:
            data->mad_Flags |= MADF_MAXWIDTH;
            data->mad_HardWidth = tag->ti_Data;
            break;

        case MUIA_Selected:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_SELECTED);
            break;

        case MUIA_ShortHelp:
            data->mad_ShortHelp = (STRPTR)tag->ti_Data;
            break;

        case MUIA_ShowMe:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_SHOWME);
            break;

        case MUIA_ShowSelState:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_SHOWSELSTATE);
            break;

        case MUIA_VertWeight:
            data->mad_VertWeight = tag->ti_Data;
            break;

        case MUIA_Weight:
            data->mad_HorizWeight = data->mad_VertWeight = tag->ti_Data;
            break;

        case MUIA_ContextMenu:
            data->mad_ContextMenu = (Object *)tag->ti_Data;
            break;
        }
    }

    /* In Soliton MUIA_Selected was set to MUIV_InputMode_RelVerify (=1)
     ** for MUIA_Input_Mode MUIV_InputMode_RelVerify which is wrong of
     ** course but MUI seems to filter this out so we have to do it also
     */
    if (data->mad_InputMode == MUIV_InputMode_RelVerify) {
        if (data->mad_Flags & MADF_SELECTED)
            D(bug("MUIA_Selected was set in OM_NEW, "
                  "although being in MUIV_InputMode_RelVerify\n"));
        data->mad_Flags &= ~MADF_SELECTED;
    }

    data->mad_ehn.ehn_Events = 0; /* Will be filled on demand */
    data->mad_ehn.ehn_Priority = -5;

    /* Please also send mui key events to us, no idea if mui handles this
     * like this */
    data->mad_ehn.ehn_Flags = MUI_EHF_ALWAYSKEYS | MUI_EHF_GUIMODE;
    data->mad_ehn.ehn_Object = obj;
    data->mad_ehn.ehn_Class = cl;

    data->mad_hiehn.ehn_Events = 0;
    data->mad_hiehn.ehn_Priority = -10;
    data->mad_hiehn.ehn_Flags = MUI_EHF_HANDLEINPUT;
    data->mad_hiehn.ehn_Object = obj;
    data->mad_hiehn.ehn_Class = 0;

    return (IPTR)obj;
}

/* Free the owned duplicated background specification, then return Notify
 * disposal result. Caller must first end show/setup, timer registration and
 * callbacks; this handler does not perform Hide or Cleanup and does not
 * dispose borrowed context-menu objects or strings.
 */
static IPTR Area__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    /* Safe to call this with NULL */
    zune_image_spec_free(data->mad_BackgroundSpec);

    return DoSuperMethodA(cl, obj, msg);
}

/* Apply supported writable tags in order, managing copied background
 * resources, flags and requested state. Rewrite unchanged
 * Background/Selected tags to TAG_IGNORE. On combined disabled-state
 * transitions cancel a press and redraw, then forward tags to Notify and
 * return its result. Resource and callback failures are not aggregated;
 * caller must preserve object/application lifetime through nested
 * notifications.
 */
static IPTR Area__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;
    CONST_STRPTR old_backgroundspec;

    int change_disable = 0; /* Has the disable state changed? */

    while ((tag = NextTagItem(&tags)) != NULL) {
        switch (tag->ti_Tag) {
        case MUIA_Background:
            old_backgroundspec = data->mad_BackgroundSpec;
            data->mad_BackgroundSpec = zune_image_spec_duplicate(tag->ti_Data);
            if (!data->mad_BackgroundSpec) {
                /* Out of memory */
                data->mad_BackgroundSpec = old_backgroundspec;
                break;
            }

            data->mad_Flags |= MADF_OWNBG;

            if (old_backgroundspec &&
                    (strcmp(data->mad_BackgroundSpec, old_backgroundspec) == 0)) {
                /* New background does not differ from old one */
                zune_image_spec_free(old_backgroundspec);
                tag->ti_Tag = TAG_IGNORE;
                break;
            }

            zune_image_spec_free(old_backgroundspec);

            if (data->mad_Background) {
                if (_flags(obj) & MADF_CANDRAW) {
                    zune_imspec_hide(data->mad_Background);
                }
                if (_flags(obj) & MADF_SETUP) {
                    zune_imspec_cleanup(data->mad_Background);
                    data->mad_Background = NULL;
                }
            }

            if (_flags(obj) & MADF_SETUP) {
                data->mad_Background = zune_imspec_setup((IPTR)data->mad_BackgroundSpec,
                                       muiRenderInfo(obj));
            }
            if (_flags(obj) & MADF_CANDRAW) {
                zune_imspec_show(data->mad_Background, obj);
            }
            MUI_Redraw(obj, MADF_DRAWOBJECT);
            break;

        case MUIA_FillArea:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_FILLAREA);
            break;

        case MUIA_Frame:
            /* this is not documented in MUI but it is possible,
               and needed to suppress frame for external images */
            data->mad_Frame = tag->ti_Data;
            if (muiGlobalInfo(obj)) {
                set_inner_sizes(obj, data);
                set_title_sizes(obj, data);
            }
            break;

        case MUIA_ControlChar:
            if (_flags(obj) & MADF_SETUP)
                cleanup_control_char(data, obj);
            data->mad_ControlChar = tag->ti_Data;
            if (_flags(obj) & MADF_SETUP)
                setup_control_char(data, obj, cl);
            break;

        case MUIA_CycleChain:
            //          if (data->mad_InputMode == MUIV_InputMode_None)
            //              break;

            if ((!(_flags(obj) & MADF_CYCLECHAIN) && tag->ti_Data) ||
                    ((_flags(obj) & MADF_CYCLECHAIN) && !tag->ti_Data)) {
                if (_flags(obj) & MADF_SETUP) {
                    cleanup_control_char(data, obj);
                    _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
                    setup_control_char(data, obj, cl);
                } else
                    _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
            }
            break;

        case MUIA_Disabled:
            if (tag->ti_Data) {
                if (data->mad_DisableCount == 0)
                    change_disable = 1;
                data->mad_DisableCount |= (1 << 31);
            } else {
                int change_val = (data->mad_DisableCount != 0) ? 1 : 0;
                data->mad_DisableCount &= ~(1 << 31);
                if (data->mad_DisableCount == 0)
                    change_disable = change_val;
            }
            break;

        case MUIA_NestedDisabled:
            if (tag->ti_Data) {
                ULONG count = data->mad_DisableCount & ~(1 << 31);
                if (data->mad_DisableCount == 0)
                    change_disable = 1;
                count += 1;
                data->mad_DisableCount =
                    (data->mad_DisableCount & (1 << 31)) | (count & ~(1 << 31));
            } else {
                int change_val = (data->mad_DisableCount != 0) ? 1 : 0;
                ULONG count = data->mad_DisableCount & ~(1 << 31);
                if (count > 0) {
                    count -= 1;
                    data->mad_DisableCount =
                        (data->mad_DisableCount & (1 << 31)) | (count & ~(1 << 31));
                }
                if (data->mad_DisableCount == 0)
                    change_disable = change_val;
            }
            break;

        case MUIA_HorizWeight:
            data->mad_HorizWeight = tag->ti_Data;
            break;

        case MUIA_Pressed:
            if (tag->ti_Data)
                data->mad_Flags |= MADF_PRESSED;
            else
                data->mad_Flags &= ~MADF_PRESSED;
            break;

        case MUIA_ShortHelp:
            data->mad_ShortHelp = (STRPTR)tag->ti_Data;
            break;

        case MUIA_ShowMe: {
            ULONG oldflags = data->mad_Flags;
            int recalc = 0;

            if (tag->ti_Data)
                data->mad_Flags |= MADF_SHOWME;
            else
                data->mad_Flags &= ~MADF_SHOWME;

            if (oldflags != data->mad_Flags) {
                if (!tag->ti_Data) {
                    /* Hide a drawable object. ShowMe does not end setup; the
                     * cleanup branch below is disabled. */
                    if (_flags(obj) & MADF_CANDRAW) {
                        DoHideMethod(obj);
                        recalc = 1;
                    }
#if 0 /* SHOWME affects only show/hide */
                    if (_flags(obj) & MADF_SETUP)
                        DoMethod(obj, MUIM_Cleanup);
#endif
                } else {
                    Object *parent = _parent(obj);
                    /* Will be NULL if direct child of a window! */

                    if (parent) {
#if 0 /* SHOWME affects only show/hide */
                        if (_flags(parent) & MADF_SETUP)
                            DoSetupMethod(obj, muiRenderInfo(parent));
#endif
                        if (_flags(parent) & MADF_CANDRAW) {
                            DoShowMethod(obj);
                            recalc = 1;
                        }
                    } else {
                        /* Check if window is open... */
                    }
                }

                /* If renderinfo is NULL _win(obj) doesn't work (crash) */
                if (recalc && muiRenderInfo(obj)) {
                    DoMethod(_win(obj), MUIM_Window_RecalcDisplay, (IPTR)_parent(obj));
                }
            }
        }
        break;

        case MUIA_Selected:
            /*  D(bug(" Area_Set(%p) :
               MUIA_Selected val=%ld sss=%d\n", obj, tag->ti_Data,
                   !!(data->mad_Flags & MADF_SHOWSELSTATE)));
             */
            if (tag->ti_Data && !(data->mad_Flags & MADF_SELECTED)) {
                data->mad_Flags |= MADF_SELECTED;
                MUI_Redraw(obj, MADF_DRAWOBJECT);
            } else if (!tag->ti_Data && (data->mad_Flags & MADF_SELECTED)) {
                data->mad_Flags &= ~MADF_SELECTED;
                MUI_Redraw(obj, MADF_DRAWOBJECT);
            } else {
                /* Attribute value not changed, don't fire notification */
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        case MUIA_Timer:
            data->mad_Timeval = tag->ti_Data;
            break;

        case MUIA_VertWeight:
            data->mad_VertWeight = tag->ti_Data;
            break;

        case MUIA_Draggable:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DRAGGABLE);
            break;

        case MUIA_Dropable:
            _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DROPABLE);
            break;

        case MUIA_ContextMenu:
            data->mad_ContextMenu = (Object *)tag->ti_Data;
            break;

        case MUIA_Font:
            data->mad_FontPreset = tag->ti_Data;
            break;

        case MUIA_InputMode:
            data->mad_InputMode = tag->ti_Data;
            break;
        }
    }

    if (change_disable) {
        /* Simulate left mouse button release if the area becomes disabled */
        if (_flags(obj) & MADF_SETUP)
            handle_release(cl, obj, TRUE);
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Write recognized geometry, flags, borrowed pointers or stored requests
 * through writable opg_Storage; otherwise delegate to Notify. Font returns
 * the preset, Background the private prepared image object. Getters do not
 * force layout or setup, and declared unsupported attributes gain no
 * behavior here.
 */
static IPTR Area__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_AreaData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID) {
    case MUIA_Background:
        STORE = (IPTR)data->mad_Background;
        return TRUE;

    case MUIA_BottomEdge:
        STORE = (IPTR)_bottom(obj);
        return TRUE;

    case MUIA_ControlChar:
        STORE = data->mad_ControlChar;
        return TRUE;

    case MUIA_CycleChain:
        STORE = ((data->mad_Flags & MADF_CYCLECHAIN) != 0);
        return TRUE;

    case MUIA_Disabled:
        STORE = !!data->mad_DisableCount; /* BOOLEAN */
        return TRUE;

    case MUIA_NestedDisabled:
        STORE =
            ((data->mad_DisableCount & ~(1 << 31)) != 0) ? (IPTR)TRUE : (IPTR)FALSE;
        return TRUE;

    case MUIA_Font:
        STORE = (IPTR)data->mad_FontPreset;
        return TRUE;

    case MUIA_Height:
        STORE = (IPTR)_height(obj);
        return TRUE;

    case MUIA_HorizWeight:
        STORE = (IPTR)data->mad_HorizWeight;
        return TRUE;

    case MUIA_InnerBottom:
        STORE = (IPTR)data->mad_InnerBottom;
        return TRUE;

    case MUIA_InnerLeft:
        STORE = (IPTR)data->mad_InnerLeft;
        return TRUE;

    case MUIA_InnerRight:
        STORE = (IPTR)data->mad_InnerRight;
        return TRUE;

    case MUIA_InnerTop:
        STORE = (IPTR)data->mad_InnerTop;
        return TRUE;

    case MUIA_LeftEdge:
        STORE = (IPTR)_left(obj);
        return TRUE;

    case MUIA_Pressed:
        STORE = !!(data->mad_Flags & MADF_PRESSED);
        return TRUE;

    case MUIA_RightEdge:
        STORE = (IPTR)_right(obj);
        return TRUE;

    case MUIA_Selected:
        STORE = !!(data->mad_Flags & MADF_SELECTED);
        return TRUE;

    case MUIA_ShortHelp:
        STORE = (IPTR)data->mad_ShortHelp;
        return TRUE;

    case MUIA_ShowMe:
        STORE = !!(data->mad_Flags & MADF_SHOWME);
        return TRUE;

    case MUIA_Timer:
        STORE = data->mad_Timeval;
        return TRUE;

    case MUIA_TopEdge:
        STORE = (IPTR)_top(obj);
        return TRUE;

    case MUIA_VertWeight:
        STORE = (IPTR)data->mad_VertWeight;
        return TRUE;

    case MUIA_Width:
        STORE = (IPTR)_width(obj);
        return TRUE;

    case MUIA_Window:
        if (muiAreaData(obj)->mad_RenderInfo)
            STORE = (IPTR)_window(obj);
        else
            STORE = 0L;
        return TRUE;

    case MUIA_WindowObject:
        if (muiAreaData(obj)->mad_RenderInfo)
            STORE = (IPTR)_win(obj);
        else
            STORE = 0L;
        return TRUE;

    case MUIA_ContextMenu:
        STORE = (IPTR)data->mad_ContextMenu;
        return TRUE;

    case MUIA_Frame:
        STORE = (IPTR)data->mad_Frame;
        return TRUE;

    case MUIA_Draggable:
        STORE = !!(data->mad_Flags & MADF_DRAGGABLE);
        return TRUE;

    case MUIA_Dropable:
        STORE = !!(data->mad_Flags & MADF_DROPABLE);
        return TRUE;
    }

    return (DoSuperMethodA(cl, obj, (Msg)msg));
#undef STORE
}

/****** Area.mui/MUIM_AskMinMax ************
*
*   NAME
*       MUIM_AskMinMax
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_AskMinMax, (IPTR)MinMaxInfo);
*
*   FUNCTION
*       Recompute frame/title/inner offsets and overwrite all six
*       min/max/default fields with the frame-and-spacing contribution.
*
*   INPUTS
*       MinMaxInfo - Non-NULL writable MUI_MinMax. Requires valid setup
*       environment and frame preferences.
*
*   RESULT
*       Always TRUE.
*
*   NOTES
*       Custom classes call the superclass and add their content requirements.
*       Area itself has no flexible content dimensions. Fixed/text/maximum
*       constraints are applied afterward by the private finalization helper
*       used by layout callers; this method alone does not apply them.
*
*/
static IPTR Area__MUIM_AskMinMax(struct IClass *cl, Object *obj,
                                 struct MUIP_AskMinMax *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    const struct MUI_FrameSpec_intern *frame;
    struct MUI_FrameSpec_intern tempframe;

    frame = get_intframe(obj, data, &tempframe);
    zframe = zune_zframe_get(obj, frame);

    area_update_msizes(obj, data, frame, zframe);

    msg->MinMaxInfo->MinWidth = _subwidth(obj);
    msg->MinMaxInfo->MinHeight = _subheight(obj);

    msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
    msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;

    /*   D(bug("Area_AskMinMax 0x%p (%s): Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n",
             obj, data->mad_FrameTitle, */
    /*        msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight, */
    /*        msg->MinMaxInfo->MaxWidth, msg->MinMaxInfo->MaxHeight, */
    /*        msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->DefHeight)); */

    return TRUE;
}

/*
 * Called after MUIM_AskMinMax, to handle fixed and max sizes.
 */
/* After a subclass AskMinMax, apply positive fixed, text-measured or
 * maximum constraints in that precedence order independently per dimension,
 * then store all six final fields in Area. Borrow the live setup object and
 * writable MinMaxInfo. Temporary text is owned and destroyed here;
 * allocation failure leaves that text-constrained dimension unchanged
 * without retrying the maximum branch. Return no status.
 */
void __area_finish_minmax(Object *obj, struct MUI_MinMax *MinMaxInfo)
{
    struct MUI_AreaData *data = muiAreaData(obj);

    if ((_flags(obj) & MADF_FIXHEIGHT) && (data->mad_HardHeight > 0)) {
        int h = data->mad_HardHeight + data->mad_subheight;

        MinMaxInfo->MinHeight = MinMaxInfo->DefHeight = MinMaxInfo->MaxHeight =
                                    CLAMP(h, MinMaxInfo->MinHeight, MinMaxInfo->MaxHeight);
    } else if (data->mad_HardHeightTxt) {
        ZText *text;

        if ((text =
                    zune_text_new(NULL, data->mad_HardHeightTxt, ZTEXT_ARG_NONE, 0))) {
            zune_text_get_bounds(text, obj);

            MinMaxInfo->MinHeight = MinMaxInfo->DefHeight = MinMaxInfo->MaxHeight =
                                        CLAMP(text->height + data->mad_subheight, MinMaxInfo->MinHeight,
                                              MinMaxInfo->MaxHeight);

            zune_text_destroy(text);
        }

    } else if (_flags(obj) & MADF_MAXHEIGHT) {
        MinMaxInfo->MaxHeight = CLAMP(data->mad_HardHeight + data->mad_subheight,
                                      MinMaxInfo->MinHeight, MinMaxInfo->MaxHeight);
    }

    if ((_flags(obj) & MADF_FIXWIDTH) && (data->mad_HardWidth > 0)) {
        int w = data->mad_HardWidth + data->mad_subwidth;

        MinMaxInfo->MinWidth = MinMaxInfo->DefWidth = MinMaxInfo->MaxWidth =
                                   CLAMP(w, MinMaxInfo->MinWidth, MinMaxInfo->MaxWidth);
    } else if (data->mad_HardWidthTxt) {
        ZText *text;

        if ((text =
                    zune_text_new(NULL, data->mad_HardWidthTxt, ZTEXT_ARG_NONE, 0))) {
            zune_text_get_bounds(text, obj);

            MinMaxInfo->MinWidth = MinMaxInfo->DefWidth = MinMaxInfo->MaxWidth =
                                       CLAMP(text->width + data->mad_subwidth, MinMaxInfo->MinWidth,
                                             MinMaxInfo->MaxWidth);

            zune_text_destroy(text);
        }

    } else if (_flags(obj) & MADF_MAXWIDTH) {
        MinMaxInfo->MaxWidth = CLAMP(data->mad_HardWidth + data->mad_subwidth,
                                     MinMaxInfo->MinWidth, MinMaxInfo->MaxWidth);
    }

    /* Set minmax */
    _minwidth(obj) = MinMaxInfo->MinWidth;
    _minheight(obj) = MinMaxInfo->MinHeight;
    _maxwidth(obj) = MinMaxInfo->MaxWidth;
    _maxheight(obj) = MinMaxInfo->MaxHeight;
    _defwidth(obj) = MinMaxInfo->DefWidth;
    _defheight(obj) = MinMaxInfo->DefHeight;
}

/*                 <-- _top(obj) (frame title position depends of _top(obj))
 *  ==== Title ===  <-- frame_top (depends of title, if centered/above)
 * |              | <-- bgtop (depends of frame, bg always begins under frame)
 * |              |
 * |              |
 *  ==============  <-- "bgbottom" and "frame_bottom" (frame overwrites bg (1))
 *
 * (1) : needed for phantom frame objects, where no frame overwrites bg, thus bg
 * must go as far as theoretical bottom frame border.
 */

/*
 * draw object background if MADF_FILLAREA.
 */
/* Draw the selected or normal background over content or surrounding strips
 * according to FillArea. For rounded frames first draw parent background
 * and try to install a rounded clip; remove an installed region afterward.
 * Requires drawable setup state, valid frame and geometry. Return no
 * status. On AddClipRegion failure this helper calls DisposeRegion although
 * AddClipRegion already freed that region, creating a double-free path.
 */
static void Area_Draw_handle_background(Object *obj, struct MUI_AreaData *data,
                                        ULONG flags,
                                        const struct ZuneFrameGfx *zframe)
{
    struct MUI_ImageSpec_intern *background;
    struct Rectangle r, r2, rects[4];
    int i, bgtop, bgleft, bgw, bgh, numrects;
    struct Region *clipregion = NULL;
    APTR cliphandle = NULL;
    BOOL use_clipping = FALSE;

    if (!(data->mad_Flags & MADF_SELECTED) ||
            !(data->mad_Flags & MADF_SHOWSELSTATE))
        background = data->mad_Background;
    else
        background = data->mad_SelBack;

    /* For rounded frames, draw parent background in corner areas first */
    if (zframe->border_radius > 0) {
        bgtop = data->mad_TitleHeightAbove;
        bgleft = _left(obj);
        bgw = _width(obj);
        bgh = _height(obj) - bgtop;
        bgtop += _top(obj);

        /* Draw parent background for the entire area first - this fills the corners
         * that will be outside the rounded frame */
        DoMethod(obj, MUIM_DrawParentBackground, bgleft, bgtop, bgw, bgh, bgleft,
                 bgtop, flags);
    } else {
        /* For non-rounded frames, use traditional inner area calculation */
        if (zframe->customframe)
            bgtop = data->mad_TitleHeightAbove;
        else
            bgtop = data->mad_TitleHeightAbove + zframe->itop;

        bgleft = _left(obj);
        bgw = _width(obj);
        bgh = _height(obj) - bgtop;
        bgtop += _top(obj);
    }

    r.MinX = bgleft;
    r.MinY = bgtop;
    r.MaxX = bgleft + bgw - 1;
    r.MaxY = bgtop + bgh - 1;

    /* For rounded frames, set up clipping to draw object background only inside
     * rounded area */
    if (zframe->border_radius > 0) {
        struct MUI_FrameSpec_intern tempframe;
        const struct MUI_FrameSpec_intern *current_frame =
            get_intframe(obj, data, &tempframe);

        // Use the current frame's border_radius, not the zframe default
        ULONG actual_radius = current_frame->border_radius;
        clipregion =
            zune_frame_create_clip_region(bgleft, bgtop, bgw, bgh, actual_radius);
        if (clipregion) {
            cliphandle = MUI_AddClipRegion(data->mad_RenderInfo, clipregion);
            if (cliphandle != (APTR)-1) {
                use_clipping = TRUE;
            } else {
                DisposeRegion(clipregion);
                clipregion = NULL;
            }
        }
    }

    if (data->mad_Flags & MADF_FILLAREA) {
        rects[0] = r;
        numrects = 1;
    } else {
        /* MADF_FILLAREA not set. Only draw frame outside of
           innerbox (_mleft, _mtop, _mright, _mbottom):

           .............
           .***********.
           .*#########*.
           .*#########*.
           .***********.
           .............

           # = innerbox
           * = frame outside of innerbox
           . = object frame
         */

        r2.MinX = _mleft(obj);
        r2.MinY = _mtop(obj);
        r2.MaxX = _mright(obj);
        r2.MaxY = _mbottom(obj);

        numrects = SubtractRectFromRect(&r, &r2, rects);
    }

    for (i = 0; i < numrects; i++) {
        if (!background) {
            /* For rounded frames, we already drew parent background, so draw object
             * background with clipping. For non-rounded frames, this will do the rest
             */
            /* D(bug(" Area_Draw(%p):%ld: MUIM_DrawBackground\n", obj,
                   __LINE__)); */
            if (zframe->border_radius == 0) {
                /* Non-rounded frames: draw background normally */
                DoMethod(obj, MUIM_DrawBackground, rects[i].MinX, rects[i].MinY,
                         rects[i].MaxX - rects[i].MinX + 1,
                         rects[i].MaxY - rects[i].MinY + 1, rects[i].MinX,
                         rects[i].MinY, data->mad_Flags);
            }
            /* For rounded frames with no background image, parent background is
             * already drawn */
        } else {
            /* D(bug(" Area_Draw(%p):%ld: zune_imspec_draw\n", obj,
                   __LINE__)); */
            zune_imspec_draw(background, data->mad_RenderInfo, rects[i].MinX,
                             rects[i].MinY, rects[i].MaxX - rects[i].MinX + 1,
                             rects[i].MaxY - rects[i].MinY + 1, rects[i].MinX,
                             rects[i].MinY, 0);
        }
    }

    /* Remove clipping region if we used one */
    if (use_clipping && cliphandle != (APTR)-1) {
        MUI_RemoveClipRegion(data->mad_RenderInfo, cliphandle);
    }

    if (muiGlobalInfo(obj)->mgi_Prefs->window_redraw ==
            WINDOW_REDRAW_WITHOUT_CLEAR) {
        if (bgtop > _top(obj) && !(flags & MADF_DRAWALL)) {
            /* Fill in the gap produced by the title with the background
             * of the parent object but only if
             * the upper object hasn't drawn it already
             * (which is the case if MADF_DRAWALL is setted) */
            DoMethod(obj, MUIM_DrawParentBackground, bgleft, _top(obj), bgw,
                     bgtop - _top(obj), bgleft, _top(obj), data->mad_Flags);
        }
    }
}

/*
 * draw object frame + title if not MADF_FRAMEPHANTOM.
 */
/* Draw the current frame and borrowed title using live display/font
 * preferences. Temporarily clip around the fitted title, release an
 * installed clip, then paint title effects. Caller suppresses this helper
 * for phantom frames. No status is returned; missing clipping can leave the
 * frame behind the title. AddClipRegion consumes the region even on
 * failure. This helper changes RastPort state.
 */
static void Area_Draw_handle_frame(Object *obj, struct MUI_AreaData *data,
                                   const struct ZuneFrameGfx *zframe)
{
    APTR textdrawclip = (APTR)-1;
    struct Region *region;
    int tx;
    int tw, frame_height, frame_top;
    int addtw;
    struct TextExtent te;
    int nchars;
    int maxtxtwidth;

    /* no frametitle, just draw frame and return */
    if (!data->mad_FrameTitle) {
        struct dt_frame_image temp_frame;
        struct MUI_FrameSpec_intern tempframe;
        const struct MUI_FrameSpec_intern *framespec =
            get_intframe(obj, data, &tempframe);
        struct dt_frame_image *frame_img =
            zune_frame_prepare_for_drawing(zframe, framespec, &temp_frame);
        zframe->draw(frame_img, muiRenderInfo(obj), _left(obj), _top(obj),
                     _width(obj), _height(obj), _left(obj), _top(obj), _width(obj),
                     _height(obj));
        return;
    }

    /* set clipping so that frame is not drawn behind title */

    switch (muiGlobalInfo(obj)->mgi_Prefs->group_title_color) {
    case GROUP_TITLE_COLOR_OUTLINE:
        addtw = 2;
        break;
    case GROUP_TITLE_COLOR_3D:
        addtw = 1;
        break;
    default:
        addtw = 0;
    }

    maxtxtwidth = _width(obj) - zframe->ileft - zframe->iright - 2 * 5 - addtw;

    nchars = TextFit(_rp(obj), data->mad_FrameTitle, strlen(data->mad_FrameTitle),
                     &te, NULL, 1, maxtxtwidth, _font(obj)->tf_YSize);

    tw = te.te_Width + addtw;
    tx = _left(obj) + (_width(obj) - tw) / 2;

    frame_top = _top(obj) + data->mad_TitleHeightAbove;
    frame_height = _height(obj) - frame_top + _top(obj);

    if ((region = NewRegion())) {
        struct Rectangle rect;
        int hspace = _font(obj)->tf_YSize / 8;

        rect.MinX = tx - hspace;
        rect.MinY = _top(obj);
        rect.MaxX = tx + tw - 1 + hspace;
        rect.MaxY = _top(obj) + _font(obj)->tf_YSize - 1;
        // frame is not thick enough anyway
        OrRectRegion(region, &rect);

        rect.MinX = _left(obj);
        rect.MinY = _top(obj);
        rect.MaxX = _right(obj);
        rect.MaxY = _bottom(obj);
        XorRectRegion(region, &rect);

        textdrawclip = MUI_AddClipRegion(muiRenderInfo(obj), region);
    }

    /* Pass our frame_img (either customframe or properly prepared local) to the
     * draw function */
    struct dt_frame_image temp_frame;
    struct MUI_FrameSpec_intern tempframe;
    const struct MUI_FrameSpec_intern *framespec =
        get_intframe(obj, data, &tempframe);
    struct dt_frame_image *frame_img =
        zune_frame_prepare_for_drawing(zframe, framespec, &temp_frame);
    zframe->draw(frame_img, muiRenderInfo(obj), _left(obj), frame_top,
                 _width(obj), frame_height, _left(obj), frame_top, _width(obj),
                 frame_height);

    if (region && textdrawclip != (APTR)-1) {
        MUI_RemoveClipRegion(muiRenderInfo(obj), textdrawclip);
        /*              DisposeRegion(region);*/ /* sba: DisposeRegion happens
                                                    in MUI_RemoveClipRegion,
                                                    this seems wrong to me */
    }

    /* Title text drawing */
    SetDrMd(_rp(obj), JAM1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color ==
            GROUP_TITLE_COLOR_3D) {
        Move(_rp(obj), tx + 1, _top(obj) + _font(obj)->tf_Baseline + 1);
        Text(_rp(obj), data->mad_FrameTitle, nchars);
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline);
        Text(_rp(obj), data->mad_FrameTitle, nchars);
    } else if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color ==
               GROUP_TITLE_COLOR_OUTLINE) {
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);

        tx += addtw / 2;
        Move(_rp(obj), tx + 1, _top(obj) + _font(obj)->tf_Baseline);
        Text(_rp(obj), data->mad_FrameTitle, nchars);
        Move(_rp(obj), tx - 1, _top(obj) + _font(obj)->tf_Baseline);
        Text(_rp(obj), data->mad_FrameTitle, nchars);
        Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline + 1);
        Text(_rp(obj), data->mad_FrameTitle, nchars);
        Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline - 1);
        Text(_rp(obj), data->mad_FrameTitle, nchars);

        SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
        Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline);
        Text(_rp(obj), data->mad_FrameTitle, nchars);
    } else {
        if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color ==
                GROUP_TITLE_COLOR_HILITE) {
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        }
        Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline);
        Text(_rp(obj), data->mad_FrameTitle, nchars);
    }
}

/****** Area.mui/MUIM_Draw ************
*
*   NAME
*       MUIM_Draw
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Draw, flags);
*
*   FUNCTION
*       Consume the instance drawing flags, promote DRAWALL to DRAWOBJECT, write
*       the resulting flags into the method message and clear the stored drawing
*       flags. If DRAWOBJECT is set, draw background, frame and title.
*
*   INPUTS
*       flags - Writable message field; its incoming value is replaced with the
*       instance flags established by MUI_Redraw. Requires drawable setup state,
*       a valid RastPort and layout.
*
*   RESULT
*       Zero when DRAWOBJECT is absent; TRUE after background/frame processing.
*
*   NOTES
*       Custom subclasses call the superclass and inspect the updated message
*       flags to draw content. DRAWUPDATE alone leaves background/frame drawing
*       to the subclass. Use MUI_Redraw to initiate drawing; a direct message
*       with flags alone does not establish the instance flags. Drawing can
*       change RastPort state. Area temporarily selects the title font, then
*       restores the object font.
*
*   BUGS
*       There is no CANDRAW guard in this method itself. The rounded-background
*       branch calls DisposeRegion after MUI_AddClipRegion reports failure,
*       although that function already frees the supplied region. This can free
*       the region twice. The return value is not a rendering-success guarantee.
*
*/
static IPTR Area__MUIM_Draw(struct IClass *cl, Object *obj,
                            struct MUIP_Draw *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    struct TextFont *obj_font = NULL;
    ULONG flags = data->mad_Flags & MADF_DRAWFLAGS;

    // APTR areaclip;

    /*      D(bug("Area_Draw(0x%p) %ldx%ldx%ldx%ld\n", obj, _left(obj),
                _top(obj),_right(obj),_bottom(obj))); */
    /*      D(bug(" Area_Draw(%p) msg=0x%08lx flags=0x%08lx\n", obj,
       msg->flags, _flags(obj))); */

    if (flags & MADF_DRAWALL)
        flags |= MADF_DRAWOBJECT;

    msg->flags = flags;
    data->mad_Flags &= ~MADF_DRAWFLAGS;

    if (!(flags & MADF_DRAWOBJECT)) {
        /* dont draw bg/frame, let subclass redraw content only
         **/
        return 0;
    }

    /* Background can't be drawn without knowing anything about frame, thus
     * some calculations are made before background and frame drawing.
     */
    {
        /* on selected state, will get the opposite frame */
        const struct MUI_FrameSpec_intern *frame;
        struct MUI_FrameSpec_intern tempframe;
        int state;

        frame = get_intframe(obj, data, &tempframe);
        state = frame->state;
        if ((data->mad_Flags & MADF_SELECTED) &&
                (data->mad_Flags & MADF_SHOWSELSTATE))
            state ^= 1;

        zframe = zune_zframe_get_with_state(obj, frame, state);

        /* update innersizes as there are frames which have different inner
         * spacings in selected state */
        area_update_msizes(obj, data, frame, zframe);
    }

    /* Background drawing */
    Area_Draw_handle_background(obj, data, msg->flags, zframe);

    obj_font = _font(obj);
    _font(obj) = zune_font_get(obj, MUIV_Font_Title);
    SetFont(_rp(obj), _font(obj));

    /* Frame and frametitle drawing */
    if (!(data->mad_Flags & MADF_FRAMEPHANTOM)) {
        Area_Draw_handle_frame(obj, data, zframe);
    }

    _font(obj) = obj_font;
    SetFont(_rp(obj), _font(obj));

    /*    MUI_RemoveClipping(muiRenderInfo(obj), areaclip);*/

    return TRUE;
}

/****** Area.mui/MUIM_DrawParentBackground ************
*
*   NAME
*       MUIM_DrawParentBackground
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DrawParentBackground, left, top, width,
*       height, xoffset, yoffset, flags);
*
*   FUNCTION
*       Forward the rectangle to the parent's DrawBackground, or to the owning
*       Window background when no parent exists.
*
*   INPUTS
*       left, top - Window coordinates of the requested rectangle.
*       width, height - Rectangle dimensions.
*       xoffset, yoffset - Background alignment coordinates passed to the
*       drawing helper.
*       flags - State flags passed by drawing callers; no ownership is
*       transferred.
*
*   RESULT
*       FALSE when not drawable; otherwise TRUE, ignoring the delegated drawing
*       result.
*
*   NOTES
*       The object must retain its live render environment. This draws
*       background only and does not change parentage or layout.
*
*/
static IPTR
Area__MUIM_DrawParentBackground(struct IClass *cl, Object *obj,
                                struct MUIP_DrawParentBackground *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    Object *parent = NULL;

    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
        return FALSE;

    get(obj, MUIA_Parent, &parent);
    if (parent) {
        DoMethod(parent, MUIM_DrawBackground, msg->left, msg->top, msg->width,
                 msg->height, msg->xoffset, msg->yoffset, msg->flags);
    } else {
        D(bug("Area_DrawParentBackground(%p) : MUIM_Window_DrawBackground\n", obj));
        DoMethod(_win(obj), MUIM_Window_DrawBackground, msg->left, msg->top,
                 msg->width, msg->height, msg->xoffset, msg->yoffset, msg->flags);
    }
    return TRUE;
}

/****** Area.mui/MUIM_DrawBackground ************
*
*   NAME
*       MUIM_DrawBackground
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DrawBackground, left, top, width,
*       height, xoffset, yoffset, flags);
*
*   FUNCTION
*       Draw a prepared background or delegate to DrawParentBackground when none
*       exists. With a custom frame, alpha characteristics control background
*       drawing and the frame is drawn for the requested region.
*
*   INPUTS
*       left, top - Window coordinates of the requested rectangle.
*       width, height - Rectangle dimensions.
*       xoffset, yoffset - Background alignment coordinates passed to the
*       drawing helper.
*       flags - State flags passed by drawing callers; no ownership is
*       transferred.
*
*   RESULT
*       FALSE when not drawable; otherwise TRUE or the delegated result.
*
*   NOTES
*       Requires live show/setup resources and usable geometry.
*       Selected-background choice tests the supplied flags and prepared
*       selected image; custom-frame state also uses the object's current
*       selection. These are not just the public DRAWOBJECT/DRAWUPDATE flags. No
*       image or frame ownership is returned.
*
*/
static IPTR Area__MUIM_DrawBackground(struct IClass *cl, Object *obj,
                                      struct MUIP_DrawBackground *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    struct MUI_ImageSpec_intern *bg;
    LONG state;

    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
        return FALSE;

    if ((msg->flags & MADF_SELECTED) && (msg->flags & MADF_SHOWSELSTATE) &&
            data->mad_SelBack) {
        /*      D(bug("Area_DrawBackground(%p): selected bg\n", obj)); */
        bg = data->mad_SelBack;
        state = IDS_SELECTED;
    } else {
        /*      D(bug("Area_DrawBackground(%p): normal bg\n", obj)); */
        bg = data->mad_Background;
        state = IDS_NORMAL;
    }

    const struct ZuneFrameGfx *zframe;
    const struct MUI_FrameSpec_intern *frame;
    struct MUI_FrameSpec_intern tempframe;

    if (!bg) {
        D(bug("Area_DrawBackground(%p) : MUIM_DrawParentBackground\n", obj));

        return DoMethod(obj, MUIM_DrawParentBackground, msg->left, msg->top,
                        msg->width, msg->height, msg->xoffset, msg->yoffset,
                        msg->flags);
    }
    frame = get_intframe(obj, data, &tempframe);

    int xstate = frame->state;
    if ((data->mad_Flags & MADF_SELECTED) &&
            (data->mad_Flags & MADF_SHOWSELSTATE))
        xstate ^= 1;

    zframe = zune_zframe_get_with_state(obj, frame, xstate);

    if (zframe->customframe == NULL) {
        /*      D(bug("Area_DrawBackground(%p): draw bg\n", obj)); */
        zune_imspec_draw(bg, data->mad_RenderInfo, msg->left, msg->top, msg->width,
                         msg->height, msg->xoffset, msg->yoffset, state);

        //     if (!data->mad_FrameTitle)
        //     {

    } else {
        if (zframe->noalpha == FALSE) {
            zune_imspec_draw(bg, data->mad_RenderInfo, msg->left, msg->top,
                             msg->width, msg->height, msg->xoffset, msg->yoffset,
                             state);
        }
    }

    if (zframe->customframe) {
        zframe->draw(zframe->customframe, muiRenderInfo(obj), _left(obj), _top(obj),
                     _width(obj), _height(obj), msg->left, msg->top, msg->width,
                     msg->height);
    }

    return TRUE;
}

/* Private MUIM_DrawBackgroundBuffered: while drawable, render the normal
 * prepared background to borrowed msg->rp with translated offsets and the
 * object inner bounds. Return FALSE when not drawable, TRUE otherwise; no
 * parent fallback or selected-background choice is made. Require valid
 * render resources and writable destination RastPort.
 */
static IPTR
Area__MUIM_DrawBackgroundBuffered(struct IClass *cl, Object *obj,
                                  struct MUIP_DrawBackgroundBuffered *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    struct MUI_ImageSpec_intern *bg;
    LONG state = 0;

    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
        return FALSE;

    bg = data->mad_Background;

    /*      D(bug("Area_DrawBackground(%p): draw bg\n", obj)); */
    zune_imspec_drawbuffered(bg, msg->rp, data->mad_RenderInfo, msg->left,
                             msg->top, msg->width, msg->height,
                             msg->xoffset + msg->left, msg->yoffset + msg->top,
                             state, msg->left, msg->top, 1, _mleft(obj),
                             _mtop(obj), _mright(obj), _mbottom(obj));

    return TRUE;
}

/* Perverting the EventHandlerNode structure to specify a shortcut.
 */
/* Register the embedded borrowed control-character node with Window when a
 * character or cycle flag exists. Map RelVerify/Immediate to PRESS and
 * Toggle to TOGGLE; None leaves the action field unchanged. Requires live
 * Window and stable registration state. Window may allocate a cycle node;
 * failure is ignored. Return no status.
 */
static void setup_control_char(struct MUI_AreaData *data, Object *obj,
                               struct IClass *cl)
{
    if (data->mad_ControlChar != 0 || data->mad_Flags & MADF_CYCLECHAIN) {
        data->mad_ccn.ehn_Events = data->mad_ControlChar;
        switch (data->mad_InputMode) {
        case MUIV_InputMode_RelVerify:
            data->mad_ccn.ehn_Flags = MUIKEY_PRESS;
            break;
        case MUIV_InputMode_Toggle:
            data->mad_ccn.ehn_Flags = MUIKEY_TOGGLE;
            break;
        case MUIV_InputMode_Immediate:
            data->mad_ccn.ehn_Flags = MUIKEY_PRESS;
            break;
        }
        data->mad_ccn.ehn_Priority = 0;
        data->mad_ccn.ehn_Object = obj;
        data->mad_ccn.ehn_Class = cl;
        DoMethod(_win(obj), MUIM_Window_AddControlCharHandler,
                 (IPTR)&data->mad_ccn);
    }
}

/* Remove the embedded control-character/cycle registration when current
 * character or cycle flags request it. Requires the corresponding live
 * Window and existing registration. Do not free the embedded node. No
 * membership validation or result is provided.
 */
static void cleanup_control_char(struct MUI_AreaData *data, Object *obj)
{
    if (data->mad_ControlChar != 0 || data->mad_Flags & MADF_CYCLECHAIN) {
        DoMethod(_win(obj), MUIM_Window_RemControlCharHandler,
                 (IPTR)&data->mad_ccn);
    }
}

/* Resolve the live object frame request using application preferences.
 * Values above 100 are parsed as a borrowed string into writable tempstore;
 * parse failure returns the None preference. Other values directly index
 * the preference array without bounds checking. Return a borrowed
 * preference pointer or tempstore; neither outlives its backing storage.
 */
static const struct MUI_FrameSpec_intern *
get_intframe(Object *obj, struct MUI_AreaData *data,
             struct MUI_FrameSpec_intern *tempstore)
{
    if (data->mad_Frame > 100) {
        /* Frame is spec in string format. Store it in tempstore and return
           a pointer to it. The caller should have such a temp variable on
           the stack. Don't want to add it to MUI_AreaData as that will
           increase mem usage of every object. */

        if (zune_frame_spec_to_intern((CONST_STRPTR)data->mad_Frame, tempstore)) {
            return tempstore;
        } else {
            return &muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_None];
        }
    } else {
        /* Frame is MUIV_Frame_<something> */
        return &muiGlobalInfo(obj)->mgi_Prefs->frames[data->mad_Frame];
    }
}

/* Resolve the frame and copy its inner spacing into components not
 * explicitly initialized by the caller. Requires connected preferences and
 * a valid frame request. Mutate Area instance data without layout, drawing
 * or allocation ownership transfer; return no status.
 */
static void set_inner_sizes(Object *obj, struct MUI_AreaData *data)
{
    const struct MUI_FrameSpec_intern *frame;
    struct MUI_FrameSpec_intern tempframe;

    frame = get_intframe(obj, data, &tempframe);
    // Use frame inner spacing when not hardcoded
    if (!(data->mad_Flags & MADF_INNERLEFT))
        data->mad_InnerLeft = frame->innerLeft;
    if (!(data->mad_Flags & MADF_INNERTOP))
        data->mad_InnerTop = frame->innerTop;
    if (!(data->mad_Flags & MADF_INNERRIGHT))
        data->mad_InnerRight = frame->innerRight;
    if (!(data->mad_Flags & MADF_INNERBOTTOM))
        data->mad_InnerBottom = frame->innerBottom;
}

/* If a borrowed frame-title string exists, resolve frame and title font and
 * store title offsets in BYTE fields from preferences. Requires live
 * RenderInfo and font resources. Sets the object font to the title font and
 * does not restore it here. No status or overflow guarantee is provided.
 */
static void set_title_sizes(Object *obj, struct MUI_AreaData *data)
{
    if (data->mad_FrameTitle) {
        const struct ZuneFrameGfx *zframe;
        const struct MUI_FrameSpec_intern *frame;
        struct MUI_FrameSpec_intern tempframe;

        frame = get_intframe(obj, data, &tempframe);
        zframe = zune_zframe_get(obj, frame);

        _font(obj) = zune_font_get(obj, MUIV_Font_Title);

        switch (muiGlobalInfo(obj)->mgi_Prefs->group_title_position) {
        case GROUP_TITLE_POSITION_ABOVE:
            data->mad_TitleHeightAbove = _font(obj)->tf_Baseline;
            break;
        case GROUP_TITLE_POSITION_CENTERED:
            data->mad_TitleHeightAbove = _font(obj)->tf_YSize / 2;
            break;
        }

        data->mad_TitleHeightAdd =
            _font(obj)->tf_YSize - data->mad_InnerTop - zframe->itop;
        data->mad_TitleHeightBelow =
            data->mad_TitleHeightAdd - data->mad_TitleHeightAbove;
    }
}

/****** Area.mui/MUIM_Setup ************
*
*   NAME
*       MUIM_Setup
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Setup, (IPTR)RenderInfo);
*
*   FUNCTION
*       Store the borrowed render environment, resolve frame spacing/font,
*       prepare backgrounds and register input/shortcut handlers. Mark the
*       object set up and initialize its repeat-handler descriptor.
*
*   INPUTS
*       RenderInfo - Non-NULL live Window render environment with screen,
*       DrawInfo and application preferences available. Call once for each setup
*       interval.
*
*   RESULT
*       Always TRUE in this implementation.
*
*   NOTES
*       This custom-class lifecycle method runs before the native window is
*       shown. The framework wrapper installs RenderInfo before entering
*       subclass Setup. Call the superclass before using its resources; if later
*       subclass setup fails, unwind completed work. No geometry-dependent
*       initialization belongs here.
*
*   BUGS
*       Background/font/handler acquisition is not comprehensively checked. TRUE
*       is not proof that every optional resource exists. Root objects lose
*       nonzero frame and title settings during this method.
*
*/
static IPTR Area__MUIM_Setup(struct IClass *cl, Object *obj,
                             struct MUIP_Setup *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    const struct MUI_FrameSpec_intern *frame;
    struct MUI_FrameSpec_intern tempframe;

    muiRenderInfo(obj) = msg->RenderInfo;

    if (data->mad_Frame) {
        /* no frame allowed for root object (see Area.doc) */
        Object *rootobj = NULL;
        get(_win(obj), MUIA_Window_RootObject, &rootobj);
        if (rootobj == obj) {
            data->mad_Frame = MUIV_Frame_None;
            data->mad_FrameTitle = NULL;
        }
    }

    set_inner_sizes(obj, data);
    set_title_sizes(obj, data);

    frame = get_intframe(obj, data, &tempframe);
    zframe = zune_zframe_get(obj, frame);

    area_update_msizes(obj, data, frame, zframe);

    if (data->mad_Flags & MADF_OWNBG) {
        data->mad_Background =
            zune_imspec_setup((IPTR)data->mad_BackgroundSpec, muiRenderInfo(obj));
    }

    if ((data->mad_Flags & MADF_SHOWSELSTATE) &&
            (data->mad_InputMode != MUIV_InputMode_None)) {
        data->mad_SelBack =
            zune_imspec_setup(MUII_SelectedBack, muiRenderInfo(obj));
    }

    if (data->mad_InputMode != MUIV_InputMode_None || data->mad_ContextMenu) {
        data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
        DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
    }

    /* Those are filled by RequestIDCMP() */
    if (data->mad_hiehn.ehn_Events)
        DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_hiehn);

    setup_control_char(data, obj, cl);
    //    setup_cycle_chain (data, obj);

    if (data->mad_FontPreset == MUIV_Font_Inherit) {
        if (_parent(obj) != NULL && _parent(obj) != _win(obj))
            data->mad_Font = _font(_parent(obj));
        else {
            D(bug("Area_Setup %p: getting normal font\n", obj));
            data->mad_Font = zune_font_get(obj, MUIV_Font_Normal);
            D(bug("Area_Setup %p: got normal font %p\n", obj, data->mad_Font));
        }
    } else {
        data->mad_Font = zune_font_get(obj, data->mad_FontPreset);
    }

    _flags(obj) |= MADF_SETUP;

    data->mad_Timer.ihn_Flags = MUIIHNF_TIMER;
    data->mad_Timer.ihn_Method = MUIM_Timer;
    data->mad_Timer.ihn_Object = obj;

    return TRUE;
}

/****** Area.mui/MUIM_Cleanup ************
*
*   NAME
*       MUIM_Cleanup
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Cleanup);
*
*   FUNCTION
*       Unregister shortcuts, timer and event handlers, clear window focus if
*       this is active, clean prepared backgrounds and clear RenderInfo.
*
*   INPUTS
*       No arguments beyond MethodID. Match successful Setup after Hide and
*       after dependent subclass resources have been released.
*
*   RESULT
*       Always TRUE.
*
*   NOTES
*       The method does not dispose the object, its context-menu object,
*       borrowed strings or supplied fonts. It is not idempotent: registration
*       fields can remain nonzero after removal. Keep the owning
*       Window/application alive throughout cleanup.
*
*   BUGS
*       Selected-background cleanup depends on current InputMode, not on whether
*       it was allocated. Changing that mode during setup can leave a resource
*       unreleased.
*
*/
static IPTR Area__MUIM_Cleanup(struct IClass *cl, Object *obj,
                               struct MUIP_Cleanup *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    _flags(obj) &= ~MADF_SETUP;

    //    cleanup_cycle_chain (data, obj);
    cleanup_control_char(data, obj);

    if (data->mad_Timer.ihn_Millis) {
        DoMethod(_app(obj), MUIM_Application_RemInputHandler,
                 (IPTR)&data->mad_Timer);
        data->mad_Timer.ihn_Millis = 0;
    }

    /* Remove the handler if it is added */
    if (data->mad_hiehn.ehn_Events)
        DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_hiehn);

    /* Remove the event handler if it has been added */
    if (data->mad_ehn.ehn_Events)
        DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);

    D(bug("Area cleanup %p active=%p\n", obj,
          (Object *)XGET(_win(obj), MUIA_Window_ActiveObject)));
    if (obj == (Object *)XGET(_win(obj), MUIA_Window_ActiveObject)) {
        D(bug("we are active, unset us\n"));
        set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
    }

    /* It's safe to call the following function with NULL */
    if ((data->mad_Flags & MADF_SHOWSELSTATE) &&
            (data->mad_InputMode != MUIV_InputMode_None)) {
        zune_imspec_cleanup(data->mad_SelBack);
        data->mad_SelBack = NULL;
    }
    if (data->mad_Flags & MADF_OWNBG) {
        zune_imspec_cleanup(data->mad_Background);
        data->mad_Background = NULL;
    }

    muiRenderInfo(obj) = NULL;

    return TRUE;
}

/****** Area.mui/MUIM_Show ************
*
*   NAME
*       MUIM_Show
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Show);
*
*   FUNCTION
*       Show prepared background images after the native window is open and
*       layout is known.
*
*   INPUTS
*       No arguments beyond MethodID. Requires completed Setup and valid native
*       display resources.
*
*   RESULT
*       Always TRUE.
*
*   NOTES
*       This custom-class lifecycle method does not itself set CANDRAW. The
*       framework wrapper sets that flag after a successful return. Initialize
*       subclass resources that depend on size/position in this phase.
*
*/
static IPTR Area__MUIM_Show(struct IClass *cl, Object *obj,
                            struct MUIP_Show *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    zune_imspec_show(data->mad_Background, obj);

    if (data->mad_Flags & MADF_SHOWSELSTATE &&
            data->mad_InputMode != MUIV_InputMode_None) {
        zune_imspec_show(data->mad_SelBack, obj);
    }

    return TRUE;
}

/****** Area.mui/MUIM_Hide ************
*
*   NAME
*       MUIM_Hide
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Hide);
*
*   FUNCTION
*       Hide prepared backgrounds and close a live native context menu.
*
*   INPUTS
*       No arguments beyond MethodID. Match Show before Cleanup; display
*       resources must still exist.
*
*   RESULT
*       Always TRUE.
*
*   NOTES
*       The framework wrapper clears CANDRAW before calling this method. This
*       method does not clear RenderInfo or free setup resources; Cleanup does
*       that. The borrowed menu object is not disposed.
*
*/
static IPTR Area__MUIM_Hide(struct IClass *cl, Object *obj,
                            struct MUIP_Hide *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    zune_imspec_hide(data->mad_Background);
    if (data->mad_Flags & MADF_SHOWSELSTATE &&
            data->mad_InputMode != MUIV_InputMode_None) {
        zune_imspec_hide(data->mad_SelBack);
    }

    if (data->mad_ContextZMenu) {
        zune_close_menu(data->mad_ContextZMenu);
        data->mad_ContextZMenu = NULL;
    }

    return TRUE;
}

/****** Area.mui/MUIM_GoActive ************
*
*   NAME
*       MUIM_GoActive
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_GoActive);
*
*   FUNCTION
*       Draw the keyboard-focus outline when drawable.
*
*   INPUTS
*       No arguments beyond MethodID. Require valid display state when drawable.
*
*   RESULT
*       Always TRUE, including when no outline is drawn.
*
*   NOTES
*       This changes visual focus only; Window_ActiveObject manages selection of
*       the active object. Root/direct-window objects have no default
*       keyboard-focus outline. Erasing uses parent background drawing.
*
*/
static IPTR Area__MUIM_GoActive(struct IClass *cl, Object *obj, Msg msg)
{
    // bug("Area_GoActive %p\n", obj);
    if (_flags(obj) & MADF_CANDRAW)
        _zune_focus_new(obj, ZUNE_FOCUS_TYPE_ACTIVE_OBJ);
    return TRUE;
}

/****** Area.mui/MUIM_GoInactive ************
*
*   NAME
*       MUIM_GoInactive
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_GoInactive);
*
*   FUNCTION
*       Erase the keyboard-focus outline when drawable.
*
*   INPUTS
*       No arguments beyond MethodID. Require valid display state when drawable.
*
*   RESULT
*       Always TRUE, including when no outline is drawn.
*
*   NOTES
*       This changes visual focus only; Window_ActiveObject manages selection of
*       the active object. Root/direct-window objects have no default
*       keyboard-focus outline. Erasing uses parent background drawing.
*
*/
static IPTR Area__MUIM_GoInactive(struct IClass *cl, Object *obj, Msg msg)
{
    // bug("Area_GoInactive %p\n", obj);
    if (_flags(obj) & MADF_CANDRAW)
        _zune_focus_destroy(obj, ZUNE_FOCUS_TYPE_ACTIVE_OBJ);
    return TRUE;
}

/****** Area.mui/MUIM_CreateShortHelp ************
*
*   NAME
*       MUIM_CreateShortHelp
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_CreateShortHelp, mx, my);
*
*   FUNCTION
*       Return the stored ShortHelp string unchanged.
*
*   INPUTS
*       mx, my - LONG window-relative pointer coordinates; ignored by Area.
*
*   RESULT
*       Borrowed STRPTR or NULL.
*
*   NOTES
*       The base implementation allocates nothing. An overriding implementation
*       may provide text whose lifetime is paired with DeleteShortHelp. Default
*       Window bubble discovery requires a non-NULL ShortHelp attribute before
*       it calls the bubble path.
*
*/
static IPTR Area__MUIM_CreateShortHelp(struct IClass *cl, Object *obj,
                                       struct MUIP_CreateShortHelp *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    return (IPTR)data->mad_ShortHelp;
}

/****** Area.mui/MUIM_DeleteShortHelp ************
*
*   NAME
*       MUIM_DeleteShortHelp
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DeleteShortHelp, (IPTR)help);
*
*   FUNCTION
*       Perform no operation for Area's borrowed ShortHelp string.
*
*   INPUTS
*       help - Text returned by CreateShortHelp; unused here.
*
*   RESULT
*       Always TRUE.
*
*   NOTES
*       Override together with CreateShortHelp if the custom implementation
*       allocates text. Do not free caller-owned text merely because the base
*       deletion method was invoked.
*
*/
static IPTR Area__MUIM_DeleteShortHelp(struct IClass *cl, Object *obj,
                                       struct MUIP_DeleteShortHelp *msg)
{
    return TRUE;
}

/****** Area.mui/MUIM_CreateBubble ************
*
*   NAME
*       MUIM_CreateBubble
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_CreateBubble, x, y, (IPTR)txt, flags);
*
*   FUNCTION
*       Create and display a nonactivating borderless help window on the
*       object's screen. NULL text requests CreateShortHelp; other text is
*       borrowed directly.
*
*   INPUTS
*       x, y - LONG coordinates relative to the native window origin. Intuition
*       may auto-adjust the bubble position.
*       txt - Readable NUL-terminated string or NULL.
*       flags - ULONG; currently ignored by the bubble engine, including
*       DontHidePointer.
*
*   RESULT
*       Opaque bubble handle, or NULL when not drawable, text is absent, or a
*       checked allocation/opening step fails.
*
*   NOTES
*       Keep the source object, display environment and text alive until
*       DeleteBubble. Delete using the same source object. If CreateShortHelp
*       supplied text, deletion calls DeleteShortHelp; explicit text is not
*       freed. Later creation failures unwind acquired resources and can call
*       DeleteShortHelp before returning NULL.
*
*/
static IPTR Area__MUIM_CreateBubble(struct IClass *cl, Object *obj,
                                    struct MUIP_CreateBubble *msg)
{
    return (IPTR)zune_bubble_create(obj, msg->x, msg->y, msg->txt, msg->flags);
}

/****** Area.mui/MUIM_DeleteBubble ************
*
*   NAME
*       MUIM_DeleteBubble
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DeleteBubble, (IPTR)bubble);
*
*   FUNCTION
*       Close and release the bubble window, shape region and parsed text, then
*       release its handle. Call DeleteShortHelp only when creation obtained
*       text through that method.
*
*   INPUTS
*       bubble - Handle returned by CreateBubble on this object, or NULL.
*
*   RESULT
*       Always TRUE; NULL is accepted.
*
*   NOTES
*       A non-NULL handle must be deleted exactly once. Explicitly supplied text
*       remains caller-owned. Keep the source object alive for a possible
*       DeleteShortHelp callback.
*
*/
static IPTR Area__MUIM_DeleteBubble(struct IClass *cl, Object *obj,
                                    struct MUIP_DeleteBubble *msg)
{
    zune_bubble_delete(obj, msg->bubble);

    return TRUE;
}

/* either lmb or press key */
/* Apply the current InputMode synchronously: RelVerify sets
 * Selected/Pressed and Timer and registers a 300 ms repeat when enabled;
 * Immediate forces a fresh Selected notification; Toggle flips Selected and
 * Pressed. Requires live setup/application state and stable object lifetime
 * through callbacks. Registration results are ignored; return no status.
 */
static void handle_press(struct IClass *cl, Object *obj)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    switch (data->mad_InputMode) {
    case MUIV_InputMode_RelVerify:
        SetAttrs(obj, MUIA_Selected, TRUE, MUIA_Pressed, TRUE, TAG_DONE);
        set(obj, MUIA_Timer, ++muiAreaData(obj)->mad_Timeval);
        if (!data->mad_Timer.ihn_Millis && data->mad_DisableCount == 0) {
            data->mad_Timer.ihn_Millis = 300;
            DoMethod(_app(obj), MUIM_Application_AddInputHandler,
                     (IPTR)&data->mad_Timer);
        }
        break;

    case MUIV_InputMode_Immediate: {
        IPTR selected = FALSE;

        get(obj, MUIA_Selected, &selected);
        if (selected) {
            /*              D(bug("handle_press(%p) : nnset MUIA_Selected
             * FALSE\n", obj)); */
            nnset(obj, MUIA_Selected, FALSE);
        }
        /*          D(bug("handle_press(%p) : set MUIA_Selected TRUE\n", obj));
         */
        set(obj, MUIA_Selected, TRUE);
        /*          D(bug("handle_press(%p) : done\n", obj)); */
        break;
    }
    case MUIV_InputMode_Toggle:
        // although undocumented, MUI sets MUIA_Pressed too
        SetAttrs(obj, MUIA_Selected, !(data->mad_Flags & MADF_SELECTED),
                 MUIA_Pressed, !(data->mad_Flags & MADF_PRESSED), TAG_DONE);
        break;
    }
}

/* either lmb or release key */
/* Restore the embedded event registration to mouse buttons, then for
 * selected RelVerify objects clear Pressed (silently on cancel) and
 * Selected. Stop an active repeat timer. Requires live Window/application;
 * it may register a mouse handler even if none existed. If not selected,
 * Pressed is not cleared. Return no status and keep state alive through
 * notifications.
 */
static void handle_release(struct IClass *cl, Object *obj, int cancel)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if (data->mad_ehn.ehn_Events)
        DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
    data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);

    if (data->mad_InputMode == MUIV_InputMode_RelVerify) {
        if (data->mad_Flags & MADF_SELECTED) {
            if (cancel)
                nnset(obj, MUIA_Pressed, FALSE);
            else
                set(obj, MUIA_Pressed, FALSE);

            set(obj, MUIA_Selected, FALSE);
        }
    }

    if (data->mad_Timer.ihn_Millis) {
        DoMethod(_app(obj), MUIM_Application_RemInputHandler,
                 (IPTR)&data->mad_Timer);
        data->mad_Timer.ihn_Millis = 0;
    }
}

/* Handle borrowed mouse-button events inside inclusive object bounds.
 * Presses may register motion/raw keys, change input state or create a
 * native context menu; release may report an item and close the native
 * menu. Return Eat for consumed press/menu paths, zero otherwise. Never
 * reply/retain imsg; callback objects and menu data must remain valid.
 */
static IPTR event_button(Class *cl, Object *obj, struct IntuiMessage *imsg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    BOOL in = _between(_left(obj), imsg->MouseX, _right(obj)) &&
              _between(_top(obj), imsg->MouseY, _bottom(obj));

    switch (imsg->Code) {
    case SELECTDOWN:
        if (data->mad_InputMode == MUIV_InputMode_None)
            break;

        if (in) {
            //            set(_win(obj), MUIA_Window_ActiveObject, obj);
            data->mad_ClickX = imsg->MouseX;
            data->mad_ClickY = imsg->MouseY;

            if ((data->mad_InputMode != MUIV_InputMode_Toggle) &&
                    (data->mad_Flags & MADF_SELECTED))
                break;
            nnset(obj, MUIA_Timer, 0);
            if (data->mad_InputMode == MUIV_InputMode_RelVerify) {
                if (data->mad_ehn.ehn_Events)
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                             (IPTR)&data->mad_ehn);
                data->mad_ehn.ehn_Events |= IDCMP_MOUSEMOVE | IDCMP_RAWKEY;
                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
            }
            handle_press(cl, obj);
            return MUI_EventHandlerRC_Eat;
        }
        break;

    case SELECTUP:
        if (data->mad_InputMode == MUIV_InputMode_None)
            break;

        if (data->mad_ehn.ehn_Events != IDCMP_MOUSEBUTTONS) {
            handle_release(cl, obj, FALSE /* cancel */);
            return 0;
        }
        break;

    case MENUDOWN:
        if (in && data->mad_ContextMenu) {
            Object *menuobj = (Object *)DoMethod(obj, MUIM_ContextMenuBuild,
                                                 imsg->MouseX, imsg->MouseY);
            if (menuobj) {
                struct NewMenu *newmenu = NULL;

                /* stegerg: HACKME, CHECKME! The menu/menu item objs should
                   automatically be connected (parentobject setup) without
                   need for this, but they aren't. Because of how family
                   class is and is used by other classes, I think!? */
                DoMethod(menuobj, MUIM_ConnectParent, obj);

                get(menuobj, MUIA_Menuitem_NewMenu, &newmenu);
                if (newmenu) {
                    if (data->mad_ContextZMenu)
                        zune_close_menu(data->mad_ContextZMenu);
                    data->mad_ContextZMenu = zune_open_menu(_window(obj), newmenu);
                }

                if (data->mad_ehn.ehn_Events)
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                             (IPTR)&data->mad_ehn);
                data->mad_ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
            }
            return MUI_EventHandlerRC_Eat;
        }
        break;

    case MENUUP:
        if (data->mad_ContextZMenu) {
            struct MenuItem *item = zune_leave_menu(data->mad_ContextZMenu);

            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
            data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);

            if (item) {
                Object *itemobj = (Object *)GTMENUITEM_USERDATA(item);

                /* CHECKME: MUIA_MenuItem_Trigger should probably be set inside
                   MUIM_ContextMenuChoice!? But there I only know about
                   itemobj, not MenuItem itself! */
                if (item->Flags & CHECKIT) {
                    set(itemobj, MUIA_Menuitem_Checked, !!(item->Flags & CHECKED));
                }

                set(itemobj, MUIA_Menuitem_Trigger, item);

                DoMethod(obj, MUIM_ContextMenuChoice, itemobj);
            }

            zune_close_menu(data->mad_ContextZMenu);
            data->mad_ContextZMenu = NULL;

            return MUI_EventHandlerRC_Eat;
        }
        break;
    }

    return 0;
}

/* Update a native context menu while the right button is held, or process
 * RelVerify selection and a three-pixel drag threshold while the left
 * button is held. Requires live set-up display and borrowed message; may
 * change handler/timer registration and notify synchronously. Always return
 * Eat, including no-action paths. Do not reply or retain imsg.
 */
static IPTR event_motion(Class *cl, Object *obj, struct IntuiMessage *imsg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if ((imsg->Qualifier & IEQUALIFIER_RBUTTON) && data->mad_ContextZMenu) {
        zune_mouse_update(data->mad_ContextZMenu, 0);
        return MUI_EventHandlerRC_Eat;
    }

    if (imsg->Qualifier & IEQUALIFIER_LEFTBUTTON) {
        BOOL in = _between(_left(obj), imsg->MouseX, _right(obj)) &&
                  _between(_top(obj), imsg->MouseY, _bottom(obj));

        if (in) {
            if ((data->mad_Flags & MADF_DRAGGABLE) &&
                    ((abs(data->mad_ClickX - imsg->MouseX) >= 3) ||
                     (abs(data->mad_ClickY - imsg->MouseY) >= 3)))
                /* should be user configurable */
            {
                if (data->mad_InputMode == MUIV_InputMode_RelVerify)
                    set(obj, MUIA_Selected, FALSE);
                nnset(obj, MUIA_Pressed, FALSE);

                if (data->mad_ehn.ehn_Events)
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                             (IPTR)&data->mad_ehn);
                data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
                if (data->mad_Timer.ihn_Millis) {
                    DoMethod(_app(obj), MUIM_Application_RemInputHandler,
                             (IPTR)&data->mad_Timer);
                    data->mad_Timer.ihn_Millis = 0;
                }

                DoMethod(obj, MUIM_DoDrag, data->mad_ClickX - _left(obj),
                         data->mad_ClickY - _top(obj), 0);
                return MUI_EventHandlerRC_Eat;
            }
        }

        if (data->mad_InputMode == MUIV_InputMode_RelVerify) {
            if (!in && (data->mad_Flags & MADF_SELECTED)) { /* going out */
                set(obj, MUIA_Selected, FALSE);
            } else if (in && !(data->mad_Flags & MADF_SELECTED)) { /* going in */
                set(obj, MUIA_Selected, TRUE);
            }
        }
    }
    return MUI_EventHandlerRC_Eat;
}

/****** Area.mui/MUIM_HandleEvent ************
*
*   NAME
*       MUIM_HandleEvent
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_HandleEvent, (IPTR)imsg, muikey);
*
*   FUNCTION
*       Process input-mode keys, mouse presses/motion, context menus and
*       matching raw control characters. Return immediately when disabled, or
*       when InputMode is None and no context menu exists.
*
*   INPUTS
*       imsg - Borrowed writable IntuiMessage or NULL for MUI-key delivery.
*       muikey - LONG MUIKEY value; non-NONE values take precedence over imsg.
*       Requires a live set-up object and owning Window/application.
*
*   RESULT
*       MUI_EventHandlerRC_Eat when consumed by the selected path; zero
*       otherwise.
*
*   NOTES
*       Never reply to or retain imsg. Window usually supplies a temporary copy.
*       The raw-key path temporarily changes Code and restores it; a matched
*       control key rewrites the method message to imsg=NULL and a press/release
*       muikey before recursion. Handlers can synchronously send notifications.
*       The default motion handler returns Eat even when it performs no action.
*       The raw-key path leaves wheel up/down unconsumed but otherwise can
*       consume unrelated keys. Shift can cancel a current press.
*
*   BUGS
*       The built-in handlers remove and re-add their own event node while
*       handling input. This is not a general guarantee that custom callbacks
*       may change arbitrary registrations safely. Keep objects and application
*       state alive across all nested notifications.
*
*/
static IPTR Area__MUIM_HandleEvent(struct IClass *cl, Object *obj,
                                   struct MUIP_HandleEvent *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if (data->mad_DisableCount)
        return 0;
    if (data->mad_InputMode == MUIV_InputMode_None && !data->mad_ContextMenu)
        return 0;

    if (msg->muikey != MUIKEY_NONE) {
        switch (msg->muikey) {
        case MUIKEY_PRESS:
            if (data->mad_Flags & MADF_SELECTED)
                break;
            if (data->mad_ehn.ehn_Events)
                DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
            data->mad_ehn.ehn_Events |= IDCMP_RAWKEY;
            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
            handle_press(cl, obj);
            return MUI_EventHandlerRC_Eat;

        case MUIKEY_TOGGLE:
            if (data->mad_InputMode == MUIV_InputMode_Toggle)
                set(obj, MUIA_Selected, !(data->mad_Flags & MADF_SELECTED));
            return MUI_EventHandlerRC_Eat;

        case MUIKEY_RELEASE:
            handle_release(cl, obj, FALSE /* cancel */);
            return MUI_EventHandlerRC_Eat;
        }
        return 0;
    }

    if (msg->imsg) {
        switch (msg->imsg->Class) {
        case IDCMP_MOUSEBUTTONS:
            return event_button(cl, obj, msg->imsg);
        case IDCMP_MOUSEMOVE:
            return event_motion(cl, obj, msg->imsg);
        case IDCMP_RAWKEY: {
            unsigned char code;
            UWORD msg_code;
            /* Remove the up prefix as convert key does not convert an
             * upkey event */
            msg_code = msg->imsg->Code;
            msg->imsg->Code &= ~IECODE_UP_PREFIX;
            code = ConvertKey(msg->imsg);
            msg->imsg->Code = msg_code;

            if (code != 0 && code == data->mad_ControlChar) {
                if (msg->imsg->Code & IECODE_UP_PREFIX) {
                    msg->muikey = MUIKEY_RELEASE;
                } else {
                    msg->muikey = MUIKEY_PRESS;
                }
                msg->imsg = NULL;
                return Area__MUIM_HandleEvent(cl, obj, msg);
            }

            if (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) {
                handle_release(cl, obj, TRUE /* cancel */);
            }

            /* Do not eat wheel movement events */
            if ((msg->imsg->Code == RAWKEY_NM_WHEEL_UP) ||
                    (msg->imsg->Code == RAWKEY_NM_WHEEL_DOWN))
                return 0;

            return MUI_EventHandlerRC_Eat;
        }
        break;
        }
    }
    return 0;
}

/****** Area.mui/MUIM_HandleInput ************
*
*   NAME
*       MUIM_HandleInput
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_HandleInput, (IPTR)imsg, muikey);
*
*   FUNCTION
*       Provide a legacy input-handler stub. Area performs no action here; its
*       default input behavior is in HandleEvent.
*
*   INPUTS
*       imsg - Borrowed IntuiMessage pointer, unused by this implementation.
*       muikey - LONG key value, unused.
*
*   RESULT
*       Always zero.
*
*   NOTES
*       Custom classes can override this method for MUI_RequestIDCMP delivery.
*       Window's private HandleInput dispatch discards the method result, so
*       returning Eat here does not stop that Window traversal. Prefer the
*       registered HandleEvent model for new handlers.
*
*/
static IPTR Area__MUIM_HandleInput(struct IClass *cl, Object *obj,
                                   struct MUIP_HandleInput *msg)
{
    /* Actually a dummy, but real MUI does handle here the input stuff which
     * Zune has in Area_HandleEvent. For compatibility we should do this too
     */
    // bug("Area_HandleEvent [%p] imsg=%p muikey=%ld\b", obj, msg->imsg,
    //     msg->muikey);
    return 0;
}

/****** Area.mui/MUIM_ContextMenuBuild ************
*
*   NAME
*       MUIM_ContextMenuBuild
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_ContextMenuBuild, mx, my);
*
*   FUNCTION
*       Return the currently stored ContextMenu pointer unchanged.
*
*   INPUTS
*       mx, my - LONG mouse coordinates in the window; ignored by the base
*       implementation.
*
*   RESULT
*       Borrowed menu object or NULL.
*
*   NOTES
*       Custom classes can override this to choose a menu dynamically. Keep the
*       result and its items alive throughout display. The default event path
*       connects it to this Area but does not dispose or later explicitly
*       disconnect it.
*
*/
static IPTR Area__MUIM_ContextMenuBuild(struct IClass *cl, Object *obj,
                                        struct MUIP_ContextMenuBuild *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    return (IPTR)data->mad_ContextMenu; /* a Menustrip object */
}

/**************************************************************************/
/****** Area.mui/MUIM_ContextMenuChoice ************
*
*   NAME
*       MUIM_ContextMenuChoice
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_ContextMenuChoice, (IPTR)item);
*
*   FUNCTION
*       Set ContextMenuTrigger to the selected item for synchronous
*       notification.
*
*   INPUTS
*       item - Borrowed selected menu-item object.
*
*   RESULT
*       Always zero; setter results are ignored.
*
*   NOTES
*       No item pointer is stored by Area and there is no ContextMenuTrigger
*       getter. The native menu is still being processed by the caller; do not
*       destroy involved objects during the callback.
*
*/
static IPTR Area__MUIM_ContextMenuChoice(struct IClass *cl, Object *obj,
        struct MUIP_ContextMenuChoice *msg)
{
    set(obj, MUIA_ContextMenuTrigger, msg->item);
    return 0;
}

/* For nonzero inherited ObjectID, copy a one-byte Selected value into the
 * supplied live Dataspace via Add. Return zero regardless of add success.
 * The temporary byte is stack storage and Dataspace must copy it; no
 * ownership of the dataspace is transferred.
 */
static IPTR Area__MUIM_Export(struct IClass *cl, Object *obj,
                              struct MUIP_Export *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID)) {
        char selected = (data->mad_Flags & MADF_SELECTED) ? 1 : 0;
        DoMethod(msg->dataspace, MUIM_Dataspace_Add, (IPTR)&selected, sizeof(char),
                 (IPTR)id);
    }
    return 0;
}

/* For nonzero ObjectID, find the borrowed Dataspace payload and set
 * Selected from its first byte when non-NULL. Return zero and ignore setter
 * results. Require a valid entry of at least one byte; this implementation
 * does not query its size or retain the payload.
 */
static IPTR Area__MUIM_Import(struct IClass *cl, Object *obj,
                              struct MUIP_Import *msg)
{
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID)) {
        char *selected =
            (char *)DoMethod(msg->dataspace, MUIM_Dataspace_Find, (IPTR)id);

        if (selected)
            set(obj, MUIA_Selected, *selected);
    }
    return 0;
}

/* Private MUIM_Timer: remove an active embedded timer, register it for 50
 * ms and increment the Timer notification counter only when Selected.
 * Require live application/object setup and safe callback lifetime.
 * Registration failures are ignored; return zero. The timer continues while
 * deselected until release/drag/cleanup removes it.
 */
static IPTR Area__MUIM_Timer(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    if (data->mad_Timer.ihn_Millis)
        DoMethod(_app(obj), MUIM_Application_RemInputHandler,
                 (IPTR)&data->mad_Timer);
    data->mad_Timer.ihn_Millis = 50;
    DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->mad_Timer);

    if (data->mad_Flags & MADF_SELECTED)
        set(obj, MUIA_Timer, ++muiAreaData(obj)->mad_Timeval);
    return 0;
}

/****** Area.mui/MUIM_DoDrag ************
*
*   NAME
*       MUIM_DoDrag
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DoDrag, touchx, touchy, flags);
*
*   FUNCTION
*       Request Window's internal drag operation for this object.
*
*   INPUTS
*       touchx, touchy - LONG click offsets from the object's left/top. The
*       value 0x80000000 independently selects current native mouse coordinate
*       minus the corresponding object edge.
*       flags - ULONG forwarded to drag-image creation.
*
*   RESULT
*       Always zero, ignoring whether Window started the drag.
*
*   NOTES
*       Custom-class operation requiring a live open Window and render
*       environment. It does not check Draggable, start a new event loop or
*       provide a completion result. Do not start a second drag while one is
*       active.
*
*/
static IPTR Area__MUIM_DoDrag(struct IClass *cl, Object *obj,
                              struct MUIP_DoDrag *msg)
{
    LONG touchx = msg->touchx;
    LONG touchy = msg->touchy;
    if (touchx == 0x80000000)
        touchx = _window(obj)->MouseX - _left(obj);
    if (touchy == 0x80000000)
        touchy = _window(obj)->MouseY - _top(obj);

    DoMethod(_win(obj), MUIM_Window_DragObject, (IPTR)obj, touchx, touchy,
             msg->flags);
    return 0;
}

/****** Area.mui/MUIM_CreateDragImage ************
*
*   NAME
*       MUIM_CreateDragImage
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_CreateDragImage, touchx, touchy,
*       flags);
*
*   FUNCTION
*       Allocate a MUI_DragImage and bitmap, copy the displayed object pixels,
*       and draw the configured drag frame around them.
*
*   INPUTS
*       touchx, touchy - LONG click offsets, stored in WORD fields without
*       adding drag-frame borders.
*       flags - ULONG input, ignored; returned flags are zero. Requires live
*       display pixels, screen, RenderInfo and frame preferences.
*
*   RESULT
*       Owned MUI_DragImage pointer or NULL when the structure allocation fails.
*       The returned structure can have bm=NULL when bitmap allocation fails.
*
*   NOTES
*       Release with DeleteDragImage on the same source. The image width/height
*       include drag-frame borders and are stored in WORD fields. The method
*       temporarily replaces the shared RenderInfo RastPort while drawing the
*       frame, then restores it.
*
*   BUGS
*       A non-NULL structure is not proof of a usable bitmap. No range/overflow
*       checks are made for dimensions or touch offsets; pointer offsets are not
*       adjusted for the newly added border. The source pixels are copied, not
*       rerendered.
*
*/
static IPTR Area__MUIM_CreateDragImage(struct IClass *cl, Object *obj,
                                       struct MUIP_CreateDragImage *msg)
{
    struct MUI_DragImage *img = (struct MUI_DragImage *)AllocVec(
                                    sizeof(struct MUI_DragImage), MEMF_CLEAR);
    if (img) {
        const struct ZuneFrameGfx *zframe;
        LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);

        zframe = zune_zframe_get(
                     obj, &muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Drag]);

        img->width = _width(obj) + zframe->ileft + zframe->iright;
        img->height = _height(obj) + zframe->itop + zframe->ibottom;

        if ((img->bm = AllocBitMap(img->width, img->height, depth, BMF_MINPLANES,
                                   _screen(obj)->RastPort.BitMap))) {
            /* Render the stuff now */
            struct RastPort *rp_save = muiRenderInfo(obj)->mri_RastPort;
            struct RastPort temprp;
            InitRastPort(&temprp);
            temprp.BitMap = img->bm;
            ClipBlit(_rp(obj), _left(obj), _top(obj), &temprp, zframe->ileft,
                     zframe->itop, _width(obj), _height(obj), 0xc0);

            muiRenderInfo(obj)->mri_RastPort = &temprp;
            zframe->draw(zframe->customframe, muiRenderInfo(obj), 0, 0, img->width,
                         img->height, 0, 0, img->width, img->height);
            muiRenderInfo(obj)->mri_RastPort = rp_save;
        }

        img->touchx = msg->touchx;
        img->touchy = msg->touchy;
        img->flags = 0;
    }
    return (IPTR)img;
}

/****** Area.mui/MUIM_DeleteDragImage ************
*
*   NAME
*       MUIM_DeleteDragImage
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DeleteDragImage, (IPTR)di);
*
*   FUNCTION
*       Free the drag image bitmap if present, then the MUI_DragImage
*       allocation.
*
*   INPUTS
*       di - Owned result of this implementation's CreateDragImage, or NULL.
*
*   RESULT
*       Always zero.
*
*   NOTES
*       Delete exactly once. A subclass with different image ownership must
*       provide a matching deletion implementation. A NULL bitmap is accepted.
*
*/
static IPTR Area__MUIM_DeleteDragImage(struct IClass *cl, Object *obj,
                                       struct MUIP_DeleteDragImage *msg)
{
    if (msg->di) {
        if (msg->di->bm)
            FreeBitMap(msg->di->bm);
        FreeVec(msg->di);
    }
    return 0;
}

/* Private MUIM_DragQueryExtended: if Dropable and inclusive window-relative
 * x/y hit the object, call DragQuery for borrowed msg->obj. Return this
 * borrowed object only for exact Accept, otherwise zero. No child search,
 * Disabled or ShowMe test is performed here; require live valid layout and
 * callback-safe objects.
 */
static IPTR Area__MUIM_DragQueryExtended(struct IClass *cl, Object *obj,
        struct MUIP_DragQueryExtended *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    if (data->mad_Flags & MADF_DROPABLE) {
        if (_left(obj) <= msg->x && msg->x <= _right(obj) && _top(obj) <= msg->y &&
                msg->y <= _bottom(obj)) {
            if (DoMethod(obj, MUIM_DragQuery, (IPTR)msg->obj) ==
                    MUIV_DragQuery_Accept)
                return (IPTR)obj;
        }
    }
    return 0;
}

/****** Area.mui/MUIM_DragBegin ************
*
*   NAME
*       MUIM_DragBegin
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DragBegin, (IPTR)obj);
*
*   FUNCTION
*       Draw the drop-target outline using the object's current bounds.
*
*   INPUTS
*       obj - Borrowed drag source; unused by the Area implementation. Requires
*       live display resources when drawable.
*
*   RESULT
*       Always zero.
*
*   NOTES
*       This marks the target visually and does not transfer ownership or
*       perform a drop. The drawing helper skips undrawable objects and direct
*       Window children.
*
*/
static IPTR Area__MUIM_DragBegin(struct IClass *cl, Object *obj,
                                 struct MUIP_DragBegin *msg)
{
    // struct MUI_AreaData *data = INST_DATA(cl, obj);
    _zune_focus_new(obj, ZUNE_FOCUS_TYPE_DROP_OBJ);
    return 0;
}

/****** Area.mui/MUIM_DragFinish ************
*
*   NAME
*       MUIM_DragFinish
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DragFinish, (IPTR)obj);
*
*   FUNCTION
*       Erase the drop-target outline using the object's current bounds.
*
*   INPUTS
*       obj - Borrowed drag source; unused by the Area implementation. Requires
*       live display resources when drawable.
*
*   RESULT
*       Always zero.
*
*   NOTES
*       This marks the target visually and does not transfer ownership or
*       perform a drop. Erasure redraws the edge under a temporary clip region
*       when allocation succeeds; without it the redraw is broader.
*
*/
static IPTR Area__MUIM_DragFinish(struct IClass *cl, Object *obj,
                                  struct MUIP_DragFinish *msg)
{
    // struct MUI_AreaData *data = INST_DATA(cl, obj);
    _zune_focus_destroy(obj, ZUNE_FOCUS_TYPE_DROP_OBJ);
    return 0;
}

/*
 * Calculates addleft, addtop, subwidth, subheight from current settings.
 * If frame phantom, ignore horizontal frame components.
 * Depends on inner sizes and frame
 */
/* Compute BYTE add-left/top and total sub-width/height from current inner
 * spacing, title height and frame borders. Phantom mode zeros both
 * horizontal contributions. Borrow live inputs and mutate instance geometry
 * offsets; frame parameter is unused. No clamping/overflow check or status
 * is provided.
 */
static void area_update_msizes(Object *obj, struct MUI_AreaData *data,
                               const struct MUI_FrameSpec_intern *frame,
                               const struct ZuneFrameGfx *zframe)
{

    /*      if (XGET(obj, MUIA_UserData) == 42) */
    /*      { */
    /*      D(bug("area_update_msizes(%p) : ileft=%ld itop=%ld\n", obj, */
    /*        zframe->ileft, zframe->itop)); */
    /*      } */

    struct dt_frame_image *fi = zframe->customframe;

    data->mad_addleft = data->mad_InnerLeft + zframe->ileft;
    data->mad_subwidth =
        data->mad_addleft + data->mad_InnerRight + zframe->iright;
    data->mad_addtop =
        data->mad_InnerTop + data->mad_TitleHeightAdd + zframe->itop;
    data->mad_subheight =
        data->mad_addtop + data->mad_InnerBottom + zframe->ibottom;

    if (data->mad_Flags & MADF_FRAMEPHANTOM) {
        data->mad_addleft = 0;
        data->mad_subwidth = 0;
    }

    // clamping ... maybe ?

    /*  D(bug("area_update_msizes(%x,%d) => addleft/top=%d/%d, " */
    /*      "subwidth/height=%d/%d\n", */
    /*        obj, data->mad_Frame, data->mad_addleft, data->mad_addtop, */
    /*      data->mad_subwidth, data->mad_subheight)); */
}

/****** Area.mui/MUIM_UnknownDropDestination ************
*
*   NAME
*       MUIM_UnknownDropDestination
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_UnknownDropDestination, (IPTR)imsg);
*
*   FUNCTION
*       Do nothing when Window reports a released drag without an accepted
*       target.
*
*   INPUTS
*       imsg - Borrowed live IntuiMessage for the release. Area does not inspect
*       it.
*
*   RESULT
*       Always zero.
*
*   NOTES
*       Override to handle this condition if needed. Do not retain or reply to
*       the message; the Window drag/event path owns reply handling.
*
*/
IPTR Area__MUIM_UnknownDropDestination(
    struct IClass *cl, Object *obj, struct MUIP_UnknownDropDestination *msg)
{
    return 0;
}

/* Private MUIM_UpdateInnerSizes: when SETUP is set, resolve the frame and
 * recompute derived offsets from current stored inner spacing/title values.
 * Always return 1, including before setup. This does not rerun
 * spacing/title selection, AskMinMax or parent layout; only use when
 * geometry constraints remain valid.
 */
static IPTR Area__MUIM_UpdateInnerSizes(struct IClass *cl, Object *obj,
                                        struct MUIP_UpdateInnerSizes *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    const struct MUI_FrameSpec_intern *frame;
    struct MUI_FrameSpec_intern tempframe;

    if (_flags(obj) & MADF_SETUP) {
        frame = get_intframe(obj, data, &tempframe);
        zframe = zune_zframe_get(obj, frame);
        area_update_msizes(obj, data, frame, zframe);
    }
    return 1;
}

/* Private MUIM_FindAreaObject: compare borrowed msg->obj with this object
 * and return this pointer on identity, otherwise NULL. Area does not
 * traverse children or take ownership.
 */
static IPTR Area__MUIM_FindAreaObject(struct IClass *cl, Object *obj,
                                      struct MUIP_FindAreaObject *msg)
{
    if (msg->obj == obj)
        return (IPTR)obj;
    else
        return (IPTR)NULL;
}

/****** Area.mui/MUIM_QueryFrameCharacteristics ************
*
*   NAME
*       MUIM_QueryFrameCharacteristics
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_QueryFrameCharacteristics,
*       (IPTR)characteristics);
*
*   FUNCTION
*       Write border_width, border_radius and has_rounded_corners from the
*       resolved frame specification.
*
*   INPUTS
*       characteristics - Non-NULL writable MUI_FrameCharacteristics. Requires
*       completed Setup and live preferences.
*
*   RESULT
*       FALSE when characteristics is NULL or the object is not set up. The
*       success path has no return statement and therefore no defined result.
*
*   NOTES
*       The output describes the specification, not a measurement of rendered
*       pixels. Keep output storage valid through the synchronous call.
*
*   BUGS
*       Do not branch on a success return from this implementation. This
*       documented source defect is not repaired by the AutoDoc change.
*
*/
static IPTR Area__MUIM_QueryFrameCharacteristics(
    struct IClass *cl, Object *obj,
    struct MUIP_QueryFrameCharacteristics *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct MUI_FrameSpec_intern *frame;
    struct MUI_FrameSpec_intern tempframe;

    if (!msg->characteristics)
        return FALSE;

    if (!(data->mad_Flags & MADF_SETUP))
        return FALSE;

    frame = get_intframe(obj, data, &tempframe);

    msg->characteristics->border_width = frame->border_width;
    msg->characteristics->border_radius = frame->border_radius;
    msg->characteristics->has_rounded_corners = frame->border_radius > 0;
}

/****** Area.mui/MUIM_CreateFrameClippingRegion ************
*
*   NAME
*       MUIM_CreateFrameClippingRegion
*
*   SYNOPSIS
*       IPTR result = DoMethodA(obj, (Msg)&message);
*
*   FUNCTION
*       For a set-up object with rounded frame corners, allocate a region for
*       the supplied rectangle and replace the message's clipinfo field with its
*       pointer. For a nonrounded frame, leave clipinfo unchanged.
*
*   INPUTS
*       message - Writable MUIP_CreateFrameClippingRegion with MethodID, LONG
*       left/top/width/height and a non-NULL initial clipinfo pointer. The
*       initial pointer is tested but not read or freed. Nonpositive
*       rounded-region dimensions fail allocation in the helper.
*
*   RESULT
*       FALSE for NULL initial clipinfo, missing setup, failed characteristic
*       query or failed rounded-region creation; otherwise TRUE.
*
*   NOTES
*       The output is the field in the caller's method message, not memory
*       addressed by the initial Region pointer. DoMethodA with a retained
*       message is needed to observe that replacement. A newly returned Region
*       is caller-owned and must be disposed when no longer used; the previous
*       region remains caller-owned too.
*
*   BUGS
*       Ordinary DoMethod arguments do not expose the replaced field to the
*       caller. NULL cannot be used as a normal output initializer because it is
*       rejected. TRUE for a nonrounded frame does not mean a region was created
*       or filled.
*
*/
static IPTR Area__MUIM_CreateFrameClippingRegion(
    struct IClass *cl, Object *obj,
    struct MUIP_CreateFrameClippingRegion *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct MUI_FrameSpec_intern *frame;
    struct MUI_FrameSpec_intern tempframe;

    if (!msg->clipinfo)
        return FALSE;

    if (!(data->mad_Flags & MADF_SETUP))
        return FALSE;

    frame = get_intframe(obj, data, &tempframe);
    struct MUI_FrameCharacteristics characteristics;

    if (!zune_frame_get_characteristics(obj, frame, &characteristics)) {
        return FALSE;
    }

    if (characteristics.has_rounded_corners) {
        msg->clipinfo = zune_frame_create_clip_region(
                            msg->left, msg->top, msg->width, msg->height,
                            characteristics.border_radius);

        if (!msg->clipinfo) {
            return FALSE;
        }
    }
    return TRUE;
}

/****** Area.mui/MUIM_Layout ************
*
*   NAME
*       MUIM_Layout
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Layout);
*
*   FUNCTION
*       Return the Area dispatcher default without laying out children or
*       changing geometry.
*
*   INPUTS
*       No arguments beyond MethodID.
*
*   RESULT
*       Always 1.
*
*   NOTES
*       Area is not a container. Parents set its box; container subclasses
*       override Layout for their own children.
*
*/

/****** Area.mui/MUIM_DragQuery ************
*
*   NAME
*       MUIM_DragQuery
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DragQuery, (IPTR)obj);
*
*   FUNCTION
*       Refuse a potential drag source in the Area dispatcher default.
*
*   INPUTS
*       obj - Borrowed source object; not inspected by Area.
*
*   RESULT
*       Always MUIV_DragQuery_Refuse.
*
*   NOTES
*       A drop-target subclass must override this to accept supported sources.
*       Dropable and hit testing are performed by the private extended query,
*       not by this default.
*
*/

/****** Area.mui/MUIM_DragDrop ************
*
*   NAME
*       MUIM_DragDrop
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DragDrop, (IPTR)obj, x, y);
*
*   FUNCTION
*       Perform no drop action in the Area dispatcher default.
*
*   INPUTS
*       obj - Borrowed source object.
*       x, y - LONG destination-window coordinates. All arguments are unused by
*       Area.
*
*   RESULT
*       Always FALSE.
*
*   NOTES
*       A target subclass must implement the transfer it accepts. No source
*       ownership, data copy or object deletion occurs here.
*
*/

/****** Area.mui/MUIM_DragReport ************
*
*   NAME
*       MUIM_DragReport
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DragReport, (IPTR)obj, x, y, update);
*
*   FUNCTION
*       Continue tracking in the Area dispatcher default without examining the
*       source or pointer position.
*
*   INPUTS
*       obj - Borrowed source.
*       x, y - LONG destination-window coordinates.
*       update - LONG redraw/update request; ignored by Area.
*
*   RESULT
*       Always MUIV_DragReport_Continue.
*
*   NOTES
*       Custom targets may return Abort, Continue or Refresh as appropriate.
*       Window's drag loop currently treats Lock as unimplemented and makes no
*       locking guarantee. It may call DragReport twice in one mouse-move cycle.
*
*/

/****** Area.mui/MUIM_CustomBackfill ************
*
*   NAME
*       MUIM_CustomBackfill
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_CustomBackfill, left, top, right,
*       bottom, xoffset, yoffset);
*
*   FUNCTION
*       Declared but not implemented by the Area dispatcher or invoked by its
*       default drawing path.
*
*   INPUTS
*       left, top, right, bottom, xoffset, yoffset - LONG fields declared by
*       MUIP_CustomBackfill; not consumed by Area.
*
*   RESULT
*       The inherited unhandled-method result is zero.
*
*   NOTES
*       MUIA_CustomBackfill also has no Area implementation. The declaration
*       alone does not establish a custom-backfill capability.
*
*/

/* Dispatch Area lifecycle, drawing, input and inherited import/export
 * messages. Provide explicit no-op Layout/DragDrop, Refuse DragQuery and
 * Continue DragReport defaults. Return handler results and forward unknown
 * IDs to Notify. Require a live object and a message with the fields used
 * by the selected operation; CustomBackfill remains unhandled.
 */
BOOPSI_DISPATCHER(IPTR, Area_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID) {
    case OM_NEW:
        return Area__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Area__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Area__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Area__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_AskMinMax:
        return Area__MUIM_AskMinMax(cl, obj, (APTR)msg);
    case MUIM_Draw:
        return Area__MUIM_Draw(cl, obj, (APTR)msg);
    case MUIM_DrawBackground:
        return Area__MUIM_DrawBackground(cl, obj, (APTR)msg);
    case MUIM_DrawBackgroundBuffered:
        return Area__MUIM_DrawBackgroundBuffered(cl, obj, (APTR)msg);
    case MUIM_DrawParentBackground:
        return Area__MUIM_DrawParentBackground(cl, obj, (APTR)msg);
    case MUIM_Setup:
        return Area__MUIM_Setup(cl, obj, (APTR)msg);
    case MUIM_Cleanup:
        return Area__MUIM_Cleanup(cl, obj, (APTR)msg);
    case MUIM_Show:
        return Area__MUIM_Show(cl, obj, (APTR)msg);
    case MUIM_Hide:
        return Area__MUIM_Hide(cl, obj, (APTR)msg);
    case MUIM_GoActive:
        return Area__MUIM_GoActive(cl, obj, (APTR)msg);
    case MUIM_GoInactive:
        return Area__MUIM_GoInactive(cl, obj, (APTR)msg);
    case MUIM_Layout:
        return 1;
    case MUIM_CreateShortHelp:
        return Area__MUIM_CreateShortHelp(cl, obj, (APTR)msg);
    case MUIM_DeleteShortHelp:
        return Area__MUIM_DeleteShortHelp(cl, obj, (APTR)msg);
    case MUIM_CreateBubble:
        return Area__MUIM_CreateBubble(cl, obj, (APTR)msg);
    case MUIM_DeleteBubble:
        return Area__MUIM_DeleteBubble(cl, obj, (APTR)msg);
    case MUIM_HandleEvent:
        return Area__MUIM_HandleEvent(cl, obj, (APTR)msg);
    case MUIM_ContextMenuBuild:
        return Area__MUIM_ContextMenuBuild(cl, obj, (APTR)msg);
    case MUIM_ContextMenuChoice:
        return Area__MUIM_ContextMenuChoice(cl, obj, (APTR)msg);
    case MUIM_Timer:
        return Area__MUIM_Timer(cl, obj, msg);
    case MUIM_UpdateInnerSizes:
        return Area__MUIM_UpdateInnerSizes(cl, obj, (APTR)msg);
    case MUIM_DragQuery:
        return MUIV_DragQuery_Refuse;
    case MUIM_DragBegin:
        return Area__MUIM_DragBegin(cl, obj, (APTR)msg);
    case MUIM_DragDrop:
        return FALSE;
    case MUIM_UnknownDropDestination:
        return Area__MUIM_UnknownDropDestination(cl, obj, (APTR)msg);
    case MUIM_DragFinish:
        return Area__MUIM_DragFinish(cl, obj, (APTR)msg);
    case MUIM_DragReport:
        return MUIV_DragReport_Continue; /* or MUIV_DragReport_Abort? */
    case MUIM_DoDrag:
        return Area__MUIM_DoDrag(cl, obj, (APTR)msg);
    case MUIM_CreateDragImage:
        return Area__MUIM_CreateDragImage(cl, obj, (APTR)msg);
    case MUIM_DeleteDragImage:
        return Area__MUIM_DeleteDragImage(cl, obj, (APTR)msg);
    case MUIM_DragQueryExtended:
        return Area__MUIM_DragQueryExtended(cl, obj, (APTR)msg);
    case MUIM_HandleInput:
        return Area__MUIM_HandleInput(cl, obj, (APTR)msg);
    case MUIM_FindAreaObject:
        return Area__MUIM_FindAreaObject(cl, obj, (APTR)msg);

    case MUIM_Export:
        return Area__MUIM_Export(cl, obj, (APTR)msg);
    case MUIM_Import:
        return Area__MUIM_Import(cl, obj, (APTR)msg);

    case MUIM_QueryFrameCharacteristics:
        return Area__MUIM_QueryFrameCharacteristics(cl, obj, (APTR)msg);
    case MUIM_CreateFrameClippingRegion:
        return Area__MUIM_CreateFrameClippingRegion(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Area_desc = {MUIC_Area, MUIC_Notify,
          sizeof(struct MUI_AreaData),
          (void *)Area_Dispatcher
};
