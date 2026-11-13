/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Isolation test for the vc4 VideoCore wedge seen when a scaled (non-native)
    secondary custom screen is used with GL. Native display is e.g. 1280x1024;
    opening a 640x480 custom screen makes the firmware upscale it (a scaled HVS
    plane). Something in the scaled-secondary + GL path hard-wedges the
    VideoCore (HDMI drops, machine dies).

    Run with increasing STEP to find the exact step that wedges; each step logs
    to the serial console, so the last [GLCS] line before death is the culprit:

      glcustomscreen               (STEP 1) open scaled custom screen, hold, close
      glcustomscreen STEP=2        + open a backdrop window on it
      glcustomscreen STEP=3        + glACreateContext (vc4gallium init)
      glcustomscreen STEP=4        + glAMakeCurrent + clear + swap (render)

    W/H override the screen size (default 640x480), HOLD the per-step hold secs.
*/

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <dos/dos.h>
#include <dos/rdargs.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#include <aros/debug.h>

#include <GL/gla.h>

CONST_STRPTR version = "$VER: glcustomscreen 1.0 (19.07.2026) AROS";

int main(void)
{
    IPTR args[4] = { 0, 0, 0, 0 };      /* STEP, W, H, HOLD */
    struct RDArgs *rda;
    LONG step = 1, w = 640, h = 480, hold = 3;
    struct Screen *scr = NULL;
    struct Window *win = NULL;
    GLAContext ctx = NULL;
    ULONG err = 0;

    rda = ReadArgs("STEP/N,W/N,H/N,HOLD/N", args, NULL);
    if (rda)
    {
        if (args[0]) step = *(LONG *)args[0];
        if (args[1]) w    = *(LONG *)args[1];
        if (args[2]) h    = *(LONG *)args[2];
        if (args[3]) hold = *(LONG *)args[3];
    }

    bug("[GLCS] START step=%ld  %ldx%ld  hold=%lds\n",
        (long)step, (long)w, (long)h, (long)hold);

    /* ---- STEP 1: open a scaled (non-native) custom screen -------------- */
    bug("[GLCS] step1: OpenScreenTags %ldx%ld depth 24...\n", (long)w, (long)h);
    scr = OpenScreenTags(NULL,
        SA_GammaControl, TRUE,
        SA_Width,        w,
        SA_Height,       h,
        SA_Depth,        24,
        SA_Quiet,        TRUE,
        SA_ShowTitle,    FALSE,
        SA_Title,        (IPTR)"GLCS",
        SA_ErrorCode,    (IPTR)&err,
        TAG_DONE);
    bug("[GLCS] step1: screen=%p err=%lu\n", scr, (unsigned long)err);
    if (!scr)
        goto done;
    bug("[GLCS] step1: opened %ldx%ld, holding...\n",
        (long)scr->Width, (long)scr->Height);
    Delay(hold * 50);
    bug("[GLCS] step1: SURVIVED\n");
    if (step < 2)
        goto close_scr;

    /* ---- STEP 2: open a backdrop window on the custom screen ----------- */
    bug("[GLCS] step2: OpenWindowTags backdrop...\n");
    win = OpenWindowTags(NULL,
        WA_Left,         0,
        WA_Top,          0,
        WA_InnerWidth,   scr->Width,
        WA_InnerHeight,  scr->Height,
        WA_CustomScreen, (IPTR)scr,
        WA_Flags,        WFLG_ACTIVATE | WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_RMBTRAP,
        WA_IDCMP,        IDCMP_VANILLAKEY,
        TAG_DONE);
    bug("[GLCS] step2: win=%p\n", win);
    if (!win)
        goto close_scr;
    Delay(hold * 50);
    bug("[GLCS] step2: SURVIVED\n");
    if (step < 3)
        goto close_win;

    /* ---- STEP 3: create a GL context (triggers vc4gallium init) -------- */
    {
        struct TagItem attrs[8];
        int i = 0;

        attrs[i].ti_Tag = GLA_Window;    attrs[i++].ti_Data = (IPTR)win;
        attrs[i].ti_Tag = GLA_DoubleBuf; attrs[i++].ti_Data = GL_TRUE;
        attrs[i].ti_Tag = GLA_RGBMode;   attrs[i++].ti_Data = GL_TRUE;
        attrs[i].ti_Tag = GLA_NoStencil; attrs[i++].ti_Data = GL_TRUE;
        attrs[i].ti_Tag = GLA_NoAccum;   attrs[i++].ti_Data = GL_TRUE;
        attrs[i].ti_Tag = TAG_DONE;

        bug("[GLCS] step3: glACreateContext...\n");
        ctx = glACreateContext(attrs);
        bug("[GLCS] step3: ctx=%p\n", ctx);
    }
    if (!ctx)
        goto close_win;
    Delay(hold * 50);
    bug("[GLCS] step3: SURVIVED\n");
    if (step < 4)
        goto destroy_ctx;

    /* ---- STEP 4: make current, clear, swap ---------------------------- */
    bug("[GLCS] step4: glAMakeCurrent + render...\n");
    glAMakeCurrent(ctx);
    glViewport(0, 0, scr->Width, scr->Height);
    {
        int f, frames = hold * 10;

        for (f = 0; f < frames; f++)
        {
            glClearColor(0.2f, 0.0f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glASwapBuffers(ctx);
            if ((f % 10) == 0)
                bug("[GLCS] step4: frame %d\n", f);
        }
    }
    bug("[GLCS] step4: SURVIVED\n");

destroy_ctx:
    bug("[GLCS] cleanup: glADestroyContext\n");
    if (ctx)
        glADestroyContext(ctx);
close_win:
    bug("[GLCS] cleanup: CloseWindow\n");
    if (win)
        CloseWindow(win);
close_scr:
    bug("[GLCS] cleanup: CloseScreen\n");
    if (scr)
        CloseScreen(scr);
done:
    if (rda)
        FreeArgs(rda);
    bug("[GLCS] DONE\n");
    return 0;
}
