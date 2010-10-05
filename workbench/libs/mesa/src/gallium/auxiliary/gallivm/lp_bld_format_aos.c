/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * AoS pixel format manipulation.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_string.h"

#include "lp_bld_arit.h"
#include "lp_bld_init.h"
#include "lp_bld_type.h"
#include "lp_bld_flow.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_gather.h"
#include "lp_bld_debug.h"
#include "lp_bld_format.h"


/**
 * Basic swizzling.  Rearrange the order of the unswizzled array elements
 * according to the format description.  PIPE_SWIZZLE_ZERO/ONE are supported
 * too.
 * Ex: if unswizzled[4] = {B, G, R, x}, then swizzled_out[4] = {R, G, B, 1}.
 */
LLVMValueRef
lp_build_format_swizzle_aos(const struct util_format_description *desc,
                            struct lp_build_context *bld,
                            LLVMValueRef unswizzled)
{
   unsigned char swizzles[4];
   unsigned chan;

   assert(bld->type.length % 4 == 0);

   for (chan = 0; chan < 4; ++chan) {
      enum util_format_swizzle swizzle;

      if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
         /*
          * For ZS formats do RGBA = ZZZ1
          */
         if (chan == 3) {
            swizzle = UTIL_FORMAT_SWIZZLE_1;
         } else if (desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_NONE) {
            swizzle = UTIL_FORMAT_SWIZZLE_0;
         } else {
            swizzle = desc->swizzle[0];
         }
      } else {
         swizzle = desc->swizzle[chan];
      }
      swizzles[chan] = swizzle;
   }

   return lp_build_swizzle_aos(bld, unswizzled, swizzles);
}


/**
 * Whether the format matches the vector type, apart of swizzles.
 */
static INLINE boolean
format_matches_type(const struct util_format_description *desc,
                    struct lp_type type)
{
   enum util_format_type chan_type;
   unsigned chan;

   assert(type.length % 4 == 0);

   if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN ||
       desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB ||
       desc->block.width != 1 ||
       desc->block.height != 1) {
      return FALSE;
   }

   if (type.floating) {
      chan_type = UTIL_FORMAT_TYPE_FLOAT;
   } else if (type.fixed) {
      chan_type = UTIL_FORMAT_TYPE_FIXED;
   } else if (type.sign) {
      chan_type = UTIL_FORMAT_TYPE_SIGNED;
   } else {
      chan_type = UTIL_FORMAT_TYPE_UNSIGNED;
   }

   for (chan = 0; chan < desc->nr_channels; ++chan) {
      if (desc->channel[chan].size != type.width) {
         return FALSE;
      }

      if (desc->channel[chan].type != UTIL_FORMAT_TYPE_VOID) {
         if (desc->channel[chan].type != chan_type ||
             desc->channel[chan].normalized != type.norm) {
            return FALSE;
         }
      }
   }

   return TRUE;
}


/**
 * Unpack a single pixel into its RGBA components.
 *
 * @param desc  the pixel format for the packed pixel value
 * @param packed integer pixel in a format such as PIPE_FORMAT_B8G8R8A8_UNORM
 *
 * @return RGBA in a float[4] or ubyte[4] or ushort[4] vector.
 */
