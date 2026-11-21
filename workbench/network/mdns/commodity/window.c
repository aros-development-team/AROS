/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * window.c - NetServices' window: Nearby (the services, or the kinds of
 * service, on the network), Handshake (how to reach one) and Share (a
 * service of this computer), and a status line, in GadTools gadgets.
 *
 * The layout is measured in the screen's font: every gadget is as wide as
 * its longest text, in the user's language, and the labels have room of
 * their own. Nearby and Handshake are side by side, or one above the other
 * when that is too wide for the screen; when neither fits, the window is
 * laid out again in topaz 8.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <libraries/gadtools.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>

#include "netservices.h"
#include "../tools/sd_types.h"

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#include "../strings.h"

struct MsgPort *NSWindowPort;

/* Gadgets, by what they do (their GadgetID). */
enum
{
    G_NB_KIND, G_NB_BROWSE, G_NB_STOP, G_NB_LIST,
    G_HS_NAME, G_HS_KIND, G_HS_GO, G_HS_RESULT,
    G_SH_NAME, G_SH_KIND, G_SH_PORT, G_SH_INFO, G_SH_GO, G_SH_STOP,
    G_STATUS, G_COUNT
};

#define CMD_HIDE    1
#define CMD_QUIT    2

#define NS_LIST_ROWS    10      /* the Nearby list's height, in lines, at most */
#define NS_LIST_MIN     3       /* and at least, before giving up a layout */
#define NS_KINDS_MAX    40      /* the types a kind gadget offers */

/*
 * A kind gadget's choices: the types sd_types knows, in words; Nearby's
 * starts with all kinds; the last may be a type it does not know, as the
 * type itself, after one was picked from the list.
 */
struct NSKinds
{
    STRPTR      nk_Labels[NS_KINDS_MAX + 3];
    CONST_STRPTR nk_Types[NS_KINDS_MAX + 3];
    LONG        nk_Known;       /* choices without the one of the list */
    char        nk_Other[256];
};

static struct NSKinds NearbyKinds, ReachKinds, ShareKinds;

static struct Window   *Win;
static struct Screen   *Scr;
static APTR             VisInfo;
static struct Menu     *Menus;
static struct Gadget   *GadList;
static struct Gadget   *Gad[G_COUNT];
static struct TextAttr *Font;
static struct TextFont *FontOpen;
static char             Title[96];
static WORD             WinLeft = -1, WinTop = -1;

/* What the fields held when the window closed, for when it opens again. */
static struct
{
    char    hs_Name[64];
    char    hs_Type[256];
    char    sh_Name[64];
    char    sh_Type[256];
    LONG    sh_Port;
    char    sh_Info[256];
} Kept = { "", "_http._tcp", "", "_http._tcp", 80, "" };

/* The section frames and their titles, where the layout put them. */
struct NSFrame
{
    WORD            nf_Left, nf_Top, nf_Width, nf_Height;
    CONST_STRPTR    nf_Title;
};
static struct NSFrame Frames[3];

static struct TextAttr Topaz8 = { (STRPTR)"topaz.font", 8, FS_NORMAL, FPF_ROMFONT };

static struct NewMenu MenuSpec[] =
{
    { NM_TITLE, NULL, NULL, 0, 0, NULL },
    { NM_ITEM,  NULL, NULL, 0, 0, (APTR)CMD_HIDE },
    { NM_ITEM,  NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM,  NULL, NULL, 0, 0, (APTR)CMD_QUIT },
    { NM_END,   NULL, NULL, 0, 0, NULL }
};

/*------------------------------------------------------------------------*/
/* The kinds */

static void
ns_kinds(struct NSKinds *k, BOOL all)
{
    LONG n = 0, i;
    CONST_STRPTR type;

    if (all) {
        k->nk_Labels[n] = (STRPTR)SDMsg(MSG_NS_ALL_KINDS);
        k->nk_Types[n++] = (CONST_STRPTR)SD_TYPE_SERVICES;
    }
    for (i = 0; n < NS_KINDS_MAX && (type = SDTypeAt(i)) != NULL; i++) {
        k->nk_Labels[n] = (STRPTR)SDTypeFriendly(type);
        k->nk_Types[n++] = type;
    }
    k->nk_Known = n;
    k->nk_Labels[n] = NULL;
    k->nk_Types[n] = NULL;
}

