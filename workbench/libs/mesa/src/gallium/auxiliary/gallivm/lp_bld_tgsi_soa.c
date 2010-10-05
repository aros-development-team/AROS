/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * TGSI to LLVM IR translation -- SoA.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 *
 * Based on tgsi_sse2.c code written by Michal Krol, Keith Whitwell,
 * Brian Paul, and others.
 */

#include "pipe/p_config.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_scan.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_gather.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_quad.h"
#include "lp_bld_tgsi.h"
#include "lp_bld_limits.h"
#include "lp_bld_debug.h"


#define FOR_EACH_CHANNEL( CHAN )\
   for (CHAN = 0; CHAN < NUM_CHANNELS; CHAN++)

#define IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   ((INST)->Dst[0].Register.WriteMask & (1 << (CHAN)))

#define IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   if (IS_DST0_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_DST0_ENABLED_CHANNEL( INST, CHAN )\
   FOR_EACH_CHANNEL( CHAN )\
      IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )

#define CHAN_X 0
#define CHAN_Y 1
#define CHAN_Z 2
#define CHAN_W 3
#define NUM_CHANNELS 4

#define LP_MAX_INSTRUCTIONS 256


struct lp_exec_mask {
   struct lp_build_context *bld;

   boolean has_mask;

   LLVMTypeRef int_vec_type;

   LLVMValueRef cond_stack[LP_MAX_TGSI_NESTING];
   int cond_stack_size;
   LLVMValueRef cond_mask;

   LLVMBasicBlockRef loop_block;
   LLVMValueRef cont_mask;
   LLVMValueRef break_mask;
   LLVMValueRef break_var;
   struct {
      LLVMBasicBlockRef loop_block;
      LLVMValueRef cont_mask;
      LLVMValueRef break_mask;
      LLVMValueRef break_var;
   } loop_stack[LP_MAX_TGSI_NESTING];
   int loop_stack_size;

   LLVMValueRef ret_mask;
   struct {
      int pc;
      LLVMValueRef ret_mask;
   } call_stack[LP_MAX_TGSI_NESTING];
   int call_stack_size;

   LLVMValueRef exec_mask;
};

struct lp_build_tgsi_soa_context
{
   struct lp_build_context base;

   /* Builder for integer masks and indices */
   struct lp_build_context int_bld;

   LLVMValueRef consts_ptr;
   const LLVMValueRef *pos;
   const LLVMValueRef (*inputs)[NUM_CHANNELS];
   LLVMValueRef (*outputs)[NUM_CHANNELS];

   const struct lp_build_sampler_soa *sampler;

   LLVMValueRef immediates[LP_MAX_TGSI_IMMEDIATES][NUM_CHANNELS];
   LLVMValueRef temps[LP_MAX_TGSI_TEMPS][NUM_CHANNELS];
   LLVMValueRef addr[LP_MAX_TGSI_ADDRS][NUM_CHANNELS];
   LLVMValueRef preds[LP_MAX_TGSI_PREDS][NUM_CHANNELS];

   /* We allocate/use this array of temps if (1 << TGSI_FILE_TEMPORARY) is
    * set in the indirect_files field.
    * The temps[] array above is unused then.
    */
   LLVMValueRef temps_array;

   /** bitmask indicating which register files are accessed indirectly */
   unsigned indirect_files;

   struct lp_build_mask_context *mask;
   struct lp_exec_mask exec_mask;

   struct tgsi_full_instruction *instructions;
   uint max_instructions;
};

static void lp_exec_mask_init(struct lp_exec_mask *mask, struct lp_build_context *bld)
{
   mask->bld = bld;
   mask->has_mask = FALSE;
   mask->cond_stack_size = 0;
   mask->loop_stack_size = 0;
   mask->call_stack_size = 0;

   mask->int_vec_type = lp_build_int_vec_type(mask->bld->type);
   mask->exec_mask = mask->ret_mask = mask->break_mask = mask->cont_mask = mask->cond_mask =
         LLVMConstAllOnes(mask->int_vec_type);
}

static void lp_exec_mask_update(struct lp_exec_mask *mask)
{
   if (mask->loop_stack_size) {
      /*for loops we need to update the entire mask at runtime */
      LLVMValueRef tmp;
      assert(mask->break_mask);
      tmp = LLVMBuildAnd(mask->bld->builder,
                         mask->cont_mask,
                         mask->break_mask,
                         "maskcb");
      mask->exec_mask = LLVMBuildAnd(mask->bld->builder,
                                     mask->cond_mask,
                                     tmp,
                                     "maskfull");
   } else
      mask->exec_mask = mask->cond_mask;

   if (mask->call_stack_size) {
      mask->exec_mask = LLVMBuildAnd(mask->bld->builder,
                                     mask->exec_mask,
                                     mask->ret_mask,
                                     "callmask");
   }

   mask->has_mask = (mask->cond_stack_size > 0 ||
                     mask->loop_stack_size > 0 ||
                     mask->call_stack_size > 0);
}

