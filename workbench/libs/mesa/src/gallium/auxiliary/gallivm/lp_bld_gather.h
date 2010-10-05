/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#ifndef LP_BLD_GATHER_H_
#define LP_BLD_GATHER_H_


#include "gallivm/lp_bld.h"


LLVMValueRef
lp_build_gather_elem_ptr(LLVMBuilderRef builder,
                         unsigned length,
                         LLVMValueRef base_ptr,
                         LLVMValueRef offsets,
                         unsigned i);

LLVMValueRef
lp_build_gather_elem(LLVMBuilderRef builder,
                     unsigned length,
                     unsigned src_width,
                     unsigned dst_width,
                     LLVMValueRef base_ptr,
                     LLVMValueRef offsets,
                     unsigned i);

LLVMValueRef
lp_build_gather(LLVMBuilderRef builder,
                unsigned length,
                unsigned src_width,
                unsigned dst_width,
                LLVMValueRef base_ptr,
                LLVMValueRef offsets);


#endif /* LP_BLD_GATHER_H_ */