/* The choice of a type: a known one, else the type itself, added last. */
static LONG
ns_kind_of(struct NSKinds *k, CONST_STRPTR type)
{
    LONG i;

    for (i = 0; i < k->nk_Known; i++) {
        if (SDStrEqualCI((const char *)k->nk_Types[i], (const char *)type)) {
            k->nk_Labels[k->nk_Known] = NULL;
            return i;
        }
    }
    SDStrCopy(k->nk_Other, (const char *)type, (LONG)sizeof(k->nk_Other));
    k->nk_Labels[i] = (STRPTR)k->nk_Other;
    k->nk_Types[i] = (CONST_STRPTR)k->nk_Other;
    k->nk_Labels[i + 1] = NULL;
    return i;
}

static IPTR
ns_get(LONG id, Tag tag)
{
    IPTR value = 0;

    GT_GetGadgetAttrs(Gad[id], Win, NULL, tag, (IPTR)&value, TAG_DONE);
    return value;
}

static CONST_STRPTR
ns_kind_chosen(const struct NSKinds *k, LONG id)
{
    return k->nk_Types[ns_get(id, GTCY_Active)];
}

static CONST_STRPTR
ns_string(LONG id)
{
    return (CONST_STRPTR)((struct StringInfo *)Gad[id]->SpecialInfo)->Buffer;
}

/*------------------------------------------------------------------------*/
/* The layout */

struct NSMeasure
{
    struct RastPort nm_RP;
    WORD            nm_Font;        /* the font's height */
    WORD            nm_Gadget;      /* a gadget's height */
    WORD            nm_Space;
};

static WORD
ns_text(struct NSMeasure *m, CONST_STRPTR s)
{
    return (WORD)TextLength(&m->nm_RP, (STRPTR)s, (ULONG)SDStrLen((const char *)s));
}

static WORD
ns_button(struct NSMeasure *m, CONST_STRPTR text)
{
    return (WORD)(ns_text(m, text) + 4 * m->nm_Space);
}

static WORD
ns_cycle(struct NSMeasure *m, struct NSKinds *k)
{
    WORD w = 0;
    LONG i;

    for (i = 0; k->nk_Labels[i] != NULL; i++) {
        WORD t = ns_text(m, (CONST_STRPTR)k->nk_Labels[i]);

        if (t > w) {
            w = t;
        }
    }
    /* the cycle glyph and its bar */
    return (WORD)(w + 2 * m->nm_Gadget + 2 * m->nm_Space);
}

static WORD
ns_max(WORD a, WORD b)
{
    return a > b ? a : b;
}

/* Every position of the gadgets, in the font ta, from the window's inner
 * top-left at (left, top); the inner size it needs. */
struct NSLayout
{
    struct NewGadget    nl_Gadget[G_COUNT];
    WORD                nl_Width, nl_Height;
};

/* label: the text left of the gadget, or NULL */
static void
ns_place(struct NSLayout *l, LONG id, WORD x, WORD y, WORD w, WORD h,
    CONST_STRPTR label)
{
    struct NewGadget *ng = &l->nl_Gadget[id];

    ng->ng_LeftEdge = x;
    ng->ng_TopEdge = y;
    ng->ng_Width = w;
    ng->ng_Height = h;
    ng->ng_GadgetText = (STRPTR)label;
    ng->ng_GadgetID = (UWORD)id;
    ng->ng_Flags = label != NULL ? PLACETEXT_LEFT : 0;
}

static void
ns_button_at(struct NSLayout *l, LONG id, WORD x, WORD y, WORD w, WORD h,
    CONST_STRPTR text)
{
    ns_place(l, id, x, y, w, h, NULL);
    l->nl_Gadget[id].ng_GadgetText = (STRPTR)text;
    l->nl_Gadget[id].ng_Flags = PLACETEXT_IN;
}

/*
 * Nearby and Handshake side by side, or, stacked, Handshake under Nearby;
 * Share and the status line under them, as wide as the window. rows: the
 * lines of the Nearby list (and, stacked, of the Handshake result); compact:
 * less room between the gadgets, for a screen as small as 640 x 256.
 */
