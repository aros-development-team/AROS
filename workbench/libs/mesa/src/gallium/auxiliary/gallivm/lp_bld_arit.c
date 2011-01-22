/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.
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
 * Helper
 *
 * LLVM IR doesn't support all basic arithmetic operations we care about (most
 * notably min/max and saturated operations), and it is often necessary to
 * resort machine-specific intrinsics directly. The functions here hide all
 * these implementation details from the other modules.
 *
 * We also do simple expressions simplification here. Reasons are:
 * - it is very easy given we have all necessary information readily available
 * - LLVM optimization passes fail to simplify several vector expressions
 * - We often know value constraints which the optimization passes have no way
 *   of knowing, such as when source arguments are known to be in [0, 1] range.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_string.h"
#include "util/u_cpu_detect.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_init.h"
#include "lp_bld_intr.h"
#include "lp_bld_logic.h"
#include "lp_bld_pack.h"
#include "lp_bld_debug.h"
#include "lp_bld_arit.h"


#define EXP_POLY_DEGREE 3

#define LOG_POLY_DEGREE 5


/**
 * Generate min(a, b)
 * No checks for special case values of a or b = 1 or 0 are done.
 */
static LLVMValueRef
lp_build_min_simple(struct lp_build_context *bld,
                    LLVMValueRef a,
                    LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   const char *intrinsic = NULL;
   LLVMValueRef cond;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   /* TODO: optimize the constant case */

   if(type.width * type.length == 128) {
      if(type.floating) {
         if(type.width == 32 && util_cpu_caps.has_sse)
            intrinsic = "llvm.x86.sse.min.ps";
         if(type.width == 64 && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.min.pd";
      }
      else {
         if(type.width == 8 && !type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pminu.b";
         if(type.width == 8 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminsb";
         if(type.width == 16 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminuw";
         if(type.width == 16 && type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pmins.w";
         if(type.width == 32 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminud";
         if(type.width == 32 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminsd";
      }
   }

   if(intrinsic)
      return lp_build_intrinsic_binary(builder, intrinsic, lp_build_vec_type(bld->gallivm, bld->type), a, b);

   cond = lp_build_cmp(bld, PIPE_FUNC_LESS, a, b);
   return lp_build_select(bld, cond, a, b);
}


/**
 * Generate max(a, b)
 * No checks for special case values of a or b = 1 or 0 are done.
 */
static LLVMValueRef
lp_build_max_simple(struct lp_build_context *bld,
                    LLVMValueRef a,
                    LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   const char *intrinsic = NULL;
   LLVMValueRef cond;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   /* TODO: optimize the constant case */

   if(type.width * type.length == 128) {
      if(type.floating) {
         if(type.width == 32 && util_cpu_caps.has_sse)
            intrinsic = "llvm.x86.sse.max.ps";
         if(type.width == 64 && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.max.pd";
      }
      else {
         if(type.width == 8 && !type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pmaxu.b";
         if(type.width == 8 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxsb";
         if(type.width == 16 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxuw";
         if(type.width == 16 && type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pmaxs.w";
         if(type.width == 32 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxud";
         if(type.width == 32 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxsd";
      }
   }

   if(intrinsic)
      return lp_build_intrinsic_binary(builder, intrinsic, lp_build_vec_type(bld->gallivm, bld->type), a, b);

   cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, b);
   return lp_build_select(bld, cond, a, b);
}


/**
 * Generate 1 - a, or ~a depending on bld->type.
 */
LLVMValueRef
lp_build_comp(struct lp_build_context *bld,
              LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(lp_check_value(type, a));

   if(a == bld->one)
      return bld->zero;
   if(a == bld->zero)
      return bld->one;

   if(type.norm && !type.floating && !type.fixed && !type.sign) {
      if(LLVMIsConstant(a))
         return LLVMConstNot(a);
      else
         return LLVMBuildNot(builder, a, "");
   }

   if(LLVMIsConstant(a))
      if (type.floating)
          return LLVMConstFSub(bld->one, a);
      else
          return LLVMConstSub(bld->one, a);
   else
      if (type.floating)
         return LLVMBuildFSub(builder, bld->one, a, "");
      else
         return LLVMBuildSub(builder, bld->one, a, "");
}


/**
 * Generate a + b
 */
LLVMValueRef
lp_build_add(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if(a == bld->zero)
      return b;
   if(b == bld->zero)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(bld->type.norm) {
      const char *intrinsic = NULL;

      if(a == bld->one || b == bld->one)
        return bld->one;

      if(util_cpu_caps.has_sse2 &&
         type.width * type.length == 128 &&
         !type.floating && !type.fixed) {
         if(type.width == 8)
            intrinsic = type.sign ? "llvm.x86.sse2.padds.b" : "llvm.x86.sse2.paddus.b";
         if(type.width == 16)
            intrinsic = type.sign ? "llvm.x86.sse2.padds.w" : "llvm.x86.sse2.paddus.w";
      }
   
      if(intrinsic)
         return lp_build_intrinsic_binary(builder, intrinsic, lp_build_vec_type(bld->gallivm, bld->type), a, b);
   }

   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      if (type.floating)
         res = LLVMConstFAdd(a, b);
      else
         res = LLVMConstAdd(a, b);
   else
      if (type.floating)
         res = LLVMBuildFAdd(builder, a, b, "");
      else
         res = LLVMBuildAdd(builder, a, b, "");

   /* clamp to ceiling of 1.0 */
   if(bld->type.norm && (bld->type.floating || bld->type.fixed))
      res = lp_build_min_simple(bld, res, bld->one);

   /* XXX clamp to floor of -1 or 0??? */

   return res;
}


/** Return the scalar sum of the elements of a */
LLVMValueRef
lp_build_sum_vector(struct lp_build_context *bld,
                    LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef index, res;
   unsigned i;

   assert(lp_check_value(type, a));

   if (type.length == 1) {
      return a;
   }

   assert(!bld->type.norm);

   index = lp_build_const_int32(bld->gallivm, 0);
   res = LLVMBuildExtractElement(builder, a, index, "");

   for (i = 1; i < type.length; i++) {
      index = lp_build_const_int32(bld->gallivm, i);
      if (type.floating)
         res = LLVMBuildFAdd(builder, res,
                            LLVMBuildExtractElement(builder,
                                                    a, index, ""),
                            "");
      else
         res = LLVMBuildAdd(builder, res,
                            LLVMBuildExtractElement(builder,
                                                    a, index, ""),
                            "");
   }

   return res;
}


/**
 * Generate a - b
 */
LLVMValueRef
lp_build_sub(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if(b == bld->zero)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;
   if(a == b)
      return bld->zero;

   if(bld->type.norm) {
      const char *intrinsic = NULL;

      if(b == bld->one)
        return bld->zero;

      if(util_cpu_caps.has_sse2 &&
         type.width * type.length == 128 &&
         !type.floating && !type.fixed) {
         if(type.width == 8)
            intrinsic = type.sign ? "llvm.x86.sse2.psubs.b" : "llvm.x86.sse2.psubus.b";
         if(type.width == 16)
            intrinsic = type.sign ? "llvm.x86.sse2.psubs.w" : "llvm.x86.sse2.psubus.w";
      }
   
      if(intrinsic)
         return lp_build_intrinsic_binary(builder, intrinsic, lp_build_vec_type(bld->gallivm, bld->type), a, b);
   }

   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      if (type.floating)
         res = LLVMConstFSub(a, b);
      else
         res = LLVMConstSub(a, b);
   else
      if (type.floating)
         res = LLVMBuildFSub(builder, a, b, "");
      else
         res = LLVMBuildSub(builder, a, b, "");

   if(bld->type.norm && (bld->type.floating || bld->type.fixed))
      res = lp_build_max_simple(bld, res, bld->zero);

   return res;
}


/**
 * Normalized 8bit multiplication.
 *
 * - alpha plus one
 *
 *     makes the following approximation to the division (Sree)
 *    
 *       a*b/255 ~= (a*(b + 1)) >> 256
 *    
 *     which is the fastest method that satisfies the following OpenGL criteria
 *    
 *       0*0 = 0 and 255*255 = 255
 *
 * - geometric series
 *
 *     takes the geometric series approximation to the division
 *
 *       t/255 = (t >> 8) + (t >> 16) + (t >> 24) ..
 *
 *     in this case just the first two terms to fit in 16bit arithmetic
 *
 *       t/255 ~= (t + (t >> 8)) >> 8
 *
 *     note that just by itself it doesn't satisfies the OpenGL criteria, as
 *     255*255 = 254, so the special case b = 255 must be accounted or roundoff
 *     must be used
 *
 * - geometric series plus rounding
 *
 *     when using a geometric series division instead of truncating the result
 *     use roundoff in the approximation (Jim Blinn)
 *
 *       t/255 ~= (t + (t >> 8) + 0x80) >> 8
 *
 *     achieving the exact results
 *
 * @sa Alvy Ray Smith, Image Compositing Fundamentals, Tech Memo 4, Aug 15, 1995, 
 *     ftp://ftp.alvyray.com/Acrobat/4_Comp.pdf
 * @sa Michael Herf, The "double blend trick", May 2000, 
 *     http://www.stereopsis.com/doubleblend.html
 */
static LLVMValueRef
lp_build_mul_u8n(struct gallivm_state *gallivm,
                 struct lp_type i16_type,
                 LLVMValueRef a, LLVMValueRef b)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef c8;
   LLVMValueRef ab;

   assert(!i16_type.floating);
   assert(lp_check_value(i16_type, a));
   assert(lp_check_value(i16_type, b));

   c8 = lp_build_const_int_vec(gallivm, i16_type, 8);
   
#if 0
   
   /* a*b/255 ~= (a*(b + 1)) >> 256 */
   b = LLVMBuildAdd(builder, b, lp_build_const_int_vec(gallium, i16_type, 1), "");
   ab = LLVMBuildMul(builder, a, b, "");

#else
   
   /* ab/255 ~= (ab + (ab >> 8) + 0x80) >> 8 */
   ab = LLVMBuildMul(builder, a, b, "");
   ab = LLVMBuildAdd(builder, ab, LLVMBuildLShr(builder, ab, c8, ""), "");
   ab = LLVMBuildAdd(builder, ab, lp_build_const_int_vec(gallivm, i16_type, 0x80), "");

#endif
   
   ab = LLVMBuildLShr(builder, ab, c8, "");

   return ab;
}


/**
 * Generate a * b
 */
LLVMValueRef
lp_build_mul(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef shift;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if(a == bld->zero)
      return bld->zero;
   if(a == bld->one)
      return b;
   if(b == bld->zero)
      return bld->zero;
   if(b == bld->one)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(!type.floating && !type.fixed && type.norm) {
      if(type.width == 8) {
         struct lp_type i16_type = lp_wider_type(type);
         LLVMValueRef al, ah, bl, bh, abl, abh, ab;

         lp_build_unpack2(bld->gallivm, type, i16_type, a, &al, &ah);
         lp_build_unpack2(bld->gallivm, type, i16_type, b, &bl, &bh);

         /* PMULLW, PSRLW, PADDW */
         abl = lp_build_mul_u8n(bld->gallivm, i16_type, al, bl);
         abh = lp_build_mul_u8n(bld->gallivm, i16_type, ah, bh);

         ab = lp_build_pack2(bld->gallivm, i16_type, type, abl, abh);
         
         return ab;
      }

      /* FIXME */
      assert(0);
   }

   if(type.fixed)
      shift = lp_build_const_int_vec(bld->gallivm, type, type.width/2);
   else
      shift = NULL;

   if(LLVMIsConstant(a) && LLVMIsConstant(b)) {
      if (type.floating)
         res = LLVMConstFMul(a, b);
      else
         res = LLVMConstMul(a, b);
      if(shift) {
         if(type.sign)
            res = LLVMConstAShr(res, shift);
         else
            res = LLVMConstLShr(res, shift);
      }
   }
   else {
      if (type.floating)
         res = LLVMBuildFMul(builder, a, b, "");
      else
         res = LLVMBuildMul(builder, a, b, "");
      if(shift) {
         if(type.sign)
            res = LLVMBuildAShr(builder, res, shift, "");
         else
            res = LLVMBuildLShr(builder, res, shift, "");
      }
   }

   return res;
}


/**
 * Small vector x scale multiplication optimization.
 */
LLVMValueRef
lp_build_mul_imm(struct lp_build_context *bld,
                 LLVMValueRef a,
                 int b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef factor;

   assert(lp_check_value(bld->type, a));

   if(b == 0)
      return bld->zero;

   if(b == 1)
      return a;

   if(b == -1)
      return lp_build_negate(bld, a);

   if(b == 2 && bld->type.floating)
      return lp_build_add(bld, a, a);

   if(util_is_power_of_two(b)) {
      unsigned shift = ffs(b) - 1;

      if(bld->type.floating) {
#if 0
         /*
          * Power of two multiplication by directly manipulating the mantissa.
          *
          * XXX: This might not be always faster, it will introduce a small error
          * for multiplication by zero, and it will produce wrong results
          * for Inf and NaN.
          */
         unsigned mantissa = lp_mantissa(bld->type);
         factor = lp_build_const_int_vec(bld->gallivm, bld->type, (unsigned long long)shift << mantissa);
         a = LLVMBuildBitCast(builder, a, lp_build_int_vec_type(bld->type), "");
         a = LLVMBuildAdd(builder, a, factor, "");
         a = LLVMBuildBitCast(builder, a, lp_build_vec_type(bld->gallivm, bld->type), "");
         return a;
#endif
      }
      else {
         factor = lp_build_const_vec(bld->gallivm, bld->type, shift);
         return LLVMBuildShl(builder, a, factor, "");
      }
   }

   factor = lp_build_const_vec(bld->gallivm, bld->type, (double)b);
   return lp_build_mul(bld, a, factor);
}


/**
 * Generate a / b
 */
LLVMValueRef
lp_build_div(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if(a == bld->zero)
      return bld->zero;
   if(a == bld->one)
      return lp_build_rcp(bld, b);
   if(b == bld->zero)
      return bld->undef;
   if(b == bld->one)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(LLVMIsConstant(a) && LLVMIsConstant(b)) {
      if (type.floating)
         return LLVMConstFDiv(a, b);
      else if (type.sign)
         return LLVMConstSDiv(a, b);
      else
         return LLVMConstUDiv(a, b);
   }

   if(util_cpu_caps.has_sse && type.width == 32 && type.length == 4)
      return lp_build_mul(bld, a, lp_build_rcp(bld, b));

   if (type.floating)
      return LLVMBuildFDiv(builder, a, b, "");
   else if (type.sign)
      return LLVMBuildSDiv(builder, a, b, "");
   else
      return LLVMBuildUDiv(builder, a, b, "");
}


/**
 * Linear interpolation -- without any checks.
 *
 * @sa http://www.stereopsis.com/doubleblend.html
 */
static INLINE LLVMValueRef
lp_build_lerp_simple(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef v0,
                     LLVMValueRef v1)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef delta;
   LLVMValueRef res;

   assert(lp_check_value(bld->type, x));
   assert(lp_check_value(bld->type, v0));
   assert(lp_check_value(bld->type, v1));

   delta = lp_build_sub(bld, v1, v0);

   res = lp_build_mul(bld, x, delta);

   res = lp_build_add(bld, v0, res);

   if (bld->type.fixed) {
      /* XXX: This step is necessary for lerping 8bit colors stored on 16bits,
       * but it will be wrong for other uses. Basically we need a more
       * powerful lp_type, capable of further distinguishing the values
       * interpretation from the value storage. */
      res = LLVMBuildAnd(builder, res, lp_build_const_int_vec(bld->gallivm, bld->type, (1 << bld->type.width/2) - 1), "");
   }

   return res;
}


/**
 * Linear interpolation.
 */
LLVMValueRef
lp_build_lerp(struct lp_build_context *bld,
              LLVMValueRef x,
              LLVMValueRef v0,
              LLVMValueRef v1)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, x));
   assert(lp_check_value(type, v0));
   assert(lp_check_value(type, v1));

   if (type.norm) {
      struct lp_type wide_type;
      struct lp_build_context wide_bld;
      LLVMValueRef xl, xh, v0l, v0h, v1l, v1h, resl, resh;
      LLVMValueRef shift;

      assert(type.length >= 2);
      assert(!type.sign);

      /*
       * Create a wider type, enough to hold the intermediate result of the
       * multiplication.
       */
      memset(&wide_type, 0, sizeof wide_type);
      wide_type.fixed  = TRUE;
      wide_type.width  = type.width*2;
      wide_type.length = type.length/2;

      lp_build_context_init(&wide_bld, bld->gallivm, wide_type);

      lp_build_unpack2(bld->gallivm, type, wide_type, x,  &xl,  &xh);
      lp_build_unpack2(bld->gallivm, type, wide_type, v0, &v0l, &v0h);
      lp_build_unpack2(bld->gallivm, type, wide_type, v1, &v1l, &v1h);

      /*
       * Scale x from [0, 255] to [0, 256]
       */

      shift = lp_build_const_int_vec(bld->gallivm, wide_type, type.width - 1);

      xl = lp_build_add(&wide_bld, xl,
                        LLVMBuildAShr(builder, xl, shift, ""));
      xh = lp_build_add(&wide_bld, xh,
                        LLVMBuildAShr(builder, xh, shift, ""));

      /*
       * Lerp both halves.
       */

      resl = lp_build_lerp_simple(&wide_bld, xl, v0l, v1l);
      resh = lp_build_lerp_simple(&wide_bld, xh, v0h, v1h);

      res = lp_build_pack2(bld->gallivm, wide_type, type, resl, resh);
   } else {
      res = lp_build_lerp_simple(bld, x, v0, v1);
   }

   return res;
}


LLVMValueRef
lp_build_lerp_2d(struct lp_build_context *bld,
                 LLVMValueRef x,
                 LLVMValueRef y,
                 LLVMValueRef v00,
                 LLVMValueRef v01,
                 LLVMValueRef v10,
                 LLVMValueRef v11)
{
   LLVMValueRef v0 = lp_build_lerp(bld, x, v00, v01);
   LLVMValueRef v1 = lp_build_lerp(bld, x, v10, v11);
   return lp_build_lerp(bld, y, v0, v1);
}


/**
 * Generate min(a, b)
 * Do checks for special cases.
 */
LLVMValueRef
lp_build_min(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   assert(lp_check_value(bld->type, a));
   assert(lp_check_value(bld->type, b));

   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(a == b)
      return a;

   if(bld->type.norm) {
      if(a == bld->zero || b == bld->zero)
         return bld->zero;
      if(a == bld->one)
         return b;
      if(b == bld->one)
         return a;
   }

   return lp_build_min_simple(bld, a, b);
}


/**
 * Generate max(a, b)
 * Do checks for special cases.
 */
LLVMValueRef
lp_build_max(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   assert(lp_check_value(bld->type, a));
   assert(lp_check_value(bld->type, b));

   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(a == b)
      return a;

   if(bld->type.norm) {
      if(a == bld->one || b == bld->one)
         return bld->one;
      if(a == bld->zero)
         return b;
      if(b == bld->zero)
         return a;
   }

   return lp_build_max_simple(bld, a, b);
}


/**
 * Generate clamp(a, min, max)
 * Do checks for special cases.
 */
LLVMValueRef
lp_build_clamp(struct lp_build_context *bld,
               LLVMValueRef a,
               LLVMValueRef min,
               LLVMValueRef max)
{
   assert(lp_check_value(bld->type, a));
   assert(lp_check_value(bld->type, min));
   assert(lp_check_value(bld->type, max));

   a = lp_build_min(bld, a, max);
   a = lp_build_max(bld, a, min);
   return a;
}


/**
 * Generate abs(a)
 */
LLVMValueRef
lp_build_abs(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);

   assert(lp_check_value(type, a));

   if(!type.sign)
      return a;

   if(type.floating) {
      /* Mask out the sign bit */
      LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);
      unsigned long long absMask = ~(1ULL << (type.width - 1));
      LLVMValueRef mask = lp_build_const_int_vec(bld->gallivm, type, ((unsigned long long) absMask));
      a = LLVMBuildBitCast(builder, a, int_vec_type, "");
      a = LLVMBuildAnd(builder, a, mask, "");
      a = LLVMBuildBitCast(builder, a, vec_type, "");
      return a;
   }

   if(type.width*type.length == 128 && util_cpu_caps.has_ssse3) {
      switch(type.width) {
      case 8:
         return lp_build_intrinsic_unary(builder, "llvm.x86.ssse3.pabs.b.128", vec_type, a);
      case 16:
         return lp_build_intrinsic_unary(builder, "llvm.x86.ssse3.pabs.w.128", vec_type, a);
      case 32:
         return lp_build_intrinsic_unary(builder, "llvm.x86.ssse3.pabs.d.128", vec_type, a);
      }
   }

   return lp_build_max(bld, a, LLVMBuildNeg(builder, a, ""));
}


LLVMValueRef
lp_build_negate(struct lp_build_context *bld,
                LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;

   assert(lp_check_value(bld->type, a));

#if HAVE_LLVM >= 0x0207
   if (bld->type.floating)
      a = LLVMBuildFNeg(builder, a, "");
   else
#endif
      a = LLVMBuildNeg(builder, a, "");

   return a;
}


/** Return -1, 0 or +1 depending on the sign of a */
LLVMValueRef
lp_build_sgn(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef cond;
   LLVMValueRef res;

   assert(lp_check_value(type, a));

   /* Handle non-zero case */
   if(!type.sign) {
      /* if not zero then sign must be positive */
      res = bld->one;
   }
   else if(type.floating) {
      LLVMTypeRef vec_type;
      LLVMTypeRef int_type;
      LLVMValueRef mask;
      LLVMValueRef sign;
      LLVMValueRef one;
      unsigned long long maskBit = (unsigned long long)1 << (type.width - 1);

      int_type = lp_build_int_vec_type(bld->gallivm, type);
      vec_type = lp_build_vec_type(bld->gallivm, type);
      mask = lp_build_const_int_vec(bld->gallivm, type, maskBit);

      /* Take the sign bit and add it to 1 constant */
      sign = LLVMBuildBitCast(builder, a, int_type, "");
      sign = LLVMBuildAnd(builder, sign, mask, "");
      one = LLVMConstBitCast(bld->one, int_type);
      res = LLVMBuildOr(builder, sign, one, "");
      res = LLVMBuildBitCast(builder, res, vec_type, "");
   }
   else
   {
      LLVMValueRef minus_one = lp_build_const_vec(bld->gallivm, type, -1.0);
      cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, bld->zero);
      res = lp_build_select(bld, cond, bld->one, minus_one);
   }

   /* Handle zero */
   cond = lp_build_cmp(bld, PIPE_FUNC_EQUAL, a, bld->zero);
   res = lp_build_select(bld, cond, bld->zero, res);

   return res;
}


/**
 * Set the sign of float vector 'a' according to 'sign'.
 * If sign==0, return abs(a).
 * If sign==1, return -abs(a);
 * Other values for sign produce undefined results.
 */
LLVMValueRef
lp_build_set_sign(struct lp_build_context *bld,
                  LLVMValueRef a, LLVMValueRef sign)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   LLVMValueRef shift = lp_build_const_int_vec(bld->gallivm, type, type.width - 1);
   LLVMValueRef mask = lp_build_const_int_vec(bld->gallivm, type,
                             ~((unsigned long long) 1 << (type.width - 1)));
   LLVMValueRef val, res;

   assert(type.floating);
   assert(lp_check_value(type, a));

   /* val = reinterpret_cast<int>(a) */
   val = LLVMBuildBitCast(builder, a, int_vec_type, "");
   /* val = val & mask */
   val = LLVMBuildAnd(builder, val, mask, "");
   /* sign = sign << shift */
   sign = LLVMBuildShl(builder, sign, shift, "");
   /* res = val | sign */
   res = LLVMBuildOr(builder, val, sign, "");
   /* res = reinterpret_cast<float>(res) */
   res = LLVMBuildBitCast(builder, res, vec_type, "");

   return res;
}


/**
 * Convert vector of (or scalar) int to vector of (or scalar) float.
 */
LLVMValueRef
lp_build_int_to_float(struct lp_build_context *bld,
                      LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);

   assert(type.floating);

   return LLVMBuildSIToFP(builder, a, vec_type, "");
}



enum lp_build_round_sse41_mode
{
   LP_BUILD_ROUND_SSE41_NEAREST = 0,
   LP_BUILD_ROUND_SSE41_FLOOR = 1,
   LP_BUILD_ROUND_SSE41_CEIL = 2,
   LP_BUILD_ROUND_SSE41_TRUNCATE = 3
};


/**
 * Helper for SSE4.1's ROUNDxx instructions.
 *
 * NOTE: In the SSE4.1's nearest mode, if two values are equally close, the
 * result is the even value.  That is, rounding 2.5 will be 2.0, and not 3.0.
 */
static INLINE LLVMValueRef
lp_build_round_sse41(struct lp_build_context *bld,
                     LLVMValueRef a,
                     enum lp_build_round_sse41_mode mode)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(bld->gallivm->context);
   const char *intrinsic;
   LLVMValueRef res;

   assert(type.floating);

   assert(lp_check_value(type, a));
   assert(util_cpu_caps.has_sse4_1);

   if (type.length == 1) {
      LLVMTypeRef vec_type;
      LLVMValueRef undef;
      LLVMValueRef args[3];
      LLVMValueRef index0 = LLVMConstInt(i32t, 0, 0);

      switch(type.width) {
      case 32:
         intrinsic = "llvm.x86.sse41.round.ss";
         break;
      case 64:
         intrinsic = "llvm.x86.sse41.round.sd";
         break;
      default:
         assert(0);
         return bld->undef;
      }

      vec_type = LLVMVectorType(bld->elem_type, 4);

      undef = LLVMGetUndef(vec_type);

      args[0] = undef;
      args[1] = LLVMBuildInsertElement(builder, undef, a, index0, "");
      args[2] = LLVMConstInt(i32t, mode, 0);

      res = lp_build_intrinsic(builder, intrinsic,
                               vec_type, args, Elements(args));

      res = LLVMBuildExtractElement(builder, res, index0, "");
   }
   else {
      assert(type.width*type.length == 128);

      switch(type.width) {
      case 32:
         intrinsic = "llvm.x86.sse41.round.ps";
         break;
      case 64:
         intrinsic = "llvm.x86.sse41.round.pd";
         break;
      default:
         assert(0);
         return bld->undef;
      }

      res = lp_build_intrinsic_binary(builder, intrinsic,
                                      bld->vec_type, a,
                                      LLVMConstInt(i32t, mode, 0));
   }

   return res;
}


static INLINE LLVMValueRef
lp_build_iround_nearest_sse2(struct lp_build_context *bld,
                             LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(bld->gallivm->context);
   LLVMTypeRef ret_type = lp_build_int_vec_type(bld->gallivm, type);
   const char *intrinsic;
   LLVMValueRef res;

   assert(type.floating);
   /* using the double precision conversions is a bit more complicated */
   assert(type.width == 32);

   assert(lp_check_value(type, a));
   assert(util_cpu_caps.has_sse2);

   /* This is relying on MXCSR rounding mode, which should always be nearest. */
   if (type.length == 1) {
      LLVMTypeRef vec_type;
      LLVMValueRef undef;
      LLVMValueRef arg;
      LLVMValueRef index0 = LLVMConstInt(i32t, 0, 0);

      vec_type = LLVMVectorType(bld->elem_type, 4);

      intrinsic = "llvm.x86.sse.cvtss2si";

      undef = LLVMGetUndef(vec_type);

      arg = LLVMBuildInsertElement(builder, undef, a, index0, "");

      res = lp_build_intrinsic_unary(builder, intrinsic,
                                     ret_type, arg);
   }
   else {
      assert(type.width*type.length == 128);

      intrinsic = "llvm.x86.sse2.cvtps2dq";

      res = lp_build_intrinsic_unary(builder, intrinsic,
                                     ret_type, a);
   }

   return res;
}


/**
 * Return the integer part of a float (vector) value (== round toward zero).
 * The returned value is a float (vector).
 * Ex: trunc(-1.5) = -1.0
 */
LLVMValueRef
lp_build_trunc(struct lp_build_context *bld,
               LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_TRUNCATE);
   }
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
      LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);
      LLVMValueRef res;
      res = LLVMBuildFPToSI(builder, a, int_vec_type, "");
      res = LLVMBuildSIToFP(builder, res, vec_type, "");
      return res;
   }
}


