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
 * Helper functions for constant building.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_CONST_H
#define LP_BLD_CONST_H


#include "pipe/p_compiler.h"
#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_init.h"



struct lp_type;


unsigned
lp_mantissa(struct lp_type type);


unsigned
lp_const_shift(struct lp_type type);


unsigned
lp_const_offset(struct lp_type type);


double
lp_const_scale(struct lp_type type);

double
lp_const_min(struct lp_type type);


double
lp_const_max(struct lp_type type);


double
lp_const_eps(struct lp_type type);


LLVMValueRef
lp_build_undef(struct gallivm_state *gallivm, struct lp_type type);


LLVMValueRef
lp_build_zero(struct gallivm_state *gallivm, struct lp_type type);


LLVMValueRef
lp_build_one(struct gallivm_state *gallivm, struct lp_type type);


LLVMValueRef
lp_build_const_elem(struct gallivm_state *gallivm, struct lp_type type,
                    double val);

LLVMValueRef
lp_build_const_vec(struct gallivm_state *gallivm, struct lp_type type,
                   double val);


LLVMValueRef
lp_build_const_int_vec(struct gallivm_state *gallivm,
                       struct lp_type type, long long val);


LLVMValueRef
lp_build_const_aos(struct gallivm_state *gallivm, struct lp_type type, 
                   double r, double g, double b, double a, 
                   const unsigned char *swizzle);


LLVMValueRef
lp_build_const_mask_aos(struct gallivm_state *gallivm,
                        struct lp_type type,
                        unsigned mask);


static INLINE LLVMValueRef
lp_build_const_int32(struct gallivm_state *gallivm, int i)
{
   return LLVMConstInt(LLVMInt32TypeInContext(gallivm->context), i, 0);
}


static INLINE LLVMValueRef
lp_build_const_float(struct gallivm_state *gallivm, float x)
{
   return LLVMConstReal(LLVMFloatTypeInContext(gallivm->context), x);
}


/** Return constant-valued pointer to int */
static INLINE LLVMValueRef
lp_build_const_int_pointer(struct gallivm_state *gallivm, const void *ptr)
{
   LLVMTypeRef int_type;
   LLVMValueRef v;

   /* int type large enough to hold a pointer */
   int_type = LLVMIntTypeInContext(gallivm->context, 8 * sizeof(void *));
   v = LLVMConstInt(int_type, (uintptr_t) ptr, 0);
   v = LLVMBuildIntToPtr(gallivm->builder, v,
                         LLVMPointerType(int_type, 0),
                         "cast int to ptr");
   return v;
}



#endif /* !LP_BLD_CONST_H */