static INLINE LLVMValueRef
lp_build_unpack_arith_rgba_aos(LLVMBuilderRef builder,
                               const struct util_format_description *desc,
                               LLVMValueRef packed)
{
   LLVMValueRef shifted, casted, scaled, masked;
   LLVMValueRef shifts[4];
   LLVMValueRef masks[4];
   LLVMValueRef scales[4];

   boolean normalized;
   boolean needs_uitofp;
   unsigned shift;
   unsigned i;

   /* TODO: Support more formats */
   assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);
   assert(desc->block.bits <= 32);

   /* Do the intermediate integer computations with 32bit integers since it
    * matches floating point size */
   assert (LLVMTypeOf(packed) == LLVMInt32Type());

   /* Broadcast the packed value to all four channels
    * before: packed = BGRA
    * after: packed = {BGRA, BGRA, BGRA, BGRA}
    */
   packed = LLVMBuildInsertElement(builder,
                                   LLVMGetUndef(LLVMVectorType(LLVMInt32Type(), 4)),
                                   packed,
                                   LLVMConstNull(LLVMInt32Type()),
                                   "");
   packed = LLVMBuildShuffleVector(builder,
                                   packed,
                                   LLVMGetUndef(LLVMVectorType(LLVMInt32Type(), 4)),
                                   LLVMConstNull(LLVMVectorType(LLVMInt32Type(), 4)),
                                   "");

   /* Initialize vector constants */
   normalized = FALSE;
   needs_uitofp = FALSE;
   shift = 0;

   /* Loop over 4 color components */
   for (i = 0; i < 4; ++i) {
      unsigned bits = desc->channel[i].size;

      if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID) {
         shifts[i] = LLVMGetUndef(LLVMInt32Type());
         masks[i] = LLVMConstNull(LLVMInt32Type());
         scales[i] =  LLVMConstNull(LLVMFloatType());
      }
      else {
         unsigned long long mask = (1ULL << bits) - 1;

         assert(desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED);

         if (bits == 32) {
            needs_uitofp = TRUE;
         }

         shifts[i] = LLVMConstInt(LLVMInt32Type(), shift, 0);
         masks[i] = LLVMConstInt(LLVMInt32Type(), mask, 0);

         if (desc->channel[i].normalized) {
            scales[i] = LLVMConstReal(LLVMFloatType(), 1.0/mask);
            normalized = TRUE;
         }
         else
            scales[i] =  LLVMConstReal(LLVMFloatType(), 1.0);
      }

      shift += bits;
   }

   /* Ex: convert packed = {BGRA, BGRA, BGRA, BGRA}
    * into masked = {B, G, R, A}
    */
   shifted = LLVMBuildLShr(builder, packed, LLVMConstVector(shifts, 4), "");
   masked = LLVMBuildAnd(builder, shifted, LLVMConstVector(masks, 4), "");


   if (!needs_uitofp) {
      /* UIToFP can't be expressed in SSE2 */
      casted = LLVMBuildSIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");
   } else {
      casted = LLVMBuildUIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");
   }

   /* At this point 'casted' may be a vector of floats such as
    * {255.0, 255.0, 255.0, 255.0}.  Next, if the pixel values are normalized
    * we'll scale this to {1.0, 1.0, 1.0, 1.0}.
    */

   if (normalized)
      scaled = LLVMBuildFMul(builder, casted, LLVMConstVector(scales, 4), "");
   else
      scaled = casted;

   return scaled;
}


/**
 * Pack a single pixel.
 *
 * @param rgba 4 float vector with the unpacked components.
 *
 * XXX: This is mostly for reference and testing -- operating a single pixel at
 * a time is rarely if ever needed.
 */