static void
ns_layout(struct NSLayout *l, struct TextAttr *ta, struct TextFont *tf,
    WORD left, WORD top, BOOL stacked, WORD rows, BOOL compact)
{
    struct NSMeasure m;
    WORD sp, gh, fh, lab, labS, col, x, y, w, listH, resultH, nearbyH, shareW;
    WORD kindW, kindAllW, browseW, stopW, reachW, shareBtnW, nameW, portW, infoW;
    WORD frameTop, frameIn, frameW;

    InitRastPort(&m.nm_RP);
    SetFont(&m.nm_RP, tf);
    fh = m.nm_Font = (WORD)tf->tf_YSize;
    sp = m.nm_Space = compact ? 2 : (WORD)(fh / 2 > 3 ? fh / 2 : 3);
    gh = m.nm_Gadget = (WORD)(fh + 6);
    frameIn = (WORD)(2 * sp);           /* the gadgets inside a frame */

    kindAllW = ns_cycle(&m, &NearbyKinds);
    kindW = ns_cycle(&m, &ReachKinds);
    browseW = ns_button(&m, SDMsg(MSG_NS_BROWSE));
    stopW = ns_button(&m, SDMsg(MSG_NS_STOP));
    reachW = ns_button(&m, SDMsg(MSG_NS_REACH));
    shareBtnW = ns_button(&m, SDMsg(MSG_NS_SHARE));
    /* a label's room: its text and a space before the gadget */
    lab = ns_max(ns_text(&m, SDMsg(MSG_NS_NAME)), ns_text(&m, SDMsg(MSG_NS_KIND)));
    lab = (WORD)(lab + 2 * sp);
    labS = ns_max(lab, (WORD)(ns_max(ns_text(&m, SDMsg(MSG_NS_PORT_LABEL)),
        ns_text(&m, SDMsg(MSG_NS_INFO_LABEL))) + 2 * sp));
    nameW = (WORD)(16 * ns_text(&m, (CONST_STRPTR)"n"));
    portW = (WORD)(ns_text(&m, (CONST_STRPTR)"000000") + 3 * sp);
    infoW = (WORD)(12 * ns_text(&m, (CONST_STRPTR)"n"));

    /* a column: as wide as the widest of its rows */
    col = ns_max((WORD)(kindAllW + sp + browseW + sp + stopW),
        (WORD)(lab + kindW + sp + reachW));
    col = ns_max(col, (WORD)(28 * ns_text(&m, (CONST_STRPTR)"n")));
    col = ns_max(col, ns_text(&m, SDMsg(MSG_NS_NEARBY_TITLE)));
    col = ns_max(col, ns_text(&m, SDMsg(MSG_NS_HANDSHAKE_TITLE)));
    /* Share's two rows, in its frame */
    shareW = ns_max((WORD)(labS + nameW + sp + lab + kindW),
        (WORD)(labS + portW + sp + labS + infoW + sp + shareBtnW + sp + stopW));
    shareW = ns_max((WORD)(shareW + 2 * frameIn), ns_text(&m, SDMsg(MSG_NS_SHARE_TITLE)));
    /* the frames above Share: as wide as it, at least */
    if (stacked) {
        frameW = ns_max((WORD)(col + 2 * frameIn), shareW);
        shareW = frameW;
    } else {
        frameW = ns_max((WORD)(col + 2 * frameIn), (WORD)((shareW - sp + 1) / 2));
        shareW = (WORD)(2 * frameW + sp);
    }
    col = (WORD)(frameW - 2 * frameIn);

    /* stacked, the lists are shorter: the window is taller */
    listH = (WORD)(rows * (fh + 1) + 4);
    resultH = stacked ? listH : (WORD)(listH - gh - sp);
    nearbyH = (WORD)(frameIn + gh + sp + listH + frameIn);
    x = (WORD)(left + sp);
    y = (WORD)(top + sp);

    /* Nearby */
    frameTop = (WORD)(y + fh + sp);
    Frames[0].nf_Left = x;
    Frames[0].nf_Top = frameTop;
    Frames[0].nf_Width = frameW;
    Frames[0].nf_Height = nearbyH;
    Frames[0].nf_Title = SDMsg(MSG_NS_NEARBY_TITLE);
    w = (WORD)(col - browseW - sp - stopW - sp);
    ns_place(l, G_NB_KIND, (WORD)(x + frameIn), (WORD)(frameTop + frameIn), w, gh, NULL);
    ns_button_at(l, G_NB_BROWSE, (WORD)(x + frameIn + w + sp), (WORD)(frameTop + frameIn),
        browseW, gh, SDMsg(MSG_NS_BROWSE));
    ns_button_at(l, G_NB_STOP, (WORD)(x + frameIn + col - stopW),
        (WORD)(frameTop + frameIn),
        stopW, gh, SDMsg(MSG_NS_STOP));
    ns_place(l, G_NB_LIST, (WORD)(x + frameIn), (WORD)(frameTop + frameIn + gh + sp),
        col, listH, NULL);

    /* Handshake: beside Nearby, or under it */
    if (stacked) {
        frameTop = (WORD)(frameTop + nearbyH + sp + fh + sp);
    } else {
        x = (WORD)(x + frameW + sp);
    }
    Frames[1].nf_Left = x;
    Frames[1].nf_Top = frameTop;
    Frames[1].nf_Width = frameW;
    Frames[1].nf_Height = (WORD)(frameIn + 2 * (gh + sp) + resultH + frameIn);
    Frames[1].nf_Title = SDMsg(MSG_NS_HANDSHAKE_TITLE);
    y = (WORD)(frameTop + frameIn);
    ns_place(l, G_HS_NAME, (WORD)(x + frameIn + lab), y, (WORD)(col - lab), gh,
        SDMsg(MSG_NS_NAME));
    y = (WORD)(y + gh + sp);
    w = (WORD)(col - lab - reachW - sp);
    ns_place(l, G_HS_KIND, (WORD)(x + frameIn + lab), y, w, gh, SDMsg(MSG_NS_KIND));
    ns_button_at(l, G_HS_GO, (WORD)(x + frameIn + lab + w + sp), y, reachW, gh,
        SDMsg(MSG_NS_REACH));
    y = (WORD)(y + gh + sp);
    ns_place(l, G_HS_RESULT, (WORD)(x + frameIn), y, col, resultH, NULL);

    /* Share */
    x = (WORD)(left + sp);
    y = (WORD)(frameTop + Frames[1].nf_Height + sp);
    frameTop = (WORD)(y + fh + sp);
    Frames[2].nf_Left = x;
    Frames[2].nf_Top = frameTop;
    Frames[2].nf_Width = shareW;
    Frames[2].nf_Title = SDMsg(MSG_NS_SHARE_TITLE);
    y = (WORD)(frameTop + frameIn);
    w = (WORD)(shareW - 2 * frameIn - labS - sp - lab - kindW);
    ns_place(l, G_SH_NAME, (WORD)(x + frameIn + labS), y, w, gh, SDMsg(MSG_NS_NAME));
    ns_place(l, G_SH_KIND, (WORD)(x + shareW - frameIn - kindW), y, kindW, gh,
        SDMsg(MSG_NS_KIND));
    y = (WORD)(y + gh + sp);
    ns_place(l, G_SH_PORT, (WORD)(x + frameIn + labS), y, portW, gh,
        SDMsg(MSG_NS_PORT_LABEL));
    w = (WORD)(shareW - 2 * frameIn - labS - portW - sp - labS - sp - shareBtnW - sp -
        stopW);
    ns_place(l, G_SH_INFO, (WORD)(x + frameIn + labS + portW + sp + labS), y, w, gh,
        SDMsg(MSG_NS_INFO_LABEL));
    ns_button_at(l, G_SH_GO, (WORD)(x + shareW - frameIn - stopW - sp - shareBtnW), y,
        shareBtnW, gh, SDMsg(MSG_NS_SHARE));
    ns_button_at(l, G_SH_STOP, (WORD)(x + shareW - frameIn - stopW), y, stopW, gh,
        SDMsg(MSG_NS_STOP));
    Frames[2].nf_Height = (WORD)(y + gh + frameIn - frameTop);

    /* the status line */
    y = (WORD)(frameTop + Frames[2].nf_Height + sp);
    ns_place(l, G_STATUS, x, y, shareW, gh, NULL);

    for (w = 0; w < G_COUNT; w++) {
        l->nl_Gadget[w].ng_TextAttr = ta;
        l->nl_Gadget[w].ng_VisualInfo = VisInfo;
        l->nl_Gadget[w].ng_UserData = NULL;
    }
    l->nl_Width = (WORD)(shareW + 2 * sp);
    l->nl_Height = (WORD)(y + gh + sp - top);
}