static void lp_exec_mask_cond_push(struct lp_exec_mask *mask,
                                   LLVMValueRef val)
{
   assert(mask->cond_stack_size < LP_MAX_TGSI_NESTING);
   if (mask->cond_stack_size == 0) {
      assert(mask->cond_mask == LLVMConstAllOnes(mask->int_vec_type));
   }
   mask->cond_stack[mask->cond_stack_size++] = mask->cond_mask;
   assert(LLVMTypeOf(val) == mask->int_vec_type);
   mask->cond_mask = LLVMBuildAnd(mask->bld->builder,
                                  mask->cond_mask,
                                  val,
                                  "");
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_invert(struct lp_exec_mask *mask)
{
   LLVMValueRef prev_mask;
   LLVMValueRef inv_mask;

   assert(mask->cond_stack_size);
   prev_mask = mask->cond_stack[mask->cond_stack_size - 1];
   if (mask->cond_stack_size == 1) {
      assert(prev_mask == LLVMConstAllOnes(mask->int_vec_type));
   }

   inv_mask = LLVMBuildNot(mask->bld->builder, mask->cond_mask, "");

   mask->cond_mask = LLVMBuildAnd(mask->bld->builder,
                                  inv_mask,
                                  prev_mask, "");
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_pop(struct lp_exec_mask *mask)
{
   assert(mask->cond_stack_size);
   mask->cond_mask = mask->cond_stack[--mask->cond_stack_size];
   lp_exec_mask_update(mask);
}

static void lp_exec_bgnloop(struct lp_exec_mask *mask)
{
   if (mask->loop_stack_size == 0) {
      assert(mask->loop_block == NULL);
      assert(mask->cont_mask == LLVMConstAllOnes(mask->int_vec_type));
      assert(mask->break_mask == LLVMConstAllOnes(mask->int_vec_type));
      assert(mask->break_var == NULL);
   }

   assert(mask->loop_stack_size < LP_MAX_TGSI_NESTING);

   mask->loop_stack[mask->loop_stack_size].loop_block = mask->loop_block;
   mask->loop_stack[mask->loop_stack_size].cont_mask = mask->cont_mask;
   mask->loop_stack[mask->loop_stack_size].break_mask = mask->break_mask;
   mask->loop_stack[mask->loop_stack_size].break_var = mask->break_var;
   ++mask->loop_stack_size;

   mask->break_var = lp_build_alloca(mask->bld->builder, mask->int_vec_type, "");
   LLVMBuildStore(mask->bld->builder, mask->break_mask, mask->break_var);

   mask->loop_block = lp_build_insert_new_block(mask->bld->builder, "bgnloop");
   LLVMBuildBr(mask->bld->builder, mask->loop_block);
   LLVMPositionBuilderAtEnd(mask->bld->builder, mask->loop_block);

   mask->break_mask = LLVMBuildLoad(mask->bld->builder, mask->break_var, "");

   lp_exec_mask_update(mask);
}

static void lp_exec_break(struct lp_exec_mask *mask)
{
   LLVMValueRef exec_mask = LLVMBuildNot(mask->bld->builder,
                                         mask->exec_mask,
                                         "break");

   mask->break_mask = LLVMBuildAnd(mask->bld->builder,
                                   mask->break_mask,
                                   exec_mask, "break_full");

   lp_exec_mask_update(mask);
}

static void lp_exec_continue(struct lp_exec_mask *mask)
{
   LLVMValueRef exec_mask = LLVMBuildNot(mask->bld->builder,
                                         mask->exec_mask,
                                         "");

   mask->cont_mask = LLVMBuildAnd(mask->bld->builder,
                                  mask->cont_mask,
                                  exec_mask, "");

   lp_exec_mask_update(mask);
}


static void lp_exec_endloop(struct lp_exec_mask *mask)
{
   LLVMBasicBlockRef endloop;
   LLVMTypeRef reg_type = LLVMIntType(mask->bld->type.width*
                                      mask->bld->type.length);
   LLVMValueRef i1cond;

   assert(mask->break_mask);

   /*
    * Restore the cont_mask, but don't pop
    */
   assert(mask->loop_stack_size);
   mask->cont_mask = mask->loop_stack[mask->loop_stack_size - 1].cont_mask;
   lp_exec_mask_update(mask);

   /*
    * Unlike the continue mask, the break_mask must be preserved across loop
    * iterations
    */
   LLVMBuildStore(mask->bld->builder, mask->break_mask, mask->break_var);

   /* i1cond = (mask == 0) */
   i1cond = LLVMBuildICmp(
      mask->bld->builder,
      LLVMIntNE,
      LLVMBuildBitCast(mask->bld->builder, mask->exec_mask, reg_type, ""),
      LLVMConstNull(reg_type), "");

   endloop = lp_build_insert_new_block(mask->bld->builder, "endloop");

   LLVMBuildCondBr(mask->bld->builder,
                   i1cond, mask->loop_block, endloop);

   LLVMPositionBuilderAtEnd(mask->bld->builder, endloop);

   assert(mask->loop_stack_size);
   --mask->loop_stack_size;
   mask->loop_block = mask->loop_stack[mask->loop_stack_size].loop_block;
   mask->cont_mask = mask->loop_stack[mask->loop_stack_size].cont_mask;
   mask->break_mask = mask->loop_stack[mask->loop_stack_size].break_mask;
   mask->break_var = mask->loop_stack[mask->loop_stack_size].break_var;

   lp_exec_mask_update(mask);
}

/* stores val into an address pointed to by dst.
 * mask->exec_mask is used to figure out which bits of val
 * should be stored into the address
 * (0 means don't store this bit, 1 means do store).
 */
static void lp_exec_mask_store(struct lp_exec_mask *mask,
                               LLVMValueRef pred,
                               LLVMValueRef val,
                               LLVMValueRef dst)
{
   /* Mix the predicate and execution mask */
   if (mask->has_mask) {
      if (pred) {
         pred = LLVMBuildAnd(mask->bld->builder, pred, mask->exec_mask, "");
      } else {
         pred = mask->exec_mask;
      }
   }

   if (pred) {
      LLVMValueRef real_val, dst_val;

      dst_val = LLVMBuildLoad(mask->bld->builder, dst, "");
      real_val = lp_build_select(mask->bld,
                                 pred,
                                 val, dst_val);

      LLVMBuildStore(mask->bld->builder, real_val, dst);
   } else
      LLVMBuildStore(mask->bld->builder, val, dst);
}

static void lp_exec_mask_call(struct lp_exec_mask *mask,
                              int func,
                              int *pc)
{
   assert(mask->call_stack_size < LP_MAX_TGSI_NESTING);
   mask->call_stack[mask->call_stack_size].pc = *pc;
   mask->call_stack[mask->call_stack_size].ret_mask = mask->ret_mask;
   mask->call_stack_size++;
   *pc = func;
}

static void lp_exec_mask_ret(struct lp_exec_mask *mask, int *pc)
{
   LLVMValueRef exec_mask;

   if (mask->call_stack_size == 0) {
      /* returning from main() */
      *pc = -1;
      return;
   }
   exec_mask = LLVMBuildNot(mask->bld->builder,
                            mask->exec_mask,
                            "ret");

   mask->ret_mask = LLVMBuildAnd(mask->bld->builder,
                                 mask->ret_mask,
                                 exec_mask, "ret_full");

   lp_exec_mask_update(mask);
}

static void lp_exec_mask_bgnsub(struct lp_exec_mask *mask)
{
}

static void lp_exec_mask_endsub(struct lp_exec_mask *mask, int *pc)
{
   assert(mask->call_stack_size);
   mask->call_stack_size--;
   *pc = mask->call_stack[mask->call_stack_size].pc;
   mask->ret_mask = mask->call_stack[mask->call_stack_size].ret_mask;
   lp_exec_mask_update(mask);
}


/**
 * Return pointer to a temporary register channel (src or dest).
 * Note that indirect addressing cannot be handled here.
 * \param index  which temporary register
 * \param chan  which channel of the temp register.
 */
static LLVMValueRef
get_temp_ptr(struct lp_build_tgsi_soa_context *bld,
             unsigned index,
             unsigned chan)
{
   assert(chan < 4);
   if (bld->indirect_files & (1 << TGSI_FILE_TEMPORARY)) {
      LLVMValueRef lindex = lp_build_const_int32(index * 4 + chan);
      return LLVMBuildGEP(bld->base.builder, bld->temps_array, &lindex, 1, "");
   }
   else {
      return bld->temps[index][chan];
   }
}


/**
 * Gather vector.
 * XXX the lp_build_gather() function should be capable of doing this
 * with a little work.
 */
static LLVMValueRef
build_gather(struct lp_build_tgsi_soa_context *bld,
             LLVMValueRef base_ptr,
             LLVMValueRef indexes)
{
   LLVMValueRef res = bld->base.undef;
   unsigned i;

   /*
    * Loop over elements of index_vec, load scalar value, insert it into 'res'.
    */
   for (i = 0; i < bld->base.type.length; i++) {
      LLVMValueRef ii = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef index = LLVMBuildExtractElement(bld->base.builder,
                                                   indexes, ii, "");
      LLVMValueRef scalar_ptr = LLVMBuildGEP(bld->base.builder, base_ptr,
                                             &index, 1, "");
      LLVMValueRef scalar = LLVMBuildLoad(bld->base.builder, scalar_ptr, "");

      res = LLVMBuildInsertElement(bld->base.builder, res, scalar, ii, "");
   }

   return res;
}


/**
 * Read the current value of the ADDR register, convert the floats to
 * ints, multiply by four and return the vector of offsets.
 * The offsets will be used to index into the constant buffer or
 * temporary register file.
 */
static LLVMValueRef
get_indirect_offsets(struct lp_build_tgsi_soa_context *bld,
                     const struct tgsi_src_register *indirect_reg)
{
   /* always use X component of address register */
   const int x = indirect_reg->SwizzleX;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->base.type);
   uint swizzle = tgsi_util_get_src_register_swizzle(indirect_reg, x);
   LLVMValueRef vec4 = lp_build_const_int_vec(bld->int_bld.type, 4); 
   LLVMValueRef addr_vec;

   addr_vec = LLVMBuildLoad(bld->base.builder,
                            bld->addr[indirect_reg->Index][swizzle],
                            "load addr reg");

   /* for indexing we want integers */
   addr_vec = LLVMBuildFPToSI(bld->base.builder, addr_vec,
                              int_vec_type, "");

   /* addr_vec = addr_vec * 4 */
   addr_vec = lp_build_mul(&bld->int_bld, addr_vec, vec4);

   return addr_vec;
}


/**
 * Register fetch.
 */
static LLVMValueRef
emit_fetch(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned src_op,
   const unsigned chan_index )
{
   const struct tgsi_full_src_register *reg = &inst->Src[src_op];
   const unsigned swizzle =
      tgsi_util_get_full_src_register_swizzle(reg, chan_index);
   LLVMValueRef res;
   LLVMValueRef addr_vec = NULL;

   if (swizzle > 3) {
      assert(0 && "invalid swizzle in emit_fetch()");
      return bld->base.undef;
   }

   if (reg->Register.Indirect) {
      assert(bld->indirect_files);
      addr_vec = get_indirect_offsets(bld, &reg->Indirect);
   }

   switch (reg->Register.File) {
   case TGSI_FILE_CONSTANT:
      if (reg->Register.Indirect) {
         LLVMValueRef index_vec;  /* index into the const buffer */

         assert(bld->indirect_files & (1 << TGSI_FILE_CONSTANT));

         /* index_vec = broadcast(reg->Register.Index * 4 + swizzle) */
         index_vec = lp_build_const_int_vec(bld->int_bld.type,
                                            reg->Register.Index * 4 + swizzle);

         /* index_vec = index_vec + addr_vec */
         index_vec = lp_build_add(&bld->int_bld, index_vec, addr_vec);

         /* Gather values from the constant buffer */
         res = build_gather(bld, bld->consts_ptr, index_vec);
      }
      else {
         LLVMValueRef index;  /* index into the const buffer */
         LLVMValueRef scalar, scalar_ptr;

         index = lp_build_const_int32(reg->Register.Index*4 + swizzle);

         scalar_ptr = LLVMBuildGEP(bld->base.builder, bld->consts_ptr,
                                   &index, 1, "");
         scalar = LLVMBuildLoad(bld->base.builder, scalar_ptr, "");

         res = lp_build_broadcast_scalar(&bld->base, scalar);
      }
      break;

   case TGSI_FILE_IMMEDIATE:
      res = bld->immediates[reg->Register.Index][swizzle];
      assert(res);
      break;

   case TGSI_FILE_INPUT:
      res = bld->inputs[reg->Register.Index][swizzle];
      assert(res);
      break;

   case TGSI_FILE_TEMPORARY:
      if (reg->Register.Indirect) {
         LLVMValueRef vec_len =
            lp_build_const_int_vec(bld->int_bld.type, bld->base.type.length);
         LLVMValueRef index_vec;  /* index into the const buffer */
         LLVMValueRef temps_array;
         LLVMTypeRef float4_ptr_type;

         assert(bld->indirect_files & (1 << TGSI_FILE_TEMPORARY));

         /* index_vec = broadcast(reg->Register.Index * 4 + swizzle) */
         index_vec = lp_build_const_int_vec(bld->int_bld.type,
                                            reg->Register.Index * 4 + swizzle);

         /* index_vec += addr_vec */
         index_vec = lp_build_add(&bld->int_bld, index_vec, addr_vec);

         /* index_vec *= vector_length */
         index_vec = lp_build_mul(&bld->int_bld, index_vec, vec_len);

         /* cast temps_array pointer to float* */
         float4_ptr_type = LLVMPointerType(LLVMFloatType(), 0);
         temps_array = LLVMBuildBitCast(bld->int_bld.builder, bld->temps_array,
                                        float4_ptr_type, "");

         /* Gather values from the temporary register array */
         res = build_gather(bld, temps_array, index_vec);
      }
      else {
         LLVMValueRef temp_ptr;
         temp_ptr = get_temp_ptr(bld, reg->Register.Index, swizzle);
         res = LLVMBuildLoad(bld->base.builder, temp_ptr, "");
         if (!res)
            return bld->base.undef;
      }
      break;

   default:
      assert(0 && "invalid src register in emit_fetch()");
      return bld->base.undef;
   }

   switch( tgsi_util_get_full_src_register_sign_mode( reg, chan_index ) ) {
   case TGSI_UTIL_SIGN_CLEAR:
      res = lp_build_abs( &bld->base, res );
      break;

   case TGSI_UTIL_SIGN_SET:
      res = lp_build_abs( &bld->base, res );
      /* fall through */
   case TGSI_UTIL_SIGN_TOGGLE:
      res = lp_build_negate( &bld->base, res );
      break;

   case TGSI_UTIL_SIGN_KEEP:
      break;
   }

   return res;
}


/**
 * Register fetch with derivatives.
 */
static void
emit_fetch_deriv(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   const unsigned chan_index,
   LLVMValueRef *res,
   LLVMValueRef *ddx,
   LLVMValueRef *ddy)
{
   LLVMValueRef src;

   src = emit_fetch(bld, inst, index, chan_index);

   if(res)
      *res = src;

   /* TODO: use interpolation coeffs for inputs */

   if(ddx)
      *ddx = lp_build_ddx(&bld->base, src);

   if(ddy)
      *ddy = lp_build_ddy(&bld->base, src);
}


/**
 * Predicate.
 */
static void
emit_fetch_predicate(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   LLVMValueRef *pred)
{
   unsigned index;
   unsigned char swizzles[4];
   LLVMValueRef unswizzled[4] = {NULL, NULL, NULL, NULL};
   LLVMValueRef value;
   unsigned chan;

   if (!inst->Instruction.Predicate) {
      FOR_EACH_CHANNEL( chan ) {
         pred[chan] = NULL;
      }
      return;
   }

   swizzles[0] = inst->Predicate.SwizzleX;
   swizzles[1] = inst->Predicate.SwizzleY;
   swizzles[2] = inst->Predicate.SwizzleZ;
   swizzles[3] = inst->Predicate.SwizzleW;

   index = inst->Predicate.Index;
   assert(index < LP_MAX_TGSI_PREDS);

   FOR_EACH_CHANNEL( chan ) {
      unsigned swizzle = swizzles[chan];

      /*
       * Only fetch the predicate register channels that are actually listed
       * in the swizzles
       */
      if (!unswizzled[swizzle]) {
         value = LLVMBuildLoad(bld->base.builder,
                               bld->preds[index][swizzle], "");

         /*
          * Convert the value to an integer mask.
          *
          * TODO: Short-circuit this comparison -- a D3D setp_xx instructions
          * is needlessly causing two comparisons due to storing the intermediate
          * result as float vector instead of an integer mask vector.
          */
         value = lp_build_compare(bld->base.builder,
                                  bld->base.type,
                                  PIPE_FUNC_NOTEQUAL,
                                  value,
                                  bld->base.zero);
         if (inst->Predicate.Negate) {
            value = LLVMBuildNot(bld->base.builder, value, "");
         }

         unswizzled[swizzle] = value;
      } else {
         value = unswizzled[swizzle];
      }

      pred[chan] = value;
   }
}


/**
 * Register store.
 */
static void
emit_store(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   unsigned chan_index,
   LLVMValueRef pred,
   LLVMValueRef value)
{
   const struct tgsi_full_dst_register *reg = &inst->Dst[index];
   LLVMValueRef addr = NULL;

   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;

   case TGSI_SAT_ZERO_ONE:
      value = lp_build_max(&bld->base, value, bld->base.zero);
      value = lp_build_min(&bld->base, value, bld->base.one);
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      value = lp_build_max(&bld->base, value, lp_build_const_vec(bld->base.type, -1.0));
      value = lp_build_min(&bld->base, value, bld->base.one);
      break;

   default:
      assert(0);
   }

   if (reg->Register.Indirect) {
      /* XXX use get_indirect_offsets() here eventually */
      LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->base.type);
      unsigned swizzle = tgsi_util_get_src_register_swizzle( &reg->Indirect, chan_index );

      assert(bld->indirect_files);

      addr = LLVMBuildLoad(bld->base.builder,
                           bld->addr[reg->Indirect.Index][swizzle],
                           "");
      /* for indexing we want integers */
      addr = LLVMBuildFPToSI(bld->base.builder, addr,
                             int_vec_type, "");
      addr = LLVMBuildExtractElement(bld->base.builder,
                                     addr, LLVMConstInt(LLVMInt32Type(), 0, 0),
                                     "");
      addr = LLVMBuildMul(bld->base.builder,
                          addr, LLVMConstInt(LLVMInt32Type(), 4, 0),
                          "");
   }

   switch( reg->Register.File ) {
   case TGSI_FILE_OUTPUT:
      lp_exec_mask_store(&bld->exec_mask, pred, value,
                         bld->outputs[reg->Register.Index][chan_index]);
      break;

   case TGSI_FILE_TEMPORARY:
      if (reg->Register.Indirect) {
         /* XXX not done yet */
         debug_printf("WARNING: LLVM scatter store of temp regs"
                      " not implemented\n");
      }
      else {
         LLVMValueRef temp_ptr = get_temp_ptr(bld, reg->Register.Index,
                                              chan_index);
         lp_exec_mask_store(&bld->exec_mask, pred, value, temp_ptr);
      }
      break;

   case TGSI_FILE_ADDRESS:
      lp_exec_mask_store(&bld->exec_mask, pred, value,
                         bld->addr[reg->Indirect.Index][chan_index]);
      break;

   case TGSI_FILE_PREDICATE:
      lp_exec_mask_store(&bld->exec_mask, pred, value,
                         bld->preds[reg->Register.Index][chan_index]);
      break;

   default:
      assert( 0 );
   }
}


