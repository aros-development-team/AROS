/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Notification server. Shows the pop-ups that application.library's
          Notify() asks for, keeps the notification log, and takes the same
          requests over ARexx at the RINGHIO port, so AmigaOS 4 scripts work
          unchanged. Settings come from Prefs/Notifications.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <dos/notify.h>
#include <dos/datetime.h>
#include <dos/var.h>
#include <devices/timer.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/intuitionbase.h>
#include <graphics/rastport.h>
#include <graphics/layers.h>
#include <libraries/decortheme.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <datatypes/soundclass.h>
#include <cybergraphx/cybergraphics.h>
#include <workbench/icon.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <libraries/application.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/cybergraphics.h>
#include <proto/icon.h>
#include <proto/rexxsyslib.h>
#include <proto/application.h>
#include <proto/openurl.h>
#include <proto/utility.h>
#include <proto/decorator.h>
#include <proto/decortheme.h>
#include <proto/alib.h>

#include <string.h>

#include "../../libs/application/application_server.h"
#include "notifyprefs.h"

#define VERSTAG "\0$VER: Notifications 1.2 (22.10.2026)"
static const char version[] __attribute__((used)) = VERSTAG;

#define REXX_PORTNAME   "RINGHIO"
#define POPUP_PAD       8
#define POPUP_GAP       6
#define POPUP_MARGIN    16
#define POPUP_IMAGE     32
#define CLOSE_BOX       11
#define TICK_MICROS     250000
#define TICKS_PER_SEC   (1000000 / TICK_MICROS)
#define AWAY_SECS       60
#define MAX_LINES       10
#define MAX_QUEUED      32
#define LOG_MAX         65536
#define SOUND_SECS      30
#define MENU_MIX        20          /* what Decoration mixes menus with */
#define THEME_VAR       "SYS/theme.var"

struct Library *ApplicationBase;
struct Library *OpenURLBase;
struct Library *CyberGfxBase;
struct Library *DataTypesBase;
struct Library *IconBase;
struct Library *DecoratorBase;
struct Library *DecorThemeBase;
struct RxsLib *RexxSysBase;

/* One notification, waiting or shown. */
struct Request
{
    ULONG   rq_AppID;
    BOOL    rq_CloseOnDC;
    BOOL    rq_ConfirmClose;
    UBYTE   rq_VAlign;
    UBYTE   rq_Pri;
    TEXT    rq_AppName[APPNOTIFY_TITLE_MAX + 1];
    TEXT    rq_Title[APPNOTIFY_TITLE_MAX + 1];
    TEXT    rq_Text[APPNOTIFY_TEXT_MAX + 1];
    TEXT    rq_Screen[NOTIFYSERVER_SCREENMAX];
    TEXT    rq_Image[NOTIFYSERVER_PATHMAX];
    TEXT    rq_Icon[NOTIFYSERVER_PATHMAX];
    TEXT    rq_Sound[NOTIFYSERVER_PATHMAX];
    TEXT    rq_BackMsg[NOTIFYSERVER_BACKMAX];
    TEXT    rq_RexxPort[64];    /* BACKRXMSG goes here as an ARexx command */
    TEXT    rq_Gauges[NOTIFYSERVER_GAUGESMAX];
};

struct Waiting
{
    struct MinNode  w_Node;
    struct Request  w_Req;
};

struct Popup
{
    struct MinNode   p_Node;
    struct Request   p_Req;
    struct Window   *p_Window;
    struct Screen   *p_Screen;      /* locked while the pop-up is open */
    struct TextFont *p_Font;
    struct DecorImage *p_Under;     /* the screen beneath the pop-up ... */
    struct DecorImage *p_Back;      /* ... and the theme's menu drawn over it */
    WORD             p_X, p_Y;
    UWORD            p_W, p_H;
    BOOL             p_Hover;
    LONG             p_TicksLeft;   /* < 0: stays until closed */
    ULONG            p_LastSecs, p_LastMicros;
    ULONG           *p_ARGB;        /* image, at most POPUP_IMAGE square, or NULL */
    UWORD            p_ImgW, p_ImgH;
    UWORD            p_NumLines;
    UWORD            p_LineStart[MAX_LINES], p_LineLen[MAX_LINES];
    UWORD            p_NumGauges;
    UBYTE            p_GaugeLevel[APPNOTIFY_GAUGES_MAX];     /* percent */
    ULONG            p_GaugeRGB[APPNOTIFY_GAUGES_MAX];       /* 0xRRGGBB */
    TEXT             p_GaugeLabel[APPNOTIFY_GAUGES_MAX][48];
};

/* Applications registered on behalf of ARexx scripts (REGISTERAPP). */
struct RexxApp
{
    struct MinNode ra_Node;
    ULONG          ra_AppID;
    TEXT           ra_Name[APPNOTIFY_TITLE_MAX + 1];
};

struct Settings
{
    LONG    s_Position;
    LONG    s_DisplayTime;
    LONG    s_MaxVisible;
    LONG    s_Width;
    BOOL    s_HoldAway;
    BOOL    s_DND;
    BOOL    s_Beep;
    BOOL    s_Sounds;
    BOOL    s_Log;
    TEXT    s_DefSound[NOTIFYSERVER_PATHMAX];
};

/* Screen area a pop-up has just left; its copy of what was beneath it
   stays valid until the stack has been rearranged. */
struct Ghost
{
    struct MinNode     g_Node;
    struct Screen     *g_Screen;
    struct DecorImage *g_Under;
    WORD               g_X, g_Y;
    UWORD              g_W, g_H;
};

static struct MinList popups, waiting, rexxapps, ghosts;
static ULONG numwaiting;

/* The menu background of the theme C:Decoration runs with. */
static struct DecorImage *menuimg;
static struct TileInfo menutile, *menutilep;
static ULONG menutint;
static UWORD menupadl = POPUP_PAD, menupadt = POPUP_PAD, menupadr = POPUP_PAD, menupadb = POPUP_PAD;
static TEXT themepath[256];
static struct TextAttr topaz8 = { "topaz.font", 8, FS_NORMAL, FPF_ROMFONT };
static struct MsgPort *idcmpport, *rexxport;
static struct Settings set;
static PrefsObject *prefs;          /* the server's own application prefs */
static ULONG myappid;

static Object *sound;
static LONG soundticks;

/* Input activity, counted by an input handler; read every tick. */
static volatile ULONG inputcount;
static ULONG lastinputcount, tickssinceinput;

/***************************************************************************/