/**
 * Return float (vector) rounded to nearest integer (vector).  The returned
 * value is a float (vector).
 * Ex: round(0.9) = 1.0
 * Ex: round(-1.5) = -2.0
 */
LLVMValueRef
lp_build_round(struct lp_build_context *bld,
               LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_NEAREST);
   }
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
      LLVMValueRef res;
      res = lp_build_iround(bld, a);
      res = LLVMBuildSIToFP(builder, res, vec_type, "");
      return res;
   }
}


/**
 * Return floor of float (vector), result is a float (vector)
 * Ex: floor(1.1) = 1.0
 * Ex: floor(-1.1) = -2.0
 */
LLVMValueRef
lp_build_floor(struct lp_build_context *bld,
               LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_FLOOR);
   }
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
      LLVMValueRef res;
      res = lp_build_ifloor(bld, a);
      res = LLVMBuildSIToFP(builder, res, vec_type, "");
      return res;
   }
}


/**
 * Return ceiling of float (vector), returning float (vector).
 * Ex: ceil( 1.1) = 2.0
 * Ex: ceil(-1.1) = -1.0
 */
LLVMValueRef
lp_build_ceil(struct lp_build_context *bld,
              LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_CEIL);
   }
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
      LLVMValueRef res;
      res = lp_build_iceil(bld, a);
      res = LLVMBuildSIToFP(builder, res, vec_type, "");
      return res;
   }
}


