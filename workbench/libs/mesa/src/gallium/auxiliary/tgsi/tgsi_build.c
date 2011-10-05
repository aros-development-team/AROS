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
#include "pipe/p_format.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi_build.h"
#include "tgsi_parse.h"


/*
 * header
 */

struct tgsi_header
tgsi_build_header( void )
{
   struct tgsi_header header;

   header.HeaderSize = 1;
   header.BodySize = 0;

   return header;
}

static void
header_headersize_grow( struct tgsi_header *header )
{
   assert( header->HeaderSize < 0xFF );
   assert( header->BodySize == 0 );

   header->HeaderSize++;
}

static void
header_bodysize_grow( struct tgsi_header *header )
{
   assert( header->BodySize < 0xFFFFFF );

   header->BodySize++;
}

struct tgsi_processor
tgsi_build_processor(
   unsigned type,
   struct tgsi_header *header )
{
   struct tgsi_processor processor;

   processor.Processor = type;
   processor.Padding = 0;

   header_headersize_grow( header );

   return processor;
}

/*
 * declaration
 */

static void
declaration_grow(
   struct tgsi_declaration *declaration,
   struct tgsi_header *header )
{
   assert( declaration->NrTokens < 0xFF );

   declaration->NrTokens++;

   header_bodysize_grow( header );
}

static struct tgsi_declaration
tgsi_default_declaration( void )
{
   struct tgsi_declaration declaration;

   declaration.Type = TGSI_TOKEN_TYPE_DECLARATION;
   declaration.NrTokens = 1;
   declaration.File = TGSI_FILE_NULL;
   declaration.UsageMask = TGSI_WRITEMASK_XYZW;
   declaration.Interpolate = TGSI_INTERPOLATE_CONSTANT;
   declaration.Dimension = 0;
   declaration.Semantic = 0;
   declaration.Centroid = 0;
   declaration.Invariant = 0;
   declaration.CylindricalWrap = 0;

   return declaration;
}

static struct tgsi_declaration
tgsi_build_declaration(
   unsigned file,
   unsigned usage_mask,
   unsigned interpolate,
   unsigned dimension,
   unsigned semantic,
   unsigned centroid,
   unsigned invariant,
   unsigned cylindrical_wrap,
   struct tgsi_header *header )
{
   struct tgsi_declaration declaration;

   assert( file < TGSI_FILE_COUNT );
   assert( interpolate < TGSI_INTERPOLATE_COUNT );

   declaration = tgsi_default_declaration();
   declaration.File = file;
   declaration.UsageMask = usage_mask;
   declaration.Interpolate = interpolate;
   declaration.Dimension = dimension;
   declaration.Semantic = semantic;
   declaration.Centroid = centroid;
   declaration.Invariant = invariant;
   declaration.CylindricalWrap = cylindrical_wrap;

   header_bodysize_grow( header );

   return declaration;
}

static struct tgsi_declaration_range
tgsi_default_declaration_range( void )
{
   struct tgsi_declaration_range dr;

   dr.First = 0;
   dr.Last = 0;

   return dr;
}

static struct tgsi_declaration_range
tgsi_build_declaration_range(
   unsigned first,
   unsigned last,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header )
{
   struct tgsi_declaration_range declaration_range;

   assert( last >= first );
   assert( last <= 0xFFFF );

   declaration_range.First = first;
   declaration_range.Last = last;

   declaration_grow( declaration, header );

   return declaration_range;
}

static struct tgsi_declaration_dimension
tgsi_build_declaration_dimension(unsigned index_2d,
                                 struct tgsi_declaration *declaration,
                                 struct tgsi_header *header)
{
   struct tgsi_declaration_dimension dd;

   assert(index_2d <= 0xFFFF);

   dd.Index2D = index_2d;
   dd.Padding = 0;

   declaration_grow(declaration, header);

   return dd;
}

static struct tgsi_declaration_semantic
tgsi_default_declaration_semantic( void )
{
   struct tgsi_declaration_semantic ds;

   ds.Name = TGSI_SEMANTIC_POSITION;
   ds.Index = 0;
   ds.Padding = 0;

   return ds;
}

static struct tgsi_declaration_semantic
tgsi_build_declaration_semantic(
   unsigned semantic_name,
   unsigned semantic_index,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header )
{
   struct tgsi_declaration_semantic ds;

   assert( semantic_name <= TGSI_SEMANTIC_COUNT );
   assert( semantic_index <= 0xFFFF );

   ds.Name = semantic_name;
   ds.Index = semantic_index;
   ds.Padding = 0;

   declaration_grow( declaration, header );

   return ds;
}


