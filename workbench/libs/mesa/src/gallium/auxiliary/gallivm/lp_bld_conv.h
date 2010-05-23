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
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_CONV_H
#define LP_BLD_CONV_H


#include <llvm-c/Core.h>  


struct lp_type;


LLVMValueRef
lp_build_clamped_float_to_unsigned_norm(LLVMBuilderRef builder,
                                        struct lp_type src_type,
                                        unsigned dst_width,
                                        LLVMValueRef src);

LLVMValueRef
lp_build_unsigned_norm_to_float(LLVMBuilderRef builder,
                                unsigned src_width,
                                struct lp_type dst_type,
                                LLVMValueRef src);


void
lp_build_conv(LLVMBuilderRef builder,
              struct lp_type src_type,
              struct lp_type dst_type,
              const LLVMValueRef *srcs, unsigned num_srcs,
              LLVMValueRef *dsts, unsigned num_dsts);

void
lp_build_conv_mask(LLVMBuilderRef builder,
                   struct lp_type src_type,
                   struct lp_type dst_type,
                   const LLVMValueRef *src, unsigned num_srcs,
                   LLVMValueRef *dst, unsigned num_dsts);

#endif /* !LP_BLD_CONV_H */