LLVMValueRef
lp_build_pack_rgba_aos(LLVMBuilderRef builder,
                       const struct util_format_description *desc,
                       LLVMValueRef rgba)
{
   LLVMTypeRef type;
   LLVMValueRef packed = NULL;
   LLVMValueRef swizzles[4];
   LLVMValueRef shifted, casted, scaled, unswizzled;
   LLVMValueRef shifts[4];
   LLVMValueRef scales[4];
   boolean normalized;
   unsigned shift;
   unsigned i, j;

   assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);

   type = LLVMIntType(desc->block.bits);

   /* Unswizzle the color components into the source vector. */
   for (i = 0; i < 4; ++i) {
      for (j = 0; j < 4; ++j) {
         if (desc->swizzle[j] == i)
            break;
      }
      if (j < 4)
         swizzles[i] = LLVMConstInt(LLVMInt32Type(), j, 0);
      else
         swizzles[i] = LLVMGetUndef(LLVMInt32Type());
   }

   unswizzled = LLVMBuildShuffleVector(builder, rgba,
                                       LLVMGetUndef(LLVMVectorType(LLVMFloatType(), 4)),
                                       LLVMConstVector(swizzles, 4), "");

   normalized = FALSE;
   shift = 0;
   for (i = 0; i < 4; ++i) {
      unsigned bits = desc->channel[i].size;

      if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID) {
         shifts[i] = LLVMGetUndef(LLVMInt32Type());
         scales[i] =  LLVMGetUndef(LLVMFloatType());
      }
      else {
         unsigned mask = (1 << bits) - 1;

         assert(desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED);
         assert(bits < 32);

         shifts[i] = LLVMConstInt(LLVMInt32Type(), shift, 0);

         if (desc->channel[i].normalized) {
            scales[i] = LLVMConstReal(LLVMFloatType(), mask);
            normalized = TRUE;
         }
         else
            scales[i] =  LLVMConstReal(LLVMFloatType(), 1.0);
      }

      shift += bits;
   }

   if (normalized)
      scaled = LLVMBuildFMul(builder, unswizzled, LLVMConstVector(scales, 4), "");
   else
      scaled = unswizzled;

   casted = LLVMBuildFPToSI(builder, scaled, LLVMVectorType(LLVMInt32Type(), 4), "");

   shifted = LLVMBuildShl(builder, casted, LLVMConstVector(shifts, 4), "");
   
   /* Bitwise or all components */
   for (i = 0; i < 4; ++i) {
      if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
         LLVMValueRef component = LLVMBuildExtractElement(builder, shifted, LLVMConstInt(LLVMInt32Type(), i, 0), "");
         if (packed)
            packed = LLVMBuildOr(builder, packed, component, "");
         else
            packed = component;
      }
   }

   if (!packed)
      packed = LLVMGetUndef(LLVMInt32Type());

   if (desc->block.bits < 32)
      packed = LLVMBuildTrunc(builder, packed, type, "");

   return packed;
}




/**
 * Fetch a pixel into a 4 float AoS.
 *
 * \param format_desc  describes format of the image we're fetching from
 * \param ptr  address of the pixel block (or the texel if uncompressed)
 * \param i, j  the sub-block pixel coordinates.  For non-compressed formats
 *              these will always be (0, 0).
 * \return  a 4 element vector with the pixel's RGBA values.
 */
