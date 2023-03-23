/* udis86 - libudis86/udis86.c
 *
 * Copyright (c) 2002-2013 Vivek Thampi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimer in the documentation
 *       and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "udint.h"
#include "extern.h"
#include "decode.h"
#include <string.h>

#if HAVE_STRING_H
# include <string.h>
#endif

static void ud_inp_init(struct ud *u);

/* =============================================================================
 * ud_init
 *    Initializes ud_t object.
 * =============================================================================
 */
extern void
ud_init(struct ud* u)
{
  memset((void*)u, 0, sizeof(struct ud));
  ud_set_mode(u, 16);
  u->mnemonic = UD_Iinvalid;
  ud_set_pc(u, 0);
#ifndef __UD_STANDALONE__
  ud_set_input_file(u, stdin);
#endif /* __UD_STANDALONE__ */
  u->asm_buf = NULL;
  u->asm_buf_size = 0;
}

/* =============================================================================
 * ud_disassemble
 *    Disassembles one instruction and returns the number of
 *    bytes disassembled. A zero means end of disassembly.
 * =============================================================================
 */
extern unsigned int
ud_disassemble(struct ud* u)
{
  int len;
  if (u->inp_end) {
    return 0;
  }
  if ((len = ud_decode(u)) > 0) {
    if (u->translator != NULL) {
      u->asm_buf_int[0] = '\0';
      if (u->asm_buf && u->asm_buf_size > 0) {
          u->asm_buf[0] = '\0';
      }
      u->translator(u);
    }
  }
  return len;
}


/* =============================================================================
 * ud_set_mode() - Set Disassemly Mode.
 * =============================================================================
 */
extern void
ud_set_mode(struct ud* u, uint8_t m)
{
  switch(m) {
  case 16:
  case 32:
  case 64: u->dis_mode = m ; return;
  default: u->dis_mode = 16; return;
  }
}

/* =============================================================================
 * ud_set_vendor() - Set vendor.
 * =============================================================================
 */
extern void
ud_set_vendor(struct ud* u, unsigned v)
{
  switch(v) {
  case UD_VENDOR_INTEL:
    u->vendor = v;
    break;
  case UD_VENDOR_ANY:
    u->vendor = v;
    break;
  default:
    u->vendor = UD_VENDOR_AMD;
  }
}

/* =============================================================================
 * ud_set_pc() - Sets code origin.
 * =============================================================================
 */
extern void
ud_set_pc(struct ud* u, uint64_t o)
{
  u->pc = o;
}

/* =============================================================================
 * ud_set_syntax() - Sets the output syntax.
 * =============================================================================
 */
extern void
ud_set_syntax(struct ud* u, void (*t)(struct ud*))
{
  u->translator = t;
}

/* =============================================================================
 * ud_insn() - returns the disassembled instruction
 * =============================================================================
 */
const char*
ud_insn_asm(const struct ud* u)
{
  if (u->asm_buf != NULL) {
    return u->asm_buf;
  }
  return u->asm_buf_int;
}

/* =============================================================================
 * ud_insn_offset() - Returns the offset.
 * =============================================================================
 */
uint64_t
ud_insn_off(const struct ud* u)
{
  return u->insn_offset;
}


/* =============================================================================
 * ud_insn_hex() - Returns hex form of disassembled instruction.
 * =============================================================================
 */
const char*
ud_insn_hex(struct ud* u)
{
  u->insn_hexcode[0] = 0;
  if (!u->error) {
    unsigned int i;
    const unsigned char *src_ptr = ud_insn_ptr(u);
    char* src_hex;
    src_hex = (char*) u->insn_hexcode;
    /* for each byte used to decode instruction */
    for (i = 0; i < ud_insn_len(u) && i < sizeof(u->insn_hexcode) / 2;
         ++i, ++src_ptr) {
      sprintf(src_hex, "%02x", *src_ptr & 0xFF);
      src_hex += 2;
    }
  }
  return u->insn_hexcode;
}