/**
 * Return fractional part of 'a' computed as a - floor(a)
 * Typically used in texture coord arithmetic.
 */
LLVMValueRef
lp_build_fract(struct lp_build_context *bld,
               LLVMValueRef a)
{
   assert(bld->type.floating);
   return lp_build_sub(bld, a, lp_build_floor(bld, a));
}


/**
 * Return the integer part of a float (vector) value (== round toward zero).
 * The returned value is an integer (vector).
 * Ex: itrunc(-1.5) = -1
 */
LLVMValueRef
lp_build_itrunc(struct lp_build_context *bld,
                LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);

   assert(type.floating);
   assert(lp_check_value(type, a));

   return LLVMBuildFPToSI(builder, a, int_vec_type, "");
}


/**
 * Return float (vector) rounded to nearest integer (vector).  The returned
 * value is an integer (vector).
 * Ex: iround(0.9) = 1
 * Ex: iround(-1.5) = -2
 */
LLVMValueRef
lp_build_iround(struct lp_build_context *bld,
                LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
   LLVMValueRef res;

   assert(type.floating);

   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse2 &&
       ((type.width == 32) && (type.length == 1 || type.length == 4))) {
      return lp_build_iround_nearest_sse2(bld, a);
   }
   else if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      res = lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_NEAREST);
   }
   else {
      LLVMValueRef half;

      half = lp_build_const_vec(bld->gallivm, type, 0.5);

      if (type.sign) {
         LLVMTypeRef vec_type = bld->vec_type;
         LLVMValueRef mask = lp_build_const_int_vec(bld->gallivm, type,
                                    (unsigned long long)1 << (type.width - 1));
         LLVMValueRef sign;

         /* get sign bit */
         sign = LLVMBuildBitCast(builder, a, int_vec_type, "");
         sign = LLVMBuildAnd(builder, sign, mask, "");

         /* sign * 0.5 */
         half = LLVMBuildBitCast(builder, half, int_vec_type, "");
         half = LLVMBuildOr(builder, sign, half, "");
         half = LLVMBuildBitCast(builder, half, vec_type, "");
      }

      res = LLVMBuildFAdd(builder, a, half, "");
   }

   res = LLVMBuildFPToSI(builder, res, int_vec_type, "");

   return res;
}


