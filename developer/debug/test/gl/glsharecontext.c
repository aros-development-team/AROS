/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    GLA_ShareContext test.

    Three contexts on one window: A, B created with GLA_ShareContext = A,
    and C created without it. A texture made in A must exist in B with
    its contents, must not exist in C, and must stay usable in B after A
    is destroyed. Prints one line per check and ends with PASS or FAIL;
    the return code is RETURN_OK or RETURN_FAIL. Needs no input.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <GL/gla.h>

#include <stdio.h>

CONST_STRPTR version = "$VER: glsharecontext 1.0 (28.11.2026) AROS";

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER          0x8D40
#define GL_COLOR_ATTACHMENT0    0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif

#define TEX_SIZE 4

typedef void   (*PFN_GENFB)(GLsizei, GLuint *);
typedef void   (*PFN_DELFB)(GLsizei, const GLuint *);
typedef void   (*PFN_BINDFB)(GLenum, GLuint);
typedef void   (*PFN_FBTEX2D)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (*PFN_CHECKFB)(GLenum);

static struct Window *win;
static int failed;

static void check(BOOL ok, const char *what)
{
    if (!ok)
        failed++;
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
}

static GLAContext create(GLAContext share)
{
    struct TagItem tags[] =
    {
        { GLA_Window,   (IPTR)win           },
        { GLA_Left,     win->BorderLeft     },
        { GLA_Top,      win->BorderTop      },
        { GLA_Right,    win->BorderRight    },
        { GLA_Bottom,   win->BorderBottom   },
        { GLA_RGBMode,  GL_TRUE             },
        { share ? GLA_ShareContext : TAG_IGNORE, (IPTR)share },
        { TAG_DONE,     0                   }
    };

    return glACreateContext(tags);
}

/* Reads the texture's first pixel through a framebuffer object of the
   current context. Framebuffer objects are not shared, so each context
   makes its own. */
static BOOL texture_is_green(GLuint tex)
{
    PFN_GENFB   genfb   = (PFN_GENFB)glAGetProcAddress((const GLubyte *)"glGenFramebuffers");
    PFN_DELFB   delfb   = (PFN_DELFB)glAGetProcAddress((const GLubyte *)"glDeleteFramebuffers");
    PFN_BINDFB  bindfb  = (PFN_BINDFB)glAGetProcAddress((const GLubyte *)"glBindFramebuffer");
    PFN_FBTEX2D fbtex   = (PFN_FBTEX2D)glAGetProcAddress((const GLubyte *)"glFramebufferTexture2D");
    PFN_CHECKFB checkfb = (PFN_CHECKFB)glAGetProcAddress((const GLubyte *)"glCheckFramebufferStatus");
    GLubyte px[4] = { 0, 0, 0, 0 };
    GLuint fbo = 0;
    BOOL complete;

    if (!genfb || !delfb || !bindfb || !fbtex || !checkfb)
    {
        printf("     no framebuffer objects on this driver\n");
        return FALSE;
    }

    genfb(1, &fbo);
    bindfb(GL_FRAMEBUFFER, fbo);
    fbtex(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    complete = checkfb(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (complete)
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    bindfb(GL_FRAMEBUFFER, 0);
    delfb(1, &fbo);

    printf("     pixel %u,%u,%u,%u%s\n", px[0], px[1], px[2], px[3], complete ? "" : " (framebuffer incomplete)");
    return complete && px[0] == 0 && px[1] == 255 && px[2] == 0 && px[3] == 255;
}

int main(void)
{
    static GLubyte green[TEX_SIZE * TEX_SIZE * 4];
    GLAContext a, b, c;
    GLuint tex = 0;
    int i;

    win = OpenWindowTags(NULL,
        WA_Title,       (IPTR)"glsharecontext",
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

    for (i = 0; i < TEX_SIZE * TEX_SIZE; i++)
    {
        green[i * 4 + 1] = 255;
        green[i * 4 + 3] = 255;
    }

    a = create(NULL);
    check(a != NULL, "context A");
    b = a ? create(a) : NULL;
    check(b != NULL, "context B, GLA_ShareContext = A");
    c = create(NULL);
    check(c != NULL, "context C, not shared");

    if (a && b && c)
    {
        glAMakeCurrent(a);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, green);
        glFinish();
        check(glIsTexture(tex), "A: the texture exists");
        check(texture_is_green(tex), "A: the texture is green");

        glAMakeCurrent(b);
        check(glIsTexture(tex), "B: the texture of A exists");
        check(texture_is_green(tex), "B: the texture of A is green");

        glAMakeCurrent(c);
        check(!glIsTexture(tex), "C: the texture of A does not exist");

        glAMakeCurrent(NULL);
        glADestroyContext(a);
        a = NULL;

        glAMakeCurrent(b);
        check(glIsTexture(tex), "B after A is destroyed: the texture exists");
        check(texture_is_green(tex), "B after A is destroyed: the texture is green");
        glDeleteTextures(1, &tex);
        check(glGetError() == GL_NO_ERROR, "B: no GL error");
        glAMakeCurrent(NULL);
    }

    if (a) glADestroyContext(a);
    if (b) glADestroyContext(b);
    if (c) glADestroyContext(c);

    /* the driver was given up by the last context; a new one must work */
    a = create(NULL);
    check(a != NULL, "a new context after all were destroyed");
    if (a)
    {
        glAMakeCurrent(a);
        check(glGetString(GL_VERSION) != NULL, "the new context answers");
        glAMakeCurrent(NULL);
        glADestroyContext(a);
    }

    CloseWindow(win);

    printf("glsharecontext: %s\n", failed ? "FAIL" : "PASS");
    return failed ? RETURN_FAIL : RETURN_OK;
}
