/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_rb_functions.c 32149 2009-12-22 18:32:30Z deadwood $
*/

#include "aros_rb_functions.h"

#include <aros/debug.h>

#include <GL/arosmesa.h>
#include "main/formats.h"
#include "swrast/swrast.h"
#include "renderbuffer.h"


/*
* Write a horizontal span of pixels with a boolean mask.  The current color
* is used for all pixels.
*/
static void aros_renderbuffer_putmonorow( GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count, GLint x, GLint y,
                                 const void *value, const GLubyte *mask)
{
    D(bug("[AROSMESA] aros_renderbuffer_putmonorow\n"));
}

static void aros_renderbuffer_putmonovalues(GLcontext* ctx, struct gl_renderbuffer *rb, GLuint count, const GLint x[], 
                const GLint y[], const void *value, const GLubyte *mask)
{
    D(bug("[AROSMESA] aros_renderbuffer_putmonovalues\n"));
}




/* Write a horizontal span of RGBA color pixels with a boolean mask. */
/* PutRow */
static void aros_renderbuffer_putrow(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count, GLint x, GLint y,
                const void * values, const GLubyte mask[])
{
    AROSMesaRenderBuffer aros_rb = GET_AROS_RB_PTR(rb);
    int i;
    ULONG *dp = NULL;
    GLubyte * rgba = (GLubyte*)values;
    GLubyte * p_get;

    D(bug("[AROSMESA] aros_renderbuffer_putrow_buffer(count=%d,x=%d,y=%d)\n", count, x, y));


    y = CorrectY(y);

    dp = (ULONG*)aros_rb->buffer;
    dp += (y * rb->Width + x);

    if (mask)
    {
        /* draw some pixels */
        for (i = 0; i < count; i++, dp++)
        {
            p_get = rgba + i * 4;
            if (mask[i])
                /* draw pixel x,y using color red[i]/green[i]/blue[i]/alpha[i] */
                *dp = TC_ARGB(*(p_get + RCOMP), *(p_get + GCOMP), *(p_get + BCOMP), *(p_get + ACOMP));
        }
    }
    else
    {
        /* draw all pixels */
        for (i = 0; i < count; i++, dp++)
        {
            p_get = rgba + i * 4;
            /* draw pixel x,y using color red[i]/green[i]/blue[i]/alpha[i] */
            *dp = TC_ARGB(*(p_get + RCOMP), *(p_get + GCOMP), *(p_get + BCOMP), *(p_get + ACOMP));
        }
    }
 
}

/* Write a horizontal span of RGB color pixels with a boolean mask. */
/* PutRowRGB */
static void aros_renderbuffer_putrowrgb(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count, GLint x, GLint y,
                const void * values, const GLubyte mask[])
{
    AROSMesaRenderBuffer aros_rb = GET_AROS_RB_PTR(rb);
    int i;
    ULONG *dp = NULL;
    GLubyte * rgba = (GLubyte*)values;
    GLubyte * p_get;

    D(bug("[AROSMESA] aros_renderbuffer_putrow_buffer(count=%d,x=%d,y=%d)\n", count, x, y));


    y = CorrectY(y);

    dp = (ULONG*)aros_rb->buffer;
    dp += (y * rb->Width + x);

    if (mask)
    {
        /* draw some pixels */
        for (i = 0; i < count; i++, dp++)
        {
            p_get = rgba + i * 3;
            if (mask[i])
                /* draw pixel x,y using color red[i]/green[i]/blue[i]/alpha[i] */
                *dp = TC_ARGB(*(p_get + RCOMP), *(p_get + GCOMP), *(p_get + BCOMP), 255);
        }
    }
    else
    {
        /* draw all pixels */
        for (i = 0; i < count; i++, dp++)
        {
            p_get = rgba + i * 3;
            /* draw pixel x,y using color red[i]/green[i]/blue[i]/alpha[i] */
            *dp = TC_ARGB(*(p_get + RCOMP), *(p_get + GCOMP), *(p_get + BCOMP), 255);
        }
    }
 
}

/* Write an array of RGBA pixels with a boolean mask. */
/* PutValues */
static void aros_renderbuffer_putvalues(GLcontext* ctx, struct gl_renderbuffer *rb, GLuint count, const GLint x[], 
                const GLint y[], const void * values, const GLubyte * mask)

{
    AROSMesaRenderBuffer aros_rb = GET_AROS_RB_PTR(rb);    
    int i;
    ULONG *dp = NULL;
    GLubyte * rgba = (GLubyte*)values;
    GLubyte * p_get;

    D(bug("[AROSMESA] aros_renderbuffer_putvalues_buffer\n"));

    for (i = 0; i < count; i++)
    {

        if (mask[i])
        {
            p_get = rgba + i * 4;
            dp = (ULONG*)aros_rb->buffer;
            dp += (CorrectY(y[i]) * rb->Width + x[i]);
            /* write pixel x[i], y[i] using red[i],green[i],blue[i],alpha[i] */
            *dp = TC_ARGB(*(p_get + RCOMP), *(p_get + GCOMP), *(p_get + BCOMP), *(p_get + ACOMP));
        }
    }
}

