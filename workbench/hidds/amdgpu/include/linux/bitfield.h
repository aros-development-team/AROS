/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BITFIELD_H_
#define _LINUX_BITFIELD_H_

#include <linux/build_bug.h>
#include <linux/bitops.h>

#define __bf_shf(x)             (__builtin_ffsll(x) - 1)
#define FIELD_MAX(_mask)        ({ (typeof(_mask))((_mask) >> __bf_shf(_mask)); })
#define FIELD_FIT(_mask, _val)  ({ !((((typeof(_mask))_val) << __bf_shf(_mask)) & ~(_mask)); })
#define FIELD_PREP(_mask, _val) ({ ((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask); })
#define FIELD_GET(_mask, _reg)  ({ (typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask)); })
#define FIELD_PREP_CONST(_mask, _val) (((_val) << __bf_shf(_mask)) & (_mask))
#define FIELD_MODIFY(_mask, _reg_p, _val) do { *(_reg_p) &= ~(_mask); *(_reg_p) |= FIELD_PREP(_mask, _val); } while (0)

#define __MAKE_OP(size)                                                 \
static inline u##size le##size##_encode_bits(u##size v, u##size field)  \
{ return (v << __bf_shf(field)) & field; }                              \
static inline u##size be##size##_encode_bits(u##size v, u##size field)  \
{ return (v << __bf_shf(field)) & field; }                              \
static inline u##size u##size##_encode_bits(u##size v, u##size field)   \
{ return (v << __bf_shf(field)) & field; }                              \
static inline u##size u##size##_get_bits(u##size v, u##size field)      \
{ return (v & field) >> __bf_shf(field); }                              \
static inline void u##size##_replace_bits(u##size *p, u##size val, u##size field) \
{ *p = (*p & ~field) | ((val << __bf_shf(field)) & field); }
__MAKE_OP(16)
__MAKE_OP(32)
__MAKE_OP(64)
#undef __MAKE_OP
static inline u8 u8_encode_bits(u8 v, u8 field) { return (v << __bf_shf(field)) & field; }
static inline u8 u8_get_bits(u8 v, u8 field) { return (v & field) >> __bf_shf(field); }
static inline void u8_replace_bits(u8 *p, u8 val, u8 field) { *p = (*p & ~field) | ((val << __bf_shf(field)) & field); }

#endif /* _LINUX_BITFIELD_H_ */