/**
 * High-level instruction translators.
 */

static void
emit_tex( struct lp_build_tgsi_soa_context *bld,
          const struct tgsi_full_instruction *inst,
          enum lp_build_tex_modifier modifier,
          LLVMValueRef *texel)
{
   unsigned unit;
   LLVMValueRef lod_bias, explicit_lod;
   LLVMValueRef oow = NULL;
   LLVMValueRef coords[3];
   LLVMValueRef ddx[3];
   LLVMValueRef ddy[3];
   unsigned num_coords;
   unsigned i;

   if (!bld->sampler) {
      _debug_printf("warning: found texture instruction but no sampler generator supplied\n");
      for (i = 0; i < 4; i++) {
         texel[i] = bld->base.undef;
      }
      return;
   }

   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D:
      num_coords = 1;
      break;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      num_coords = 2;
      break;
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      num_coords = 3;
      break;
   default:
      assert(0);
      return;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS) {
      lod_bias = emit_fetch( bld, inst, 0, 3 );
      explicit_lod = NULL;
   }
   else if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
      lod_bias = NULL;
      explicit_lod = emit_fetch( bld, inst, 0, 3 );
   }
   else {
      lod_bias = NULL;
      explicit_lod = NULL;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED) {
      oow = emit_fetch( bld, inst, 0, 3 );
      oow = lp_build_rcp(&bld->base, oow);
   }

   for (i = 0; i < num_coords; i++) {
      coords[i] = emit_fetch( bld, inst, 0, i );
      if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED)
         coords[i] = lp_build_mul(&bld->base, coords[i], oow);
   }
   for (i = num_coords; i < 3; i++) {
      coords[i] = bld->base.undef;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV) {
      for (i = 0; i < num_coords; i++) {
         ddx[i] = emit_fetch( bld, inst, 1, i );
         ddy[i] = emit_fetch( bld, inst, 2, i );
      }
      unit = inst->Src[3].Register.Index;
   }  else {
      for (i = 0; i < num_coords; i++) {
         ddx[i] = lp_build_ddx( &bld->base, coords[i] );
         ddy[i] = lp_build_ddy( &bld->base, coords[i] );
      }
      unit = inst->Src[1].Register.Index;
   }
   for (i = num_coords; i < 3; i++) {
      ddx[i] = bld->base.undef;
      ddy[i] = bld->base.undef;
   }

   bld->sampler->emit_fetch_texel(bld->sampler,
                                  bld->base.builder,
                                  bld->base.type,
                                  unit, num_coords, coords,
                                  ddx, ddy,
                                  lod_bias, explicit_lod,
                                  texel);
}


