/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include "util/u_memory.h"
#include "util/u_prim.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "tgsi_text.h"
#include "tgsi_build.h"
#include "tgsi_info.h"
#include "tgsi_parse.h"
#include "tgsi_sanity.h"
#include "tgsi_util.h"

static boolean is_alpha_underscore( const char *cur )
{
   return
      (*cur >= 'a' && *cur <= 'z') ||
      (*cur >= 'A' && *cur <= 'Z') ||
      *cur == '_';
}

static boolean is_digit( const char *cur )
{
   return *cur >= '0' && *cur <= '9';
}

static boolean is_digit_alpha_underscore( const char *cur )
{
   return is_digit( cur ) || is_alpha_underscore( cur );
}

static char uprcase( char c )
{
   if (c >= 'a' && c <= 'z')
      return c + 'A' - 'a';
   return c;
}

/*
 * Ignore case of str1 and assume str1 is already uppercase.
 * Return TRUE iff str1 and str2 are equal.
 */
static int
streq_nocase_uprcase(const char *str1,
                     const char *str2)
{
   while (*str1 && *str2) {
      if (*str1 != uprcase(*str2))
         return FALSE;
      str1++;
      str2++;
   }
   return *str1 == 0 && *str2 == 0;
}

static boolean str_match_no_case( const char **pcur, const char *str )
{
   const char *cur = *pcur;

   while (*str != '\0' && *str == uprcase( *cur )) {
      str++;
      cur++;
   }
   if (*str == '\0') {
      *pcur = cur;
      return TRUE;
   }
   return FALSE;
}

/* Eat zero or more whitespaces.
 */
static void eat_opt_white( const char **pcur )
{
   while (**pcur == ' ' || **pcur == '\t' || **pcur == '\n')
      (*pcur)++;
}

/* Eat one or more whitespaces.
 * Return TRUE if at least one whitespace eaten.
 */
static boolean eat_white( const char **pcur )
{
   const char *cur = *pcur;

   eat_opt_white( pcur );
   return *pcur > cur;
}

/* Parse unsigned integer.
 * No checks for overflow.
 */
static boolean parse_uint( const char **pcur, uint *val )
{
   const char *cur = *pcur;

   if (is_digit( cur )) {
      *val = *cur++ - '0';
      while (is_digit( cur ))
         *val = *val * 10 + *cur++ - '0';
      *pcur = cur;
      return TRUE;
   }
   return FALSE;
}

static boolean parse_identifier( const char **pcur, char *ret )
{
   const char *cur = *pcur;
   int i = 0;
   if (is_alpha_underscore( cur )) {
      ret[i++] = *cur++;
      while (is_alpha_underscore( cur ))
         ret[i++] = *cur++;
      ret[i++] = '\0';
      *pcur = cur;
      return TRUE;
   }
   return FALSE;
}

/* Parse floating point.
 */
static boolean parse_float( const char **pcur, float *val )
{
   const char *cur = *pcur;
   boolean integral_part = FALSE;
   boolean fractional_part = FALSE;

   *val = (float) atof( cur );

   if (*cur == '-' || *cur == '+')
      cur++;
   if (is_digit( cur )) {
      cur++;
      integral_part = TRUE;
      while (is_digit( cur ))
         cur++;
   }
   if (*cur == '.') {
      cur++;
      if (is_digit( cur )) {
         cur++;
         fractional_part = TRUE;
         while (is_digit( cur ))
            cur++;
      }
   }
   if (!integral_part && !fractional_part)
      return FALSE;
   if (uprcase( *cur ) == 'E') {
      cur++;
      if (*cur == '-' || *cur == '+')
         cur++;
      if (is_digit( cur )) {
         cur++;
         while (is_digit( cur ))
            cur++;
      }
      else
         return FALSE;
   }
   *pcur = cur;
   return TRUE;
}

struct translate_ctx
{
   const char *text;
   const char *cur;
   struct tgsi_token *tokens;
   struct tgsi_token *tokens_cur;
   struct tgsi_token *tokens_end;
   struct tgsi_header *header;
   unsigned processor : 4;
   int implied_array_size : 5;
};

static void report_error( struct translate_ctx *ctx, const char *msg )
{
   int line = 1;
   int column = 1;
   const char *itr = ctx->text;

   while (itr != ctx->cur) {
      if (*itr == '\n') {
         column = 1;
         ++line;
      }
      ++column;
      ++itr;
   }

   debug_printf( "\nTGSI asm error: %s [%d : %d] \n", msg, line, column );
}

/* Parse shader header.
 * Return TRUE for one of the following headers.
 *    FRAG
 *    GEOM
 *    VERT
 */