static void CopyStr(STRPTR dst, CONST_STRPTR src, ULONG size)
{
    if (!src)
        src = "";
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

static void LoadSettings(void)
{
    static CONST_STRPTR positions[] = NPREFS_POSITIONS;
    CONST_STRPTR s;

    if (prefs && myappid)
        ReadPrefs(prefs, READPREFS_AppID, myappid, TAG_DONE);

    set.s_Position = DictGetOptionForKey(prefs, NPK_POSITION, positions, NDEF_POSITION);
    set.s_DisplayTime = DictGetIntegerForKey(prefs, NPK_DISPLAYTIME, NDEF_DISPLAYTIME);
    set.s_MaxVisible = DictGetIntegerForKey(prefs, NPK_MAXVISIBLE, NDEF_MAXVISIBLE);
    set.s_Width = DictGetIntegerForKey(prefs, NPK_WIDTH, NDEF_WIDTH);
    set.s_HoldAway = DictGetBoolForKey(prefs, NPK_HOLDAWAY, NDEF_HOLDAWAY);
    set.s_DND = DictGetBoolForKey(prefs, NPK_DND, NDEF_DND);
    set.s_Beep = DictGetBoolForKey(prefs, NPK_BEEP, NDEF_BEEP);
    set.s_Sounds = DictGetBoolForKey(prefs, NPK_SOUNDS, NDEF_SOUNDS);
    set.s_Log = DictGetBoolForKey(prefs, NPK_LOG, NDEF_LOG);
    s = DictGetStringForKey(prefs, NPK_DEFSOUND, "");
    CopyStr(set.s_DefSound, s, sizeof(set.s_DefSound));

    if (set.s_DisplayTime < 0 || set.s_DisplayTime > NLIM_DISPLAYTIME)
        set.s_DisplayTime = NDEF_DISPLAYTIME;
    if (set.s_MaxVisible < 1 || set.s_MaxVisible > NLIM_MAXVISIBLE)
        set.s_MaxVisible = NDEF_MAXVISIBLE;
    if (set.s_Width < NLIM_WIDTHMIN || set.s_Width > NLIM_WIDTHMAX)
        set.s_Width = NDEF_WIDTH;
}

/* Applications switched off in the preferences show nothing. */
static BOOL AppAllowed(CONST_STRPTR name)
{
    return DictGetBoolForKey(DictGetObjectForKey(prefs, NPK_APPS), name, TRUE);
}

/***************************************************************************/

static void LogRequest(struct Request *rq)
{
    struct DateTime dt;
    TEXT date[LEN_DATSTRING], time[LEN_DATSTRING];
    BPTR fh;

    if (!set.s_Log)
        return;

    /* Keep the log small: start over, keeping one old generation. */
    if ((fh = Open(NPREFS_LOGFILE, MODE_OLDFILE)))
    {
        LONG size;

        Seek(fh, 0, OFFSET_END);
        size = Seek(fh, 0, OFFSET_CURRENT);
        Close(fh);
        if (size > LOG_MAX)
        {
            DeleteFile(NPREFS_LOGFILE ".old");
            Rename(NPREFS_LOGFILE, NPREFS_LOGFILE ".old");
        }
    }

    memset(&dt, 0, sizeof(dt));
    DateStamp(&dt.dat_Stamp);
    dt.dat_Format = FORMAT_INT;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    DateToStr(&dt);

    if (!(fh = Open(NPREFS_LOGFILE, MODE_READWRITE)))
        return;
    Seek(fh, 0, OFFSET_END);
    FPrintf(fh, "%s %s [%s] %s: %s", (IPTR)date, (IPTR)time, (IPTR)rq->rq_AppName,
            (IPTR)rq->rq_Title, (IPTR)rq->rq_Text);
    if (rq->rq_Gauges[0])
    {
        /* "62;#000000;Black\n9;#00FFFF;Cyan" -> " [Black 62%, Cyan 9%]" */
        CONST_STRPTR g = rq->rq_Gauges;

        FPuts(fh, " [");
        while (*g)
        {
            CONST_STRPTR level = g, label;
            LONG n;

            while (*g && *g != ';' && *g != '\n')
                g++;
            n = g - level;
            label = g;
            if (*label == ';' && (label = strchr(label + 1, ';')))
                label++;
            else
                label = "";
            while (*label && *label != '\n')
                FPutC(fh, *label++);
            FPutC(fh, ' ');
            FWrite(fh, (STRPTR)level, 1, n);
            FPutC(fh, '%');
            g = strchr(g, '\n');
            if (!g)
                break;
            g++;
            if (*g)
                FPuts(fh, ", ");
        }
        FPutC(fh, ']');
    }
    FPutC(fh, '\n');
    Close(fh);
}

static void StopSound(void)
{
    if (sound)
    {
        DisposeDTObject(sound);
        sound = NULL;
    }
}

static void PlaySound(CONST_STRPTR file)
{
    struct dtTrigger dtt;

    if (!set.s_Sounds || !file || !*file || !DataTypesBase)
        return;
    StopSound();
    if (!(sound = NewDTObject((APTR)file, DTA_GroupID, GID_SOUND, TAG_DONE)))
        return;
    dtt.MethodID = DTM_TRIGGER;
    dtt.dtt_GInfo = NULL;
    dtt.dtt_Function = STM_PLAY;
    dtt.dtt_Data = NULL;
    DoMethodA(sound, (Msg)&dtt);
    soundticks = SOUND_SECS * TICKS_PER_SEC;
}

/***************************************************************************/

/* "FRONT" is the frontmost screen if it is public, otherwise Workbench. */
static struct Screen *LockTargetScreen(CONST_STRPTR name)
{
    struct Screen *scr = NULL;

    if (name && *name && Stricmp(name, "FRONT") == 0)
    {
        struct List *list;
        struct PubScreenNode *psn;
        struct Screen *front;
        TEXT pubname[MAXPUBSCREENNAME + 1] = "";
        ULONG ilock = LockIBase(0);

        front = IntuitionBase->FirstScreen;
        UnlockIBase(ilock);

        list = LockPubScreenList();
        ForeachNode(list, psn)
        {
            if (psn->psn_Screen == front)
            {
                CopyStr(pubname, psn->psn_Node.ln_Name, sizeof(pubname));
                break;
            }
        }
        UnlockPubScreenList();

        if (pubname[0])
            scr = LockPubScreen(pubname);
    }
    else if (name && *name)
        scr = LockPubScreen(name);

    if (!scr)
        scr = LockPubScreen(NULL);
    return scr;
}

/* Keep an ARGB picture, scaled down to fit POPUP_IMAGE x POPUP_IMAGE. */
static void KeepImage(struct Popup *p, CONST ULONG *src, UWORD w, UWORD h)
{
    UWORD x, y, ow = w, oh = h;

    if (w > POPUP_IMAGE || h > POPUP_IMAGE)
    {
        /* keep the aspect ratio */
        if (w >= h)
        {
            ow = POPUP_IMAGE;
            oh = (h * POPUP_IMAGE) / w ? (h * POPUP_IMAGE) / w : 1;
        }
        else
        {
            oh = POPUP_IMAGE;
            ow = (w * POPUP_IMAGE) / h ? (w * POPUP_IMAGE) / h : 1;
        }
    }
    if ((p->p_ARGB = AllocVec(ow * oh * 4, MEMF_ANY)))
    {
        for (y = 0; y < oh; y++)
            for (x = 0; x < ow; x++)
                p->p_ARGB[y * ow + x] = src[(y * h / oh) * w + (x * w / ow)];
        p->p_ImgW = ow;
        p->p_ImgH = oh;
    }
}

static BOOL LoadImage(struct Popup *p, CONST_STRPTR file)
{
    struct BitMapHeader *bmh = NULL;
    ULONG *src;
    Object *o;

    if (!file || !*file || !DataTypesBase)
        return FALSE;

    if (!(o = NewDTObject((APTR)file,
                          DTA_GroupID,      GID_PICTURE,
                          PDTA_DestMode,    PMODE_V43,
                          PDTA_Remap,       FALSE,
                          TAG_DONE)))
        return FALSE;

    if (GetDTAttrs(o, PDTA_BitMapHeader, (IPTR)&bmh, TAG_DONE) && bmh
        && bmh->bmh_Width && bmh->bmh_Height)
    {
        UWORD w = bmh->bmh_Width, h = bmh->bmh_Height;

        if ((src = AllocVec(w * h * 4, MEMF_ANY)))
        {
            if (DoMethod(o, PDTM_READPIXELARRAY, (IPTR)src, PBPAFMT_ARGB, w * 4, 0, 0, w, h))
                KeepImage(p, src, w, h);
            FreeVec(src);
        }
    }
    DisposeDTObject(o);
    return p->p_ARGB != NULL;
}

/* A Workbench icon: its true-colour image, or its palette image. */
static BOOL LoadIcon(struct Popup *p, CONST_STRPTR file)
{
    struct DiskObject *dobj;
    TEXT name[NOTIFYSERVER_PATHMAX];
    ULONG len, w = 0, h = 0, *argb = NULL;

    if (!file || !*file || !IconBase)
        return FALSE;
    CopyStr(name, file, sizeof(name));
    len = strlen(name);
    if (len > 5 && Stricmp(name + len - 5, ".info") == 0)
        name[len - 5] = '\0';

    if (!(dobj = GetIconTags(name, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE)))
        return FALSE;

    IconControl(dobj, ICONCTRLA_GetWidth, (IPTR)&w, ICONCTRLA_GetHeight, (IPTR)&h,
                ICONCTRLA_GetARGBImageData1, (IPTR)&argb, TAG_DONE);
    if (w && h && w <= 512 && h <= 512)
    {
        if (argb)
            KeepImage(p, argb, w, h);
        else
        {
            struct ColorRegister *pal = NULL;
            UBYTE *pix = NULL;
            LONG transp = -1, npal = 0;
            ULONG *tmp, i;

            IconControl(dobj, ICONCTRLA_GetImageData1, (IPTR)&pix, ICONCTRLA_GetPalette1, (IPTR)&pal,
                        ICONCTRLA_GetPaletteSize1, (IPTR)&npal,
                        ICONCTRLA_GetTransparentColor1, (IPTR)&transp, TAG_DONE);
            if (pix && pal && (tmp = AllocVec(w * h * 4, MEMF_ANY)))
            {
                for (i = 0; i < w * h; i++)
                {
                    UBYTE *d = (UBYTE *)&tmp[i];
                    UBYTE c = pix[i];

                    if (c >= npal)
                        c = 0;
                    d[0] = (c == transp) ? 0 : 0xff;
                    d[1] = pal[c].red;
                    d[2] = pal[c].green;
                    d[3] = pal[c].blue;
                }
                KeepImage(p, tmp, w, h);
                FreeVec(tmp);
            }
        }
    }
    FreeDiskObject(dobj);
    return p->p_ARGB != NULL;
}

/* Break the text into lines no wider than 'width' pixels. */
static void WrapText(struct Popup *p, struct RastPort *rp, UWORD width)
{
    STRPTR t = p->p_Req.rq_Text;
    UWORD start = 0, len = strlen(t);

    p->p_NumLines = 0;
    while (start < len && p->p_NumLines < MAX_LINES)
    {
        UWORD end = start, lastspace = 0;

        while (end < len && t[end] != '\n')
        {
            if (TextLength(rp, t + start, end - start + 1) > width)
                break;
            if (t[end] == ' ')
                lastspace = end;
            end++;
        }
        if (end < len && t[end] != '\n' && lastspace > start)
            end = lastspace;
        if (end == start)
            end++;

        p->p_LineStart[p->p_NumLines] = start;
        p->p_LineLen[p->p_NumLines] = end - start;
        p->p_NumLines++;

        start = end;
        while (start < len && (t[start] == ' ' || t[start] == '\n'))
            start++;
    }
}

static BOOL Sticky(struct Popup *p)
{
    return p->p_TicksLeft < 0;
}

/* APPNOTIFY_Gauges: lines of "<percent>;#RRGGBB;<label>". */
static void ParseGauges(struct Popup *p, CONST_STRPTR g)
{
    while (g && *g && p->p_NumGauges < APPNOTIFY_GAUGES_MAX)
    {
        UWORD n = p->p_NumGauges;
        LONG level = 0, i;
        ULONG rgb = 0;
        CONST_STRPTR end = strchr(g, '\n');

        if (!end)
            end = g + strlen(g);
        while (g < end && *g >= '0' && *g <= '9')
            level = level * 10 + (*g++ - '0');
        if (g < end && *g == ';')
            g++;
        if (g < end && *g == '#')
        {
            for (g++, i = 0; i < 6 && g < end; i++, g++)
            {
                TEXT c = *g | 0x20;
                rgb = (rgb << 4) | (c >= 'a' ? c - 'a' + 10 : c - '0');
            }
        }
        else
            rgb = 0x808080;
        if (g < end && *g == ';')
            g++;
        for (i = 0; g < end && i < (LONG)sizeof(p->p_GaugeLabel[0]) - 1; i++)
            p->p_GaugeLabel[n][i] = *g++;
        p->p_GaugeLabel[n][i] = '\0';
        p->p_GaugeLevel[n] = level > 100 ? 100 : level;
        p->p_GaugeRGB[n] = rgb;
        p->p_NumGauges++;
        g = *end ? end + 1 : end;
    }
}

static void FreeMenuTheme(void)
{
    if (menuimg)
        DDisposeImageContainer(menuimg);
    menuimg = NULL;
    menutilep = NULL;
    menupadl = menupadt = menupadr = menupadb = POPUP_PAD;
}

/* Keep only the menu background of the theme named by SYS/theme.var, the
   one C:Decoration loads at boot; reload when the variable changes. */
static void LoadMenuTheme(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DecorTheme *theme;
    struct DecorImages *di;
    struct DecorConfig *dc;
    APTR oldwin;
    TEXT path[256];

    if (!DecorThemeBase || !DecoratorBase || !CyberGfxBase)
        return;
    if (GetVar(THEME_VAR, path, sizeof(path), GVF_GLOBAL_ONLY) <= 0)
        path[0] = '\0';
    if (strcmp(path, themepath) == 0)
        return;
    FreeMenuTheme();
    strcpy(themepath, path);
    if (!path[0])
        return;

    oldwin = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    theme = DTLoadTheme(path);
    me->pr_WindowPtr = oldwin;
    if (!theme)
        return;

    dc = DTGetConfig(theme);
    di = DTGetImages(theme);
    if (dc && di && di->img_menu && di->img_menu->data
        && (menuimg = DNewDecorImageContainer(di->img_menu->w, di->img_menu->h)))
    {
        CopyMem(di->img_menu->data, menuimg->data,
                (ULONG)menuimg->w * menuimg->h * sizeof(ULONG));
        menuimg->ok = TRUE;
        if (dc->MenuIsTiled)
        {
            menutile.TileLeft = dc->MenuTileLeft;
            menutile.TileTop = dc->MenuTileTop;
            menutile.TileRight = dc->MenuTileRight;
            menutile.TileBottom = dc->MenuTileBottom;
            menutilep = &menutile;
        }
        menutint = dc->MenuHighlightTint;
        if (dc->MenuInnerLeft > menupadl)
            menupadl = dc->MenuInnerLeft;
        if (dc->MenuInnerTop > menupadt)
            menupadt = dc->MenuInnerTop;
        if (dc->MenuInnerRight > menupadr)
            menupadr = dc->MenuInnerRight;
        if (dc->MenuInnerBottom > menupadb)
            menupadb = dc->MenuInnerBottom;
    }
    DTFreeTheme(theme);
}

static void DropGhosts(void)
{
    struct Ghost *g;

    while ((g = (struct Ghost *)REMHEAD((struct List *)&ghosts)))
    {
        DDisposeImageContainer(g->g_Under);
        FreeVec(g);
    }
}

/* Remember what was beneath a pop-up that is about to leave its place. */
static void LeaveGhost(struct Popup *p)
{
    struct Ghost *g;

    if (!p->p_Under)
        return;
    if ((g = AllocVec(sizeof(*g), MEMF_ANY)))
    {
        g->g_Screen = p->p_Screen;
        g->g_Under = p->p_Under;
        g->g_X = p->p_X;
        g->g_Y = p->p_Y;
        g->g_W = p->p_W;
        g->g_H = p->p_H;
        ADDTAIL((struct List *)&ghosts, g);
        p->p_Under = NULL;
    }
}

static void DropBackground(struct Popup *p)
{
    if (p->p_Under)
        DDisposeImageContainer(p->p_Under);
    if (p->p_Back)
        DDisposeImageContainer(p->p_Back);
    p->p_Under = p->p_Back = NULL;
}

static void CopyUnder(struct DecorImage *src, WORD sx, WORD sy, UWORD sw, UWORD sh,
                      struct DecorImage *dst, WORD dx, WORD dy, UWORD dw, UWORD dh)
{
    WORD x1 = sx > dx ? sx : dx, y1 = sy > dy ? sy : dy;
    WORD x2 = sx + sw < dx + dw ? sx + sw : dx + dw;
    WORD y2 = sy + sh < dy + dh ? sy + sh : dy + dh;

    if (x1 < x2 && y1 < y2)
        DDrawPartToImage(src, dst, x1 - sx, y1 - sy, x2 - x1, y2 - y1, x1 - dx, y1 - dy);
}

/* The screen beneath a rectangle. Where a pop-up is, or has just been,
   its own copy of the screen is used, as the display there is ours. */
static struct DecorImage *ScreenUnder(struct Screen *s, WORD x, WORD y, UWORD w, UWORD h)
{
    struct DecorImage *di = DGetImageFromRP(&s->RastPort, x, y, w, h);
    struct Popup *q;
    struct Ghost *g;

    if (!di)
        return NULL;
    ForeachNode(&popups, q)
        if (q->p_Screen == s && q->p_Under)
            CopyUnder(q->p_Under, q->p_X, q->p_Y, q->p_W, q->p_H, di, x, y, w, h);
    ForeachNode(&ghosts, g)
        if (g->g_Screen == s)
            CopyUnder(g->g_Under, g->g_X, g->g_Y, g->g_W, g->g_H, di, x, y, w, h);
    di->ok = TRUE;
    return di;
}

/* The theme's menu background for a pop-up at x,y, as Decoration makes it
   for a menu window: the screen beneath, blurred, with the menu tiles
   mixed over it. */
static BOOL MakeBackground(struct Popup *p, WORD x, WORD y)
{
    struct DecorImage *under, *back;

    if (!menuimg || GetBitMapAttr(p->p_Screen->RastPort.BitMap, BMA_DEPTH) < 15)
        return FALSE;
    if (!(under = ScreenUnder(p->p_Screen, x, y, p->p_W, p->p_H)))
        return FALSE;
    if (!(back = DNewDecorImageContainer(p->p_W, p->p_H)))
    {
        DDisposeImageContainer(under);
        return FALSE;
    }
    DDrawPartToImage(under, back, 0, 0, p->p_W, p->p_H, 0, 0);
    back->ok = TRUE;
    DRenderMenuBackground(back, menuimg, menutilep, MENU_MIX);
    DropBackground(p);
    p->p_Under = under;
    p->p_Back = back;
    return TRUE;
}

#define GAUGE_HEIGHT(p) ((p)->p_Font->tf_YSize + 6)

/* One bar: the level in its colour, the rest left empty, the label centred on it. */
static void DrawGauge(struct Popup *p, struct RastPort *rp, struct DrawInfo *dri, UWORD i,
                      UWORD x, UWORD y, UWORD w)
{
    UWORD h = GAUGE_HEIGHT(p) - 2, fill = (w - 2) * p->p_GaugeLevel[i] / 100, len, tw;
    ULONG rgb = p->p_GaugeRGB[i];
    UWORD lum = (((rgb >> 16) & 0xff) * 3 + ((rgb >> 8) & 0xff) * 6 + (rgb & 0xff)) / 10;
    TEXT text[64];

    SetAPen(rp, dri ? dri->dri_Pens[SHADOWPEN] : 1);
    Move(rp, x, y);
    Draw(rp, x + w - 1, y);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x, y + h - 1);
    Draw(rp, x, y);
    if (fill)
    {
        if (CyberGfxBase)
            FillPixelArray(rp, x + 1, y + 1, fill, h - 2, rgb);
        else
        {
            SetAPen(rp, dri ? dri->dri_Pens[FILLPEN] : 3);
            RectFill(rp, x + 1, y + 1, x + fill, y + h - 2);
        }
    }

    strcpy(text, p->p_GaugeLabel[i]);
    if (strlen(text) < sizeof(text) - 8)
    {
        TEXT num[8], *q = num + sizeof(num) - 1;
        UBYTE v = p->p_GaugeLevel[i];

        *q = '\0';
        *--q = '%';
        do
            *--q = '0' + v % 10;
        while (v /= 10);
        if (text[0])
            strcat(text, ": ");
        strcat(text, q);
    }
    len = strlen(text);
    while (len && (tw = TextLength(rp, text, len)) > w - 4)
        len--;
    tw = TextLength(rp, text, len);
    /* light text where it sits on a dark fill */
    SetAPen(rp, dri ? dri->dri_Pens[(fill > w / 2 && lum < 110) ? SHINEPEN : TEXTPEN] : 1);
    Move(rp, x + (w - tw) / 2, y + (h - p->p_Font->tf_YSize) / 2 + p->p_Font->tf_Baseline);
    Text(rp, text, len);
}

