/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

#define MENUF_CHECKED        (1<<0)
#define MENUF_CHECKIT        (1<<1)
#define MENUF_COMMANDSTRING  (1<<2)
#define MENUF_ENABLED        (1<<3)
#define MENUF_TOGGLE         (1<<4)

#define MENUF_MENUSTRIP      (1<<15)
#define MENUF_MENU           (1<<16)
#define MENUF_MENUITEM       (1<<17)

#define MUIA_Menuitem_Type   (MUIB_Menuitem | 0x00000001)

#define MUIV_Menuitem_Type_Menustrip    (-1)
#define MUIV_Menuitem_Type_Menu         (-2)
#define MUIV_Menuitem_Type_Menuitem     (-3)

struct MUI_MenuitemData
{
    ULONG flags;
    ULONG exclude;

    char *shortcut;
    char *title;

    struct NewMenu *newmenu;

    struct MenuItem *trigger;
};

/****** Menustrip.mui/MUIC_Menustrip ************
*
*   NAME
*       MUIC_Menustrip
*
*   SUPERCLASS
*       Family.mui
*
*   DESCRIPTION
*       Represent a container of menu titles and their entries. The conventional
*       tree is Menustrip, then Menu children, then Menuitem children and
*       optional Menuitem subitems.
*       All three menu classes use the same attribute and method handlers and
*       derive directly from Family.mui. They differ by an internal type flag.
*       Family owns attached children and disposes of them; title and shortcut
*       strings remain borrowed. Enabled defaults to TRUE; Checked, Checkit,
*       CommandString, Toggle, and Exclude default to zero; title, shortcut,
*       trigger, and generated-menu pointers default to NULL.
*       A recognized menu-attribute set requests MUIM_Application_UpdateMenus
*       when the object has global info with an application pointer. That
*       request is synchronous and its result is ignored. Changes made before
*       connection are stored without requesting an application update. String
*       storage must remain valid while the object or any generated menu still
*       refers to it.
*
*   METHODS
*       ConnectParent first obtains the parent's global info through Notify,
*       stores the parent pointer, and recursively connects every child to this
*       object. It returns TRUE. DisconnectParent recursively forwards the
*       message to children, clears this object's parent pointer, then calls the
*       superclass and returns TRUE. Global info remains borrowed and is not
*       cleared by Notify.
*       AddHead, AddTail, Insert, Remove, OM_ADDMEMBER, and OM_REMMEMBER forward
*       to Family and then request an application menu update when connected,
*       even if the operation returned failure. Their original return value is
*       preserved. Sort is inherited without an update request; Transfer invokes
*       the normal Remove and AddTail methods for its children. Serialize all
*       tree operations and do not mutate the traversal from callbacks.
*       Other methods and attributes follow Family and Notify. These classes add
*       no new public method identifiers.
*
*   BUGS
*       The three constructors do not check whether common superclass
*       construction returned NULL before writing their type flag. Allocation
*       failure or a NULL child that makes Family construction fail can
*       therefore cause invalid memory access instead of a clean NULL return. Do
*       not rely on the normal constructor-failure contract for these
*       implementations.
*
*/
/****** Menu.mui/MUIC_Menu ************
*
*   NAME
*       MUIC_Menu
*
*   SUPERCLASS
*       Family.mui
*
*   DESCRIPTION
*       Represent a menu title with owned Menuitem children. This is a sibling
*       of Menuitem in the class hierarchy, not a subclass of it.
*       All three menu classes use the same attribute and method handlers and
*       derive directly from Family.mui. They differ by an internal type flag.
*       Family owns attached children and disposes of them; title and shortcut
*       strings remain borrowed. Enabled defaults to TRUE; Checked, Checkit,
*       CommandString, Toggle, and Exclude default to zero; title, shortcut,
*       trigger, and generated-menu pointers default to NULL.
*       A recognized menu-attribute set requests MUIM_Application_UpdateMenus
*       when the object has global info with an application pointer. That
*       request is synchronous and its result is ignored. Changes made before
*       connection are stored without requesting an application update. String
*       storage must remain valid while the object or any generated menu still
*       refers to it.
*
*   METHODS
*       ConnectParent first obtains the parent's global info through Notify,
*       stores the parent pointer, and recursively connects every child to this
*       object. It returns TRUE. DisconnectParent recursively forwards the
*       message to children, clears this object's parent pointer, then calls the
*       superclass and returns TRUE. Global info remains borrowed and is not
*       cleared by Notify.
*       AddHead, AddTail, Insert, Remove, OM_ADDMEMBER, and OM_REMMEMBER forward
*       to Family and then request an application menu update when connected,
*       even if the operation returned failure. Their original return value is
*       preserved. Sort is inherited without an update request; Transfer invokes
*       the normal Remove and AddTail methods for its children. Serialize all
*       tree operations and do not mutate the traversal from callbacks.
*       Other methods and attributes follow Family and Notify. These classes add
*       no new public method identifiers.
*
*   BUGS
*       The three constructors do not check whether common superclass
*       construction returned NULL before writing their type flag. Allocation
*       failure or a NULL child that makes Family construction fail can
*       therefore cause invalid memory access instead of a clean NULL return. Do
*       not rely on the normal constructor-failure contract for these
*       implementations.
*
*/
/****** Menuitem.mui/MUIC_Menuitem ************
*
*   NAME
*       MUIC_Menuitem
*
*   SUPERCLASS
*       Family.mui
*
*   DESCRIPTION
*       Represent a menu entry, separator, or entry with submenu children.
*       Rendering depends on tree depth when the root exports its NewMenu array.
*       All three menu classes use the same attribute and method handlers and
*       derive directly from Family.mui. They differ by an internal type flag.
*       Family owns attached children and disposes of them; title and shortcut
*       strings remain borrowed. Enabled defaults to TRUE; Checked, Checkit,
*       CommandString, Toggle, and Exclude default to zero; title, shortcut,
*       trigger, and generated-menu pointers default to NULL.
*       A recognized menu-attribute set requests MUIM_Application_UpdateMenus
*       when the object has global info with an application pointer. That
*       request is synchronous and its result is ignored. Changes made before
*       connection are stored without requesting an application update. String
*       storage must remain valid while the object or any generated menu still
*       refers to it.
*
*   METHODS
*       ConnectParent first obtains the parent's global info through Notify,
*       stores the parent pointer, and recursively connects every child to this
*       object. It returns TRUE. DisconnectParent recursively forwards the
*       message to children, clears this object's parent pointer, then calls the
*       superclass and returns TRUE. Global info remains borrowed and is not
*       cleared by Notify.
*       AddHead, AddTail, Insert, Remove, OM_ADDMEMBER, and OM_REMMEMBER forward
*       to Family and then request an application menu update when connected,
*       even if the operation returned failure. Their original return value is
*       preserved. Sort is inherited without an update request; Transfer invokes
*       the normal Remove and AddTail methods for its children. Serialize all
*       tree operations and do not mutate the traversal from callbacks.
*       Other methods and attributes follow Family and Notify. These classes add
*       no new public method identifiers.
*
*   BUGS
*       The three constructors do not check whether common superclass
*       construction returned NULL before writing their type flag. Allocation
*       failure or a NULL child that makes Family construction fail can
*       therefore cause invalid memory access instead of a clean NULL return. Do
*       not rely on the normal constructor-failure contract for these
*       implementations.
*
*/
/****** Menustrip.mui/MUIA_Menustrip_Enabled ************
*
*   NAME
*       MUIA_Menustrip_Enabled
*
*   FUNCTION
*       This identifier is declared in the public header but is not handled by
*       the shared menu constructor, setter, or getter.
*
*   INPUTS
*       BOOL; declared initialize, set, get.
*
*   NOTES
*       The implementation handles MUIA_Menu_Enabled and MUIA_Menuitem_Enabled
*       instead, as aliases for one object-local flag. Setting that flag on a
*       Menustrip does not disable all descendants: NewMenu generation emits
*       children and does not propagate the root's enabled state.
*
*   BUGS
*       Do not treat this attribute as a working whole-menustrip enable switch.
*       An OM_SET tag can reach Notify and trigger a registration without any
*       corresponding menu state being stored. There is no successful menu
*       getter for this identifier.
*
*/
/****** Menu.mui/MUIA_Menu_Enabled ************
*
*   NAME
*       MUIA_Menu_Enabled
*
*   FUNCTION
*       Store whether this object's generated menu entry is enabled. Default
*       TRUE. MUIA_Menu_Enabled and MUIA_Menuitem_Enabled address the same flag
*       in the shared implementation.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       The getter returns TRUE or FALSE. A disabled generated title receives
*       NM_MENUDISABLED; other generated entries receive NM_ITEMDISABLED. The
*       flag is not recursively copied to descendants. The setter requests an
*       application update when connected.
*
*   BUGS
*       OM_SET changes the supplied enabled tag's identifier to TAG_IGNORE
*       before forwarding the tag list to Notify. Consequently this tag does not
*       trigger normal Notify delivery, and the caller's tag list is modified.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Enabled ************
*
*   NAME
*       MUIA_Menuitem_Enabled
*
*   FUNCTION
*       Store whether this object's generated menu entry is enabled. Default
*       TRUE. MUIA_Menu_Enabled and MUIA_Menuitem_Enabled address the same flag
*       in the shared implementation.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       The getter returns TRUE or FALSE. A disabled generated title receives
*       NM_MENUDISABLED; other generated entries receive NM_ITEMDISABLED. The
*       flag is not recursively copied to descendants. The setter requests an
*       application update when connected.
*
*   BUGS
*       OM_SET changes the supplied enabled tag's identifier to TAG_IGNORE
*       before forwarding the tag list to Notify. Consequently this tag does not
*       trigger normal Notify delivery, and the caller's tag list is modified.
*
*/
/****** Menu.mui/MUIA_Menu_Title ************
*
*   NAME
*       MUIA_Menu_Title
*
*   FUNCTION
*       Store a borrowed title pointer, initially NULL. Menu_Title and
*       Menuitem_Title address the same field. The pointer becomes nm_Label when
*       this object is emitted into a NewMenu array.
*
*   INPUTS
*       STRPTR; initialize, set, get.
*
*   NOTES
*       Use a readable NUL-terminated label for an ordinary menu entry.
*       NM_BARLABEL denotes a separator at an item position. The class does not
*       copy or free the label. Keep its storage alive while the object or a
*       generated menu refers to it. Setting the title requests an application
*       update when connected.
*
*   BUGS
*       OM_SET rewrites the supplied title tag to TAG_IGNORE before Notify
*       processes the list. A title change therefore does not produce normal
*       Notify delivery for that tag, and the caller's tag list is changed.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Title ************
*
*   NAME
*       MUIA_Menuitem_Title
*
*   FUNCTION
*       Store a borrowed title pointer, initially NULL. Menu_Title and
*       Menuitem_Title address the same field. The pointer becomes nm_Label when
*       this object is emitted into a NewMenu array.
*
*   INPUTS
*       STRPTR; initialize, set, get.
*
*   NOTES
*       Use a readable NUL-terminated label for an ordinary menu entry.
*       NM_BARLABEL denotes a separator at an item position. The class does not
*       copy or free the label. Keep its storage alive while the object or a
*       generated menu refers to it. Setting the title requests an application
*       update when connected.
*
*   BUGS
*       OM_SET rewrites the supplied title tag to TAG_IGNORE before Notify
*       processes the list. A title change therefore does not produce normal
*       Notify delivery for that tag, and the caller's tag list is changed.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Checked ************
*
*   NAME
*       MUIA_Menuitem_Checked
*
*   FUNCTION
*       Store the checked state, initially FALSE. The generated NewMenu flags
*       contain CHECKED when this state is true.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       When set to a true value and Exclude is nonzero, walk the parent's
*       immediate children and clear Checked on other children selected by the
*       exclusion mask that are both Checkit and Checked. These ordinary setters
*       can trigger notifications and menu updates. Initialization stores the
*       flag without this exclusion pass. The current object is not required to
*       have Checkit set for the exclusion pass to run. Keep parent membership
*       stable during callbacks.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Checkit ************
*
*   NAME
*       MUIA_Menuitem_Checkit
*
*   FUNCTION
*       Store whether the generated entry has CHECKIT. Default FALSE.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       The class stores Checked and Toggle independently; enabling Checkit does
*       not set or clear them. Setting the attribute requests an application
*       menu update when connected.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_CommandString ************
*
*   NAME
*       MUIA_Menuitem_CommandString
*
*   FUNCTION
*       Select command-string presentation for the shortcut. Default FALSE.
*       Generation sets COMMSEQ, which is the AROS NewMenu flag
*       NM_COMMANDSTRING.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       With a non-NULL shortcut, AROS GadTools uses the complete string as
*       command text when NM_COMMANDSTRING is set. Otherwise it takes the first
*       shortcut character as the command key. Menuitem itself only stores the
*       flag and forwards the shortcut pointer. Setting it requests an
*       application update when connected.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Exclude ************
*
*   NAME
*       MUIA_Menuitem_Exclude
*
*   FUNCTION
*       Store a mutual-exclusion bit mask, initially zero, and copy it to
*       nm_MutualExclude in the generated entry.
*
*   INPUTS
*       LONG in the header; initialize, set, get. Stored as ULONG.
*
*   NOTES
*       For the setter-side exclusion pass, bit zero selects the first immediate
*       child in the parent's current list, bit one the second, and so on
*       through the ULONG mask width. The pass skips this object and clears only
*       siblings that are both Checkit and Checked. Changing Exclude alone does
*       not run the pass; setting Checked true does. Reordering children changes
*       which sibling a mask bit selects.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Shortcut ************
*
*   NAME
*       MUIA_Menuitem_Shortcut
*
*   FUNCTION
*       Store a borrowed shortcut string pointer, initially NULL, and pass it
*       unchanged as nm_CommKey.
*
*   INPUTS
*       STRPTR; initialize, set, get.
*
*   NOTES
*       A NULL pointer means no shortcut. Keep any non-NULL string readable and
*       NUL-terminated while the object or generated menus use it. CommandString
*       controls its GadTools interpretation. Setting the attribute requests an
*       application update when connected.
*
*   BUGS
*       MUIV_Menuitem_Shortcut_Check (-1) is declared in the header but is not
*       interpreted by this class. It would be forwarded as a pointer to
*       GadTools, which reads the shortcut string; do not supply that sentinel
*       as a shortcut.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Toggle ************
*
*   NAME
*       MUIA_Menuitem_Toggle
*
*   FUNCTION
*       Store whether generation sets MENUTOGGLE. Default FALSE.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       The implementation does not enforce a relationship with Checkit or
*       Checked. It stores the flag and requests an application menu update when
*       connected; actual menu interaction is handled by Intuition and the
*       window event path.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_Trigger ************
*
*   NAME
*       MUIA_Menuitem_Trigger
*
*   FUNCTION
*       Return the last trigger pointer, initially NULL. The shared setter also
*       accepts and stores this attribute; the constructor ignores it. The
*       window menu-pick path sets it to the selected native MenuItem.
*
*   INPUTS
*       Pointer to struct MenuItem; declared get only.
*
*   NOTES
*       An ordinary setter tag reaches Notify after the menu update request, so
*       a notification can observe a selection even if the stored value repeats.
*       The pointer is borrowed; the Menuitem object does not own the native
*       menu structure.
*
*   BUGS
*       Setting Trigger requests a synchronous application menu update before
*       Notify delivery. Window menu rebuilding can free the native menu
*       containing that very pointer. Treat the stored or notified pointer as an
*       event value, not as a safely dereferenceable or retained MenuItem. Menu
*       disposal and later rebuilds can also invalidate it.
*
*/
/****** Menuitem.mui/MUIA_Menuitem_NewMenu ************
*
*   NAME
*       MUIA_Menuitem_NewMenu
*
*   FUNCTION
*       Rebuild and return an owned, zero-terminated NewMenu array describing
*       this object's children. The object itself is not emitted. Every getter
*       first frees the previous array, including when the new build fails.
*
*   INPUTS
*       Pointer to struct NewMenu; get only.
*
*   NOTES
*       The getter reports success even when the stored pointer is NULL: that
*       occurs for no descendants or allocation failure. A non-NULL result is
*       borrowed until the next NewMenu getter on this object or its disposal.
*       Do not free it. Labels and shortcuts in the array remain borrowed, and
*       nm_UserData contains the child object pointer.
*       Emission is depth first. At depth zero, direct Menuitem children become
*       NM_ITEM and other child types become NM_TITLE; depth one becomes
*       NM_ITEM; depth two becomes NM_SUB. Deeper levels are not emitted. A
*       cleared trailing element supplies NM_END. The allocation count
*       nevertheless visits all descendants.
*
*   BUGS
*       The recursive traversal has no cycle or allocation-overflow checks.
*       Supply an acyclic, stable tree of Family-compatible menu objects. The
*       class does not validate the ordering required for a usable native menu.
*       Rebuilding is a getter side effect, not an immutable cached lookup.
*
*/

