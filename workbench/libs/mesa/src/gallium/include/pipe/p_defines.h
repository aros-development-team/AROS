/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef PIPE_DEFINES_H
#define PIPE_DEFINES_H

#include "p_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gallium error codes.
 *
 * - A zero value always means success.
 * - A negative value always means failure.
 * - The meaning of a positive value is function dependent.
 */
enum pipe_error {
   PIPE_OK = 0,
   PIPE_ERROR = -1,    /**< Generic error */
   PIPE_ERROR_BAD_INPUT = -2,
   PIPE_ERROR_OUT_OF_MEMORY = -3,
   PIPE_ERROR_RETRY = -4
   /* TODO */
};


#define PIPE_BLENDFACTOR_ONE                 0x1
#define PIPE_BLENDFACTOR_SRC_COLOR           0x2
#define PIPE_BLENDFACTOR_SRC_ALPHA           0x3
#define PIPE_BLENDFACTOR_DST_ALPHA           0x4
#define PIPE_BLENDFACTOR_DST_COLOR           0x5
#define PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE  0x6
#define PIPE_BLENDFACTOR_CONST_COLOR         0x7
#define PIPE_BLENDFACTOR_CONST_ALPHA         0x8
#define PIPE_BLENDFACTOR_SRC1_COLOR          0x9
#define PIPE_BLENDFACTOR_SRC1_ALPHA          0x0A
#define PIPE_BLENDFACTOR_ZERO                0x11
#define PIPE_BLENDFACTOR_INV_SRC_COLOR       0x12
#define PIPE_BLENDFACTOR_INV_SRC_ALPHA       0x13
#define PIPE_BLENDFACTOR_INV_DST_ALPHA       0x14
#define PIPE_BLENDFACTOR_INV_DST_COLOR       0x15
#define PIPE_BLENDFACTOR_INV_CONST_COLOR     0x17
#define PIPE_BLENDFACTOR_INV_CONST_ALPHA     0x18
#define PIPE_BLENDFACTOR_INV_SRC1_COLOR      0x19
#define PIPE_BLENDFACTOR_INV_SRC1_ALPHA      0x1A

#define PIPE_BLEND_ADD               0
#define PIPE_BLEND_SUBTRACT          1
#define PIPE_BLEND_REVERSE_SUBTRACT  2
#define PIPE_BLEND_MIN               3
#define PIPE_BLEND_MAX               4

#define PIPE_LOGICOP_CLEAR            0
#define PIPE_LOGICOP_NOR              1
#define PIPE_LOGICOP_AND_INVERTED     2
#define PIPE_LOGICOP_COPY_INVERTED    3
#define PIPE_LOGICOP_AND_REVERSE      4
#define PIPE_LOGICOP_INVERT           5
#define PIPE_LOGICOP_XOR              6
#define PIPE_LOGICOP_NAND             7
#define PIPE_LOGICOP_AND              8
#define PIPE_LOGICOP_EQUIV            9
#define PIPE_LOGICOP_NOOP             10
#define PIPE_LOGICOP_OR_INVERTED      11
#define PIPE_LOGICOP_COPY             12
#define PIPE_LOGICOP_OR_REVERSE       13
#define PIPE_LOGICOP_OR               14
#define PIPE_LOGICOP_SET              15  

#define PIPE_MASK_R  0x1
#define PIPE_MASK_G  0x2
#define PIPE_MASK_B  0x4
#define PIPE_MASK_A  0x8
#define PIPE_MASK_RGBA 0xf


/**
 * Inequality functions.  Used for depth test, stencil compare, alpha
 * test, shadow compare, etc.
 */
#define PIPE_FUNC_NEVER    0
#define PIPE_FUNC_LESS     1
#define PIPE_FUNC_EQUAL    2
#define PIPE_FUNC_LEQUAL   3
#define PIPE_FUNC_GREATER  4
#define PIPE_FUNC_NOTEQUAL 5
#define PIPE_FUNC_GEQUAL   6
#define PIPE_FUNC_ALWAYS   7

/** Polygon fill mode */
#define PIPE_POLYGON_MODE_FILL  0
#define PIPE_POLYGON_MODE_LINE  1
#define PIPE_POLYGON_MODE_POINT 2

