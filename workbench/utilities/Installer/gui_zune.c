/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

/* gui_zune.c -- here are all functions for the ZUNE gui */

#include "Installer.h"
#include "cleanup.h"
#include "execute.h"
#include "locale.h"
#include "texts.h"
#include "misc.h"
#include "gui.h"
#include "variables.h"

/* External variables */
extern BPTR inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;
extern InstallerPrefs preferences;
extern int error, grace_exit;
extern int doing_abort;

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/datatypes.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#ifdef __AROS__
#include <libraries/coolimages.h>
#else
#define HBar(x) MUI_MakeObject(MUIO_HBar,x)
#define CoolImageIDButton(label,imageid) SimpleButton(label)
#endif

Object *app;
Object *wnd;
Object *reqwnd, *helpwnd, *helptext;
Object *reqroot, *root;
Object *btproceed, *btabort, *btskip, *bthelp;
Object *btback;              /* shown in place of Abort while (back) applies */
Object *btabortpage;         /* the page group Abort and Back share */
static int back_shown = FALSE;
int back_pressed = FALSE;    /* the user went back from the page, see execute.c */
Object *intermediate = NULL;
Object *working_text = NULL; /* the TextObject inside intermediate, see update_working() */
Object *copy_gauge = NULL;   /* the Gauge inside intermediate, see update_copying() */

enum
{
    Push_NULL,
    Push_Proceed,
    Push_Abort,
    Push_Skip,
    Push_Help,
    Push_About,
    Push_Ok,
    Push_Cancel,
    Push_Back,
    Push_Last
};


/* ######################################################################## */

#define disable_proceed(val)        set(btproceed, MUIA_Disabled, val)
#define disable_abort(val)        set(btabort, MUIA_Disabled, val)
#define disable_skip(val)        set(btskip, MUIA_Disabled, val)
#define disable_help(val)        set(bthelp, MUIA_Disabled, val)

/* Take down the working/copying page, if one is up (root must be in
   InitChange). The copy page had every button but Abort disabled. */
static void drop_intermediate(void)
{
    if (intermediate != NULL)
    {
        DoMethod(root, OM_REMMEMBER, (IPTR)intermediate);
        MUI_DisposeObject(intermediate);
        intermediate = NULL;
        working_text = NULL;
        if (copy_gauge)
        {
            copy_gauge = NULL;
            disable_proceed(FALSE);
            disable_skip(FALSE);
            disable_help(FALSE);
        }
    }
}

void AddContents(Object *obj)
{
    DoMethod(root, MUIM_Group_InitChange);
    drop_intermediate();
    DoMethod(root, OM_ADDMEMBER, (IPTR)obj);
    DoMethod(root, MUIM_Group_ExitChange);
}
void DelContents(Object *obj)
{
    DoMethod(root, MUIM_Group_InitChange);
    DoMethod(root, OM_REMMEMBER, (IPTR)obj);
    DoMethod(root, MUIM_Group_ExitChange);
    MUI_DisposeObject(obj);
}

/* A page with (back) offers "Back" instead of "Abort", like the original
   Installer. Called with the page's parameters, and with NULL to restore. */
static void show_back(struct ParameterList *pl)
{
    int on = (pl != NULL && GetPL(pl, _BACK).used == 1);

    if (on != back_shown)
    {
        set(btabortpage, MUIA_Group_ActivePage, on ? 1 : 0);
        back_shown = on;
    }
}

#define WaitCTRL(sigs)                                                        \
        if (sigs)                                                        \
        {                                                                \
            sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);        \
            if (sigs & SIGBREAKF_CTRL_C) break;                                \
            if (sigs & SIGBREAKF_CTRL_D) break;                                \
        }


#define NeedPROMPT(pl)                                        \
        if ( GetPL(pl, _PROMPT).intval == 0 )                \
        {                                                \
            error = SCRIPTERROR;                        \
            traperr("Missing prompt!\n", NULL);        \
        }

#define NeedHELP(pl)                                        \
        if ( GetPL(pl, _HELP).intval == 0 )                \
        {                                                \
            error = SCRIPTERROR;                        \
            traperr("Missing help!\n", NULL);                \
        }

#define TRANSSCRIPT()                                                \
    if ( preferences.transcriptstream != BNULL )                \
    {                                                                \
    int len = 0;                                                \
    char *out;                                                        \
    int m = GetPL(pl, _PROMPT).intval;                                \
        for ( i = 0 ; i < m ; i ++ )                                \
        {                                                        \
            len += strlen(GetPL(pl, _PROMPT).arg[i]) + 2;        \
        }                                                        \
        out = malloc((len+2)*sizeof(char));        \
        outofmem(out);                                                \
        out[0] = 0;                                                \
        for ( i = 0 ; i < m ; i ++ )                                \
        {                                                        \
            strcat(out,">");                                        \
            strcat(out,GetPL(pl, _PROMPT).arg[i]);                \
            strcat(out,"\n");                                        \
        }                                                        \
        Write(preferences.transcriptstream, out, len);                \
        free(out);                                                \
    }

/*
 * Ask user if he really wants to abort
 */
void abort_install()
{
BOOL running = TRUE, quit = FALSE;
LONG sigs = 0;
Object *wc;
Object *btok, *btcancel;

    wc = VGroup,
        Child, TextObject,
            GroupFrameT(_(MSG_MESSAGE)),
            MUIA_Text_Contents, (IPTR)ASKQUIT_STRING,
        End,
        Child, HBar(TRUE),
        Child, HGroup,
            MUIA_Group_SameSize, TRUE,
            Child, btok = CoolImageIDButton(_(MSG_OK), COOL_USEIMAGE_ID),
            Child, btcancel = CoolImageIDButton(_(MSG_CANCEL), COOL_CANCELIMAGE_ID),
        End,
    End;
    set(btok,MUIA_CycleChain,1);
    set(btcancel,MUIA_CycleChain,1);
    DoMethod(btok, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_Ok);
    DoMethod(btcancel, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_Cancel);
    DoMethod(reqroot, OM_ADDMEMBER, (IPTR)wc);

    set(reqwnd, MUIA_Window_Open, TRUE);
    while (running)
    {
        switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
        {
            case Push_Cancel:
                running = FALSE;
                break;
            case Push_Ok:
                quit = TRUE;
                running = FALSE;
                break;
            default:
                break;
        }
        WaitCTRL(sigs);
    }
    set(reqwnd, MUIA_Window_Open, FALSE);

    DoMethod(reqroot, OM_REMMEMBER, (IPTR)wc);
    MUI_DisposeObject(wc);

    if (quit)
    {
        error = USERABORT;
        grace_exit = TRUE;
        /* Execute trap(1) */
        traperr( "User aborted!\n", NULL );
    }
}


/* ######################################################################## */


const char GuiWinTitle[] ="AROS - Installer V44.10";


#define WINDOWWIDTH  400
#define WINDOWHEIGHT 300

void helpwin(char *title, char *text)
{
BOOL running = TRUE;
LONG sigs = 0;

    set(helptext, MUIA_Text_Contents, text);
    set(helpwnd, MUIA_Window_Title, title);
    set(helpwnd, MUIA_Window_Open, TRUE);
    while (running)
    {
        switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
        {
            case MUIV_Application_ReturnID_Quit:
                running = FALSE;
                break;
            default:
                break;
        }
        WaitCTRL(sigs);
    }
    set(helpwnd, MUIA_Window_Open, FALSE);
}