static boolean parse_header( struct translate_ctx *ctx )
{
   uint processor;

   if (str_match_no_case( &ctx->cur, "FRAG" ))
      processor = TGSI_PROCESSOR_FRAGMENT;
   else if (str_match_no_case( &ctx->cur, "VERT" ))
      processor = TGSI_PROCESSOR_VERTEX;
   else if (str_match_no_case( &ctx->cur, "GEOM" ))
      processor = TGSI_PROCESSOR_GEOMETRY;
   else {
      report_error( ctx, "Unknown header" );
      return FALSE;
   }

   if (ctx->tokens_cur >= ctx->tokens_end)
      return FALSE;
   ctx->header = (struct tgsi_header *) ctx->tokens_cur++;
   *ctx->header = tgsi_build_header();

   if (ctx->tokens_cur >= ctx->tokens_end)
      return FALSE;
   *(struct tgsi_processor *) ctx->tokens_cur++ = tgsi_build_processor( processor, ctx->header );
   ctx->processor = processor;

   return TRUE;
}

static boolean parse_label( struct translate_ctx *ctx, uint *val )
{
   const char *cur = ctx->cur;

   if (parse_uint( &cur, val )) {
      eat_opt_white( &cur );
      if (*cur == ':') {
         cur++;
         ctx->cur = cur;
         return TRUE;
      }
   }
   return FALSE;
}

static const char *file_names[TGSI_FILE_COUNT] =
{
   "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMP",
   "ADDR",
   "IMM",
   "PRED",
   "SV",
   "IMMX",
   "TEMPX"
};

static boolean
parse_file( const char **pcur, uint *file )
{
   uint i;

   for (i = 0; i < TGSI_FILE_COUNT; i++) {
      const char *cur = *pcur;

      if (str_match_no_case( &cur, file_names[i] )) {
         if (!is_digit_alpha_underscore( cur )) {
            *pcur = cur;
            *file = i;
            return TRUE;
         }
      }
   }
   return FALSE;
}

static boolean
parse_opt_writemask(
   struct translate_ctx *ctx,
   uint *writemask )
{
   const char *cur;

   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == '.') {
      cur++;
      *writemask = TGSI_WRITEMASK_NONE;
      eat_opt_white( &cur );
      if (uprcase( *cur ) == 'X') {
         cur++;
         *writemask |= TGSI_WRITEMASK_X;
      }
      if (uprcase( *cur ) == 'Y') {
         cur++;
         *writemask |= TGSI_WRITEMASK_Y;
      }
      if (uprcase( *cur ) == 'Z') {
         cur++;
         *writemask |= TGSI_WRITEMASK_Z;
      }
      if (uprcase( *cur ) == 'W') {
         cur++;
         *writemask |= TGSI_WRITEMASK_W;
      }

      if (*writemask == TGSI_WRITEMASK_NONE) {
         report_error( ctx, "Writemask expected" );
         return FALSE;
      }

      ctx->cur = cur;
   }
   else {
      *writemask = TGSI_WRITEMASK_XYZW;
   }
   return TRUE;
}


/* <register_file_bracket> ::= <file> `['
 */
