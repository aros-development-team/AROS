/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    NVK's OpenCL C kernels as NIR builder code; see nvkcl.h. Each function
    follows its .cl source statement by statement.
*/

#include "nvkcl.h"

#include "cl/nvk_copy_indirect.h"

#include "nv_push.h"
#include "nv_push_cl90b5.h"

#include "vulkan/vulkan_core.h"

/* uint i = get_sub_group_local_id() + cl_group_id.x * 32; */
static nir_def *
invocation_index(nir_builder *b)
{
   nir_def *group = nir_channel(b, nir_load_workgroup_id(b), 0);

   return nir_iadd(b, nir_load_subgroup_invocation(b),
                   nir_imul_imm(b, group, 32));
}

/* libcl_vk.h vk_write_query(): a 32- or 64-bit result at dst_addr[idx] */
static void
write_query(nir_builder *b, nir_def *dst_addr, nir_def *idx, nir_def *flags,
            nir_def *result)
{
   nir_def *is_64 = nir_test_mask(b, flags, VK_QUERY_RESULT_64_BIT);

   nir_push_if(b, is_64);
   {
      nir_def *addr = nir_iadd(b, dst_addr,
                               nir_u2u64(b, nir_imul_imm(b, idx, 8)));
      nir_store_global(b, result, addr, .write_mask = 0x1, .align_mul = 8);
   }
   nir_push_else(b, NULL);
   {
      nir_def *addr = nir_iadd(b, dst_addr,
                               nir_u2u64(b, nir_imul_imm(b, idx, 4)));
      nir_store_global(b, nir_u2u32(b, result), addr,
                       .write_mask = 0x1, .align_mul = 4);
   }
   nir_pop_if(b, NULL);
}

void
nvk_copy_queries(nir_builder *b, nir_def *pool_addr, nir_def *available_stride,
                 nir_def *reports_start, nir_def *report_count,
                 nir_def *query_stride, nir_def *first_query,
                 nir_def *query_count, nir_def *dst_addr, nir_def *dst_stride,
                 nir_def *flags)
{
   nir_def *i = invocation_index(b);

   nir_push_if(b, nir_ult(b, i, query_count));
   {
      nir_def *query = nir_iadd(b, first_query, i);

      nir_def *available_offs =
         nir_imul(b, nir_u2u64(b, query), nir_u2u64(b, available_stride));
      nir_def *available_u32 =
         nir_load_global(b, 1, 32, nir_iadd(b, pool_addr, available_offs),
                         .align_mul = 4);
      nir_def *available = nir_ine_imm(b, available_u32, 0);

      nir_def *report_offs =
         nir_iadd(b, nir_u2u64(b, reports_start),
                  nir_imul(b, nir_u2u64(b, query), nir_u2u64(b, query_stride)));
      nir_def *report = nir_iadd(b, pool_addr, report_offs);

      nir_def *dst_offset = nir_imul(b, dst_stride, nir_u2u64(b, i));
      nir_def *dst = nir_iadd(b, dst_addr, dst_offset);

      /* for (uint r = 0; r < report_count; ++r)
       *    vk_write_query(dst, r, flags, report[r * 2]);
       */
      nir_variable *r_var =
         nir_local_variable_create(b->impl, glsl_uint_type(), "r");
      nir_store_var(b, r_var, nir_imm_int(b, 0), 0x1);
      nir_push_loop(b);
      {
         nir_def *r = nir_load_var(b, r_var);

         nir_push_if(b, nir_uge(b, r, report_count));
         nir_jump(b, nir_jump_break);
         nir_pop_if(b, NULL);

         /* report[r * 2]: struct nvk_query_report { value; timestamp } */
         nir_def *addr = nir_iadd(b, report,
                                  nir_u2u64(b, nir_imul_imm(b, r, 16)));
         nir_def *value = nir_load_global(b, 1, 64, addr, .align_mul = 8);
         write_query(b, dst, r, flags, value);

         nir_store_var(b, r_var, nir_iadd_imm(b, r, 1), 0x1);
      }
      nir_pop_loop(b, NULL);

      nir_push_if(b, nir_test_mask(b, flags,
                                   VK_QUERY_RESULT_WITH_AVAILABILITY_BIT));
      write_query(b, dst, report_count, flags, nir_b2i64(b, available));
      nir_pop_if(b, NULL);
   }
   nir_pop_if(b, NULL);
}