void helpwinpl(char *title, struct ParameterList *pl, int what)
{
char *text;

    text = collatestrings(GetPL(pl, what).intval, GetPL(pl, what).arg);
    if ( text != NULL )
    {
        helpwin(title, text);
        free(text);
    }
}


/*
 * Initialize the GUI
 */
void init_gui()
{
struct Screen *scr;
Object *banner = NULL;

    scr = LockPubScreen(NULL);

    /* APPBANNER: the application's own picture, centred above the pages.
       Without one (or when it does not load) the window gets no child
       there at all - a NULL Child would fail the group, a filler would
       soak up the slack - so it packs around the page like the original
       Installer's. */
    if (preferences.bannerfile != NULL)
    {
        banner = HGroup,
            Child, HSpace(0),
            Child, DtpicObject,
                MUIA_Dtpic_Name, (IPTR)preferences.bannerfile,
            End,
            Child, HSpace(0),
        End;
    }

    app = ApplicationObject,
        MUIA_Application_Title, "AROS - Installer",
        MUIA_Application_DoubleStart, TRUE,

           SubWindow, wnd = WindowObject,
            MUIA_Window_Title,        GuiWinTitle,
            MUIA_Window_Width,        WINDOWWIDTH,
            MUIA_Window_Height,        WINDOWHEIGHT,
            MUIA_Window_CloseGadget,        FALSE,
            MUIA_Window_NoMenus,        TRUE,
            MUIA_Window_ID,        MAKE_ID('A','I','N','S'),
            WindowContents, VGroup,
                banner ? MUIA_Group_Child : TAG_IGNORE, (IPTR)banner,
                Child, root = VGroup, End,
                Child, HBar(TRUE),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    Child, btproceed = CoolImageIDButton("Proceed", COOL_USEIMAGE_ID),
                    /* a page with (back) shows Back in Abort's place */
                    Child, btabortpage = PageGroup,
                        Child, btabort = CoolImageIDButton("Abort", COOL_CANCELIMAGE_ID),
                        Child, btback  = CoolImageIDButton("Back", COOL_CANCELIMAGE_ID),
                    End,
                    Child, btskip    = CoolImageIDButton("Skip", COOL_WARNIMAGE_ID),
                    Child, bthelp    = CoolImageIDButton("Help", COOL_INFOIMAGE_ID),
                End,
            End,
        End,
           SubWindow, helpwnd = WindowObject,
            MUIA_Window_Width,        WINDOWWIDTH,
            MUIA_Window_Height,        WINDOWHEIGHT,
            MUIA_Window_CloseGadget,        TRUE,
            MUIA_Window_SizeGadget,        FALSE,
            MUIA_Window_NoMenus,        TRUE,
            WindowContents, VGroup,
                Child, helptext = TextObject,
                    GroupFrame,
                    MUIA_Background, MUII_GroupBack,
                    MUIA_Text_Contents, (IPTR)NULL,
                End,
                Child, VSpace(1),
            End,
        End,
           SubWindow, reqwnd = WindowObject,
            MUIA_Window_Width,        WINDOWWIDTH,
            MUIA_Window_Height,        WINDOWHEIGHT,
            MUIA_Window_CloseGadget,        FALSE,
            MUIA_Window_SizeGadget,        FALSE,
            MUIA_Window_NoMenus,        TRUE,
            WindowContents, VGroup,
                Child, reqroot = VGroup, End,
            End,
        End,
    End;
    if (app == NULL)
    {
        /* failed to initialize GUI */
#if DEBUG
printf("Failed to intialize Zune GUI\n");
#endif
        exit(-1);
    }
    set(btproceed,MUIA_CycleChain,1);
    set(btabort,MUIA_CycleChain,1);
    set(btback,MUIA_CycleChain,1);
    set(btskip,MUIA_CycleChain,1);
    set(bthelp,MUIA_CycleChain,1);
    DoMethod(helpwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(btproceed, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
        MUIM_Application_ReturnID, Push_Proceed);
    DoMethod(btabort, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
        MUIM_Application_ReturnID, Push_Abort);
    DoMethod(btback, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
        MUIM_Application_ReturnID, Push_Back);
    DoMethod(btskip, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
        MUIM_Application_ReturnID, Push_Skip);
    DoMethod(bthelp, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
        MUIM_Application_ReturnID, Push_Help);
    set(wnd, MUIA_Window_Open, TRUE);
    UnlockPubScreen(NULL, scr);
}

/*
 * Close GUI
 */
void deinit_gui()
{
int i;

    for (i = 0 ; i < MAXMEDIA ; i++)
    {
        close_media(i);
    }
    set(wnd, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
}


/*
 * Show user that we are going to "(abort)" install
 * Don't confuse NOVICE...
 */
void show_abort(char *msg)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;

    if ( get_var_int("@user-level") > _NOVICE )
    {
        disable_abort(TRUE);
        disable_skip(TRUE);
        disable_help(TRUE);

        wc = VGroup,
            Child, TextObject,
                GroupFrameT("Aborting Installation:"),
                MUIA_Text_Contents, (IPTR)(msg),
            End,
            End;

        if (wc)
        {
            AddContents(wc);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }

            DelContents(wc);
        }

        disable_abort(FALSE);
        disable_skip(FALSE);
        disable_help(FALSE);
    }

}


/*
 * Show user how much we have completed yet
 */
void show_complete(long int percent)
{
/* MUIA_Window_Title keeps the pointer, so the buffer must outlive the call */
static char *text = NULL;

  if ( text == NULL )
  {
    text = malloc(strlen(GuiWinTitle) + 13);
    if ( text == NULL )
    {
      end_alloc();
    }
  }
  sprintf(text, "%s (Done %3ld%%)", GuiWinTitle, percent);
  set(wnd, MUIA_Window_Title, (IPTR)text);
}


/*
 * (effect): the original Installer opens its own screen with a colour
 * gradient behind the window; here the gradient becomes the background of
 * the window itself. The colours are $RRGGBB, "top" fades into "bottom";
 * a "radial" effect is drawn as the same top-to-bottom fade (as InstallerLG
 * does), Zune's gradients being linear.
 */
void show_effect(unsigned long top, unsigned long bottom)
{
/* Zune keeps the pointer only while it parses the spec, so a local is fine */
char spec[80];
Object *contents = NULL;

    sprintf(spec, "7:v,%08lx,%08lx,%08lx-%08lx,%08lx,%08lx",
        ((top >> 16) & 0xff) * 0x01010101UL, ((top >> 8) & 0xff) * 0x01010101UL, (top & 0xff) * 0x01010101UL,
        ((bottom >> 16) & 0xff) * 0x01010101UL, ((bottom >> 8) & 0xff) * 0x01010101UL, (bottom & 0xff) * 0x01010101UL);
    get(wnd, MUIA_Window_RootObject, &contents);
    if (contents != NULL)
    {
        /* not forwarded: the buttons and the page keep their own backgrounds */
        nfset(contents, MUIA_Background, (IPTR)spec);
    }
}

/*
 * (querydisplay <object> <option>): the width, height, depth or number of
 * colours of the screen, or the width, height and the distances to the
 * upper, lower, left and right edges of the screen of the window
 */