static boolean
parse_register_file_bracket(
   struct translate_ctx *ctx,
   uint *file )
{
   if (!parse_file( &ctx->cur, file )) {
      report_error( ctx, "Unknown register file" );
      return FALSE;
   }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '[') {
      report_error( ctx, "Expected `['" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

/* <register_file_bracket_index> ::= <register_file_bracket> <uint>
 */
static boolean
parse_register_file_bracket_index(
   struct translate_ctx *ctx,
   uint *file,
   int *index )
{
   uint uindex;

   if (!parse_register_file_bracket( ctx, file ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (!parse_uint( &ctx->cur, &uindex )) {
      report_error( ctx, "Expected literal unsigned integer" );
      return FALSE;
   }
   *index = (int) uindex;
   return TRUE;
}

/* Parse simple 1d register operand.
 *    <register_dst> ::= <register_file_bracket_index> `]'
 */
static boolean
parse_register_1d(struct translate_ctx *ctx,
                  uint *file,
                  int *index )
{
   if (!parse_register_file_bracket_index( ctx, file, index ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

struct parsed_bracket {
   int index;

   uint ind_file;
   int ind_index;
   uint ind_comp;
};


static boolean
parse_register_bracket(
   struct translate_ctx *ctx,
   struct parsed_bracket *brackets)
{
   const char *cur;
   uint uindex;

   memset(brackets, 0, sizeof(struct parsed_bracket));

   eat_opt_white( &ctx->cur );

   cur = ctx->cur;
   if (parse_file( &cur, &brackets->ind_file )) {
      if (!parse_register_1d( ctx, &brackets->ind_file,
                              &brackets->ind_index ))
         return FALSE;
      eat_opt_white( &ctx->cur );

      if (*ctx->cur == '.') {
         ctx->cur++;
         eat_opt_white(&ctx->cur);

         switch (uprcase(*ctx->cur)) {
         case 'X':
            brackets->ind_comp = TGSI_SWIZZLE_X;
            break;
         case 'Y':
            brackets->ind_comp = TGSI_SWIZZLE_Y;
            break;
         case 'Z':
            brackets->ind_comp = TGSI_SWIZZLE_Z;
            break;
         case 'W':
            brackets->ind_comp = TGSI_SWIZZLE_W;
            break;
         default:
            report_error(ctx, "Expected indirect register swizzle component `x', `y', `z' or `w'");
            return FALSE;
         }
         ctx->cur++;
         eat_opt_white(&ctx->cur);
      }

      if (*ctx->cur == '+' || *ctx->cur == '-') {
         boolean negate;

         negate = *ctx->cur == '-';
         ctx->cur++;
         eat_opt_white( &ctx->cur );
         if (!parse_uint( &ctx->cur, &uindex )) {
            report_error( ctx, "Expected literal unsigned integer" );
            return FALSE;
         }
         if (negate)
            brackets->index = -(int) uindex;
         else
            brackets->index = (int) uindex;
      }
      else {
         brackets->index = 0;
      }
   }
   else {
      if (!parse_uint( &ctx->cur, &uindex )) {
         report_error( ctx, "Expected literal unsigned integer" );
         return FALSE;
      }
      brackets->index = (int) uindex;
      brackets->ind_file = TGSI_FILE_NULL;
      brackets->ind_index = 0;
   }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

static boolean
parse_opt_register_src_bracket(
   struct translate_ctx *ctx,
   struct parsed_bracket *brackets,
   int *parsed_brackets)
{
   const char *cur = ctx->cur;

   *parsed_brackets = 0;

   eat_opt_white( &cur );
   if (cur[0] == '[') {
      ++cur;
      ctx->cur = cur;

      if (!parse_register_bracket(ctx, brackets))
         return FALSE;

      *parsed_brackets = 1;
   }

   return TRUE;
}


/* Parse source register operand.
 *    <register_src> ::= <register_file_bracket_index> `]' |
 *                       <register_file_bracket> <register_dst> [`.' (`x' | `y' | `z' | `w')] `]' |
 *                       <register_file_bracket> <register_dst> [`.' (`x' | `y' | `z' | `w')] `+' <uint> `]' |
 *                       <register_file_bracket> <register_dst> [`.' (`x' | `y' | `z' | `w')] `-' <uint> `]'
 */
static boolean
parse_register_src(
   struct translate_ctx *ctx,
   uint *file,
   struct parsed_bracket *brackets)
{
   brackets->ind_comp = TGSI_SWIZZLE_X;
   if (!parse_register_file_bracket( ctx, file ))
      return FALSE;
   if (!parse_register_bracket( ctx, brackets ))
       return FALSE;

   return TRUE;
}

struct parsed_dcl_bracket {
   uint first;
   uint last;
};

static boolean
parse_register_dcl_bracket(
   struct translate_ctx *ctx,
   struct parsed_dcl_bracket *bracket)
{
   uint uindex;
   memset(bracket, 0, sizeof(struct parsed_dcl_bracket));

   eat_opt_white( &ctx->cur );

   if (!parse_uint( &ctx->cur, &uindex )) {
      /* it can be an empty bracket [] which means its range
       * is from 0 to some implied size */
      if (ctx->cur[0] == ']' && ctx->implied_array_size != 0) {
         bracket->first = 0;
         bracket->last = ctx->implied_array_size - 1;
         goto cleanup;
      }
      report_error( ctx, "Expected literal unsigned integer" );
      return FALSE;
   }
   bracket->first = uindex;

   eat_opt_white( &ctx->cur );

   if (ctx->cur[0] == '.' && ctx->cur[1] == '.') {
      uint uindex;

      ctx->cur += 2;
      eat_opt_white( &ctx->cur );
      if (!parse_uint( &ctx->cur, &uindex )) {
         report_error( ctx, "Expected literal integer" );
         return FALSE;
      }
      bracket->last = (int) uindex;
      eat_opt_white( &ctx->cur );
   }
   else {
      bracket->last = bracket->first;
   }

cleanup:
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]' or `..'" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

/* Parse register declaration.
 *    <register_dcl> ::= <register_file_bracket_index> `]' |
 *                       <register_file_bracket_index> `..' <index> `]'
 */
static boolean
parse_register_dcl(
   struct translate_ctx *ctx,
   uint *file,
   struct parsed_dcl_bracket *brackets,
   int *num_brackets)
{
   const char *cur;

   *num_brackets = 0;

   if (!parse_register_file_bracket( ctx, file ))
      return FALSE;
   if (!parse_register_dcl_bracket( ctx, &brackets[0] ))
      return FALSE;

   *num_brackets = 1;

   cur = ctx->cur;
   eat_opt_white( &cur );

   if (cur[0] == '[') {
      ++cur;
      ctx->cur = cur;
      if (!parse_register_dcl_bracket( ctx, &brackets[1] ))
         return FALSE;
      /* for geometry shader we don't really care about
       * the first brackets it's always the size of the
       * input primitive. so we want to declare just
       * the index relevant to the semantics which is in
       * the second bracket */
      if (ctx->processor == TGSI_PROCESSOR_GEOMETRY && *file == TGSI_FILE_INPUT) {
         brackets[0] = brackets[1];
         *num_brackets = 1;
      } else {
         *num_brackets = 2;
      }
   }

   return TRUE;
}


/* Parse destination register operand.*/
static boolean
parse_register_dst(
   struct translate_ctx *ctx,
   uint *file,
   struct parsed_bracket *brackets)
{
   brackets->ind_comp = TGSI_SWIZZLE_X;
   if (!parse_register_file_bracket( ctx, file ))
      return FALSE;
   if (!parse_register_bracket( ctx, brackets ))
       return FALSE;

   return TRUE;
}

static boolean
parse_dst_operand(
   struct translate_ctx *ctx,
   struct tgsi_full_dst_register *dst )
{
   uint file;
   uint writemask;
   const char *cur;
   struct parsed_bracket bracket[2];
   int parsed_opt_brackets;

   if (!parse_register_dst( ctx, &file, &bracket[0] ))
      return FALSE;
   if (!parse_opt_register_src_bracket(ctx, &bracket[1], &parsed_opt_brackets))
      return FALSE;

   cur = ctx->cur;
   eat_opt_white( &cur );

   if (!parse_opt_writemask( ctx, &writemask ))
      return FALSE;

   dst->Register.File = file;
   if (parsed_opt_brackets) {
      dst->Register.Dimension = 1;
      dst->Dimension.Indirect = 0;
      dst->Dimension.Dimension = 0;
      dst->Dimension.Index = bracket[0].index;
      bracket[0] = bracket[1];
   }
   dst->Register.Index = bracket[0].index;
   dst->Register.WriteMask = writemask;
   if (bracket[0].ind_file != TGSI_FILE_NULL) {
      dst->Register.Indirect = 1;
      dst->Indirect.File = bracket[0].ind_file;
      dst->Indirect.Index = bracket[0].ind_index;
      dst->Indirect.SwizzleX = bracket[0].ind_comp;
      dst->Indirect.SwizzleY = bracket[0].ind_comp;
      dst->Indirect.SwizzleZ = bracket[0].ind_comp;
      dst->Indirect.SwizzleW = bracket[0].ind_comp;
   }
   return TRUE;
}

static boolean
parse_optional_swizzle(
   struct translate_ctx *ctx,
   uint swizzle[4],
   boolean *parsed_swizzle )
{
   const char *cur = ctx->cur;

   *parsed_swizzle = FALSE;

   eat_opt_white( &cur );
   if (*cur == '.') {
      uint i;

      cur++;
      eat_opt_white( &cur );
      for (i = 0; i < 4; i++) {
         if (uprcase( *cur ) == 'X')
            swizzle[i] = TGSI_SWIZZLE_X;
         else if (uprcase( *cur ) == 'Y')
            swizzle[i] = TGSI_SWIZZLE_Y;
         else if (uprcase( *cur ) == 'Z')
            swizzle[i] = TGSI_SWIZZLE_Z;
         else if (uprcase( *cur ) == 'W')
            swizzle[i] = TGSI_SWIZZLE_W;
         else {
	    report_error( ctx, "Expected register swizzle component `x', `y', `z' or `w'" );
	    return FALSE;
         }
         cur++;
      }
      *parsed_swizzle = TRUE;
      ctx->cur = cur;
   }
   return TRUE;
}

static boolean
parse_src_operand(
   struct translate_ctx *ctx,
   struct tgsi_full_src_register *src )
{
   uint file;
   uint swizzle[4];
   boolean parsed_swizzle;
   struct parsed_bracket bracket[2];
   int parsed_opt_brackets;

   if (*ctx->cur == '-') {
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      src->Register.Negate = 1;
   }

   if (*ctx->cur == '|') {
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      src->Register.Absolute = 1;
   }

   if (!parse_register_src(ctx, &file, &bracket[0]))
      return FALSE;
   if (!parse_opt_register_src_bracket(ctx, &bracket[1], &parsed_opt_brackets))
      return FALSE;

   src->Register.File = file;
   if (parsed_opt_brackets) {
      src->Register.Dimension = 1;
      src->Dimension.Indirect = 0;
      src->Dimension.Dimension = 0;
      src->Dimension.Index = bracket[0].index;
      bracket[0] = bracket[1];
   }
   src->Register.Index = bracket[0].index;
   if (bracket[0].ind_file != TGSI_FILE_NULL) {
      src->Register.Indirect = 1;
      src->Indirect.File = bracket[0].ind_file;
      src->Indirect.Index = bracket[0].ind_index;
      src->Indirect.SwizzleX = bracket[0].ind_comp;
      src->Indirect.SwizzleY = bracket[0].ind_comp;
      src->Indirect.SwizzleZ = bracket[0].ind_comp;
      src->Indirect.SwizzleW = bracket[0].ind_comp;
   }

   /* Parse optional swizzle.
    */
   if (parse_optional_swizzle( ctx, swizzle, &parsed_swizzle )) {
      if (parsed_swizzle) {
         src->Register.SwizzleX = swizzle[0];
         src->Register.SwizzleY = swizzle[1];
         src->Register.SwizzleZ = swizzle[2];
         src->Register.SwizzleW = swizzle[3];
      }
   }

   if (src->Register.Absolute) {
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != '|') {
         report_error( ctx, "Expected `|'" );
         return FALSE;
      }
      ctx->cur++;
   }


   return TRUE;
}

static const char *texture_names[TGSI_TEXTURE_COUNT] =
{
   "UNKNOWN",
   "1D",
   "2D",
   "3D",
   "CUBE",
   "RECT",
   "SHADOW1D",
   "SHADOW2D",
   "SHADOWRECT"
};

static boolean
match_inst_mnemonic(const char **pcur,
                    const struct tgsi_opcode_info *info)
{
   if (str_match_no_case(pcur, info->mnemonic)) {
      return TRUE;
   }
   return FALSE;
}

static boolean
parse_instruction(
   struct translate_ctx *ctx,
   boolean has_label )
{
   uint i;
   uint saturate = TGSI_SAT_NONE;
   const struct tgsi_opcode_info *info;
   struct tgsi_full_instruction inst;
   uint advance;

   inst = tgsi_default_full_instruction();

   /* Parse predicate.
    */
   eat_opt_white( &ctx->cur );
   if (*ctx->cur == '(') {
      uint file;
      int index;
      uint swizzle[4];
      boolean parsed_swizzle;

      inst.Instruction.Predicate = 1;

      ctx->cur++;
      if (*ctx->cur == '!') {
         ctx->cur++;
         inst.Predicate.Negate = 1;
      }

      if (!parse_register_1d( ctx, &file, &index ))
         return FALSE;

      if (parse_optional_swizzle( ctx, swizzle, &parsed_swizzle )) {
         if (parsed_swizzle) {
            inst.Predicate.SwizzleX = swizzle[0];
            inst.Predicate.SwizzleY = swizzle[1];
            inst.Predicate.SwizzleZ = swizzle[2];
            inst.Predicate.SwizzleW = swizzle[3];
         }
      }

      if (*ctx->cur != ')') {
         report_error( ctx, "Expected `)'" );
         return FALSE;
      }

      ctx->cur++;
   }

   /* Parse instruction name.
    */
   eat_opt_white( &ctx->cur );
   for (i = 0; i < TGSI_OPCODE_LAST; i++) {
      const char *cur = ctx->cur;

      info = tgsi_get_opcode_info( i );
      if (match_inst_mnemonic(&cur, info)) {
         if (str_match_no_case( &cur, "_SATNV" ))
            saturate = TGSI_SAT_MINUS_PLUS_ONE;
         else if (str_match_no_case( &cur, "_SAT" ))
            saturate = TGSI_SAT_ZERO_ONE;

         if (info->num_dst + info->num_src + info->is_tex == 0) {
            if (!is_digit_alpha_underscore( cur )) {
               ctx->cur = cur;
               break;
            }
         }
         else if (*cur == '\0' || eat_white( &cur )) {
            ctx->cur = cur;
            break;
         }
      }
   }
   if (i == TGSI_OPCODE_LAST) {
      if (has_label)
         report_error( ctx, "Unknown opcode" );
      else
         report_error( ctx, "Expected `DCL', `IMM' or a label" );
      return FALSE;
   }

   inst.Instruction.Opcode = i;
   inst.Instruction.Saturate = saturate;
   inst.Instruction.NumDstRegs = info->num_dst;
   inst.Instruction.NumSrcRegs = info->num_src;

   /* Parse instruction operands.
    */
   for (i = 0; i < info->num_dst + info->num_src + info->is_tex; i++) {
      if (i > 0) {
         eat_opt_white( &ctx->cur );
         if (*ctx->cur != ',') {
            report_error( ctx, "Expected `,'" );
            return FALSE;
         }
         ctx->cur++;
         eat_opt_white( &ctx->cur );
      }

      if (i < info->num_dst) {
         if (!parse_dst_operand( ctx, &inst.Dst[i] ))
            return FALSE;
      }
      else if (i < info->num_dst + info->num_src) {
         if (!parse_src_operand( ctx, &inst.Src[i - info->num_dst] ))
            return FALSE;
      }
      else {
         uint j;

         for (j = 0; j < TGSI_TEXTURE_COUNT; j++) {
            if (str_match_no_case( &ctx->cur, texture_names[j] )) {
               if (!is_digit_alpha_underscore( ctx->cur )) {
                  inst.Instruction.Texture = 1;
                  inst.Texture.Texture = j;
                  break;
               }
            }
         }
         if (j == TGSI_TEXTURE_COUNT) {
            report_error( ctx, "Expected texture target" );
            return FALSE;
         }
      }
   }

   if (info->is_branch) {
      uint target;

      eat_opt_white( &ctx->cur );
      if (*ctx->cur != ':') {
         report_error( ctx, "Expected `:'" );
         return FALSE;
      }
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      if (!parse_uint( &ctx->cur, &target )) {
         report_error( ctx, "Expected a label" );
         return FALSE;
      }
      inst.Instruction.Label = 1;
      inst.Label.Label = target;
   }

   advance = tgsi_build_full_instruction(
      &inst,
      ctx->tokens_cur,
      ctx->header,
      (uint) (ctx->tokens_end - ctx->tokens_cur) );
   if (advance == 0)
      return FALSE;
   ctx->tokens_cur += advance;

   return TRUE;
}

static const char *semantic_names[TGSI_SEMANTIC_COUNT] =
{
   "POSITION",
   "COLOR",
   "BCOLOR",
   "FOG",
   "PSIZE",
   "GENERIC",
   "NORMAL",
   "FACE",
   "EDGEFLAG",
   "PRIM_ID",
   "INSTANCEID"
};

static const char *interpolate_names[TGSI_INTERPOLATE_COUNT] =
{
   "CONSTANT",
   "LINEAR",
   "PERSPECTIVE"
};


/* parses a 4-touple of the form {x, y, z, w}
 * where x, y, z, w are numbers */
static boolean parse_immediate_data(struct translate_ctx *ctx,
                                    float *values)
{
   unsigned i;

   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '{') {
      report_error( ctx, "Expected `{'" );
      return FALSE;
   }
   ctx->cur++;
   for (i = 0; i < 4; i++) {
      eat_opt_white( &ctx->cur );
      if (i > 0) {
         if (*ctx->cur != ',') {
            report_error( ctx, "Expected `,'" );
            return FALSE;
         }
         ctx->cur++;
         eat_opt_white( &ctx->cur );
      }
      if (!parse_float( &ctx->cur, &values[i] )) {
         report_error( ctx, "Expected literal floating point" );
         return FALSE;
      }
   }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '}') {
      report_error( ctx, "Expected `}'" );
      return FALSE;
   }
   ctx->cur++;

   return TRUE;
}

static boolean parse_declaration( struct translate_ctx *ctx )
{
   struct tgsi_full_declaration decl;
   uint file;
   struct parsed_dcl_bracket brackets[2];
   int num_brackets;
   uint writemask;
   const char *cur;
   uint advance;
   boolean is_vs_input;
   boolean is_imm_array;

   assert(Elements(semantic_names) == TGSI_SEMANTIC_COUNT);
   assert(Elements(interpolate_names) == TGSI_INTERPOLATE_COUNT);

   if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      return FALSE;
   }
   if (!parse_register_dcl( ctx, &file, brackets, &num_brackets))
      return FALSE;
   if (!parse_opt_writemask( ctx, &writemask ))
      return FALSE;

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = file;
   decl.Declaration.UsageMask = writemask;

   if (num_brackets == 1) {
      decl.Range.First = brackets[0].first;
      decl.Range.Last = brackets[0].last;
   } else {
      decl.Range.First = brackets[1].first;
      decl.Range.Last = brackets[1].last;

      decl.Declaration.Dimension = 1;
      decl.Dim.Index2D = brackets[0].first;
   }

   is_vs_input = (file == TGSI_FILE_INPUT &&
                  ctx->processor == TGSI_PROCESSOR_VERTEX);
   is_imm_array = (file == TGSI_FILE_IMMEDIATE_ARRAY);

   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',' && !is_vs_input) {
      uint i;

      cur++;
      eat_opt_white( &cur );
      for (i = 0; i < TGSI_SEMANTIC_COUNT; i++) {
         if (str_match_no_case( &cur, semantic_names[i] )) {
            const char *cur2 = cur;
            uint index;

            if (is_digit_alpha_underscore( cur ))
               continue;
            eat_opt_white( &cur2 );
            if (*cur2 == '[') {
               cur2++;
               eat_opt_white( &cur2 );
               if (!parse_uint( &cur2, &index )) {
                  report_error( ctx, "Expected literal integer" );
                  return FALSE;
               }
               eat_opt_white( &cur2 );
               if (*cur2 != ']') {
                  report_error( ctx, "Expected `]'" );
                  return FALSE;
               }
               cur2++;

               decl.Semantic.Index = index;

               cur = cur2;
            }

            decl.Declaration.Semantic = 1;
            decl.Semantic.Name = i;

            ctx->cur = cur;
            break;
         }
      }
   } else if (is_imm_array) {
      unsigned i;
      float *vals_itr;
      /* we have our immediate data */
      if (*cur != '{') {
         report_error( ctx, "Immediate array without data" );
         return FALSE;
      }
      ++cur;
      ctx->cur = cur;

      decl.ImmediateData.u =
         MALLOC(sizeof(union tgsi_immediate_data) * 4 *
                (decl.Range.Last + 1));
      vals_itr = (float*)decl.ImmediateData.u;
      for (i = 0; i <= decl.Range.Last; ++i) {
         if (!parse_immediate_data(ctx, vals_itr)) {
            FREE(decl.ImmediateData.u);
            return FALSE;
         }
         vals_itr += 4;
         eat_opt_white( &ctx->cur );
         if (*ctx->cur != ',') {
            if (i !=  decl.Range.Last) {
               report_error( ctx, "Not enough data in immediate array!" );
               FREE(decl.ImmediateData.u);
               return FALSE;
            }
         } else
            ++ctx->cur;
      }
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != '}') {
         FREE(decl.ImmediateData.u);
         report_error( ctx, "Immediate array data missing closing '}'" );
         return FALSE;
      }
      ++ctx->cur;
   }

   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',' && !is_vs_input) {
      uint i;

      cur++;
      eat_opt_white( &cur );
      for (i = 0; i < TGSI_INTERPOLATE_COUNT; i++) {
         if (str_match_no_case( &cur, interpolate_names[i] )) {
            if (is_digit_alpha_underscore( cur ))
               continue;
            decl.Declaration.Interpolate = i;

            ctx->cur = cur;
            break;
         }
      }
      if (i == TGSI_INTERPOLATE_COUNT) {
         report_error( ctx, "Expected semantic or interpolate attribute" );
         return FALSE;
      }
   }

   advance = tgsi_build_full_declaration(
      &decl,
      ctx->tokens_cur,
      ctx->header,
      (uint) (ctx->tokens_end - ctx->tokens_cur) );

   if (is_imm_array)
      FREE(decl.ImmediateData.u);

   if (advance == 0)
      return FALSE;
   ctx->tokens_cur += advance;

   return TRUE;
}

static boolean parse_immediate( struct translate_ctx *ctx )
{
   struct tgsi_full_immediate imm;
   float values[4];
   uint advance;

   if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      return FALSE;
   }
   if (!str_match_no_case( &ctx->cur, "FLT32" ) ||
       is_digit_alpha_underscore( ctx->cur )) {
      report_error( ctx, "Expected `FLT32'" );
      return FALSE;
   }

   parse_immediate_data(ctx, values);

   imm = tgsi_default_full_immediate();
   imm.Immediate.NrTokens += 4;
   imm.Immediate.DataType = TGSI_IMM_FLOAT32;
   imm.u[0].Float = values[0];
   imm.u[1].Float = values[1];
   imm.u[2].Float = values[2];
   imm.u[3].Float = values[3];

   advance = tgsi_build_full_immediate(
      &imm,
      ctx->tokens_cur,
      ctx->header,
      (uint) (ctx->tokens_end - ctx->tokens_cur) );
   if (advance == 0)
      return FALSE;
   ctx->tokens_cur += advance;

   return TRUE;
}

