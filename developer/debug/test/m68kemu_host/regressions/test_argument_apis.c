/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Guest argument APIs: the actual GetArgStr and SetArgStr thunks and the
    memory accessors of rom/m68kemu, extracted by run_argument_apis.py, with
    controlled native collaborators.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef uintptr_t IPTR;
typedef int BOOL;
typedef void *APTR;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
#define TRUE 1
#define FALSE 0
#include "constants.inc"
struct M68KEmuContext { UBYTE *mem; ULONG mem_size,m68k_sysbase,m68k_argptr,m68k_argsize; BOOL low_guard; };
static ULONG cpu_d[8];
#define THUNK_D(n) (((ULONG *)cpu)[n])
static unsigned checks,failures,native_gets,native_sets,low_traps;
static LONG io_error;
static char native_initial[]="native args are separate";
static const char *native_args;
static LONG SetIoErr(LONG n) { LONG old=io_error;io_error=n;return old; }
static void m68k_low_write_trap(struct M68KEmuContext *ctx,ULONG addr,const char *why) { (void)ctx;(void)addr;(void)why;++low_traps; }
#include "accessors.inc"
static char *native_get(struct M68KEmuContext *ctx,void *cpu) { (void)ctx;(void)cpu;++native_gets;return (char *)native_args; }
static char *native_set(const char *s,void *cpu) { const char *old=native_args;(void)cpu;++native_sets;native_args=s;return (char *)old; }
#define GetArgStr() native_get(ctx,cpu)
#define SetArgStr(s) native_set((s),cpu)
#include "api.inc"
static void check(int ok,const char *label) { ++checks;if(!ok){if(failures<50)printf("FAIL: %s\n",label);++failures;} }
static void raw32(struct M68KEmuContext *ctx,ULONG a,ULONG v) { assert(a<=ctx->mem_size-4);ctx->mem[a]=(UBYTE)(v>>24);ctx->mem[a+1]=(UBYTE)(v>>16);ctx->mem[a+2]=(UBYTE)(v>>8);ctx->mem[a+3]=(UBYTE)v; }
static void reset(struct M68KEmuContext *ctx) {
    memset(ctx->mem,0,ctx->mem_size);ctx->m68k_sysbase=0x400;ctx->m68k_argptr=0x600;ctx->m68k_argsize=8;ctx->low_guard=TRUE;
    raw32(ctx,0x400+M68K_EXECBASE_THISTASK,0x800);raw32(ctx,0x800+M68K_PR_ARGUMENTS,0x600);
    memcpy(ctx->mem+0x600,"--help \n",9);memcpy(ctx->mem+0x700,"changed",8);
    native_args=native_initial;native_gets=native_sets=low_traps=0;io_error=73;memset(cpu_d,0,sizeof(cpu_d));
}
static IPTR get(struct M68KEmuContext *ctx) { return API_GET(ctx,cpu_d); }
static IPTR set(struct M68KEmuContext *ctx,ULONG s) {cpu_d[1]=s;return API_SET(ctx,cpu_d);}
static void test_semantics(struct M68KEmuContext *ctx) {
    unsigned before=failures;
    reset(ctx);check(get(ctx)==0x600,"GetArgStr returns guest Process pointer");check(io_error==73,"successful getter preserves IoErr");
    raw32(ctx,0x800+M68K_PR_ARGUMENTS,0x700);check(get(ctx)==0x700,"getter sees direct guest Process mutation");
    check(set(ctx,0x600)==0x700,"setter returns exact old guest pointer");check(m68k_read32(ctx,0x800+M68K_PR_ARGUMENTS)==0x600,"setter publishes new guest pointer");
    check(get(ctx)==0x600,"get after set preserves identity");check(set(ctx,0x600)==0x600,"aliased replacement returns same pointer");
    check(set(ctx,0)==0x600,"NULL replacement returns old pointer");check(get(ctx)==0,"NULL Process argument is legal");check(io_error==73,"successful setters preserve IoErr");
    check(set(ctx,0x700)==0,"setter from NULL returns NULL");check(get(ctx)==0x700,"setter from NULL publishes value");
    ctx->mem[4095]=0;check(set(ctx,4095)==0x700,"last byte empty string accepted");check(get(ctx)==4095,"last byte pointer identity retained");
    reset(ctx);raw32(ctx,0x400+M68K_EXECBASE_THISTASK,4096-(M68K_PR_ARGUMENTS+4));raw32(ctx,4092,0x600);
    check(get(ctx)==0x600,"Process argument field may end exactly at arena boundary");check(set(ctx,0x700)==0x600&&m68k_read32(ctx,4092)==0x700,"boundary setter writes one full field");
    printf("GROUP semantics: %u failed\n",failures-before);
}
static void test_bounds(struct M68KEmuContext *ctx) {
    unsigned before=failures,i;static const ULONG invalid[]={4096,UINT32_MAX,4095};
    for(i=0;i<3;i++) {
        reset(ctx);ctx->mem[4095]='x';raw32(ctx,0x800+M68K_PR_ARGUMENTS,invalid[i]);
        check(get(ctx)==0&&io_error==ERROR_BAD_NUMBER,"invalid or unterminated old string rejected");
        io_error=73;check(set(ctx,0x700)==0&&io_error==ERROR_BAD_NUMBER,"setter rejects invalid old string");
        check(m68k_read32(ctx,0x800+M68K_PR_ARGUMENTS)==invalid[i],"invalid-old rejection does not mutate Process");
        reset(ctx);ctx->mem[4095]='x';check(set(ctx,invalid[i])==0&&io_error==ERROR_BAD_NUMBER,"invalid or unterminated new string rejected");
        check(m68k_read32(ctx,0x800+M68K_PR_ARGUMENTS)==0x600,"invalid-new rejection leaves old pointer");
    }
    for(i=0;i<6;i++) {
        reset(ctx);
        if(i==0)ctx->m68k_sysbase=0;
        if(i==1)ctx->m68k_sysbase=UINT32_MAX-100;
        if(i==2)raw32(ctx,0x400+M68K_EXECBASE_THISTASK,0);
        if(i==3)raw32(ctx,0x400+M68K_EXECBASE_THISTASK,UINT32_MAX-100);
        if(i==4)raw32(ctx,0x400+M68K_EXECBASE_THISTASK,4096-M68K_PR_ARGUMENTS-3);
        if(i==5)ctx->m68k_sysbase=4096-M68K_EXECBASE_THISTASK-3;
        check(get(ctx)==0&&io_error==ERROR_BAD_NUMBER,"bad SysBase or Process range rejected");
        io_error=73;check(set(ctx,0x700)==0&&io_error==ERROR_BAD_NUMBER,"setter validates entire Process field before addition");
        check(!low_traps,"invalid range never reaches low-write trap");
    }
    reset(ctx);raw32(ctx,0x400+M68K_EXECBASE_THISTASK,0x300-M68K_PR_ARGUMENTS);raw32(ctx,0x300,0x600);
    check(set(ctx,0x700)==0&&io_error==ERROR_BAD_NUMBER,"protected low-memory field rejected before mutation");
    check(m68k_read32(ctx,0x300)==0x600&&!low_traps,"low-memory rejection retains data without a trap");
    printf("GROUP bounds: %u failed\n",failures-before);
}
static void test_ownership(struct M68KEmuContext *ctx) {
    unsigned before=failures,i;static const ULONG replacements[]={0x700,0,UINT32_MAX};
    for(i=0;i<3;i++) {
        reset(ctx);(void)get(ctx);(void)set(ctx,replacements[i]);
        check(native_args==native_initial,"guest setter never replaces native Process argument pointer");
        check(!native_gets&&!native_sets,"guest APIs never expose or call native argument accessors");
        check(ctx->m68k_argptr==0x600&&ctx->m68k_argsize==8,"startup registers remain immutable");
        check(!strcmp((char *)ctx->mem+0x600,"--help \n"),"startup argument bytes retain lifetime");
    }
    printf("GROUP ownership: %u failed\n",failures-before);
}
int main(void) {
    struct M68KEmuContext ctx={0};ctx.mem_size=4096;ctx.mem=malloc(ctx.mem_size);assert(ctx.mem);
    test_semantics(&ctx);test_bounds(&ctx);test_ownership(&ctx);free(ctx.mem);
    printf("argument APIs: %u checks, %u failed\n",checks,failures);return failures?1:0;
}
