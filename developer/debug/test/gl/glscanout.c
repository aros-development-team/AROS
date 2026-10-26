/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    glscanout: 3D scan-out demo (VIDEO-OVERLAY-3D-SCANOUT-PLAN 3.7 / V5).

    Renders a rotating coloured cube through mesa3dgl into a window on the
    public screen or (SCREEN/S) on an own 32-bit screen, and prints once a
    second whether the window presents through a gallium.library scan-out
    framebuffer (the display flips the colour buffers itself) and whether
    a hardware plane currently shows it (SFBA_OnPlane), or whether the
    BltPipeResourceRastPort() blit path is in use. From second 2 until two
    thirds of the run a second plain window partially covers the GL window
    (NOCOVER/S skips it), so the visible-region tracking is exercised.

    ENV:MESA3DGL_NOSCANOUT forces the blit path for comparison.
*/
#include <aros/debug.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/datetime.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <GL/gla.h>
#include <gallium/gallium.h>
#include <proto/gallium.h>

#include <stdio.h>

CONST_STRPTR version = "$VER: glscanout 1.0 (25.10.2026) AROS";

struct Library *GalliumBase = NULL;

static GLAContext     glcont   = NULL;
static struct Window *win      = NULL;
static struct Window *cover    = NULL;
static struct Screen *scr      = NULL;
static BOOL           finished = FALSE;

static const char template[] = "SCREEN/S,SECONDS/N,WIDTH/N,HEIGHT/N,NOCOVER/S,BUFFERS/N";
enum { ARG_SCREEN, ARG_SECONDS, ARG_WIDTH, ARG_HEIGHT, ARG_NOCOVER, ARG_BUFFERS, ARG_COUNT };

static BOOL initgl(void)
{
    struct TagItem attributes[] =
    {
        { GLA_Window,    (IPTR)win        },
        { GLA_Left,      win->BorderLeft   },
        { GLA_Top,       win->BorderTop    },
        { GLA_Bottom,    win->BorderBottom },
        { GLA_Right,     win->BorderRight  },
        { GLA_DoubleBuf, GL_TRUE           },
        { GLA_RGBMode,   GL_TRUE           },
        { GLA_NoStencil, GL_TRUE           },
        { GLA_NoAccum,   GL_TRUE           },
        { TAG_DONE,      0                 }
    };

    glcont = glACreateContext(attributes);
    if (!glcont)
        return FALSE;

    glAMakeCurrent(glcont);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    return TRUE;
}

static void set_viewport(void)
{
    GLsizei w = win->Width - win->BorderLeft - win->BorderRight;
    GLsizei h = win->Height - win->BorderTop - win->BorderBottom;
    GLfloat aspect = (h > 0) ? (GLfloat)w / (GLfloat)h : 1.0f;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-aspect, aspect, -1.0, 1.0, 2.0, 20.0);
    glMatrixMode(GL_MODELVIEW);
}