/*------------------------------------------------------------------------*/
/* The gadgets */

static BOOL
ns_gadgets(struct NSLayout *l)
{
    struct Gadget *g = CreateContext(&GadList);
    struct NewGadget *ng = l->nl_Gadget;

    Gad[G_NB_KIND] = g = CreateGadget(CYCLE_KIND, g, &ng[G_NB_KIND],
        GTCY_Labels, (IPTR)NearbyKinds.nk_Labels, TAG_DONE);
    Gad[G_NB_BROWSE] = g = CreateGadget(BUTTON_KIND, g, &ng[G_NB_BROWSE], TAG_DONE);
    Gad[G_NB_STOP] = g = CreateGadget(BUTTON_KIND, g, &ng[G_NB_STOP], TAG_DONE);
    Gad[G_NB_LIST] = g = CreateGadget(LISTVIEW_KIND, g, &ng[G_NB_LIST],
        GTLV_Labels, (IPTR)&NSD.ds_Nearby, GTLV_ShowSelected, (IPTR)NULL, TAG_DONE);
    Gad[G_HS_NAME] = g = CreateGadget(STRING_KIND, g, &ng[G_HS_NAME],
        GTST_MaxChars, 63, GTST_String, (IPTR)Kept.hs_Name, TAG_DONE);
    Gad[G_HS_KIND] = g = CreateGadget(CYCLE_KIND, g, &ng[G_HS_KIND],
        GTCY_Labels, (IPTR)ReachKinds.nk_Labels,
        GTCY_Active, ns_kind_of(&ReachKinds, (CONST_STRPTR)Kept.hs_Type), TAG_DONE);
    Gad[G_HS_GO] = g = CreateGadget(BUTTON_KIND, g, &ng[G_HS_GO], TAG_DONE);
    Gad[G_HS_RESULT] = g = CreateGadget(LISTVIEW_KIND, g, &ng[G_HS_RESULT],
        GTLV_Labels, (IPTR)&NSD.ds_Result, GTLV_ReadOnly, TRUE, TAG_DONE);
    Gad[G_SH_NAME] = g = CreateGadget(STRING_KIND, g, &ng[G_SH_NAME],
        GTST_MaxChars, 63, GTST_String,
        (IPTR)(Kept.sh_Name[0] != '\0' ? (CONST_STRPTR)Kept.sh_Name :
            SDMsg(MSG_NS_SHARE_DEFAULT)),
        TAG_DONE);
    Gad[G_SH_KIND] = g = CreateGadget(CYCLE_KIND, g, &ng[G_SH_KIND],
        GTCY_Labels, (IPTR)ShareKinds.nk_Labels,
        GTCY_Active, ns_kind_of(&ShareKinds, (CONST_STRPTR)Kept.sh_Type), TAG_DONE);
    Gad[G_SH_PORT] = g = CreateGadget(INTEGER_KIND, g, &ng[G_SH_PORT],
        GTIN_MaxChars, 5, GTIN_Number, Kept.sh_Port, TAG_DONE);
    Gad[G_SH_INFO] = g = CreateGadget(STRING_KIND, g, &ng[G_SH_INFO],
        GTST_MaxChars, 255, GTST_String, (IPTR)Kept.sh_Info, TAG_DONE);
    Gad[G_SH_GO] = g = CreateGadget(BUTTON_KIND, g, &ng[G_SH_GO], TAG_DONE);
    Gad[G_SH_STOP] = g = CreateGadget(BUTTON_KIND, g, &ng[G_SH_STOP], TAG_DONE);
    Gad[G_STATUS] = g = CreateGadget(TEXT_KIND, g, &ng[G_STATUS],
        GTTX_Border, TRUE, GTTX_Text, (IPTR)NSD.ds_Status, TAG_DONE);
    return g != NULL;
}

