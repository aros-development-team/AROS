/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Minimal GL test: clear the back buffer to a known colour and swap.
    Exercises the present path with no geometry or shaders.
    Solid dark red = clear/swap pipeline works.
*/

#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <GL/gla.h>

#include <stdio.h>

#define VISIBLE_WIDTH  300
#define VISIBLE_HEIGHT 300

CONST_STRPTR version = "$VER: glclear 1.0 (18.05.2026) AROS";

static GLAContext     glcont = NULL;
static struct Window *win    = NULL;
static BOOL           finished = FALSE;

static void initgl(void)
{
    struct TagItem attributes[10];
    int i = 0;

    attributes[i].ti_Tag = GLA_Window;    attributes[i++].ti_Data = (IPTR)win;
    attributes[i].ti_Tag = GLA_Left;      attributes[i++].ti_Data = win->BorderLeft;
    attributes[i].ti_Tag = GLA_Top;       attributes[i++].ti_Data = win->BorderTop;
    attributes[i].ti_Tag = GLA_Bottom;    attributes[i++].ti_Data = win->BorderBottom;
    attributes[i].ti_Tag = GLA_Right;     attributes[i++].ti_Data = win->BorderRight;
    attributes[i].ti_Tag = GLA_DoubleBuf; attributes[i++].ti_Data = GL_TRUE;
    attributes[i].ti_Tag = GLA_RGBMode;   attributes[i++].ti_Data = GL_TRUE;
    attributes[i].ti_Tag = GLA_NoStencil; attributes[i++].ti_Data = GL_TRUE;
    attributes[i].ti_Tag = GLA_NoAccum;   attributes[i++].ti_Data = GL_TRUE;
    attributes[i].ti_Tag = TAG_DONE;

    glcont = glACreateContext(attributes);
    if (glcont)
    {
        glAMakeCurrent(glcont);
        glViewport(0, 0, VISIBLE_WIDTH, VISIBLE_HEIGHT);
    }
    else
    {
        finished = TRUE;
    }
}

static void handle_messages(void)
{
    struct IntuiMessage *msg;
    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
    {
        switch (msg->Class)
        {
        case IDCMP_CLOSEWINDOW:
            finished = TRUE;
            break;
        case IDCMP_VANILLAKEY:
            if (msg->Code == 27)
                finished = TRUE;
            break;
        }
        ReplyMsg((struct Message *)msg);
    }
}

int main(void)
{
    struct Screen *pubscreen;

    if ((pubscreen = LockPubScreen(NULL)) == NULL)
        return 1;

    win = OpenWindowTags(0,
        WA_Title,         (IPTR)"glclear",
        WA_PubScreen,     pubscreen,
        WA_CloseGadget,   TRUE,
        WA_DragBar,       TRUE,
        WA_DepthGadget,   TRUE,
        WA_Left,          50,
        WA_Top,           200,
        WA_InnerWidth,    VISIBLE_WIDTH,
        WA_InnerHeight,   VISIBLE_HEIGHT,
        WA_Activate,      TRUE,
        WA_RMBTrap,       TRUE,
        WA_SimpleRefresh, TRUE,
        WA_NoCareRefresh, TRUE,
        WA_IDCMP,         IDCMP_VANILLAKEY | IDCMP_CLOSEWINDOW,
        TAG_DONE);

    UnlockPubScreen(NULL, pubscreen);

    if (!win)
        return 1;

    initgl();

    while (!finished)
    {
        glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glASwapBuffers(glcont);

        handle_messages();
    }

    if (glcont)
        glADestroyContext(glcont);

    CloseWindow(win);
    return 0;
}