static void DrawPopup(struct Popup *p)
{
    struct Window *win = p->p_Window;
    struct RastPort *rp = win->RPort;
    struct DrawInfo *dri = GetScreenDrawInfo(p->p_Screen);
    UWORD w = win->Width, h = win->Height;
    UWORD tx = menupadl, ty, i;
    BOOL themed = p->p_Back != NULL;
    UWORD bg = dri ? dri->dri_Pens[BACKGROUNDPEN] : 0;
    UWORD fg = dri ? dri->dri_Pens[(themed && p->p_Hover) ? FILLTEXTPEN : TEXTPEN] : 1;
    UWORD shine = dri ? dri->dri_Pens[SHINEPEN] : 2;
    UWORD shadow = dri ? dri->dri_Pens[SHADOWPEN] : 1;
    UWORD hl = dri ? dri->dri_Pens[FILLPEN] : 3;

    SetFont(rp, p->p_Font);
    SetDrMd(rp, JAM1);

    if (themed)
    {
        struct DecorImage *di = NULL;

        /* under the pointer: the theme's menu item highlight */
        if (p->p_Hover && (di = DNewDecorImageContainer(w, h)))
        {
            DDrawPartToImage(p->p_Back, di, 0, 0, w, h, 0, 0);
            DSetImageTint(di, 255 - (menutint >> 24), menutint & 0xffffff);
            DPutImageToRP(rp, di, 0, 0);
        }
        else
            DDrawPartImageToRP(rp, p->p_Back, 0, 0, 0, 0, w, h);
    }
    else
    {
        SetAPen(rp, bg);
        RectFill(rp, 0, 0, w - 1, h - 1);

        /* a raised frame with an accent line along the top; urgent ones get
           a thicker one */
        SetAPen(rp, shine);
        Move(rp, 0, h - 1);
        Draw(rp, 0, 0);
        Draw(rp, w - 1, 0);
        SetAPen(rp, shadow);
        Draw(rp, w - 1, h - 1);
        Draw(rp, 0, h - 1);
        SetAPen(rp, hl);
        RectFill(rp, 1, 1, w - 2, p->p_Req.rq_Pri > 8 ? 4 : 2);
    }

    if (Sticky(p))
    {
        UWORD cx = w - menupadr - CLOSE_BOX, cy = menupadt;

        SetAPen(rp, shadow);
        Move(rp, cx, cy);
        Draw(rp, cx + CLOSE_BOX - 1, cy);
        Draw(rp, cx + CLOSE_BOX - 1, cy + CLOSE_BOX - 1);
        Draw(rp, cx, cy + CLOSE_BOX - 1);
        Draw(rp, cx, cy);
        SetAPen(rp, fg);
        Move(rp, cx + 3, cy + 3);
        Draw(rp, cx + CLOSE_BOX - 4, cy + CLOSE_BOX - 4);
        Move(rp, cx + CLOSE_BOX - 4, cy + 3);
        Draw(rp, cx + 3, cy + CLOSE_BOX - 4);
    }

    if (p->p_ARGB && CyberGfxBase)
    {
        UWORD iy = menupadt + 2;

        if (p->p_Req.rq_VAlign == 1)
            iy = (h - p->p_ImgH) / 2;
        else if (p->p_Req.rq_VAlign == 2)
            iy = h - menupadb - p->p_ImgH;
        WritePixelArrayAlpha(p->p_ARGB, 0, 0, p->p_ImgW * 4, rp,
                             menupadl + (POPUP_IMAGE - p->p_ImgW) / 2, iy,
                             p->p_ImgW, p->p_ImgH, 0xffffffff);
        tx += POPUP_IMAGE + POPUP_PAD;
    }

    ty = menupadt + 2 + p->p_Font->tf_Baseline;
    SetAPen(rp, fg);
    if (p->p_Req.rq_Title[0])
    {
        SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
        Move(rp, tx, ty);
        Text(rp, p->p_Req.rq_Title, strlen(p->p_Req.rq_Title));
        SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
        ty += p->p_Font->tf_YSize + 4;
    }
    for (i = 0; i < p->p_NumLines; i++)
    {
        Move(rp, tx, ty);
        Text(rp, p->p_Req.rq_Text + p->p_LineStart[i], p->p_LineLen[i]);
        ty += p->p_Font->tf_YSize;
    }

    ty = ty - p->p_Font->tf_Baseline + 4;
    for (i = 0; i < p->p_NumGauges; i++)
    {
        DrawGauge(p, rp, dri, i, tx, ty, w - tx - menupadr);
        ty += GAUGE_HEIGHT(p);
    }

    if (dri)
        FreeScreenDrawInfo(p->p_Screen, dri);
}

