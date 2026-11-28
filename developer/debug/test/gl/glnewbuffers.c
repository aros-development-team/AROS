/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    GL test: the colour buffers of a new or resized window are black.

    The program reads the window's buffer before it has drawn anything into it:
    right after the context is created, and after each of three size changes
    of the window. Between the steps it fills the window with magenta, so
    that memory freed by a size change holds a colour that is easy to tell
    from black, and it fills free memory with a pattern before each step,
    because memory that happens to be zero hides the defect. Every pixel
    read must be black.

    Prints one line per step and PASS or FAIL; the return code is 0 or 20.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <GL/gl.h>
#include <GL/gla.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CONST_STRPTR version = "$VER: glnewbuffers 1.1 (28.11.2026) AROS";

static GLAContext     glcont = NULL;
static struct Window *win    = NULL;

static BOOL initgl(void)
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
    if (!glcont)
        return FALSE;

    glAMakeCurrent(glcont);
    return TRUE;
}

/* Number of pixels of the window's colour buffer that are not black, -1
 * without memory, -2 when OpenGL reports an error for the read; *filled
 * gets how many of them have the fill colour. */
static LONG not_black(LONG width, LONG height, LONG *filled)
{
    GLubyte *pixels = calloc((size_t)width * height, 4);
    LONG count = 0, i;

    *filled = 0;

    if (!pixels)
        return -1;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    while (glGetError() != GL_NO_ERROR)
        ;
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (glGetError() != GL_NO_ERROR)
    {
        free(pixels);
        return -2;
    }

    for (i = 0; i < width * height; i++)
    {
        if (pixels[i * 4] || pixels[i * 4 + 1] || pixels[i * 4 + 2])
        {
            count++;
            if (pixels[i * 4] == 255 && pixels[i * 4 + 1] == 0 && pixels[i * 4 + 2] == 255)
                (*filled)++;
        }
    }

    free(pixels);
    return count;
}

static BOOL check(CONST_STRPTR step, LONG width, LONG height)
{
    LONG filled;
    LONG count = not_black(width, height, &filled);

    if (count < 0)
    {
        printf("%s (%ldx%ld): %s\n", step, (long)width, (long)height,
            count == -1 ? "no memory for the pixels" : "OpenGL reports an error for the read");
        return FALSE;
    }

    printf("%s (%ldx%ld): %ld of %ld pixels are not black, %ld of them in the fill colour\n",
        step, (long)width, (long)height, (long)count, (long)(width * height), (long)filled);
    return count == 0;
}

/*
 * Free memory is often zero, which hides a buffer that was never cleared.
 * Fill a number of blocks with a pattern and free them again, so that the
 * buffers allocated next come from memory that is not black.
 */
#define DIRTY_BLOCKS 48
#define DIRTY_SIZE   (1024 * 1024)

static void dirty_free_memory(void)
{
    static APTR blocks[DIRTY_BLOCKS];
    int i;

    for (i = 0; i < DIRTY_BLOCKS; i++)
    {
        blocks[i] = AllocMem(DIRTY_SIZE, MEMF_ANY);
        if (blocks[i])
            memset(blocks[i], 0xA5, DIRTY_SIZE);
    }
    for (i = 0; i < DIRTY_BLOCKS; i++)
    {
        if (blocks[i])
            FreeMem(blocks[i], DIRTY_SIZE);
    }
}

/* Leave magenta in both buffers. */
static void fill(LONG width, LONG height)
{
    int i;

    glViewport(0, 0, width, height);
    for (i = 0; i < 3; i++)
    {
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glASwapBuffers(glcont);
    }
}

/* Change the inner size and wait until the window has it. */
static BOOL resize(LONG width, LONG height)
{
    struct IntuiMessage *msg;
    LONG wanted_w = width + win->BorderLeft + win->BorderRight;
    LONG wanted_h = height + win->BorderTop + win->BorderBottom;
    int tries;

    ChangeWindowBox(win, win->LeftEdge, win->TopEdge, wanted_w, wanted_h);

    for (tries = 0; tries < 100; tries++)
    {
        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
            ReplyMsg((struct Message *)msg);

        if (win->Width == wanted_w && win->Height == wanted_h)
            return TRUE;

        Delay(2);
    }

    return FALSE;
}

int main(void)
{
    static const LONG sizes[3][2] = { { 200, 150 }, { 360, 280 }, { 320, 240 } };
    struct Screen *pubscreen;
    BOOL ok = TRUE;
    int i;

    if ((pubscreen = LockPubScreen(NULL)) == NULL)
        return RETURN_FAIL;

    win = OpenWindowTags(0,
        WA_Title,         (IPTR)"glnewbuffers",
        WA_PubScreen,     pubscreen,
        WA_DragBar,       TRUE,
        WA_DepthGadget,   TRUE,
        WA_SizeGadget,    TRUE,
        WA_Left,          50,
        WA_Top,           50,
        WA_InnerWidth,    320,
        WA_InnerHeight,   240,
        WA_MinWidth,      100,
        WA_MinHeight,     100,
        WA_MaxWidth,      ~0,
        WA_MaxHeight,     ~0,
        WA_Activate,      TRUE,
        WA_RMBTrap,       TRUE,
        WA_SimpleRefresh, TRUE,
        WA_NoCareRefresh, TRUE,
        WA_IDCMP,         IDCMP_NEWSIZE,
        TAG_DONE);

    UnlockPubScreen(NULL, pubscreen);

    if (!win)
    {
        printf("the window does not open\n");
        return RETURN_FAIL;
    }

    dirty_free_memory();

    if (!initgl())
    {
        printf("no GL context\n");
        CloseWindow(win);
        return RETURN_FAIL;
    }

    ok = check("new window", 320, 240) && ok;
    fill(320, 240);

    for (i = 0; i < 3; i++)
    {
        char step[40];

        if (!resize(sizes[i][0], sizes[i][1]))
        {
            printf("size change %d: the window did not take the size\n", i + 1);
            ok = FALSE;
            break;
        }

        dirty_free_memory();

        /* gl.library notices the new size at a swap */
        glASwapBuffers(glcont);

        snprintf(step, sizeof(step), "size change %d", i + 1);
        ok = check((CONST_STRPTR)step, sizes[i][0], sizes[i][1]) && ok;
        fill(sizes[i][0], sizes[i][1]);
    }

    glADestroyContext(glcont);
    CloseWindow(win);

    printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? RETURN_OK : RETURN_FAIL;
}