static struct tgsi_declaration_resource
tgsi_default_declaration_resource(void)
{
   struct tgsi_declaration_resource declaration_resource;

   declaration_resource.Resource = TGSI_TEXTURE_UNKNOWN;
   declaration_resource.ReturnTypeX = PIPE_TYPE_UNORM;
   declaration_resource.ReturnTypeY = PIPE_TYPE_UNORM;
   declaration_resource.ReturnTypeZ = PIPE_TYPE_UNORM;
   declaration_resource.ReturnTypeW = PIPE_TYPE_UNORM;

   return declaration_resource;
}

static struct tgsi_declaration_resource
tgsi_build_declaration_resource(unsigned texture,
                                unsigned return_type_x,
                                unsigned return_type_y,
                                unsigned return_type_z,
                                unsigned return_type_w,
                                struct tgsi_declaration *declaration,
                                struct tgsi_header *header)
{
   struct tgsi_declaration_resource declaration_resource;

   declaration_resource = tgsi_default_declaration_resource();
   declaration_resource.Resource = texture;
   declaration_resource.ReturnTypeX = return_type_x;
   declaration_resource.ReturnTypeY = return_type_y;
   declaration_resource.ReturnTypeZ = return_type_z;
   declaration_resource.ReturnTypeW = return_type_w;

   declaration_grow(declaration, header);

   return declaration_resource;
}


struct tgsi_full_declaration
tgsi_default_full_declaration( void )
{
   struct tgsi_full_declaration  full_declaration;

   full_declaration.Declaration  = tgsi_default_declaration();
   full_declaration.Range = tgsi_default_declaration_range();
   full_declaration.Semantic = tgsi_default_declaration_semantic();
   full_declaration.ImmediateData.u = NULL;
   full_declaration.Resource = tgsi_default_declaration_resource();

   return full_declaration;
}

unsigned
tgsi_build_full_declaration(
   const struct tgsi_full_declaration *full_decl,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   unsigned maxsize )
{
   unsigned size = 0;
   struct tgsi_declaration *declaration;
   struct tgsi_declaration_range *dr;

   if( maxsize <= size )
      return 0;
   declaration = (struct tgsi_declaration *) &tokens[size];
   size++;

   *declaration = tgsi_build_declaration(
      full_decl->Declaration.File,
      full_decl->Declaration.UsageMask,
      full_decl->Declaration.Interpolate,
      full_decl->Declaration.Dimension,
      full_decl->Declaration.Semantic,
      full_decl->Declaration.Centroid,
      full_decl->Declaration.Invariant,
      full_decl->Declaration.CylindricalWrap,
      header );

   if (maxsize <= size)
      return 0;
   dr = (struct tgsi_declaration_range *) &tokens[size];
   size++;

   *dr = tgsi_build_declaration_range(
      full_decl->Range.First,
      full_decl->Range.Last,
      declaration,
      header );

   if (full_decl->Declaration.Dimension) {
      struct tgsi_declaration_dimension *dd;

      if (maxsize <= size) {
         return 0;
      }
      dd = (struct tgsi_declaration_dimension *)&tokens[size];
      size++;

      *dd = tgsi_build_declaration_dimension(full_decl->Dim.Index2D,
                                             declaration,
                                             header);
   }

   if( full_decl->Declaration.Semantic ) {
      struct tgsi_declaration_semantic *ds;

      if( maxsize <= size )
         return  0;
      ds = (struct tgsi_declaration_semantic *) &tokens[size];
      size++;

      *ds = tgsi_build_declaration_semantic(
         full_decl->Semantic.Name,
         full_decl->Semantic.Index,
         declaration,
         header );
   }

   if (full_decl->Declaration.File == TGSI_FILE_IMMEDIATE_ARRAY) {
      unsigned i, j;
      union tgsi_immediate_data *data;

      for (i = 0; i <= dr->Last; ++i) {
         for (j = 0; j < 4; ++j) {
            unsigned idx = i*4 + j;
            if (maxsize <= size)
               return 0;
            data = (union tgsi_immediate_data *) &tokens[size];
            ++size;

            *data = full_decl->ImmediateData.u[idx];
            declaration_grow( declaration, header );
         }
      }
   }

   if (full_decl->Declaration.File == TGSI_FILE_RESOURCE) {
      struct tgsi_declaration_resource *dr;

      if (maxsize <= size) {
         return  0;
      }
      dr = (struct tgsi_declaration_resource *)&tokens[size];
      size++;

      *dr = tgsi_build_declaration_resource(full_decl->Resource.Resource,
                                            full_decl->Resource.ReturnTypeX,
                                            full_decl->Resource.ReturnTypeY,
                                            full_decl->Resource.ReturnTypeZ,
                                            full_decl->Resource.ReturnTypeW,
                                            declaration,
                                            header);
   }

   return size;
}