/* Where a pop-up goes, given the height of those between it and the
   chosen corner. */
static void Place(struct Popup *p, LONG offset, WORD *x, WORD *y)
{
    BOOL top = (set.s_Position == NPOS_TOPRIGHT || set.s_Position == NPOS_TOPLEFT);
    BOOL left = (set.s_Position == NPOS_BOTTOMLEFT || set.s_Position == NPOS_TOPLEFT);
    struct Screen *s = p->p_Screen;
    LONG minY = s->BarHeight + 1, px, py;

    if (top)
        py = minY + POPUP_MARGIN / 2 + offset;
    else
        py = s->Height - POPUP_MARGIN - offset - p->p_H;
    if (py < minY)
        py = minY;
    if (py + p->p_H > s->Height)
        py = s->Height - p->p_H;
    px = left ? POPUP_MARGIN : s->Width - p->p_W - POPUP_MARGIN;
    if (px < 0)
        px = 0;
    *x = px;
    *y = py;
}

static LONG StackOffset(struct Popup *p)
{
    struct Popup *q;
    LONG offset = 0;

    ForeachNode(&popups, q)
    {
        if (q == p)
            break;
        if (q->p_Screen == p->p_Screen && q->p_Window)
            offset += q->p_H + POPUP_GAP;
    }
    return offset;
}

/* CloseWindow() on a window sharing a port: drop its messages first. */
static void ClosePopupWindow(struct Popup *p)
{
    struct IntuiMessage *im;
    struct Node *succ;

    Forbid();
    im = (struct IntuiMessage *)idcmpport->mp_MsgList.lh_Head;
    while ((succ = im->ExecMessage.mn_Node.ln_Succ))
    {
        if (im->IDCMPWindow == p->p_Window)
        {
            REMOVE(im);
            ReplyMsg(&im->ExecMessage);
        }
        im = (struct IntuiMessage *)succ;
    }
    p->p_Window->UserPort = NULL;
    ModifyIDCMP(p->p_Window, 0);
    Permit();

    CloseWindow(p->p_Window);
    p->p_Window = NULL;
}

static BOOL OpenPopupWindow(struct Popup *p, WORD x, WORD y)
{
    p->p_X = x;
    p->p_Y = y;
    MakeBackground(p, x, y);
    p->p_Window = OpenWindowTags(NULL,
                                 WA_PubScreen,   (IPTR)p->p_Screen,
                                 WA_Left,        x,
                                 WA_Top,         y,
                                 WA_Width,       p->p_W,
                                 WA_Height,      p->p_H,
                                 WA_Borderless,  TRUE,
                                 WA_Activate,    FALSE,
                                 WA_ToolBox,     TRUE,
                                 WA_RMBTrap,     TRUE,
                                 WA_SmartRefresh, TRUE,
                                 WA_NoCareRefresh, TRUE,
                                 WA_BackFill,    (IPTR)LAYERS_NOBACKFILL,
                                 WA_Title,       (IPTR)p->p_Req.rq_Title,
                                 TAG_DONE);
    if (!p->p_Window)
    {
        DropBackground(p);
        return FALSE;
    }
    p->p_Window->UserPort = idcmpport;
    ModifyIDCMP(p->p_Window, IDCMP_MOUSEBUTTONS);
    DrawPopup(p);
    return TRUE;
}

static void FreePopup(struct Popup *p)
{
    DropBackground(p);
    CloseFont(p->p_Font);
    UnlockPubScreen(NULL, p->p_Screen);
    FreeVec(p->p_ARGB);
    FreeVec(p);
}

/* Stack the pop-ups of each screen away from the chosen corner, the
   newest nearest to it. A pop-up that has to move is opened afresh at
   its new place, so its background is taken from a display that holds
   nothing of ours there. */
/* APPNOTIFY_ConfirmClose: the application hears when its notification is
   done with, whether it was shown or not. */