/* Recursively count all descendants through Family_List. Return an int
 * count. The caller supplies an acyclic, stable tree of objects with valid
 * Family lists and keeps the count in range; getter failures and cycles are
 * not checked.
 */
static int Menuitem_GetTotalChildren(Object *obj)
{
    Object *cstate;
    Object *child;
    struct MinList *ChildList = NULL;
    int num = 0;

    get(obj, MUIA_Family_List, &ChildList);
    cstate = (Object *) ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        num++;
        num += Menuitem_GetTotalChildren(child);
    }
    return num;
}

/* Fill caller-owned cleared NewMenu storage from children in depth-first
 * order and return the number emitted. Emit depths zero through two only,
 * borrowing labels, shortcuts and child pointers. The caller supplies
 * sufficient space and a stable menu tree; unsupported getters are not
 * handled as failures.
 */
static int Menuitem_FillNewMenu(Object *obj, struct NewMenu *menu,
    int depth)
{
    Object *cstate;
    Object *child;
    struct MinList *ChildList = NULL;
    int num = 0;

    if (depth > 2)
        return 0;

    get(obj, MUIA_Family_List, &ChildList);
    cstate = (Object *) ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        int entries;
        IPTR checkit = 0, checked = 0, toggle = 0, enabled = 0, cmdstring = 0, exclude = 0;

        get(child, MUIA_Menuitem_Title, &menu->nm_Label);
        get(child, MUIA_Menuitem_Shortcut, &menu->nm_CommKey);
        get(child, MUIA_Menuitem_Checkit, &checkit);
        get(child, MUIA_Menuitem_Checked, &checked);
        get(child, MUIA_Menuitem_Toggle, &toggle);
        get(child, MUIA_Menuitem_Enabled, &enabled);
        get(child, MUIA_Menuitem_CommandString, &cmdstring);
        if (checkit)
            menu->nm_Flags |= CHECKIT;
        if (checked)
            menu->nm_Flags |= CHECKED;
        if (toggle)
            menu->nm_Flags |= MENUTOGGLE;
        if (cmdstring)
            menu->nm_Flags |= COMMSEQ;
        get(child, MUIA_Menuitem_Exclude, &exclude);
        menu->nm_MutualExclude = exclude;

        if (depth == 0)
        {
            SIPTR type = 0;
            get(child, MUIA_Menuitem_Type, &type);

            if (type == MUIV_Menuitem_Type_Menuitem) {
                /* Depth 0 Menuitems are to become items of current menu,
                 * not menus. MUI behavior */
                menu->nm_Type = NM_ITEM;
            } else
                menu->nm_Type = NM_TITLE;
        }
        else if (depth == 1)
            menu->nm_Type = NM_ITEM;
        else if (depth == 2)
            menu->nm_Type = NM_SUB;

        if (!enabled)
        {
            if (menu->nm_Type == NM_TITLE)
                menu->nm_Flags |= NM_MENUDISABLED;
            else
                menu->nm_Flags |= NM_ITEMDISABLED;
        }

        menu->nm_UserData = child;

        menu++;
        num++;
        entries = Menuitem_FillNewMenu(child, menu, depth + 1);

        menu += entries;
        num += entries;
    }
    return num;
}

