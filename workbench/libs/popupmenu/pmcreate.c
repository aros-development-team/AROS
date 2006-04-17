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

void __saveds ASM PM_FreeIDList(
    register __a0 struct PM_IDLst *f GNUCREG(a0))
{
    FreeIDList(f);
}

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

struct PM_IDLst * __saveds ASM PM_MakeIDListA(register __a1 struct TagItem *tags GNUCREG(a1))
{
    struct TagItem *tag, *tstate;
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