static const char *property_names[] =
{
   "GS_INPUT_PRIMITIVE",
   "GS_OUTPUT_PRIMITIVE",
   "GS_MAX_OUTPUT_VERTICES",
   "FS_COORD_ORIGIN",
   "FS_COORD_PIXEL_CENTER"
};

static const char *primitive_names[] =
{
   "POINTS",
   "LINES",
   "LINE_LOOP",
   "LINE_STRIP",
   "TRIANGLES",
   "TRIANGLE_STRIP",
   "TRIANGLE_FAN",
   "QUADS",
   "QUAD_STRIP",
   "POLYGON"
};

static const char *fs_coord_origin_names[] =
{
   "UPPER_LEFT",
   "LOWER_LEFT"
};

static const char *fs_coord_pixel_center_names[] =
{
   "HALF_INTEGER",
   "INTEGER"
};


static boolean
parse_primitive( const char **pcur, uint *primitive )
{
   uint i;

   for (i = 0; i < PIPE_PRIM_MAX; i++) {
      const char *cur = *pcur;

      if (str_match_no_case( &cur, primitive_names[i])) {
         *primitive = i;
         *pcur = cur;
         return TRUE;
      }
   }
   return FALSE;
}

static boolean
parse_fs_coord_origin( const char **pcur, uint *fs_coord_origin )
{
   uint i;

   for (i = 0; i < sizeof(fs_coord_origin_names) / sizeof(fs_coord_origin_names[0]); i++) {
      const char *cur = *pcur;

      if (str_match_no_case( &cur, fs_coord_origin_names[i])) {
         *fs_coord_origin = i;
         *pcur = cur;
         return TRUE;
      }
   }
   return FALSE;
}