/**
 * Kill fragment if any of the src register values are negative.
 */
static void
emit_kil(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst )
{
   const struct tgsi_full_src_register *reg = &inst->Src[0];
   LLVMValueRef terms[NUM_CHANNELS];
   LLVMValueRef mask;
   unsigned chan_index;

   memset(&terms, 0, sizeof terms);

   FOR_EACH_CHANNEL( chan_index ) {
      unsigned swizzle;

      /* Unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_swizzle( reg, chan_index );

      /* Check if the component has not been already tested. */
      assert(swizzle < NUM_CHANNELS);
      if( !terms[swizzle] )
         /* TODO: change the comparison operator instead of setting the sign */
         terms[swizzle] =  emit_fetch(bld, inst, 0, chan_index );
   }

   mask = NULL;
   FOR_EACH_CHANNEL( chan_index ) {
      if(terms[chan_index]) {
         LLVMValueRef chan_mask;

         /*
          * If term < 0 then mask = 0 else mask = ~0.
          */
         chan_mask = lp_build_cmp(&bld->base, PIPE_FUNC_GEQUAL, terms[chan_index], bld->base.zero);

         if(mask)
            mask = LLVMBuildAnd(bld->base.builder, mask, chan_mask, "");
         else
            mask = chan_mask;
      }
   }

   if(mask)
      lp_build_mask_update(bld->mask, mask);
}