/**
 * Return floor of float (vector), result is an int (vector)
 * Ex: ifloor(1.1) = 1.0
 * Ex: ifloor(-1.1) = -2.0
 */
LLVMValueRef
lp_build_ifloor(struct lp_build_context *bld,
                LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
   LLVMValueRef res;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      res = lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_FLOOR);
   }
   else {
      res = a;

      if (type.sign) {
         /* Take the sign bit and add it to 1 constant */
         LLVMTypeRef vec_type = bld->vec_type;
         unsigned mantissa = lp_mantissa(type);
         LLVMValueRef mask = lp_build_const_int_vec(bld->gallivm, type,
                                  (unsigned long long)1 << (type.width - 1));
         LLVMValueRef sign;
         LLVMValueRef offset;

         /* sign = a < 0 ? ~0 : 0 */
         sign = LLVMBuildBitCast(builder, a, int_vec_type, "");
         sign = LLVMBuildAnd(builder, sign, mask, "");
         sign = LLVMBuildAShr(builder, sign,
                              lp_build_const_int_vec(bld->gallivm, type,
                                                     type.width - 1),
                              "ifloor.sign");

         /* offset = -0.99999(9)f */
         offset = lp_build_const_vec(bld->gallivm, type,
                                     -(double)(((unsigned long long)1 << mantissa) - 10)/((unsigned long long)1 << mantissa));
         offset = LLVMConstBitCast(offset, int_vec_type);

         /* offset = a < 0 ? offset : 0.0f */
         offset = LLVMBuildAnd(builder, offset, sign, "");
         offset = LLVMBuildBitCast(builder, offset, vec_type, "ifloor.offset");

         res = LLVMBuildFAdd(builder, res, offset, "ifloor.res");
      }
   }

   /* round to nearest (toward zero) */
   res = LLVMBuildFPToSI(builder, res, int_vec_type, "ifloor.res");

   return res;
}


/**
 * Return ceiling of float (vector), returning int (vector).
 * Ex: iceil( 1.1) = 2
 * Ex: iceil(-1.1) = -1
 */
LLVMValueRef
lp_build_iceil(struct lp_build_context *bld,
               LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
   LLVMValueRef res;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      res = lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_CEIL);
   }
   else {
      LLVMTypeRef vec_type = bld->vec_type;
      unsigned mantissa = lp_mantissa(type);
      LLVMValueRef offset;

      /* offset = 0.99999(9)f */
      offset = lp_build_const_vec(bld->gallivm, type,
                                  (double)(((unsigned long long)1 << mantissa) - 10)/((unsigned long long)1 << mantissa));

      if (type.sign) {
         LLVMValueRef mask = lp_build_const_int_vec(bld->gallivm, type,
                                (unsigned long long)1 << (type.width - 1));
         LLVMValueRef sign;

         /* sign = a < 0 ? 0 : ~0 */
         sign = LLVMBuildBitCast(builder, a, int_vec_type, "");
         sign = LLVMBuildAnd(builder, sign, mask, "");
         sign = LLVMBuildAShr(builder, sign,
                              lp_build_const_int_vec(bld->gallivm, type,
                                                     type.width - 1),
                              "iceil.sign");
         sign = LLVMBuildNot(builder, sign, "iceil.not");

         /* offset = a < 0 ? 0.0 : offset */
         offset = LLVMConstBitCast(offset, int_vec_type);
         offset = LLVMBuildAnd(builder, offset, sign, "");
         offset = LLVMBuildBitCast(builder, offset, vec_type, "iceil.offset");
      }

      res = LLVMBuildFAdd(builder, a, offset, "iceil.res");
   }

   /* round to nearest (toward zero) */
   res = LLVMBuildFPToSI(builder, res, int_vec_type, "iceil.res");

   return res;
}


/**
 * Combined ifloor() & fract().
 *
 * Preferred to calling the functions separately, as it will ensure that the
 * stratergy (floor() vs ifloor()) that results in less redundant work is used.
 */
void
lp_build_ifloor_fract(struct lp_build_context *bld,
                      LLVMValueRef a,
                      LLVMValueRef *out_ipart,
                      LLVMValueRef *out_fpart)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef ipart;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if (util_cpu_caps.has_sse4_1 &&
       (type.length == 1 || type.width*type.length == 128)) {
      /*
       * floor() is easier.
       */

      ipart = lp_build_floor(bld, a);
      *out_fpart = LLVMBuildFSub(builder, a, ipart, "fpart");
      *out_ipart = LLVMBuildFPToSI(builder, ipart, bld->int_vec_type, "ipart");
   }
   else {
      /*
       * ifloor() is easier.
       */

      *out_ipart = lp_build_ifloor(bld, a);
      ipart = LLVMBuildSIToFP(builder, *out_ipart, bld->vec_type, "ipart");
      *out_fpart = LLVMBuildFSub(builder, a, ipart, "fpart");
   }
}


LLVMValueRef
lp_build_sqrt(struct lp_build_context *bld,
              LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   char intrinsic[32];

   assert(lp_check_value(type, a));

   /* TODO: optimize the constant case */
   /* TODO: optimize the constant case */

   assert(type.floating);
   util_snprintf(intrinsic, sizeof intrinsic, "llvm.sqrt.v%uf%u", type.length, type.width);

   return lp_build_intrinsic_unary(builder, intrinsic, vec_type, a);
}


/**
 * Do one Newton-Raphson step to improve reciprocate precision:
 *
 *   x_{i+1} = x_i * (2 - a * x_i)
 *
 * XXX: Unfortunately this won't give IEEE-754 conformant results for 0 or
 * +/-Inf, giving NaN instead.  Certain applications rely on this behavior,
 * such as Google Earth, which does RCP(RSQRT(0.0) when drawing the Earth's
 * halo. It would be necessary to clamp the argument to prevent this.
 *
 * See also:
 * - http://en.wikipedia.org/wiki/Division_(digital)#Newton.E2.80.93Raphson_division
 * - http://softwarecommunity.intel.com/articles/eng/1818.htm
 */
static INLINE LLVMValueRef
lp_build_rcp_refine(struct lp_build_context *bld,
                    LLVMValueRef a,
                    LLVMValueRef rcp_a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef two = lp_build_const_vec(bld->gallivm, bld->type, 2.0);
   LLVMValueRef res;

   res = LLVMBuildFMul(builder, a, rcp_a, "");
   res = LLVMBuildFSub(builder, two, res, "");
   res = LLVMBuildFMul(builder, rcp_a, res, "");

   return res;
}


LLVMValueRef
lp_build_rcp(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(lp_check_value(type, a));

   if(a == bld->zero)
      return bld->undef;
   if(a == bld->one)
      return bld->one;
   if(a == bld->undef)
      return bld->undef;

   assert(type.floating);

   if(LLVMIsConstant(a))
      return LLVMConstFDiv(bld->one, a);

   /*
    * We don't use RCPPS because:
    * - it only has 10bits of precision
    * - it doesn't even get the reciprocate of 1.0 exactly
    * - doing Newton-Rapshon steps yields wrong (NaN) values for 0.0 or Inf
    * - for recent processors the benefit over DIVPS is marginal, a case
    *   depedent
    *
    * We could still use it on certain processors if benchmarks show that the
    * RCPPS plus necessary workarounds are still preferrable to DIVPS; or for
    * particular uses that require less workarounds.
    */

   if (FALSE && util_cpu_caps.has_sse && type.width == 32 && type.length == 4) {
      const unsigned num_iterations = 0;
      LLVMValueRef res;
      unsigned i;

      res = lp_build_intrinsic_unary(builder, "llvm.x86.sse.rcp.ps", bld->vec_type, a);

      for (i = 0; i < num_iterations; ++i) {
         res = lp_build_rcp_refine(bld, a, res);
      }

      return res;
   }

   return LLVMBuildFDiv(builder, bld->one, a, "");
}


