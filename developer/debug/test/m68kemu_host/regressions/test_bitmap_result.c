/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    AllocBitMap result projection and BitMap lifecycle: the actual manual
    allocation thunk, the FreeBitMap thunk registered for LVO -924, the shadow
    factory, destructor and registry and the bitmap converters of rom/m68kemu,
    extracted by run_bitmap_result.py, over a controlled native allocation
    and a guest heap that records every live block.
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
 struct {ULONG m68k_addr;void *native_ptr;UWORD type;UBYTE streams_valid;BPTR stream_native[3];ULONG stream_handle[3];} shadow_map[M68KEMU_MAX_SHADOWS];
 BPTR *handles;ULONG num_handles;UWORD num_libs;struct {ULONG m68k_addr;} libs[4];char program_name[32];
};
static unsigned checks,failures,alloc_calls,native_frees,bad_frees,live_native,guest_alloc_calls,guest_frees,syncs,logs;
static BOOL alloc_fail,heap_fail,missing_layout;
static struct BitMap actual_bitmap,friend_bitmap,screen_bitmap;
static struct Screen screen;
static struct BitMap *allocation_result,*last_freed,*seen_friend;
/* The guest heap records its live blocks. The native side hands out the one
   configured bitmap (pool_mode 0), a distinct bitmap of a pool per call (1)
   or the same pool bitmap again (2), and counts each bitmap's releases. */