static void Confirm(const struct Request *rq)
{
    struct ApplicationCustomMsg acm;

    if (!rq->rq_ConfirmClose || !rq->rq_AppID || !ApplicationBase)
        return;
    memset(&acm, 0, sizeof(acm));
    acm.customMsg = rq->rq_BackMsg[0] ? (STRPTR)rq->rq_BackMsg : NULL;
    SendApplicationMsg(myappid, rq->rq_AppID, &acm.almsg, APPLIBMT_ConfirmClose);
}

static void Restack(void)
{
    struct Popup *p, *next;

    ForeachNodeSafe(&popups, p, next)
    {
        WORD x, y;

        if (!p->p_Window)
            continue;
        Place(p, StackOffset(p), &x, &y);
        if (x == p->p_X && y == p->p_Y)
            continue;
        ClosePopupWindow(p);
        LeaveGhost(p);
        if (!OpenPopupWindow(p, x, y))
        {
            REMOVE(p);
            Confirm(&p->p_Req);
            FreePopup(p);
        }
    }
    DropGhosts();
}

static void ShowWaiting(void);

static void ClosePopup(struct Popup *p, BOOL next)
{
    REMOVE(p);
    ClosePopupWindow(p);
    LeaveGhost(p);
    Confirm(&p->p_Req);
    FreePopup(p);

    Restack();
    if (next)
        ShowWaiting();
}

static ULONG NumShown(void)
{
    struct Popup *p;
    ULONG n = 0;

    ForeachNode(&popups, p)
        n++;
    return n;
}

static ULONG OpenPopup(struct Request *rq)
{
    struct Popup *p;
    struct RastPort tmprp;
    UWORD textw, h, minh, len, width = set.s_Width;
    WORD x, y;

    if (!(p = AllocVec(sizeof(*p), MEMF_ANY | MEMF_CLEAR)))
        return APPNOTIFY_ERROR_OUTOFMEM;
    p->p_Req = *rq;

    if (!(p->p_Screen = LockTargetScreen(rq->rq_Screen)))
    {
        FreeVec(p);
        return APPNOTIFY_ERROR_GENERIC;
    }
    if (!(p->p_Font = OpenFont(p->p_Screen->Font)))
        p->p_Font = OpenFont(&topaz8);
    if (!p->p_Font)
    {
        UnlockPubScreen(NULL, p->p_Screen);
        FreeVec(p);
        return APPNOTIFY_ERROR_GENERIC;
    }

    if (!LoadImage(p, rq->rq_Image) && !LoadIcon(p, rq->rq_Icon))
        LoadImage(p, rq->rq_Icon);

    if (IsListEmpty((struct List *)&popups))
        LoadMenuTheme();

    if (width > p->p_Screen->Width - 2 * POPUP_MARGIN)
        width = p->p_Screen->Width - 2 * POPUP_MARGIN;
    textw = width - menupadl - menupadr - (p->p_ARGB ? POPUP_IMAGE + POPUP_PAD : 0);
    if (rq->rq_Pri >= 10)
        p->p_TicksLeft = -1;
    InitRastPort(&tmprp);
    SetFont(&tmprp, p->p_Font);
    WrapText(p, &tmprp, textw);

    ParseGauges(p, rq->rq_Gauges);
    h = menupadt + 2 + menupadb + (rq->rq_Title[0] ? p->p_Font->tf_YSize + 4 : 0)
        + p->p_NumLines * p->p_Font->tf_YSize
        + (p->p_NumGauges ? 4 + p->p_NumGauges * GAUGE_HEIGHT(p) : 0);
    minh = menupadt + menupadb + 2 + (p->p_ARGB ? POPUP_IMAGE : 0);
    if (Sticky(p) && minh < menupadt + menupadb + CLOSE_BOX)
        minh = menupadt + menupadb + CLOSE_BOX;
    if (menutilep && minh < menutile.TileTop + menutile.TileBottom)
        minh = menutile.TileTop + menutile.TileBottom;
    if (h < minh)
        h = minh;
    p->p_W = width;
    p->p_H = h;

    if (!Sticky(p))
    {
        if (set.s_DisplayTime)
            p->p_TicksLeft = set.s_DisplayTime * TICKS_PER_SEC;
        else
        {
            /* Long texts stay up longer: 4 s, plus 1 s per 20 characters, up to 10 s. */
            len = strlen(rq->rq_Text) + strlen(rq->rq_Title);
            p->p_TicksLeft = (4 + len / 20) * TICKS_PER_SEC;
            if (p->p_TicksLeft > 10 * TICKS_PER_SEC)
                p->p_TicksLeft = 10 * TICKS_PER_SEC;
        }
    }

    ADDHEAD((struct List *)&popups, p);
    Place(p, 0, &x, &y);
    if (!OpenPopupWindow(p, x, y))
    {
        REMOVE(p);
        FreePopup(p);
        return APPNOTIFY_ERROR_GENERIC;
    }
    Restack();

    if (rq->rq_Pri > 8 && set.s_Beep)
        DisplayBeep(p->p_Screen);
    PlaySound(rq->rq_Sound[0] ? rq->rq_Sound : set.s_DefSound);
    return APPNOTIFY_OK_MSGQUEUED;
}

static void ShowWaiting(void)
{
    struct Waiting *w;

    while (NumShown() < (ULONG)set.s_MaxVisible
           && (w = (struct Waiting *)REMHEAD((struct List *)&waiting)))
    {
        numwaiting--;
        if (OpenPopup(&w->w_Req) != APPNOTIFY_OK_MSGQUEUED)
            Confirm(&w->w_Req);
        FreeVec(w);
    }
}

/* APPNOTIFY_Update: this application's earlier notifications go; with
   backmsg, only those with that BackMsg (APPNOTIFY_Withdraw). */
static void DropAppNotifications(ULONG appID, CONST_STRPTR backmsg)
{
    struct Popup *p, *pn;
    struct Waiting *w, *wn;

    ForeachNodeSafe(&waiting, w, wn)
    {
        if (w->w_Req.rq_AppID == appID && (!backmsg || strcmp(w->w_Req.rq_BackMsg, backmsg) == 0))
        {
            REMOVE(w);
            Confirm(&w->w_Req);
            FreeVec(w);
            numwaiting--;
        }
    }
    /* ClosePopup() restacks, which may free any other pop-up: start over each time */
    for (;;)
    {
        pn = NULL;
        ForeachNode(&popups, p)
        {
            if (p->p_Req.rq_AppID == appID && (!backmsg || strcmp(p->p_Req.rq_BackMsg, backmsg) == 0))
            {
                pn = p;
                break;
            }
        }
        if (!pn)
            break;
        ClosePopup(pn, FALSE);
    }
}

/* Log a notification, then show it, queue it, or leave it in the log only. */
static ULONG Deliver(struct Request *rq, BOOL update, BOOL logonly)
{
    struct Waiting *w;

    LogRequest(rq);
    if (logonly)
    {
        Confirm(rq);
        return APPNOTIFY_OK_MSGQUEUED;
    }
    if (!AppAllowed(rq->rq_AppName))
        return APPNOTIFY_ERROR_APPNOTALLOWED;
    if (update)
        DropAppNotifications(rq->rq_AppID, NULL);
    if (set.s_DND && rq->rq_Pri < 10)
    {
        Confirm(rq);
        return APPNOTIFY_OK_MSGQUEUED;
    }

    if (NumShown() < (ULONG)set.s_MaxVisible)
        return OpenPopup(rq);

    if (numwaiting >= MAX_QUEUED)
        return APPNOTIFY_ERROR_TOOMANYMESSAGESQUEUED;
    if (!(w = AllocVec(sizeof(*w), MEMF_ANY)))
        return APPNOTIFY_ERROR_OUTOFMEM;
    w->w_Req = *rq;
    ADDTAIL((struct List *)&waiting, w);
    numwaiting++;
    return APPNOTIFY_OK_MSGQUEUED;
}

/***************************************************************************/

static ULONG rexxout;   /* BACKRXMSGs out at scripts, their replies due at rexxport */

static void SendRexxBack(CONST_STRPTR portname, CONST_STRPTR command)
{
    struct MsgPort *port;
    struct RexxMsg *rm;

    if (!RexxSysBase || !rexxport || !(rm = CreateRexxMsg(rexxport, NULL, NULL)))
        return;
    if (!(rm->rm_Args[0] = (IPTR)CreateArgstring((STRPTR)command, strlen(command))))
    {
        DeleteRexxMsg(rm);
        return;
    }
    rm->rm_Action = RXCOMM;
    Forbid();
    if ((port = FindPort(portname)))
    {
        PutMsg(port, &rm->rm_Node);
        rexxout++;
    }
    Permit();
    if (!port)
    {
        DeleteArgstring((UBYTE *)rm->rm_Args[0]);
        DeleteRexxMsg(rm);
    }
}