/**
 * Do one Newton-Raphson step to improve rsqrt precision:
 *
 *   x_{i+1} = 0.5 * x_i * (3.0 - a * x_i * x_i)
 *
 * See also:
 * - http://softwarecommunity.intel.com/articles/eng/1818.htm
 */
static INLINE LLVMValueRef
lp_build_rsqrt_refine(struct lp_build_context *bld,
                      LLVMValueRef a,
                      LLVMValueRef rsqrt_a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, bld->type, 0.5);
   LLVMValueRef three = lp_build_const_vec(bld->gallivm, bld->type, 3.0);
   LLVMValueRef res;

   res = LLVMBuildFMul(builder, rsqrt_a, rsqrt_a, "");
   res = LLVMBuildFMul(builder, a, res, "");
   res = LLVMBuildFSub(builder, three, res, "");
   res = LLVMBuildFMul(builder, rsqrt_a, res, "");
   res = LLVMBuildFMul(builder, half, res, "");

   return res;
}


/**
 * Generate 1/sqrt(a)
 */
LLVMValueRef
lp_build_rsqrt(struct lp_build_context *bld,
               LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;

   assert(lp_check_value(type, a));

   assert(type.floating);

   if (util_cpu_caps.has_sse && type.width == 32 && type.length == 4) {
      const unsigned num_iterations = 0;
      LLVMValueRef res;
      unsigned i;

      res = lp_build_intrinsic_unary(builder, "llvm.x86.sse.rsqrt.ps", bld->vec_type, a);

      for (i = 0; i < num_iterations; ++i) {
         res = lp_build_rsqrt_refine(bld, a, res);
      }

      return res;
   }

   return lp_build_rcp(bld, lp_build_sqrt(bld, a));
}


static inline LLVMValueRef
lp_build_const_v4si(struct gallivm_state *gallivm, unsigned long value)
{
   LLVMValueRef element = lp_build_const_int32(gallivm, value);
   LLVMValueRef elements[4] = { element, element, element, element };
   return LLVMConstVector(elements, 4);
}

static inline LLVMValueRef
lp_build_const_v4sf(struct gallivm_state *gallivm, float value)
{
   LLVMValueRef element = lp_build_const_float(gallivm, value);
   LLVMValueRef elements[4] = { element, element, element, element };
   return LLVMConstVector(elements, 4);
}


/**
 * Generate sin(a) using SSE2
 */
LLVMValueRef
lp_build_sin(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct gallivm_state *gallivm = bld->gallivm;
   struct lp_type int_type = lp_int_type(bld->type);
   LLVMBuilderRef b = builder;
   LLVMTypeRef v4sf = LLVMVectorType(LLVMFloatTypeInContext(bld->gallivm->context), 4);
   LLVMTypeRef v4si = LLVMVectorType(LLVMInt32TypeInContext(bld->gallivm->context), 4);

   /*
    *  take the absolute value,
    *  x = _mm_and_ps(x, *(v4sf*)_ps_inv_sign_mask);
    */

   LLVMValueRef inv_sig_mask = lp_build_const_v4si(bld->gallivm, ~0x80000000);
   LLVMValueRef a_v4si = LLVMBuildBitCast(b, a, v4si, "a_v4si");

   LLVMValueRef absi = LLVMBuildAnd(b, a_v4si, inv_sig_mask, "absi");
   LLVMValueRef x_abs = LLVMBuildBitCast(b, absi, v4sf, "x_abs");

   /*
    * extract the sign bit (upper one)
    * sign_bit = _mm_and_ps(sign_bit, *(v4sf*)_ps_sign_mask);
    */
   LLVMValueRef sig_mask = lp_build_const_v4si(bld->gallivm, 0x80000000);
   LLVMValueRef sign_bit_i = LLVMBuildAnd(b, a_v4si, sig_mask, "sign_bit_i");

   /*
    * scale by 4/Pi
    * y = _mm_mul_ps(x, *(v4sf*)_ps_cephes_FOPI);
    */
   
   LLVMValueRef FOPi = lp_build_const_v4sf(gallivm, 1.27323954473516);
   LLVMValueRef scale_y = LLVMBuildFMul(b, x_abs, FOPi, "scale_y");

   /*
    * store the integer part of y in mm0
    * emm2 = _mm_cvttps_epi32(y);
    */
   
   LLVMValueRef emm2_i = LLVMBuildFPToSI(b, scale_y, v4si, "emm2_i");

   /*
    * j=(j+1) & (~1) (see the cephes sources)
    * emm2 = _mm_add_epi32(emm2, *(v4si*)_pi32_1);
    */

   LLVMValueRef all_one = lp_build_const_v4si(bld->gallivm, 1);
   LLVMValueRef emm2_add =  LLVMBuildAdd(b, emm2_i, all_one, "emm2_add");
   /*
    * emm2 = _mm_and_si128(emm2, *(v4si*)_pi32_inv1);
    */
   LLVMValueRef inv_one = lp_build_const_v4si(bld->gallivm, ~1);
   LLVMValueRef emm2_and =  LLVMBuildAnd(b, emm2_add, inv_one, "emm2_and");

   /*
    * y = _mm_cvtepi32_ps(emm2);
    */
   LLVMValueRef y_2 = LLVMBuildSIToFP(b, emm2_and, v4sf, "y_2");

   /* get the swap sign flag
    * emm0 = _mm_and_si128(emm2, *(v4si*)_pi32_4);
    */
   LLVMValueRef pi32_4 = lp_build_const_v4si(bld->gallivm, 4);
   LLVMValueRef emm0_and =  LLVMBuildAnd(b, emm2_add, pi32_4, "emm0_and");
   
   /*
    * emm2 = _mm_slli_epi32(emm0, 29);
    */  
   LLVMValueRef const_29 = lp_build_const_v4si(bld->gallivm, 29);
   LLVMValueRef swap_sign_bit = LLVMBuildShl(b, emm0_and, const_29, "swap_sign_bit");

   /*
    * get the polynom selection mask 
    * there is one polynom for 0 <= x <= Pi/4
    * and another one for Pi/4<x<=Pi/2
    * Both branches will be computed.
    *  
    * emm2 = _mm_and_si128(emm2, *(v4si*)_pi32_2);
    * emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());
    */

   LLVMValueRef pi32_2 = lp_build_const_v4si(bld->gallivm, 2);
   LLVMValueRef emm2_3 =  LLVMBuildAnd(b, emm2_and, pi32_2, "emm2_3");
   LLVMValueRef poly_mask = lp_build_compare(bld->gallivm,
                                             int_type, PIPE_FUNC_EQUAL,
                                             emm2_3, lp_build_const_v4si(bld->gallivm, 0));
   /*
    *   sign_bit = _mm_xor_ps(sign_bit, swap_sign_bit);
    */
   LLVMValueRef sign_bit_1 =  LLVMBuildXor(b, sign_bit_i, swap_sign_bit, "sign_bit");

   /*
    * _PS_CONST(minus_cephes_DP1, -0.78515625);
    * _PS_CONST(minus_cephes_DP2, -2.4187564849853515625e-4);
    * _PS_CONST(minus_cephes_DP3, -3.77489497744594108e-8);
    */
   LLVMValueRef DP1 = lp_build_const_v4sf(gallivm, -0.78515625);
   LLVMValueRef DP2 = lp_build_const_v4sf(gallivm, -2.4187564849853515625e-4);
   LLVMValueRef DP3 = lp_build_const_v4sf(gallivm, -3.77489497744594108e-8);

   /*
    * The magic pass: "Extended precision modular arithmetic" 
    * x = ((x - y * DP1) - y * DP2) - y * DP3; 
    * xmm1 = _mm_mul_ps(y, xmm1);
    * xmm2 = _mm_mul_ps(y, xmm2);
    * xmm3 = _mm_mul_ps(y, xmm3);
    */
   LLVMValueRef xmm1 = LLVMBuildFMul(b, y_2, DP1, "xmm1");
   LLVMValueRef xmm2 = LLVMBuildFMul(b, y_2, DP2, "xmm2");
   LLVMValueRef xmm3 = LLVMBuildFMul(b, y_2, DP3, "xmm3");

   /*
    * x = _mm_add_ps(x, xmm1);
    * x = _mm_add_ps(x, xmm2);
    * x = _mm_add_ps(x, xmm3);
    */ 

   LLVMValueRef x_1 = LLVMBuildFAdd(b, x_abs, xmm1, "x_1");
   LLVMValueRef x_2 = LLVMBuildFAdd(b, x_1, xmm2, "x_2");
   LLVMValueRef x_3 = LLVMBuildFAdd(b, x_2, xmm3, "x_3");

   /*
    * Evaluate the first polynom  (0 <= x <= Pi/4)
    *
    * z = _mm_mul_ps(x,x);
    */
   LLVMValueRef z = LLVMBuildFMul(b, x_3, x_3, "z");

   /*
    * _PS_CONST(coscof_p0,  2.443315711809948E-005);
    * _PS_CONST(coscof_p1, -1.388731625493765E-003);
    * _PS_CONST(coscof_p2,  4.166664568298827E-002);
    */
   LLVMValueRef coscof_p0 = lp_build_const_v4sf(gallivm, 2.443315711809948E-005);
   LLVMValueRef coscof_p1 = lp_build_const_v4sf(gallivm, -1.388731625493765E-003);
   LLVMValueRef coscof_p2 = lp_build_const_v4sf(gallivm, 4.166664568298827E-002);

   /*
    * y = *(v4sf*)_ps_coscof_p0;
    * y = _mm_mul_ps(y, z);
    */
   LLVMValueRef y_3 = LLVMBuildFMul(b, z, coscof_p0, "y_3");
   LLVMValueRef y_4 = LLVMBuildFAdd(b, y_3, coscof_p1, "y_4");
   LLVMValueRef y_5 = LLVMBuildFMul(b, y_4, z, "y_5");
   LLVMValueRef y_6 = LLVMBuildFAdd(b, y_5, coscof_p2, "y_6");
   LLVMValueRef y_7 = LLVMBuildFMul(b, y_6, z, "y_7");
   LLVMValueRef y_8 = LLVMBuildFMul(b, y_7, z, "y_8");


   /*
    * tmp = _mm_mul_ps(z, *(v4sf*)_ps_0p5);
    * y = _mm_sub_ps(y, tmp);
    * y = _mm_add_ps(y, *(v4sf*)_ps_1);
    */ 
   LLVMValueRef half = lp_build_const_v4sf(gallivm, 0.5);
   LLVMValueRef tmp = LLVMBuildFMul(b, z, half, "tmp");
   LLVMValueRef y_9 = LLVMBuildFSub(b, y_8, tmp, "y_8");
   LLVMValueRef one = lp_build_const_v4sf(gallivm, 1.0);
   LLVMValueRef y_10 = LLVMBuildFAdd(b, y_9, one, "y_9");

   /*
    * _PS_CONST(sincof_p0, -1.9515295891E-4);
    * _PS_CONST(sincof_p1,  8.3321608736E-3);
    * _PS_CONST(sincof_p2, -1.6666654611E-1);
    */
   LLVMValueRef sincof_p0 = lp_build_const_v4sf(gallivm, -1.9515295891E-4);
   LLVMValueRef sincof_p1 = lp_build_const_v4sf(gallivm, 8.3321608736E-3);
   LLVMValueRef sincof_p2 = lp_build_const_v4sf(gallivm, -1.6666654611E-1);

   /*
    * Evaluate the second polynom  (Pi/4 <= x <= 0)
    *
    * y2 = *(v4sf*)_ps_sincof_p0;
    * y2 = _mm_mul_ps(y2, z);
    * y2 = _mm_add_ps(y2, *(v4sf*)_ps_sincof_p1);
    * y2 = _mm_mul_ps(y2, z);
    * y2 = _mm_add_ps(y2, *(v4sf*)_ps_sincof_p2);
    * y2 = _mm_mul_ps(y2, z);
    * y2 = _mm_mul_ps(y2, x);
    * y2 = _mm_add_ps(y2, x);
    */

   LLVMValueRef y2_3 = LLVMBuildFMul(b, z, sincof_p0, "y2_3");
   LLVMValueRef y2_4 = LLVMBuildFAdd(b, y2_3, sincof_p1, "y2_4");
   LLVMValueRef y2_5 = LLVMBuildFMul(b, y2_4, z, "y2_5");
   LLVMValueRef y2_6 = LLVMBuildFAdd(b, y2_5, sincof_p2, "y2_6");
   LLVMValueRef y2_7 = LLVMBuildFMul(b, y2_6, z, "y2_7");
   LLVMValueRef y2_8 = LLVMBuildFMul(b, y2_7, x_3, "y2_8");
   LLVMValueRef y2_9 = LLVMBuildFAdd(b, y2_8, x_3, "y2_9");

   /*
    * select the correct result from the two polynoms
    * xmm3 = poly_mask;
    * y2 = _mm_and_ps(xmm3, y2); //, xmm3);
    * y = _mm_andnot_ps(xmm3, y);
    * y = _mm_add_ps(y,y2);
    */
   LLVMValueRef y2_i = LLVMBuildBitCast(b, y2_9, v4si, "y2_i");
   LLVMValueRef y_i = LLVMBuildBitCast(b, y_10, v4si, "y_i");
   LLVMValueRef y2_and = LLVMBuildAnd(b, y2_i, poly_mask, "y2_and");
   LLVMValueRef inv = lp_build_const_v4si(bld->gallivm, ~0);
   LLVMValueRef poly_mask_inv = LLVMBuildXor(b, poly_mask, inv, "poly_mask_inv");
   LLVMValueRef y_and = LLVMBuildAnd(b, y_i, poly_mask_inv, "y_and");
   LLVMValueRef y_combine = LLVMBuildAdd(b, y_and, y2_and, "y_combine");

   /*
    * update the sign
    * y = _mm_xor_ps(y, sign_bit);
    */
   LLVMValueRef y_sign = LLVMBuildXor(b, y_combine, sign_bit_1, "y_sin");
   LLVMValueRef y_result = LLVMBuildBitCast(b, y_sign, v4sf, "y_result");
   return y_result;
}


