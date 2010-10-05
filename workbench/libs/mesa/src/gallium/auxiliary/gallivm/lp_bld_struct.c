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
 * Helper functions for manipulation structures.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"
#include "util/u_memory.h"

#include "lp_bld_debug.h"
#include "lp_bld_struct.h"


LLVMValueRef
lp_build_struct_get_ptr(LLVMBuilderRef builder,
                        LLVMValueRef ptr,
                        unsigned member,
                        const char *name)
{
   LLVMValueRef indices[2];
   LLVMValueRef member_ptr;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(ptr))) == LLVMStructTypeKind);
   indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indices[1] = LLVMConstInt(LLVMInt32Type(), member, 0);
   member_ptr = LLVMBuildGEP(builder, ptr, indices, Elements(indices), "");
   lp_build_name(member_ptr, "%s.%s_ptr", LLVMGetValueName(ptr), name);
   return member_ptr;
}


LLVMValueRef
lp_build_struct_get(LLVMBuilderRef builder,
                    LLVMValueRef ptr,
                    unsigned member,
                    const char *name)
{
   LLVMValueRef member_ptr;
   LLVMValueRef res;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(ptr))) == LLVMStructTypeKind);
   member_ptr = lp_build_struct_get_ptr(builder, ptr, member, name);
   res = LLVMBuildLoad(builder, member_ptr, "");
   lp_build_name(res, "%s.%s", LLVMGetValueName(ptr), name);
   return res;
}


LLVMValueRef
lp_build_array_get_ptr(LLVMBuilderRef builder,
                       LLVMValueRef ptr,
                       LLVMValueRef index)
{
   LLVMValueRef indices[2];
   LLVMValueRef element_ptr;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(ptr))) == LLVMArrayTypeKind);
   indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indices[1] = index;
   element_ptr = LLVMBuildGEP(builder, ptr, indices, Elements(indices), "");
#ifdef DEBUG
   lp_build_name(element_ptr, "&%s[%s]",
                 LLVMGetValueName(ptr), LLVMGetValueName(index));
#endif
   return element_ptr;
}


LLVMValueRef
lp_build_array_get(LLVMBuilderRef builder,
                   LLVMValueRef ptr,
                   LLVMValueRef index)
{
   LLVMValueRef element_ptr;
   LLVMValueRef res;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(ptr))) == LLVMArrayTypeKind);
   element_ptr = lp_build_array_get_ptr(builder, ptr, index);
   res = LLVMBuildLoad(builder, element_ptr, "");
#ifdef DEBUG
   lp_build_name(res, "%s[%s]", LLVMGetValueName(ptr), LLVMGetValueName(index));
#endif
   return res;
}


void
lp_build_array_set(LLVMBuilderRef builder,
                   LLVMValueRef ptr,
                   LLVMValueRef index,
                   LLVMValueRef value)
{
   LLVMValueRef element_ptr;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(ptr))) == LLVMArrayTypeKind);
   element_ptr = lp_build_array_get_ptr(builder, ptr, index);
   LLVMBuildStore(builder, value, element_ptr);
}


LLVMValueRef
lp_build_pointer_get(LLVMBuilderRef builder,
                     LLVMValueRef ptr,
                     LLVMValueRef index)
{
   LLVMValueRef element_ptr;
   LLVMValueRef res;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   element_ptr = LLVMBuildGEP(builder, ptr, &index, 1, "");
   res = LLVMBuildLoad(builder, element_ptr, "");
#ifdef DEBUG
   lp_build_name(res, "%s[%s]", LLVMGetValueName(ptr), LLVMGetValueName(index));
#endif
   return res;
}


void
lp_build_pointer_set(LLVMBuilderRef builder,
                     LLVMValueRef ptr,
                     LLVMValueRef index,
                     LLVMValueRef value)
{
   LLVMValueRef element_ptr;
   element_ptr = LLVMBuildGEP(builder, ptr, &index, 1, "");
   LLVMBuildStore(builder, value, element_ptr);
}
