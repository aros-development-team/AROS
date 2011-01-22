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

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "lp_bld_assert.h"
#include "lp_bld_init.h"
#include "lp_bld_printf.h"


/**
 * A call to lp_build_assert() will build a function call to this function.
 */
static void
lp_assert(int condition, const char *msg)
{
   if (!condition) {
      debug_printf("LLVM assertion '%s' failed!\n", msg);
      assert(condition);
   }
}



/**
 * lp_build_assert.
 *
 * Build an assertion in LLVM IR by building a function call to the
 * lp_assert() function above.
 *
 * \param condition should be an 'i1' or 'i32' value
 * \param msg  a string to print if the assertion fails.
 */
LLVMValueRef
lp_build_assert(struct gallivm_state *gallivm,
                LLVMValueRef condition,
                const char *msg)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMContextRef context = gallivm->context;
   LLVMModuleRef module = gallivm->module;
   LLVMTypeRef arg_types[2];
   LLVMValueRef msg_string, assert_func, params[2], r;

   msg_string = lp_build_const_string_variable(module, context,
                                               msg, strlen(msg) + 1);

   arg_types[0] = LLVMInt32TypeInContext(context);
   arg_types[1] = LLVMPointerType(LLVMInt8TypeInContext(context), 0);

   /* lookup the lp_assert function */
   assert_func = LLVMGetNamedFunction(module, "lp_assert");

   /* Create the assertion function if not found */
   if (!assert_func) {
      LLVMTypeRef func_type =
         LLVMFunctionType(LLVMVoidTypeInContext(context), arg_types, 2, 0);

      assert_func = LLVMAddFunction(module, "lp_assert", func_type);
      LLVMSetFunctionCallConv(assert_func, LLVMCCallConv);
      LLVMSetLinkage(assert_func, LLVMExternalLinkage);
      LLVMAddGlobalMapping(gallivm->engine, assert_func,
                           func_to_pointer((func_pointer)lp_assert));
   }
   assert(assert_func);

   /* build function call param list */
   params[0] = LLVMBuildZExt(builder, condition, arg_types[0], "");
   params[1] = LLVMBuildBitCast(builder, msg_string, arg_types[1], "");

   /* check arg types */
   assert(LLVMTypeOf(params[0]) == arg_types[0]);
   assert(LLVMTypeOf(params[1]) == arg_types[1]);

   r = LLVMBuildCall(builder, assert_func, params, 2, "");

   return r;
}