/**
 * Predicated fragment kill.
 * XXX Actually, we do an unconditional kill (as in tgsi_exec.c).
 * The only predication is the execution mask which will apply if
 * we're inside a loop or conditional.
 */
static void
emit_kilp(struct lp_build_tgsi_soa_context *bld,
          const struct tgsi_full_instruction *inst)
{
   LLVMValueRef mask;

   /* For those channels which are "alive", disable fragment shader
    * execution.
    */
   if (bld->exec_mask.has_mask) {
      mask = LLVMBuildNot(bld->base.builder, bld->exec_mask.exec_mask, "kilp");
   }
   else {
      mask = bld->base.zero;
   }

   lp_build_mask_update(bld->mask, mask);
}

static void
emit_declaration(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_declaration *decl)
{
   LLVMTypeRef vec_type = lp_build_vec_type(bld->base.type);

   unsigned first = decl->Range.First;
   unsigned last = decl->Range.Last;
   unsigned idx, i;

   for (idx = first; idx <= last; ++idx) {
      switch (decl->Declaration.File) {
      case TGSI_FILE_TEMPORARY:
         assert(idx < LP_MAX_TGSI_TEMPS);
         if (bld->indirect_files & (1 << TGSI_FILE_TEMPORARY)) {
            LLVMValueRef array_size = LLVMConstInt(LLVMInt32Type(),
                                                   last*4 + 4, 0);
            bld->temps_array = lp_build_array_alloca(bld->base.builder,
                                                     vec_type, array_size, "");
         } else {
            for (i = 0; i < NUM_CHANNELS; i++)
               bld->temps[idx][i] = lp_build_alloca(bld->base.builder,
                                                    vec_type, "");
         }
         break;

      case TGSI_FILE_OUTPUT:
         for (i = 0; i < NUM_CHANNELS; i++)
            bld->outputs[idx][i] = lp_build_alloca(bld->base.builder,
                                                   vec_type, "");
         break;

      case TGSI_FILE_ADDRESS:
         assert(idx < LP_MAX_TGSI_ADDRS);
         for (i = 0; i < NUM_CHANNELS; i++)
            bld->addr[idx][i] = lp_build_alloca(bld->base.builder,
                                                vec_type, "");
         break;

      case TGSI_FILE_PREDICATE:
         assert(idx < LP_MAX_TGSI_PREDS);
         for (i = 0; i < NUM_CHANNELS; i++)
            bld->preds[idx][i] = lp_build_alloca(bld->base.builder,
                                                 vec_type, "");
         break;

      default:
         /* don't need to declare other vars */
         break;
      }
   }
}


/**
 * Emit LLVM for one TGSI instruction.
 * \param return TRUE for success, FALSE otherwise
 */
