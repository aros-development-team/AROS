//
// PopupMenu
// ©1996-1997 Henrik Isaksson
//
// Get/Set Attrs functions
//

#include "pmpriv.h"

#ifdef __AROS__
void PM_FreeIDList(struct PM_IDLst *list);
#endif

/// PM_SetItemAttrsA
/*****************************************************************************
    NAME
        LONG PM_SetItemAttrsA(

    SYNOPSIS
        struct PopupMenu *p, struct TagItem *tags)

    FUNCTION
        Apply item attributes in tag order. Changes take effect immediately;
        allocation failures do not roll back earlier changes.

    INPUTS
        p - Item to change, or NULL.
        tags - Terminated tag list, or NULL. ti_Data contains the value.

    RESULT
        Number of processed attributes, excluding unsupported tags and failed
        string copies. PM_Title with NULL clears the title but does not count.
        Zero for NULL p. The count is not an all-or-nothing success result.

    TAGS
        PM_Title (STRPTR) - Copy a terminated string; NULL clears the title.
        PM_TitleID (ULONG) - Store a locale string ID instead of a title.
        PM_Object (Object *) - Borrow an object instead of a title.
        PM_UserData (APTR) - Borrow arbitrary data returned on selection.
        PM_UserDataString (STRPTR) - Copy a non-NULL terminated string.
        PM_ID (ULONG) - Set the item ID.
        PM_Sub (struct PopupMenu *) - Replace and own the child list, or
            clear it with NULL. Free the old children and select normal
            submenu mode. Do not pass the current list or its descendants.
        PM_Members (struct PopupMenu *) - Replace and own group children.
            Free the old list; use horizontal layout if layout was zero.
        PM_LayoutMode (ULONG) - Set layout; nonzero enables group mode.
            PML_Horizontal and PML_Vertical select the respective directions.
        PM_Flags (ULONG) - Replace all flags, including ownership flags.
            Reserved for callers that preserve the item's ownership state.
        PM_NoSelect (BOOL) - Set or clear the no-selection flag.
        PM_Disabled (BOOL) - Set or clear disabled and no-selection flags.
        PM_FillPen, PM_ShadowPen, PM_ShinePen (BOOL) - Enable the respective
            DrawInfo text-pen flag independently.
        PM_Italic, PM_Bold, PM_Underlined (BOOL) - Set text-style flags.
        PM_Shadowed, PM_Center (BOOL) - Enable shadowed or centered text.
        PM_TextPen (ULONG) - Set a custom text pen, stored as an unsigned byte.
        PM_TitleBar, PM_WideTitleBar (BOOL) - Set or clear separator masks,
            which also include the no-selection flag.
        PM_Checkit (BOOL) - Enable checkmark space.
        PM_Checked (BOOL) - Set checked and initial-checked flags. Does not
            write through an existing AutoStore pointer.
        PM_Toggle (BOOL) - Allow a checked item to be toggled off by popup
            selection; initially TRUE.
        PM_AutoStore (BOOL *) - Borrow writable Boolean storage, or detach
            with NULL. Read its current value into the checked flag now.
        PM_Exclude (struct PM_IDLst *) - Attach an ID list. Free the old list
            unless the current ExcludeShared flag is set. See BUGS for NULL.
        PM_ExcludeShared (BOOL) - Suppress freeing the attached ID list when
            TRUE. The caller owns a shared list and must keep it alive.
        PM_ImageSelected, PM_ImageUnselected (struct Image *) - Borrow the
            selected or normal image and enable image mode. If the opposite
            slot is empty, assign the same pointer to it.
        PM_IconSelected, PM_IconUnselected (struct Image *) - Assign the same
            slots but select icon mode; fill an empty opposite slot as above.
        PM_CommKey (char *) - Copy the first byte from a readable pointer.
            NULL leaves the key unchanged. This is not a numeric character.
        PM_ColourBox (ULONG) - Enable a colour box; store the pen as a byte.
        PM_SubConstruct, PM_SubDestruct (struct Hook *) - Borrow submenu hooks.
            Construction receives the parent item and an IPTR message holding
            an opaque handle; return the child list or NULL to cancel.
            Destruction receives the parent item and a NULL message.
        PM_Hidden (BOOL) - Set or clear hidden state.

    NOTES
        Replace title modes only with independent input: an owned old title
        is freed before the new title is copied. The same restriction applies
        to copied user data. Replaced children and owned ID lists are freed
        before assignment. Shared-list flags affect the old list at the time
        of replacement, so tag order changes ownership behaviour.

        Keep borrowed objects, images, hooks and AutoStore storage valid while
        the item uses them. Serialize menu changes with display, traversal
        and callbacks. This function performs no redraw. Dynamic submenu
        hooks run synchronously during popup use; the destructor is also
        called if construction returns NULL. PM_FreePopupMenu() does not
        invoke that destructor.

    BUGS
        PM_Exclude with NULL can free the old list while leaving its address
        attached. Do not use it to detach an existing list.

        BOOPSI layout and rendering are incomplete; PM_Object does not provide
        a usable object-rendering contract. A constructor without an existing
        child list does not enter the submenu-opening path.

    SEE ALSO
        PM_GetItemAttrsA(), PM_MakeItemA(), PM_AbortHook(), PM_FreePopupMenu()

*****************************************************************************/
LONG __saveds ASM PM_SetItemAttrsA(
    register __a2 struct PopupMenu *p GNUCREG(a2),
    register __a1 struct TagItem *tags GNUCREG(a1))
{
    struct TagItem *tag, *tstate;
    LONG count=0;

    if(!p) return 0;

    tstate = tags;
    while((tag=NextTagItem(&tstate))) {
        switch(tag->ti_Tag) {
            case PM_Title:
                PM_FreeTitle(p);
                if(tag->ti_Data) {
                    p->Title=PM_Mem_Alloc(strlen((STRPTR)tag->ti_Data)+1);
                    if(p->Title) {
                        strcpy(p->Title, (STRPTR)tag->ti_Data);
                        count++;
                    } else {
                        p->Title=0L;
                    }
                }
                SET_TXTMODE(p, NPX_TXTNORMAL);
                break;
            case PM_TitleID:
                PM_FreeTitle(p);
                p->TitleID=tag->ti_Data;
                SET_TXTMODE(p, NPX_TXTLOCALE);
                count++;
                break;
            case PM_Object:
                PM_FreeTitle(p);
                p->Boopsi=(Object *)tag->ti_Data;
                SET_TXTMODE(p, NPX_TXTBOOPSI);
                count++;
                break;
            case PM_UserData:
                if((p->Flags&NPM_UDATASTRING) && p->UserData)
                    PM_Mem_Free(p->UserData);
                p->Flags&=~(NPM_UDATASTRING);
                p->UserData=(APTR)tag->ti_Data;
                count++;
                break;
            case PM_UserDataString:
                if((p->Flags&NPM_UDATASTRING) && p->UserData)
                    PM_Mem_Free(p->UserData);
                p->Flags|=NPM_UDATASTRING;
                p->UserData=PM_Mem_Alloc(strlen((STRPTR)tag->ti_Data)+1);
                if(p->UserData) {
                    strcpy(p->UserData, (STRPTR)tag->ti_Data);
                    count++;
                }
                break;
            case PM_ID:
                count++;
                p->ID=tag->ti_Data;
                break;
            case PM_Sub:
                count++;
                if(p->Sub) PM_FreePopupMenu(p->Sub);
                p->Sub=(struct PopupMenu *)tag->ti_Data;
                p->Flags&=~NPM_GROUP;
                p->Layout=0;
                break;
            case PM_Members:
                count++;
                if(p->Sub) PM_FreePopupMenu(p->Sub);
                p->Sub=(struct PopupMenu *)tag->ti_Data;
                if(p->Layout==0) p->Layout=PML_Horizontal;
                p->Flags|=NPM_GROUP;
                break;
            case PM_LayoutMode:
                count++;
                p->Layout=tag->ti_Data;
                if(tag->ti_Data) {
                    p->Flags|=NPM_GROUP;
                } else {
                    p->Flags&=~NPM_GROUP;
                }
                break;
            case PM_Flags:
                count++;
                p->Flags=tag->ti_Data;
                break;
            case PM_NoSelect:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_NOSELECT;
                else {
                    p->Flags&=~NPM_NOSELECT;
                    p->Flags|=NPM_SELECTABLE;
                }
                break;
            case PM_FillPen:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_FILLTEXT;
                else p->Flags&=~NPM_FILLTEXT;
                break;
            case PM_ShadowPen:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_SHADOWTEXT;
                else p->Flags&=~NPM_SHADOWTEXT;
                break;
            case PM_ShinePen:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_SHINETEXT;
                else p->Flags&=~NPM_SHINETEXT;
                break;
            case PM_Checkit:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_CHECKIT;
                else p->Flags&=~NPM_CHECKIT;
                break;
            case PM_Checked:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_CHECKED|NPM_INITIAL_CHECKED;
                else p->Flags&=~(NPM_CHECKED|NPM_INITIAL_CHECKED);
                break;
            case PM_Italic:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_ITALICTEXT;
                else p->Flags&=~NPM_ITALICTEXT;
                break;
            case PM_Bold:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_BOLDTEXT;
                else p->Flags&=~NPM_BOLDTEXT;
                break;
            case PM_Underlined:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_UNDERLINEDTEXT;
                else p->Flags&=~NPM_UNDERLINEDTEXT;
                break;
            case PM_TitleBar:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_HBAR;
                else p->Flags&=~NPM_HBAR;
                break;
            case PM_WideTitleBar:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_WIDE_BAR;
                else p->Flags&=~NPM_WIDE_BAR;
                break;
            case PM_ExcludeShared:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_EXCLUDE_SHARED;
                else p->Flags&=~NPM_EXCLUDE_SHARED;
                break;
            case PM_Exclude:
                count++;
                if(p->Exclude && !(p->Flags&NPM_EXCLUDE_SHARED))
                    PM_FreeIDList(p->Exclude);
                if(tag->ti_Data) p->Exclude=(struct PM_IDLst *)tag->ti_Data;
                break;
            case PM_Disabled:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_DISABLED|NPM_NOSELECT;
                else p->Flags&=~(NPM_DISABLED|NPM_NOSELECT);
                break;
            case PM_ImageSelected:
                count++;
                p->Images[1]=(struct Image *)tag->ti_Data;
                if(!p->Images[0]) p->Images[0]=p->Images[1];
                p->Flags|=NPM_ISIMAGE;
                break;
            case PM_ImageUnselected:
                count++;
                p->Images[0]=(struct Image *)tag->ti_Data;
                if(!p->Images[1]) p->Images[1]=p->Images[0];
                p->Flags|=NPM_ISIMAGE;
                break;
            case PM_IconSelected:
                count++;
                p->Images[1]=(struct Image *)tag->ti_Data;
                if(!p->Images[0]) p->Images[0]=p->Images[1];
                p->Flags&=~NPM_ISIMAGE;
                break;
            case PM_IconUnselected:
                count++;
                p->Images[0]=(struct Image *)tag->ti_Data;
                if(!p->Images[1]) p->Images[1]=p->Images[0];
                p->Flags&=~NPM_ISIMAGE;
                break;
            case PM_AutoStore:
                count++;
                p->AutoSetPtr=(BOOL *)tag->ti_Data;
                if(p->AutoSetPtr) {
                    if(*p->AutoSetPtr) p->Flags|=NPM_CHECKED;
                    else p->Flags&=~NPM_CHECKED;
                }
                break;
            case PM_TextPen:
                count++;
                p->TextPen=tag->ti_Data;
                p->Flags|=NPM_CUSTOMPEN;
                break;
            case PM_Shadowed:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_SHADOWED;
                else p->Flags&=~NPM_SHADOWED;
                break;
            case PM_Center:
                count++;
                if(tag->ti_Data) p->Flags|=NPM_CENTERED;
                else p->Flags&=~NPM_CENTERED;
                break;
            case PM_CommKey:
                count++;
                if(tag->ti_Data) p->CommKey=((char *)tag->ti_Data)[0];
                break;
            case PM_ColourBox:
                count++;
                p->Flags|=NPM_COLOURBOX;
                p->CBox=(UBYTE)tag->ti_Data;
                break;
                        case PM_SubConstruct:
                                p->SubConstruct=(struct Hook *)tag->ti_Data;
                                count++;
                                break;
                        case PM_SubDestruct:
                p->SubDestruct=(struct Hook *)tag->ti_Data;
                count++;
                                break;
            case PM_Hidden:
                if(tag->ti_Data) p->Flags|=NPM_HIDDEN;
                else p->Flags&=~NPM_HIDDEN;
                count++;
                break;
            case PM_Toggle:
                if(tag->ti_Data) p->Flags&=~NPM_NOTOGGLE;
                else p->Flags|=NPM_NOTOGGLE;
                count++;
                break;
        }
    }

    return count;
}
///