static void
ns_frames(void)
{
    struct RastPort *rp = Win->RPort;
    struct DrawInfo *dri = GetScreenDrawInfo(Scr);
    LONG i;

    SetFont(rp, FontOpen);
    SetDrMd(rp, JAM1);
    for (i = 0; i < 3; i++) {
        struct NSFrame *f = &Frames[i];
        CONST_STRPTR t = f->nf_Title;

        DrawBevelBox(rp, f->nf_Left, f->nf_Top, f->nf_Width, f->nf_Height,
            GT_VisualInfo, (IPTR)VisInfo, GTBB_Recessed, TRUE, TAG_DONE);
        if (dri != NULL) {
            SetAPen(rp, dri->dri_Pens[HIGHLIGHTTEXTPEN]);
        }
        Move(rp, f->nf_Left, (WORD)(f->nf_Top - rp->TxHeight - 2 + rp->TxBaseline));
        Text(rp, (STRPTR)t, (ULONG)SDStrLen((const char *)t));
    }
    if (dri != NULL) {
        FreeScreenDrawInfo(Scr, dri);
    }
}

/*------------------------------------------------------------------------*/
/* What the window shows */

static void
ns_set(LONG id, Tag tag, IPTR value)
{
    GT_SetGadgetAttrs(Gad[id], Win, NULL, tag, value, TAG_DONE);
}