static boolean
emit_instruction(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   const struct tgsi_opcode_info *info,
   int *pc)
{
   unsigned chan_index;
   LLVMValueRef src0, src1, src2;
   LLVMValueRef tmp0, tmp1, tmp2;
   LLVMValueRef tmp3 = NULL;
   LLVMValueRef tmp4 = NULL;
   LLVMValueRef tmp5 = NULL;
   LLVMValueRef tmp6 = NULL;
   LLVMValueRef tmp7 = NULL;
   LLVMValueRef res;
   LLVMValueRef dst0[NUM_CHANNELS];

   /*
    * Stores and write masks are handled in a general fashion after the long
    * instruction opcode switch statement.
    *
    * Although not stricitly necessary, we avoid generating instructions for
    * channels which won't be stored, in cases where's that easy. For some
    * complex instructions, like texture sampling, it is more convenient to
    * assume a full writemask and then let LLVM optimization passes eliminate
    * redundant code.
    */

   (*pc)++;

   assert(info->num_dst <= 1);
   if (info->num_dst) {
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = bld->base.undef;
      }
   }

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         tmp0 = lp_build_floor(&bld->base, tmp0);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_MOV:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = emit_fetch( bld, inst, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LIT:
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ) {
         dst0[CHAN_X] = bld->base.one;
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ) {
         src0 = emit_fetch( bld, inst, 0, CHAN_X );
         dst0[CHAN_Y] = lp_build_max( &bld->base, src0, bld->base.zero);
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) ) {
         /* XMM[1] = SrcReg[0].yyyy */
         tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
         /* XMM[1] = max(XMM[1], 0) */
         tmp1 = lp_build_max( &bld->base, tmp1, bld->base.zero);
         /* XMM[2] = SrcReg[0].wwww */
         tmp2 = emit_fetch( bld, inst, 0, CHAN_W );
         tmp1 = lp_build_pow( &bld->base, tmp1, tmp2);
         tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
         tmp2 = lp_build_cmp(&bld->base, PIPE_FUNC_GREATER, tmp0, bld->base.zero);
         dst0[CHAN_Z] = lp_build_select(&bld->base, tmp2, tmp1, bld->base.zero);
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) ) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_RCP:
   /* TGSI_OPCODE_RECIP */
      src0 = emit_fetch( bld, inst, 0, CHAN_X );
      res = lp_build_rcp(&bld->base, src0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = res;
      }
      break;

   case TGSI_OPCODE_RSQ:
   /* TGSI_OPCODE_RECIPSQRT */
      src0 = emit_fetch( bld, inst, 0, CHAN_X );
      src0 = lp_build_abs(&bld->base, src0);
      res = lp_build_rsqrt(&bld->base, src0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = res;
      }
      break;

   case TGSI_OPCODE_EXP:
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z )) {
         LLVMValueRef *p_exp2_int_part = NULL;
         LLVMValueRef *p_frac_part = NULL;
         LLVMValueRef *p_exp2 = NULL;

         src0 = emit_fetch( bld, inst, 0, CHAN_X );

         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            p_exp2_int_part = &tmp0;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ))
            p_frac_part = &tmp1;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            p_exp2 = &tmp2;

         lp_build_exp2_approx(&bld->base, src0, p_exp2_int_part, p_frac_part, p_exp2);

         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            dst0[CHAN_X] = tmp0;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ))
            dst0[CHAN_Y] = tmp1;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            dst0[CHAN_Z] = tmp2;
      }
      /* dst.w = 1.0 */
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_W )) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_LOG:
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z )) {
         LLVMValueRef *p_floor_log2 = NULL;
         LLVMValueRef *p_exp = NULL;
         LLVMValueRef *p_log2 = NULL;

         src0 = emit_fetch( bld, inst, 0, CHAN_X );
         src0 = lp_build_abs( &bld->base, src0 );

         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            p_floor_log2 = &tmp0;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ))
            p_exp = &tmp1;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            p_log2 = &tmp2;

         lp_build_log2_approx(&bld->base, src0, p_exp, p_floor_log2, p_log2);

         /* dst.x = floor(lg2(abs(src.x))) */
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            dst0[CHAN_X] = tmp0;
         /* dst.y = abs(src)/ex2(floor(lg2(abs(src.x)))) */
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y )) {
            dst0[CHAN_Y] = lp_build_div( &bld->base, src0, tmp1);
         }
         /* dst.z = lg2(abs(src.x)) */
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            dst0[CHAN_Z] = tmp2;
      }
      /* dst.w = 1.0 */
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_W )) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_MUL:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_mul(&bld->base, src0, src1);
      }
      break;

   case TGSI_OPCODE_ADD:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_add(&bld->base, src0, src1);
      }
      break;

   case TGSI_OPCODE_DP3:
   /* TGSI_OPCODE_DOT3 */
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Z );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Z );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_DP4:
   /* TGSI_OPCODE_DOT4 */
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Z );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Z );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_W );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_W );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_DST:
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) {
         dst0[CHAN_X] = bld->base.one;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_Y );
         tmp1 = emit_fetch( bld, inst, 1, CHAN_Y );
         dst0[CHAN_Y] = lp_build_mul( &bld->base, tmp0, tmp1);
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) {
         dst0[CHAN_Z] = emit_fetch( bld, inst, 0, CHAN_Z );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) {
         dst0[CHAN_W] = emit_fetch( bld, inst, 1, CHAN_W );
      }
      break;

   case TGSI_OPCODE_MIN:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_min( &bld->base, src0, src1 );
      }
      break;

   case TGSI_OPCODE_MAX:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_max( &bld->base, src0, src1 );
      }
      break;

   case TGSI_OPCODE_SLT:
   /* TGSI_OPCODE_SETLT */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_LESS, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SGE:
   /* TGSI_OPCODE_SETGE */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_GEQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_MAD:
   /* TGSI_OPCODE_MADD */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         tmp1 = emit_fetch( bld, inst, 1, chan_index );
         tmp2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
         tmp0 = lp_build_add( &bld->base, tmp0, tmp2);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_SUB:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         tmp1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_sub( &bld->base, tmp0, tmp1);
      }
      break;

   case TGSI_OPCODE_LRP:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_sub( &bld->base, src1, src2 );
         tmp0 = lp_build_mul( &bld->base, src0, tmp0 );
         dst0[chan_index] = lp_build_add( &bld->base, tmp0, src2 );
      }
      break;

   case TGSI_OPCODE_CND:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp1 = lp_build_const_vec(bld->base.type, 0.5);
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_GREATER, src2, tmp1);
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, src0, src1 );
      }
      break;

   case TGSI_OPCODE_DP2A:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );  /* xmm0 = src[0].x */
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );  /* xmm1 = src[1].x */
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 * xmm1 */
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );  /* xmm1 = src[0].y */
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );  /* xmm2 = src[1].y */
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);              /* xmm1 = xmm1 * xmm2 */
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 + xmm1 */
      tmp1 = emit_fetch( bld, inst, 2, CHAN_X );  /* xmm1 = src[2].x */
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 + xmm1 */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;  /* dest[ch] = xmm0 */
      }
      break;

   case TGSI_OPCODE_FRC:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         tmp0 = lp_build_floor(&bld->base, src0);
         tmp0 = lp_build_sub(&bld->base, src0, tmp0);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_CLAMP:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_max(&bld->base, tmp0, src1);
         tmp0 = lp_build_min(&bld->base, tmp0, src2);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_FLR:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_floor(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_ROUND:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_round(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_EX2: {
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_exp2( &bld->base, tmp0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;
   }

   case TGSI_OPCODE_LG2:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_log2( &bld->base, tmp0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_POW:
      src0 = emit_fetch( bld, inst, 0, CHAN_X );
      src1 = emit_fetch( bld, inst, 1, CHAN_X );
      res = lp_build_pow( &bld->base, src0, src1 );
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = res;
      }
      break;

   case TGSI_OPCODE_XPD:
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ) {
         tmp1 = emit_fetch( bld, inst, 1, CHAN_Z );
         tmp3 = emit_fetch( bld, inst, 0, CHAN_Z );
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_Y );
         tmp4 = emit_fetch( bld, inst, 1, CHAN_Y );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) {
         tmp2 = tmp0;
         tmp2 = lp_build_mul( &bld->base, tmp2, tmp1);
         tmp5 = tmp3;
         tmp5 = lp_build_mul( &bld->base, tmp5, tmp4);
         tmp2 = lp_build_sub( &bld->base, tmp2, tmp5);
         dst0[CHAN_X] = tmp2;
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) ) {
         tmp2 = emit_fetch( bld, inst, 1, CHAN_X );
         tmp5 = emit_fetch( bld, inst, 0, CHAN_X );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) {
         tmp3 = lp_build_mul( &bld->base, tmp3, tmp2);
         tmp1 = lp_build_mul( &bld->base, tmp1, tmp5);
         tmp3 = lp_build_sub( &bld->base, tmp3, tmp1);
         dst0[CHAN_Y] = tmp3;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) {
         tmp5 = lp_build_mul( &bld->base, tmp5, tmp4);
         tmp0 = lp_build_mul( &bld->base, tmp0, tmp2);
         tmp5 = lp_build_sub( &bld->base, tmp5, tmp0);
         dst0[CHAN_Z] = tmp5;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_ABS:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_abs( &bld->base, tmp0 );
      }
      break;

   case TGSI_OPCODE_RCC:
      /* deprecated? */
      assert(0);
      return FALSE;

   case TGSI_OPCODE_DPH:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Z );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Z );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 1, CHAN_W );
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_COS:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_cos( &bld->base, tmp0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_DDX:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_fetch_deriv( bld, inst, 0, chan_index, NULL, &dst0[chan_index], NULL);
      }
      break;

   case TGSI_OPCODE_DDY:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_fetch_deriv( bld, inst, 0, chan_index, NULL, NULL, &dst0[chan_index]);
      }
      break;

   case TGSI_OPCODE_KILP:
      /* predicated kill */
      emit_kilp( bld, inst );
      break;

   case TGSI_OPCODE_KIL:
      /* conditional kill */
      emit_kil( bld, inst );
      break;

   case TGSI_OPCODE_PK2H:
      return FALSE;
      break;

   case TGSI_OPCODE_PK2US:
      return FALSE;
      break;

   case TGSI_OPCODE_PK4B:
      return FALSE;
      break;

   case TGSI_OPCODE_PK4UB:
      return FALSE;
      break;

   case TGSI_OPCODE_RFL:
      return FALSE;
      break;

   case TGSI_OPCODE_SEQ:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_EQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SFL:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = bld->base.zero;
      }
      break;

   case TGSI_OPCODE_SGT:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_GREATER, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SIN:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_sin( &bld->base, tmp0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_SLE:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_LEQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SNE:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_NOTEQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_STR:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_TEX:
      emit_tex( bld, inst, LP_BLD_TEX_MODIFIER_NONE, dst0 );
      break;

   case TGSI_OPCODE_TXD:
      emit_tex( bld, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV, dst0 );
      break;

   case TGSI_OPCODE_UP2H:
      /* deprecated */
      assert (0);
      return FALSE;
      break;

   case TGSI_OPCODE_UP2US:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_UP4B:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_UP4UB:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_X2D:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_ARA:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_ARR:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         tmp0 = lp_build_round(&bld->base, tmp0);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_BRA:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_CAL:
      lp_exec_mask_call(&bld->exec_mask,
                        inst->Label.Label,
                        pc);

      break;

   case TGSI_OPCODE_RET:
      lp_exec_mask_ret(&bld->exec_mask, pc);
      break;

   case TGSI_OPCODE_END:
      *pc = -1;
      break;

   case TGSI_OPCODE_SSG:
   /* TGSI_OPCODE_SGN */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_sgn( &bld->base, tmp0 );
      }
      break;

   case TGSI_OPCODE_CMP:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_LESS, src0, bld->base.zero );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, src1, src2);
      }
      break;

   case TGSI_OPCODE_SCS:
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
         dst0[CHAN_X] = lp_build_cos( &bld->base, tmp0 );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
         dst0[CHAN_Y] = lp_build_sin( &bld->base, tmp0 );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) {
         dst0[CHAN_Z] = bld->base.zero;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_TXB:
      emit_tex( bld, inst, LP_BLD_TEX_MODIFIER_LOD_BIAS, dst0 );
      break;

   case TGSI_OPCODE_NRM:
      /* fall-through */
   case TGSI_OPCODE_NRM4:
      /* 3 or 4-component normalization */
      {
         uint dims = (inst->Instruction.Opcode == TGSI_OPCODE_NRM) ? 3 : 4;

         if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X) ||
             IS_DST0_CHANNEL_ENABLED(inst, CHAN_Y) ||
             IS_DST0_CHANNEL_ENABLED(inst, CHAN_Z) ||
             (IS_DST0_CHANNEL_ENABLED(inst, CHAN_W) && dims == 4)) {

            /* NOTE: Cannot use xmm regs 2/3 here (see emit_rsqrt() above). */

            /* xmm4 = src.x */
            /* xmm0 = src.x * src.x */
            tmp0 = emit_fetch(bld, inst, 0, CHAN_X);
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X)) {
               tmp4 = tmp0;
            }
            tmp0 = lp_build_mul( &bld->base, tmp0, tmp0);

            /* xmm5 = src.y */
            /* xmm0 = xmm0 + src.y * src.y */
            tmp1 = emit_fetch(bld, inst, 0, CHAN_Y);
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Y)) {
               tmp5 = tmp1;
            }
            tmp1 = lp_build_mul( &bld->base, tmp1, tmp1);
            tmp0 = lp_build_add( &bld->base, tmp0, tmp1);

            /* xmm6 = src.z */
            /* xmm0 = xmm0 + src.z * src.z */
            tmp1 = emit_fetch(bld, inst, 0, CHAN_Z);
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Z)) {
               tmp6 = tmp1;
            }
            tmp1 = lp_build_mul( &bld->base, tmp1, tmp1);
            tmp0 = lp_build_add( &bld->base, tmp0, tmp1);

            if (dims == 4) {
               /* xmm7 = src.w */
               /* xmm0 = xmm0 + src.w * src.w */
               tmp1 = emit_fetch(bld, inst, 0, CHAN_W);
               if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_W)) {
                  tmp7 = tmp1;
               }
               tmp1 = lp_build_mul( &bld->base, tmp1, tmp1);
               tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
            }

            /* xmm1 = 1 / sqrt(xmm0) */
            tmp1 = lp_build_rsqrt( &bld->base, tmp0);

            /* dst.x = xmm1 * src.x */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X)) {
               dst0[CHAN_X] = lp_build_mul( &bld->base, tmp4, tmp1);
            }

            /* dst.y = xmm1 * src.y */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Y)) {
               dst0[CHAN_Y] = lp_build_mul( &bld->base, tmp5, tmp1);
            }

            /* dst.z = xmm1 * src.z */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Z)) {
               dst0[CHAN_Z] = lp_build_mul( &bld->base, tmp6, tmp1);
            }

            /* dst.w = xmm1 * src.w */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X) && dims == 4) {
               dst0[CHAN_W] = lp_build_mul( &bld->base, tmp7, tmp1);
            }
         }

         /* dst.w = 1.0 */
         if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_W) && dims == 3) {
            dst0[CHAN_W] = bld->base.one;
         }
      }
      break;

   case TGSI_OPCODE_DIV:
      /* deprecated */
      assert( 0 );
      return FALSE;
      break;

   case TGSI_OPCODE_DP2:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );  /* xmm0 = src[0].x */
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );  /* xmm1 = src[1].x */
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 * xmm1 */
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );  /* xmm1 = src[0].y */
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );  /* xmm2 = src[1].y */
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);              /* xmm1 = xmm1 * xmm2 */
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 + xmm1 */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;  /* dest[ch] = xmm0 */
      }
      break;

   case TGSI_OPCODE_TXL:
      emit_tex( bld, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD, dst0 );
      break;

   case TGSI_OPCODE_TXP:
      emit_tex( bld, inst, LP_BLD_TEX_MODIFIER_PROJECTED, dst0 );
      break;

   case TGSI_OPCODE_BRK:
      lp_exec_break(&bld->exec_mask);
      break;

   case TGSI_OPCODE_IF:
      tmp0 = emit_fetch(bld, inst, 0, CHAN_X);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_NOTEQUAL,
                          tmp0, bld->base.zero);
      lp_exec_mask_cond_push(&bld->exec_mask, tmp0);
      break;

   case TGSI_OPCODE_BGNLOOP:
      lp_exec_bgnloop(&bld->exec_mask);
      break;

   case TGSI_OPCODE_BGNSUB:
      lp_exec_mask_bgnsub(&bld->exec_mask);
      break;

   case TGSI_OPCODE_ELSE:
      lp_exec_mask_cond_invert(&bld->exec_mask);
      break;

   case TGSI_OPCODE_ENDIF:
      lp_exec_mask_cond_pop(&bld->exec_mask);
      break;

   case TGSI_OPCODE_ENDLOOP:
      lp_exec_endloop(&bld->exec_mask);
      break;

   case TGSI_OPCODE_ENDSUB:
      lp_exec_mask_endsub(&bld->exec_mask, pc);
      break;

   case TGSI_OPCODE_PUSHA:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_POPA:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_CEIL:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_ceil(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_I2F:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_NOT:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_TRUNC:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_trunc(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_SHL:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_ISHR:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_AND:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_OR:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_MOD:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_XOR:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_SAD:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_TXF:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_TXQ:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_CONT:
      lp_exec_continue(&bld->exec_mask);
      break;

   case TGSI_OPCODE_EMIT:
      return FALSE;
      break;

   case TGSI_OPCODE_ENDPRIM:
      return FALSE;
      break;

   case TGSI_OPCODE_NOP:
      break;

   default:
      return FALSE;
   }
   
   if(info->num_dst) {
      LLVMValueRef pred[NUM_CHANNELS];

      emit_fetch_predicate( bld, inst, pred );

      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_store( bld, inst, 0, chan_index, pred[chan_index], dst0[chan_index]);
      }
   }

   return TRUE;
}