long int query_display(char *object, char *option)
{
struct Screen *scr = NULL;
struct Window *win = NULL;
long int width = 0, height = 0, depth = 0, colors = 0, upper = 0, lower = 0, left = 0, right = 0;

    get(wnd, MUIA_Window_Screen, &scr);
    get(wnd, MUIA_Window_Window, &win);
    if (scr != NULL && strcasecmp(object, "screen") == 0)
    {
        width = scr->Width;
        height = scr->Height;
        depth = GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH);
        colors = 1L << (depth > 24 ? 24 : depth);
    }
    else if (scr != NULL && win != NULL && strcasecmp(object, "window") == 0)
    {
        width = win->Width;
        height = win->Height;
        left = win->LeftEdge;
        right = scr->Width - width - left;
        upper = win->TopEdge;
        lower = scr->Height - height - upper;
    }
    if (strcasecmp(option, "width") == 0) return width;
    if (strcasecmp(option, "height") == 0) return height;
    if (strcasecmp(option, "depth") == 0) return depth;
    if (strcasecmp(option, "colors") == 0) return colors;
    if (strcasecmp(option, "upper") == 0) return upper;
    if (strcasecmp(option, "lower") == 0) return lower;
    if (strcasecmp(option, "left") == 0) return left;
    if (strcasecmp(option, "right") == 0) return right;
    return 0;
}

/*
 * (showmedia): a picture in a window of its own, next to the Installer's.
 * The window is placed by "upper_left" ... "lower_right" (anything else
 * centres it), sized "small", "medium" or "large" (1/8, 1/4, 1/2 of the
 * screen, a _small/_medium/_large suffix giving the height its own class)
 * or left at the size of the picture, with or without a border. The
 * datatype is looked at first: only pictures are shown, the other kinds of
 * media the original Installer knows are not. Returns the media id, or -1.
 */
static Object *mediawnd[MAXMEDIA];
static char *medianame[MAXMEDIA];    /* Dtpic keeps the pointer, so do we */

static int has_part(char *s, char *part)
{
size_t n = strlen(part);

    for ( ; *s != '\0' ; s++)
    {
        if (strncasecmp(s, part, n) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* "small" = 1/8, "medium" = 1/4, "large" = 1/2, else 0 */
static int size_shift(char *s)
{
    if (strncasecmp(s, "small", 5) == 0) return 3;
    if (strncasecmp(s, "medium", 6) == 0) return 2;
    if (strncasecmp(s, "large", 5) == 0) return 1;
    return 0;
}

long int show_media(char *file, char *position, char *size, int border)
{
struct Screen *scr = NULL;
struct Window *win = NULL;
struct DataType *dt;
BPTR lock;
Object *obj;
IPTR opened = FALSE;
long int id, width = 0, height = 0, x, y;
int wshift, hshift, picture = FALSE;
char *suffix;

    for (id = 0 ; id < MAXMEDIA && mediawnd[id] != NULL ; id++);
    if (id == MAXMEDIA)
    {
        return -1;
    }

    lock = Lock(file, SHARED_LOCK);
    if (lock == BNULL)
    {
        set_variable("@ioerr", NULL, IoErr());
        return -1;
    }
    dt = ObtainDataType(DTST_FILE, (APTR)lock, NULL);
    if (dt != NULL)
    {
        picture = (dt->dtn_Header->dth_GroupID == GID_PICTURE);
        ReleaseDataType(dt);
    }
    UnLock(lock);
    if (!picture)
    {
        return -1;
    }

    get(wnd, MUIA_Window_Screen, &scr);
    wshift = size_shift(size);
    if (scr != NULL && wshift != 0)
    {
        suffix = strchr(size, '_');
        hshift = suffix != NULL ? size_shift(suffix + 1) : 0;
        width = scr->Width >> wshift;
        height = scr->Height >> (hshift != 0 ? hshift : wshift);
    }

    medianame[id] = strdup(file);
    if (medianame[id] == NULL)
    {
        end_alloc();
    }
    obj = WindowObject,
        MUIA_Window_Activate, FALSE,
        MUIA_Window_CloseGadget, FALSE,
        MUIA_Window_SizeGadget, FALSE,
        MUIA_Window_NoMenus, TRUE,
        border ? TAG_IGNORE : MUIA_Window_Borderless, TRUE,
        width != 0 ? MUIA_Window_Width : TAG_IGNORE, width,
        height != 0 ? MUIA_Window_Height : TAG_IGNORE, height,
        WindowContents, border ?
            ScrollgroupObject,
                MUIA_Scrollgroup_Contents, VirtgroupObject,
                    Child, DtpicObject, MUIA_Dtpic_Name, (IPTR)medianame[id], End,
                End,
            End
        :
            VGroup,
                Child, DtpicObject, MUIA_Dtpic_Name, (IPTR)medianame[id], End,
            End,
    End;
    if (obj != NULL)
    {
        DoMethod(app, OM_ADDMEMBER, (IPTR)obj);
        set(obj, MUIA_Window_Open, TRUE);
        get(obj, MUIA_Window_Open, &opened);
        if (!opened)
        {
            DoMethod(app, OM_REMMEMBER, (IPTR)obj);
            MUI_DisposeObject(obj);
            obj = NULL;
        }
    }
    if (obj == NULL)
    {
        free(medianame[id]);
        medianame[id] = NULL;
        return -1;
    }
    mediawnd[id] = obj;

    /* Zune places windows at the centre or at given coordinates, so the
       right and lower edges are reached once the size is known */
    get(obj, MUIA_Window_Window, &win);
    if (scr != NULL && win != NULL)
    {
        x = win->LeftEdge;
        y = win->TopEdge;
        if (has_part(position, "left")) x = 0;
        if (has_part(position, "right")) x = scr->Width - win->Width;
        if (has_part(position, "upper")) y = 0;
        if (has_part(position, "lower")) y = scr->Height - win->Height;
        if (x != win->LeftEdge || y != win->TopEdge)
        {
            ChangeWindowBox(win, x, y, win->Width, win->Height);
        }
    }
    return id;
}

/*
 * (setmedia): pictures have nothing to play, pause or locate; the action
 * counts when the media is there
 */
long int set_media(long int id)
{
    return (id >= 0 && id < MAXMEDIA && mediawnd[id] != NULL) ? 1 : 0;
}

/*
 * (closemedia): close the window of the media and forget the id
 */
long int close_media(long int id)
{
    if (id < 0 || id >= MAXMEDIA || mediawnd[id] == NULL)
    {
        return 0;
    }
    set(mediawnd[id], MUIA_Window_Open, FALSE);
    DoMethod(app, OM_REMMEMBER, (IPTR)mediawnd[id]);
    MUI_DisposeObject(mediawnd[id]);
    mediawnd[id] = NULL;
    free(medianame[id]);
    medianame[id] = NULL;
    return 1;
}


/*
 * Show user that we "(exit)" the installation
 */
void show_exit(char *msg)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;
char *msg2;

    if (msg == NULL)
    {
        msg = "";
    }
    msg2 = malloc(strlen(msg) + strlen(DONE_TEXT) + 2);
    outofmem(msg2);
    strcpy(msg2, msg);
    if (msg[0] != 0)
    {
        strcat(msg2, "\n");
    }
    strcat(msg2, DONE_TEXT);
    disable_abort(TRUE);
    disable_skip(TRUE);
    disable_help(TRUE);

    wc = VGroup,
            Child, TextObject,
                GroupFrameT("Installation complete:"),
                MUIA_Text_Contents, (IPTR)(msg2),
            End,
        End;

    if (wc)
    {
        AddContents(wc);

        while (running)
        {
            switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
            {
                case Push_Proceed:
                    running = FALSE;
                    break;
                default:
                    break;
            }
            WaitCTRL(sigs);
        }

        DelContents(wc);
    }

    disable_abort(FALSE);
    disable_skip(FALSE);
    disable_help(FALSE);
}


/*
 * Show the line which caused the parse-error
 */
void show_parseerror(char * msg, int errline)
{
char *text;

    text = malloc(strlen(msg) + 64);
    if (text == NULL)
        return;
    sprintf(text, "Error in script line %d:\n%s", errline, msg);
    if (preferences.fromcli)
    {
        /* a shell can capture it, the way (debug) output is captured */
        printf("Installer: %s\n", text);
    }
    display_text(text);
    free(text);
}


/*
 * Tell user that some big task is to be done
 * "Be patient..."
 */
void show_working(char *msg)
{
    DoMethod(root, MUIM_Group_InitChange);
    drop_intermediate();
    DoMethod(root, MUIM_Group_ExitChange);
    
    intermediate = VGroup,
        Child, working_text = TextObject,
        GroupFrameT(_(MSG_MESSAGE)),
            MUIA_Text_Contents, (IPTR)(msg),
        End,
    End;

    if (intermediate)
    {
        DoMethod(root, MUIM_Group_InitChange);
        DoMethod(root, OM_ADDMEMBER, (IPTR)intermediate);
        DoMethod(root, MUIM_Group_ExitChange);
    }
}


/*
 * Display a "(message)" to the user
 * Don't confuse NOVICE unless "(all)" users want to get this info
 */
void show_message(char * msg, struct ParameterList * pl)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;

    if ( GetPL(pl, _ALL).used == 1 || get_var_int("@user-level") > _NOVICE )
    {
        disable_skip(TRUE);
        if ( GetPL(pl, _HELP).used != 1 )
        {
            disable_help(TRUE);
        }

        wc = VGroup,
            Child, TextObject,
                GroupFrameT(_(MSG_MESSAGE)),
                MUIA_Text_Contents, (IPTR)(msg),
            End,
            End;

        if (wc)
        {
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    case Push_Help:
                        helpwinpl(HELP_ON_MESSAGE, pl, _HELP);
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }

            DelContents(wc);
            show_back(NULL);
        }

        disable_skip(FALSE);
        disable_help(FALSE);
    }
}