/* =============================================================================
 * ud_insn_ptr
 *    Returns a pointer to buffer containing the bytes that were
 *    disassembled.
 * =============================================================================
 */
extern const uint8_t*
ud_insn_ptr(const struct ud* u)
{
  return (u->inp_buf == NULL) ?
            u->inp_sess : u->inp_buf + (u->inp_buf_index - u->inp_ctr);
}


/* =============================================================================
 * ud_insn_len
 *    Returns the count of bytes disassembled.
 * =============================================================================
 */
extern unsigned int
ud_insn_len(const struct ud* u)
{
  return (unsigned int) u->inp_ctr;
}


/* =============================================================================
 * ud_insn_get_opr
 *    Return the operand struct representing the nth operand of
 *    the currently disassembled instruction. Returns NULL if
 *    there's no such operand.
 * =============================================================================
 */
const struct ud_operand*
ud_insn_opr(const struct ud *u, unsigned int n)
{
  if (n > 3 || u->operand[n].type == UD_NONE) {
    return NULL;
  } else {
    return &u->operand[n];
  }
}


/* =============================================================================
 * ud_opr_is_sreg
 *    Returns non-zero if the given operand is of a segment register type.
 * =============================================================================
 */
int
ud_opr_is_sreg(const struct ud_operand *opr)
{
  return opr->type == UD_OP_REG &&
         opr->base >= UD_R_ES   &&
         opr->base <= UD_R_GS;
}


/* =============================================================================
 * ud_opr_is_sreg
 *    Returns non-zero if the given operand is of a general purpose
 *    register type.
 * =============================================================================
 */
int
ud_opr_is_gpr(const struct ud_operand *opr)
{
  return opr->type == UD_OP_REG &&
         opr->base >= UD_R_AL   &&
         opr->base <= UD_R_R15;
}


/* =============================================================================
 * ud_set_user_opaque_data
 * ud_get_user_opaque_data
 *    Get/set user opaqute data pointer
 * =============================================================================
 */
void
ud_set_user_opaque_data(struct ud * u, void* opaque)
{
  u->user_opaque_data = opaque;
}

void*
ud_get_user_opaque_data(const struct ud *u)
{
  return u->user_opaque_data;
}


/* =============================================================================
 * ud_set_asm_buffer
 *    Allow the user to set an assembler output buffer. If `buf` is NULL,
 *    we switch back to the internal buffer.
 * =============================================================================
 */
void
ud_set_asm_buffer(struct ud *u, char *buf, size_t size)
{
  if (buf == NULL) {
    ud_set_asm_buffer(u, u->asm_buf_int, sizeof(u->asm_buf_int));
  } else {
    u->asm_buf = buf;
    u->asm_buf_size = size;
  }
}


/* =============================================================================
 * ud_set_sym_resolver
 *    Set symbol resolver for relative targets used in the translation
 *    phase.
 *
 *    The resolver is a function that takes a uint64_t address and returns a
 *    symbolic name for the that address. The function also takes a second
 *    argument pointing to an integer that the client can optionally set to a
 *    non-zero value for offsetted targets. (symbol+offset) The function may
 *    also return NULL, in which case the translator only prints the target
 *    address.
 *
 *    The function pointer maybe NULL which resets symbol resolution.
 * =============================================================================
 */
void
ud_set_sym_resolver(struct ud *u, const char* (*resolver)(struct ud*,
                                                          uint64_t addr,
                                                          int64_t *offset))
{
  u->sym_resolver = resolver;
}


/* =============================================================================
 * ud_insn_mnemonic
 *    Return the current instruction mnemonic.
 * =============================================================================
 */
enum ud_mnemonic_code
ud_insn_mnemonic(const struct ud *u)
{
  return u->mnemonic;
}


/* =============================================================================
 * ud_lookup_mnemonic
 *    Looks up mnemonic code in the mnemonic string table.
 *    Returns NULL if the mnemonic code is invalid.
 * =============================================================================
 */
const char*
ud_lookup_mnemonic(enum ud_mnemonic_code c)
{
  if (c < UD_MAX_MNEMONIC_CODE) {
    return ud_mnemonics_str[c];
  } else {
    return NULL;
  }
}

