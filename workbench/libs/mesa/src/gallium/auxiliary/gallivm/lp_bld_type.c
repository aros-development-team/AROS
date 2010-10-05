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


#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"


LLVMTypeRef
lp_build_elem_type(struct lp_type type)
{
   if (type.floating) {
      switch(type.width) {
      case 32:
         return LLVMFloatType();
         break;
      case 64:
         return LLVMDoubleType();
         break;
      default:
         assert(0);
         return LLVMFloatType();
      }
   }
   else {
      return LLVMIntType(type.width);
   }
}


LLVMTypeRef
lp_build_vec_type(struct lp_type type)
{
   LLVMTypeRef elem_type = lp_build_elem_type(type);
   if (type.length == 1)
      return elem_type;
   else
      return LLVMVectorType(elem_type, type.length);
}


/**
 * This function is a mirror of lp_build_elem_type() above.
 *
 * XXX: I'm not sure if it wouldn't be easier/efficient to just recreate the
 * type and check for identity.
 */
boolean
lp_check_elem_type(struct lp_type type, LLVMTypeRef elem_type) 
{
   LLVMTypeKind elem_kind;

   assert(elem_type);
   if(!elem_type)
      return FALSE;

   elem_kind = LLVMGetTypeKind(elem_type);

   if (type.floating) {
      switch(type.width) {
      case 32:
         if(elem_kind != LLVMFloatTypeKind)
            return FALSE;
         break;
      case 64:
         if(elem_kind != LLVMDoubleTypeKind)
            return FALSE;
         break;
      default:
         assert(0);
         return FALSE;
      }
   }
   else {
      if(elem_kind != LLVMIntegerTypeKind)
         return FALSE;

      if(LLVMGetIntTypeWidth(elem_type) != type.width)
         return FALSE;
   }

   return TRUE; 
}


boolean
lp_check_vec_type(struct lp_type type, LLVMTypeRef vec_type) 
{
   LLVMTypeRef elem_type;

   assert(vec_type);
   if(!vec_type)
      return FALSE;

   if (type.length == 1)
      return lp_check_elem_type(type, vec_type);

   if(LLVMGetTypeKind(vec_type) != LLVMVectorTypeKind)
      return FALSE;

   if(LLVMGetVectorSize(vec_type) != type.length)
      return FALSE;

   elem_type = LLVMGetElementType(vec_type);

   return lp_check_elem_type(type, elem_type);
}


boolean
lp_check_value(struct lp_type type, LLVMValueRef val) 
{
   LLVMTypeRef vec_type;

   assert(val);
   if(!val)
      return FALSE;

   vec_type = LLVMTypeOf(val);

   return lp_check_vec_type(type, vec_type);
}


LLVMTypeRef
lp_build_int_elem_type(struct lp_type type)
{
   return LLVMIntType(type.width);
}


LLVMTypeRef
lp_build_int_vec_type(struct lp_type type)
{
   LLVMTypeRef elem_type = lp_build_int_elem_type(type);
   if (type.length == 1)
      return elem_type;
   else
      return LLVMVectorType(elem_type, type.length);
}


/**
 * Build int32[4] vector type
 */
LLVMTypeRef
lp_build_int32_vec4_type(void)
{
   struct lp_type t;
   LLVMTypeRef type;

   memset(&t, 0, sizeof(t));
   t.floating = FALSE; /* floating point values */
   t.sign = TRUE;      /* values are signed */
   t.norm = FALSE;     /* values are not limited to [0,1] or [-1,1] */
   t.width = 32;       /* 32-bit int */
   t.length = 4;       /* 4 elements per vector */

   type = lp_build_int_elem_type(t);
   return LLVMVectorType(type, t.length);
}


/**
 * Create unsigned integer type variation of given type.
 */
struct lp_type
lp_uint_type(struct lp_type type)
{
   struct lp_type res_type;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);
   memset(&res_type, 0, sizeof res_type);
   res_type.width = type.width;
   res_type.length = type.length;

   return res_type;
}


/**
 * Create signed integer type variation of given type.
 */
struct lp_type
lp_int_type(struct lp_type type)
{
   struct lp_type res_type;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);
   memset(&res_type, 0, sizeof res_type);
   res_type.width = type.width;
   res_type.length = type.length;
   res_type.sign = 1;

   return res_type;
}


/**
 * Return the type with twice the bit width (hence half the number of elements).
 */
struct lp_type
lp_wider_type(struct lp_type type)
{
   struct lp_type res_type;

   memcpy(&res_type, &type, sizeof res_type);
   res_type.width *= 2;
   res_type.length /= 2;

   assert(res_type.length);

   return res_type;
}


/**
 * Return the size of the LLVMType in bits.
 * XXX this function doesn't necessarily handle all LLVM types.
 */