/**************************************************************************
 ...
**************************************************************************/
/* Count descendants, free the old owned array, and allocate a cleared array
 * with one extra terminator entry. Return the replacement pointer, or NULL
 * for no descendants or allocation failure. A failure also invalidates the
 * previous array. Counts and allocation arithmetic must fit their types.
 */
static struct NewMenu *Menuitem_BuildNewMenu(struct MUI_MenuitemData *data,
    Object *obj)
{
    int entries = Menuitem_GetTotalChildren(obj);
    if (data->newmenu)
        FreeVec(data->newmenu);
    data->newmenu = NULL;
    if (!entries)
        return NULL;

    if ((data->newmenu =
            (struct NewMenu *)AllocVec((entries +
                    1) * sizeof(struct NewMenu), MEMF_CLEAR)))
    {
        Menuitem_FillNewMenu(obj, data->newmenu, 0);
    }
    return data->newmenu;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/* Construct Family, returning zero if it fails. Otherwise initialize
 * enabled state and recognized menu tags without requesting application
 * updates or applying sibling exclusions. Store borrowed strings and return
 * the object. The class-specific wrappers must account for the possible
 * zero result.
 */
IPTR Common__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MenuitemData *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
        /* We need no tags */
    if (!obj)
        return 0;

    data = INST_DATA(cl, obj);

    data->flags = MENUF_ENABLED;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Menuitem_Checked:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKED);
            break;

        case MUIA_Menuitem_Checkit:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKIT);
            break;

        case MUIA_Menuitem_CommandString:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_COMMANDSTRING);
            break;

        case MUIA_Menu_Enabled:
        case MUIA_Menuitem_Enabled:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_ENABLED);
            break;

        case MUIA_Menuitem_Toggle:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_TOGGLE);
            break;

        case MUIA_Menuitem_Exclude:
            data->exclude = tag->ti_Data;
            break;

        case MUIA_Menuitem_Shortcut:
            data->shortcut = (char *)tag->ti_Data;
            break;

        case MUIA_Menu_Title:
        case MUIA_Menuitem_Title:
            data->title = (char *)tag->ti_Data;
            break;
        }
    }

    return (IPTR) obj;
}

