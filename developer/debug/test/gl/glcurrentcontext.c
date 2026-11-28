/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    glAGetCurrentContext() test.

    The function must return the GLAContext that was made current, the
    value glACreateContext() returned, so that a program can put a
    context back with glAMakeCurrent(). Prints one line per check and
    ends with PASS or FAIL; the return code is RETURN_OK or RETURN_FAIL.
    Needs no input.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <GL/gla.h>

#include <stdio.h>

CONST_STRPTR version = "$VER: glcurrentcontext 1.0 (28.11.2026) AROS";

static struct Window *win;
static int failed;

static void check(BOOL ok, const char *what)
{
    if (!ok)
        failed++;
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
}

static GLAContext create(void)
{
    struct TagItem tags[] =
    {
        { GLA_Window,   (IPTR)win           },
        { GLA_Left,     win->BorderLeft     },
        { GLA_Top,      win->BorderTop      },
        { GLA_Right,    win->BorderRight    },
        { GLA_Bottom,   win->BorderBottom   },
        { GLA_RGBMode,  GL_TRUE             },
        { TAG_DONE,     0                   }
    };

    return glACreateContext(tags);
}

int main(void)
{
    GLAContext a, b, saved;

    win = OpenWindowTags(NULL,
        WA_Title,       (IPTR)"glcurrentcontext",
        WA_InnerWidth,  64,
        WA_InnerHeight, 64,
        WA_DragBar,     TRUE,
        WA_DepthGadget, TRUE,
        TAG_DONE);
    if (!win)
    {
        printf("FAIL no window\n");
        return RETURN_FAIL;
    }

    a = create();
    b = create();
    check(a != NULL && b != NULL, "two contexts");

    if (a && b)
    {
        check(glAGetCurrentContext() == NULL, "no context is current at the start");

        glAMakeCurrent(a);
        check(glAGetCurrentContext() == a, "A made current: A is returned");

        glAMakeCurrent(b);
        check(glAGetCurrentContext() == b, "B made current: B is returned");

        /* What a library does that must leave the caller's context in
           place. A value that is not a GLAContext must not be handed to
           glAMakeCurrent(), so this is left out after a failure above. */
        saved = glAGetCurrentContext();
        if (saved == b)
        {
            glAMakeCurrent(a);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glAMakeCurrent(saved);
            check(glAGetCurrentContext() == b, "saved and put back: B is current again");
            check(glGetError() == GL_NO_ERROR, "no GL error");
        }
        else
            check(FALSE, "saved and put back: not tried, the saved value is not B");

        glAMakeCurrent(NULL);
        check(glAGetCurrentContext() == NULL, "no context is current after glAMakeCurrent(NULL)");
    }

    if (a) glADestroyContext(a);
    if (b) glADestroyContext(b);
    CloseWindow(win);

    printf("glcurrentcontext: %s\n", failed ? "FAIL" : "PASS");
    return failed ? RETURN_FAIL : RETURN_OK;
}
