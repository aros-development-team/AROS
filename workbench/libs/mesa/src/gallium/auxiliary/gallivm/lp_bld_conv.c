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
 * Helper functions for type conversions.
 *
 * We want to use the fastest type for a given computation whenever feasible.
 * The other side of this is that we need to be able convert between several
 * types accurately and efficiently.
 *
 * Conversion between types of different bit width is quite complex since a 
 *
 * To remember there are a few invariants in type conversions:
 *
 * - register width must remain constant:
 *
 *     src_type.width * src_type.length == dst_type.width * dst_type.length
 *
 * - total number of elements must remain constant:
 *
 *     src_type.length * num_srcs == dst_type.length * num_dsts
 *
 * It is not always possible to do the conversion both accurately and
 * efficiently, usually due to lack of adequate machine instructions. In these
 * cases it is important not to cut shortcuts here and sacrifice accuracy, as
 * there this functions can be used anywhere. In the future we might have a
 * precision parameter which can gauge the accuracy vs efficiency compromise,
 * but for now if the data conversion between two stages happens to be the
 * bottleneck, then most likely should just avoid converting at all and run
 * both stages with the same type.
 *
 * Make sure to run lp_test_conv unit test after any change to this file.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_cpu_detect.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_pack.h"
#include "lp_bld_conv.h"


/**
 * Special case for converting clamped IEEE-754 floats to unsigned norms.
 *
 * The mathematical voodoo below may seem excessive but it is actually
 * paramount we do it this way for several reasons. First, there is no single
 * precision FP to unsigned integer conversion Intel SSE instruction. Second,
 * secondly, even if there was, since the FP's mantissa takes only a fraction
 * of register bits the typically scale and cast approach would require double
 * precision for accurate results, and therefore half the throughput
 *
 * Although the result values can be scaled to an arbitrary bit width specified
 * by dst_width, the actual result type will have the same width.
 *
 * Ex: src = { float, float, float, float }
 * return { i32, i32, i32, i32 } where each value is in [0, 2^dst_width-1].
 */
LLVMValueRef
lp_build_clamped_float_to_unsigned_norm(struct gallivm_state *gallivm,
                                        struct lp_type src_type,
                                        unsigned dst_width,
                                        LLVMValueRef src)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(gallivm, src_type);
   LLVMValueRef res;
   unsigned mantissa;

   assert(src_type.floating);
   assert(dst_width <= src_type.width);
   src_type.sign = FALSE;

   mantissa = lp_mantissa(src_type);

   if (dst_width <= mantissa) {
      /*
       * Apply magic coefficients that will make the desired result to appear
       * in the lowest significant bits of the mantissa, with correct rounding.
       *
       * This only works if the destination width fits in the mantissa.
       */

      unsigned long long ubound;
      unsigned long long mask;
      double scale;
      double bias;

      ubound = (1ULL << dst_width);
      mask = ubound - 1;
      scale = (double)mask/ubound;
      bias = (double)(1ULL << (mantissa - dst_width));

      res = LLVMBuildFMul(builder, src, lp_build_const_vec(gallivm, src_type, scale), "");
      res = LLVMBuildFAdd(builder, res, lp_build_const_vec(gallivm, src_type, bias), "");
      res = LLVMBuildBitCast(builder, res, int_vec_type, "");
      res = LLVMBuildAnd(builder, res,
                         lp_build_const_int_vec(gallivm, src_type, mask), "");
   }
   else if (dst_width == (mantissa + 1)) {
      /*
       * The destination width matches exactly what can be represented in
       * floating point (i.e., mantissa + 1 bits). So do a straight
       * multiplication followed by casting. No further rounding is necessary.
       */

      double scale;

      scale = (double)((1ULL << dst_width) - 1);

      res = LLVMBuildFMul(builder, src,
                          lp_build_const_vec(gallivm, src_type, scale), "");
      res = LLVMBuildFPToSI(builder, res, int_vec_type, "");
   }
   else {
      /*
       * The destination exceeds what can be represented in the floating point.
       * So multiply by the largest power two we get away with, and when
       * subtract the most significant bit to rescale to normalized values.
       *
       * The largest power of two factor we can get away is
       * (1 << (src_type.width - 1)), because we need to use signed . In theory it
       * should be (1 << (src_type.width - 2)), but IEEE 754 rules states
       * INT_MIN should be returned in FPToSI, which is the correct result for
       * values near 1.0!
       *
       * This means we get (src_type.width - 1) correct bits for values near 0.0,
       * and (mantissa + 1) correct bits for values near 1.0. Equally or more
       * important, we also get exact results for 0.0 and 1.0.
       */

      unsigned n = MIN2(src_type.width - 1, dst_width);

      double scale = (double)(1ULL << n);
      unsigned lshift = dst_width - n;
      unsigned rshift = n;
      LLVMValueRef lshifted;
      LLVMValueRef rshifted;

      res = LLVMBuildFMul(builder, src,
                          lp_build_const_vec(gallivm, src_type, scale), "");
      res = LLVMBuildFPToSI(builder, res, int_vec_type, "");

      /*
       * Align the most significant bit to its final place.
       *
       * This will cause 1.0 to overflow to 0, but the later adjustment will
       * get it right.
       */
      if (lshift) {
         lshifted = LLVMBuildShl(builder, res,
                                 lp_build_const_int_vec(gallivm, src_type,
                                                        lshift), "");
      } else {
         lshifted = res;
      }

      /*
       * Align the most significant bit to the right.
       */
      rshifted =  LLVMBuildAShr(builder, res,
                                lp_build_const_int_vec(gallivm, src_type, rshift),
                                "");

      /*
       * Subtract the MSB to the LSB, therefore re-scaling from
       * (1 << dst_width) to ((1 << dst_width) - 1).
       */

      res = LLVMBuildSub(builder, lshifted, rshifted, "");
   }

   return res;
}