/* Call common construction, mark the instance as a Menuitem, and return it.
 * This implementation does not check the common zero failure result before
 * accessing instance data; failure is not safely propagated.
 */
IPTR Menuitem__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MenuitemData *data;
    obj = (Object *) Common__OM_NEW(cl, obj, msg);
    data = INST_DATA(cl, obj);

    data->flags |= MENUF_MENUITEM;

    return (IPTR) obj;
}

/* Call common construction, mark the instance as a Menu, and return it.
 * This implementation does not check the common zero failure result before
 * accessing instance data; failure is not safely propagated.
 */
IPTR Menu__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MenuitemData *data;
    obj = (Object *) Common__OM_NEW(cl, obj, msg);
    data = INST_DATA(cl, obj);

    data->flags |= MENUF_MENU;

    return (IPTR) obj;
}

/* Call common construction, mark the instance as a Menustrip, and return
 * it. This implementation does not check the common zero failure result
 * before accessing instance data; failure is not safely propagated.
 */
IPTR Menustrip__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MenuitemData *data;
    obj = (Object *) Common__OM_NEW(cl, obj, msg);
    data = INST_DATA(cl, obj);

    data->flags |= MENUF_MENUSTRIP;

    return (IPTR) obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
/* Store recognized tags and apply sibling exclusion when setting Checked
 * true. Rewrite enabled/title tag IDs to TAG_IGNORE in caller-provided
 * writable tag storage. Request an application update before forwarding to
 * Family/Notify, then return the superclass result. String and trigger
 * pointers remain borrowed; callbacks must not invalidate the object or
 * traversal.
 */
