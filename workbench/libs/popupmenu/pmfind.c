//
// pmfind.c - Various support functions used to search the menu item tree.
//
// Copyright ©1996 - 2002 Henrik Isaksson
// All Rights Reserved.
//

#include "pmpriv.h"

//
// Find an item
//

// Find a selectable item after a->Selected. If none is found, return NULL.
struct PopupMenu *PM_FindNextSelectable(struct PM_Window *a, struct PopupMenu *pm, BOOL *found)
{
        struct PopupMenu *tmp;

        if(pm) do {
                if(pm->Flags&NPM_GROUP) {
                        tmp=PM_FindNextSelectable(a, pm->Sub, found);
                        if(tmp) {
                                if(*found)
                                        return tmp;
                        }
                }

                if(!(pm->Flags&NPM_NOSELECT) && !(pm->Flags&NPM_HIDDEN) && *found)
                        return pm;

                if(pm==a->Selected) {
                        *found=TRUE;
                }

                pm=pm->Next;
        } while(pm);

        return NULL;
}

// Find the first selectable item in the menu. If none is found, return NULL.
struct PopupMenu *PM_FindFirstSelectable(struct PopupMenu *pm)
{
        struct PopupMenu *tmp;

        if(pm) do {
                if(pm->Flags&NPM_GROUP) {
                        tmp=PM_FindFirstSelectable(pm->Sub);
                        if(tmp) {
                                return tmp;
                        }
                }

                if(!(pm->Flags&NPM_NOSELECT) && !(pm->Flags&NPM_HIDDEN))
                        return pm;

                pm=pm->Next;
        } while(pm);

        return NULL;
}


// Find a selectable item ahead of a->Selected. If none is found, return NULL.
struct PopupMenu *PM_FindPrevSelectable(struct PM_Window *a, struct PopupMenu *pm, BOOL *found)
{
        struct PopupMenu *prv=NULL, *tmp;

        if(pm) do {
                if(pm->Flags&NPM_GROUP) {
                        tmp=PM_FindPrevSelectable(a, pm->Sub, found);
                        if(tmp) {
                                if(*found)
                                        return tmp;
                                prv=tmp;
                        } else if(*found)
                                return prv;
                }

                if(pm==a->Selected) {
                        *found=TRUE;
                        return prv;
                }
                if(!(pm->Flags&NPM_NOSELECT) && !(pm->Flags&NPM_HIDDEN))
                        prv=pm;
                pm=pm->Next;
        } while(pm);

        return prv;
}

// Find the last selectable item in the menu. If none is found, return NULL.
struct PopupMenu *PM_FindLastSelectable(struct PopupMenu *pm)
{
        struct PopupMenu *prv=NULL, *tmp;

        if(pm) do {
                if(pm->Flags&NPM_GROUP) {
                        tmp=PM_FindLastSelectable(pm->Sub);
                        if(tmp) {
                                prv=tmp;
                        }
                } else if(!(pm->Flags&NPM_NOSELECT) && !(pm->Flags&NPM_HIDDEN))
                        prv=pm;

                pm=pm->Next;
        } while(pm);

        return prv;
}

// PM_FindID - Find an item based on it's ID
struct PopupMenu *PM_FindID(struct PopupMenu *base, ULONG ID)
{
        struct PopupMenu *pm=base,*xm;

        while(pm) {
                if(pm->ID==ID) return pm;
                if(pm->Sub) {
                        xm=PM_FindID(pm->Sub, ID);
                        if(xm) return xm;
                }
                pm=pm->Next;
        }
        return NULL;
}

// PM_FindBeforeID - find the item before item with ID in pm
struct PopupMenu *PM_FindBeforeID(struct PopupMenu *pm, ULONG ID)
{
        struct PopupMenu *prev=pm,*xm;

        while(pm) {
                if(pm->ID==ID) return prev;
                if(pm->Sub) {
                        xm=PM_FindBeforeID(pm->Sub, ID);
                        if(xm) return xm;
                }
        		prev=pm;
                pm=pm->Next;
        }
        return NULL;
}

// PM_FindBefore - Find the item before fm in pm
struct PopupMenu *PM_FindBefore(struct PopupMenu *pm, struct PopupMenu *fm)
{
        struct PopupMenu *prev=pm,*xm;