LLVMValueRef
lp_build_fetch_rgba_aos(LLVMBuilderRef builder,
                        const struct util_format_description *format_desc,
                        struct lp_type type,
                        LLVMValueRef base_ptr,
                        LLVMValueRef offset,
                        LLVMValueRef i,
                        LLVMValueRef j)
{
   unsigned num_pixels = type.length / 4;
   struct lp_build_context bld;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);
   assert(type.length % 4 == 0);

   lp_build_context_init(&bld, builder, type);

   /*
    * Trivial case
    *
    * The format matches the type (apart of a swizzle) so no need for
    * scaling or converting.
    */

   if (format_matches_type(format_desc, type) &&
       format_desc->block.bits <= type.width * 4 &&
       util_is_power_of_two(format_desc->block.bits)) {
      LLVMValueRef packed;

      /*
       * The format matches the type (apart of a swizzle) so no need for
       * scaling or converting.
       */

      packed = lp_build_gather(builder, type.length/4,
                               format_desc->block.bits, type.width*4,
                               base_ptr, offset);

      assert(format_desc->block.bits <= type.width * type.length);

      packed = LLVMBuildBitCast(builder, packed, lp_build_vec_type(type), "");

      return lp_build_format_swizzle_aos(format_desc, &bld, packed);
   }

   /*
    * Bit arithmetic
    */

   if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
       (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) &&
       format_desc->block.width == 1 &&
       format_desc->block.height == 1 &&
       util_is_power_of_two(format_desc->block.bits) &&
       format_desc->block.bits <= 32 &&
       format_desc->is_bitmask &&
       !format_desc->is_mixed &&
       (format_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED ||
        format_desc->channel[1].type == UTIL_FORMAT_TYPE_UNSIGNED)) {

      LLVMValueRef tmps[LP_MAX_VECTOR_LENGTH/4];
      LLVMValueRef res;
      unsigned k;

      /*
       * Unpack a pixel at a time into a <4 x float> RGBA vector
       */

      for (k = 0; k < num_pixels; ++k) {
         LLVMValueRef packed;

         packed = lp_build_gather_elem(builder, num_pixels,
                                       format_desc->block.bits, 32,
                                       base_ptr, offset, k);

         tmps[k] = lp_build_unpack_arith_rgba_aos(builder, format_desc,
                                                  packed);
      }

      /*
       * Type conversion.
       *
       * TODO: We could avoid floating conversion for integer to
       * integer conversions.
       */

      if (gallivm_debug & GALLIVM_DEBUG_PERF && !type.floating) {
         debug_printf("%s: unpacking %s with floating point\n",
                      __FUNCTION__, format_desc->short_name);
      }

      lp_build_conv(builder,
                    lp_float32_vec4_type(),
                    type,
                    tmps, num_pixels, &res, 1);

      return lp_build_format_swizzle_aos(format_desc, &bld, res);
   }

   /*
    * YUV / subsampled formats
    */

   if (format_desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      struct lp_type tmp_type;
      LLVMValueRef tmp;

      memset(&tmp_type, 0, sizeof tmp_type);
      tmp_type.width = 8;
      tmp_type.length = num_pixels * 4;
      tmp_type.norm = TRUE;

      tmp = lp_build_fetch_subsampled_rgba_aos(builder,
                                               format_desc,
                                               num_pixels,
                                               base_ptr,
                                               offset,
                                               i, j);

      lp_build_conv(builder,
                    tmp_type, type,
                    &tmp, 1, &tmp, 1);

      return tmp;
   }

   /*
    * Fallback to util_format_description::fetch_rgba_8unorm().
    */

   if (format_desc->fetch_rgba_8unorm &&
       !type.floating && type.width == 8 && !type.sign && type.norm) {
      /*
       * Fallback to calling util_format_description::fetch_rgba_8unorm.
       *
       * This is definitely not the most efficient way of fetching pixels, as
       * we miss the opportunity to do vectorization, but this it is a
       * convenient for formats or scenarios for which there was no opportunity
       * or incentive to optimize.
       */

      LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
      char name[256];
      LLVMTypeRef i8t = LLVMInt8Type();
      LLVMTypeRef pi8t = LLVMPointerType(i8t, 0);
      LLVMTypeRef i32t = LLVMInt32Type();
      LLVMValueRef function;
      LLVMValueRef tmp_ptr;
      LLVMValueRef tmp;
      LLVMValueRef res;
      unsigned k;

      util_snprintf(name, sizeof name, "util_format_%s_fetch_rgba_8unorm",
                    format_desc->short_name);

      if (gallivm_debug & GALLIVM_DEBUG_PERF) {
         debug_printf("%s: falling back to %s\n", __FUNCTION__, name);
      }

      /*
       * Declare and bind format_desc->fetch_rgba_8unorm().
       */

      function = LLVMGetNamedFunction(module, name);
      if (!function) {
         LLVMTypeRef ret_type;
         LLVMTypeRef arg_types[4];
         LLVMTypeRef function_type;

         ret_type = LLVMVoidType();
         arg_types[0] = pi8t;
         arg_types[1] = pi8t;
         arg_types[3] = arg_types[2] = LLVMIntType(sizeof(unsigned) * 8);
         function_type = LLVMFunctionType(ret_type, arg_types, Elements(arg_types), 0);
         function = LLVMAddFunction(module, name, function_type);

         LLVMSetFunctionCallConv(function, LLVMCCallConv);
         LLVMSetLinkage(function, LLVMExternalLinkage);

         assert(LLVMIsDeclaration(function));

         LLVMAddGlobalMapping(lp_build_engine, function,
                              func_to_pointer((func_pointer)format_desc->fetch_rgba_8unorm));
      }

      tmp_ptr = lp_build_alloca(builder, i32t, "");

      res = LLVMGetUndef(LLVMVectorType(i32t, num_pixels));

      /*
       * Invoke format_desc->fetch_rgba_8unorm() for each pixel and insert the result
       * in the SoA vectors.
       */

      for (k = 0; k < num_pixels; ++k) {
         LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), k, 0);
         LLVMValueRef args[4];

         args[0] = LLVMBuildBitCast(builder, tmp_ptr, pi8t, "");
         args[1] = lp_build_gather_elem_ptr(builder, num_pixels,
                                            base_ptr, offset, k);

         if (num_pixels == 1) {
            args[2] = i;
            args[3] = j;
         }
         else {
            args[2] = LLVMBuildExtractElement(builder, i, index, "");
            args[3] = LLVMBuildExtractElement(builder, j, index, "");
         }

         LLVMBuildCall(builder, function, args, Elements(args), "");

         tmp = LLVMBuildLoad(builder, tmp_ptr, "");

         if (num_pixels == 1) {
            res = tmp;
         }
         else {
            res = LLVMBuildInsertElement(builder, res, tmp, index, "");
         }
      }

      /* Bitcast from <n x i32> to <4n x i8> */
      res = LLVMBuildBitCast(builder, res, bld.vec_type, "");

      return res;
   }


   /*
    * Fallback to util_format_description::fetch_rgba_float().
    */

   if (format_desc->fetch_rgba_float) {
      /*
       * Fallback to calling util_format_description::fetch_rgba_float.
       *
       * This is definitely not the most efficient way of fetching pixels, as
       * we miss the opportunity to do vectorization, but this it is a
       * convenient for formats or scenarios for which there was no opportunity
       * or incentive to optimize.
       */

      LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
      char name[256];
      LLVMTypeRef f32t = LLVMFloatType();
      LLVMTypeRef f32x4t = LLVMVectorType(f32t, 4);
      LLVMTypeRef pf32t = LLVMPointerType(f32t, 0);
      LLVMValueRef function;
      LLVMValueRef tmp_ptr;
      LLVMValueRef tmps[LP_MAX_VECTOR_LENGTH/4];
      LLVMValueRef res;
      unsigned k;

      util_snprintf(name, sizeof name, "util_format_%s_fetch_rgba_float",
                    format_desc->short_name);

      if (gallivm_debug & GALLIVM_DEBUG_PERF) {
         debug_printf("%s: falling back to %s\n", __FUNCTION__, name);
      }

      /*
       * Declare and bind format_desc->fetch_rgba_float().
       */

      function = LLVMGetNamedFunction(module, name);
      if (!function) {
         LLVMTypeRef ret_type;
         LLVMTypeRef arg_types[4];
         LLVMTypeRef function_type;

         ret_type = LLVMVoidType();
         arg_types[0] = pf32t;
         arg_types[1] = LLVMPointerType(LLVMInt8Type(), 0);
         arg_types[3] = arg_types[2] = LLVMIntType(sizeof(unsigned) * 8);
         function_type = LLVMFunctionType(ret_type, arg_types, Elements(arg_types), 0);
         function = LLVMAddFunction(module, name, function_type);

         LLVMSetFunctionCallConv(function, LLVMCCallConv);
         LLVMSetLinkage(function, LLVMExternalLinkage);

         assert(LLVMIsDeclaration(function));

         LLVMAddGlobalMapping(lp_build_engine, function,
                              func_to_pointer((func_pointer)format_desc->fetch_rgba_float));
      }

      tmp_ptr = lp_build_alloca(builder, f32x4t, "");

      /*
       * Invoke format_desc->fetch_rgba_float() for each pixel and insert the result
       * in the SoA vectors.
       */

      for (k = 0; k < num_pixels; ++k) {
         LLVMValueRef args[4];

         args[0] = LLVMBuildBitCast(builder, tmp_ptr, pf32t, "");
         args[1] = lp_build_gather_elem_ptr(builder, num_pixels,
                                            base_ptr, offset, k);

         if (num_pixels == 1) {
            args[2] = i;
            args[3] = j;
         }
         else {
            LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), k, 0);
            args[2] = LLVMBuildExtractElement(builder, i, index, "");
            args[3] = LLVMBuildExtractElement(builder, j, index, "");
         }

         LLVMBuildCall(builder, function, args, Elements(args), "");

         tmps[k] = LLVMBuildLoad(builder, tmp_ptr, "");
      }

      lp_build_conv(builder,
                    lp_float32_vec4_type(),
                    type,
                    tmps, num_pixels, &res, 1);

      return res;
   }

   assert(0);
   return lp_build_undef(type);
}
