/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    AllocBitMap result projection: the actual manual allocation thunk, shadow
    factory and registry, bitmap converters and generated FreeBitMap consumer
    of rom/m68kemu, extracted by run_bitmap_result.py, over a controlled
    native allocation and guest heap.
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
typedef uint64_t IPTR;
typedef uint64_t BPTR;
typedef int BOOL;
typedef void *APTR;
typedef UBYTE *PLANEPTR;
#define TRUE 1
#define FALSE 0
#define BNULL ((BPTR)0)
#define MKBADDR(p) ((BPTR)(uintptr_t)(p))
#define BADDR(p) ((void *)(uintptr_t)(p))
#include "constants.inc"
struct BitMap { UWORD BytesPerRow,Rows;UBYTE Flags,Depth;UWORD pad;PLANEPTR Planes[8]; };
struct RastPort { struct BitMap *BitMap; };
struct Screen {struct RastPort RastPort;};
struct DrawInfo {int unused;};struct TextFont {int unused;};
struct M68KBitMapView {ULONG bpr,rows,depth;ULONG planes[8];};
struct M68KFieldMap {int unused;};
struct M68KStructLayout {const char *name;UWORD m68k_size,shadow_type;const struct M68KFieldMap *fields;};
struct M68KEmuContext {
 UBYTE *mem;ULONG mem_size,bump;UWORD num_shadows;BOOL shadow_full_logged;
 struct {ULONG m68k_addr;void *native_ptr;UWORD type;} shadow_map[M68KEMU_MAX_SHADOWS];
 BPTR *handles;ULONG num_handles;UWORD num_libs;struct {ULONG m68k_addr;} libs[4];char program_name[32];
};
static unsigned checks,failures,alloc_calls,native_frees,bad_frees,live_native,guest_alloc_calls,guest_frees,syncs,logs;
static BOOL alloc_fail,heap_fail,missing_layout;
static struct BitMap actual_bitmap,friend_bitmap,screen_bitmap;
static struct Screen screen;
static struct BitMap *allocation_result,*last_freed,*seen_friend;
static struct BitMap friend_snapshot;
static ULONG seen_width,seen_height,seen_depth,seen_flags;
static struct M68KStructLayout bitmap_layout={"BitMap",BITMAP_GUEST_SIZE,BITMAP_SHADOW_TYPE,NULL};
static void fixture_bug(const char *fmt,...) {(void)fmt;}
#define bug(...) fixture_bug(__VA_ARGS__)
#define D(x) do {x;} while(0)
static void m68k_log(struct M68KEmuContext *ctx,const char *fmt,...) {(void)ctx;(void)fmt;++logs;}
static void *AllocVec(size_t n,ULONG flags) {(void)flags;void *p=calloc(1,n);assert(p);return p;}
static ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx,ULONG n,ULONG flags) {
 ULONG p=ctx->bump;(void)flags;++guest_alloc_calls;if(heap_fail)return 0;n=(n+3)&~3u;assert(p+n<=ctx->mem_size);ctx->bump+=n;return p;
}
static void M68KEmu_HeapFree(struct M68KEmuContext *ctx,ULONG p,ULONG n) {(void)ctx;assert(p&&n==BITMAP_GUEST_SIZE);++guest_frees;}
BPTR m68k_bptr_in(struct M68KEmuContext *ctx,ULONG value);
#include "accessors.inc"
#include "registry.inc"
static const struct M68KStructLayout *shadow_find_layout(const char *name) {assert(!strcmp(name,"BitMap"));return missing_layout?NULL:&bitmap_layout;}
static void sync_fields(struct M68KEmuContext *ctx,const struct M68KFieldMap *f,ULONG p,void *native) {(void)ctx;(void)f;assert(p&&native==allocation_result);++syncs;}
static void m68k_screen_extras(struct M68KEmuContext *ctx,ULONG p,struct Screen *s) {(void)ctx;(void)p;(void)s;assert(0);}
static void m68k_drawinfo_extras(struct M68KEmuContext *ctx,ULONG p,struct DrawInfo *s) {(void)ctx;(void)p;(void)s;assert(0);}
static void m68k_textfont_extras(struct M68KEmuContext *ctx,ULONG p,struct TextFont *s) {(void)ctx;(void)p;(void)s;assert(0);}
#include "factory.inc"
#include "bitmap-consumer.inc"
struct Cpu {ULONG d[8],a[8];};
#define THUNK_D(n) (((struct Cpu *)cpu)->d[n])
#define THUNK_A(n) (((struct Cpu *)cpu)->a[n])
static struct BitMap *record_alloc(ULONG w,ULONG h,ULONG depth,ULONG flags,struct BitMap *friend) {
 ++alloc_calls;seen_width=w;seen_height=h;seen_depth=depth;seen_flags=flags;seen_friend=friend;
 if(friend)friend_snapshot=*friend;
 if(alloc_fail)return NULL;
 ++live_native;return allocation_result;
}
static void record_free(struct BitMap *b,void *cpu) {(void)cpu;++native_frees;last_freed=b;if(b==allocation_result&&live_native)--live_native;else ++bad_frees;}
#define AllocBitMap(w,h,d,f,b) record_alloc((w),(h),(d),(f),(b))
#define FreeBitMap(b) record_free((b),cpu)
#include "allocation.inc"
#include "free-consumer.inc"
static void check(int ok,const char *label) {++checks;if(!ok){if(failures<80)printf("FAIL: %s\n",label);++failures;}}
static void put16(struct M68KEmuContext *ctx,ULONG p,UWORD v) {ctx->mem[p]=(UBYTE)(v>>8);ctx->mem[p+1]=(UBYTE)v;}
static void put32(struct M68KEmuContext *ctx,ULONG p,ULONG v) {ctx->mem[p]=(UBYTE)(v>>24);ctx->mem[p+1]=(UBYTE)(v>>16);ctx->mem[p+2]=(UBYTE)(v>>8);ctx->mem[p+3]=(UBYTE)v;}
static void reset(struct M68KEmuContext *ctx) {
 free(ctx->handles);memset(ctx->mem,0,ctx->mem_size);memset(ctx->shadow_map,0,sizeof(ctx->shadow_map));ctx->handles=NULL;ctx->num_handles=ctx->num_shadows=ctx->num_libs=0;ctx->shadow_full_logged=FALSE;ctx->bump=0x1000;ctx->program_name[0]=0;
 alloc_calls=native_frees=bad_frees=live_native=guest_alloc_calls=guest_frees=syncs=logs=0;alloc_fail=heap_fail=missing_layout=FALSE;
 allocation_result=&actual_bitmap;last_freed=seen_friend=NULL;memset(&friend_snapshot,0,sizeof(friend_snapshot));memset(&friend_bitmap,0,sizeof(friend_bitmap));screen.RastPort.BitMap=&screen_bitmap;
}
static ULONG prepare_friend(struct M68KEmuContext *ctx,unsigned mode) {
 switch(mode) {
 case 0:return 0;
 case 1:assert(shadow_register(ctx,0x8000,&friend_bitmap,BITMAP_SHADOW_TYPE));return 0x8000;
 case 2:assert(shadow_register(ctx,0x9000,&screen,5));return 0x9000+M68K_SCR_BITMAP;
 case 3:return m68k_bptr_out(ctx,(BPTR)(uintptr_t)&friend_bitmap);
 default:put16(ctx,0xA000,4);put16(ctx,0xA002,10);ctx->mem[0xA005]=2;put32(ctx,0xA008,0xB000);put32(ctx,0xA00C,UINT32_MAX);return 0xA000;
 }
}
static int friend_correct(struct M68KEmuContext *ctx,unsigned mode) {
 if(mode==0)return !seen_friend;
 if(mode==1||mode==3)return seen_friend==&friend_bitmap;
 if(mode==2)return seen_friend==&screen_bitmap;
 return seen_friend&&friend_snapshot.BytesPerRow==4&&friend_snapshot.Rows==10&&friend_snapshot.Depth==2&&friend_snapshot.Planes[0]==ctx->mem+0xB000&&friend_snapshot.Planes[1]==(PLANEPTR)-1;
}
static struct Cpu call_regs(void) {struct Cpu c={0};c.d[0]=820;c.d[1]=560;c.d[2]=8;c.d[3]=17;return c;}
static void test_contract(struct M68KEmuContext *ctx) {
 unsigned before=failures,mode,kind;
 for(mode=0;mode<5;mode++)for(kind=0;kind<3;kind++) {
  struct Cpu cpu=call_regs();struct BitMap scratch;IPTR result;UWORD initial;
  reset(ctx);if(kind==1)allocation_result=(struct BitMap *)(uintptr_t)0x0125fed0;if(kind==2)allocation_result=(struct BitMap *)(uintptr_t)UINT64_C(0x123400000125fed0);
  cpu.a[0]=prepare_friend(ctx,mode);initial=ctx->num_shadows;
  result=thunk_gfx_AllocBitMap_m(ctx,&cpu);
  check(result&&result<ctx->mem_size&&shadow_lookup(ctx,(ULONG)result)==allocation_result,"allocation publishes registered guest bitmap, never raw native pointer");
  check(m68k_bitmap_in(ctx,(ULONG)result,&scratch)==allocation_result,"own RastPort bitmap consumer resolves exact native allocation");
  check(alloc_calls==1&&seen_width==820&&seen_height==560&&seen_depth==8&&seen_flags==17,"native allocation dimensions depth and flags unchanged");
  check(friend_correct(ctx,mode),"null shadow screen handle and guest-planar friend conversion retained");
  check(!native_frees&&live_native==1,"successful result retains native allocation for caller");
  check(ctx->num_shadows==initial+1&&guest_alloc_calls==1&&syncs==1,"actual shadow factory registers and syncs exactly once");
  cpu.a[0]=(ULONG)result;(void)thunk_graphics_FreeBitMap(ctx,&cpu);
  check(native_frees==1&&last_freed==allocation_result&&!bad_frees&&!live_native,"existing generated FreeBitMap consumer receives original native bitmap");
 }
 printf("GROUP contract: %u failed\n",failures-before);
}
static void test_failures(struct M68KEmuContext *ctx) {
 unsigned before=failures;struct Cpu cpu=call_regs();IPTR result;
 reset(ctx);alloc_fail=TRUE;result=thunk_gfx_AllocBitMap_m(ctx,&cpu);
 check(!result,"native allocation failure returns zero");check(alloc_calls==1&&!live_native&&!native_frees,"native failure owns nothing to free");check(!guest_alloc_calls&&!ctx->num_shadows,"native failure never creates a shadow");
 reset(ctx);heap_fail=TRUE;result=thunk_gfx_AllocBitMap_m(ctx,&cpu);
 check(!result,"guest allocation failure returns zero");check(native_frees==1&&!live_native&&!bad_frees,"guest allocation failure releases native bitmap once");check(guest_alloc_calls==1&&!guest_frees,"failed guest allocation has no guest block to release");check(!ctx->num_shadows,"failed guest allocation registers nothing");
 reset(ctx);ctx->num_shadows=M68KEMU_MAX_SHADOWS;result=thunk_gfx_AllocBitMap_m(ctx,&cpu);
 check(!result,"full shadow table returns zero");check(native_frees==1&&!live_native&&!bad_frees,"full shadow table releases native bitmap once");check(guest_alloc_calls==1&&guest_frees==1,"actual factory rolls back its guest block");check(ctx->num_shadows==M68KEMU_MAX_SHADOWS,"full shadow table remains unchanged");check(logs==1&&ctx->shadow_full_logged,"real full-table refusal remains observable");
 reset(ctx);missing_layout=TRUE;result=thunk_gfx_AllocBitMap_m(ctx,&cpu);
 check(!result,"missing bitmap layout returns zero");check(native_frees==1&&!live_native&&!bad_frees,"missing layout releases native bitmap once");check(!guest_alloc_calls,"missing layout does not allocate a guest block");check(!ctx->num_shadows,"missing layout registers nothing");
 printf("GROUP failures: %u failed\n",failures-before);
}
int main(void) {
 struct M68KEmuContext ctx={0};ctx.mem_size=32u*1024u*1024u;ctx.mem=calloc(1,ctx.mem_size);assert(ctx.mem);
 test_contract(&ctx);test_failures(&ctx);free(ctx.handles);free(ctx.mem);
 printf("bitmap result: %u checks, %u failed\n",checks,failures);return failures?1:0;
}