static void DoubleClicked(struct Popup *p)
{
    struct Request *rq = &p->p_Req;

    if (rq->rq_BackMsg[0])
    {
        if (rq->rq_RexxPort[0])
            SendRexxBack(rq->rq_RexxPort, rq->rq_BackMsg);
        else if (Strnicmp(rq->rq_BackMsg, "URL:", 4) == 0)
        {
            if (OpenURLBase)
                URL_OpenA(rq->rq_BackMsg + 4, NULL);
        }
        else if (rq->rq_AppID && ApplicationBase)
        {
            struct ApplicationCustomMsg acm;

            memset(&acm, 0, sizeof(acm));
            acm.customMsg = rq->rq_BackMsg;
            SendApplicationMsg(myappid, rq->rq_AppID, &acm.almsg, APPLIBMT_CustomMsg);
        }
    }
    if (rq->rq_CloseOnDC)
        ClosePopup(p, TRUE);
}

static BOOL InCloseBox(struct Popup *p, WORD x, WORD y)
{
    WORD cx = p->p_Window->Width - menupadr - CLOSE_BOX;

    return Sticky(p) && x >= cx && x < cx + CLOSE_BOX && y >= menupadt && y < menupadt + CLOSE_BOX;
}

static void HandleIDCMP(void)
{
    struct IntuiMessage *im;

    while ((im = (struct IntuiMessage *)GetMsg(idcmpport)))
    {
        struct Window *win = im->IDCMPWindow;
        ULONG class = im->Class;
        UWORD code = im->Code;
        WORD mx = im->MouseX, my = im->MouseY;
        ULONG secs = im->Seconds, micros = im->Micros;
        struct Popup *p, *hit = NULL;

        ReplyMsg(&im->ExecMessage);

        ForeachNode(&popups, p)
        {
            if (p->p_Window == win)
            {
                hit = p;
                break;
            }
        }
        if (!hit || class != IDCMP_MOUSEBUTTONS)
            continue;

        if (code == MENUDOWN)
            ClosePopup(hit, TRUE);
        else if (code == SELECTDOWN)
        {
            if (InCloseBox(hit, mx, my))
                ClosePopup(hit, TRUE);
            else if (DoubleClick(hit->p_LastSecs, hit->p_LastMicros, secs, micros))
            {
                hit->p_LastSecs = hit->p_LastMicros = 0;
                DoubleClicked(hit);
            }
            else
            {
                hit->p_LastSecs = secs;
                hit->p_LastMicros = micros;
            }
        }
    }
}

/* A pop-up under the pointer does not expire, so it can be read and clicked. */
static BOOL PointerOver(struct Popup *p)
{
    struct Window *w = p->p_Window;
    WORD x = p->p_Screen->MouseX, y = p->p_Screen->MouseY;

    return x >= w->LeftEdge && x < w->LeftEdge + w->Width
           && y >= w->TopEdge && y < w->TopEdge + w->Height;
}

static void Tick(void)
{
    struct Popup *p, *next;
    ULONG count = inputcount;
    BOOL away;

    if (count != lastinputcount)
    {
        lastinputcount = count;
        tickssinceinput = 0;
    }
    else if (tickssinceinput < AWAY_SECS * TICKS_PER_SEC)
        tickssinceinput++;
    away = set.s_HoldAway && tickssinceinput >= AWAY_SECS * TICKS_PER_SEC;

    ForeachNodeSafe(&popups, p, next)
    {
        BOOL over = PointerOver(p);

        if (p->p_Back && over != p->p_Hover)
        {
            p->p_Hover = over;
            DrawPopup(p);
        }
        if (Sticky(p))
            continue;
        if (over)
        {
            if (p->p_TicksLeft < 2 * TICKS_PER_SEC)
                p->p_TicksLeft = 2 * TICKS_PER_SEC;
        }
        else if (!away && --p->p_TicksLeft <= 0)
        {
            ClosePopup(p, TRUE);
            break;      /* Restack() may have freed next */
        }
    }

    if (sound && --soundticks <= 0)
        StopSound();
}

AROS_UFH2(struct InputEvent *, InputWatch,
          AROS_UFHA(struct InputEvent *, events, A0),
          AROS_UFHA(APTR, data, A1))
{
    AROS_USERFUNC_INIT

    struct InputEvent *ie;

    (void)data;
    for (ie = events; ie; ie = ie->ie_NextEvent)
    {
        if (ie->ie_Class == IECLASS_RAWKEY || ie->ie_Class == IECLASS_RAWMOUSE
            || ie->ie_Class == IECLASS_NEWPOINTERPOS)
        {
            inputcount++;
            break;
        }
    }
    return events;

    AROS_USERFUNC_EXIT
}

/***************************************************************************
 * ARexx, as the AmigaOS 4 notification server takes it:
 *   REGISTERAPP APP/K/A,UPDATE/S,IMG/K,ICON/K,TITLE/K,TEXT/F
 *   NOTIFY APP/K/A,UPDATE/S,CLOSEONDC/S,PRI/K/N,SCREEN/K,IMG/K,ICON/K,
 *          IMGVALIGN/K/N,LOGONLY/S,TITLE/K,AREXXPORT/K,BACKRXMSG/K,TEXT/F
 *   UNREGISTERAPP APP/K/A
 * and RINGHIO, the older form some scripts use:
 *   RINGHIO APP/K/A,SCREEN/K,TITLE/K,IMG/K,IMGVALIGN/K/N,CLOSEONDC/S,
 *           BACKMSG/K,TEXT/K/F
 * RC is 0, or 10 with the APPNOTIFY_ code in RC2.
 ***************************************************************************/

static struct RexxApp *FindRexxApp(CONST_STRPTR name)
{
    struct RexxApp *ra;

    ForeachNode(&rexxapps, ra)
    {
        if (Stricmp(ra->ra_Name, name) == 0)
            return ra;
    }
    return NULL;
}

static ULONG RexxAppID(CONST_STRPTR name, BOOL create)
{
    struct RexxApp *ra;
    ULONG id;

    if ((ra = FindRexxApp(name)))
        return ra->ra_AppID;
    if (!ApplicationBase)
        return 0;
    if ((id = FindApplication(FINDAPP_Name, (IPTR)name, TAG_DONE)) || !create)
        return id;

    if (!(ra = AllocVec(sizeof(*ra), MEMF_ANY | MEMF_CLEAR)))
        return 0;
    if (!(ra->ra_AppID = RegisterApplication(name, REGAPP_Description, (IPTR)"ARexx script",
                                             REGAPP_NoIcon, TRUE, TAG_DONE)))
    {
        FreeVec(ra);
        return 0;
    }
    CopyStr(ra->ra_Name, name, sizeof(ra->ra_Name));
    ADDTAIL((struct List *)&rexxapps, ra);
    return ra->ra_AppID;
}

static void RexxRequest(struct Request *rq, ULONG id, CONST_STRPTR app, CONST_STRPTR title,
                        CONST_STRPTR text)
{
    memset(rq, 0, sizeof(*rq));
    rq->rq_AppID = id;
    CopyStr(rq->rq_AppName, app, sizeof(rq->rq_AppName));
    CopyStr(rq->rq_Title, title ? title : app, sizeof(rq->rq_Title));
    CopyStr(rq->rq_Text, text, sizeof(rq->rq_Text));
}