/*
 * immediate
 */

static struct tgsi_immediate
tgsi_default_immediate( void )
{
   struct tgsi_immediate immediate;

   immediate.Type = TGSI_TOKEN_TYPE_IMMEDIATE;
   immediate.NrTokens = 1;
   immediate.DataType = TGSI_IMM_FLOAT32;
   immediate.Padding = 0;

   return immediate;
}

static struct tgsi_immediate
tgsi_build_immediate(
   struct tgsi_header *header )
{
   struct tgsi_immediate immediate;

   immediate = tgsi_default_immediate();

   header_bodysize_grow( header );

   return immediate;
}

struct tgsi_full_immediate
tgsi_default_full_immediate( void )
{
   struct tgsi_full_immediate fullimm;

   fullimm.Immediate = tgsi_default_immediate();
   fullimm.u[0].Float = 0.0f;
   fullimm.u[1].Float = 0.0f;
   fullimm.u[2].Float = 0.0f;
   fullimm.u[3].Float = 0.0f;

   return fullimm;
}

static void
immediate_grow(
   struct tgsi_immediate *immediate,
   struct tgsi_header *header )
{
   assert( immediate->NrTokens < 0xFF );

   immediate->NrTokens++;

   header_bodysize_grow( header );
}

static union tgsi_immediate_data
tgsi_build_immediate_float32(
   float value,
   struct tgsi_immediate *immediate,
   struct tgsi_header *header )
{
   union tgsi_immediate_data immediate_data;

   immediate_data.Float = value;

   immediate_grow( immediate, header );

   return immediate_data;
}

unsigned
tgsi_build_full_immediate(
   const struct tgsi_full_immediate *full_imm,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   unsigned maxsize )
{
   unsigned size = 0, i;
   struct tgsi_immediate *immediate;

   if( maxsize <= size )
      return 0;
   immediate = (struct tgsi_immediate *) &tokens[size];
   size++;

   *immediate = tgsi_build_immediate( header );

   assert( full_imm->Immediate.NrTokens <= 4 + 1 );

   for( i = 0; i < full_imm->Immediate.NrTokens - 1; i++ ) {
      union tgsi_immediate_data *data;

      if( maxsize <= size )
         return  0;
      data = (union tgsi_immediate_data *) &tokens[size];
      size++;

      *data = tgsi_build_immediate_float32(
         full_imm->u[i].Float,
         immediate,
         header );
   }

   return size;
}

/*
 * instruction
 */

struct tgsi_instruction
tgsi_default_instruction( void )
{
   struct tgsi_instruction instruction;

   instruction.Type = TGSI_TOKEN_TYPE_INSTRUCTION;
   instruction.NrTokens = 0;
   instruction.Opcode = TGSI_OPCODE_MOV;
   instruction.Saturate = TGSI_SAT_NONE;
   instruction.Predicate = 0;
   instruction.NumDstRegs = 1;
   instruction.NumSrcRegs = 1;
   instruction.Label = 0;
   instruction.Texture = 0;
   instruction.Padding  = 0;

   return instruction;
}

static struct tgsi_instruction
tgsi_build_instruction(unsigned opcode,
                       unsigned saturate,
                       unsigned predicate,
                       unsigned num_dst_regs,
                       unsigned num_src_regs,
                       struct tgsi_header *header)
{
   struct tgsi_instruction instruction;

   assert (opcode <= TGSI_OPCODE_LAST);
   assert (saturate <= TGSI_SAT_MINUS_PLUS_ONE);
   assert (num_dst_regs <= 3);
   assert (num_src_regs <= 15);

   instruction = tgsi_default_instruction();
   instruction.Opcode = opcode;
   instruction.Saturate = saturate;
   instruction.Predicate = predicate;
   instruction.NumDstRegs = num_dst_regs;
   instruction.NumSrcRegs = num_src_regs;