IPTR Menuitem__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MenuitemData *data;
    struct TagItem *tags, *tag;

    data = INST_DATA(cl, obj);

    BOOL rebuild = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Menuitem_Checked:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKED);
            if (data->exclude && (data->flags & MENUF_CHECKED))
            {
                Object *parent = NULL;
                get(obj, MUIA_Parent, &parent);

                if (parent)
                {
                    Object *child;
                    Object *cstate;
                    struct MinList *ChildList = NULL;
                    ULONG i = 1;

                    get(parent, MUIA_Family_List, &ChildList);
                    cstate = (Object *) ChildList->mlh_Head;
                    while ((child = NextObject(&cstate)))
                    {
                        if ((i & data->exclude) && (child != obj))
                        {
                            IPTR checkit = 0, checked = 0;

                            get(child, MUIA_Menuitem_Checkit, &checkit);
                            get(child, MUIA_Menuitem_Checked, &checked);

                            if (checkit && checked)
                            {
                                set(child, MUIA_Menuitem_Checked, FALSE);
                            }
                        }

                        i <<= 1;
                    }
                }

            }
            rebuild = TRUE;
            break;

        case MUIA_Menuitem_Checkit:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKIT);
            rebuild = TRUE;
            break;

        case MUIA_Menuitem_CommandString:
            _handle_bool_tag(data->flags, tag->ti_Data,
                MENUF_COMMANDSTRING);
            rebuild = TRUE;
            break;

        case MUIA_Menu_Enabled:
        case MUIA_Menuitem_Enabled:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_ENABLED);
            tag->ti_Tag = TAG_IGNORE;
            rebuild = TRUE;
            break;

        case MUIA_Menuitem_Toggle:
            _handle_bool_tag(data->flags, tag->ti_Data, MENUF_TOGGLE);
            rebuild = TRUE;
            break;

        case MUIA_Menuitem_Exclude:
            data->exclude = tag->ti_Data;
            rebuild = TRUE;
            break;

        case MUIA_Menuitem_Shortcut:
            data->shortcut = (char *)tag->ti_Data;
            rebuild = TRUE;
            break;

        case MUIA_Menu_Title:
        case MUIA_Menuitem_Title:
            data->title = (char *)tag->ti_Data;
            tag->ti_Tag = TAG_IGNORE;
            rebuild = TRUE;
            break;

        case MUIA_Menuitem_Trigger:
            data->trigger = (struct MenuItem *)tag->ti_Data;
            rebuild = TRUE;
            break;
        }
    }

    if (rebuild)
    {
        if (muiNotifyData(obj) &&
            muiNotifyData(obj)->mnd_GlobalInfo &&
            muiNotifyData(obj)->mnd_GlobalInfo->mgi_ApplicationObject)
        {
            DoMethod(_app(obj), MUIM_Application_UpdateMenus);
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
#define STORE *(msg->opg_Storage)
/* Store menu state through writable opg_Storage and return 1; NewMenu
 * rebuilds the owned array and may store NULL despite success. The private
 * Type getter selects the constructor flag. Forward unknown attributes to
 * Family. Pointer results are borrowed with the lifetimes documented on
 * each attribute.
 */
IPTR Menuitem__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_MenuitemData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Menuitem_Checked:
        STORE = ((data->flags & MENUF_CHECKED) != 0);
        return 1;

    case MUIA_Menuitem_Checkit:
        STORE = ((data->flags & MENUF_CHECKIT) != 0);
        return 1;

    case MUIA_Menuitem_CommandString:
        STORE = ((data->flags & MENUF_COMMANDSTRING) != 0);
        return 1;

    case MUIA_Menu_Enabled:
    case MUIA_Menuitem_Enabled:
        STORE = ((data->flags & MENUF_ENABLED) != 0);
        return 1;

    case MUIA_Menuitem_Toggle:
        STORE = ((data->flags & MENUF_TOGGLE) != 0);
        return 1;

    case MUIA_Menuitem_Exclude:
        STORE = data->exclude;
        return 1;

    case MUIA_Menuitem_Shortcut:
        STORE = (IPTR) data->shortcut;
        return 1;

    case MUIA_Menu_Title:
    case MUIA_Menuitem_Title:
        STORE = (IPTR) data->title;
        return 1;

    case MUIA_Menuitem_NewMenu:
        Menuitem_BuildNewMenu(data, obj);
        STORE = (IPTR) data->newmenu;
        return 1;

    case MUIA_Menuitem_Trigger:
        STORE = (IPTR) data->trigger;
        return 1;

    case MUIA_Menuitem_Type:
        if (data->flags & MENUF_MENUSTRIP)
            STORE = (IPTR) MUIV_Menuitem_Type_Menustrip;
        else if(data->flags & MENUF_MENU)
            STORE = (IPTR) MUIV_Menuitem_Type_Menu;
        else
            STORE = (IPTR) MUIV_Menuitem_Type_Menuitem;
        return 1;

    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}
#undef STORE


/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/* After all consumers have finished, free the owned generated NewMenu
 * array, then dispose through Family and return its result. Family disposes
 * children. Borrowed label and shortcut strings are not freed here.
 */
IPTR Menuitem__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_MenuitemData *data = INST_DATA(cl, obj);

    FreeVec(data->newmenu);

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 MUIM_ConnectParent
**************************************************************************/
/* Connect through Notify, store the supplied live parent, and recursively
 * connect each child to this object. Return TRUE, ignoring child results.
 * Parent and global info remain borrowed; the tree must stay stable during
 * traversal.
 */
IPTR Menuitem__MUIM_ConnectParent(struct IClass *cl, Object *obj,
    struct MUIP_ConnectParent *msg)
{
    Object *cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    D(bug("Menuitem_ConnectParent(%p) %s\n", obj, OCLASS(obj)->cl_ID));

    DoSuperMethodA(cl, obj, (Msg) msg);

    muiNotifyData(obj)->mnd_ParentObject = msg->parent;

    get(obj, MUIA_Family_List, &ChildList);
    cstate = (Object *) ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethod(child, MUIM_ConnectParent, (IPTR) obj);
    }
    return TRUE;
}

