/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    FBO/render-to-texture test for the vc4 T-format path.

    Pass 1 renders a known quadrant pattern into a 320x200 RGBA texture
    via an EXT_framebuffer_object FBO:
        top half           = red
        bottom-left        = green
        bottom-right       = blue (clear colour)
    Pass 2 draws that texture as a fullscreen quad in a 640x480 window.

    Both passes are read back with glReadPixels and sample points are
    printed. glReadPixels of the texture goes through Mesa's CPU-side
    tiled->raster conversion, so it verifies the STORE side; the window
    readback (and the visible result) verifies the SAMPLING side.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <GL/gla.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define WIN_W 640
#define WIN_H 480
#define FBO_W 320
#define FBO_H 200

CONST_STRPTR version = "$VER: gltexfbo 1.0 (04.07.2026) AROS";

#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT          0x8D40
#define GL_COLOR_ATTACHMENT0_EXT    0x8CE0
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

typedef void   (*PFN_GENFB)(GLsizei, GLuint *);
typedef void   (*PFN_BINDFB)(GLenum, GLuint);
typedef void   (*PFN_FBTEX2D)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (*PFN_CHECKFB)(GLenum);

static PFN_GENFB   pglGenFramebuffers;
static PFN_BINDFB  pglBindFramebuffer;
static PFN_FBTEX2D pglFramebufferTexture2D;
static PFN_CHECKFB pglCheckFramebufferStatus;

/* GL2 shader/attrib entry points for pass C (the scummvm-style path) */
static PFNGLCREATESHADERPROC            pglCreateShader;
static PFNGLSHADERSOURCEPROC            pglShaderSource;
static PFNGLCOMPILESHADERPROC           pglCompileShader;
static PFNGLGETSHADERIVPROC             pglGetShaderiv;
static PFNGLCREATEPROGRAMPROC           pglCreateProgram;
static PFNGLATTACHSHADERPROC            pglAttachShader;
static PFNGLLINKPROGRAMPROC             pglLinkProgram;
static PFNGLUSEPROGRAMPROC              pglUseProgram;
static PFNGLBINDATTRIBLOCATIONPROC      pglBindAttribLocation;
static PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC pglDisableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC     pglVertexAttribPointer;
static PFNGLGETUNIFORMLOCATIONPROC      pglGetUniformLocation;
static PFNGLUNIFORM1IPROC               pglUniform1i;

#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#endif

static const GLchar *vs_src =
"attribute vec2 a_pos;"
"attribute vec2 a_uv;"
"varying vec2 v_uv;"
"void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }";

static const GLchar *fs_src =
"uniform sampler2D u_tex;"
"varying vec2 v_uv;"
"void main() { gl_FragColor = texture2D(u_tex, v_uv); }";

static GLuint prg_global = 0;

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
        glAMakeCurrent(glcont);
    else
        finished = TRUE;
}

/* Print to both stdout and the serial log (driver lines live there). */
static void out(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("%s", buf);
    bug("%s", buf);
}

static const char *classify(const GLubyte *px)
{
    if (px[0] > 128 && px[1] < 96 && px[2] < 96)   return "RED  ";
    if (px[0] < 96 && px[1] > 128 && px[2] < 96)   return "GREEN";
    if (px[0] < 96 && px[1] < 96 && px[2] > 128)   return "BLUE ";
    if (px[0] < 32 && px[1] < 32 && px[2] < 32)    return "BLACK";
    return "OTHER";
}