   header_bodysize_grow( header );

   return instruction;
}

static void
instruction_grow(
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   assert (instruction->NrTokens <   0xFF);

   instruction->NrTokens++;

   header_bodysize_grow( header );
}

struct tgsi_instruction_predicate
tgsi_default_instruction_predicate(void)
{
   struct tgsi_instruction_predicate instruction_predicate;

   instruction_predicate.SwizzleX = TGSI_SWIZZLE_X;
   instruction_predicate.SwizzleY = TGSI_SWIZZLE_Y;
   instruction_predicate.SwizzleZ = TGSI_SWIZZLE_Z;
   instruction_predicate.SwizzleW = TGSI_SWIZZLE_W;
   instruction_predicate.Negate = 0;
   instruction_predicate.Index = 0;
   instruction_predicate.Padding = 0;

   return instruction_predicate;
}

static struct tgsi_instruction_predicate
tgsi_build_instruction_predicate(int index,
                                 unsigned negate,
                                 unsigned swizzleX,
                                 unsigned swizzleY,
                                 unsigned swizzleZ,
                                 unsigned swizzleW,
                                 struct tgsi_instruction *instruction,
                                 struct tgsi_header *header)
{
   struct tgsi_instruction_predicate instruction_predicate;

   instruction_predicate = tgsi_default_instruction_predicate();
   instruction_predicate.SwizzleX = swizzleX;
   instruction_predicate.SwizzleY = swizzleY;
   instruction_predicate.SwizzleZ = swizzleZ;
   instruction_predicate.SwizzleW = swizzleW;
   instruction_predicate.Negate = negate;
   instruction_predicate.Index = index;

   instruction_grow(instruction, header);

   return instruction_predicate;
}

static struct tgsi_instruction_label
tgsi_default_instruction_label( void )
{
   struct tgsi_instruction_label instruction_label;

   instruction_label.Label = 0;
   instruction_label.Padding = 0;

   return instruction_label;
}

static struct tgsi_instruction_label
tgsi_build_instruction_label(
   unsigned label,
   struct tgsi_token  *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_instruction_label instruction_label;

   instruction_label.Label = label;
   instruction_label.Padding = 0;
   instruction->Label = 1;

   instruction_grow( instruction, header );

   return instruction_label;
}

static struct tgsi_instruction_texture
tgsi_default_instruction_texture( void )
{
   struct tgsi_instruction_texture instruction_texture;

   instruction_texture.Texture = TGSI_TEXTURE_UNKNOWN;
   instruction_texture.Padding = 0;

   return instruction_texture;
}

static struct tgsi_instruction_texture
tgsi_build_instruction_texture(
   unsigned texture,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_instruction_texture instruction_texture;

   instruction_texture.Texture = texture;
   instruction_texture.Padding = 0;
   instruction->Texture = 1;

   instruction_grow( instruction, header );

   return instruction_texture;
}

static struct tgsi_src_register
tgsi_default_src_register( void )
{
   struct tgsi_src_register src_register;

   src_register.File = TGSI_FILE_NULL;
   src_register.SwizzleX = TGSI_SWIZZLE_X;
   src_register.SwizzleY = TGSI_SWIZZLE_Y;
   src_register.SwizzleZ = TGSI_SWIZZLE_Z;
   src_register.SwizzleW = TGSI_SWIZZLE_W;
   src_register.Negate = 0;
   src_register.Absolute = 0;
   src_register.Indirect = 0;
   src_register.Dimension = 0;
   src_register.Index = 0;

   return src_register;
}

static struct tgsi_src_register
tgsi_build_src_register(
   unsigned file,
   unsigned swizzle_x,
   unsigned swizzle_y,
   unsigned swizzle_z,
   unsigned swizzle_w,
   unsigned negate,
   unsigned absolute,
   unsigned indirect,
   unsigned dimension,
   int index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_src_register   src_register;

   assert( file < TGSI_FILE_COUNT );
   assert( swizzle_x <= TGSI_SWIZZLE_W );
   assert( swizzle_y <= TGSI_SWIZZLE_W );
   assert( swizzle_z <= TGSI_SWIZZLE_W );
   assert( swizzle_w <= TGSI_SWIZZLE_W );
   assert( negate <= 1 );
   assert( index >= -0x8000 && index <= 0x7FFF );

   src_register.File = file;
   src_register.SwizzleX = swizzle_x;
   src_register.SwizzleY = swizzle_y;
   src_register.SwizzleZ = swizzle_z;
   src_register.SwizzleW = swizzle_w;
   src_register.Negate = negate;
   src_register.Absolute = absolute;
   src_register.Indirect = indirect;
   src_register.Dimension = dimension;
   src_register.Index = index;

   instruction_grow( instruction, header );

   return src_register;
}