void
NSWindowDetach(void)
{
    if (Win != NULL) {
        ns_set(G_NB_LIST, GTLV_Labels, (IPTR)~0);
        ns_set(G_HS_RESULT, GTLV_Labels, (IPTR)~0);
    }
}

void
NSWindowAttach(void)
{
    if (Win != NULL) {
        /* rows come and go: no row stays picked under another */
        GT_SetGadgetAttrs(Gad[G_NB_LIST], Win, NULL, GTLV_Labels, (IPTR)&NSD.ds_Nearby,
            GTLV_Selected, ~0, TAG_DONE);
        ns_set(G_HS_RESULT, GTLV_Labels, (IPTR)&NSD.ds_Result);
    }
}

void
NSWindowShow(ULONG changed)
{
    if (Win == NULL) {
        return;
    }
    if (changed & NS_CHANGED_STATE) {
        BOOL sharing = NSD.ds_Share != NULL;

        ns_set(G_NB_STOP, GA_Disabled, !NSD.ds_Browsing);
        ns_set(G_HS_GO, GA_Disabled, NSD.ds_Resolve != NULL);
        ns_set(G_SH_NAME, GA_Disabled, sharing);
        ns_set(G_SH_KIND, GA_Disabled, sharing);
        ns_set(G_SH_PORT, GA_Disabled, sharing);
        ns_set(G_SH_INFO, GA_Disabled, sharing);
        ns_set(G_SH_GO, GA_Disabled, sharing);
        ns_set(G_SH_STOP, GA_Disabled, !sharing);
    }
    if (changed & NS_CHANGED_STATUS) {
        ns_set(G_STATUS, GTTX_Text, (IPTR)NSD.ds_Status);
    }
}

/*------------------------------------------------------------------------*/
/* Opening and closing */

static BOOL
ns_open_font(struct TextAttr *ta)
{
    Font = ta;
    FontOpen = OpenFont(ta);
    return FontOpen != NULL;
}

/*
 * The first layout that fits the screen: side by side; stacked; stacked and
 * compact - each with the longest lists that fit, down to NS_LIST_MIN lines.
 * FALSE when none fits (the last one tried stays).
 */
static BOOL
ns_fit(struct NSLayout *l, WORD bl, WORD bt, WORD br, WORD bb)
{
    LONG way;
    WORD rows;

    for (way = 0; way < 3; way++) {
        for (rows = NS_LIST_ROWS; rows >= NS_LIST_MIN; rows--) {
            ns_layout(l, Font, FontOpen, bl, bt, (BOOL)(way != 0), rows,
                (BOOL)(way == 2));
            if (bl + l->nl_Width + br <= Scr->Width &&
                bt + l->nl_Height + bb <= Scr->Height) {
                return TRUE;
            }
            if (bl + l->nl_Width + br > Scr->Width) {
                break;          /* too wide: shorter lists will not help */
            }
        }
    }
    return FALSE;
}