static void draw_cube(GLfloat angle)
{
    static const GLfloat v[8][3] =
    {
        { -1, -1, -1 }, {  1, -1, -1 }, {  1,  1, -1 }, { -1,  1, -1 },
        { -1, -1,  1 }, {  1, -1,  1 }, {  1,  1,  1 }, { -1,  1,  1 }
    };
    /* counter-clockwise seen from outside */
    static const int faces[6][4] =
    {
        { 4, 5, 6, 7 }, /* front  (+z) */
        { 1, 0, 3, 2 }, /* back   (-z) */
        { 0, 4, 7, 3 }, /* left   (-x) */
        { 5, 1, 2, 6 }, /* right  (+x) */
        { 7, 6, 2, 3 }, /* top    (+y) */
        { 0, 1, 5, 4 }  /* bottom (-y) */
    };
    static const GLfloat colours[6][3] =
    {
        { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
        { 1, 1, 0 }, { 0, 1, 1 }, { 1, 0, 1 }
    };
    int f, i;

    glClearColor(0.08f, 0.08f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -6.0f);
    glRotatef(angle, 1.0f, 0.0f, 0.0f);
    glRotatef(angle * 0.7f, 0.0f, 1.0f, 0.0f);
    glRotatef(angle * 0.3f, 0.0f, 0.0f, 1.0f);

    glBegin(GL_QUADS);
    for (f = 0; f < 6; f++)
    {
        glColor3fv(colours[f]);
        for (i = 0; i < 4; i++)
            glVertex3fv(v[faces[f][i]]);
    }
    glEnd();
}

static void handle_messages(struct Window *w)
{
    struct IntuiMessage *msg;

    if (!w || !w->UserPort)
        return;

    while ((msg = (struct IntuiMessage *)GetMsg(w->UserPort)))
    {
        switch (msg->Class)
        {
        case IDCMP_CLOSEWINDOW:
            if (msg->IDCMPWindow == win)
                finished = TRUE;
            break;
        case IDCMP_VANILLAKEY:
            if (msg->Code == 27 || msg->Code == 'q')
                finished = TRUE;
            break;
        case IDCMP_NEWSIZE:
            if (msg->IDCMPWindow == win)
                set_viewport();
            break;
        }
        ReplyMsg((struct Message *)msg);
    }
}

static ULONG ticks_now(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ULONG)(ds.ds_Days * 24 * 60 + ds.ds_Minute) * TICKS_PER_SECOND + (ULONG)ds.ds_Tick;
}

static void report(ULONG second, ULONG frames)
{
    APTR fb = GalliumBase ? FindScanoutFrameBuffer(win) : NULL;

    if (fb)
    {
        printf("[%3lu s] %4lu fps  scan-out ACTIVE: %lu buffers, back %lu, %lu swaps, %s, %s\n",
               (unsigned long)second, (unsigned long)frames,
               (unsigned long)GetScanoutFrameBufferAttr(fb, SFBA_Buffers),
               (unsigned long)GetScanoutFrameBufferAttr(fb, SFBA_BackBuffer),
               (unsigned long)GetScanoutFrameBufferAttr(fb, SFBA_Swaps),
               GetScanoutFrameBufferAttr(fb, SFBA_OnPlane) ? "on a hardware PLANE" : "composited (blit)",
               cover ? "partially covered" : "uncovered");
    }
    else
    {
        printf("[%3lu s] %4lu fps  scan-out inactive: BltPipeResourceRastPort path%s, %s\n",
               (unsigned long)second, (unsigned long)frames,
               GalliumBase ? "" : " (gallium.library not open)",
               cover ? "partially covered" : "uncovered");
    }
    fflush(stdout);
}