/** Polygon face specification, eg for culling */
#define PIPE_FACE_NONE           0
#define PIPE_FACE_FRONT          1
#define PIPE_FACE_BACK           2
#define PIPE_FACE_FRONT_AND_BACK (PIPE_FACE_FRONT | PIPE_FACE_BACK)

/** Stencil ops */
#define PIPE_STENCIL_OP_KEEP       0
#define PIPE_STENCIL_OP_ZERO       1
#define PIPE_STENCIL_OP_REPLACE    2
#define PIPE_STENCIL_OP_INCR       3
#define PIPE_STENCIL_OP_DECR       4
#define PIPE_STENCIL_OP_INCR_WRAP  5
#define PIPE_STENCIL_OP_DECR_WRAP  6
#define PIPE_STENCIL_OP_INVERT     7

/** Texture types.
 * See the documentation for info on PIPE_TEXTURE_RECT vs PIPE_TEXTURE_2D */
enum pipe_texture_target {
   PIPE_BUFFER       = 0,
   PIPE_TEXTURE_1D   = 1,
   PIPE_TEXTURE_2D   = 2,
   PIPE_TEXTURE_3D   = 3,
   PIPE_TEXTURE_CUBE = 4,
   PIPE_TEXTURE_RECT = 5,
   PIPE_MAX_TEXTURE_TYPES
};

#define PIPE_TEX_FACE_POS_X 0
#define PIPE_TEX_FACE_NEG_X 1
#define PIPE_TEX_FACE_POS_Y 2
#define PIPE_TEX_FACE_NEG_Y 3
#define PIPE_TEX_FACE_POS_Z 4
#define PIPE_TEX_FACE_NEG_Z 5
#define PIPE_TEX_FACE_MAX   6

#define PIPE_TEX_WRAP_REPEAT                   0
#define PIPE_TEX_WRAP_CLAMP                    1
#define PIPE_TEX_WRAP_CLAMP_TO_EDGE            2
#define PIPE_TEX_WRAP_CLAMP_TO_BORDER          3
#define PIPE_TEX_WRAP_MIRROR_REPEAT            4
#define PIPE_TEX_WRAP_MIRROR_CLAMP             5
#define PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE     6
#define PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER   7

/* Between mipmaps, ie mipfilter
 */
#define PIPE_TEX_MIPFILTER_NEAREST  0
#define PIPE_TEX_MIPFILTER_LINEAR   1
#define PIPE_TEX_MIPFILTER_NONE     2

/* Within a mipmap, ie min/mag filter 
 */
#define PIPE_TEX_FILTER_NEAREST      0
#define PIPE_TEX_FILTER_LINEAR       1

#define PIPE_TEX_COMPARE_NONE          0
#define PIPE_TEX_COMPARE_R_TO_TEXTURE  1


/**
 * Surface layout -- a hint?  Or some driver-internal poking out into
 * the interface?
 */
#define PIPE_SURFACE_LAYOUT_LINEAR  0


/**
 * Clear buffer bits
 */
/** All color buffers currently bound */
#define PIPE_CLEAR_COLOR        (1 << 0)
#define PIPE_CLEAR_DEPTH        (1 << 1)
#define PIPE_CLEAR_STENCIL      (1 << 2)
/** Depth/stencil combined */
#define PIPE_CLEAR_DEPTHSTENCIL (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)

/**
 * Transfer object usage flags
 */
enum pipe_transfer_usage {
   /**
    * Resource contents read back (or accessed directly) at transfer
    * create time.
    */
   PIPE_TRANSFER_READ = (1 << 0),
   
   /**
    * Resource contents will be written back at transfer_destroy
    * time (or modified as a result of being accessed directly).
    */
   PIPE_TRANSFER_WRITE = (1 << 1),

   /**
    * Read/modify/write
    */
   PIPE_TRANSFER_READ_WRITE = PIPE_TRANSFER_READ | PIPE_TRANSFER_WRITE,

