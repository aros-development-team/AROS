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


#ifndef LP_BLD_STRUCT_H
#define LP_BLD_STRUCT_H


#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_init.h"

#include "util/u_debug.h"
#include "util/u_memory.h"


#define LP_CHECK_STRUCT_SIZE(_ctype, _ltarget, _ltype) \
      assert(LLVMABISizeOfType(_ltarget, _ltype) == \
             sizeof(_ctype))

#define LP_CHECK_MEMBER_OFFSET(_ctype, _cmember, _ltarget, _ltype, _lindex) \
      assert(LLVMOffsetOfElement(_ltarget, _ltype, _lindex) == \
             offsetof(_ctype, _cmember))


/**
 * Get value pointer to a structure member.
 */
LLVMValueRef
lp_build_struct_get_ptr(struct gallivm_state *gallivm,
                        LLVMValueRef ptr,
                        unsigned member,
                        const char *name);

/**
 * Get the value of a structure member.
 */
LLVMValueRef
lp_build_struct_get(struct gallivm_state *gallivm,
                    LLVMValueRef ptr,
                    unsigned member,
                    const char *name);

/**
 * Get value pointer to an array element.
 */
LLVMValueRef
lp_build_array_get_ptr(struct gallivm_state *gallivm,
                       LLVMValueRef ptr,
                       LLVMValueRef index);

/**
 * Get the value of an array element.
 */
LLVMValueRef
lp_build_array_get(struct gallivm_state *gallivm,
                   LLVMValueRef ptr,
                   LLVMValueRef index);

/**
 * Set the value of an array element.
 */
void
lp_build_array_set(struct gallivm_state *gallivm,
                   LLVMValueRef ptr,
                   LLVMValueRef index,
                   LLVMValueRef value);

/**
 * Get the value of an array element.
 */
LLVMValueRef
lp_build_pointer_get(LLVMBuilderRef builder,
                   LLVMValueRef ptr,
                   LLVMValueRef index);

/**
 * Set the value of an array element.
 */
void
lp_build_pointer_set(LLVMBuilderRef builder,
                     LLVMValueRef ptr,
                     LLVMValueRef index,
                     LLVMValueRef value);

#endif /* !LP_BLD_STRUCT_H */
