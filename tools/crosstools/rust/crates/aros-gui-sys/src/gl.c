/* gl.library's GLA context for Rust on AROS. Contract: glue.h.

   A separate object from glue.c, so only programs that use GL pull in
   libGL's autoinit, which opens gl.library at startup. */
#include <stdint.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <GL/gla.h>

#include "glue.h"

/* The window interior, which is what the context renders into. */
static void set_rast_tags(struct TagItem *tags, struct Window *window)
{
    tags[0].ti_Tag = GLA_Window; tags[0].ti_Data = (IPTR)window;
    tags[1].ti_Tag = GLA_Left;   tags[1].ti_Data = window->BorderLeft;
    tags[2].ti_Tag = GLA_Top;    tags[2].ti_Data = window->BorderTop;
    tags[3].ti_Tag = GLA_Width;
    tags[3].ti_Data = window->Width - window->BorderLeft - window->BorderRight;
    tags[4].ti_Tag = GLA_Height;
    tags[4].ti_Data = window->Height - window->BorderTop - window->BorderBottom;
}

uintptr_t aros_gui_gl_create(uintptr_t handle, int alpha, int depth, int stencil)
{
    struct Window *window = (struct Window *)handle;
    struct TagItem tags[11];

    if (!window)
        return 0;
    set_rast_tags(tags, window);
    tags[5].ti_Tag = GLA_DoubleBuf; tags[5].ti_Data = GL_TRUE;
    tags[6].ti_Tag = GLA_RGBMode;   tags[6].ti_Data = GL_TRUE;
    tags[7].ti_Tag = GLA_AlphaFlag; tags[7].ti_Data = alpha ? GL_TRUE : GL_FALSE;
    tags[8].ti_Tag = GLA_NoDepth;   tags[8].ti_Data = depth ? GL_FALSE : GL_TRUE;
    tags[9].ti_Tag = GLA_NoStencil; tags[9].ti_Data = stencil ? GL_FALSE : GL_TRUE;
    tags[10].ti_Tag = TAG_DONE;
    return (uintptr_t)glACreateContext(tags);
}

void aros_gui_gl_destroy(uintptr_t ctx)
{
    if (ctx)
        glADestroyContext((GLAContext)ctx);
}

void aros_gui_gl_make_current(uintptr_t ctx)
{
    glAMakeCurrent((GLAContext)ctx);
}

uintptr_t aros_gui_gl_current(void)
{
    return (uintptr_t)glAGetCurrentContext();
}

void aros_gui_gl_swap_buffers(uintptr_t ctx)
{
    if (!ctx)
        return;
    glASwapBuffers((GLAContext)ctx);
}

void aros_gui_gl_set_window(uintptr_t ctx, uintptr_t handle)
{
    struct TagItem tags[6];

    if (!ctx || !handle)
        return;
    set_rast_tags(tags, (struct Window *)handle);
    tags[5].ti_Tag = TAG_DONE;
    glASetRast((GLAContext)ctx, tags);
}

/* gl.library's fixed vectors stop at GL 1.x; everything newer is looked up. */
void *aros_gui_gl_proc(const char *name)
{
    return name ? (void *)glAGetProcAddress((const GLubyte *)name) : NULL;
}