   /** 
    * The transfer should map the texture storage directly. The driver may
    * return NULL if that isn't possible, and the state tracker needs to cope
    * with that and use an alternative path without this flag.
    *
    * E.g. the state tracker could have a simpler path which maps textures and
    * does read/modify/write cycles on them directly, and a more complicated
    * path which uses minimal read and write transfers.
    */
   PIPE_TRANSFER_MAP_DIRECTLY = (1 << 2),

   /**
    * Discards the memory within the mapped region.
    *
    * It should not be used with PIPE_TRANSFER_CPU_READ.
    *
    * See also:
    * - OpenGL's ARB_map_buffer_range extension, MAP_INVALIDATE_RANGE_BIT flag.
    * - Direct3D's D3DLOCK_DISCARD flag.
    */
   PIPE_TRANSFER_DISCARD = (1 << 8),

   /**
    * Fail if the resource cannot be mapped immediately.
    *
    * See also:
    * - Direct3D's D3DLOCK_DONOTWAIT flag.
    * - Mesa3D's MESA_MAP_NOWAIT_BIT flag.
    * - WDDM's D3DDDICB_LOCKFLAGS.DonotWait flag.
    */
   PIPE_TRANSFER_DONTBLOCK = (1 << 9),

   /**
    * Do not attempt to synchronize pending operations on the resource when mapping.
    *
    * It should not be used with PIPE_TRANSFER_CPU_READ.
    *
    * See also:
    * - OpenGL's ARB_map_buffer_range extension, MAP_UNSYNCHRONIZED_BIT flag.
    * - Direct3D's D3DLOCK_NOOVERWRITE flag.
    * - WDDM's D3DDDICB_LOCKFLAGS.IgnoreSync flag.
    */
   PIPE_TRANSFER_UNSYNCHRONIZED = (1 << 10),
   PIPE_TRANSFER_NOOVERWRITE = (1 << 10), /* are these really the same?? */

   /**
    * Written ranges will be notified later with
    * pipe_context::transfer_flush_region.
    *
    * It should not be used with PIPE_TRANSFER_CPU_READ.
    *
    * See also:
    * - pipe_context::transfer_flush_region
    * - OpenGL's ARB_map_buffer_range extension, MAP_FLUSH_EXPLICIT_BIT flag.
    */
   PIPE_TRANSFER_FLUSH_EXPLICIT = (1 << 11)

};


/*
 * Resource binding flags -- state tracker must specify in advance all
 * the ways a resource might be used.
 */
#define PIPE_BIND_DEPTH_STENCIL        (1 << 0) /* get_tex_surface */
#define PIPE_BIND_RENDER_TARGET        (1 << 1) /* get_tex_surface */
#define PIPE_BIND_SAMPLER_VIEW         (1 << 2) /* get_sampler_view */
#define PIPE_BIND_VERTEX_BUFFER        (1 << 3) /* set_vertex_buffers */
#define PIPE_BIND_INDEX_BUFFER         (1 << 4) /* draw_elements */
#define PIPE_BIND_CONSTANT_BUFFER      (1 << 5) /* set_constant_buffer */
#define PIPE_BIND_DISPLAY_TARGET       (1 << 8) /* flush_front_buffer */
#define PIPE_BIND_TRANSFER_WRITE       (1 << 9) /* get_transfer */
#define PIPE_BIND_TRANSFER_READ        (1 << 10) /* get_transfer */
#define PIPE_BIND_STREAM_OUTPUT        (1 << 11) /* set_stream_output_buffers */
#define PIPE_BIND_CUSTOM               (1 << 16) /* state-tracker/winsys usages */

/* The first two flags above were previously part of the amorphous
 * TEXTURE_USAGE, most of which are now descriptions of the ways a
 * particular texture can be bound to the gallium pipeline.  The two flags
 * below do not fit within that and probably need to be migrated to some
 * other place.
 *
 * It seems like scanout is used by the Xorg state tracker to ask for
 * a texture suitable for actual scanout (hence the name), which
 * implies extra layout constraints on some hardware.  It may also
 * have some special meaning regarding mouse cursor images.
 *
 * The shared flag is quite underspecified, but certainly isn't a
 * binding flag - it seems more like a message to the winsys to create
 * a shareable allocation.
 */