static struct tgsi_dimension
tgsi_default_dimension( void )
{
   struct tgsi_dimension dimension;

   dimension.Indirect = 0;
   dimension.Dimension = 0;
   dimension.Padding = 0;
   dimension.Index = 0;

   return dimension;
}

static struct tgsi_full_src_register
tgsi_default_full_src_register( void )
{
   struct tgsi_full_src_register full_src_register;

   full_src_register.Register = tgsi_default_src_register();
   full_src_register.Indirect = tgsi_default_src_register();
   full_src_register.Dimension = tgsi_default_dimension();
   full_src_register.DimIndirect = tgsi_default_src_register();

   return full_src_register;
}

static struct tgsi_dimension
tgsi_build_dimension(
   unsigned indirect,
   unsigned index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_dimension dimension;

   dimension.Indirect = indirect;
   dimension.Dimension = 0;
   dimension.Padding = 0;
   dimension.Index = index;

   instruction_grow( instruction, header );

   return dimension;
}

static struct tgsi_dst_register
tgsi_default_dst_register( void )
{
   struct tgsi_dst_register dst_register;

   dst_register.File = TGSI_FILE_NULL;
   dst_register.WriteMask = TGSI_WRITEMASK_XYZW;
   dst_register.Indirect = 0;
   dst_register.Dimension = 0;
   dst_register.Index = 0;
   dst_register.Padding = 0;

   return dst_register;
}

static struct tgsi_dst_register
tgsi_build_dst_register(
   unsigned file,
   unsigned mask,
   unsigned indirect,
   unsigned dimension,
   int index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_dst_register dst_register;

   assert( file < TGSI_FILE_COUNT );
   assert( mask <= TGSI_WRITEMASK_XYZW );
   assert( index >= -32768 && index <= 32767 );

   dst_register.File = file;
   dst_register.WriteMask = mask;
   dst_register.Indirect = indirect;
   dst_register.Dimension = dimension;
   dst_register.Index = index;
   dst_register.Padding = 0;

   instruction_grow( instruction, header );

   return dst_register;
}

static struct tgsi_full_dst_register
tgsi_default_full_dst_register( void )
{
   struct tgsi_full_dst_register full_dst_register;

   full_dst_register.Register = tgsi_default_dst_register();
   full_dst_register.Indirect = tgsi_default_src_register();
   full_dst_register.Dimension = tgsi_default_dimension();
   full_dst_register.DimIndirect = tgsi_default_src_register();

   return full_dst_register;
}

struct tgsi_full_instruction
tgsi_default_full_instruction( void )
{
   struct tgsi_full_instruction full_instruction;
   unsigned i;

   full_instruction.Instruction = tgsi_default_instruction();
   full_instruction.Predicate = tgsi_default_instruction_predicate();
   full_instruction.Label = tgsi_default_instruction_label();
   full_instruction.Texture = tgsi_default_instruction_texture();
   for( i = 0;  i < TGSI_FULL_MAX_DST_REGISTERS; i++ ) {
      full_instruction.Dst[i] = tgsi_default_full_dst_register();
   }
   for( i = 0;  i < TGSI_FULL_MAX_SRC_REGISTERS; i++ ) {
      full_instruction.Src[i] = tgsi_default_full_src_register();
   }

   return full_instruction;
}