static boolean
parse_fs_coord_pixel_center( const char **pcur, uint *fs_coord_pixel_center )
{
   uint i;

   for (i = 0; i < sizeof(fs_coord_pixel_center_names) / sizeof(fs_coord_pixel_center_names[0]); i++) {
      const char *cur = *pcur;

      if (str_match_no_case( &cur, fs_coord_pixel_center_names[i])) {
         *fs_coord_pixel_center = i;
         *pcur = cur;
         return TRUE;
      }
   }
   return FALSE;
}


static boolean parse_property( struct translate_ctx *ctx )
{
   struct tgsi_full_property prop;
   uint property_name;
   uint values[8];
   uint advance;
   char id[64];

   if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      return FALSE;
   }
   if (!parse_identifier( &ctx->cur, id )) {
      report_error( ctx, "Syntax error" );
      return FALSE;
   }
   for (property_name = 0; property_name < TGSI_PROPERTY_COUNT;
        ++property_name) {
      if (streq_nocase_uprcase(property_names[property_name], id)) {
         break;
      }
   }
   if (property_name >= TGSI_PROPERTY_COUNT) {
      debug_printf( "\nError: Unknown property : '%s'", id );
      return FALSE;
   }

   eat_opt_white( &ctx->cur );
   switch(property_name) {
   case TGSI_PROPERTY_GS_INPUT_PRIM:
   case TGSI_PROPERTY_GS_OUTPUT_PRIM:
      if (!parse_primitive(&ctx->cur, &values[0] )) {
         report_error( ctx, "Unknown primitive name as property!" );
         return FALSE;
      }
      if (property_name == TGSI_PROPERTY_GS_INPUT_PRIM &&
          ctx->processor == TGSI_PROCESSOR_GEOMETRY) {
         ctx->implied_array_size = u_vertices_per_prim(values[0]);
      }
      break;
   case TGSI_PROPERTY_FS_COORD_ORIGIN:
      if (!parse_fs_coord_origin(&ctx->cur, &values[0] )) {
         report_error( ctx, "Unknown coord origin as property: must be UPPER_LEFT or LOWER_LEFT!" );
         return FALSE;
      }
      break;
   case TGSI_PROPERTY_FS_COORD_PIXEL_CENTER:
      if (!parse_fs_coord_pixel_center(&ctx->cur, &values[0] )) {
         report_error( ctx, "Unknown coord pixel center as property: must be HALF_INTEGER or INTEGER!" );
         return FALSE;
      }
      break;
   default:
      if (!parse_uint(&ctx->cur, &values[0] )) {
         report_error( ctx, "Expected unsigned integer as property!" );
         return FALSE;
      }
   }

   prop = tgsi_default_full_property();
   prop.Property.PropertyName = property_name;
   prop.Property.NrTokens += 1;
   prop.u[0].Data = values[0];

   advance = tgsi_build_full_property(
      &prop,
      ctx->tokens_cur,
      ctx->header,
      (uint) (ctx->tokens_end - ctx->tokens_cur) );
   if (advance == 0)
      return FALSE;
   ctx->tokens_cur += advance;

   return TRUE;
}