#define PIPE_BIND_SCANOUT     (1 << 14) /*  */
#define PIPE_BIND_SHARED      (1 << 15) /* get_texture_handle ??? */


/* Flags for the driver about resource behaviour:
 */
#define PIPE_RESOURCE_FLAG_GEN_MIPS    (1 << 0)  /* Driver performs autogen mips */
#define PIPE_RESOURCE_FLAG_DRV_PRIV    (1 << 16) /* driver/winsys private */
#define PIPE_RESOURCE_FLAG_ST_PRIV     (1 << 24) /* state-tracker/winsys private */

/* Hint about the expected lifecycle of a resource.
 */
#define PIPE_USAGE_DEFAULT        0 /* many uploads, draws intermixed */
#define PIPE_USAGE_DYNAMIC        1 /* many uploads, draws intermixed */
#define PIPE_USAGE_STATIC         2 /* same as immutable?? */
#define PIPE_USAGE_IMMUTABLE      3 /* no change after first upload */
#define PIPE_USAGE_STREAM         4 /* upload, draw, upload, draw */
#define PIPE_USAGE_STAGING        5 /* supports data transfers from the GPU to the CPU */


/* These are intended to be used in calls to is_format_supported, but
 * no driver actually uses these flags, and only the glx/xlib state
 * tracker issues them.
 *
 * Deprecate?
 */
#define PIPE_TEXTURE_GEOM_NON_SQUARE       0x1
#define PIPE_TEXTURE_GEOM_NON_POWER_OF_TWO 0x2


/** 
 * Flush types:
 */
#define PIPE_FLUSH_RENDER_CACHE   0x1
#define PIPE_FLUSH_TEXTURE_CACHE  0x2
#define PIPE_FLUSH_SWAPBUFFERS    0x4
#define PIPE_FLUSH_FRAME          0x8 /**< Mark the end of a frame */


/**
 * Shaders
 */
#define PIPE_SHADER_VERTEX   0
#define PIPE_SHADER_FRAGMENT 1
#define PIPE_SHADER_GEOMETRY 2
#define PIPE_SHADER_TYPES    3


/**
 * Primitive types:
 */
#define PIPE_PRIM_POINTS               0
#define PIPE_PRIM_LINES                1
#define PIPE_PRIM_LINE_LOOP            2
#define PIPE_PRIM_LINE_STRIP           3
#define PIPE_PRIM_TRIANGLES            4
#define PIPE_PRIM_TRIANGLE_STRIP       5
#define PIPE_PRIM_TRIANGLE_FAN         6
#define PIPE_PRIM_QUADS                7
#define PIPE_PRIM_QUAD_STRIP           8
#define PIPE_PRIM_POLYGON              9
#define PIPE_PRIM_LINES_ADJACENCY          10
#define PIPE_PRIM_LINE_STRIP_ADJACENCY    11
#define PIPE_PRIM_TRIANGLES_ADJACENCY      12
#define PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY 13
#define PIPE_PRIM_MAX                      14


/**
 * Query object types
 */
#define PIPE_QUERY_OCCLUSION_COUNTER     0
#define PIPE_QUERY_PRIMITIVES_GENERATED  1
#define PIPE_QUERY_PRIMITIVES_EMITTED    2
#define PIPE_QUERY_TIME_ELAPSED          3
#define PIPE_QUERY_SO_STATISTICS         5
#define PIPE_QUERY_GPU_FINISHED          6
#define PIPE_QUERY_TIMESTAMP_DISJOINT    7
#define PIPE_QUERY_TYPES                 8


/**
 * Conditional rendering modes
 */
#define PIPE_RENDER_COND_WAIT              0
#define PIPE_RENDER_COND_NO_WAIT           1
#define PIPE_RENDER_COND_BY_REGION_WAIT    2
#define PIPE_RENDER_COND_BY_REGION_NO_WAIT 3


/**
 * Point sprite coord modes
 */
#define PIPE_SPRITE_COORD_UPPER_LEFT 0
#define PIPE_SPRITE_COORD_LOWER_LEFT 1


/**
 * Texture swizzles
 */