/* =============================================================================
 * ud_lookup_eflags
 *    Looks up eflags information structure
 *    Returns NULL if invalid.
 * =============================================================================
 */
const struct ud_eflags*
ud_lookup_eflags(struct ud *u)
{
  if (u == NULL || u->itab_entry == NULL) {
    return NULL;
  } else {
    return &u->itab_entry->eflags;
  }
}

/* =============================================================================
 * ud_lookup_implicit_reg_used_list
 *    Returns the list of register implicitly used.
 *    The list is terminated by UD_NONE.
 *    Returns NULL if invalid.
 * =============================================================================
 */
const enum ud_type*
ud_lookup_implicit_reg_used_list(struct ud *u) {
  if (u == NULL || u->itab_entry == NULL) {
    return NULL;
  } else {
    return u->itab_entry->implicit_register_uses;
  }
}

/* =============================================================================
 * ud_lookup_implicit_reg_used_list
 *    Returns the list of register implicitly modified.
 *    The list is terminated by UD_NONE.
 *    Returns NULL if invalid.
 * =============================================================================
 */
const enum ud_type*
ud_lookup_implicit_reg_defined_list(struct ud *u) {
  if (u == NULL || u->itab_entry == NULL) {
    return NULL;
  } else {
    return u->itab_entry->implicit_register_defs;
  }
}

/*
 * ud_inp_init
 *    Initializes the input system.
 */
static void
ud_inp_init(struct ud *u)
{
  u->inp_hook      = NULL;
  u->inp_buf       = NULL;
  u->inp_buf_size  = 0;
  u->inp_buf_index = 0;
  u->inp_curr      = 0;
  u->inp_ctr       = 0;
  u->inp_end       = 0;
  u->inp_peek      = UD_EOI;
  UD_NON_STANDALONE(u->inp_file = NULL);
}


/* =============================================================================
 * ud_inp_set_hook
 *    Sets input hook.
 * =============================================================================
 */
void
ud_set_input_hook(register struct ud* u, int (*hook)(struct ud*))
{
  ud_inp_init(u);
  u->inp_hook = hook;
}

/* =============================================================================
 * ud_inp_set_buffer
 *    Set buffer as input.
 * =============================================================================
 */
void
ud_set_input_buffer(register struct ud* u, const uint8_t* buf, size_t len)
{
  ud_inp_init(u);
  u->inp_buf = buf;
  u->inp_buf_size = len;
  u->inp_buf_index = 0;
}


#ifndef __UD_STANDALONE__
/* =============================================================================
 * ud_input_set_file
 *    Set FILE as input.
 * =============================================================================
 */
static int
inp_file_hook(struct ud* u)
{
  return fgetc(u->inp_file);
}

void
ud_set_input_file(register struct ud* u, FILE* f)
{
  ud_inp_init(u);
  u->inp_hook = inp_file_hook;
  u->inp_file = f;
}
#endif /* __UD_STANDALONE__ */


/* =============================================================================
 * ud_input_skip
 *    Skip n input bytes.
 * ============================================================================
 */
void
ud_input_skip(struct ud* u, size_t n)
{
  if (u->inp_end) {
    return;
  }
  if (u->inp_buf == NULL) {
    while (n--) {
      int c = u->inp_hook(u);
      if (c == UD_EOI) {
        goto eoi;
      }
    }
    return;
  } else {
    if (n > u->inp_buf_size ||
        u->inp_buf_index > u->inp_buf_size - n) {
      u->inp_buf_index = u->inp_buf_size;
      goto eoi;
    }
    u->inp_buf_index += n;
    return;
  }
eoi:
  u->inp_end = 1;
  UDERR(u, "cannot skip, eoi received\b");
  return;
}


/* =============================================================================
 * ud_input_end
 *    Returns non-zero on end-of-input.
 * =============================================================================
 */
int
ud_input_end(const struct ud *u)
{
  return u->inp_end;
}