static boolean translate( struct translate_ctx *ctx )
{
   eat_opt_white( &ctx->cur );
   if (!parse_header( ctx ))
      return FALSE;

   while (*ctx->cur != '\0') {
      uint label_val = 0;
      if (!eat_white( &ctx->cur )) {
         report_error( ctx, "Syntax error" );
         return FALSE;
      }

      if (*ctx->cur == '\0')
         break;
      if (parse_label( ctx, &label_val )) {
         if (!parse_instruction( ctx, TRUE ))
            return FALSE;
      }
      else if (str_match_no_case( &ctx->cur, "DCL" )) {
         if (!parse_declaration( ctx ))
            return FALSE;
      }
      else if (str_match_no_case( &ctx->cur, "IMM" )) {
         if (!parse_immediate( ctx ))
            return FALSE;
      }
      else if (str_match_no_case( &ctx->cur, "PROPERTY" )) {
         if (!parse_property( ctx ))
            return FALSE;
      }
      else if (!parse_instruction( ctx, FALSE )) {
         return FALSE;
      }
   }

   return TRUE;
}

boolean
tgsi_text_translate(
   const char *text,
   struct tgsi_token *tokens,
   uint num_tokens )
{
   struct translate_ctx ctx;

   ctx.text = text;
   ctx.cur = text;
   ctx.tokens = tokens;
   ctx.tokens_cur = tokens;
   ctx.tokens_end = tokens + num_tokens;

   if (!translate( &ctx ))
      return FALSE;

   return tgsi_sanity_check( tokens );
}