unsigned
lp_sizeof_llvm_type(LLVMTypeRef t)
{
   LLVMTypeKind k = LLVMGetTypeKind(t);

   switch (k) {
   case LLVMIntegerTypeKind:
      return LLVMGetIntTypeWidth(t);
   case LLVMFloatTypeKind:
      return 8 * sizeof(float);
   case LLVMDoubleTypeKind:
      return 8 * sizeof(double);
   case LLVMVectorTypeKind:
      {
         LLVMTypeRef elem = LLVMGetElementType(t);
         unsigned len = LLVMGetVectorSize(t);
         return len * lp_sizeof_llvm_type(elem);
      }
      break;
   case LLVMArrayTypeKind:
      {
         LLVMTypeRef elem = LLVMGetElementType(t);
         unsigned len = LLVMGetArrayLength(t);
         return len * lp_sizeof_llvm_type(elem);
      }
      break;
   default:
      assert(0 && "Unexpected type in lp_get_llvm_type_size()");
      return 0;
   }
}


/**
 * Return string name for a LLVMTypeKind.  Useful for debugging.
 */
const char *
lp_typekind_name(LLVMTypeKind t)
{
   switch (t) {
   case LLVMVoidTypeKind:
      return "LLVMVoidTypeKind";
   case LLVMFloatTypeKind:
      return "LLVMFloatTypeKind";
   case LLVMDoubleTypeKind:
      return "LLVMDoubleTypeKind";
   case LLVMX86_FP80TypeKind:
      return "LLVMX86_FP80TypeKind";
   case LLVMFP128TypeKind:
      return "LLVMFP128TypeKind";
   case LLVMPPC_FP128TypeKind:
      return "LLVMPPC_FP128TypeKind";
   case LLVMLabelTypeKind:
      return "LLVMLabelTypeKind";
   case LLVMIntegerTypeKind:
      return "LLVMIntegerTypeKind";
   case LLVMFunctionTypeKind:
      return "LLVMFunctionTypeKind";
   case LLVMStructTypeKind:
      return "LLVMStructTypeKind";
   case LLVMArrayTypeKind:
      return "LLVMArrayTypeKind";
   case LLVMPointerTypeKind:
      return "LLVMPointerTypeKind";
   case LLVMOpaqueTypeKind:
      return "LLVMOpaqueTypeKind";
   case LLVMVectorTypeKind:
      return "LLVMVectorTypeKind";
   case LLVMMetadataTypeKind:
      return "LLVMMetadataTypeKind";
   /* Only in LLVM 2.7 and later???
   case LLVMUnionTypeKind:
      return "LLVMUnionTypeKind";
   */
   default:
      return "unknown LLVMTypeKind";
   }
}


/**
 * Print an LLVMTypeRef.  Like LLVMDumpValue().  For debugging.
 */
void
lp_dump_llvmtype(LLVMTypeRef t)
{
   LLVMTypeKind k = LLVMGetTypeKind(t);

   if (k == LLVMVectorTypeKind) {
      LLVMTypeRef te = LLVMGetElementType(t);
      LLVMTypeKind ke = LLVMGetTypeKind(te);
      unsigned len = LLVMGetVectorSize(t);
      if (ke == LLVMIntegerTypeKind) {
         unsigned b = LLVMGetIntTypeWidth(te);
         debug_printf("Vector [%u] of %u-bit Integer\n", len, b);
      }
      else {
         debug_printf("Vector [%u] of %s\n", len, lp_typekind_name(ke));
      }
   }
   else if (k == LLVMArrayTypeKind) {
      LLVMTypeRef te = LLVMGetElementType(t);
      LLVMTypeKind ke = LLVMGetTypeKind(te);
      unsigned len = LLVMGetArrayLength(t);
      debug_printf("Array [%u] of %s\n", len, lp_typekind_name(ke));
   }
   else if (k == LLVMIntegerTypeKind) {
      unsigned b = LLVMGetIntTypeWidth(t);
      debug_printf("%u-bit Integer\n", b);
   }
   else if (k == LLVMPointerTypeKind) {
      LLVMTypeRef te = LLVMGetElementType(t);
      debug_printf("Pointer to ");
      lp_dump_llvmtype(te);
   }
   else {
      debug_printf("%s\n", lp_typekind_name(k));
   }
}


void
lp_build_context_init(struct lp_build_context *bld,
                      LLVMBuilderRef builder,
                      struct lp_type type)
{
   bld->builder = builder;
   bld->type = type;

   bld->int_elem_type = lp_build_int_elem_type(type);
   if (type.floating)
      bld->elem_type = lp_build_elem_type(type);
   else
      bld->elem_type = bld->int_elem_type;

   if (type.length == 1) {
      bld->int_vec_type = bld->int_elem_type;
      bld->vec_type = bld->elem_type;
   }
   else {
      bld->int_vec_type = LLVMVectorType(bld->int_elem_type, type.length);
      bld->vec_type = LLVMVectorType(bld->elem_type, type.length);
   }

   bld->undef = LLVMGetUndef(bld->vec_type);
   bld->zero = LLVMConstNull(bld->vec_type);
   bld->one = lp_build_one(type);
}