/**************************************************************************
 MUIM_DisconnectParent
**************************************************************************/
/* Forward the disconnection message to every child, clear this parent
 * pointer, then call Family/Notify and return TRUE. Global info is not
 * cleared by Notify. The signature uses MUIP_ConnectParent but reads no
 * parent argument; a DisconnectParent message suffices. Keep the tree
 * stable.
 */
IPTR Menuitem__MUIM_DisconnectParent(struct IClass *cl, Object *obj,
    struct MUIP_ConnectParent *msg)
{
    Object *cstate;
    Object *child;
    struct MinList *ChildList = NULL;

    D(bug("Menuitem_DisconnectParent(%p) %s\n", obj, OCLASS(obj)->cl_ID));

    get(obj, MUIA_Family_List, &ChildList);
    cstate = (Object *) ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethodA(child, (Msg) msg);
    }
    muiNotifyData(obj)->mnd_ParentObject = NULL;
    DoSuperMethodA(cl, obj, (Msg) msg);
    return TRUE;
}

/**************************************************************************
 MUIM_Family_Insert, MUIM_Family_Remove
**************************************************************************/
/* Run the requested Family mutation, then request an application menu
 * update when global info provides an application, regardless of mutation
 * success. Return the original Family result. This does not roll back a
 * mutation if rebuilding fails.
 */