static ULONG RexxCommand(STRPTR line)
{
    struct RDArgs *rda;
    struct Request rq;
    STRPTR args, buf;
    TEXT cmd[16];
    UWORD i = 0;
    ULONG alen, res = APPNOTIFY_ERROR_TAGS;
    IPTR a[13];

    while (*line == ' ' || *line == '\t')
        line++;
    while (line[i] && line[i] != ' ' && line[i] != '\t' && i < sizeof(cmd) - 1)
    {
        cmd[i] = line[i];
        i++;
    }
    cmd[i] = '\0';
    args = line + i;

    if (!(rda = AllocDosObject(DOS_RDARGS, NULL)))
        return APPNOTIFY_ERROR_OUTOFMEM;

    /* ReadArgs() wants the line to end in a newline. */
    alen = strlen(args);
    if (!(buf = AllocVec(alen + 2, MEMF_ANY)))
    {
        FreeDosObject(DOS_RDARGS, rda);
        return APPNOTIFY_ERROR_OUTOFMEM;
    }
    CopyMem(args, buf, alen);
    buf[alen] = '\n';
    buf[alen + 1] = '\0';
    rda->RDA_Source.CS_Buffer = buf;
    rda->RDA_Source.CS_Length = alen + 1;
    rda->RDA_Source.CS_CurChr = 0;
    rda->RDA_Flags |= RDAF_NOPROMPT;
    memset(a, 0, sizeof(a));

    if (Stricmp(cmd, "REGISTERAPP") == 0)
    {
        if (ReadArgs("APP/K/A,UPDATE/S,IMG/K,ICON/K,TITLE/K,TEXT/F", a, rda))
        {
            ULONG id = RexxAppID((STRPTR)a[0], TRUE);

            if (!id)
                res = APPNOTIFY_ERROR_GENERIC;
            else if (a[5] && *(STRPTR)a[5])
            {
                RexxRequest(&rq, id, (STRPTR)a[0], (STRPTR)a[4], (STRPTR)a[5]);
                CopyStr(rq.rq_Image, (STRPTR)a[2], sizeof(rq.rq_Image));
                CopyStr(rq.rq_Icon, (STRPTR)a[3], sizeof(rq.rq_Icon));
                res = Deliver(&rq, a[1] != 0, FALSE);
            }
            else
                res = APPNOTIFY_OK_MSGQUEUED;
            FreeArgs(rda);
        }
    }
    else if (Stricmp(cmd, "UNREGISTERAPP") == 0)
    {
        if (ReadArgs("APP/K/A", a, rda))
        {
            struct RexxApp *ra = FindRexxApp((STRPTR)a[0]);

            if (ra)
            {
                DropAppNotifications(ra->ra_AppID, NULL);
                REMOVE(ra);
                UnregisterApplication(ra->ra_AppID, NULL);
                FreeVec(ra);
                res = APPNOTIFY_OK_MSGQUEUED;
            }
            else
                res = APPNOTIFY_ERROR_APPNOTREGISTERED;
            FreeArgs(rda);
        }
    }
    else if (Stricmp(cmd, "NOTIFY") == 0)
    {
        if (ReadArgs("APP/K/A,UPDATE/S,CLOSEONDC/S,PRI/K/N,SCREEN/K,IMG/K,ICON/K,IMGVALIGN/K/N,"
                     "LOGONLY/S,TITLE/K,AREXXPORT/K,BACKRXMSG/K,TEXT/F", a, rda))
        {
            ULONG id = RexxAppID((STRPTR)a[0], FALSE);

            if (!id)
                res = APPNOTIFY_ERROR_APPNOTREGISTERED;
            else
            {
                LONG pri = a[3] ? *(LONG *)a[3] : 0;

                RexxRequest(&rq, id, (STRPTR)a[0], (STRPTR)a[9], (STRPTR)a[12]);
                rq.rq_CloseOnDC = a[2] != 0;
                rq.rq_Pri = pri < 0 ? 0 : (pri > 10 ? 10 : pri);
                rq.rq_VAlign = a[7] ? *(LONG *)a[7] : 0;
                CopyStr(rq.rq_Screen, (STRPTR)a[4], sizeof(rq.rq_Screen));
                CopyStr(rq.rq_Image, (STRPTR)a[5], sizeof(rq.rq_Image));
                CopyStr(rq.rq_Icon, (STRPTR)a[6], sizeof(rq.rq_Icon));
                CopyStr(rq.rq_RexxPort, (STRPTR)a[10], sizeof(rq.rq_RexxPort));
                CopyStr(rq.rq_BackMsg, (STRPTR)a[11], sizeof(rq.rq_BackMsg));
                res = (rq.rq_Text[0] || rq.rq_Title[0]) ? Deliver(&rq, a[1] != 0, a[8] != 0)
                                                        : APPNOTIFY_ERROR_TAGS;
            }
            FreeArgs(rda);
        }
    }
    else if (Stricmp(cmd, "RINGHIO") == 0)
    {
        if (ReadArgs("APP/K/A,SCREEN/K,TITLE/K,IMG/K,IMGVALIGN/K/N,CLOSEONDC/S,BACKMSG/K,TEXT/K/F",
                     a, rda))
        {
            ULONG id = RexxAppID((STRPTR)a[0], TRUE);

            if (!id)
                res = APPNOTIFY_ERROR_GENERIC;
            else if (a[7] && *(STRPTR)a[7])
            {
                RexxRequest(&rq, id, (STRPTR)a[0], (STRPTR)a[2], (STRPTR)a[7]);
                rq.rq_CloseOnDC = a[5] != 0;
                rq.rq_VAlign = a[4] ? *(LONG *)a[4] : 0;
                CopyStr(rq.rq_Screen, (STRPTR)a[1], sizeof(rq.rq_Screen));
                CopyStr(rq.rq_Image, (STRPTR)a[3], sizeof(rq.rq_Image));
                CopyStr(rq.rq_BackMsg, (STRPTR)a[6], sizeof(rq.rq_BackMsg));
                res = Deliver(&rq, FALSE, FALSE);
            }
            FreeArgs(rda);
        }
    }
    else
        res = APPNOTIFY_ERROR_GENERIC;

    FreeVec(buf);
    FreeDosObject(DOS_RDARGS, rda);
    return res;
}

