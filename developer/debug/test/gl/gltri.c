/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Minimal GL test: clear + one immediate-mode triangle, fixed-function
    pipeline (no glCreateShader/glUseProgram). Triangle over dark red =
    basic rendering works, isolating any corruption to the user-shader path.
*/
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <GL/gla.h>

#include <stdio.h>

#define VISIBLE_WIDTH  300
#define VISIBLE_HEIGHT 300

CONST_STRPTR version = "$VER: gltri 1.0 (18.05.2026) AROS";

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
        bug("[gltri] before glAMakeCurrent\n");
        glAMakeCurrent(glcont);
        bug("[gltri] before glViewport\n");
        glViewport(0, 0, VISIBLE_WIDTH, VISIBLE_HEIGHT);
        bug("[gltri] before glMatrixMode(GL_PROJECTION)\n");
        glMatrixMode(GL_PROJECTION);
        bug("[gltri] before glLoadIdentity (1)\n");
        glLoadIdentity();
        bug("[gltri] before glMatrixMode(GL_MODELVIEW)\n");
        glMatrixMode(GL_MODELVIEW);
        bug("[gltri] before glLoadIdentity (2)\n");
        glLoadIdentity();
        bug("[gltri] initgl done\n");
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
        WA_Title,         (IPTR)"gltri",
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

    static int dbg_once = 0;
    while (!finished)
    {
        glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!dbg_once) bug("[gltri] before glBegin\n");
        glBegin(GL_TRIANGLES);
        if (!dbg_once) bug("[gltri] before glColor3f #1\n");
            glColor3f(1.0f, 0.0f, 0.0f);
        if (!dbg_once) bug("[gltri] before glVertex2f #1\n");
            glVertex2f(-0.5f, -0.5f);
        if (!dbg_once) bug("[gltri] before glColor3f #2\n");
            glColor3f(0.0f, 1.0f, 0.0f);
        if (!dbg_once) bug("[gltri] before glVertex2f #2\n");
            glVertex2f( 0.5f, -0.5f);
        if (!dbg_once) bug("[gltri] before glColor3f #3\n");
            glColor3f(0.0f, 0.0f, 1.0f);
        if (!dbg_once) bug("[gltri] before glVertex2f #3\n");
            glVertex2f( 0.0f,  0.5f);
        if (!dbg_once) bug("[gltri] before glEnd\n");
        glEnd();
        if (!dbg_once) bug("[gltri] before glASwapBuffers\n");
        glASwapBuffers(glcont);
        if (!dbg_once) bug("[gltri] frame done\n");
        dbg_once = 1;

        handle_messages();
    }


    if (glcont)
        glADestroyContext(glcont);

    CloseWindow(win);
    return 0;
}