#define PIPE_SWIZZLE_RED   0
#define PIPE_SWIZZLE_GREEN 1
#define PIPE_SWIZZLE_BLUE  2
#define PIPE_SWIZZLE_ALPHA 3
#define PIPE_SWIZZLE_ZERO  4
#define PIPE_SWIZZLE_ONE   5


/**
 * Implementation capabilities/limits which are queried through
 * pipe_screen::get_param() and pipe_screen::get_paramf().
 */
enum pipe_cap {
   PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS,
   PIPE_CAP_NPOT_TEXTURES,
   PIPE_CAP_TWO_SIDED_STENCIL,
   PIPE_CAP_GLSL,  /* XXX need something better */
   PIPE_CAP_DUAL_SOURCE_BLEND,
   PIPE_CAP_ANISOTROPIC_FILTER,
   PIPE_CAP_POINT_SPRITE,
   PIPE_CAP_MAX_RENDER_TARGETS,
   PIPE_CAP_OCCLUSION_QUERY,
   PIPE_CAP_TIMER_QUERY,
   PIPE_CAP_TEXTURE_SHADOW_MAP,
   PIPE_CAP_TEXTURE_SWIZZLE,
   PIPE_CAP_MAX_TEXTURE_2D_LEVELS,
   PIPE_CAP_MAX_TEXTURE_3D_LEVELS,
   PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS,
   PIPE_CAP_MAX_LINE_WIDTH,
   PIPE_CAP_MAX_LINE_WIDTH_AA,
   PIPE_CAP_MAX_POINT_WIDTH,
   PIPE_CAP_MAX_POINT_WIDTH_AA,
   PIPE_CAP_MAX_TEXTURE_ANISOTROPY,
   PIPE_CAP_MAX_TEXTURE_LOD_BIAS,
   PIPE_CAP_GUARD_BAND_LEFT,  /*< float */
   PIPE_CAP_GUARD_BAND_TOP,  /*< float */
   PIPE_CAP_GUARD_BAND_RIGHT,  /*< float */
   PIPE_CAP_GUARD_BAND_BOTTOM,  /*< float */
   PIPE_CAP_TEXTURE_MIRROR_CLAMP,
   PIPE_CAP_TEXTURE_MIRROR_REPEAT,
   PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS,
   PIPE_CAP_BLEND_EQUATION_SEPARATE,
   PIPE_CAP_SM3,  /*< Shader Model, supported */
   PIPE_CAP_STREAM_OUTPUT,
   /** Maximum texture image units accessible from vertex and fragment shaders
    * combined */
   PIPE_CAP_MAX_COMBINED_SAMPLERS,
   /** blend enables and write masks per rendertarget */
   PIPE_CAP_INDEP_BLEND_ENABLE,
   /** different blend funcs per rendertarget */
   PIPE_CAP_INDEP_BLEND_FUNC,
   PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE,
   PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT,
   PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT,
   PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER,
   PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER,
   PIPE_CAP_DEPTH_CLAMP
};

/* Shader caps not specific to any single stage */
enum pipe_shader_cap
{
   PIPE_SHADER_CAP_MAX_INSTRUCTIONS, /* if 0, it means the stage is unsupported */
   PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS,
   PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS,
   PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS,
   PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH,
   PIPE_SHADER_CAP_MAX_INPUTS,
   PIPE_SHADER_CAP_MAX_CONSTS,
   PIPE_SHADER_CAP_MAX_CONST_BUFFERS,
   PIPE_SHADER_CAP_MAX_TEMPS,
   PIPE_SHADER_CAP_MAX_ADDRS,
   PIPE_SHADER_CAP_MAX_PREDS,
   PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED,
};

/**
 * Referenced query flags.
 */

#define PIPE_UNREFERENCED         0
#define PIPE_REFERENCED_FOR_READ  (1 << 0)
#define PIPE_REFERENCED_FOR_WRITE (1 << 1)

/**
 * Composite query types
 */
struct pipe_query_data_so_statistics
{
   uint64_t num_primitives_written;
   uint64_t primitives_storage_needed;
};
struct pipe_query_data_timestamp_disjoint
{
   uint64_t frequency;
   boolean  disjoint;
};

#ifdef __cplusplus
}
#endif

#endif