/*
 * Ask user for his user-level
 */
void request_userlevel(char *msg)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;
Object *btabout;

LONG usrlevel, logval;
char *welcome;

Object *levelmx;
char **mxlabels;

/* Ask for User Level */
    usrlevel = preferences.defusrlevel;
    disable_skip(TRUE);

    if ( msg != NULL )
    {
        welcome = strdup(msg);
    }
    else
    {
        welcome = malloc(sizeof(char *)*(strlen(WELCOME_TEMPLATE)+strlen(get_var_arg("@app-name"))));
        outofmem(welcome);
        sprintf(welcome, WELCOME_TEMPLATE, get_var_arg("@app-name"));
    }
    mxlabels = malloc(4*sizeof(STRPTR));
    mxlabels[0] = strdup(NOVICE_NAME);
    mxlabels[1] = strdup(ADVANCED_NAME);
    mxlabels[2] = strdup(EXPERT_NAME);
    mxlabels[3] = NULL;


    wc = VGroup,
        Child, VGroup, GroupFrame,
            Child, TextObject,
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                MUIA_Text_Contents, (IPTR)(welcome),
            End,
            Child, levelmx = RadioObject,
                GroupFrameT(USERLEVEL_REQUEST),
                MUIA_Radio_Entries, (IPTR)(mxlabels),
            End,
            Child, btabout = CoolImageIDButton("About Installer" ,COOL_ASKIMAGE_ID),
            End,
        End;

    if (wc)
    {
        set(levelmx, MUIA_Radio_Active, usrlevel);
        DoMethod(btabout, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_About);
        AddContents(wc);

        while (running)
        {
            switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
            {
                case Push_Abort:
                    abort_install();
                    break;
                case Push_Proceed:
                    running = FALSE;
                    break;
                case Push_Help:
                    helpwin(HELP_ON_USERLEVEL, USERLEVEL_HELP);
                    break;
                case Push_About:
                    {
                    char *helptext;

/* TODO: help/about for Installer */
                        helptext = malloc(512 * sizeof(char));
                        sprintf(helptext, ABOUT_INSTALLER, INSTALLER_VERSION, INSTALLER_REVISION);
                        helpwin(ABOUT_ON_INSTALLER, helptext);
                        free(helptext);
                    }
                    break;
                default:
                    break;
            }
            WaitCTRL(sigs);
        }
        GetAttr(MUIA_Radio_Active, levelmx, (IPTR *)&usrlevel);
        set_variable("@user-level", NULL, usrlevel);

        DelContents(wc);
    }
    free(welcome);
    free(mxlabels[0]);
    free(mxlabels[1]);
    free(mxlabels[2]);

/* Ask for Logfile creation - a novice is not asked: LOG=FALSE/NOLOG
   decide for them; under NOLOG there is no log to ask anyone about */
    if ( usrlevel == 0 && !preferences.novicelog )
    {
        free(preferences.transcriptfile);
        preferences.transcriptfile = NULL;
    }
    if ( usrlevel > 0 )
    {
        if ( preferences.transcriptfile != NULL )
        {
            mxlabels[0] = strdup(LOG_FILE_TEXT);
            mxlabels[1] = strdup(LOG_PRINT_TEXT);
            mxlabels[2] = strdup(LOG_NOLOG_TEXT);

            wc = VGroup,
                Child, levelmx = RadioObject,
                    GroupFrameT(LOG_QUESTION),
                    MUIA_Radio_Entries, (IPTR)(mxlabels),
                    End,
                End;

            if (wc)
            {
                AddContents(wc);

                running = TRUE;
                while (running)
                {
                    switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                    {
                        case Push_Abort:
                            abort_install();
                            break;
                        case Push_Proceed:
                            running = FALSE;
                            break;
                        case Push_Help:
                            {
                            char *helptext;

/* TODO: help for logfile-requester */
                              helptext = malloc(512 * sizeof(char));
                              sprintf(helptext, LOG_HELP, preferences.transcriptfile);
                              helpwin(HELP_ON_LOGFILES, helptext);
                                free(helptext);
                            }
                            break;
                        default:
                            break;
                    }
                    WaitCTRL(sigs);
                }
                GetAttr(MUIA_Radio_Active, levelmx, (IPTR *)&logval);
                switch (logval)
                {
                    case 0: /* Log to file */
/* TODO: Handle Logging output selection */
                        break;
                    case 1: /* Log to printer */
                        free(preferences.transcriptfile);
                        preferences.transcriptfile = strdup("PRT:");
                        break;
                    case 2: /* No Log */
                        free(preferences.transcriptfile);
                        preferences.transcriptfile = NULL;
                        break;
                }

                DelContents(wc);
            }
            free(mxlabels[0]);
            free(mxlabels[1]);
            free(mxlabels[2]);
        }

        if (!preferences.nopretend)
        {
            mxlabels[0] = strdup(NOPRETEND_TEXT);
            mxlabels[1] = strdup(PRETEND_TEXT);
            mxlabels[2] = NULL;

            wc = VGroup,
                Child, levelmx = RadioObject,
                    GroupFrameT(PRETEND_QUESTION),
                    MUIA_Radio_Entries, (IPTR)(mxlabels),
                    End,
                End;

            if (wc)
            {
                AddContents(wc);

                running = TRUE;
                while (running)
                {
                    switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                    {
                        case Push_Abort:
                            abort_install();
                            break;
                        case Push_Proceed:
                            running = FALSE;
                            break;
                        case Push_Help:
                            helpwin(HELP_ON_PRETEND, PRETEND_HELP);
                            break;
                        default:
                            break;
                    }
                    WaitCTRL(sigs);
                }
                GetAttr(MUIA_Radio_Active, levelmx, (IPTR *)&logval);
                switch (logval)
                {
                    case 0: /* Really Install */
                        preferences.pretend = FALSE;
                        break;
                    case 1: /* Only pretend to install */
                        preferences.pretend = TRUE;
                        break;
                }

                DelContents(wc);
            }

            free(mxlabels[0]);
            free(mxlabels[1]);
        }
    }

    free(mxlabels);
    disable_skip(FALSE);
}