unsigned
tgsi_build_full_instruction(
   const struct tgsi_full_instruction *full_inst,
   struct  tgsi_token *tokens,
   struct  tgsi_header *header,
   unsigned  maxsize )
{
   unsigned size = 0;
   unsigned i;
   struct tgsi_instruction *instruction;
   struct tgsi_token *prev_token;

   if( maxsize <= size )
      return 0;
   instruction = (struct tgsi_instruction *) &tokens[size];
   size++;

   *instruction = tgsi_build_instruction(full_inst->Instruction.Opcode,
                                         full_inst->Instruction.Saturate,
                                         full_inst->Instruction.Predicate,
                                         full_inst->Instruction.NumDstRegs,
                                         full_inst->Instruction.NumSrcRegs,
                                         header);
   prev_token = (struct tgsi_token  *) instruction;

   if (full_inst->Instruction.Predicate) {
      struct tgsi_instruction_predicate *instruction_predicate;

      if (maxsize <= size) {
         return 0;
      }
      instruction_predicate = (struct tgsi_instruction_predicate *)&tokens[size];
      size++;

      *instruction_predicate =
         tgsi_build_instruction_predicate(full_inst->Predicate.Index,
                                          full_inst->Predicate.Negate,
                                          full_inst->Predicate.SwizzleX,
                                          full_inst->Predicate.SwizzleY,
                                          full_inst->Predicate.SwizzleZ,
                                          full_inst->Predicate.SwizzleW,
                                          instruction,
                                          header);
   }

   if (full_inst->Instruction.Label) {
      struct tgsi_instruction_label *instruction_label;

      if( maxsize <= size )
         return 0;
      instruction_label =
         (struct  tgsi_instruction_label *) &tokens[size];
      size++;

      *instruction_label = tgsi_build_instruction_label(
         full_inst->Label.Label,
         prev_token,
         instruction,
         header );
      prev_token = (struct tgsi_token  *) instruction_label;
   }

   if (full_inst->Instruction.Texture) {
      struct tgsi_instruction_texture *instruction_texture;

      if( maxsize <= size )
         return 0;
      instruction_texture =
         (struct  tgsi_instruction_texture *) &tokens[size];
      size++;

      *instruction_texture = tgsi_build_instruction_texture(
         full_inst->Texture.Texture,
         prev_token,
         instruction,
         header   );
      prev_token = (struct tgsi_token  *) instruction_texture;
   }

   for( i = 0;  i <   full_inst->Instruction.NumDstRegs; i++ ) {
      const struct tgsi_full_dst_register *reg = &full_inst->Dst[i];
      struct tgsi_dst_register *dst_register;

      if( maxsize <= size )
         return 0;
      dst_register = (struct tgsi_dst_register *) &tokens[size];
      size++;

      *dst_register = tgsi_build_dst_register(
         reg->Register.File,
         reg->Register.WriteMask,
         reg->Register.Indirect,
         reg->Register.Dimension,
         reg->Register.Index,
         instruction,
         header );

      if( reg->Register.Indirect ) {
         struct tgsi_src_register *ind;

         if( maxsize <= size )
            return 0;
         ind = (struct tgsi_src_register *) &tokens[size];
         size++;

         *ind = tgsi_build_src_register(
            reg->Indirect.File,
            reg->Indirect.SwizzleX,
            reg->Indirect.SwizzleY,
            reg->Indirect.SwizzleZ,
            reg->Indirect.SwizzleW,
            reg->Indirect.Negate,
            reg->Indirect.Absolute,
            reg->Indirect.Indirect,
            reg->Indirect.Dimension,
            reg->Indirect.Index,
            instruction,
            header );
      }

      if( reg->Register.Dimension ) {
         struct  tgsi_dimension *dim;

         assert( !reg->Dimension.Dimension );

         if( maxsize <= size )
            return 0;
         dim = (struct tgsi_dimension *) &tokens[size];
         size++;

         *dim = tgsi_build_dimension(
            reg->Dimension.Indirect,
            reg->Dimension.Index,
            instruction,
            header );

         if( reg->Dimension.Indirect ) {
            struct tgsi_src_register *ind;

            if( maxsize <= size )
               return 0;
            ind = (struct tgsi_src_register *) &tokens[size];
            size++;

            *ind = tgsi_build_src_register(
               reg->DimIndirect.File,
               reg->DimIndirect.SwizzleX,
               reg->DimIndirect.SwizzleY,
               reg->DimIndirect.SwizzleZ,
               reg->DimIndirect.SwizzleW,
               reg->DimIndirect.Negate,
               reg->DimIndirect.Absolute,
               reg->DimIndirect.Indirect,
               reg->DimIndirect.Dimension,
               reg->DimIndirect.Index,
               instruction,
               header );
         }
      }
   }

   for( i = 0;  i < full_inst->Instruction.NumSrcRegs; i++ ) {
      const struct tgsi_full_src_register *reg = &full_inst->Src[i];
      struct tgsi_src_register *src_register;

      if( maxsize <= size )
         return 0;
      src_register = (struct tgsi_src_register *)  &tokens[size];
      size++;

      *src_register = tgsi_build_src_register(
         reg->Register.File,
         reg->Register.SwizzleX,
         reg->Register.SwizzleY,
         reg->Register.SwizzleZ,
         reg->Register.SwizzleW,
         reg->Register.Negate,
         reg->Register.Absolute,
         reg->Register.Indirect,
         reg->Register.Dimension,
         reg->Register.Index,
         instruction,
         header );

      if( reg->Register.Indirect ) {
         struct  tgsi_src_register *ind;

         if( maxsize <= size )
            return 0;
         ind = (struct tgsi_src_register *) &tokens[size];
         size++;

         *ind = tgsi_build_src_register(
            reg->Indirect.File,
            reg->Indirect.SwizzleX,
            reg->Indirect.SwizzleY,
            reg->Indirect.SwizzleZ,
            reg->Indirect.SwizzleW,
            reg->Indirect.Negate,
            reg->Indirect.Absolute,
            reg->Indirect.Indirect,
            reg->Indirect.Dimension,
            reg->Indirect.Index,
            instruction,
            header );
      }

      if( reg->Register.Dimension ) {
         struct  tgsi_dimension *dim;

         assert( !reg->Dimension.Dimension );

         if( maxsize <= size )
            return 0;
         dim = (struct tgsi_dimension *) &tokens[size];
         size++;

         *dim = tgsi_build_dimension(
            reg->Dimension.Indirect,
            reg->Dimension.Index,
            instruction,
            header );

         if( reg->Dimension.Indirect ) {
            struct tgsi_src_register *ind;

            if( maxsize <= size )
               return 0;
            ind = (struct tgsi_src_register *) &tokens[size];
            size++;

            *ind = tgsi_build_src_register(
               reg->DimIndirect.File,
               reg->DimIndirect.SwizzleX,
               reg->DimIndirect.SwizzleY,
               reg->DimIndirect.SwizzleZ,
               reg->DimIndirect.SwizzleW,
               reg->DimIndirect.Negate,
               reg->DimIndirect.Absolute,
               reg->DimIndirect.Indirect,
               reg->DimIndirect.Dimension,
               reg->DimIndirect.Index,
               instruction,
               header );
         }
      }
   }

   return size;
}