/// PM_GetItemAttrsA
/*****************************************************************************
    NAME
        LONG PM_GetItemAttrsA(

    SYNOPSIS
        struct PopupMenu *p, struct TagItem *tags)

    FUNCTION
        Copy supported item attributes into caller-provided storage.

    INPUTS
        p - Item to query, or NULL.
        tags - Terminated tag list, or NULL. Each ti_Data is the address
            of writable storage of the type stated below.

    RESULT
        Number of attributes written. Zero for NULL p. Unsupported tags and
        title queries for an inactive title mode leave storage unchanged.

    TAGS
        PM_Title (STRPTR *) - Borrowed copied title, in text mode only.
        PM_TitleID (ULONG *) - Locale ID, in locale mode only.
        PM_Object (Object **) - Borrowed object, in object mode only.
        PM_UserData (APTR *) - Stored user data, including a copied string.
        PM_ID (ULONG *) - Item ID.
        PM_Sub, PM_Members (struct PopupMenu **) - Borrowed child list.
        PM_LayoutMode, PM_Flags (ULONG *) - Layout value or flags.
        PM_Exclude (struct PM_IDLst **) - Borrowed ID list.
        PM_ImageSelected, PM_IconSelected (struct Image **) - Selected image.
        PM_ImageUnselected, PM_IconUnselected (struct Image **) - Normal image.
        PM_AutoStore (BOOL **) - Borrowed Boolean storage pointer.
        PM_TextPen (ULONG *) - Stored pen number.
        PM_CommKey (char *) - One command-key byte; no terminator is written.
        PM_NoSelect, PM_FillPen, PM_ShadowPen, PM_ShinePen, PM_Checkit,
        PM_Checked, PM_Italic, PM_Bold, PM_Underlined, PM_TitleBar,
        PM_WideTitleBar, PM_ExcludeShared, PM_Disabled, PM_Shadowed,
        PM_Center, PM_Hidden, PM_Toggle (BOOL *) - Corresponding state.

    NOTES
        Tags are processed in order, including repeats. Returned pointers
        are not copied or retained. Item-owned values become invalid when
        replaced or when the item is freed. Keep the item stable during the
        query; the function does not lock it.

    BUGS
        PM_TitleBar and PM_WideTitleBar test masks that include the
        no-selection flag. An unselectable item can therefore report TRUE
        without having the requested separator style.

    SEE ALSO
        PM_SetItemAttrsA(), PM_ItemChecked()

*****************************************************************************/
LONG __saveds ASM PM_GetItemAttrsA(
    register __a2 struct PopupMenu *p GNUCREG(a2),
    register __a1 struct TagItem *tags GNUCREG(a1))
{
    struct TagItem *tag;
    struct TagItem *tstate;
    LONG count=0;

    if(!p) return 0;

    tstate = tags;
    while((tag=NextTagItem(&tstate))) {
        switch(tag->ti_Tag) {
            case PM_Title:
                if(GET_TXTMODE(p)==0) {
                    *((STRPTR *)tag->ti_Data)=p->Title;
                    count++;
                }
                break;
            case PM_TitleID:
                if(GET_TXTMODE(p)==NPX_TXTLOCALE) {
                    *((ULONG *)tag->ti_Data)=p->TitleID;
                    count++;
                }
                break;
            case PM_Object:
                if(GET_TXTMODE(p)==NPX_TXTBOOPSI) {
                    *((Object **)tag->ti_Data)=p->Boopsi;
                    count++;
                }
                break;
            case PM_UserData:
                *((APTR *)tag->ti_Data)=p->UserData;
                count++;
                break;
            case PM_ID:
                count++;
                *((ULONG *)tag->ti_Data)=p->ID;
                break;
            case PM_Members:
            case PM_Sub:
                count++;
                *((struct PopupMenu **)tag->ti_Data)=p->Sub;
                break;
            case PM_LayoutMode:
                count++;
                *((ULONG *)tag->ti_Data)=p->Layout;
                break;
            case PM_Flags:
                count++;
                *((ULONG *)tag->ti_Data)=p->Flags;
                break;
            case PM_NoSelect:
                count++;
                if(p->Flags&NPM_NOSELECT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_FillPen:
                count++;
                if(p->Flags&NPM_FILLTEXT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_ShadowPen:
                count++;
                if(p->Flags&NPM_SHADOWTEXT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_ShinePen:
                count++;
                if(p->Flags&NPM_SHINETEXT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_Checkit:
                count++;
                if(p->Flags&NPM_CHECKIT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_Checked:
                count++;
                if(p->Flags&NPM_CHECKED) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_Italic:
                count++;
                if(p->Flags&NPM_ITALICTEXT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_Bold:
                count++;
                if(p->Flags&NPM_BOLDTEXT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_Underlined:
                count++;
                if(p->Flags&NPM_UNDERLINEDTEXT) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_TitleBar:
                count++;
                if(p->Flags&NPM_HBAR) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_WideTitleBar:
                count++;
                if(p->Flags&NPM_WIDE_BAR) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_ExcludeShared:
                count++;
                if(p->Flags&NPM_EXCLUDE_SHARED) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_Exclude:
                count++;
                *((struct PM_IDLst **)tag->ti_Data)=p->Exclude;
                break;
            case PM_Disabled:
                count++;
                if(p->Flags&NPM_DISABLED) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_ImageSelected:
            case PM_IconSelected:
                count++;
                *((struct Image **)tag->ti_Data)=p->Images[1];
                break;
            case PM_ImageUnselected:
            case PM_IconUnselected:
                count++;
                *((struct Image **)tag->ti_Data)=p->Images[0];
                break;
            case PM_AutoStore:
                count++;
                *((BOOL **)tag->ti_Data)=p->AutoSetPtr;
                break;
            case PM_TextPen:
                count++;
                *((ULONG *)tag->ti_Data)=p->TextPen;
                break;
            case PM_Shadowed:
                if(p->Flags&NPM_SHADOWED) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                count++;
                break;
            case PM_Center:
                if(p->Flags&NPM_CENTERED) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                count++;
                break;
            case PM_CommKey:
                *((STRPTR)tag->ti_Data)=p->CommKey;
                count++;
                break;
            case PM_Hidden:
                count++;
                if(p->Flags&NPM_HIDDEN) *((BOOL *)tag->ti_Data)=TRUE;
                else *((BOOL *)tag->ti_Data)=FALSE;
                break;
            case PM_Toggle:
                count++;
                if(p->Flags&NPM_NOTOGGLE) *((BOOL *)tag->ti_Data)=FALSE;
                else *((BOOL *)tag->ti_Data)=TRUE;
                break;
        }
    }

    return count;
}
///