/**
 * Generate cos(a) using SSE2
 */
LLVMValueRef
lp_build_cos(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct gallivm_state *gallivm = bld->gallivm;
   struct lp_type int_type = lp_int_type(bld->type);
   LLVMBuilderRef b = builder;
   LLVMTypeRef v4sf = LLVMVectorType(LLVMFloatTypeInContext(bld->gallivm->context), 4);
   LLVMTypeRef v4si = LLVMVectorType(LLVMInt32TypeInContext(bld->gallivm->context), 4);

   /*
    *  take the absolute value,
    *  x = _mm_and_ps(x, *(v4sf*)_ps_inv_sign_mask);
    */

   LLVMValueRef inv_sig_mask = lp_build_const_v4si(bld->gallivm, ~0x80000000);
   LLVMValueRef a_v4si = LLVMBuildBitCast(b, a, v4si, "a_v4si");

   LLVMValueRef absi = LLVMBuildAnd(b, a_v4si, inv_sig_mask, "absi");
   LLVMValueRef x_abs = LLVMBuildBitCast(b, absi, v4sf, "x_abs");

   /*
    * scale by 4/Pi
    * y = _mm_mul_ps(x, *(v4sf*)_ps_cephes_FOPI);
    */
   
   LLVMValueRef FOPi = lp_build_const_v4sf(gallivm, 1.27323954473516);
   LLVMValueRef scale_y = LLVMBuildFMul(b, x_abs, FOPi, "scale_y");

   /*
    * store the integer part of y in mm0
    * emm2 = _mm_cvttps_epi32(y);
    */
   
   LLVMValueRef emm2_i = LLVMBuildFPToSI(b, scale_y, v4si, "emm2_i");

   /*
    * j=(j+1) & (~1) (see the cephes sources)
    * emm2 = _mm_add_epi32(emm2, *(v4si*)_pi32_1);
    */

   LLVMValueRef all_one = lp_build_const_v4si(bld->gallivm, 1);
   LLVMValueRef emm2_add =  LLVMBuildAdd(b, emm2_i, all_one, "emm2_add");
   /*
    * emm2 = _mm_and_si128(emm2, *(v4si*)_pi32_inv1);
    */
   LLVMValueRef inv_one = lp_build_const_v4si(bld->gallivm, ~1);
   LLVMValueRef emm2_and =  LLVMBuildAnd(b, emm2_add, inv_one, "emm2_and");

   /*
    * y = _mm_cvtepi32_ps(emm2);
    */
   LLVMValueRef y_2 = LLVMBuildSIToFP(b, emm2_and, v4sf, "y_2");


   /*
    * emm2 = _mm_sub_epi32(emm2, *(v4si*)_pi32_2);
    */
   LLVMValueRef const_2 = lp_build_const_v4si(bld->gallivm, 2);
   LLVMValueRef emm2_2 = LLVMBuildSub(b, emm2_and, const_2, "emm2_2");


   /* get the swap sign flag
    * emm0 = _mm_andnot_si128(emm2, *(v4si*)_pi32_4);
    */
   LLVMValueRef inv = lp_build_const_v4si(bld->gallivm, ~0);
   LLVMValueRef emm0_not = LLVMBuildXor(b, emm2_2, inv, "emm0_not");
   LLVMValueRef pi32_4 = lp_build_const_v4si(bld->gallivm, 4);
   LLVMValueRef emm0_and =  LLVMBuildAnd(b, emm0_not, pi32_4, "emm0_and");
   
   /*
    * emm2 = _mm_slli_epi32(emm0, 29);
    */  
   LLVMValueRef const_29 = lp_build_const_v4si(bld->gallivm, 29);
   LLVMValueRef sign_bit = LLVMBuildShl(b, emm0_and, const_29, "sign_bit");

   /*
    * get the polynom selection mask 
    * there is one polynom for 0 <= x <= Pi/4
    * and another one for Pi/4<x<=Pi/2
    * Both branches will be computed.
    *  
    * emm2 = _mm_and_si128(emm2, *(v4si*)_pi32_2);
    * emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());
    */

   LLVMValueRef pi32_2 = lp_build_const_v4si(bld->gallivm, 2);
   LLVMValueRef emm2_3 =  LLVMBuildAnd(b, emm2_2, pi32_2, "emm2_3");
   LLVMValueRef poly_mask = lp_build_compare(bld->gallivm,
                                             int_type, PIPE_FUNC_EQUAL,
   				             emm2_3, lp_build_const_v4si(bld->gallivm, 0));

   /*
    * _PS_CONST(minus_cephes_DP1, -0.78515625);
    * _PS_CONST(minus_cephes_DP2, -2.4187564849853515625e-4);
    * _PS_CONST(minus_cephes_DP3, -3.77489497744594108e-8);
    */
   LLVMValueRef DP1 = lp_build_const_v4sf(gallivm, -0.78515625);
   LLVMValueRef DP2 = lp_build_const_v4sf(gallivm, -2.4187564849853515625e-4);
   LLVMValueRef DP3 = lp_build_const_v4sf(gallivm, -3.77489497744594108e-8);

   /*
    * The magic pass: "Extended precision modular arithmetic" 
    * x = ((x - y * DP1) - y * DP2) - y * DP3; 
    * xmm1 = _mm_mul_ps(y, xmm1);
    * xmm2 = _mm_mul_ps(y, xmm2);
    * xmm3 = _mm_mul_ps(y, xmm3);
    */
   LLVMValueRef xmm1 = LLVMBuildFMul(b, y_2, DP1, "xmm1");
   LLVMValueRef xmm2 = LLVMBuildFMul(b, y_2, DP2, "xmm2");
   LLVMValueRef xmm3 = LLVMBuildFMul(b, y_2, DP3, "xmm3");

   /*
    * x = _mm_add_ps(x, xmm1);
    * x = _mm_add_ps(x, xmm2);
    * x = _mm_add_ps(x, xmm3);
    */ 

   LLVMValueRef x_1 = LLVMBuildFAdd(b, x_abs, xmm1, "x_1");
   LLVMValueRef x_2 = LLVMBuildFAdd(b, x_1, xmm2, "x_2");
   LLVMValueRef x_3 = LLVMBuildFAdd(b, x_2, xmm3, "x_3");

   /*
    * Evaluate the first polynom  (0 <= x <= Pi/4)
    *
    * z = _mm_mul_ps(x,x);
    */
   LLVMValueRef z = LLVMBuildFMul(b, x_3, x_3, "z");

   /*
    * _PS_CONST(coscof_p0,  2.443315711809948E-005);
    * _PS_CONST(coscof_p1, -1.388731625493765E-003);
    * _PS_CONST(coscof_p2,  4.166664568298827E-002);
    */
   LLVMValueRef coscof_p0 = lp_build_const_v4sf(gallivm, 2.443315711809948E-005);
   LLVMValueRef coscof_p1 = lp_build_const_v4sf(gallivm, -1.388731625493765E-003);
   LLVMValueRef coscof_p2 = lp_build_const_v4sf(gallivm, 4.166664568298827E-002);

   /*
    * y = *(v4sf*)_ps_coscof_p0;
    * y = _mm_mul_ps(y, z);
    */
   LLVMValueRef y_3 = LLVMBuildFMul(b, z, coscof_p0, "y_3");
   LLVMValueRef y_4 = LLVMBuildFAdd(b, y_3, coscof_p1, "y_4");
   LLVMValueRef y_5 = LLVMBuildFMul(b, y_4, z, "y_5");
   LLVMValueRef y_6 = LLVMBuildFAdd(b, y_5, coscof_p2, "y_6");
   LLVMValueRef y_7 = LLVMBuildFMul(b, y_6, z, "y_7");
   LLVMValueRef y_8 = LLVMBuildFMul(b, y_7, z, "y_8");


   /*
    * tmp = _mm_mul_ps(z, *(v4sf*)_ps_0p5);
    * y = _mm_sub_ps(y, tmp);
    * y = _mm_add_ps(y, *(v4sf*)_ps_1);
    */ 
   LLVMValueRef half = lp_build_const_v4sf(gallivm, 0.5);
   LLVMValueRef tmp = LLVMBuildFMul(b, z, half, "tmp");
   LLVMValueRef y_9 = LLVMBuildFSub(b, y_8, tmp, "y_8");
   LLVMValueRef one = lp_build_const_v4sf(gallivm, 1.0);
   LLVMValueRef y_10 = LLVMBuildFAdd(b, y_9, one, "y_9");

   /*
    * _PS_CONST(sincof_p0, -1.9515295891E-4);
    * _PS_CONST(sincof_p1,  8.3321608736E-3);
    * _PS_CONST(sincof_p2, -1.6666654611E-1);
    */
   LLVMValueRef sincof_p0 = lp_build_const_v4sf(gallivm, -1.9515295891E-4);
   LLVMValueRef sincof_p1 = lp_build_const_v4sf(gallivm, 8.3321608736E-3);
   LLVMValueRef sincof_p2 = lp_build_const_v4sf(gallivm, -1.6666654611E-1);

   /*
    * Evaluate the second polynom  (Pi/4 <= x <= 0)
    *
    * y2 = *(v4sf*)_ps_sincof_p0;
    * y2 = _mm_mul_ps(y2, z);
    * y2 = _mm_add_ps(y2, *(v4sf*)_ps_sincof_p1);
    * y2 = _mm_mul_ps(y2, z);
    * y2 = _mm_add_ps(y2, *(v4sf*)_ps_sincof_p2);
    * y2 = _mm_mul_ps(y2, z);
    * y2 = _mm_mul_ps(y2, x);
    * y2 = _mm_add_ps(y2, x);
    */

   LLVMValueRef y2_3 = LLVMBuildFMul(b, z, sincof_p0, "y2_3");
   LLVMValueRef y2_4 = LLVMBuildFAdd(b, y2_3, sincof_p1, "y2_4");
   LLVMValueRef y2_5 = LLVMBuildFMul(b, y2_4, z, "y2_5");
   LLVMValueRef y2_6 = LLVMBuildFAdd(b, y2_5, sincof_p2, "y2_6");
   LLVMValueRef y2_7 = LLVMBuildFMul(b, y2_6, z, "y2_7");
   LLVMValueRef y2_8 = LLVMBuildFMul(b, y2_7, x_3, "y2_8");
   LLVMValueRef y2_9 = LLVMBuildFAdd(b, y2_8, x_3, "y2_9");

   /*
    * select the correct result from the two polynoms
    * xmm3 = poly_mask;
    * y2 = _mm_and_ps(xmm3, y2); //, xmm3);
    * y = _mm_andnot_ps(xmm3, y);
    * y = _mm_add_ps(y,y2);
    */
   LLVMValueRef y2_i = LLVMBuildBitCast(b, y2_9, v4si, "y2_i");
   LLVMValueRef y_i = LLVMBuildBitCast(b, y_10, v4si, "y_i");
   LLVMValueRef y2_and = LLVMBuildAnd(b, y2_i, poly_mask, "y2_and");
   LLVMValueRef poly_mask_inv = LLVMBuildXor(b, poly_mask, inv, "poly_mask_inv");
   LLVMValueRef y_and = LLVMBuildAnd(b, y_i, poly_mask_inv, "y_and");
   LLVMValueRef y_combine = LLVMBuildAdd(b, y_and, y2_and, "y_combine");

   /*
    * update the sign
    * y = _mm_xor_ps(y, sign_bit);
    */
   LLVMValueRef y_sign = LLVMBuildXor(b, y_combine, sign_bit, "y_sin");
   LLVMValueRef y_result = LLVMBuildBitCast(b, y_sign, v4sf, "y_result");
   return y_result;
}