static void HandleRexx(struct MsgPort *port)
{
    struct RexxMsg *rm;

    while ((rm = (struct RexxMsg *)GetMsg(port)))
    {
        ULONG res;

        if (rm->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
        {
            /* a BACKRXMSG we sent */
            DeleteArgstring((UBYTE *)rm->rm_Args[0]);
            DeleteRexxMsg(rm);
            rexxout--;
            continue;
        }

        if (IsRexxMsg(rm) && (rm->rm_Action & RXCODEMASK) == RXCOMM && rm->rm_Args[0])
            res = RexxCommand((STRPTR)rm->rm_Args[0]);
        else
            res = APPNOTIFY_ERROR_GENERIC;
        rm->rm_Result1 = res ? RC_ERROR : RC_OK;
        rm->rm_Result2 = res;
        ReplyMsg(&rm->rm_Node);
    }
}

/***************************************************************************/

static void HandleRequests(struct MsgPort *port)
{
    struct NotifyServerMsg *nsm;
    struct Request rq;

    while ((nsm = (struct NotifyServerMsg *)GetMsg(port)))
    {
        if (nsm->nsm_Version == NOTIFYSERVER_VERSION)
        {
            memset(&rq, 0, sizeof(rq));
            rq.rq_AppID = nsm->nsm_AppID;
            rq.rq_CloseOnDC = nsm->nsm_CloseOnDC;
            rq.rq_ConfirmClose = nsm->nsm_ConfirmClose;
            rq.rq_VAlign = nsm->nsm_ImageVertAlign;
            rq.rq_Pri = nsm->nsm_Pri;
            CopyStr(rq.rq_AppName, nsm->nsm_AppName, sizeof(rq.rq_AppName));
            CopyStr(rq.rq_Title, nsm->nsm_Title, sizeof(rq.rq_Title));
            CopyStr(rq.rq_Text, nsm->nsm_Text, sizeof(rq.rq_Text));
            CopyStr(rq.rq_Screen, nsm->nsm_PubScreen, sizeof(rq.rq_Screen));
            CopyStr(rq.rq_Image, nsm->nsm_ImageFile, sizeof(rq.rq_Image));
            CopyStr(rq.rq_Icon, nsm->nsm_IconFile, sizeof(rq.rq_Icon));
            CopyStr(rq.rq_Sound, nsm->nsm_SoundFile, sizeof(rq.rq_Sound));
            CopyStr(rq.rq_BackMsg, nsm->nsm_BackMsg, sizeof(rq.rq_BackMsg));
            CopyStr(rq.rq_Gauges, nsm->nsm_Gauges, sizeof(rq.rq_Gauges));
            if (nsm->nsm_Withdraw)
            {
                DropAppNotifications(rq.rq_AppID, rq.rq_BackMsg);
                ShowWaiting();
                nsm->nsm_Result = APPNOTIFY_OK_MSGQUEUED;
            }
            else
                nsm->nsm_Result = Deliver(&rq, nsm->nsm_Update, nsm->nsm_LogOnly);
        }
        else
            nsm->nsm_Result = APPNOTIFY_ERROR_GENERIC;
        ReplyMsg(&nsm->nsm_Msg);
    }
}

static void Reconfigure(void)
{
    LoadSettings();
    Restack();
    ShowWaiting();
}

/* Messages to the server as an application. Returns FALSE on Quit. */
static BOOL HandleAppMsgs(struct MsgPort *port)
{
    struct ApplicationMsg *am;
    BOOL run = TRUE;

    while ((am = (struct ApplicationMsg *)GetMsg(port)))
    {
        switch (am->type)
        {
        case APPLIBMT_Quit:
        case APPLIBMT_ForceQuit:
            run = FALSE;
            break;
        case APPLIBMT_ReloadPrefs:
            Reconfigure();
            break;
        case APPLIBMT_OpenPrefs:
            SystemTags(NPREFS_EDITOR, SYS_Asynch, TRUE, SYS_Input, 0, SYS_Output, 0, TAG_DONE);
            break;
        }
        ReplyMsg(&am->msg);
    }
    return run;
}

static struct MsgPort *MakePublicPort(CONST_STRPTR name)
{
    struct MsgPort *port;

    if (!(port = CreateMsgPort()))
        return NULL;
    port->mp_Node.ln_Name = (STRPTR)name;
    port->mp_Node.ln_Pri = 0;
    Forbid();
    if (FindPort(name))
    {
        Permit();
        DeleteMsgPort(port);
        return NULL;
    }
    AddPort(port);
    Permit();
    return port;
}

static void DropPublicPort(struct MsgPort *port, void (*drain)(struct MsgPort *))
{
    if (!port)
        return;
    Forbid();
    RemPort(port);
    Permit();
    drain(port);
    DeleteMsgPort(port);
}

static void RefuseRequests(struct MsgPort *port)
{
    struct NotifyServerMsg *nsm;

    while ((nsm = (struct NotifyServerMsg *)GetMsg(port)))
    {
        nsm->nsm_Result = APPNOTIFY_ERROR_SERVERNOTRUNNING;
        ReplyMsg(&nsm->nsm_Msg);
    }
}

static void RefuseRexx(struct MsgPort *port)
{
    struct RexxMsg *rm;

    while ((rm = (struct RexxMsg *)GetMsg(port)))
    {
        if (rm->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
        {
            DeleteArgstring((UBYTE *)rm->rm_Args[0]);
            DeleteRexxMsg(rm);
            rexxout--;
            continue;
        }
        rm->rm_Result1 = RC_FATAL;
        rm->rm_Result2 = APPNOTIFY_ERROR_SERVERNOTRUNNING;
        ReplyMsg(&rm->rm_Node);
    }
}

/* The port is the reply port of every BACKRXMSG still out at a script. A
   port that is not an ARexx host may hold such a message forever, so the
   wait is bounded; a port still owed replies then stays allocated and stops
   signalling, and a late reply lands on its list instead of in freed
   memory. */
static void DropRexxPort(struct MsgPort *port)
{
    int left = 25;

    if (!port)
        return;
    Forbid();
    RemPort(port);
    Permit();
    RefuseRexx(port);
    while (rexxout && left--)
    {
        Delay(10);
        RefuseRexx(port);
    }
    if (rexxout)
    {
        Forbid();
        port->mp_Flags = (port->mp_Flags & ~PF_ACTION) | PA_IGNORE;
        port->mp_SigTask = NULL;
        Permit();
        return;
    }
    DeleteMsgPort(port);
}

int main(void)
{
    struct MsgPort *reqport = NULL, *timerport = NULL, *inputport = NULL, *appport = NULL;
    struct timerequest *treq = NULL;
    struct IOStdReq *inputreq = NULL;
    struct Interrupt inputint;
    struct NotifyRequest nr;
    struct RexxApp *ra;
    struct Waiting *w;
    BOOL timeropen = FALSE, inputopen = FALSE, inputon = FALSE, notifyon = FALSE;
    BYTE notifysig = -1;
    int ret = RETURN_FAIL;

    NEWLIST((struct List *)&popups);
    NEWLIST((struct List *)&waiting);
    NEWLIST((struct List *)&rexxapps);
    NEWLIST((struct List *)&ghosts);

    if (!(ApplicationBase = OpenLibrary("application.library", 1)))
        return RETURN_FAIL;
    DataTypesBase = OpenLibrary("datatypes.library", 39);
    CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
    DecoratorBase = OpenLibrary("decorator.library", 0);
    DecorThemeBase = OpenLibrary("decortheme.library", 0);
    IconBase = OpenLibrary("icon.library", 44);
    RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    OpenURLBase = OpenLibrary("openurl.library", 0);

    if (!(reqport = MakePublicPort(NOTIFYSERVER_PORTNAME)))
    {
        /* Already running. */
        ret = RETURN_WARN;
        goto out;
    }
    if (RexxSysBase)
        rexxport = MakePublicPort(REXX_PORTNAME);

    myappid = RegisterApplication(NPREFS_APPNAME,
                                  REGAPP_URLIdentifier,       (IPTR)NPREFS_URL,
                                  REGAPP_Description,         (IPTR)"Shows notifications",
                                  REGAPP_UniqueApplication,   TRUE,
                                  REGAPP_NoIcon,              TRUE,
                                  REGAPP_HasPrefsWindow,      TRUE,
                                  REGAPP_CustomPrefsFileName, (IPTR)NPREFS_FILE,
                                  TAG_DONE);
    if (myappid)
        GetApplicationAttrs(myappid, APPATTR_Port, (IPTR)&appport,
                            APPATTR_MainPrefsDict, (IPTR)&prefs, TAG_DONE);
    LoadSettings();

    if (!(idcmpport = CreateMsgPort()) || !(timerport = CreateMsgPort())
        || !(treq = (struct timerequest *)CreateIORequest(timerport, sizeof(*treq))))
        goto out;
    if (OpenDevice(TIMERNAME, UNIT_VBLANK, &treq->tr_node, 0))
        goto out;
    timeropen = TRUE;

    /* Count input events, to tell when the user is away. */
    if ((inputport = CreateMsgPort())
        && (inputreq = (struct IOStdReq *)CreateIORequest(inputport, sizeof(*inputreq)))
        && !OpenDevice("input.device", 0, (struct IORequest *)inputreq, 0))
    {
        inputopen = TRUE;
        memset(&inputint, 0, sizeof(inputint));
        inputint.is_Code = (VOID_FUNC)AROS_ASMSYMNAME(InputWatch);
        inputint.is_Node.ln_Pri = 60;
        inputint.is_Node.ln_Name = (STRPTR)NPREFS_APPNAME;
        inputreq->io_Command = IND_ADDHANDLER;
        inputreq->io_Data = &inputint;
        inputon = (DoIO((struct IORequest *)inputreq) == 0);
    }

    /* Follow changes made by Prefs/Notifications. */
    if ((notifysig = AllocSignal(-1)) >= 0)
    {
        memset(&nr, 0, sizeof(nr));
        nr.nr_Name = "ENV:" NPREFS_FILE;
        nr.nr_Flags = NRF_SEND_SIGNAL;
        nr.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
        nr.nr_stuff.nr_Signal.nr_SignalNum = notifysig;
        notifyon = StartNotify(&nr);
    }

    treq->tr_node.io_Command = TR_ADDREQUEST;
    treq->tr_time.tv_secs = 0;
    treq->tr_time.tv_micro = TICK_MICROS;
    SendIO(&treq->tr_node);

    ret = RETURN_OK;
    for (;;)
    {
        ULONG sigs = Wait((1UL << reqport->mp_SigBit) | (1UL << idcmpport->mp_SigBit)
                          | (1UL << timerport->mp_SigBit)
                          | (rexxport ? 1UL << rexxport->mp_SigBit : 0)
                          | (appport ? 1UL << appport->mp_SigBit : 0)
                          | (notifyon ? 1UL << notifysig : 0)
                          | SIGBREAKF_CTRL_C);

        /* new settings first, so a request sent right after them uses them */
        if (notifyon && (sigs & (1UL << notifysig)))
            Reconfigure();
        if (sigs & (1UL << reqport->mp_SigBit))
            HandleRequests(reqport);
        if (rexxport && (sigs & (1UL << rexxport->mp_SigBit)))
            HandleRexx(rexxport);
        if (sigs & (1UL << idcmpport->mp_SigBit))
            HandleIDCMP();
        if (appport && (sigs & (1UL << appport->mp_SigBit)) && !HandleAppMsgs(appport))
            break;
        if ((sigs & (1UL << timerport->mp_SigBit)) && CheckIO(&treq->tr_node))
        {
            WaitIO(&treq->tr_node);
            Tick();
            treq->tr_node.io_Command = TR_ADDREQUEST;
            treq->tr_time.tv_secs = 0;
            treq->tr_time.tv_micro = TICK_MICROS;
            SendIO(&treq->tr_node);
        }
        if (sigs & SIGBREAKF_CTRL_C)
            break;
    }

out:
    DropPublicPort(reqport, RefuseRequests);
    DropRexxPort(rexxport);
    rexxport = NULL;

    while ((w = (struct Waiting *)REMHEAD((struct List *)&waiting)))
    {
        Confirm(&w->w_Req);
        FreeVec(w);
    }
    while (!IsListEmpty((struct List *)&popups))
        ClosePopup((struct Popup *)popups.mlh_TailPred, FALSE);
    StopSound();
    if (DecoratorBase)
        FreeMenuTheme();

    while ((ra = (struct RexxApp *)REMHEAD((struct List *)&rexxapps)))
    {
        UnregisterApplication(ra->ra_AppID, NULL);
        FreeVec(ra);
    }

    if (notifyon)
        EndNotify(&nr);
    if (notifysig >= 0)
        FreeSignal(notifysig);
    if (inputon)
    {
        inputreq->io_Command = IND_REMHANDLER;
        inputreq->io_Data = &inputint;
        DoIO((struct IORequest *)inputreq);
    }
    if (inputopen)
        CloseDevice((struct IORequest *)inputreq);
    if (inputreq)
        DeleteIORequest((struct IORequest *)inputreq);
    if (inputport)
        DeleteMsgPort(inputport);

    if (timeropen)
    {
        AbortIO(&treq->tr_node);
        WaitIO(&treq->tr_node);
        CloseDevice(&treq->tr_node);
    }
    if (treq)
        DeleteIORequest(&treq->tr_node);
    if (timerport)
        DeleteMsgPort(timerport);
    if (idcmpport)
        DeleteMsgPort(idcmpport);

    if (myappid)
        UnregisterApplication(myappid, NULL);

    if (OpenURLBase)
        CloseLibrary(OpenURLBase);
    if (DecorThemeBase)
        CloseLibrary(DecorThemeBase);
    if (DecoratorBase)
        CloseLibrary(DecoratorBase);
    if (RexxSysBase)
        CloseLibrary((struct Library *)RexxSysBase);
    if (IconBase)
        CloseLibrary(IconBase);
    if (CyberGfxBase)
        CloseLibrary(CyberGfxBase);
    if (DataTypesBase)
        CloseLibrary(DataTypesBase);
    CloseLibrary(ApplicationBase);

    return ret;
}