int main(void)
{
    struct RDArgs *rda;
    IPTR args[ARG_COUNT] = { 0 };
    LONG seconds = 20, width = 400, height = 300;
    BOOL ownscreen = FALSE, nocover = FALSE;
    struct Screen *pubscreen = NULL;
    ULONG start, lastsec = 0, frames = 0, totalframes = 0;
    GLfloat angle = 0.0f;
    int rc = RETURN_FAIL;

    rda = ReadArgs(template, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "glscanout");
        return RETURN_ERROR;
    }
    ownscreen = args[ARG_SCREEN] ? TRUE : FALSE;
    nocover   = args[ARG_NOCOVER] ? TRUE : FALSE;
    if (args[ARG_SECONDS]) seconds = *(LONG *)args[ARG_SECONDS];
    if (args[ARG_WIDTH])   width   = *(LONG *)args[ARG_WIDTH];
    if (args[ARG_HEIGHT])  height  = *(LONG *)args[ARG_HEIGHT];
    if (args[ARG_BUFFERS])
    {
        LONG b = *(LONG *)args[ARG_BUFFERS];
        if (b == 2 || b == 3)
        {
            TEXT buf[2] = { '0' + b, 0 };
            SetVar("MESA3DGL_SCANOUT_BUFFERS", buf, 1, LV_VAR); /* local to this process */
        }
    }
    FreeArgs(rda);

    if (seconds < 1) seconds = 1;
    if (width < 64) width = 64;
    if (height < 64) height = 64;

    GalliumBase = OpenLibrary("gallium.library", 0);

    if (ownscreen)
    {
        scr = OpenScreenTags(NULL,
            SA_Width,     width + 200,
            SA_Height,    height + 160,
            SA_Depth,     24,
            SA_Title,     (IPTR)"glscanout",
            SA_ShowTitle, TRUE,
            TAG_DONE);
        if (!scr)
        {
            printf("glscanout: cannot open an own screen\n");
            goto out;
        }
    }
    else
    {
        pubscreen = LockPubScreen(NULL);
        if (!pubscreen)
            goto out;
    }

    win = OpenWindowTags(NULL,
        WA_Title,         (IPTR)"glscanout - rotating cube",
        scr ? WA_CustomScreen : WA_PubScreen, scr ? (IPTR)scr : (IPTR)pubscreen,
        WA_CloseGadget,   TRUE,
        WA_DragBar,       TRUE,
        WA_DepthGadget,   TRUE,
        WA_SizeGadget,    TRUE,
        WA_Left,          40,
        WA_Top,           60,
        WA_InnerWidth,    width,
        WA_InnerHeight,   height,
        WA_MinWidth,      100,
        WA_MinHeight,     100,
        WA_MaxWidth,      -1,
        WA_MaxHeight,     -1,
        WA_Activate,      TRUE,
        WA_RMBTrap,       TRUE,
        WA_SimpleRefresh, TRUE,
        WA_NoCareRefresh, TRUE,
        WA_IDCMP,         IDCMP_VANILLAKEY | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
        TAG_DONE);

    if (pubscreen)
        UnlockPubScreen(NULL, pubscreen);

    if (!win)
    {
        printf("glscanout: cannot open the window\n");
        goto out;
    }

    if (!initgl())
    {
        printf("glscanout: glACreateContext failed\n");
        goto out;
    }
    set_viewport();

    printf("glscanout: %s, %ldx%ld, %ld seconds%s; ESC or close gadget ends\n",
           scr ? "own screen" : "public screen", (long)width, (long)height, (long)seconds,
           nocover ? "" : ", a cover window from second 2");
    fflush(stdout);

    start = ticks_now();
    while (!finished)
    {
        ULONG now = ticks_now();
        ULONG elapsed = (now - start) / TICKS_PER_SECOND;

        draw_cube(angle);
        angle += 1.5f;
        if (angle >= 360.0f)
            angle -= 360.0f;

        glASwapBuffers(glcont);
        frames++;
        totalframes++;

        if (elapsed != lastsec)
        {
            report(elapsed, frames);
            frames = 0;
            lastsec = elapsed;
        }

        /* Cover the right half of the GL window from second 2 on, uncover
         * at two thirds of the run */
        if (!nocover && !cover && elapsed >= 2 && elapsed < (ULONG)(seconds * 2 / 3))
        {
            cover = OpenWindowTags(NULL,
                WA_Title,        (IPTR)"cover",
                WA_CustomScreen, (IPTR)win->WScreen,
                WA_DragBar,      TRUE,
                WA_DepthGadget,  TRUE,
                WA_Left,         win->LeftEdge + win->Width / 2,
                WA_Top,          win->TopEdge + win->Height / 4,
                WA_InnerWidth,   win->Width / 2 + 40,
                WA_InnerHeight,  win->Height / 2,
                WA_Activate,     FALSE,
                WA_IDCMP,        0,
                TAG_DONE);
        }
        if (cover && elapsed >= (ULONG)(seconds * 2 / 3))
        {
            CloseWindow(cover);
            cover = NULL;
        }

        if (elapsed >= (ULONG)seconds)
            finished = TRUE;

        handle_messages(win);
    }

    printf("glscanout: %lu frames\n", (unsigned long)totalframes);
    rc = RETURN_OK;

out:
    if (cover)
        CloseWindow(cover);
    if (glcont)
        glADestroyContext(glcont);
    if (win)
        CloseWindow(win);
    if (scr)
        CloseScreen(scr);
    if (GalliumBase)
        CloseLibrary(GalliumBase);

    return rc;
}
