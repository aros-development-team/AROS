//
// PopupMenu
// ©1996-2002 Henrik Isaksson
//
// Menu creation/disposal & id list functions
//

#include "pmpriv.h"

void FreeIDList(struct PM_IDLst *f)
{
    if(f) {
        struct PM_IDLst *n;

        while(f) {
            n=f->Next;
            PM_Mem_Free(f);
            f=n;
        }
    }
}

void PM_FreeTitle(struct PopupMenu *p)
{
    if(!p) return;

    if(p->Title && GET_TXTMODE(p)==0) PM_Mem_Free(p->Title);

    p->Title=NULL;
}

/*****************************************************************************
    NAME
        void PM_FreePopupMenu(

    SYNOPSIS
        struct PopupMenu *p)

    FUNCTION
        Free an item, all following siblings and every descendant. Free
        copied titles, copied user-data strings and owned ID lists.

    INPUTS
        p - First item to free, or NULL.

    RESULT
        None.

    NOTES
        Detach an item before freeing it if following siblings must survive.
        Do not free a menu while it is displayed or used by a callback.
        Objects, images, hooks, AutoStore targets, ordinary UserData and
        shared exclusion lists remain caller-owned. SubDestruct hooks are
        not called. All pointers to freed items and their owned data become
        invalid. Keep popupmenu.library open until disposal completes.

    SEE ALSO
        PM_RemoveMenuItem(), PM_SetItemAttrsA(), PM_FreeIDList()

*****************************************************************************/
void __saveds ASM PM_FreePopupMenu(register __a1 struct PopupMenu *p GNUCREG(a1))
{
    if(p) {
        struct PopupMenu *n;

        while(p) {
            n=p->Next;
            if(p->Sub) PM_FreePopupMenu(p->Sub);
            PM_FreeTitle(p);
            if(p->UserData && (p->Flags&NPM_UDATASTRING)) PM_Mem_Free(p->UserData);
            if(p->Exclude && !(p->Flags&NPM_EXCLUDE_SHARED)) {
                FreeIDList(p->Exclude);
            }
            PM_Mem_Free(p);
            p=n;
        }
    }
}

/*****************************************************************************
    NAME
        void PM_FreeIDList(

    SYNOPSIS
        struct PM_IDLst *list)

    FUNCTION
        Free every node of an ID list allocated by popupmenu.library.

    INPUTS
        list - First node, or NULL.

    RESULT
        None.

    NOTES
        Do not free a list still owned by an item. A shared list must remain
        valid until every referring item has released it; its owner then
        frees it once. Keep popupmenu.library open until disposal completes.

    SEE ALSO
        PM_MakeIDListA(), PM_ExLstA(), PM_SetItemAttrsA()

*****************************************************************************/
void __saveds ASM PM_FreeIDList(
    register __a0 struct PM_IDLst *f GNUCREG(a0))
{
    FreeIDList(f);
}

/*****************************************************************************
    NAME
        struct PopupMenu * PM_MakeItemA(

    SYNOPSIS
        struct TagItem *tags)

    FUNCTION
        Allocate a cleared menu item and apply its attributes in tag order.

    INPUTS
        tags - Tag list accepted by PM_SetItemAttrsA(), or NULL.

    RESULT
        The new item, or NULL if the item allocation fails.

    NOTES
        The caller owns the item until it is linked into an owning menu.
        Release an unlinked item with PM_FreePopupMenu(). Attribute data
        ownership follows PM_SetItemAttrsA(). Keep popupmenu.library open
        until its items and ID lists have been released.

    BUGS
        Attribute allocation failures do not make this function return NULL.
        A returned item can have an unset title or copied user-data string.

    SEE ALSO
        PM_SetItemAttrsA(), PM_MakeMenuA(), PM_FreePopupMenu()

*****************************************************************************/
struct PopupMenu *__saveds ASM PM_MakeItemA(register __a1 struct TagItem *tags GNUCREG(a1))
{
    struct PopupMenu *p;

    p=PM_Mem_Alloc(sizeof(struct PopupMenu));
    if(p) {
        PM_SetItemAttrsA(p, tags);

        return p;
    }

    return NULL;
}

/*****************************************************************************
    NAME
        struct PopupMenu * PM_MakeMenuA(

    SYNOPSIS
        struct TagItem *tags)

    FUNCTION
        Link existing items into a sibling list in tag order. No container
        item is allocated.

    INPUTS
        tags - Terminated tag list, or NULL.

    RESULT
        The first item, or NULL for an empty list or a NULL PM_Item.

    TAGS
        PM_Item (struct PopupMenu *) - Item to append. May be repeated.
            Supply distinct, unlinked items. Existing child lists remain
            attached. Other application tags are ignored.

    NOTES
        Ownership of the supplied items passes to the resulting menu. If
        any PM_Item is NULL, the scan continues and the entire linked result
        is freed, including items supplied after the NULL tag. Do not free
        those items again. Dispose of a successful result with
        PM_FreePopupMenu(). A menu must be acyclic and must not share owned
        items with another menu.

    SEE ALSO
        PM_MakeItemA(), PM_FreePopupMenu()

*****************************************************************************/
struct PopupMenu * __saveds ASM PM_MakeMenuA(register __a1 struct TagItem *tags GNUCREG(a1))
{
    struct TagItem *tag, *tstate;
    struct PopupMenu *first=0L, *last=0L;
    BOOL error=0;

    tstate = tags;
    while((tag=NextTagItem(&tstate))) {
        switch(tag->ti_Tag) {
            case PM_Item:
                if(tag->ti_Data) {
                    if(last) {
                        last->Next=(struct PopupMenu *)tag->ti_Data;
                        last=last->Next;
                    } else {
                        last=first=(struct PopupMenu *)tag->ti_Data;
                    }
                } else error=1;
                break;
        }
    }