void
NSWindowOpen(void)
{
    struct NSLayout *l;
    WORD bl, bt, br, bb;

    if (Win != NULL) {
        WindowToFront(Win);
        ActivateWindow(Win);
        return;
    }
    ns_kinds(&NearbyKinds, TRUE);
    ns_kinds(&ReachKinds, FALSE);
    ns_kinds(&ShareKinds, FALSE);
    if ((l = AllocVec(sizeof(*l), MEMF_ANY | MEMF_CLEAR)) == NULL) {
        return;
    }
    if ((Scr = LockPubScreen(NULL)) == NULL) {
        FreeVec(l);
        return;
    }
    if ((VisInfo = GetVisualInfo(Scr, TAG_DONE)) == NULL ||
        !ns_open_font(Scr->Font)) {
        NSWindowClose();
        FreeVec(l);
        return;
    }
    bl = Scr->WBorLeft;
    br = Scr->WBorRight;
    bt = (WORD)(Scr->WBorTop + Scr->Font->ta_YSize + 1);
    bb = Scr->WBorBottom;
    if (!ns_fit(l, bl, bt, br, bb)) {
        /* too big in the screen's font: topaz 8 */
        CloseFont(FontOpen);
        if (!ns_open_font(&Topaz8)) {
            NSWindowClose();
            FreeVec(l);
            return;
        }
        ns_fit(l, bl, bt, br, bb);
    }
    if (!ns_gadgets(l)) {
        NSWindowClose();
        FreeVec(l);
        return;
    }
    MenuSpec[0].nm_Label = (STRPTR)SDMsg(MSG_NS_MENU_PROJECT);
    MenuSpec[1].nm_Label = (STRPTR)SDMsg(MSG_NS_MENU_HIDE) + 2;
    MenuSpec[1].nm_CommKey = (STRPTR)SDMsg(MSG_NS_MENU_HIDE);
    MenuSpec[3].nm_Label = (STRPTR)SDMsg(MSG_NS_MENU_QUIT) + 2;
    MenuSpec[3].nm_CommKey = (STRPTR)SDMsg(MSG_NS_MENU_QUIT);
    if ((Menus = CreateMenusA(MenuSpec, NULL)) == NULL ||
        !LayoutMenus(Menus, VisInfo, GTMN_NewLookMenus, TRUE, TAG_DONE)) {
        NSWindowClose();
        FreeVec(l);
        return;
    }
    SDFormat(Title, (LONG)sizeof(Title), SDMsg(MSG_NS_WINDOW_TITLE), NSPopKey);
    Win = OpenWindowTags(NULL,
        WA_Left, WinLeft >= 0 ? WinLeft : (Scr->Width - l->nl_Width - bl - br) / 2,
        WA_Top, WinTop >= 0 ? WinTop : (Scr->Height - l->nl_Height - bt - bb) / 2,
        WA_InnerWidth, l->nl_Width,
        WA_InnerHeight, l->nl_Height,
        WA_Title, (IPTR)Title,
        WA_ScreenTitle, (IPTR)Title,
        WA_PubScreen, (IPTR)Scr,
        WA_Gadgets, (IPTR)GadList,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK |
            BUTTONIDCMP | LISTVIEWIDCMP | STRINGIDCMP | CYCLEIDCMP | INTEGERIDCMP,
        WA_Flags, WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET |
            WFLG_ACTIVATE | WFLG_SIMPLE_REFRESH,
        WA_NewLookMenus, TRUE,
        WA_AutoAdjust, TRUE,
        TAG_DONE);
    FreeVec(l);
    if (Win == NULL) {
        NSWindowClose();
        return;
    }
    UnlockPubScreen(NULL, Scr);
    NSWindowPort = Win->UserPort;
    SetMenuStrip(Win, Menus);
    GT_RefreshWindow(Win, NULL);
    ns_frames();
    NSWindowShow(NS_CHANGED_STATE | NS_CHANGED_STATUS);
}

void
NSWindowClose(void)
{
    if (Win != NULL) {
        WinLeft = Win->LeftEdge;
        WinTop = Win->TopEdge;
        SDStrCopy(Kept.hs_Name, (const char *)ns_string(G_HS_NAME),
            (LONG)sizeof(Kept.hs_Name));
        SDStrCopy(Kept.hs_Type, (const char *)ns_kind_chosen(&ReachKinds, G_HS_KIND),
            (LONG)sizeof(Kept.hs_Type));
        SDStrCopy(Kept.sh_Name, (const char *)ns_string(G_SH_NAME),
            (LONG)sizeof(Kept.sh_Name));
        SDStrCopy(Kept.sh_Type, (const char *)ns_kind_chosen(&ShareKinds, G_SH_KIND),
            (LONG)sizeof(Kept.sh_Type));
        Kept.sh_Port = (LONG)ns_get(G_SH_PORT, GTIN_Number);
        SDStrCopy(Kept.sh_Info, (const char *)ns_string(G_SH_INFO),
            (LONG)sizeof(Kept.sh_Info));
        ClearMenuStrip(Win);
        CloseWindow(Win);
        Win = NULL;
        Scr = NULL;     /* unlocked when the window opened */
    }
    if (Scr != NULL) {
        UnlockPubScreen(NULL, Scr);
        Scr = NULL;
    }
    NSWindowPort = NULL;
    FreeMenus(Menus);
    Menus = NULL;
    FreeGadgets(GadList);
    GadList = NULL;
    FreeVisualInfo(VisInfo);
    VisInfo = NULL;
    if (FontOpen != NULL) {
        CloseFont(FontOpen);
        FontOpen = NULL;
    }
    /* nobody to show the list to: the browse ends with the window, and
     * what it found, which would be stale when the window opens again */
    NSStopBrowse();
    NSClearNearby();
}