/* Read an array of color pixels. */
/* GetValues */
static void aros_renderbuffer_getvalues( GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count, const GLint x[], 
                    const GLint y[], void * values)
{
    AROSMesaRenderBuffer aros_rb = GET_AROS_RB_PTR(rb);
    int i;
    ULONG *dp = NULL;
    ULONG col;
    GLubyte * rgba = (GLubyte*)values;
    GLubyte * p_set;

    D(bug("[AROSMESA] aros_renderbuffer_getvalues_buffer\n"));

    dp = (ULONG*)aros_rb->buffer;

    for (i = 0; i < count; i++)
    {
        col = *(dp + (CorrectY(y[i]) * rb->Width + x[i]));
        p_set = rgba + i * 4;

        *(p_set + RCOMP) = (col & 0xff0000) >> 16;
        *(p_set + GCOMP) = (col & 0xff00) >> 8;
        *(p_set + BCOMP) = col & 0xff;
        *(p_set + ACOMP) = 255;
    }
}

/* Read a horizontal span of color pixels. */
/* GetRow */
static void aros_renderbuffer_getrow(GLcontext* ctx, struct gl_renderbuffer *rb,
                          GLuint count, GLint x, GLint y, void * values)
{
    AROSMesaRenderBuffer aros_rb = GET_AROS_RB_PTR(rb);
    int i;
    ULONG *dp = NULL;
    ULONG col;
    GLubyte * rgba = (GLubyte*)values;
    GLubyte * p_set;

    D(bug("[AROSMESA] aros_renderbuffer_getrow_buffer\n"));

    y = CorrectY(y);
    
    dp = (ULONG*)aros_rb->buffer;
    dp += (y * rb->Width + x);

    for (i=0; i<count; i++, dp++)
    {
        col = *dp;
        p_set = rgba + i * 4;
        *(p_set + RCOMP) = (col & 0xff0000) >> 16;
        *(p_set + GCOMP) = (col & 0xff00) >> 8;
        *(p_set + BCOMP) = col & 0xff;
        *(p_set + ACOMP) = 255;
    }
}

static GLboolean 
_aros_renderbuffer_allocstorage(GLcontext *ctx, struct gl_renderbuffer *rb,
                            GLenum internalFormat, GLuint width, GLuint height)
{
    D(bug("[AROSMESA] _aros_renderbuffer_storage\n"));
    
    rb->PutRow = aros_renderbuffer_putrow;
    rb->PutRowRGB = aros_renderbuffer_putrowrgb;
    rb->PutMonoRow = aros_renderbuffer_putmonorow;
    rb->PutValues = aros_renderbuffer_putvalues;
    rb->PutMonoValues = aros_renderbuffer_putmonovalues;
    rb->GetRow = aros_renderbuffer_getrow;
    rb->GetValues = aros_renderbuffer_getvalues;
    rb->Width = width;
    rb->Height = height;

    if (GET_AROS_RB_PTR(rb)->buffer)
    {        
        FreeVec(GET_AROS_RB_PTR(rb)->buffer);
        GET_AROS_RB_PTR(rb)->buffer = NULL;
    }
    
    GET_AROS_RB_PTR(rb)->buffer = AllocVec(rb->Width * rb->Height * 4, MEMF_ANY);

    if (GET_AROS_RB_PTR(rb)->buffer == NULL)
        return GL_FALSE;

    return GL_TRUE;
}

static void 
_aros_renderbuffer_delete(struct gl_renderbuffer *rb)
{
    D(bug("[AROSMESA] _aros_renderbuffer_delete\n"));
    AROSMesaRenderBuffer aros_rb = GET_AROS_RB_PTR(rb);
    
    if (aros_rb && aros_rb->buffer)
    {
        FreeVec(aros_rb->buffer);
        aros_rb->buffer = NULL;
    }
    
    if (aros_rb)
    {
        FreeVec(aros_rb);
    }
}

AROSMesaRenderBuffer aros_new_renderbuffer(void)
{
    AROSMesaRenderBuffer aros_rb = NULL;
    struct gl_renderbuffer * rb = NULL;

    D(bug("[AROSMESA] aros_new_renderbuffer\n"));

    /* Allocated memory for aros structure */
    aros_rb = AllocVec(sizeof(struct arosmesa_renderbuffer), MEMF_PUBLIC | MEMF_CLEAR);
    rb = GET_GL_RB_PTR(aros_rb);

    /* Initialize mesa structure */
    _mesa_init_renderbuffer(rb, 0);

    if (rb)
    {
        rb->InternalFormat = GL_RGBA;
        rb->_BaseFormat = GL_RGBA;
        rb->Format = MESA_FORMAT_RGBA8888;
        rb->DataType = GL_UNSIGNED_BYTE;
        rb->AllocStorage = _aros_renderbuffer_allocstorage;
        rb->Delete = _aros_renderbuffer_delete;
        /* This will trigger AllocStorage */
        rb->Width = 0;
        rb->Height = 0;
    }
    
    /* Initialize aros structure */
    aros_rb->buffer = NULL;

    return aros_rb;
}