IPTR Menuitem__MUIM_Update(struct IClass *cl, Object *obj, Msg msg)
{
    // struct MUI_MenuitemData *data = INST_DATA(cl, obj);

    IPTR retval = DoSuperMethodA(cl, obj, msg);

    if (muiNotifyData(obj) &&
        muiNotifyData(obj)->mnd_GlobalInfo &&
        muiNotifyData(obj)->mnd_GlobalInfo->mgi_ApplicationObject)
    {
        DoMethod(_app(obj), MUIM_Application_UpdateMenus);
    }

    return retval;
}

/* Dispatch shared getters, setters, disposal, connection and selected
 * Family mutations. Forward other methods to Family. Sort is not
 * intercepted for a menu update. Return the selected handler or superclass
 * result; require a correctly sized message.
 */
IPTR Common_Dispatcher(Class *cl, Object *obj, Msg msg)
{
    switch (msg->MethodID)
    {
    case OM_DISPOSE:
        return Menuitem__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Menuitem__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Menuitem__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_ConnectParent:
        return Menuitem__MUIM_ConnectParent(cl, obj, (APTR) msg);
    case MUIM_DisconnectParent:
        return Menuitem__MUIM_DisconnectParent(cl, obj, (APTR) msg);
    case MUIM_Family_Insert:
        return Menuitem__MUIM_Update(cl, obj, (APTR) msg);
    case MUIM_Family_Remove:
        return Menuitem__MUIM_Update(cl, obj, (APTR) msg);
    case MUIM_Family_AddTail:
        return Menuitem__MUIM_Update(cl, obj, (APTR) msg);
    case MUIM_Family_AddHead:
        return Menuitem__MUIM_Update(cl, obj, (APTR) msg);
    case OM_ADDMEMBER:
        return Menuitem__MUIM_Update(cl, obj, (APTR) msg);
    case OM_REMMEMBER:
        return Menuitem__MUIM_Update(cl, obj, (APTR) msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}

/* Handle construction with the Menuitem type flag; otherwise dispatch
 * through the common menu handler. Return its result. The caller supplies a
 * correctly sized message.
 */
BOOPSI_DISPATCHER(IPTR, Menuitem_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Menuitem__OM_NEW(cl, obj, (struct opSet *)msg);
    }

    return Common_Dispatcher(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/* Handle construction with the Menu type flag; otherwise dispatch through
 * the common menu handler. Return its result. The caller supplies a
 * correctly sized message.
 */
BOOPSI_DISPATCHER(IPTR, Menu_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Menu__OM_NEW(cl, obj, (struct opSet *)msg);
    }

    return Common_Dispatcher(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/* Handle construction with the Menustrip type flag; otherwise dispatch
 * through the common menu handler. Return its result. The caller supplies a
 * correctly sized message.
 */
BOOPSI_DISPATCHER(IPTR, Menustrip_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Menustrip__OM_NEW(cl, obj, (struct opSet *)msg);
    }

    return Common_Dispatcher(cl, obj, msg);
}
BOOPSI_DISPATCHER_END
/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Menuitem_desc =
{
    MUIC_Menuitem,
    MUIC_Family,
    sizeof(struct MUI_MenuitemData),
    (void *) Menuitem_Dispatcher
};

/*
 * Class descriptor.- this class is the same like menuitem
 */
const struct __MUIBuiltinClass _MUI_Menu_desc =
{
    MUIC_Menu,
    MUIC_Family,
    sizeof(struct MUI_MenuitemData),
    (void *) Menu_Dispatcher
};

/*
 * Class descriptor.- this class is the same like menuitem
 */
const struct __MUIBuiltinClass _MUI_Menustrip_desc =
{
    MUIC_Menustrip,
    MUIC_Family,
    sizeof(struct MUI_MenuitemData),
    (void *) Menustrip_Dispatcher
};