#define LIVE_MAX 2048
#define POOL_SIZE 4200
static ULONG live_addr[LIVE_MAX],live_size[LIVE_MAX];
static unsigned live_blocks,live_bytes,bad_guest_frees,pool_mode,pool_next;
static struct BitMap pool[POOL_SIZE];
static UBYTE pool_out[POOL_SIZE],pool_released[POOL_SIZE];
static struct BitMap *last_alloc;
/* The native DOS object behind an ExAll control, and what was released as one. */
struct ExAllControl {int unused;};
static struct ExAllControl exall_native;
static unsigned dos_allocs,dos_frees;
static void *dos_freed;
static ULONG dos_freed_type;
static struct BitMap friend_snapshot;
static ULONG seen_width,seen_height,seen_depth,seen_flags;
static struct M68KStructLayout bitmap_layout={"BitMap",BITMAP_GUEST_SIZE,BITMAP_SHADOW_TYPE,NULL};
static void fixture_bug(const char *fmt,...) {(void)fmt;}
#define bug(...) fixture_bug(__VA_ARGS__)
#define D(x) do {x;} while(0)
static void m68k_log(struct M68KEmuContext *ctx,const char *fmt,...) {(void)ctx;(void)fmt;++logs;}
static void *AllocVec(size_t n,ULONG flags) {(void)flags;void *p=calloc(1,n);assert(p);return p;}
static ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx,ULONG n,ULONG flags) {
 ULONG p=ctx->bump;(void)flags;++guest_alloc_calls;if(heap_fail)return 0;n=(n+3)&~3u;assert(p+n<=ctx->mem_size);ctx->bump+=n;
 assert(live_blocks<LIVE_MAX);live_addr[live_blocks]=p;live_size[live_blocks]=n;++live_blocks;live_bytes+=n;return p;
}
static void M68KEmu_HeapFree(struct M68KEmuContext *ctx,ULONG p,ULONG n) {
 unsigned i=0;(void)ctx;assert(p&&(pool_mode||n==BITMAP_GUEST_SIZE));++guest_frees;
 while(i<live_blocks&&live_addr[i]!=p)++i;
 /* a block that is not live, or freed with another size than it was allocated with: bad, and it stays */
 if(i==live_blocks||live_size[i]!=((n+3)&~3u)){++bad_guest_frees;return;}
 live_bytes-=live_size[i];--live_blocks;live_addr[i]=live_addr[live_blocks];live_size[i]=live_size[live_blocks];
}
BPTR m68k_bptr_in(struct M68KEmuContext *ctx,ULONG value);
#include "accessors.inc"
#include "registry.inc"
static const struct M68KStructLayout *shadow_find_layout(const char *name) {assert(!strcmp(name,"BitMap"));return missing_layout?NULL:&bitmap_layout;}
static void sync_fields(struct M68KEmuContext *ctx,const struct M68KFieldMap *f,ULONG p,void *native) {(void)ctx;(void)f;assert(p&&native==(void *)last_alloc);++syncs;}
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
 if(pool_mode){unsigned k=pool_mode==1?pool_next++:0;assert(k<POOL_SIZE&&!pool_out[k]);pool_out[k]=1;last_alloc=&pool[k];}
 else last_alloc=allocation_result;
 ++live_native;return last_alloc;
}
static void record_free(struct BitMap *b,void *cpu) {
 uintptr_t at=(uintptr_t)b,first=(uintptr_t)pool;(void)cpu;++native_frees;last_freed=b;
 if(pool_mode) {
  if(at>=first&&at<first+sizeof(pool)&&(at-first)%sizeof(pool[0])==0&&pool_out[(at-first)/sizeof(pool[0])]) {
   size_t k=(at-first)/sizeof(pool[0]);pool_out[k]=0;if(pool_released[k]<255)++pool_released[k];if(live_native)--live_native;
  }
  else ++bad_frees;
 }
 else if(b==allocation_result&&live_native)--live_native;else ++bad_frees;
}
#define AllocBitMap(w,h,d,f,b) record_alloc((w),(h),(d),(f),(b))
#define FreeBitMap(b) record_free((b),cpu)
#include "allocation.inc"
#include "free-consumer.inc"
static void *record_dos_alloc(ULONG type,void *tags) {(void)tags;assert(type==DOS_EXALLCONTROL);++dos_allocs;return &exall_native;}
static void record_dos_free(ULONG type,void *object) {++dos_frees;dos_freed_type=type;dos_freed=object;}
#define AllocDosObject(type,tags) record_dos_alloc((type),(tags))
#define FreeDosObject(type,object) record_dos_free((type),(object))
#define THUNK_PTR(n) m68k_to_host_or_shadow(ctx,THUNK_A(n))
#include "dos-objects.inc"
static void check(int ok,const char *label) {++checks;if(!ok){if(failures<80)printf("FAIL: %s\n",label);++failures;}}
static void put16(struct M68KEmuContext *ctx,ULONG p,UWORD v) {ctx->mem[p]=(UBYTE)(v>>8);ctx->mem[p+1]=(UBYTE)v;}
static void put32(struct M68KEmuContext *ctx,ULONG p,ULONG v) {ctx->mem[p]=(UBYTE)(v>>24);ctx->mem[p+1]=(UBYTE)(v>>16);ctx->mem[p+2]=(UBYTE)(v>>8);ctx->mem[p+3]=(UBYTE)v;}
static void reset(struct M68KEmuContext *ctx) {
 free(ctx->handles);memset(ctx->mem,0,ctx->mem_size);memset(ctx->shadow_map,0,sizeof(ctx->shadow_map));ctx->handles=NULL;ctx->num_handles=ctx->num_shadows=ctx->num_libs=0;ctx->shadow_full_logged=FALSE;ctx->bump=0x1000;ctx->program_name[0]=0;
 alloc_calls=native_frees=bad_frees=live_native=guest_alloc_calls=guest_frees=syncs=logs=0;alloc_fail=heap_fail=missing_layout=FALSE;
 allocation_result=&actual_bitmap;last_freed=seen_friend=NULL;memset(&friend_snapshot,0,sizeof(friend_snapshot));memset(&friend_bitmap,0,sizeof(friend_bitmap));screen.RastPort.BitMap=&screen_bitmap;
 live_blocks=live_bytes=bad_guest_frees=pool_mode=pool_next=0;memset(pool_out,0,sizeof(pool_out));memset(pool_released,0,sizeof(pool_released));last_alloc=NULL;
 dos_allocs=dos_frees=0;dos_freed=NULL;dos_freed_type=0;
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
  cpu.a[0]=(ULONG)result;(void)API_FREE(ctx,&cpu);
  check(native_frees==1&&last_freed==allocation_result&&!bad_frees&&!live_native,"FreeBitMap thunk registered for LVO -924 receives original native bitmap");
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
static IPTR alloc_one(struct M68KEmuContext *ctx) {struct Cpu cpu=call_regs();return thunk_gfx_AllocBitMap_m(ctx,&cpu);}
static void free_one(struct M68KEmuContext *ctx,ULONG guest) {struct Cpu cpu=call_regs();cpu.a[0]=guest;(void)API_FREE(ctx,&cpu);}
static void dos_free_one(struct M68KEmuContext *ctx,ULONG guest) {struct Cpu cpu=call_regs();cpu.d[1]=DOS_EXALLCONTROL;cpu.d[2]=guest;(void)thunk_dos_FreeDosObject(ctx,&cpu);}
static unsigned released_wrong(unsigned count) {unsigned i,wrong=0;for(i=0;i<count;i++)if(pool_out[i]||pool_released[i]!=1)++wrong;return wrong;}
/* No check stands inside a loop here: a loop feeds counters, and each
   counter is checked once. */
static void test_lifecycle(struct M68KEmuContext *ctx) {
 static ULONG guest[1100];static struct BitMap *native[1100];
 unsigned before=failures,t,i,k,base,refused,unmapped,occupied,leaked,stale,survivors_wrong,lo,hi,alias=0,declined=0,owner=0;
 int moved,clean;ULONG g,h;IPTR r;struct BitMap scratch,*n;UWORD slot;
 /* more turns than the registry has entries, a distinct native bitmap each */
 reset(ctx);pool_mode=1;if(!shadow_register(ctx,0x9000,&screen,5)||!shadow_register(ctx,0x9800,&friend_bitmap,0))abort();
 base=ctx->num_shadows;refused=unmapped=occupied=leaked=stale=0;
 for(t=0;t<4096;t++) {
  r=alloc_one(ctx);if(!r){++refused;continue;}
  if(shadow_lookup(ctx,(ULONG)r)!=(void *)last_alloc)++unmapped;
  free_one(ctx,(ULONG)r);
  if(ctx->num_shadows!=base)++occupied;
  if(live_blocks||live_bytes)++leaked;
  if(shadow_lookup(ctx,(ULONG)r))++stale;
 }
 check(!refused&&alloc_calls==4096,"4096 allocations, four times the registry, all succeed");
 check(!unmapped,"each allocation is registered for its own native bitmap");
 check(!occupied&&ctx->num_shadows==base,"the registry is at its baseline after every free");
 check(!leaked&&guest_alloc_calls==4096&&guest_frees==4096&&!bad_guest_frees,"every guest BitMap block is released: live blocks and bytes at their baseline after every free");
 check(!stale,"a freed guest address resolves to no native bitmap");
 check(native_frees==4096&&!bad_frees&&!live_native&&!released_wrong(4096),"each native bitmap is released exactly once");
 check(!logs&&!ctx->shadow_full_logged,"the full-registry refusal is never logged");
 /* the same native address comes back every time */
 reset(ctx);pool_mode=2;base=ctx->num_shadows;refused=occupied=leaked=stale=0;
 for(t=0;t<1100;t++) {
  r=alloc_one(ctx);if(!r){++refused;continue;}
  free_one(ctx,(ULONG)r);
  if(ctx->num_shadows!=base)++occupied;
  if(live_blocks||live_bytes)++leaked;
  if(shadow_lookup(ctx,(ULONG)r))++stale;
 }
 check(!refused&&alloc_calls==1100&&native_frees==1100&&!bad_frees&&!live_native,"1100 turns with a native address that returns: each allocates and releases once");
 check(!occupied&&!leaked&&!stale&&guest_alloc_calls==1100&&guest_frees==1100&&!bad_guest_frees,"a native address that returns leaves no entry and no guest block behind");
 /* 1000 alive together, freed first-in, last-in and alternating */
 reset(ctx);pool_mode=1;if(!shadow_register(ctx,0x9000,&screen,5))abort();
 base=ctx->num_shadows;refused=stale=survivors_wrong=0;
 for(i=0;i<1000;i++){r=alloc_one(ctx);if(!r)++refused;guest[i]=(ULONG)r;native[i]=last_alloc;}
 check(!refused&&ctx->num_shadows==base+1000&&live_blocks==1000,"1000 bitmaps are alive together beside another entry");
 lo=0;hi=1000;
 for(t=0;lo<hi;t++) {
  if(t<333)i=lo++;else if(t<666)i=--hi;else if(t&1)i=lo++;else i=--hi;
  free_one(ctx,guest[i]);
  if(shadow_lookup(ctx,guest[i]))++stale;
  /* every bitmap still alive, after every free: the ones from lo to hi-1 */
  for(k=lo;k<hi;k++)if(shadow_lookup(ctx,guest[k])!=(void *)native[k])++survivors_wrong;
  if(shadow_lookup(ctx,0x9000)!=(void *)&screen)++survivors_wrong;
 }
 check(!survivors_wrong,"after each free every live bitmap and the other entry still resolve to their own native objects");
 check(!stale&&ctx->num_shadows==base&&!live_blocks&&!live_bytes&&!bad_guest_frees,"all freed in mixed order: registry and guest heap at their baseline");
 check(native_frees==1000&&!bad_frees&&!live_native&&!released_wrong(1000),"each of the 1000 native bitmaps is released exactly once");
 /* the registry full, then one free makes room */
 reset(ctx);pool_mode=1;refused=0;
 for(i=0;i<M68KEMU_MAX_SHADOWS;i++){r=alloc_one(ctx);if(!r)++refused;if(i<1100)guest[i]=(ULONG)r;}
 r=alloc_one(ctx);
 check(!refused&&ctx->num_shadows==M68KEMU_MAX_SHADOWS&&!r&&logs==1,"with the registry full the next allocation is refused and logged once");
 check(native_frees==1&&!bad_frees&&live_native==M68KEMU_MAX_SHADOWS&&live_blocks==M68KEMU_MAX_SHADOWS&&!bad_guest_frees,"the refused allocation releases its native bitmap and its guest block");
 free_one(ctx,guest[0]);r=alloc_one(ctx);
 check(r&&shadow_lookup(ctx,(ULONG)r)==(void *)last_alloc&&ctx->num_shadows==M68KEMU_MAX_SHADOWS,"after one free the next allocation succeeds");
 /* a second free, and NULL */
 reset(ctx);pool_mode=1;g=(ULONG)alloc_one(ctx);n=last_alloc;base=ctx->num_shadows;
 free_one(ctx,g);free_one(ctx,g);
 check(g&&native_frees==1&&last_freed==n&&!bad_frees&&guest_frees==1&&!bad_guest_frees&&ctx->num_shadows==base-1,"a second free of the same guest address releases nothing again");
 free_one(ctx,0);
 check(native_frees==1&&guest_frees==1&&ctx->num_shadows==base-1,"NULL is not passed to the native call and changes nothing");
 /* arguments that are not the program's to free, then the owner's address */
 for(i=0;i<7;i++) {
  reset(ctx);pool_mode=1;g=(ULONG)alloc_one(ctx);n=last_alloc;
  if(i==0)h=m68k_bptr_out(ctx,(BPTR)(uintptr_t)&friend_bitmap);
  else if(i==1){screen.RastPort.BitMap=n;if(!shadow_register(ctx,0x9000,&screen,5))abort();h=0x9000+M68K_SCR_BITMAP;}
  else if(i==2){put16(ctx,0xA000,4);put16(ctx,0xA002,10);ctx->mem[0xA005]=2;put32(ctx,0xA008,0xB000);h=0xA000;}
  else if(i==3){if(!shadow_register(ctx,0x9000,&screen,5))abort();h=0x9000;}
  else if(i==4){if(!shadow_register(ctx,0x9800,&friend_bitmap,0))abort();h=0x9800;}
  else if(i==5)h=ctx->mem_size+64;
  else h=g+2;
  base=ctx->num_shadows;
  if(i==1&&m68k_bitmap_in(ctx,h,NULL)==n)++alias;
  free_one(ctx,h);
  if(g&&!native_frees&&!guest_frees&&ctx->num_shadows==base&&shadow_lookup(ctx,g)==(void *)n)++declined;
  free_one(ctx,g);
  if(native_frees==1&&last_freed==n&&!bad_frees&&guest_frees==1&&!bad_guest_frees&&ctx->num_shadows==base-1)++owner;
 }
 check(alias==1,"fixture: a Screen's embedded bitmap is a second address of an allocated native bitmap");
 check(declined==7,"a handle, a Screen's bitmap, a planar structure, entries of other types, an address outside the arena and one inside a guest BitMap are not passed to the native call");
 check(owner==7,"after each of them the owner's address still frees its bitmap exactly once");
 /* after a free the address resolves to nothing */
 reset(ctx);pool_mode=1;g=(ULONG)alloc_one(ctx);n=last_alloc;free_one(ctx,g);
 check(g&&!shadow_lookup(ctx,g)&&!m68k_bitmap_in(ctx,g,NULL),"a freed guest address is no registered bitmap any more");
 check(m68k_bitmap_in(ctx,g,&scratch)!=n,"with a scratch bitmap it is read as planes of the program, never as the freed native bitmap");
 /* the type cannot be verified without the layout */
 reset(ctx);pool_mode=1;g=(ULONG)alloc_one(ctx);n=last_alloc;missing_layout=TRUE;free_one(ctx,g);
 check(g&&!native_frees&&!guest_frees&&shadow_lookup(ctx,g)==(void *)n,"without the BitMap layout nothing is passed to the native call");
 missing_layout=FALSE;free_one(ctx,g);
 check(native_frees==1&&last_freed==n&&!bad_frees&&guest_frees==1&&!shadow_lookup(ctx,g),"with the layout again the bitmap is freed once");
 /* a free moves the last entry into the freed slot; a registration takes the slot the move vacated */
 reset(ctx);pool_mode=1;g=(ULONG)alloc_one(ctx);
 if(!shadow_register(ctx,0xC000,&screen,1))abort();
 slot=(UWORD)(ctx->num_shadows-1);ctx->shadow_map[slot].streams_valid=1;
 for(i=0;i<3;i++){ctx->shadow_map[slot].stream_native[i]=UINT64_C(0x100000010)*(i+1);ctx->shadow_map[slot].stream_handle[i]=(ULONG)(M68KEMU_HANDLE_BASE+4*i);}
 free_one(ctx,g);
 moved=g&&ctx->num_shadows==1&&ctx->shadow_map[0].m68k_addr==0xC000&&ctx->shadow_map[0].native_ptr==(void *)&screen&&ctx->shadow_map[0].type==1&&ctx->shadow_map[0].streams_valid==1;
 for(i=0;i<3;i++)if(ctx->shadow_map[0].stream_native[i]!=UINT64_C(0x100000010)*(i+1)||ctx->shadow_map[0].stream_handle[i]!=(ULONG)(M68KEMU_HANDLE_BASE+4*i))moved=0;
 check(moved,"the entry moved into a freed bitmap's slot keeps its address, native pointer, type and stream state");
 r=alloc_one(ctx);
 clean=r&&ctx->num_shadows==2&&ctx->shadow_map[1].m68k_addr==(ULONG)r&&!ctx->shadow_map[1].streams_valid;
 for(i=0;i<3;i++)if(ctx->shadow_map[1].stream_native[i]||ctx->shadow_map[1].stream_handle[i])clean=0;
 check(clean,"a registration into the slot that the move vacated starts with no stream state");
 /* an ExAll control, made and freed by the actual DOS object code, and a bitmap: each is freed by its own call only */
 reset(ctx);pool_mode=1;g=(ULONG)alloc_one(ctx);n=last_alloc;h=exall_control_new(ctx);base=ctx->num_shadows;
 check(g&&h&&dos_allocs==1&&live_blocks==2&&live_bytes==56,"fixture: a bitmap of 40 guest bytes and an ExAll control of 16 are alive");
 free_one(ctx,h);
 check(!native_frees&&!bad_frees&&!guest_frees&&!bad_guest_frees&&!dos_frees&&ctx->num_shadows==base&&exall_control(ctx,h)==&exall_native&&live_bytes==56,"an ExAll control is not freed as a bitmap: its entry, its native object and its 16 guest bytes stay");
 check(!exall_control(ctx,g),"a bitmap is not found as an ExAll control");
 dos_free_one(ctx,g);
 check(!dos_frees&&!guest_frees&&!bad_guest_frees&&shadow_lookup(ctx,g)==(void *)n&&ctx->num_shadows==base,"FreeDosObject for an ExAll control leaves a bitmap alone: its entry, its native bitmap and its 40 guest bytes stay");
 dos_free_one(ctx,h);
 check(dos_frees==1&&dos_freed==(void *)&exall_native&&dos_freed_type==DOS_EXALLCONTROL&&guest_frees==1&&!bad_guest_frees&&live_bytes==40&&!shadow_lookup(ctx,h),"the ExAll control's owner frees it once, with its 16 guest bytes");
 free_one(ctx,g);
 check(native_frees==1&&last_freed==n&&!bad_frees&&guest_frees==2&&!bad_guest_frees&&!live_blocks&&!live_bytes&&!shadow_lookup(ctx,g)&&ctx->num_shadows==base-2,"the bitmap's owner frees it once, with its 40 guest bytes");
 printf("GROUP lifecycle: %u failed\n",failures-before);
}
int main(void) {
 struct M68KEmuContext ctx={0};ctx.mem_size=32u*1024u*1024u;ctx.mem=calloc(1,ctx.mem_size);assert(ctx.mem);
 test_contract(&ctx);test_failures(&ctx);test_lifecycle(&ctx);free(ctx.handles);free(ctx.mem);
 printf("bitmap result: %u checks, %u failed\n",checks,failures);return failures?1:0;
}