/*------------------------------------------------------------------------*/
/* The user */

static void
ns_picked(LONG index)
{
    struct NSRow *r = NSRowAt(index);
    LONG kind;

    if (r == NULL) {
        return;
    }
    if (r->nr_IsKind) {
        ULONG changed;

        /* a kind: browse for its services */
        kind = ns_kind_of(&NearbyKinds, (CONST_STRPTR)r->nr_Type);
        ns_set(G_NB_KIND, GTCY_Labels, (IPTR)NearbyKinds.nk_Labels);
        ns_set(G_NB_KIND, GTCY_Active, kind);
        NSWindowDetach();
        changed = NSBrowse(NearbyKinds.nk_Types[kind]);
        NSWindowAttach();
        NSWindowShow(changed);
        return;
    }
    /* a service: to Handshake */
    kind = ns_kind_of(&ReachKinds, (CONST_STRPTR)r->nr_Type);
    ns_set(G_HS_NAME, GTST_String, (IPTR)r->nr_Name);
    ns_set(G_HS_KIND, GTCY_Labels, (IPTR)ReachKinds.nk_Labels);
    ns_set(G_HS_KIND, GTCY_Active, kind);
}

static BOOL
ns_command(LONG cmd)
{
    if (cmd == CMD_QUIT) {
        return FALSE;
    }
    if (cmd == CMD_HIDE) {
        NSWindowClose();
    }
    return TRUE;
}

static void
ns_gadget(const struct Gadget *g, UWORD code)
{
    ULONG changed = 0;

    NSWindowDetach();
    switch (g->GadgetID) {
    case G_NB_BROWSE:
        changed = NSBrowse(ns_kind_chosen(&NearbyKinds, G_NB_KIND));
        break;
    case G_NB_STOP:
        changed = NSStopBrowse();
        break;
    case G_NB_LIST:
        NSWindowAttach();
        ns_picked((LONG)code);
        return;
    case G_HS_NAME:
    case G_HS_GO:
        if (ns_string(G_HS_NAME)[0] != '\0') {
            changed = NSReach(ns_string(G_HS_NAME),
                ns_kind_chosen(&ReachKinds, G_HS_KIND));
        }
        break;
    case G_SH_STOP:
        changed = NSUnshare();
        break;
    case G_SH_GO:
        if (ns_string(G_SH_NAME)[0] != '\0') {
            changed = NSShare(ns_string(G_SH_NAME),
                ns_kind_chosen(&ShareKinds, G_SH_KIND),
                (LONG)ns_get(G_SH_PORT, GTIN_Number), ns_string(G_SH_INFO));
        }
        break;
    }
    NSWindowAttach();
    NSWindowShow(changed);
}

BOOL
NSWindowMessages(void)
{
    struct IntuiMessage *im;
    BOOL ok = TRUE;

    while (ok && Win != NULL && (im = GT_GetIMsg(Win->UserPort)) != NULL) {
        ULONG class = im->Class;
        UWORD code = im->Code;
        struct Gadget *g = (struct Gadget *)im->IAddress;
        UWORD item;

        GT_ReplyIMsg(im);
        switch (class) {
        case IDCMP_CLOSEWINDOW:
            NSWindowClose();
            break;
        case IDCMP_REFRESHWINDOW:
            GT_BeginRefresh(Win);
            ns_frames();
            GT_EndRefresh(Win, TRUE);
            break;
        case IDCMP_GADGETUP:
            ns_gadget(g, code);
            break;
        case IDCMP_MENUPICK:
            for (item = code; ok && Win != NULL && item != MENUNULL; ) {
                struct MenuItem *mi = ItemAddress(Menus, item);

                ok = ns_command((LONG)(IPTR)GTMENUITEM_USERDATA(mi));
                item = Win != NULL ? mi->NextSelect : MENUNULL;
            }
            break;
        }
    }
    return ok;
}