/*
 * Ask user for a boolean
 */
long int request_bool(struct ParameterList *pl)
{
long int retval;
char **mxlabels;
int i, m;

    NeedPROMPT(pl);
    NeedHELP(pl);

    m = GetPL(pl, _PROMPT).intval;

    retval = ( GetPL(pl, _DEFAULT).intval != 0 );

    mxlabels = malloc(3*sizeof(STRPTR));
    outofmem(mxlabels);
    if ( GetPL(pl, _CHOICES).used == 1 )
    {
        if (GetPL(pl, _CHOICES).intval >= 2)
        {
            mxlabels[0] = strdup(GetPL(pl, _CHOICES).arg[0]);
            mxlabels[1] = strdup(GetPL(pl, _CHOICES).arg[1]);
        }
        else if (GetPL(pl, _CHOICES).intval == 1)
        {
            mxlabels[0] = strdup(GetPL(pl, _CHOICES).arg[0]);
            mxlabels[1] = strdup(_(MSG_NO));
        }
        else
        {
            mxlabels[0] = strdup(_(MSG_YES));
            mxlabels[1] = strdup(_(MSG_NO));
        }
    }
    else
    {
        mxlabels[0] = strdup(_(MSG_YES));
        mxlabels[1] = strdup(_(MSG_NO));
    }
    mxlabels[2] = NULL;

    TRANSSCRIPT();

    if ( get_var_int("@user-level") > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *levelmx, *wc;
    ULONG sigs = 0;

        disable_skip(TRUE);
        out = collatestrings(m, GetPL(pl, _PROMPT).arg);

        wc = VGroup,
        Child, VGroup, GroupFrame,
                    MUIA_Background, MUII_GroupBack,
                    Child, TextObject,
                        MUIA_Text_Contents, (IPTR)(out),
                    End,
                    Child, levelmx = RadioObject,
                        MUIA_Radio_Entries, (IPTR)(mxlabels),
                    End,
                End,
            End;

        if (wc)
        {
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    case Push_Help:
                        if (GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(HELP_ON_ASKNUMBER, pl, _HELP);
                        }
                        else
                        {
                            helpwin(HELP_ON_ASKNUMBER, get_var_arg("@asknumber-help"));
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }
            GetAttr(MUIA_Radio_Active, levelmx, &retval);

            DelContents(wc);
            show_back(NULL);
        }
        free(out);
        disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, "Ask Question: Result was \"", 26);
        Write(preferences.transcriptstream, mxlabels[retval], strlen(mxlabels[retval]));
        Write(preferences.transcriptstream, "\".\n\n", 4);
    }
    free(mxlabels[0]);
    free(mxlabels[1]);
    free(mxlabels);

return retval;
}


/*
 * Ask user for a number
 */
long int request_number(struct ParameterList *pl)
{
long int retval;
long int i, min, max;
char minmax[MAXARGSIZE];

    retval = GetPL( pl, _DEFAULT ).intval;
    if ( GetPL( pl, _RANGE ).used == 1 )
    {
        min = GetPL( pl, _RANGE ).intval;
        max = GetPL( pl, _RANGE ).intval2;
        /* Wrong order ? Change order */
        if ( max < min )
        {
            i = min;
            min = max;
            max = i;
        }
        sprintf(minmax, "Range = [%ld, %ld]", min, max);
    }
    else
    {
        minmax[0] = 0;
#define INTMAX  32767
        max = INTMAX;
        min = ( retval < 0 ) ? retval : 0;
    }


    TRANSSCRIPT();
    if ( get_var_int( "@user-level" ) > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *st, *wc;
    ULONG sigs = 0;

        disable_skip(TRUE);
        out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

        wc = VGroup,
        Child, VGroup, GroupFrame,
                    MUIA_Background, MUII_GroupBack,
                    Child, TextObject,
                        MUIA_Text_Contents, (IPTR)(out),
                    End,
                    Child, st  = StringObject,
                        StringFrame,
                        MUIA_String_Accept,        (IPTR)"-0123456789",
                        MUIA_String_Integer,        retval,
                        MUIA_String_AdvanceOnCR,TRUE,
                        MUIA_CycleChain,        TRUE,
                    End,
                    Child, TextObject,
                        MUIA_Text_Contents, (IPTR)(minmax),
                    End,
                End,
            End;

        if (wc)
        {
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        GetAttr(MUIA_String_Integer, st, &retval);
                        if ( retval <= max && retval >= min)
                        {
                            running = FALSE;
                        }
                        break;
                    case Push_Help:
                        if (GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(HELP_ON_ASKSTRING, pl, _HELP);
                        }
                        else
                        {
                            helpwin(HELP_ON_ASKNUMBER, get_var_arg("@asknumber-help"));
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }
            GetAttr(MUIA_String_Integer, st, &retval);

            DelContents(wc);
            show_back(NULL);
        }
        free(out);
        disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != BNULL )
    {
    char tmpbuf[MAXARGSIZE];
        Write(preferences.transcriptstream, "Ask Number: Result was \"", 24);
        sprintf(tmpbuf, "%ld", retval);
        Write(preferences.transcriptstream, tmpbuf, strlen(tmpbuf));
        Write(preferences.transcriptstream, "\".\n\n", 4);
    }

return retval;
}


/*
 * Ask user for a string
 */