/**
 * Inverse of lp_build_clamped_float_to_unsigned_norm above.
 * Ex: src = { i32, i32, i32, i32 } with values in range [0, 2^src_width-1]
 * return {float, float, float, float} with values in range [0, 1].
 */
LLVMValueRef
lp_build_unsigned_norm_to_float(struct gallivm_state *gallivm,
                                unsigned src_width,
                                struct lp_type dst_type,
                                LLVMValueRef src)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef vec_type = lp_build_vec_type(gallivm, dst_type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(gallivm, dst_type);
   LLVMValueRef bias_;
   LLVMValueRef res;
   unsigned mantissa;
   unsigned n;
   unsigned long long ubound;
   unsigned long long mask;
   double scale;
   double bias;

   assert(dst_type.floating);

   mantissa = lp_mantissa(dst_type);

   if (src_width <= (mantissa + 1)) {
      /*
       * The source width matches fits what can be represented in floating
       * point (i.e., mantissa + 1 bits). So do a straight multiplication
       * followed by casting. No further rounding is necessary.
       */

      scale = 1.0/(double)((1ULL << src_width) - 1);
      res = LLVMBuildSIToFP(builder, src, vec_type, "");
      res = LLVMBuildFMul(builder, res,
                          lp_build_const_vec(gallivm, dst_type, scale), "");
      return res;
   }
   else {
      /*
       * The source width exceeds what can be represented in floating
       * point. So truncate the incoming values.
       */

      n = MIN2(mantissa, src_width);

      ubound = ((unsigned long long)1 << n);
      mask = ubound - 1;
      scale = (double)ubound/mask;
      bias = (double)((unsigned long long)1 << (mantissa - n));

      res = src;

      if (src_width > mantissa) {
         int shift = src_width - mantissa;
         res = LLVMBuildLShr(builder, res,
                             lp_build_const_int_vec(gallivm, dst_type, shift), "");
      }

      bias_ = lp_build_const_vec(gallivm, dst_type, bias);

      res = LLVMBuildOr(builder,
                        res,
                        LLVMBuildBitCast(builder, bias_, int_vec_type, ""), "");

      res = LLVMBuildBitCast(builder, res, vec_type, "");

      res = LLVMBuildFSub(builder, res, bias_, "");
      res = LLVMBuildFMul(builder, res, lp_build_const_vec(gallivm, dst_type, scale), "");
   }

   return res;
}