static void sample(const char *what, int x, int y, const char *expect)
{
    GLubyte px[4] = { 0, 0, 0, 0 };
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    out("[gltexfbo] %s (%3d,%3d) = %02x%02x%02x%02x %s (expect %s)\n",
        what, x, y, px[0], px[1], px[2], px[3], classify(px), expect);
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
    GLuint tex = 0, tex2 = 0, fbo = 0;
    int round = 0;
    static GLuint srcpix[640 * 480];

    if ((pubscreen = LockPubScreen(NULL)) == NULL)
        return 1;

    win = OpenWindowTags(0,
        WA_Title,         (IPTR)"gltexfbo",
        WA_PubScreen,     pubscreen,
        WA_CloseGadget,   TRUE,
        WA_DragBar,       TRUE,
        WA_DepthGadget,   TRUE,
        WA_Left,          50,
        WA_Top,           100,
        WA_InnerWidth,    WIN_W,
        WA_InnerHeight,   WIN_H,
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
    if (finished)
    {
        CloseWindow(win);
        return 1;
    }

    pglGenFramebuffers        = (PFN_GENFB)glAGetProcAddress("glGenFramebuffersEXT");
    pglBindFramebuffer        = (PFN_BINDFB)glAGetProcAddress("glBindFramebufferEXT");
    pglFramebufferTexture2D   = (PFN_FBTEX2D)glAGetProcAddress("glFramebufferTexture2DEXT");
    pglCheckFramebufferStatus = (PFN_CHECKFB)glAGetProcAddress("glCheckFramebufferStatusEXT");

    if (!pglGenFramebuffers || !pglBindFramebuffer ||
        !pglFramebufferTexture2D || !pglCheckFramebufferStatus)
    {
        out("[gltexfbo] EXT_framebuffer_object entry points missing\n");
        CloseWindow(win);
        return 1;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FBO_W, FBO_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    /* tex2: CPU-uploaded source texture (the scummvm game-screen role) */
    glGenTextures(1, &tex2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    {
        int x, y;
        for (y = 0; y < 480; y++)
            for (x = 0; x < 640; x++)          /* t=0 rows: YELLOW; t=1: CYAN */
                srcpix[y * 640 + x] = (y < 240) ? 0xFF00FFFFUL : 0xFFFFFF00UL;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 640, 480, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, srcpix);
    glBindTexture(GL_TEXTURE_2D, tex);

    pglGenFramebuffers(1, &fbo);

    while (!finished)
    {
        GLenum status;

        /* ---- Pass 1: render pattern into the texture ---- */
        pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
        pglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                GL_TEXTURE_2D, tex, 0);
        status = pglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            if (round == 0)
                out("[gltexfbo] FBO incomplete: 0x%04x\n", (unsigned)status);
            finished = TRUE;
            break;
        }

        glViewport(0, 0, FBO_W, FBO_H);
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);          /* blue */
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);                              /* red: top half */
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
        glVertex2f( 1.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
        glColor3f(0.0f, 1.0f, 0.0f);                    /* green: bottom-left */
        glVertex2f(-1.0f, -1.0f); glVertex2f(0.0f, -1.0f);
        glVertex2f( 0.0f,  0.0f); glVertex2f(-1.0f, 0.0f);
        glEnd();

        if (round == 0)
        {
            out("[gltexfbo] -- pass 1 readback (FBO, GL origin=bottom) --\n");
            sample("fbo", 10, 10, "GREEN");
            sample("fbo", 310, 10, "BLUE ");
            sample("fbo", 10, 190, "RED  ");
            sample("fbo", 310, 190, "RED  ");
            sample("fbo", 160, 100, "RED  ");
        }

        /* ---- Pass 2: draw the texture as a fullscreen quad ---- */
        pglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
        glViewport(0, 0, WIN_W, WIN_H);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
        glEnd();

        if (round == 0)
        {
            int r;
            out("[gltexfbo] -- pass 2 readback (window, GL origin=bottom) --\n");
            sample("win", 20, 20, "GREEN");
            sample("win", 620, 20, "BLUE ");
            sample("win", 20, 460, "RED  ");
            sample("win", 620, 460, "RED  ");
            sample("win", 320, 240, "RED  ");
            out("[gltexfbo] -- vertical map, x=320 --\n");
            for (r = 10; r < WIN_H; r += 60)
                sample("col", 320, r, r < WIN_H / 2 ? "GREEN/BLUE" : "RED  ");

            /* ---- Pass B: client vertex arrays + GL_LINEAR ---- */
            {
                static const GLfloat vtx[] = {
                    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
                };
                static const GLfloat uv[] = {
                    0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
                };

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glClear(GL_COLOR_BUFFER_BIT);
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glVertexPointer(2, GL_FLOAT, 0, vtx);
                glTexCoordPointer(2, GL_FLOAT, 0, uv);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);

                out("[gltexfbo] -- pass B (vertex arrays + LINEAR) --\n");
                sample("pB ", 20, 20, "GREEN");
                sample("pB ", 620, 20, "BLUE ");
                sample("pB ", 20, 460, "RED  ");
                sample("pB ", 320, 240, "RED  ");
                for (r = 10; r < WIN_H; r += 120)
                    sample("pBc", 320, r, r < WIN_H / 2 ? "GREEN/BLUE" : "RED  ");
            }

            /* ---- Pass C: GLSL shader + vertex attrib arrays ---- */
            pglCreateShader   = (PFNGLCREATESHADERPROC)glAGetProcAddress("glCreateShader");
            pglShaderSource   = (PFNGLSHADERSOURCEPROC)glAGetProcAddress("glShaderSource");
            pglCompileShader  = (PFNGLCOMPILESHADERPROC)glAGetProcAddress("glCompileShader");
            pglGetShaderiv    = (PFNGLGETSHADERIVPROC)glAGetProcAddress("glGetShaderiv");
            pglCreateProgram  = (PFNGLCREATEPROGRAMPROC)glAGetProcAddress("glCreateProgram");
            pglAttachShader   = (PFNGLATTACHSHADERPROC)glAGetProcAddress("glAttachShader");
            pglLinkProgram    = (PFNGLLINKPROGRAMPROC)glAGetProcAddress("glLinkProgram");
            pglUseProgram     = (PFNGLUSEPROGRAMPROC)glAGetProcAddress("glUseProgram");
            pglBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)glAGetProcAddress("glBindAttribLocation");
            pglEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glAGetProcAddress("glEnableVertexAttribArray");
            pglDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)glAGetProcAddress("glDisableVertexAttribArray");
            pglVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glAGetProcAddress("glVertexAttribPointer");
            pglGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glAGetProcAddress("glGetUniformLocation");
            pglUniform1i      = (PFNGLUNIFORM1IPROC)glAGetProcAddress("glUniform1i");

            if (pglCreateShader && pglVertexAttribPointer)
            {
                static const GLfloat vtx[] = {
                    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
                };
                static const GLfloat uv[] = {
                    0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
                };
                GLuint vsh = pglCreateShader(GL_VERTEX_SHADER);
                GLuint fsh = pglCreateShader(GL_FRAGMENT_SHADER);
                GLuint prg = pglCreateProgram();
                prg_global = prg;
                GLint  ok1 = 0, ok2 = 0;

                pglShaderSource(vsh, 1, &vs_src, NULL);
                pglCompileShader(vsh);
                pglGetShaderiv(vsh, GL_COMPILE_STATUS, &ok1);
                pglShaderSource(fsh, 1, &fs_src, NULL);
                pglCompileShader(fsh);
                pglGetShaderiv(fsh, GL_COMPILE_STATUS, &ok2);

                pglAttachShader(prg, vsh);
                pglAttachShader(prg, fsh);
                pglBindAttribLocation(prg, 0, "a_pos");
                pglBindAttribLocation(prg, 1, "a_uv");
                pglLinkProgram(prg);
                pglUseProgram(prg);
                pglUniform1i(pglGetUniformLocation(prg, "u_tex"), 0);

                glClear(GL_COLOR_BUFFER_BIT);
                pglEnableVertexAttribArray(0);
                pglEnableVertexAttribArray(1);
                pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
                pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                pglDisableVertexAttribArray(0);
                pglDisableVertexAttribArray(1);
                pglUseProgram(0);

                out("[gltexfbo] -- pass C (GLSL + attrib arrays) vs=%d fs=%d --\n",
                       (int)ok1, (int)ok2);
                sample("pC ", 20, 20, "GREEN");
                sample("pC ", 620, 20, "BLUE ");
                sample("pC ", 20, 460, "RED  ");
                sample("pC ", 320, 240, "RED  ");
                for (r = 10; r < WIN_H; r += 120)
                    sample("pCc", 320, r, r < WIN_H / 2 ? "GREEN/BLUE" : "RED  ");
            }
            else
                out("[gltexfbo] pass C skipped: no GL2 entry points\n");

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        /* ---- Rounds 2/3: scummvm-style flow. Round 2 draws the CPU-
         * uploaded tex2 INTO the FBO (shader path, two draws = two shader
         * recs), then FBO -> window. Round 3 first UPDATES tex2 while it
         * is GPU-busy (glTexSubImage2D after sampling), then repeats —
         * exercising Mesa's busy-resource upload path. ---- */
        if ((round == 1 || round == 2) && pglCreateShader)
        {
            static const GLfloat vtx[] = {
                -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
            };
            static const GLfloat uv[] = {
                0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
            };
            static const GLfloat vtx2[] = {   /* small quad, bottom-left */
                -1.0f, -1.0f,  -0.75f, -1.0f,  -0.75f, -0.75f,  -1.0f, -0.75f
            };
            const char *tag = (round == 1) ? "r2 " : "r3 ";
            const char *topexp = (round == 1) ? "YELLOWISH" : "GREEN";
            const char *botexp = (round == 1) ? "CYANISH" : "RED  ";
            int r;

            if (round == 2)
            {
                /* Update the (busy) source texture: swap the pattern to
                 * GREEN top / RED bottom. */
                int x, y;
                for (y = 0; y < 480; y++)
                    for (x = 0; x < 640; x++)
                        srcpix[y * 640 + x] =
                            (y < 240) ? 0xFF00FF00UL : 0xFF0000FFUL;
                glBindTexture(GL_TEXTURE_2D, tex2);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 640, 480,
                                GL_RGBA, GL_UNSIGNED_BYTE, srcpix);
            }

            /* Pass 1': draw tex2 into the FBO via the GLSL path */
            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
            glViewport(0, 0, FBO_W, FBO_H);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            pglUseProgram(prg_global);
            glBindTexture(GL_TEXTURE_2D, tex2);
            pglEnableVertexAttribArray(0);
            pglEnableVertexAttribArray(1);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            /* second draw: small quad, second shader rec in the submit */
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx2);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            out("[gltexfbo] -- %s FBO readback (tex2 -> fbo, GLSL) --\n", tag);
            sample(tag, 160, 150, topexp);   /* top half of tex2 (t<0.5) */
            sample(tag, 160, 50, botexp);    /* bottom half */

            /* Pass 2': FBO texture -> window */
            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
            glViewport(0, 0, WIN_W, WIN_H);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, tex);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            pglDisableVertexAttribArray(0);
            pglDisableVertexAttribArray(1);
            pglUseProgram(0);

            out("[gltexfbo] -- %s window readback --\n", tag);
            sample(tag, 320, 360, topexp);
            sample(tag, 320, 120, botexp);
            for (r = 10; r < WIN_H; r += 120)
                sample(tag, 320, r, "map  ");
        }

        /* ---- Round 4: the remaining scummvm particulars — PARTIAL
         * glTexSubImage2D (dirty-rect with x/y offset), GL_LINEAR on the
         * FBO texture, and a letterboxed sub-viewport in the FBO pass. */
        if (round == 3 && pglCreateShader)
        {
            static const GLfloat vtx[] = {
                -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
            };
            static const GLfloat uv[] = {
                0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
            };
            static GLuint strip[400 * 20];
            int i, r;

            /* white dirty-rect at tex rows 100..119, x 50..449 */
            for (i = 0; i < 400 * 20; i++)
                strip[i] = 0xFFFFFFFFUL;
            glBindTexture(GL_TEXTURE_2D, tex2);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 50, 100, 400, 20,
                            GL_RGBA, GL_UNSIGNED_BYTE, strip);

            /* FBO texture sampled with LINEAR from now on */
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            /* Pass 1'': letterboxed viewport inside the FBO */
            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
            glViewport(0, 0, FBO_W, FBO_H);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glViewport(0, 20, FBO_W, FBO_H - 40);   /* 20px letterbox */

            pglUseProgram(prg_global);
            glBindTexture(GL_TEXTURE_2D, tex2);
            pglEnableVertexAttribArray(0);
            pglEnableVertexAttribArray(1);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            out("[gltexfbo] -- r4 FBO map x=160 (letterbox 0..19/180..199 BLACK, "
                "white strip ~y=53..60) --\n");
            for (r = 5; r < FBO_H; r += 13)
                sample("r4f", 160, r, "map  ");

            /* Pass 2'': FBO -> window (LINEAR) */
            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
            glViewport(0, 0, WIN_W, WIN_H);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, tex);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            pglDisableVertexAttribArray(0);
            pglDisableVertexAttribArray(1);
            pglUseProgram(0);

            out("[gltexfbo] -- r4 window map x=320 (black<48, white ~127..145, "
                "black>432) --\n");
            for (r = 10; r < WIN_H; r += 30)
                sample("r4w", 320, r, "map  ");
        }

        /* ---- Round 5: BLENDING + SCISSOR in the FBO pass (the last
         * untested scummvm mechanics: alpha-blended cursor/overlay draws
         * and scissored dirty-region rendering). */
        if (round == 4 && pglCreateShader)
        {
            static const GLfloat vtx[] = {
                -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
            };
            static const GLfloat uv[] = {
                0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
            };
            int r;

            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
            glViewport(0, 0, FBO_W, FBO_H);
            glClearColor(0.0f, 0.0f, 1.0f, 1.0f);      /* blue base */
            glClear(GL_COLOR_BUFFER_BIT);

            /* scissored region: only rows 80..119 may be written */
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 80, FBO_W, 40);

            /* blended full quad: tex2 at 50% alpha over blue */
            glEnable(GL_BLEND);
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
            /* CONSTANT_ALPHA needs glBlendColor — fall back to additive
             * which needs no extra entry points and still exercises the
             * blend path: result = src + dst. */
            glBlendFunc(GL_ONE, GL_ONE);

            pglUseProgram(prg_global);
            glBindTexture(GL_TEXTURE_2D, tex2);
            pglEnableVertexAttribArray(0);
            pglEnableVertexAttribArray(1);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            pglDisableVertexAttribArray(0);
            pglDisableVertexAttribArray(1);
            pglUseProgram(0);
            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);

            /* Expected FBO content: pure blue everywhere EXCEPT rows
             * 80..119: green+blue=cyan (rows 80..99 sample tex2 green
             * region? tex2 t<0.5=GREEN rows -> fbo lower half) and
             * red+blue=magenta above t=0.5. The white strip adds white. */
            out("[gltexfbo] -- r5 FBO map x=160 (blend+scissor: blue outside "
                "80..119, cyan/magenta inside) --\n");
            for (r = 60; r < 140; r += 10)
                sample("r5f", 160, r, "map  ");
        }

        /* ---- Round 6: the scummvm dirty-rect torture — many small
         * position-coded glTexSubImage2D updates interleaved with draws
         * (texture busy), with GL_UNPACK_ROW_LENGTH set. Each rect is
         * filled with a colour encoding its index; the readback then
         * shows exactly where each rect actually landed. */
        if (round == 5 && pglCreateShader)
        {
            static const GLfloat vtx[] = {
                -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
            };
            static const GLfloat uv[] = {
                0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
            };
            static GLuint big[640 * 480];  /* ROW_LENGTH=640 source surface */
            int i, x, y;

            /* Base: mid-grey everywhere */
            for (i = 0; i < 640 * 480; i++)
                big[i] = 0xFF808080UL;
            glBindTexture(GL_TEXTURE_2D, tex2);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 640, 480,
                            GL_RGBA, GL_UNSIGNED_BYTE, big);

            glEnableClientState(GL_VERTEX_ARRAY); /* keep attribs simple */
            glDisableClientState(GL_VERTEX_ARRAY);

            pglUseProgram(prg_global);
            pglEnableVertexAttribArray(0);
            pglEnableVertexAttribArray(1);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);

            glPixelStorei(GL_UNPACK_ROW_LENGTH, 640);

            /* 12 rects, 40x20 each, at spread positions; colour = index
             * coded in the green channel (0x10 * (i+1)). Interleave each
             * upload with a draw so the texture is busy. */
            for (i = 0; i < 12; i++)
            {
                int rx = (i % 4) * 150 + 20;   /* 20,170,320,470 */
                int ry = (i / 4) * 150 + 30;   /* 30,180,330 */
                GLuint col = 0xFF000000UL | ((0x10UL * (i + 1)) << 8);

                /* paint the rect region inside the big surface */
                for (y = 0; y < 20; y++)
                    for (x = 0; x < 40; x++)
                        big[(ry + y) * 640 + (rx + x)] = col;

                glTexSubImage2D(GL_TEXTURE_2D, 0, rx, ry, 40, 20,
                                GL_RGBA, GL_UNSIGNED_BYTE,
                                &big[ry * 640 + rx]);

                /* draw into the FBO so the texture is busy for the next
                 * upload */
                pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
                glViewport(0, 0, FBO_W, FBO_H);
                glBindTexture(GL_TEXTURE_2D, tex2);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

            /* Final: FBO -> window, then verify every rect via the FBO
             * coordinates (640x480 -> 320x200: x/2, y*200/480). */
            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
            glViewport(0, 0, WIN_W, WIN_H);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, tex);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            pglDisableVertexAttribArray(0);
            pglDisableVertexAttribArray(1);
            pglUseProgram(0);

            out("[gltexfbo] -- r6 dirty-rect torture: each rect i should "
                "read green=0x10*(i+1) --\n");
            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
            for (i = 0; i < 12; i++)
            {
                int rx = (i % 4) * 150 + 20 + 20;   /* rect centre */
                int ry = (i / 4) * 150 + 30 + 10;
                /* tex2 y -> fbo y: v = y/480; fbo row = v*200.
                 * GL readback y = fbo row (same origin as render). */
                int fx = rx / 2;
                int fy = (int)((long)ry * 200 / 480);
                char what[8];
                snprintf(what, sizeof(what), "r6_%02d", i);
                sample(what, fx, fy, "GREEN-coded");
            }
            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
        }

        /* ---- Round 7: 8-bit (cpp=1) texture — the scummvm CLUT8/eduke32
         * indexed path. GL_LUMINANCE 320x200 with position-coded content:
         * horizontal bands of distinct grey levels (32*band) plus a white
         * column at x=100..107. cpp=1 T-format uses 8x8 utiles — a layout
         * never exercised by the cpp=4 rounds. */
        if (round == 6 && pglCreateShader)
        {
            static const GLfloat vtx[] = {
                -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
            };
            static const GLfloat uv[] = {
                0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
            };
            static GLubyte lum[320 * 200];
            GLuint tex3 = 0;
            int x, y, r;

            for (y = 0; y < 200; y++)
                for (x = 0; x < 320; x++)
                    lum[y * 320 + x] =
                        (x >= 100 && x < 108) ? 0xFF
                                              : (GLubyte)((y / 25) * 32 + 16);

            glGenTextures(1, &tex3);
            glBindTexture(GL_TEXTURE_2D, tex3);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 320, 200, 0,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE, lum);

            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
            glViewport(0, 0, FBO_W, FBO_H);
            glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            pglUseProgram(prg_global);
            pglEnableVertexAttribArray(0);
            pglEnableVertexAttribArray(1);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            pglDisableVertexAttribArray(0);
            pglDisableVertexAttribArray(1);
            pglUseProgram(0);

            /* 1:1 mapping (320x200 tex -> 320x200 fbo, v flipped by uv).
             * Band value at fbo row y = ((199-y)/25)*32+16.
             * Expect x=50: band greys; x=103: white 0xFF. */
            out("[gltexfbo] -- r7 8-bit LUMINANCE map (fbo row y: grey = "
                "((199-y)/25)*32+16; x=103 white) --\n");
            for (r = 12; r < FBO_H; r += 25)
                sample("r7a", 50, r, "band ");
            for (r = 12; r < FBO_H; r += 50)
                sample("r7b", 103, r, "WHITE");
        }

        /* ---- Round 8: isolate the cpp=1 failure seen in round 7.
         * r8a: pure CPU round-trip (glTexImage2D store -> glGetTexImage
         * load) of a T-format 8-bit texture. The two directions use the
         * same address math, so a mismatch means the CPU tiling code
         * itself is broken; a clean pass means the layout is at least
         * self-consistent and the TMU disagrees with it.
         * r8b: 32-wide 8-bit texture = LT format (not T) drawn and read
         * back, to tell whether LT cpp=1 works while T cpp=1 fails. */
        if (round == 7 && pglCreateShader)
        {
            static const GLfloat vtx[] = {
                -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f
            };
            static const GLfloat uv[] = {
                0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
            };
            static GLubyte lum[320 * 200], back[320 * 200];
            static GLubyte lumlt[32 * 200];
            GLuint t8 = 0, t8lt = 0;
            int x, y, bad = 0, r;

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);

            for (y = 0; y < 200; y++)
                for (x = 0; x < 320; x++)
                    lum[y * 320 + x] = (GLubyte)(x * 7 + y * 13);

            glGenTextures(1, &t8);
            glBindTexture(GL_TEXTURE_2D, t8);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 320, 200, 0,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE, lum);

            memset(back, 0xAA, sizeof(back));
            glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                          back);
            for (y = 0; y < 200; y++)
                for (x = 0; x < 320; x++)
                    if (back[y * 320 + x] != lum[y * 320 + x])
                    {
                        if (bad < 4)
                            out("[gltexfbo] r8a MISMATCH (%3d,%3d) "
                                "got=%02x want=%02x\n", x, y,
                                back[y * 320 + x], lum[y * 320 + x]);
                        bad++;
                    }
            out("[gltexfbo] r8a T cpp=1 CPU round-trip: %d mismatches "
                "of 64000\n", bad);

            for (y = 0; y < 200; y++)
                for (x = 0; x < 32; x++)
                    lumlt[y * 32 + x] = (GLubyte)((y / 25) * 32 + 16);

            glGenTextures(1, &t8lt);
            glBindTexture(GL_TEXTURE_2D, t8lt);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 32, 200, 0,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE, lumlt);

            memset(back, 0xAA, sizeof(back));
            glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                          back);
            bad = 0;
            for (y = 0; y < 200; y++)
                for (x = 0; x < 32; x++)
                    if (back[y * 32 + x] != lumlt[y * 32 + x])
                        bad++;
            out("[gltexfbo] r8b LT cpp=1 CPU round-trip: %d mismatches "
                "of 6400\n", bad);

            pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
            glViewport(0, 0, FBO_W, FBO_H);
            glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            pglUseProgram(prg_global);
            pglEnableVertexAttribArray(0);
            pglEnableVertexAttribArray(1);
            pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vtx);
            pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, uv);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            pglDisableVertexAttribArray(0);
            pglDisableVertexAttribArray(1);
            pglUseProgram(0);

            out("[gltexfbo] -- r8c LT cpp=1 TMU sample (8 clean bands = "
                "LT ok) --\n");
            for (r = 12; r < FBO_H; r += 25)
                sample("r8c", 50, r, "band ");
        }

        if (round < 8)
            round++;

        glASwapBuffers(glcont);
        handle_messages();
    }

    if (fbo)
    {
        /* no delete fn fetched; process exit cleans up */
    }
    if (glcont)
        glADestroyContext(glcont);

    CloseWindow(win);
    return 0;
}