char *request_string(struct ParameterList *pl)
{
char *retval, *string = NULL;
int i;

    if ( GetPL(pl, _DEFAULT).used == 1 )
    {
        string = strdup(GetPL(pl, _DEFAULT).arg[0]);
    }
    else
    {
        string = strdup(EMPTY_STRING);
    }
    TRANSSCRIPT();
    if ( get_var_int( "@user-level" ) > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *st, *wc;
    ULONG sigs = 0;

        disable_skip(TRUE);
        out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

        wc = VGroup,
        Child, VGroup, GroupFrame,
                    MUIA_Background, MUII_GroupBack,
                    Child, TextObject,
                        MUIA_Text_Contents, (IPTR)(out),
                    End,
                    Child, st  = StringObject,
                        StringFrame,
                        MUIA_String_Contents,        (IPTR)string,
                        MUIA_String_MaxLen,        128,
                        MUIA_String_AdvanceOnCR,TRUE,
                        MUIA_CycleChain,        TRUE,
                    End,
                End,
            End;

        if (wc)
        {
            char *str = "";
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    case Push_Help:
                        if (GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(HELP_ON_ASKSTRING, pl, _HELP);
                        }
                        else
                        {
                            helpwin(HELP_ON_ASKNUMBER, get_var_arg("@asknumber-help"));
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }
            free(string);
            get(st, MUIA_String_Contents, &str);
            string = strdup(str);

            DelContents(wc);
            show_back(NULL);
        }
        free(out);
        disable_skip(FALSE);
    }
    retval = addquotes(string);
    free(string);
    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, "Ask String: Result was ", 23);
        Write(preferences.transcriptstream, retval, strlen(retval));
        Write(preferences.transcriptstream, ".\n\n", 3);
    }

return retval;
}

/*
 * Ask user to choose one of N items
 */
long int request_choice(struct ParameterList *pl)
{
long int retval;
char **mxlabels = NULL;
int i, max = 0;

    NeedPROMPT(pl);

    retval = GetPL(pl, _DEFAULT).intval;

    if ( GetPL(pl, _CHOICES).used == 1 )
    {
        max = GetPL(pl, _CHOICES).intval;

        if ( max > 32 )
        {
            error = SCRIPTERROR;
            traperr("More than 32 choices given!\n", NULL);
        }

        mxlabels = malloc((max+1)*sizeof(STRPTR));
        outofmem(mxlabels);

        for ( i = 0 ; i < max ; i++ )
        {
            mxlabels[i] = strdup(GetPL(pl, _CHOICES).arg[i]);
        }
        mxlabels[i] = NULL;
    }
    else
    {
        error = SCRIPTERROR;
        traperr("No choices given!\n", NULL);
    }

    TRANSSCRIPT();

    if ( get_var_int("@user-level") > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *levelmx, *wc;
    ULONG sigs = 0;

        disable_skip(TRUE);
        out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

        wc = VGroup,
        Child, VGroup, GroupFrame,
                    MUIA_Background, MUII_GroupBack,
                    Child, TextObject,
                        MUIA_Text_Contents, (IPTR)(out),
                    End,
                    Child, levelmx = RadioObject,
                        MUIA_Radio_Entries, (IPTR)(mxlabels),
                    End,
                End,
            End;

        if (wc)
        {
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    case Push_Help:
                        if (GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(HELP_ON_ASKCHOICE, pl, _HELP);
                        }
                        else
                        {
                            helpwin(HELP_ON_ASKCHOICE, get_var_arg("@asknumber-help"));
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }
            GetAttr(MUIA_Radio_Active, levelmx, &retval);

            DelContents(wc);
            show_back(NULL);
        }
        free(out);
        disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, "Ask Choice: Result was \"", 24);
        Write(preferences.transcriptstream, mxlabels[retval], strlen(mxlabels[retval]));
        Write(preferences.transcriptstream, "\".\n\n", 4);
    }
    for ( i = 0 ; i < max ; i++ )
    {
        free(mxlabels[i]);
    }
    free(mxlabels);

return retval;
}


/*
 * Shared body of askdir/askfile: prompt, string gadget, requester popup.
 * Novice users never see it and get the default.
 */
static char *request_path(struct ParameterList *pl, int dirsonly)
{
char *retval, *string = NULL;
int i;

    NeedPROMPT(pl);
    if ( GetPL(pl, _DEFAULT).used == 0 )
    {
        error = SCRIPTERROR;
        traperr("No default specified!", NULL);
    }
    string = strdup(GetPL(pl, _DEFAULT).arg[0]);
    outofmem(string);
    TRANSSCRIPT();
    if ( get_var_int( "@user-level" ) > _NOVICE )
    {
    char *out, *title, *nl;
    BOOL running = TRUE;
    Object *st, *wc;
    ULONG sigs = 0;

        disable_skip(TRUE);
        out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);
        /* the requester's title bar is one line: use the prompt's first */
        title = strdup(out);
        outofmem(title);
        if ((nl = strchr(title, '\n')) != NULL)
        {
            *nl = 0;
        }

        wc = VGroup,
            Child, VGroup, GroupFrame,
                MUIA_Background, MUII_GroupBack,
                Child, TextObject,
                    MUIA_Text_Contents, (IPTR)(out),
                End,
                Child, PopaslObject,
                    MUIA_Popasl_Type, ASL_FileRequest,
                    MUIA_Popstring_String, (IPTR)(st = StringObject,
                        StringFrame,
                        MUIA_String_Contents,   (IPTR)string,
                        MUIA_String_MaxLen,     256,
                        MUIA_String_AdvanceOnCR,TRUE,
                        MUIA_CycleChain,        TRUE,
                    End),
                    MUIA_Popstring_Button, (IPTR)PopButton(dirsonly ? MUII_PopDrawer : MUII_PopFile),
                    ASLFR_TitleText, (IPTR)title,
                    ASLFR_DrawersOnly, dirsonly,
                End,
            End,
        End;

        if (wc)
        {
            char *str = "";
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    case Push_Help:
                        if (GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(dirsonly ? HELP_ON_ASKDIR : HELP_ON_ASKFILE, pl, _HELP);
                        }
                        else
                        {
                            helpwin(dirsonly ? HELP_ON_ASKDIR : HELP_ON_ASKFILE,
                                    dirsonly ? ASKDIR_HELP : ASKFILE_HELP);
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }
            free(string);
            get(st, MUIA_String_Contents, &str);
            string = strdup(str);
            outofmem(string);

            DelContents(wc);
            show_back(NULL);
        }
        free(out);
        free(title);
        disable_skip(FALSE);
    }
    retval = addquotes(string);
    free(string);
    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, dirsonly ? "Ask Directory: Result was " : "Ask File: Result was ", dirsonly ? 26 : 21);
        Write(preferences.transcriptstream, retval, strlen(retval));
        Write(preferences.transcriptstream, ".\n\n", 3);
    }

return retval;
}


/*
 * Ask user for a directory
 */
char *request_dir(struct ParameterList *pl)
{
    return request_path(pl, TRUE);
}


/*
 * Ask user to insert a specific disk: loop until "dest:" can be locked.
 * (newname) assigns the found volume under a second name.
 */