/**
 * Generate pow(x, y)
 */
LLVMValueRef
lp_build_pow(struct lp_build_context *bld,
             LLVMValueRef x,
             LLVMValueRef y)
{
   /* TODO: optimize the constant case */
   if (gallivm_debug & GALLIVM_DEBUG_PERF &&
       LLVMIsConstant(x) && LLVMIsConstant(y)) {
      debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                   __FUNCTION__);
   }

   return lp_build_exp2(bld, lp_build_mul(bld, lp_build_log2(bld, x), y));
}


/**
 * Generate exp(x)
 */
LLVMValueRef
lp_build_exp(struct lp_build_context *bld,
             LLVMValueRef x)
{
   /* log2(e) = 1/log(2) */
   LLVMValueRef log2e = lp_build_const_vec(bld->gallivm, bld->type,
                                           1.4426950408889634);

   assert(lp_check_value(bld->type, x));

   return lp_build_mul(bld, log2e, lp_build_exp2(bld, x));
}


/**
 * Generate log(x)
 */
LLVMValueRef
lp_build_log(struct lp_build_context *bld,
             LLVMValueRef x)
{
   /* log(2) */
   LLVMValueRef log2 = lp_build_const_vec(bld->gallivm, bld->type,
                                          0.69314718055994529);

   assert(lp_check_value(bld->type, x));

   return lp_build_mul(bld, log2, lp_build_exp2(bld, x));
}


/**
 * Generate polynomial.
 * Ex:  coeffs[0] + x * coeffs[1] + x^2 * coeffs[2].
 */
static LLVMValueRef
lp_build_polynomial(struct lp_build_context *bld,
                    LLVMValueRef x,
                    const double *coeffs,
                    unsigned num_coeffs)
{
   const struct lp_type type = bld->type;
   LLVMValueRef res = NULL;
   unsigned i;

   assert(lp_check_value(bld->type, x));

   /* TODO: optimize the constant case */
   if (gallivm_debug & GALLIVM_DEBUG_PERF &&
       LLVMIsConstant(x)) {
      debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                   __FUNCTION__);
   }

   for (i = num_coeffs; i--; ) {
      LLVMValueRef coeff;

      coeff = lp_build_const_vec(bld->gallivm, type, coeffs[i]);

      if(res)
         res = lp_build_add(bld, coeff, lp_build_mul(bld, x, res));
      else
         res = coeff;
   }

   if(res)
      return res;
   else
      return bld->undef;
}


/**
 * Minimax polynomial fit of 2**x, in range [0, 1[
 */
const double lp_build_exp2_polynomial[] = {
#if EXP_POLY_DEGREE == 5
   0.999999999690134838155,
   0.583974334321735217258,
   0.164553105719676828492,
   0.0292811063701710962255,
   0.00354944426657875141846,
   0.000296253726543423377365
#elif EXP_POLY_DEGREE == 4
   1.00000001502262084505,
   0.563586057338685991394,
   0.150436017652442413623,
   0.0243220604213317927308,
   0.0025359088446580436489
#elif EXP_POLY_DEGREE == 3
   0.999925218562710312959,
   0.695833540494823811697,
   0.226067155427249155588,
   0.0780245226406372992967
#elif EXP_POLY_DEGREE == 2
   1.00172476321474503578,
   0.657636275736077639316,
   0.33718943461968720704
#else
#error
#endif
};


