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
LONG __saveds ASM PM_GetItemAttrsA(
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