static struct tgsi_property
tgsi_default_property( void )
{
   struct tgsi_property property;

   property.Type = TGSI_TOKEN_TYPE_PROPERTY;
   property.NrTokens = 1;
   property.PropertyName = TGSI_PROPERTY_GS_INPUT_PRIM;
   property.Padding = 0;

   return property;
}

static struct tgsi_property
tgsi_build_property(unsigned property_name,
                    struct tgsi_header *header)
{
   struct tgsi_property property;

   property = tgsi_default_property();
   property.PropertyName = property_name;

   header_bodysize_grow( header );

   return property;
}


struct tgsi_full_property
tgsi_default_full_property( void )
{
   struct tgsi_full_property  full_property;

   full_property.Property  = tgsi_default_property();
   memset(full_property.u, 0,
          sizeof(struct tgsi_property_data) * 8);

   return full_property;
}

static void
property_grow(
   struct tgsi_property *property,
   struct tgsi_header *header )
{
   assert( property->NrTokens < 0xFF );

   property->NrTokens++;

   header_bodysize_grow( header );
}

static struct tgsi_property_data
tgsi_build_property_data(
   unsigned value,
   struct tgsi_property *property,
   struct tgsi_header *header )
{
   struct tgsi_property_data property_data;

   property_data.Data = value;

   property_grow( property, header );

   return property_data;
}

unsigned
tgsi_build_full_property(
   const struct tgsi_full_property *full_prop,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   unsigned maxsize )
{
   unsigned size = 0, i;
   struct tgsi_property *property;

   if( maxsize <= size )
      return 0;
   property = (struct tgsi_property *) &tokens[size];
   size++;

   *property = tgsi_build_property(
      full_prop->Property.PropertyName,
      header );

   assert( full_prop->Property.NrTokens <= 8 + 1 );

   for( i = 0; i < full_prop->Property.NrTokens - 1; i++ ) {
      struct tgsi_property_data *data;

      if( maxsize <= size )
         return  0;
      data = (struct tgsi_property_data *) &tokens[size];
      size++;

      *data = tgsi_build_property_data(
         full_prop->u[i].Data,
         property,
         header );
   }

   return size;
}