void
lp_build_exp2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp2_int_part,
                     LLVMValueRef *p_frac_part,
                     LLVMValueRef *p_exp2)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);
   LLVMValueRef ipart = NULL;
   LLVMValueRef fpart = NULL;
   LLVMValueRef expipart = NULL;
   LLVMValueRef expfpart = NULL;
   LLVMValueRef res = NULL;

   assert(lp_check_value(bld->type, x));

   if(p_exp2_int_part || p_frac_part || p_exp2) {
      /* TODO: optimize the constant case */
      if (gallivm_debug & GALLIVM_DEBUG_PERF &&
          LLVMIsConstant(x)) {
         debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                      __FUNCTION__);
      }

      assert(type.floating && type.width == 32);

      x = lp_build_min(bld, x, lp_build_const_vec(bld->gallivm, type,  129.0));
      x = lp_build_max(bld, x, lp_build_const_vec(bld->gallivm, type, -126.99999));

      /* ipart = floor(x) */
      ipart = lp_build_floor(bld, x);

      /* fpart = x - ipart */
      fpart = LLVMBuildFSub(builder, x, ipart, "");
   }

   if(p_exp2_int_part || p_exp2) {
      /* expipart = (float) (1 << ipart) */
      ipart = LLVMBuildFPToSI(builder, ipart, int_vec_type, "");
      expipart = LLVMBuildAdd(builder, ipart,
                              lp_build_const_int_vec(bld->gallivm, type, 127), "");
      expipart = LLVMBuildShl(builder, expipart,
                              lp_build_const_int_vec(bld->gallivm, type, 23), "");
      expipart = LLVMBuildBitCast(builder, expipart, vec_type, "");
   }

   if(p_exp2) {
      expfpart = lp_build_polynomial(bld, fpart, lp_build_exp2_polynomial,
                                     Elements(lp_build_exp2_polynomial));

      res = LLVMBuildFMul(builder, expipart, expfpart, "");
   }

   if(p_exp2_int_part)
      *p_exp2_int_part = expipart;

   if(p_frac_part)
      *p_frac_part = fpart;

   if(p_exp2)
      *p_exp2 = res;
}


LLVMValueRef
lp_build_exp2(struct lp_build_context *bld,
              LLVMValueRef x)
{
   LLVMValueRef res;
   lp_build_exp2_approx(bld, x, NULL, NULL, &res);
   return res;
}


/**
 * Extract the exponent of a IEEE-754 floating point value.
 *
 * Optionally apply an integer bias.
 *
 * Result is an integer value with
 *
 *   ifloor(log2(x)) + bias
 */
LLVMValueRef
lp_build_extract_exponent(struct lp_build_context *bld,
                          LLVMValueRef x,
                          int bias)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   unsigned mantissa = lp_mantissa(type);
   LLVMValueRef res;

   assert(type.floating);

   assert(lp_check_value(bld->type, x));

   x = LLVMBuildBitCast(builder, x, bld->int_vec_type, "");

   res = LLVMBuildLShr(builder, x,
                       lp_build_const_int_vec(bld->gallivm, type, mantissa), "");
   res = LLVMBuildAnd(builder, res,
                      lp_build_const_int_vec(bld->gallivm, type, 255), "");
   res = LLVMBuildSub(builder, res,
                      lp_build_const_int_vec(bld->gallivm, type, 127 - bias), "");

   return res;
}


/**
 * Extract the mantissa of the a floating.
 *
 * Result is a floating point value with
 *
 *   x / floor(log2(x))
 */
LLVMValueRef
lp_build_extract_mantissa(struct lp_build_context *bld,
                          LLVMValueRef x)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   unsigned mantissa = lp_mantissa(type);
   LLVMValueRef mantmask = lp_build_const_int_vec(bld->gallivm, type,
                                                  (1ULL << mantissa) - 1);
   LLVMValueRef one = LLVMConstBitCast(bld->one, bld->int_vec_type);
   LLVMValueRef res;

   assert(lp_check_value(bld->type, x));

   assert(type.floating);

   x = LLVMBuildBitCast(builder, x, bld->int_vec_type, "");

   /* res = x / 2**ipart */
   res = LLVMBuildAnd(builder, x, mantmask, "");
   res = LLVMBuildOr(builder, res, one, "");
   res = LLVMBuildBitCast(builder, res, bld->vec_type, "");

   return res;
}



/**
 * Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[
 * These coefficients can be generate with
 * http://www.boost.org/doc/libs/1_36_0/libs/math/doc/sf_and_dist/html/math_toolkit/toolkit/internals2/minimax.html
 */
const double lp_build_log2_polynomial[] = {
#if LOG_POLY_DEGREE == 6
   3.11578814719469302614,
   -3.32419399085241980044,
   2.59883907202499966007,
   -1.23152682416275988241,
   0.318212422185251071475,
   -0.0344359067839062357313
#elif LOG_POLY_DEGREE == 5
   2.8882704548164776201,
   -2.52074962577807006663,
   1.48116647521213171641,
   -0.465725644288844778798,
   0.0596515482674574969533
#elif LOG_POLY_DEGREE == 4
   2.61761038894603480148,
   -1.75647175389045657003,
   0.688243882994381274313,
   -0.107254423828329604454
#elif LOG_POLY_DEGREE == 3
   2.28330284476918490682,
   -1.04913055217340124191,
   0.204446009836232697516
#else
#error
#endif
};


/**
 * See http://www.devmaster.net/forums/showthread.php?p=43580
 */
void
lp_build_log2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp,
                     LLVMValueRef *p_floor_log2,
                     LLVMValueRef *p_log2)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);

   LLVMValueRef expmask = lp_build_const_int_vec(bld->gallivm, type, 0x7f800000);
   LLVMValueRef mantmask = lp_build_const_int_vec(bld->gallivm, type, 0x007fffff);
   LLVMValueRef one = LLVMConstBitCast(bld->one, int_vec_type);

   LLVMValueRef i = NULL;
   LLVMValueRef exp = NULL;
   LLVMValueRef mant = NULL;
   LLVMValueRef logexp = NULL;
   LLVMValueRef logmant = NULL;
   LLVMValueRef res = NULL;

   assert(lp_check_value(bld->type, x));

   if(p_exp || p_floor_log2 || p_log2) {
      /* TODO: optimize the constant case */
      if (gallivm_debug & GALLIVM_DEBUG_PERF &&
          LLVMIsConstant(x)) {
         debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                      __FUNCTION__);
      }

      assert(type.floating && type.width == 32);

      i = LLVMBuildBitCast(builder, x, int_vec_type, "");

      /* exp = (float) exponent(x) */
      exp = LLVMBuildAnd(builder, i, expmask, "");
   }

   if(p_floor_log2 || p_log2) {
      logexp = LLVMBuildLShr(builder, exp, lp_build_const_int_vec(bld->gallivm, type, 23), "");
      logexp = LLVMBuildSub(builder, logexp, lp_build_const_int_vec(bld->gallivm, type, 127), "");
      logexp = LLVMBuildSIToFP(builder, logexp, vec_type, "");
   }

   if(p_log2) {
      /* mant = (float) mantissa(x) */
      mant = LLVMBuildAnd(builder, i, mantmask, "");
      mant = LLVMBuildOr(builder, mant, one, "");
      mant = LLVMBuildBitCast(builder, mant, vec_type, "");

      logmant = lp_build_polynomial(bld, mant, lp_build_log2_polynomial,
                                    Elements(lp_build_log2_polynomial));

      /* This effectively increases the polynomial degree by one, but ensures that log2(1) == 0*/
      logmant = LLVMBuildFMul(builder, logmant, LLVMBuildFSub(builder, mant, bld->one, ""), "");

      res = LLVMBuildFAdd(builder, logmant, logexp, "");
   }

   if(p_exp) {
      exp = LLVMBuildBitCast(builder, exp, vec_type, "");
      *p_exp = exp;
   }

   if(p_floor_log2)
      *p_floor_log2 = logexp;

   if(p_log2)
      *p_log2 = res;
}


LLVMValueRef
lp_build_log2(struct lp_build_context *bld,
              LLVMValueRef x)
{
   LLVMValueRef res;
   lp_build_log2_approx(bld, x, NULL, NULL, &res);
   return res;
}


/**
 * Faster (and less accurate) log2.
 *
 *    log2(x) = floor(log2(x)) - 1 + x / 2**floor(log2(x))
 *
 * Piece-wise linear approximation, with exact results when x is a
 * power of two.
 *
 * See http://www.flipcode.com/archives/Fast_log_Function.shtml
 */
LLVMValueRef
lp_build_fast_log2(struct lp_build_context *bld,
                   LLVMValueRef x)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef ipart;
   LLVMValueRef fpart;

   assert(lp_check_value(bld->type, x));

   assert(bld->type.floating);

   /* ipart = floor(log2(x)) - 1 */
   ipart = lp_build_extract_exponent(bld, x, -1);
   ipart = LLVMBuildSIToFP(builder, ipart, bld->vec_type, "");

   /* fpart = x / 2**ipart */
   fpart = lp_build_extract_mantissa(bld, x);

   /* ipart + fpart */
   return LLVMBuildFAdd(builder, ipart, fpart, "");
}


/**
 * Fast implementation of iround(log2(x)).
 *
 * Not an approximation -- it should give accurate results all the time.
 */
LLVMValueRef
lp_build_ilog2(struct lp_build_context *bld,
               LLVMValueRef x)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef sqrt2 = lp_build_const_vec(bld->gallivm, bld->type, M_SQRT2);
   LLVMValueRef ipart;

   assert(bld->type.floating);

   assert(lp_check_value(bld->type, x));

   /* x * 2^(0.5)   i.e., add 0.5 to the log2(x) */
   x = LLVMBuildFMul(builder, x, sqrt2, "");

   /* ipart = floor(log2(x) + 0.5)  */
   ipart = lp_build_extract_exponent(bld, x, 0);

   return ipart;
}