char *request_disk(struct ParameterList *pl)
{
char *retval, *dest, *volname;
BPTR lock = BNULL;
int i, len, skipped = FALSE;
struct Process *proc;
APTR oldwin;

    NeedPROMPT(pl);
    if ( GetPL(pl, _DEST).used == 0 )
    {
        error = SCRIPTERROR;
        traperr("No dest specified!", NULL);
    }
    dest = GetPL(pl, _DEST).arg[0];
    TRANSSCRIPT();

    /* "Work:" or "Work" -- both mean the volume */
    len = strlen(dest);
    volname = malloc(len + 2);
    outofmem(volname);
    strcpy(volname, dest);
    if (len > 0 && volname[len - 1] == ':')
    {
        volname[len - 1] = 0;
    }
    strcat(volname, ":");

    /* This page *is* the "please insert volume" requester, so DOS must
       not put up its own one for every probe */
    proc = (struct Process *)FindTask(NULL);
    oldwin = proc->pr_WindowPtr;
    proc->pr_WindowPtr = (APTR)-1;
    while ((lock = Lock(volname, SHARED_LOCK)) == BNULL && !skipped)
    {
    char *out;
    BOOL running = TRUE;
    Object *wc;
    ULONG sigs = 0;

        out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);
        wc = VGroup,
            Child, TextObject,
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                MUIA_Text_Contents, (IPTR)(out),
            End,
            End;
        if (wc)
        {
            AddContents(wc);
            show_back(pl);
            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed: /* retry */
                        running = FALSE;
                        break;
                    case Push_Skip:
                        skipped = TRUE;
                        running = FALSE;
                        break;
                    case Push_Help:
                        if (GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(HELP_ON_ASKDISK, pl, _HELP);
                        }
                        else
                        {
                            helpwin(HELP_ON_ASKDISK, ASKDISK_HELP);
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }
            DelContents(wc);
            show_back(NULL);
        }
        free(out);
    }
    proc->pr_WindowPtr = oldwin;

    if (lock != BNULL)
    {
        if (GetPL(pl, _NEWNAME).used == 1 && GetPL(pl, _NEWNAME).intval > 0
            && (preferences.pretend == 0 || GetPL(pl, _SAFE).used == 1))
        {
            /* AssignLock() owns the lock on success */
            if (AssignLock(GetPL(pl, _NEWNAME).arg[0], lock) == DOSFALSE)
            {
                UnLock(lock);
            }
            else
            {
                manifest_log('A', GetPL(pl, _NEWNAME).arg[0]);
            }
        }
        else
        {
            UnLock(lock);
        }
    }
    free(volname);

    retval = addquotes(dest);
    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, "Ask Disk: Result was ", 21);
        Write(preferences.transcriptstream, retval, strlen(retval));
        Write(preferences.transcriptstream, lock != BNULL ? " (found).\n\n" : " (skipped).\n\n", lock != BNULL ? 11 : 13);
    }

return retval;
}


/*
 * Ask user for a file
 */
char *request_file(struct ParameterList *pl)
{
    return request_path(pl, FALSE);
}


/*
 * Yes/no question during a file operation (overwrite? unprotect? replace
 * newer library?): Proceed = yes, Skip = no. Novice never sees it and
 * gets "def".
 */
int request_yesno(char *msg, struct ParameterList *pl, int def)
{
int retval = def;
BOOL running = TRUE;
Object *wc;
ULONG sigs = 0;

    if ( get_var_int("@user-level") > _NOVICE )
    {
        wc = VGroup,
            Child, TextObject,
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                MUIA_Text_Contents, (IPTR)(msg),
            End,
            End;

        if (wc)
        {
            AddContents(wc);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Proceed:
                        retval = TRUE;
                        running = FALSE;
                        break;
                    case Push_Skip:
                        retval = FALSE;
                        running = FALSE;
                        break;
                    case Push_Help:
                        if (pl != NULL && GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(HELP_ON_CONFIRM, pl, _HELP);
                        }
                        else
                        {
                            helpwin(HELP_ON_CONFIRM, USERCONFIRM_HELP);
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }

            DelContents(wc);
        }
    }
    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, ">", 1);
        Write(preferences.transcriptstream, msg, strlen(msg));
        Write(preferences.transcriptstream, retval ? "\nAnswer: yes\n\n" : "\nAnswer: no\n\n", retval ? 14 : 13);
    }

return retval;
}


/*
 * copyfiles (confirm): let the user pick which of n names to copy.
 * Returns a malloc'd flag per name (all preselected), NULL if skipped.
 */
char *request_files(struct ParameterList *pl, char **names, int n)
{
char *flags, *out;
BOOL running = TRUE;
Object *wc, *lv, *list;
ULONG sigs = 0;
int i;

    NeedPROMPT(pl);
    NeedHELP(pl);
    flags = malloc(n > 0 ? n : 1);
    outofmem(flags);
    for (i = 0 ; i < n ; i++)
    {
        flags[i] = 1;
    }
    if ( get_var_int("@user-level") < GetPL(pl, _CONFIRM).intval )
    {
        return flags;
    }

    out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);
    wc = VGroup,
        Child, TextObject,
            GroupFrame,
            MUIA_Background, MUII_GroupBack,
            MUIA_Text_Contents, (IPTR)(out),
        End,
        Child, lv = ListviewObject,
            MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_Always,
            MUIA_Listview_List, (IPTR)(list = ListObject,
                InputListFrame,
                MUIA_List_SourceArray, (IPTR)names,
            End),
        End,
    End;

    if (wc)
    {
        AddContents(wc);
        DoMethod(list, MUIM_List_Select, MUIV_List_Select_All, MUIV_List_Select_On, NULL);

        while (running)
        {
            switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
            {
                case Push_Abort:
                    abort_install();
                    break;
                case Push_Proceed:
                    for (i = 0 ; i < n ; i++)
                    {
                        LONG state = 0;
                        DoMethod(list, MUIM_List_Select, i, MUIV_List_Select_Ask, (IPTR)&state);
                        flags[i] = state ? 1 : 0;
                    }
                    running = FALSE;
                    break;
                case Push_Skip:
                    free(flags);
                    flags = NULL;
                    running = FALSE;
                    break;
                case Push_Help:
                    helpwinpl(HELP_ON_CONFIRM, pl, _HELP);
                    break;
                default:
                    break;
            }
            WaitCTRL(sigs);
        }

        DelContents(wc);
    }
    free(out);

    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, "Select files: ", 14);
        if (flags == NULL)
        {
            Write(preferences.transcriptstream, "skipped.\n\n", 10);
        }
        else
        {
            for (i = 0 ; i < n ; i++)
            {
                if (flags[i])
                {
                    Write(preferences.transcriptstream, names[i], strlen(names[i]));
                    Write(preferences.transcriptstream, " ", 1);
                }
            }
            Write(preferences.transcriptstream, "\n\n", 2);
        }
    }

return flags;
}


/*
 * Refresh the "working" text and give the GUI a chance to notice Abort.
 */
void update_working(char *msg)
{
ULONG sigs = 0;

    if (intermediate == NULL || working_text == NULL)
    {
        show_working(msg);
        return;
    }
    set(working_text, MUIA_Text_Contents, (IPTR)msg);
    if (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) == Push_Abort)
    {
        abort_install();
    }
}


/*
 * The copyfiles/copylib page: what is being copied, and the gauge the
 * original Installer shows for it (one tick per file, the current name
 * in the bar) - (nogauge) keeps a script from asking for this page.
 */