    if(error) {
        PM_FreePopupMenu(first);
        first=0L;
    }

    return first;
}

/*****************************************************************************
    NAME
        struct PM_IDLst * PM_MakeIDListA(

    SYNOPSIS
        struct TagItem *tags)

    FUNCTION
        Allocate an ordered list of item IDs and state operations.

    INPUTS
        tags - Terminated tag list, or NULL.

    RESULT
        The new list, or NULL if empty or if any allocation fails. On failure,
        all nodes allocated by this call are freed.

    TAGS
        PM_ExcludeID (ULONG) - Clear this ID when the source is selected.
        PM_IncludeID (ULONG) - Check this ID when the source is selected.
        PM_ReflectID (ULONG) - Check on selection; clear on deselection.
        PM_InverseID (ULONG) - Clear on selection; check on deselection.
        Each tag may repeat. Zero is a valid ID. Other tags are ignored.

    NOTES
        The list contains IDs, not references to menu items. Missing IDs are
        skipped when the list is applied. The caller owns the list until it
        is assigned as an owned PM_Exclude attribute. Otherwise release it
        with PM_FreeIDList().

    SEE ALSO
        PM_AlterState(), PM_ExLstA(), PM_SetItemAttrsA(), PM_FreeIDList()

*****************************************************************************/
struct PM_IDLst * __saveds ASM PM_MakeIDListA(register __a1 struct TagItem *tags GNUCREG(a1))
{
    struct TagItem *tag;
    struct TagItem *tstate;
    struct PM_IDLst *first=0L, *last=0L, *n=0L;
    BOOL error=0;

    tstate = tags;
    while((tag=NextTagItem(&tstate))) {
        switch(tag->ti_Tag) {
            case PM_ExcludeID:
                n=PM_Mem_Alloc(sizeof(struct PM_IDLst));
                if(n) {
                    n->Next=0L;
                    n->ID=tag->ti_Data;
                    n->Kind=IDKND_EXCLUDE;
                    n->Flags=0L;

                    if(last) {
                        last->Next=n;
                        last=last->Next;
                    } else {
                        last=first=n;
                    }
                } else error=1;
                break;
            case PM_IncludeID:
                n=PM_Mem_Alloc(sizeof(struct PM_IDLst));
                if(n) {
                    n->Next=0L;
                    n->ID=tag->ti_Data;
                    n->Kind=IDKND_INCLUDE;
                    n->Flags=0L;

                    if(last) {
                        last->Next=n;
                        last=last->Next;
                    } else {
                        last=first=n;
                    }
                } else error=1;
                break;
            case PM_ReflectID:
                n=PM_Mem_Alloc(sizeof(struct PM_IDLst));
                if(n) {
                    n->Next=0L;
                    n->ID=tag->ti_Data;
                    n->Kind=IDKND_REFLECT;
                    n->Flags=0L;

                    if(last) {
                        last->Next=n;
                        last=last->Next;
                    } else {
                        last=first=n;
                    }
                } else error=1;
                break;
            case PM_InverseID:
                n=PM_Mem_Alloc(sizeof(struct PM_IDLst));
                if(n) {
                    n->Next=0L;
                    n->ID=tag->ti_Data;
                    n->Kind=IDKND_INVERSE;
                    n->Flags=0L;

                    if(last) {
                        last->Next=n;
                        last=last->Next;
                    } else {
                        last=first=n;
                    }
                } else error=1;
                break;
        }
    }

    if(error) {
        PM_FreeIDList(first);
        first=0L;
    }

    return first;
}

/*****************************************************************************
    NAME
        struct PM_IDLst * PM_ExLstA(

    SYNOPSIS
        ULONG *id)

    FUNCTION
        Create an exclusion list from an array of item IDs.

    INPUTS
        id - Non-NULL readable ULONG array terminated by zero.

    RESULT
        The new list, or NULL for an empty array or any allocation failure.
        On failure, all nodes allocated by this call are freed.

    NOTES
        IDs are copied in order; the array is not retained. Zero cannot be
        represented in this form. Use PM_MakeIDListA() for ID zero or other
        operation kinds. Ownership follows PM_MakeIDListA().

    SEE ALSO
        PM_MakeIDListA(), PM_FreeIDList()

*****************************************************************************/
struct PM_IDLst * __saveds ASM PM_ExLstA(register __a1 ULONG *id GNUCREG(a1))
{
    struct PM_IDLst *first=0L, *last=0L, *n=0L;
    BOOL error=0;
    int i=0;

    while(id[i]) {
        n=PM_Mem_Alloc(sizeof(struct PM_IDLst));
        if(n) {
            n->Next=0L;
            n->ID=id[i];
            n->Kind=IDKND_EXCLUDE;
            n->Flags=0L;

            if(last) {
                last->Next=n;
                last=last->Next;
            } else {
                last=first=n;
            }
        } else error=1;
        i++;
    }

    if(error) {
        PM_FreeIDList(first);
        first=0L;
    }

    return first;
}

//
// Allocate local variables to save stack
//
struct PM_Root *PM_AllocPMRoot(struct Window *w)
{
        struct PM_Root *p;

        p=PM_Mem_Alloc(sizeof(struct PM_Root));
        if(p) {
                p->ShadowWidth=p->ShadowHeight=4;
                p->ShadowAddX=p->ShadowAddY=2;

                p->BorderWidth=p->BorderHeight=1;

                p->DrawInfo=GetScreenDrawInfo(w->WScreen);
                
                p->PM=0L;

                return p;
        }
        return NULL;
}