void
nvk_copy_indirect(nir_builder *b, nir_def *in, nir_def *in_stride,
                  nir_def *out, nir_def *count)
{
   /* The method stream of one copy, with the five data words left zero:
    * the headers come from the same push macros the kernel uses, the data
    * is filled in by the shader.
    */
   uint32_t tmpl[NVK_COPY_INDIRECT_CMD_WORDS] = { 0 };
   struct nv_push push;
   struct nv_push *p = &push;

   nv_push_init(p, tmpl, ARRAY_SIZE(tmpl), BITFIELD_BIT(SUBC_NV90B5));

   P_MTHD(p, NV90B5, OFFSET_IN_UPPER);
   P_NV90B5_OFFSET_IN_UPPER(p, 0);
   P_NV90B5_OFFSET_IN_LOWER(p, 0);
   P_NV90B5_OFFSET_OUT_UPPER(p, 0);
   P_NV90B5_OFFSET_OUT_LOWER(p, 0);

   P_MTHD(p, NV90B5, LINE_LENGTH_IN);
   P_NV90B5_LINE_LENGTH_IN(p, 0);

   P_IMMD_WORD(p, NV90B5, LAUNCH_DMA, {
      .data_transfer_type = DATA_TRANSFER_TYPE_PIPELINED,
      .multi_line_enable = MULTI_LINE_ENABLE_FALSE,
      .flush_enable = FLUSH_ENABLE_TRUE,
      .src_memory_layout = SRC_MEMORY_LAYOUT_PITCH,
      .dst_memory_layout = DST_MEMORY_LAYOUT_PITCH,
      .remap_enable = REMAP_ENABLE_FALSE,
   });

   assert(nv_push_dw_count(p) == NVK_COPY_INDIRECT_CMD_WORDS);

   nir_def *i = invocation_index(b);

   nir_push_if(b, nir_ult(b, i, count));
   {
      /* VkCopyMemoryIndirectCommandKHR { srcAddress, dstAddress, size } */
      nir_def *src = nir_iadd(b, in, nir_imul(b, nir_u2u64(b, i), in_stride));
      nir_def *cmd = nir_load_global(b, 3, 64, src, .align_mul = 8);
      nir_def *src_addr = nir_channel(b, cmd, 0);
      nir_def *dst_addr = nir_channel(b, cmd, 1);
      nir_def *size = nir_u2u32(b, nir_channel(b, cmd, 2));

      /* OFFSET_*_UPPER carries 8 bits, as the push macro asserts */
      nir_def *src_hi = nir_iand_imm(b, nir_u2u32(b, nir_ushr_imm(b, src_addr, 32)), 0xff);
      nir_def *src_lo = nir_u2u32(b, src_addr);
      nir_def *dst_hi = nir_iand_imm(b, nir_u2u32(b, nir_ushr_imm(b, dst_addr, 32)), 0xff);
      nir_def *dst_lo = nir_u2u32(b, dst_addr);

      nir_def *words[NVK_COPY_INDIRECT_CMD_WORDS] = {
         nir_imm_int(b, tmpl[0]),
         src_hi,
         src_lo,
         dst_hi,
         dst_lo,
         nir_imm_int(b, tmpl[5]),
         size,
         nir_imm_int(b, tmpl[7]),
      };

      nir_def *dst = nir_iadd(b, out,
                              nir_u2u64(b, nir_imul_imm(b, i, NVK_COPY_INDIRECT_CMD_BYTES)));
      nir_store_global(b, nir_vec(b, &words[0], 4), dst,
                       .write_mask = 0xf, .align_mul = 4);
      nir_store_global(b, nir_vec(b, &words[4], 4), nir_iadd_imm(b, dst, 16),
                       .write_mask = 0xf, .align_mul = 4);
   }
   nir_pop_if(b, NULL);
}