void show_copying(char *msg, long total)
{
    DoMethod(root, MUIM_Group_InitChange);
    drop_intermediate();
    DoMethod(root, MUIM_Group_ExitChange);

    intermediate = VGroup,
        GroupFrameT(_(MSG_MESSAGE)),
        Child, working_text = TextObject,
            MUIA_Text_Contents, (IPTR)(msg),
        End,
        Child, copy_gauge = GaugeObject,
            GaugeFrame,
            MUIA_Gauge_Horiz, TRUE,
            MUIA_Gauge_Max, total > 0 ? total : 1,
            MUIA_Gauge_Current, 0,
            MUIA_Gauge_DupInfoText, TRUE,
            MUIA_Gauge_InfoText, (IPTR)"",
        End,
    End;

    if (intermediate)
    {
        /* only Abort while copying, as in the original */
        disable_proceed(TRUE);
        disable_skip(TRUE);
        disable_help(TRUE);
        DoMethod(root, MUIM_Group_InitChange);
        DoMethod(root, OM_ADDMEMBER, (IPTR)intermediate);
        DoMethod(root, MUIM_Group_ExitChange);
    }
}


/*
 * Advance the copy gauge to <done> files with <file> named in the bar,
 * and give the GUI a chance to notice Abort.
 */
void update_copying(char *file, long done)
{
ULONG sigs = 0;

    if (intermediate == NULL || copy_gauge == NULL)
    {
        update_working(file);
        return;
    }
    SetAttrs(copy_gauge, MUIA_Gauge_InfoText, (IPTR)file,
                         MUIA_Gauge_Current, done, TAG_DONE);
    if (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) == Push_Abort)
    {
        abort_install();
    }
}


/*
 * Ask user for a selection of multiple items (choose m of n items)
 */
long int request_options(struct ParameterList *pl)
{
long int retval;
char **mxlabels = NULL;
int i, max = 0;
Object *marks[33];
Object *labels[33];
long int val;
BOOL j;

    NeedPROMPT(pl);

    retval = GetPL(pl, _DEFAULT).intval;

    if ( GetPL(pl, _CHOICES).used == 1 )
    {
        max = GetPL(pl, _CHOICES).intval;

        if ( max > 32 )
        {
            error = SCRIPTERROR;
            traperr("More than 32 choices given!\n", NULL);
        }

        mxlabels = malloc((max+1)*sizeof(STRPTR));
        outofmem(mxlabels);

        for ( i = 0 ; i < max ; i++ )
        {
            mxlabels[i] = strdup(GetPL(pl, _CHOICES).arg[i]);
            marks[i] = MUI_MakeObject(MUIO_Checkmark, (IPTR)mxlabels[i]);
            labels[i] = LLabel(mxlabels[i]);

            set(marks[i], MUIA_CycleChain, TRUE);
            if ( (retval & 1<<i) != 0 )
                set(marks[i], MUIA_Selected, TRUE);
        }
        mxlabels[i] = NULL;
    }
    else
    {
        error = SCRIPTERROR;
        traperr("No choices given!\n", NULL);
    }

    TRANSSCRIPT();

    if ( get_var_int("@user-level") > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *levelmx, *wc;
    ULONG sigs = 0;

        disable_skip(TRUE);
        out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

        wc = VGroup,
        Child, VGroup, GroupFrame,
                    MUIA_Background, MUII_GroupBack,
                    Child, TextObject,
                        MUIA_Text_Contents, (IPTR)(out),
                    End,
                    Child, levelmx = ColGroup(2),
                        GroupFrame,
                        MUIA_Background, MUII_GroupBack,
                    End,
                End,
            End;

        if (wc)
        {
            for ( i = 0 ; i < max ; i++ )
            {
                DoMethod(levelmx, OM_ADDMEMBER, (IPTR)marks[i]);
                DoMethod(levelmx, OM_ADDMEMBER, (IPTR)labels[i]);
            }
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    case Push_Help:
                        if (GetPL(pl, _HELP).intval)
                        {
                            helpwinpl(HELP_ON_ASKCHOICE, pl, _HELP);
                        }
                        else
                        {
                            helpwin(HELP_ON_ASKCHOICE, get_var_arg("@asknumber-help"));
                        }
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }
            retval = 0;
            for ( i = 0 ; i < max ; i++ )
            {
                GetAttr(MUIA_Selected, marks[i], &val);
                if (val)
                {
                    retval |= 1<<i;
                }
            }

            DelContents(wc);
            show_back(NULL);
        }
        free(out);
        disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != BNULL )
    {
        Write(preferences.transcriptstream, "Ask Options: Result was \"", 24);
        j = FALSE;
        for ( i = 0 ; i < max ; i++ )
        {
            if ( (retval & (1<<i)) != 0 )
            {
                if ( j )
                {
                    Write(preferences.transcriptstream, "\", \"", 4);
                }
                Write(preferences.transcriptstream, mxlabels[i], strlen(mxlabels[i]));
                j = TRUE;
            }
        }
        Write(preferences.transcriptstream, "\".\n\n", 4);
    }
    for ( i = 0 ; i < max ; i++ )
    {
        free(mxlabels[i]);
    }
    free(mxlabels);

return retval;
}


/*
 * Ask user to confirm
 */
int request_confirm(struct ParameterList * pl)
{
int retval = 1;
BOOL running = TRUE;
Object *wc;
ULONG sigs = 0;
char *out;

    NeedPROMPT(pl);
    NeedHELP(pl);

    if ( get_var_int("@user-level") >= GetPL(pl, _CONFIRM).intval )
    {
        out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

        wc = VGroup,
            Child, TextObject,
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                MUIA_Text_Contents, (IPTR)(out),
            End,
            End;

        if (wc)
        {
            AddContents(wc);
            show_back(pl);

            while (running)
            {
                switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
                {
                    case Push_Abort:
                        abort_install();
                        break;
                    case Push_Back:
                        back_pressed = TRUE;
                        retval = 0;
                        running = FALSE;
                        break;
                    case Push_Proceed:
                        running = FALSE;
                        break;
                    case Push_Skip:
                        retval = 0;
                        running = FALSE;
                        break;
                    case Push_Help:
                        helpwinpl(HELP_ON_CONFIRM, pl, _HELP);
                        break;
                    default:
                        break;
                }
                WaitCTRL(sigs);
            }

            DelContents(wc);
            show_back(NULL);
        }
        free(out);
    }

return retval;
}


/*
 * Give a short summary on what was done
 */
void final_report()
{
#ifdef DEBUG
    printf("Application has been installed in %s.\n", get_var_arg("@default-dest"));
#endif /* DEBUG */
}


void display_text(char * msg)
{
BOOL running = TRUE;
LONG sigs = 0;
Object *wc;
Object *btok;

    wc = VGroup,
        Child, TextObject,
            GroupFrameT(_(MSG_MESSAGE)),
            MUIA_Text_Contents, (IPTR)msg,
        End,
        Child, HBar(TRUE),
        Child, HGroup,
            Child, btok = CoolImageIDButton(_(MSG_OK), COOL_USEIMAGE_ID),
        End,
    End;
    set(btok,MUIA_CycleChain,1);
    DoMethod(btok, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_Ok);
    DoMethod(reqroot, OM_ADDMEMBER, (IPTR)wc);

    set(reqwnd, MUIA_Window_Open, TRUE);
    while (running)
    {
        switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
        {
            case Push_Ok:
                running = FALSE;
                break;
            default:
                break;
        }
        WaitCTRL(sigs);
    }
    set(reqwnd, MUIA_Window_Open, FALSE);

    DoMethod(reqroot, OM_REMMEMBER, (IPTR)wc);
    MUI_DisposeObject(wc);
}


