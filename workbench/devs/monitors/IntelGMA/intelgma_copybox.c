/*
    Copyright © 2012-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Chris Wilson (intel-gpu-tools)
 *    2012-2017, The AROS Development Team.
 */

#include <proto/exec.h>
#include <hidd/gfx.h>
#include <aros/debug.h>

#include "intelgma_hidd.h"
#include "intelG45_regs.h"

//#include "i915/i915_reg.h" // crashes !?
#include "i915_reg.h"

#include "i915_3d.h"


extern struct g45staticdata sd;
#define sd ((struct g45staticdata*)&(sd))


static inline uint32_t pack_float(float f)
{
    union {
        uint32_t dw;
        float f;
    } u;
    u.f = f;
    return u.dw;
}

#define IS_915G(Id) ((Id) == 0x2582 || \
                     (Id) == 0x258a)
#define IS_915GM(Id) ((Id) == 0x2592)

#define IS_915(Id) (IS_915G(Id) || \
                    IS_915GM(Id))

#define IS_945G(Id)  ((Id) == 0x2772)
#define IS_945GM(Id) ((Id) == 0x27A2 || \
                      (Id) == 0x27AE)

#define IS_945(Id) (IS_945G(Id) ||  \
                    IS_945GM(Id) || \
                    IS_G33(Id) ||   \
                    IS_PINEVIEW(Id))

#define IS_G33(Id) ((Id) == 0x29C2 || \
                    (Id) == 0x29B2 || \
                    (Id) == 0x29D2)

#define IS_PINEVIEW(Id) ((Id) == 0xa001 || \
                         (Id) == 0xa011)

#define IS_GEN3(Id) (IS_915(Id) || \
                     IS_945(Id) || \
                     IS_G33(Id) || \
                     IS_PINEVIEW(Id))
              
BOOL copybox3d_supported()
{
    // supported chipsets
    if( IS_GEN3( sd->ProductID ) )
    {
        return TRUE;
    } 
    return FALSE;
}