/**
 * Generic type conversion.
 *
 * TODO: Take a precision argument, or even better, add a new precision member
 * to the lp_type union.
 */
void
lp_build_conv(struct gallivm_state *gallivm,
              struct lp_type src_type,
              struct lp_type dst_type,
              const LLVMValueRef *src, unsigned num_srcs,
              LLVMValueRef *dst, unsigned num_dsts)
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_type tmp_type;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned num_tmps;
   unsigned i;

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   assert(src_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(dst_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(num_srcs <= LP_MAX_VECTOR_LENGTH);
   assert(num_dsts <= LP_MAX_VECTOR_LENGTH);

   tmp_type = src_type;
   for(i = 0; i < num_srcs; ++i) {
      assert(lp_check_value(src_type, src[i]));
      tmp[i] = src[i];
   }
   num_tmps = num_srcs;


   /* Special case 4x4f --> 1x16ub 
    */
   if (src_type.floating == 1 &&
       src_type.fixed    == 0 &&
       src_type.sign     == 1 &&
       src_type.norm     == 0 &&
       src_type.width    == 32 &&
       src_type.length   == 4 &&

       dst_type.floating == 0 &&
       dst_type.fixed    == 0 &&
       dst_type.sign     == 0 &&
       dst_type.norm     == 1 &&
       dst_type.width    == 8 &&
       dst_type.length   == 16 &&

       util_cpu_caps.has_sse2)
   {
      int i;

      for (i = 0; i < num_dsts; i++, src += 4) {
         struct lp_type int16_type = dst_type;
         struct lp_type int32_type = dst_type;
         LLVMValueRef lo, hi;
         LLVMValueRef src_int0;
         LLVMValueRef src_int1;
         LLVMValueRef src_int2;
         LLVMValueRef src_int3;
         LLVMTypeRef int16_vec_type;
         LLVMTypeRef int32_vec_type;
         LLVMTypeRef src_vec_type;
         LLVMTypeRef dst_vec_type;
         LLVMValueRef const_255f;
         LLVMValueRef a, b, c, d;

         int16_type.width *= 2;
         int16_type.length /= 2;
         int16_type.sign = 1;

         int32_type.width *= 4;
         int32_type.length /= 4;
         int32_type.sign = 1;

         src_vec_type   = lp_build_vec_type(gallivm, src_type);
         dst_vec_type   = lp_build_vec_type(gallivm, dst_type);
         int16_vec_type = lp_build_vec_type(gallivm, int16_type);
         int32_vec_type = lp_build_vec_type(gallivm, int32_type);

         const_255f = lp_build_const_vec(gallivm, src_type, 255.0f);

         a = LLVMBuildFMul(builder, src[0], const_255f, "");
         b = LLVMBuildFMul(builder, src[1], const_255f, "");
         c = LLVMBuildFMul(builder, src[2], const_255f, "");
         d = LLVMBuildFMul(builder, src[3], const_255f, "");

         {
            struct lp_build_context bld;

            bld.gallivm = gallivm;
            bld.type = src_type;
            bld.vec_type = src_vec_type;
            bld.int_elem_type = lp_build_elem_type(gallivm, int32_type);
            bld.int_vec_type = int32_vec_type;
            bld.undef = lp_build_undef(gallivm, src_type);
            bld.zero = lp_build_zero(gallivm, src_type);
            bld.one = lp_build_one(gallivm, src_type);

            src_int0 = lp_build_iround(&bld, a);
            src_int1 = lp_build_iround(&bld, b);
            src_int2 = lp_build_iround(&bld, c);
            src_int3 = lp_build_iround(&bld, d);
         }
         /* relying on clamping behavior of sse2 intrinsics here */
         lo = lp_build_pack2(gallivm, int32_type, int16_type, src_int0, src_int1);
         hi = lp_build_pack2(gallivm, int32_type, int16_type, src_int2, src_int3);
         dst[i] = lp_build_pack2(gallivm, int16_type, dst_type, lo, hi);
      }
      return; 
   }

   /*
    * Clamp if necessary
    */

   if(memcmp(&src_type, &dst_type, sizeof src_type) != 0) {
      struct lp_build_context bld;
      double src_min = lp_const_min(src_type);
      double dst_min = lp_const_min(dst_type);
      double src_max = lp_const_max(src_type);
      double dst_max = lp_const_max(dst_type);
      LLVMValueRef thres;

      lp_build_context_init(&bld, gallivm, tmp_type);

      if(src_min < dst_min) {
         if(dst_min == 0.0)
            thres = bld.zero;
         else
            thres = lp_build_const_vec(gallivm, src_type, dst_min);
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_max(&bld, tmp[i], thres);
      }

      if(src_max > dst_max) {
         if(dst_max == 1.0)
            thres = bld.one;
         else
            thres = lp_build_const_vec(gallivm, src_type, dst_max);
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_min(&bld, tmp[i], thres);
      }
   }

   /*
    * Scale to the narrowest range
    */

   if(dst_type.floating) {
      /* Nothing to do */
   }
   else if(tmp_type.floating) {
      if(!dst_type.fixed && !dst_type.sign && dst_type.norm) {
         for(i = 0; i < num_tmps; ++i) {
            tmp[i] = lp_build_clamped_float_to_unsigned_norm(gallivm,
                                                             tmp_type,
                                                             dst_type.width,
                                                             tmp[i]);
         }
         tmp_type.floating = FALSE;
      }
      else {
         double dst_scale = lp_const_scale(dst_type);
         LLVMTypeRef tmp_vec_type;

         if (dst_scale != 1.0) {
            LLVMValueRef scale = lp_build_const_vec(gallivm, tmp_type, dst_scale);
            for(i = 0; i < num_tmps; ++i)
               tmp[i] = LLVMBuildFMul(builder, tmp[i], scale, "");
         }

         /* Use an equally sized integer for intermediate computations */
         tmp_type.floating = FALSE;
         tmp_vec_type = lp_build_vec_type(gallivm, tmp_type);
         for(i = 0; i < num_tmps; ++i) {
#if 0
            if(dst_type.sign)
               tmp[i] = LLVMBuildFPToSI(builder, tmp[i], tmp_vec_type, "");
            else
               tmp[i] = LLVMBuildFPToUI(builder, tmp[i], tmp_vec_type, "");
#else
           /* FIXME: there is no SSE counterpart for LLVMBuildFPToUI */
            tmp[i] = LLVMBuildFPToSI(builder, tmp[i], tmp_vec_type, "");
#endif
         }
      }
   }
   else {
      unsigned src_shift = lp_const_shift(src_type);
      unsigned dst_shift = lp_const_shift(dst_type);

      /* FIXME: compensate different offsets too */
      if(src_shift > dst_shift) {
         LLVMValueRef shift = lp_build_const_int_vec(gallivm, tmp_type,
                                                     src_shift - dst_shift);
         for(i = 0; i < num_tmps; ++i)
            if(src_type.sign)
               tmp[i] = LLVMBuildAShr(builder, tmp[i], shift, "");
            else
               tmp[i] = LLVMBuildLShr(builder, tmp[i], shift, "");
      }
   }

   /*
    * Truncate or expand bit width
    *
    * No data conversion should happen here, although the sign bits are
    * crucial to avoid bad clamping.
    */

   {
      struct lp_type new_type;

      new_type = tmp_type;
      new_type.sign   = dst_type.sign;
      new_type.width  = dst_type.width;
      new_type.length = dst_type.length;

      lp_build_resize(gallivm, tmp_type, new_type, tmp, num_srcs, tmp, num_dsts);

      tmp_type = new_type;
      num_tmps = num_dsts;
   }

   /*
    * Scale to the widest range
    */

   if(src_type.floating) {
      /* Nothing to do */
   }
   else if(!src_type.floating && dst_type.floating) {
      if(!src_type.fixed && !src_type.sign && src_type.norm) {
         for(i = 0; i < num_tmps; ++i) {
            tmp[i] = lp_build_unsigned_norm_to_float(gallivm,
                                                     src_type.width,
                                                     dst_type,
                                                     tmp[i]);
         }
         tmp_type.floating = TRUE;
      }
      else {
         double src_scale = lp_const_scale(src_type);
         LLVMTypeRef tmp_vec_type;

         /* Use an equally sized integer for intermediate computations */
         tmp_type.floating = TRUE;
         tmp_type.sign = TRUE;
         tmp_vec_type = lp_build_vec_type(gallivm, tmp_type);
         for(i = 0; i < num_tmps; ++i) {
#if 0
            if(dst_type.sign)
               tmp[i] = LLVMBuildSIToFP(builder, tmp[i], tmp_vec_type, "");
            else
               tmp[i] = LLVMBuildUIToFP(builder, tmp[i], tmp_vec_type, "");
#else
            /* FIXME: there is no SSE counterpart for LLVMBuildUIToFP */
            tmp[i] = LLVMBuildSIToFP(builder, tmp[i], tmp_vec_type, "");
#endif
          }

          if (src_scale != 1.0) {
             LLVMValueRef scale = lp_build_const_vec(gallivm, tmp_type, 1.0/src_scale);
             for(i = 0; i < num_tmps; ++i)
                tmp[i] = LLVMBuildFMul(builder, tmp[i], scale, "");
          }
      }
    }
    else {
       unsigned src_shift = lp_const_shift(src_type);
       unsigned dst_shift = lp_const_shift(dst_type);

       /* FIXME: compensate different offsets too */
       if(src_shift < dst_shift) {
          LLVMValueRef shift = lp_build_const_int_vec(gallivm, tmp_type, dst_shift - src_shift);
          for(i = 0; i < num_tmps; ++i)
             tmp[i] = LLVMBuildShl(builder, tmp[i], shift, "");
       }
    }

   for(i = 0; i < num_dsts; ++i) {
      dst[i] = tmp[i];
      assert(lp_check_value(dst_type, dst[i]));
   }
}


/**
 * Bit mask conversion.
 *
 * This will convert the integer masks that match the given types.
 *
 * The mask values should 0 or -1, i.e., all bits either set to zero or one.
 * Any other value will likely cause in unpredictable results.
 *
 * This is basically a very trimmed down version of lp_build_conv.
 */
void
lp_build_conv_mask(struct gallivm_state *gallivm,
                   struct lp_type src_type,
                   struct lp_type dst_type,
                   const LLVMValueRef *src, unsigned num_srcs,
                   LLVMValueRef *dst, unsigned num_dsts)
{
   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   /*
    * Drop
    *
    * We assume all values are 0 or -1
    */

   src_type.floating = FALSE;
   src_type.fixed = FALSE;
   src_type.sign = TRUE;
   src_type.norm = FALSE;

   dst_type.floating = FALSE;
   dst_type.fixed = FALSE;
   dst_type.sign = TRUE;
   dst_type.norm = FALSE;

   /*
    * Truncate or expand bit width
    */

   if(src_type.width > dst_type.width) {
      assert(num_dsts == 1);
      dst[0] = lp_build_pack(gallivm, src_type, dst_type, TRUE, src, num_srcs);
   }
   else if(src_type.width < dst_type.width) {
      assert(num_srcs == 1);
      lp_build_unpack(gallivm, src_type, dst_type, src[0], dst, num_dsts);
   }
   else {
      assert(num_srcs == num_dsts);
      memcpy(dst, src, num_dsts * sizeof *dst);
   }
}
