/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "util/u_debug.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi_parse.h"
#include "tgsi_util.h"

union pointer_hack
{
   void *pointer;
   uint64_t uint64;
};

void *
tgsi_align_128bit(
   void *unaligned )
{
   union pointer_hack ph;

   ph.uint64 = 0;
   ph.pointer = unaligned;
   ph.uint64 = (ph.uint64 + 15) & ~15;
   return ph.pointer;
}

unsigned
tgsi_util_get_src_register_swizzle(
   const struct tgsi_src_register *reg,
   unsigned component )
{
   switch( component ) {
   case 0:
      return reg->SwizzleX;
   case 1:
      return reg->SwizzleY;
   case 2:
      return reg->SwizzleZ;
   case 3:
      return reg->SwizzleW;
   default:
      assert( 0 );
   }
   return 0;
}


unsigned
tgsi_util_get_full_src_register_swizzle(
   const struct tgsi_full_src_register  *reg,
   unsigned component )
{
   return tgsi_util_get_src_register_swizzle(
      &reg->Register,
      component );
}

void
tgsi_util_set_src_register_swizzle(
   struct tgsi_src_register *reg,
   unsigned swizzle,
   unsigned component )
{
   switch( component ) {
   case 0:
      reg->SwizzleX = swizzle;
      break;
   case 1:
      reg->SwizzleY = swizzle;
      break;
   case 2:
      reg->SwizzleZ = swizzle;
      break;
   case 3:
      reg->SwizzleW = swizzle;
      break;
   default:
      assert( 0 );
   }
}

unsigned
tgsi_util_get_full_src_register_sign_mode(
   const struct  tgsi_full_src_register *reg,
   unsigned component )
{
   unsigned sign_mode;

   if( reg->Register.Absolute ) {
      /* Consider only the post-abs negation. */

      if( reg->Register.Negate ) {
         sign_mode = TGSI_UTIL_SIGN_SET;
      }
      else {
         sign_mode = TGSI_UTIL_SIGN_CLEAR;
      }
   }
   else {
      if( reg->Register.Negate ) {
         sign_mode = TGSI_UTIL_SIGN_TOGGLE;
      }
      else {
         sign_mode = TGSI_UTIL_SIGN_KEEP;
      }
   }

   return sign_mode;
}

void
tgsi_util_set_full_src_register_sign_mode(
   struct tgsi_full_src_register *reg,
   unsigned sign_mode )
{
   switch (sign_mode)
   {
   case TGSI_UTIL_SIGN_CLEAR:
      reg->Register.Negate = 0;
      reg->Register.Absolute = 1;
      break;

   case TGSI_UTIL_SIGN_SET:
      reg->Register.Absolute = 1;
      reg->Register.Negate = 1;
      break;

   case TGSI_UTIL_SIGN_TOGGLE:
      reg->Register.Negate = 1;
      reg->Register.Absolute = 0;
      break;

   case TGSI_UTIL_SIGN_KEEP:
      reg->Register.Negate = 0;
      reg->Register.Absolute = 0;
      break;

   default:
      assert( 0 );
   }
}

/**
 * Determine which channels of the specificed src register are effectively
 * used by this instruction.
 */
unsigned
tgsi_util_get_inst_usage_mask(const struct tgsi_full_instruction *inst,
                              unsigned src_idx)
{
   const struct tgsi_full_src_register *src = &inst->Src[src_idx];
   unsigned write_mask = inst->Dst[0].Register.WriteMask;
   unsigned read_mask;
   unsigned usage_mask;
   unsigned chan;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_ARL:
   case TGSI_OPCODE_ARR:
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_DIV:
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MAX:
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGE:
   case TGSI_OPCODE_MAD:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_LRP:
   case TGSI_OPCODE_CND:
   case TGSI_OPCODE_FRC:
   case TGSI_OPCODE_CEIL:
   case TGSI_OPCODE_CLAMP:
   case TGSI_OPCODE_FLR:
   case TGSI_OPCODE_ROUND:
   case TGSI_OPCODE_POW:
   case TGSI_OPCODE_ABS:
   case TGSI_OPCODE_COS:
   case TGSI_OPCODE_SIN:
   case TGSI_OPCODE_DDX:
   case TGSI_OPCODE_DDY:
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_SSG:
   case TGSI_OPCODE_CMP:
   case TGSI_OPCODE_TRUNC:
   case TGSI_OPCODE_NOT:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_XOR:
   case TGSI_OPCODE_SAD:
      /* Channel-wise operations */
      read_mask = write_mask;
      break;

   case TGSI_OPCODE_EX2:
   case TGSI_OPCODE_LG2:
   case TGSI_OPCODE_RCC:
      read_mask = TGSI_WRITEMASK_X;
      break;

   case TGSI_OPCODE_SCS:
      read_mask = write_mask & TGSI_WRITEMASK_XY ? TGSI_WRITEMASK_X : 0;
      break;

   case TGSI_OPCODE_EXP:
   case TGSI_OPCODE_LOG:
      read_mask = write_mask & TGSI_WRITEMASK_XYZ ? TGSI_WRITEMASK_X : 0;
      break;

   case TGSI_OPCODE_DP2A:
      read_mask = src_idx == 2 ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_XY;
      break;

   case TGSI_OPCODE_DP2:
      read_mask = TGSI_WRITEMASK_XY;
      break;

   case TGSI_OPCODE_DP3:
      read_mask = TGSI_WRITEMASK_XYZ;
      break;

   case TGSI_OPCODE_DP4:
      read_mask = TGSI_WRITEMASK_XYZW;
      break;

   case TGSI_OPCODE_DPH:
      read_mask = src_idx == 0 ? TGSI_WRITEMASK_XYZ : TGSI_WRITEMASK_XYZW;
      break;

   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXD:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXP:
      if (src_idx == 0) {
         /* Note that the SHADOW variants use the Z component too */
         switch (inst->Texture.Texture) {
         case TGSI_TEXTURE_1D:
            read_mask = TGSI_WRITEMASK_X;
            break;
         case TGSI_TEXTURE_SHADOW1D:
            read_mask = TGSI_WRITEMASK_XZ;
            break;
         case TGSI_TEXTURE_2D:
         case TGSI_TEXTURE_RECT:
            read_mask = TGSI_WRITEMASK_XY;
            break;
         case TGSI_TEXTURE_SHADOW2D:
         case TGSI_TEXTURE_SHADOWRECT:
         case TGSI_TEXTURE_3D:
         case TGSI_TEXTURE_CUBE:
            read_mask = TGSI_WRITEMASK_XYZ;
            break;
         case TGSI_TEXTURE_1D_ARRAY:
            read_mask = TGSI_WRITEMASK_XY;
            break;
         case TGSI_TEXTURE_2D_ARRAY:
            read_mask = TGSI_WRITEMASK_XYZ;
            break;
         default:
            assert(0);
            read_mask = 0;
         }

         if (inst->Instruction.Opcode != TGSI_OPCODE_TEX) {
            read_mask |= TGSI_WRITEMASK_W;
         }
      } else {
         /* A safe approximation */
         read_mask = TGSI_WRITEMASK_XYZW;
      }
      break;

   default:
      /* Assume all channels are read */
      read_mask = TGSI_WRITEMASK_XYZW;
      break;
   }

   usage_mask = 0;
   for (chan = 0; chan < 4; ++chan) {
      if (read_mask & (1 << chan)) {
         usage_mask |= 1 << tgsi_util_get_full_src_register_swizzle(src, chan);
      }
   }

   return usage_mask;
}