void
lp_build_tgsi_soa(LLVMBuilderRef builder,
                  const struct tgsi_token *tokens,
                  struct lp_type type,
                  struct lp_build_mask_context *mask,
                  LLVMValueRef consts_ptr,
                  const LLVMValueRef *pos,
                  const LLVMValueRef (*inputs)[NUM_CHANNELS],
                  LLVMValueRef (*outputs)[NUM_CHANNELS],
                  struct lp_build_sampler_soa *sampler,
                  const struct tgsi_shader_info *info)
{
   struct lp_build_tgsi_soa_context bld;
   struct tgsi_parse_context parse;
   uint num_immediates = 0;
   uint num_instructions = 0;
   unsigned i;
   int pc = 0;

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.base, builder, type);
   lp_build_context_init(&bld.int_bld, builder, lp_int_type(type));
   bld.mask = mask;
   bld.pos = pos;
   bld.inputs = inputs;
   bld.outputs = outputs;
   bld.consts_ptr = consts_ptr;
   bld.sampler = sampler;
   bld.indirect_files = info->indirect_files;
   bld.instructions = (struct tgsi_full_instruction *)
                      MALLOC( LP_MAX_INSTRUCTIONS * sizeof(struct tgsi_full_instruction) );
   bld.max_instructions = LP_MAX_INSTRUCTIONS;

   if (!bld.instructions) {
      return;
   }

   lp_exec_mask_init(&bld.exec_mask, &bld.base);

   tgsi_parse_init( &parse, tokens );

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         /* Inputs already interpolated */
         emit_declaration( &bld, &parse.FullToken.FullDeclaration );
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            /* save expanded instruction */
            if (num_instructions == bld.max_instructions) {
               struct tgsi_full_instruction *instructions;
               instructions = REALLOC(bld.instructions,
                                      bld.max_instructions
                                      * sizeof(struct tgsi_full_instruction),
                                      (bld.max_instructions + LP_MAX_INSTRUCTIONS)
                                      * sizeof(struct tgsi_full_instruction));
               if (!instructions) {
                  break;
               }
               bld.instructions = instructions;
               bld.max_instructions += LP_MAX_INSTRUCTIONS;
            }

            memcpy(bld.instructions + num_instructions,
                   &parse.FullToken.FullInstruction,
                   sizeof(bld.instructions[0]));

            num_instructions++;
         }

         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* simply copy the immediate values into the next immediates[] slot */
         {
            const uint size = parse.FullToken.FullImmediate.Immediate.NrTokens - 1;
            assert(size <= 4);
            assert(num_immediates < LP_MAX_TGSI_IMMEDIATES);
            for( i = 0; i < size; ++i )
               bld.immediates[num_immediates][i] =
                  lp_build_const_vec(type, parse.FullToken.FullImmediate.u[i].Float);
            for( i = size; i < 4; ++i )
               bld.immediates[num_immediates][i] = bld.base.undef;
            num_immediates++;
         }
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
         assert( 0 );
      }
   }

   while (pc != -1) {
      struct tgsi_full_instruction *instr = bld.instructions + pc;
      const struct tgsi_opcode_info *opcode_info =
         tgsi_get_opcode_info(instr->Instruction.Opcode);
      if (!emit_instruction( &bld, instr, opcode_info, &pc ))
         _debug_printf("warning: failed to translate tgsi opcode %s to LLVM\n",
                       opcode_info->mnemonic);
   }

   if (0) {
      LLVMBasicBlockRef block = LLVMGetInsertBlock(builder);
      LLVMValueRef function = LLVMGetBasicBlockParent(block);
      debug_printf("11111111111111111111111111111 \n");
      tgsi_dump(tokens, 0);
      lp_debug_dump_value(function);
      debug_printf("2222222222222222222222222222 \n");
   }
   tgsi_parse_free( &parse );

   if (0) {
      LLVMModuleRef module = LLVMGetGlobalParent(
         LLVMGetBasicBlockParent(LLVMGetInsertBlock(bld.base.builder)));
      LLVMDumpModule(module);

   }

   FREE( bld.instructions );
}

