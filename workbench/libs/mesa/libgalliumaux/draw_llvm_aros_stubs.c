/*
 * AROS weak fallbacks for Mesa's optional LLVM draw entry points.
 * The real implementations remain available to llvmpipe; the core library
 * uses these no-op fallbacks when it does not link the LLVM draw library.
 */

#include <stddef.h>
#include <stdint.h>

struct draw_llvm;
struct draw_context;
struct lp_context_ref;
struct draw_llvm_variant;
struct draw_pt_middle_end;
struct nir_shader;
struct tgsi_scan_info;

__attribute__((weak)) struct draw_llvm *
draw_llvm_create(struct draw_context *draw, struct lp_context_ref *ctx)
{
    (void)draw; (void)ctx;
    return NULL;
}

__attribute__((weak)) void
draw_llvm_destroy(struct draw_llvm *llvm)
{
    (void)llvm;
}

__attribute__((weak)) void
draw_llvm_set_sampler_state(struct draw_context *draw, int shader_stage)
{
    (void)draw; (void)shader_stage;
}

__attribute__((weak)) void
draw_llvm_set_mapped_texture(struct draw_context *draw, int shader_stage,
                             unsigned sview_idx, uint32_t width,
                             uint32_t height, uint32_t depth,
                             uint32_t first_level, uint32_t last_level,
                             uint32_t num_samples, uint32_t sample_stride,
                             const void *base_ptr, uint32_t *row_stride,
                             uint32_t *img_stride, uint32_t *mip_offsets)
{
    (void)draw; (void)shader_stage; (void)sview_idx;
    (void)width; (void)height; (void)depth; (void)first_level;
    (void)last_level; (void)num_samples; (void)sample_stride;
    (void)base_ptr; (void)row_stride; (void)img_stride; (void)mip_offsets;
}

__attribute__((weak)) void
draw_llvm_set_mapped_image(struct draw_context *draw, int shader_stage,
                           unsigned idx, uint32_t width, uint32_t height,
                           uint32_t depth, const void *base_ptr,
                           uint32_t row_stride, uint32_t img_stride,
                           uint32_t num_samples, uint32_t sample_stride)
{
    (void)draw; (void)shader_stage; (void)idx; (void)width;
    (void)height; (void)depth; (void)base_ptr; (void)row_stride;
    (void)img_stride; (void)num_samples; (void)sample_stride;
}

__attribute__((weak)) struct draw_llvm_variant *
draw_create_vs_llvm(struct draw_context *draw, const void *key)
{
    (void)draw; (void)key;
    return NULL;
}

__attribute__((weak)) void
draw_gs_llvm_destroy_variant(struct draw_llvm_variant *variant)
{
    (void)variant;
}

__attribute__((weak)) void
draw_pt_fetch_pipeline_or_emit_llvm(struct draw_context *draw,
                                    unsigned prim,
                                    struct draw_pt_middle_end *middle)
{
    (void)draw; (void)prim; (void)middle;
}

__attribute__((weak)) void
draw_tcs_llvm_destroy_variant(struct draw_llvm_variant *variant)
{
    (void)variant;
}

__attribute__((weak)) void
draw_tes_llvm_destroy_variant(struct draw_llvm_variant *variant)
{
    (void)variant;
}

__attribute__((weak)) void
nir_tgsi_scan_shader(struct nir_shader *nir, struct tgsi_scan_info *info)
{
    (void)nir; (void)info;
}

__attribute__((weak)) void p_tess_init(void) {}
__attribute__((weak)) void p_tess_destroy(void) {}

__attribute__((weak)) uint32_t
p_tessellate(const void *key, const uint32_t *in_vertices)
{
    (void)key; (void)in_vertices;
    return 0;
}