        while(pm) {
                if(pm==fm) return prev;
                if(pm->Sub) {
                        xm=PM_FindBefore(pm->Sub, fm);
                        if(xm) return xm;
                }
        		prev=pm;
                pm=pm->Next;
        }
        return NULL;
}

// PM_FindSortedInsertPoint - find where to insert the item fm in the list pm
struct PopupMenu *PM_FindSortedInsertPoint(struct PopupMenu *pm, struct PopupMenu *fm)
{
        struct PopupMenu *prev=pm;

        while(pm) {
                if(PM_String_Compare(pm->Title, fm->Title) < 0)
                	return prev;
        		prev=pm;
                pm=pm->Next;
        }
        return NULL;
}

// PM_FindLast - Find the end of the list.
struct PopupMenu *PM_FindLast(struct PopupMenu *base)
{
    struct PopupMenu *pm=base;

    while(pm) {
                if(pm->Next==NULL) return pm;
                pm=pm->Next;
    }

    return NULL;
}

// PM_FindItemCommKey - Find an item based on it's CommKey (command key)
struct PopupMenu *PM_FindItemCommKey(struct PopupMenu *base, UBYTE key)
{
        struct PopupMenu *pm=base,*xm;

        while(pm) {
        		if(ToUpper((UBYTE)pm->CommKey)==ToUpper(key)) return pm;

                if(pm->Sub) {
                        xm=PM_FindItemCommKey(pm->Sub, key);
                        if(xm) return xm;
                }
                pm=pm->Next;
        }
        return NULL;
}

// PM_FindItem - find an item based on it's ID
struct PopupMenu * __saveds ASM PM_FindItem(register __a1 struct PopupMenu *menu GNUCREG(a1),
    register __d1 ULONG ID GNUCREG(d1))
{

    if(!menu) return NULL;
        return PM_FindID(menu, ID);
}

//
// See if an item is checked
//
BOOL __saveds ASM PM_ItemChecked(register __a1 struct PopupMenu *pm GNUCREG(a1),
    register __d1 ULONG ID GNUCREG(d1))
{
        struct PopupMenu *p;

        p=PM_FindID(pm, ID);

        if(p) {
                if(p->Flags&NPM_CHECKED) return TRUE;
                return FALSE;
        }
        return -5L;
}

//
// Set/Clear all items in a list
//

void __saveds ASM PM_AlterState(register __a1 struct PopupMenu *base GNUCREG(a1),
    register __a2 struct PM_IDLst *ids GNUCREG(a2),
    register __d1 UWORD action GNUCREG(d1))
{
        struct PopupMenu *pm;
        struct PM_IDLst *id=ids;

        while(id) {
                pm=PM_FindID(base, id->ID);
                if(pm) {
                        if(action==PMACT_SELECT) {
                                if(id->Kind==IDKND_REFLECT) {
                                        pm->Flags|=NPM_CHECKED|NPM_ISSELECTED;
                                        if(pm->AutoSetPtr) *pm->AutoSetPtr=TRUE;
                                } else if(id->Kind==IDKND_INVERSE) {
                                        pm->Flags&=~(NPM_CHECKED|NPM_ISSELECTED);
                                        if(pm->AutoSetPtr) *pm->AutoSetPtr=FALSE;
                                } else if(id->Kind==IDKND_INCLUDE) {
                                        pm->Flags|=NPM_CHECKED|NPM_ISSELECTED;
                                        if(pm->AutoSetPtr) *pm->AutoSetPtr=TRUE;
                                } else if(id->Kind==IDKND_EXCLUDE) {
                                        pm->Flags&=~(NPM_CHECKED|NPM_ISSELECTED);
                                        if(pm->AutoSetPtr) *pm->AutoSetPtr=FALSE;
                                }
                        } else  if(action==PMACT_DESELECT) {
                                if(id->Kind==IDKND_INVERSE) {
                                        pm->Flags|=NPM_CHECKED|NPM_ISSELECTED;
                                        if(pm->AutoSetPtr) *pm->AutoSetPtr=TRUE;
                                } else if(id->Kind==IDKND_REFLECT) {
                                        pm->Flags&=~(NPM_CHECKED|NPM_ISSELECTED);
                                        if(pm->AutoSetPtr) *pm->AutoSetPtr=FALSE;
                                }
                        }
                }
                id=id->Next;
        }
}