const char *ud_type_to_value(enum ud_type x)
{
  if (x == UD_NONE) return "UD_NONE";
  if (x == UD_R_AL) return "UD_R_AL";
  if (x == UD_R_CL) return "UD_R_CL";
  if (x == UD_R_DL) return "UD_R_DL";
  if (x == UD_R_BL) return "UD_R_BL";
  if (x == UD_R_AH) return "UD_R_AH";
  if (x == UD_R_CH) return "UD_R_CH";
  if (x == UD_R_DH) return "UD_R_DH";
  if (x == UD_R_BH) return "UD_R_BH";
  if (x == UD_R_SPL) return "UD_R_SPL";
  if (x == UD_R_BPL) return "UD_R_BPL";
  if (x == UD_R_SIL) return "UD_R_SIL";
  if (x == UD_R_DIL) return "UD_R_DIL";
  if (x == UD_R_R8B) return "UD_R_R8B";
  if (x == UD_R_R9B) return "UD_R_R9B";
  if (x == UD_R_R10B) return "UD_R_R10B";
  if (x == UD_R_R11B) return "UD_R_R11B";
  if (x == UD_R_R12B) return "UD_R_R12B";
  if (x == UD_R_R13B) return "UD_R_R13B";
  if (x == UD_R_R14B) return "UD_R_R14B";
  if (x == UD_R_R15B) return "UD_R_R15B";
  if (x == UD_R_AX) return "UD_R_AX";
  if (x == UD_R_CX) return "UD_R_CX";
  if (x == UD_R_DX) return "UD_R_DX";
  if (x == UD_R_BX) return "UD_R_BX";
  if (x == UD_R_SP) return "UD_R_SP";
  if (x == UD_R_BP) return "UD_R_BP";
  if (x == UD_R_SI) return "UD_R_SI";
  if (x == UD_R_DI) return "UD_R_DI";
  if (x == UD_R_R8W) return "UD_R_R8W";
  if (x == UD_R_R9W) return "UD_R_R9W";
  if (x == UD_R_R10W) return "UD_R_R10W";
  if (x == UD_R_R11W) return "UD_R_R11W";
  if (x == UD_R_R12W) return "UD_R_R12W";
  if (x == UD_R_R13W) return "UD_R_R13W";
  if (x == UD_R_R14W) return "UD_R_R14W";
  if (x == UD_R_R15W) return "UD_R_R15W";
  if (x == UD_R_EAX) return "UD_R_EAX";
  if (x == UD_R_ECX) return "UD_R_ECX";
  if (x == UD_R_EDX) return "UD_R_EDX";
  if (x == UD_R_EBX) return "UD_R_EBX";
  if (x == UD_R_ESP) return "UD_R_ESP";
  if (x == UD_R_EBP) return "UD_R_EBP";
  if (x == UD_R_ESI) return "UD_R_ESI";
  if (x == UD_R_EDI) return "UD_R_EDI";
  if (x == UD_R_R8D) return "UD_R_R8D";
  if (x == UD_R_R9D) return "UD_R_R9D";
  if (x == UD_R_R10D) return "UD_R_R10D";
  if (x == UD_R_R11D) return "UD_R_R11D";
  if (x == UD_R_R12D) return "UD_R_R12D";
  if (x == UD_R_R13D) return "UD_R_R13D";
  if (x == UD_R_R14D) return "UD_R_R14D";
  if (x == UD_R_R15D) return "UD_R_R15D";
  if (x == UD_R_RAX) return "UD_R_RAX";
  if (x == UD_R_RCX) return "UD_R_RCX";
  if (x == UD_R_RDX) return "UD_R_RDX";
  if (x == UD_R_RBX) return "UD_R_RBX";
  if (x == UD_R_RSP) return "UD_R_RSP";
  if (x == UD_R_RBP) return "UD_R_RBP";
  if (x == UD_R_RSI) return "UD_R_RSI";
  if (x == UD_R_RDI) return "UD_R_RDI";
  if (x == UD_R_R8) return "UD_R_R8";
  if (x == UD_R_R9) return "UD_R_R9";
  if (x == UD_R_R10) return "UD_R_R10";
  if (x == UD_R_R11) return "UD_R_R11";
  if (x == UD_R_R12) return "UD_R_R12";
  if (x == UD_R_R13) return "UD_R_R13";
  if (x == UD_R_R14) return "UD_R_R14";
  if (x == UD_R_R15) return "UD_R_R15";
  if (x == UD_R_ES) return "UD_R_ES";
  if (x == UD_R_CS) return "UD_R_CS";
  if (x == UD_R_SS) return "UD_R_SS";
  if (x == UD_R_DS) return "UD_R_DS";
  if (x == UD_R_FS) return "UD_R_FS";
  if (x == UD_R_GS) return "UD_R_GS";
  if (x == UD_R_CR0) return "UD_R_CR0";
  if (x == UD_R_CR1) return "UD_R_CR1";
  if (x == UD_R_CR2) return "UD_R_CR2";
  if (x == UD_R_CR3) return "UD_R_CR3";
  if (x == UD_R_CR4) return "UD_R_CR4";
  if (x == UD_R_CR5) return "UD_R_CR5";
  if (x == UD_R_CR6) return "UD_R_CR6";
  if (x == UD_R_CR7) return "UD_R_CR7";
  if (x == UD_R_CR8) return "UD_R_CR8";
  if (x == UD_R_CR9) return "UD_R_CR9";
  if (x == UD_R_CR10) return "UD_R_CR10";
  if (x == UD_R_CR11) return "UD_R_CR11";
  if (x == UD_R_CR12) return "UD_R_CR12";
  if (x == UD_R_CR13) return "UD_R_CR13";
  if (x == UD_R_CR14) return "UD_R_CR14";
  if (x == UD_R_CR15) return "UD_R_CR15";
  if (x == UD_R_DR0) return "UD_R_DR0";
  if (x == UD_R_DR1) return "UD_R_DR1";
  if (x == UD_R_DR2) return "UD_R_DR2";
  if (x == UD_R_DR3) return "UD_R_DR3";
  if (x == UD_R_DR4) return "UD_R_DR4";
  if (x == UD_R_DR5) return "UD_R_DR5";
  if (x == UD_R_DR6) return "UD_R_DR6";
  if (x == UD_R_DR7) return "UD_R_DR7";
  if (x == UD_R_DR8) return "UD_R_DR8";
  if (x == UD_R_DR9) return "UD_R_DR9";
  if (x == UD_R_DR10) return "UD_R_DR10";
  if (x == UD_R_DR11) return "UD_R_DR11";
  if (x == UD_R_DR12) return "UD_R_DR12";
  if (x == UD_R_DR13) return "UD_R_DR13";
  if (x == UD_R_DR14) return "UD_R_DR14";
  if (x == UD_R_DR15) return "UD_R_DR15";
  if (x == UD_R_MM0) return "UD_R_MM0";
  if (x == UD_R_MM1) return "UD_R_MM1";
  if (x == UD_R_MM2) return "UD_R_MM2";
  if (x == UD_R_MM3) return "UD_R_MM3";
  if (x == UD_R_MM4) return "UD_R_MM4";
  if (x == UD_R_MM5) return "UD_R_MM5";
  if (x == UD_R_MM6) return "UD_R_MM6";
  if (x == UD_R_MM7) return "UD_R_MM7";
  if (x == UD_R_ST0) return "UD_R_ST0";
  if (x == UD_R_ST1) return "UD_R_ST1";
  if (x == UD_R_ST2) return "UD_R_ST2";
  if (x == UD_R_ST3) return "UD_R_ST3";
  if (x == UD_R_ST4) return "UD_R_ST4";
  if (x == UD_R_ST5) return "UD_R_ST5";
  if (x == UD_R_ST6) return "UD_R_ST6";
  if (x == UD_R_ST7) return "UD_R_ST7";
  if (x == UD_R_XMM0) return "UD_R_XMM0";
  if (x == UD_R_XMM1) return "UD_R_XMM1";
  if (x == UD_R_XMM2) return "UD_R_XMM2";
  if (x == UD_R_XMM3) return "UD_R_XMM3";
  if (x == UD_R_XMM4) return "UD_R_XMM4";
  if (x == UD_R_XMM5) return "UD_R_XMM5";
  if (x == UD_R_XMM6) return "UD_R_XMM6";
  if (x == UD_R_XMM7) return "UD_R_XMM7";
  if (x == UD_R_XMM8) return "UD_R_XMM8";
  if (x == UD_R_XMM9) return "UD_R_XMM9";
  if (x == UD_R_XMM10) return "UD_R_XMM10";
  if (x == UD_R_XMM11) return "UD_R_XMM11";
  if (x == UD_R_XMM12) return "UD_R_XMM12";
  if (x == UD_R_XMM13) return "UD_R_XMM13";
  if (x == UD_R_XMM14) return "UD_R_XMM14";
  if (x == UD_R_XMM15) return "UD_R_XMM15";
  if (x == UD_R_XMM16) return "UD_R_XMM16";
  if (x == UD_R_XMM17) return "UD_R_XMM17";
  if (x == UD_R_XMM18) return "UD_R_XMM18";
  if (x == UD_R_XMM19) return "UD_R_XMM19";
  if (x == UD_R_XMM20) return "UD_R_XMM20";
  if (x == UD_R_XMM21) return "UD_R_XMM21";
  if (x == UD_R_XMM22) return "UD_R_XMM22";
  if (x == UD_R_XMM23) return "UD_R_XMM23";
  if (x == UD_R_XMM24) return "UD_R_XMM24";
  if (x == UD_R_XMM25) return "UD_R_XMM25";
  if (x == UD_R_XMM26) return "UD_R_XMM26";
  if (x == UD_R_XMM27) return "UD_R_XMM27";
  if (x == UD_R_XMM28) return "UD_R_XMM28";
  if (x == UD_R_XMM29) return "UD_R_XMM29";
  if (x == UD_R_XMM30) return "UD_R_XMM30";
  if (x == UD_R_XMM31) return "UD_R_XMM31";
  if (x == UD_R_YMM0) return "UD_R_YMM0";
  if (x == UD_R_YMM1) return "UD_R_YMM1";
  if (x == UD_R_YMM2) return "UD_R_YMM2";
  if (x == UD_R_YMM3) return "UD_R_YMM3";
  if (x == UD_R_YMM4) return "UD_R_YMM4";
  if (x == UD_R_YMM5) return "UD_R_YMM5";
  if (x == UD_R_YMM6) return "UD_R_YMM6";
  if (x == UD_R_YMM7) return "UD_R_YMM7";
  if (x == UD_R_YMM8) return "UD_R_YMM8";
  if (x == UD_R_YMM9) return "UD_R_YMM9";
  if (x == UD_R_YMM10) return "UD_R_YMM10";
  if (x == UD_R_YMM11) return "UD_R_YMM11";
  if (x == UD_R_YMM12) return "UD_R_YMM12";
  if (x == UD_R_YMM13) return "UD_R_YMM13";
  if (x == UD_R_YMM14) return "UD_R_YMM14";
  if (x == UD_R_YMM15) return "UD_R_YMM15";
  if (x == UD_R_YMM16) return "UD_R_YMM16";
  if (x == UD_R_YMM17) return "UD_R_YMM17";
  if (x == UD_R_YMM18) return "UD_R_YMM18";
  if (x == UD_R_YMM19) return "UD_R_YMM19";
  if (x == UD_R_YMM20) return "UD_R_YMM20";
  if (x == UD_R_YMM21) return "UD_R_YMM21";
  if (x == UD_R_YMM22) return "UD_R_YMM22";
  if (x == UD_R_YMM23) return "UD_R_YMM23";
  if (x == UD_R_YMM24) return "UD_R_YMM24";
  if (x == UD_R_YMM25) return "UD_R_YMM25";
  if (x == UD_R_YMM26) return "UD_R_YMM26";
  if (x == UD_R_YMM27) return "UD_R_YMM27";
  if (x == UD_R_YMM28) return "UD_R_YMM28";
  if (x == UD_R_YMM29) return "UD_R_YMM29";
  if (x == UD_R_YMM30) return "UD_R_YMM30";
  if (x == UD_R_YMM31) return "UD_R_YMM31";
  if (x == UD_R_ZMM0) return "UD_R_ZMM0";
  if (x == UD_R_ZMM1) return "UD_R_ZMM1";
  if (x == UD_R_ZMM2) return "UD_R_ZMM2";
  if (x == UD_R_ZMM3) return "UD_R_ZMM3";
  if (x == UD_R_ZMM4) return "UD_R_ZMM4";
  if (x == UD_R_ZMM5) return "UD_R_ZMM5";
  if (x == UD_R_ZMM6) return "UD_R_ZMM6";
  if (x == UD_R_ZMM7) return "UD_R_ZMM7";
  if (x == UD_R_ZMM8) return "UD_R_ZMM8";
  if (x == UD_R_ZMM9) return "UD_R_ZMM9";
  if (x == UD_R_ZMM10) return "UD_R_ZMM10";
  if (x == UD_R_ZMM11) return "UD_R_ZMM11";
  if (x == UD_R_ZMM12) return "UD_R_ZMM12";
  if (x == UD_R_ZMM13) return "UD_R_ZMM13";
  if (x == UD_R_ZMM14) return "UD_R_ZMM14";
  if (x == UD_R_ZMM15) return "UD_R_ZMM15";
  if (x == UD_R_ZMM16) return "UD_R_ZMM16";
  if (x == UD_R_ZMM17) return "UD_R_ZMM17";
  if (x == UD_R_ZMM18) return "UD_R_ZMM18";
  if (x == UD_R_ZMM19) return "UD_R_ZMM19";
  if (x == UD_R_ZMM20) return "UD_R_ZMM20";
  if (x == UD_R_ZMM21) return "UD_R_ZMM21";
  if (x == UD_R_ZMM22) return "UD_R_ZMM22";
  if (x == UD_R_ZMM23) return "UD_R_ZMM23";
  if (x == UD_R_ZMM24) return "UD_R_ZMM24";
  if (x == UD_R_ZMM25) return "UD_R_ZMM25";
  if (x == UD_R_ZMM26) return "UD_R_ZMM26";
  if (x == UD_R_ZMM27) return "UD_R_ZMM27";
  if (x == UD_R_ZMM28) return "UD_R_ZMM28";
  if (x == UD_R_ZMM29) return "UD_R_ZMM29";
  if (x == UD_R_ZMM30) return "UD_R_ZMM30";
  if (x == UD_R_ZMM31) return "UD_R_ZMM31";
  if (x == UD_R_K0) return "UD_R_K0";
  if (x == UD_R_K1) return "UD_R_K1";
  if (x == UD_R_K2) return "UD_R_K2";
  if (x == UD_R_K3) return "UD_R_K3";
  if (x == UD_R_K4) return "UD_R_K4";
  if (x == UD_R_K5) return "UD_R_K5";
  if (x == UD_R_K6) return "UD_R_K6";
  if (x == UD_R_K7) return "UD_R_K7";
  if (x == UD_R_BND0) return "UD_R_BND0";
  if (x == UD_R_BND1) return "UD_R_BND1";
  if (x == UD_R_BND2) return "UD_R_BND2";
  if (x == UD_R_BND3) return "UD_R_BND3";
  if (x == UD_R_RIP) return "UD_R_RIP";
  if (x == UD_OP_REG) return "UD_OP_REG";
  if (x == UD_OP_MEM) return "UD_OP_MEM";
  if (x == UD_OP_PTR) return "UD_OP_PTR";
  if (x == UD_OP_IMM) return "UD_OP_IMM";
  if (x == UD_OP_JIMM) return "UD_OP_JIMM";
  if (x == UD_OP_CONST) return "UD_OP_CONST";
  return "???";
}

/* vim:set ts=2 sw=2 expandtab */