BOOL copybox3d( GMABitMap_t *bm_dst, GMABitMap_t *bm_src,
               ULONG dst_x,ULONG dst_y,ULONG dst_width, ULONG dst_height,
               ULONG src_x,ULONG src_y,ULONG src_width, ULONG src_height )
{
    uint32_t dst_format;
    uint32_t src_format;

    if( !copybox3d_supported() ) 
    {
        return FALSE;
    }

    // buffers in gfx memory ?
    if( ! (bm_src->fbgfx && bm_src->fbgfx) )
    {
        return FALSE;
    }

    // Max texture size, src and dst must be differend (at least if overlaps)
    if( bm_src->pitch/4 > 2048 || bm_src->height > 2048 || bm_dst == bm_src )
    {
        return FALSE;
    }
    
    // src pitch must be long aligmented.
    if( bm_src->pitch & 0x3 )
    {
        bug("[GMA] copybox3d: ERROR bm_src->pitch=%d/n",bm_src->pitch);
        return FALSE;
    }

    if(bm_src->bpp == 4)
    {
        src_format = MAPSURF_32BIT | MT_32BIT_ARGB8888;
    }
    else if(bm_src->bpp == 2)
    {
        src_format = MAPSURF_16BIT | MT_16BIT_RGB565;
    }
    else
    {
        bug("[GMA] copybox3d: ERROR src_bpp=%d/n",bm_src->bpp);
        return FALSE;
    }

    if(bm_dst->bpp == 4)
    {
        dst_format = COLR_BUF_ARGB8888;
    }
    else if(bm_dst->bpp == 2)
    {
        dst_format = COLR_BUF_RGB565;
    }
    else
    {
        bug("[GMA] copybox3d: ERROR dst_bpp=%d/n",bm_dst->bpp);
        return FALSE;
    }

    LOCK_HW
    START_RING(72);

    /* invariant state */
    OUT_RING( _3DSTATE_AA_CMD |
        AA_LINE_ECAAR_WIDTH_ENABLE |
        AA_LINE_ECAAR_WIDTH_1_0 |
        AA_LINE_REGION_WIDTH_ENABLE | AA_LINE_REGION_WIDTH_1_0 );
    OUT_RING( _3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD |
        IAB_MODIFY_ENABLE |
        IAB_MODIFY_FUNC | (BLENDFUNC_ADD << IAB_FUNC_SHIFT) |
        IAB_MODIFY_SRC_FACTOR | (BLENDFACT_ONE <<
                     IAB_SRC_FACTOR_SHIFT) |
        IAB_MODIFY_DST_FACTOR | (BLENDFACT_ZERO <<
                     IAB_DST_FACTOR_SHIFT) );
    OUT_RING( _3DSTATE_DFLT_DIFFUSE_CMD );
    OUT_RING( 0 );
    OUT_RING( _3DSTATE_DFLT_SPEC_CMD );
    OUT_RING( 0 );
    OUT_RING( _3DSTATE_DFLT_Z_CMD );
    OUT_RING( 0 );
    OUT_RING( _3DSTATE_COORD_SET_BINDINGS |
        CSB_TCB(0, 0) |
        CSB_TCB(1, 1) |
        CSB_TCB(2, 2) |
        CSB_TCB(3, 3) |
        CSB_TCB(4, 4) |
        CSB_TCB(5, 5) | CSB_TCB(6, 6) | CSB_TCB(7, 7) );
    OUT_RING( _3DSTATE_RASTER_RULES_CMD |
        ENABLE_POINT_RASTER_RULE |
        OGL_POINT_RASTER_RULE |
        ENABLE_LINE_STRIP_PROVOKE_VRTX |
        ENABLE_TRI_FAN_PROVOKE_VRTX |
        LINE_STRIP_PROVOKE_VRTX(1) |
        TRI_FAN_PROVOKE_VRTX(2) | ENABLE_TEXKILL_3D_4D | TEXKILL_4D );
    OUT_RING( _3DSTATE_MODES_4_CMD |
        ENABLE_LOGIC_OP_FUNC | LOGIC_OP_FUNC(LOGICOP_COPY) |
        ENABLE_STENCIL_WRITE_MASK | STENCIL_WRITE_MASK(0xff) |
        ENABLE_STENCIL_TEST_MASK | STENCIL_TEST_MASK(0xff) );
    OUT_RING( _3DSTATE_LOAD_STATE_IMMEDIATE_1 | I1_LOAD_S(3) | I1_LOAD_S(4) | I1_LOAD_S(5) | 2 );
    OUT_RING( 0x00000000 );    /* Disable texture coordinate wrap-shortest */
    OUT_RING( (1 << S4_POINT_WIDTH_SHIFT) |
        S4_LINE_WIDTH_ONE |
        S4_CULLMODE_NONE |
        S4_VFMT_XY );
    OUT_RING( 0x00000000 );    /* Stencil. */
    OUT_RING( _3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT );
    OUT_RING( _3DSTATE_SCISSOR_RECT_0_CMD );
    OUT_RING( 0 );
    OUT_RING( 0 );
    OUT_RING( _3DSTATE_DEPTH_SUBRECT_DISABLE );
    OUT_RING( _3DSTATE_LOAD_INDIRECT | 0 );    /* disable indirect state */
    OUT_RING( 0 );
    OUT_RING( _3DSTATE_STIPPLE );
    OUT_RING( 0x00000000 );
    OUT_RING( _3DSTATE_BACKFACE_STENCIL_OPS | BFO_ENABLE_STENCIL_TWO_SIDE | 0 );

    /* samler state */
#define TEX_COUNT 1
    OUT_RING( _3DSTATE_MAP_STATE | (3 * TEX_COUNT) );
    OUT_RING( (1 << TEX_COUNT) - 1 );

    // Source buffer
    OUT_RING( bm_src->framebuffer );
    OUT_RING( src_format |
        (bm_src->height - 1) << MS3_HEIGHT_SHIFT |
        (bm_src->pitch/bm_src->bpp - 1)  << MS3_WIDTH_SHIFT );
    OUT_RING( (bm_src->pitch/4 -1) << MS4_PITCH_SHIFT );

    OUT_RING( _3DSTATE_SAMPLER_STATE | (3 * TEX_COUNT) );
    OUT_RING( (1 << TEX_COUNT) - 1 );
    OUT_RING( MIPFILTER_NONE << SS2_MIP_FILTER_SHIFT |
        FILTER_NEAREST << SS2_MAG_FILTER_SHIFT |
        FILTER_NEAREST << SS2_MIN_FILTER_SHIFT );
    OUT_RING( TEXCOORDMODE_WRAP << SS3_TCX_ADDR_MODE_SHIFT |
        TEXCOORDMODE_WRAP << SS3_TCY_ADDR_MODE_SHIFT |
        0 << SS3_TEXTUREMAP_INDEX_SHIFT );
    OUT_RING( 0x00000000 );

    /* render target state */
    
    // Destination buffer
    OUT_RING( _3DSTATE_BUF_INFO_CMD );
    OUT_RING( BUF_3D_ID_COLOR_BACK | bm_dst->pitch );
    OUT_RING( bm_dst->framebuffer );
    OUT_RING( _3DSTATE_DST_BUF_VARS_CMD );
    OUT_RING( dst_format |
        DSTORG_HORT_BIAS(0x8) |
        DSTORG_VERT_BIAS(0x8) );

    /* draw rect is unconditional */
    OUT_RING( _3DSTATE_DRAW_RECT_CMD );

    OUT_RING( 0x00000000 );
    OUT_RING( 0x00000000 );    // ymin, xmin 
    OUT_RING( DRAW_YMAX(dst_y + dst_height - 1) |
              DRAW_XMAX(dst_x + dst_width - 1) );

    /* yorig, xorig (relate to color buffer?) */
    OUT_RING( 0x00000000 );

    /* texfmt */
    OUT_RING( _3DSTATE_LOAD_STATE_IMMEDIATE_1 | I1_LOAD_S(1) | I1_LOAD_S(2) | I1_LOAD_S(6) | 2 );
    OUT_RING( (4 << S1_VERTEX_WIDTH_SHIFT) | (4 << S1_VERTEX_PITCH_SHIFT) );
    OUT_RING( ~S2_TEXCOORD_FMT(0, TEXCOORDFMT_NOT_PRESENT) |
        S2_TEXCOORD_FMT(0, TEXCOORDFMT_2D) );
    OUT_RING( S6_CBUF_BLEND_ENABLE | S6_COLOR_WRITE_ENABLE |
        BLENDFUNC_ADD << S6_CBUF_BLEND_FUNC_SHIFT |
        BLENDFACT_ONE << S6_CBUF_SRC_BLEND_FACT_SHIFT |
        BLENDFACT_ZERO << S6_CBUF_DST_BLEND_FACT_SHIFT );

    /* pixel shader */
    OUT_RING( _3DSTATE_PIXEL_SHADER_PROGRAM | (1 + 3*3 - 2) );
    /* decl FS_T0 */
    OUT_RING( D0_DCL |
        REG_TYPE(FS_T0) << D0_TYPE_SHIFT |
        REG_NR(FS_T0) << D0_NR_SHIFT |
        ((REG_TYPE(FS_T0) != REG_TYPE_S) ? D0_CHANNEL_ALL : 0) );
    OUT_RING( 0 );
    OUT_RING( 0 );
    /* decl FS_S0 */
    OUT_RING( D0_DCL |
        (REG_TYPE(FS_S0) << D0_TYPE_SHIFT) |
        (REG_NR(FS_S0) << D0_NR_SHIFT) |
        ((REG_TYPE(FS_S0) != REG_TYPE_S) ? D0_CHANNEL_ALL : 0) );
    OUT_RING( 0 );
    OUT_RING( 0 );
    /* texld(FS_OC, FS_S0, FS_T0 */
    OUT_RING( T0_TEXLD |
        (REG_TYPE(FS_OC) << T0_DEST_TYPE_SHIFT) |
        (REG_NR(FS_OC) << T0_DEST_NR_SHIFT) |
        (REG_NR(FS_S0) << T0_SAMPLER_NR_SHIFT) );
    OUT_RING( (REG_TYPE(FS_T0) << T1_ADDRESS_REG_TYPE_SHIFT) |
        (REG_NR(FS_T0) << T1_ADDRESS_REG_NR_SHIFT) );
    OUT_RING( 0 );
    
    // rectangle 
    // 3--x
    // |  |
    // 2--1
    OUT_RING( PRIM3D_RECTLIST | (3*4 - 1) );
    OUT_RING( pack_float( dst_x + dst_width) );
    OUT_RING( pack_float( dst_y + dst_height) );
    OUT_RING( pack_float(src_x + src_width) );
    OUT_RING( pack_float(src_y + src_height) );

    OUT_RING( pack_float( dst_x + 0 ) );
    OUT_RING( pack_float( dst_y +dst_height) );
    OUT_RING( pack_float(src_x + 0) );
    OUT_RING( pack_float(src_y + src_height) );

    OUT_RING( pack_float( dst_x + 0 ) );
    OUT_RING( pack_float( dst_y + 0 ) );
    OUT_RING( pack_float(src_x + 0) );
    OUT_RING( pack_float(src_y + 0) );

    ADVANCE_RING();
    DO_FLUSH();
    UNLOCK_HW

    return TRUE;
}
