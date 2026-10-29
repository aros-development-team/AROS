/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "fbgfx_refresh.h"

using UBYTE = uint8_t;
using BYTE = int8_t;
using UWORD = uint16_t;
using WORD = int16_t;
using ULONG = uint32_t;
using LONG = int32_t;
using IPTR = uintptr_t;
using APTR = uint8_t *;
using BOOL = int;
using VOID = void;
using OOP_MethodID = uint32_t;
using HIDDT_ModeID = IPTR;
using OOP_Msg = void *;
constexpr BOOL TRUE = 1, FALSE = 0;

static unsigned checks, failures;
static void check(bool ok, const char *what)
{
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL %s\n", what); }
}

struct OOP_Object;
struct HWData {
    APTR framebuffer;
    ULONG fbsize, width, height, depth, bytesperpixel, bitsperpixel;
    ULONG redmask, greenmask, bluemask, redshift, greenshift, blueshift, bytesperline;
    BOOL owned;
    UBYTE palettewidth;
    UBYTE DAC[768];
    IPTR pciDeviceAttrBase;
};
struct FBGfxBitMapData {
    UBYTE *VideoData;
    LONG width, height;
    UBYTE bytesperpix;
    ULONG bytesperline;
    UBYTE *DAC;
    BYTE bpp, disp;
    OOP_Object *pixfmtobj, *displayhidd;
    LONG disp_width, disp_height, xoffset, yoffset;
};
struct FBGfx_staticdata {
    HWData data{};
    OOP_Object *visible = nullptr;
    unsigned lock_depth = 0, lock_max = 0, lock_acquires = 0, lock_releases = 0;
};
struct OOP_Class { FBGfx_staticdata *shared; };
struct OOP_Object {
    FBGfxBitMapData *bitmap = nullptr;
    OOP_Class *bitmap_class = nullptr;
    std::map<IPTR, IPTR> attrs;
    OOP_Object *sync = nullptr, *pixfmt = nullptr;
    LONG left = 0, top = 0;
    bool disposed = false;
};
struct TagItem { IPTR ti_Tag, ti_Data; };
enum : IPTR {
    TAG_DONE = 0, HiddBitMapAttrBase = 100,
    aoHidd_BitMap_Visible = 0, aoHidd_BitMap_LeftEdge = 1, aoHidd_BitMap_TopEdge = 2,
    num_Hidd_BitMap_Attrs = 3,
    aHidd_BitMap_Visible = HiddBitMapAttrBase + aoHidd_BitMap_Visible,
    aHidd_BitMap_LeftEdge = HiddBitMapAttrBase + aoHidd_BitMap_LeftEdge,
    aHidd_BitMap_TopEdge = HiddBitMapAttrBase + aoHidd_BitMap_TopEdge,
    aHidd_BitMap_Display = 110, aHidd_BitMap_PixFmt, aHidd_BitMap_ModeID,
    aHidd_ChunkyBM_Buffer, aHidd_BitMap_Width, aHidd_BitMap_Height,
    aHidd_BitMap_BytesPerRow, aHidd_Display_DMEnumerator,
    aHidd_PixFmt_BytesPerPixel, aHidd_PixFmt_BitsPerPixel,
    aHidd_PixFmt_ColorModel, aHidd_Sync_HDisp, aHidd_Sync_VDisp,
    vHidd_ColorModel_Palette = 1,
    KATTR_FrameBuffer = 0, KATTR_FrameBufferWidth, KATTR_FrameBufferHeight,
    KATTR_FrameBufferPitch, KATTR_FrameBufferDepth,
    MID_NEW = 1000, MID_DISPOSE, MID_SET, MID_COLORS, MID_UPDATE, MID_SHOW,
    IID_Root = 2000, moRoot_Dispose = MID_DISPOSE,
    MEMF_ANY = 0, MEMF_CLEAR = 1
};
struct pRoot_New { OOP_MethodID mID; TagItem *attrList; };
struct pRoot_Set { OOP_MethodID mID; TagItem *attrList; };
struct Color { UWORD red, green, blue, alpha; ULONG pixval; };
struct pHidd_BitMap_SetColors {
    OOP_MethodID mID; Color *colors; ULONG firstColor, numColors;
};
struct pHidd_BitMap_UpdateRect { OOP_MethodID mID; WORD x, y, width, height; };
struct pHidd_Display_Show { OOP_MethodID mID; OOP_Object *bitMap; ULONG flags; };

#define D(x) do {} while (0)
#define EnterFunc(x) do {} while (0)
#define ReturnPtr(label, type, value) return (value)
#define ReturnVoid(label) return
#define MNAME_ROOT(x) FBGfxBM__Root__ ## x
#define MNAME_BM(x) FBGfxBM__Hidd_BitMap__ ## x
#define OOP_INST_DATA(cl, object) ((object)->bitmap)
#define XSD(cl) ((cl)->shared)
#define IS_BM_ATTR(attr, idx) (((idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
static void lock_framebuffer(FBGfx_staticdata *data)
{
    ++data->lock_acquires;
    data->lock_max = std::max(data->lock_max, ++data->lock_depth);
}
static void unlock_framebuffer(FBGfx_staticdata *data)
{
    check(data->lock_depth != 0, "framebuffer unlock has an owner");
    if (data->lock_depth) --data->lock_depth;
    ++data->lock_releases;
}
#define LOCK_FRAMEBUFFER(data) lock_framebuffer(data)
#define UNLOCK_FRAMEBUFFER(data) unlock_framebuffer(data)

static TagItem *NextTagItem(TagItem **state)
{
    if (!*state || (*state)->ti_Tag == TAG_DONE) return nullptr;
    return (*state)++;
}
static void OOP_GetAttr(OOP_Object *object, IPTR attr, void *storage)
{
    IPTR value = 0;
    if (object) {
        if (attr == aHidd_BitMap_LeftEdge) value = static_cast<IPTR>(object->left);
        else if (attr == aHidd_BitMap_TopEdge) value = static_cast<IPTR>(object->top);
        else value = object->attrs[attr];
    }
    std::memcpy(storage, &value, sizeof(value));
}
static IPTR get_attr(OOP_Object *object, IPTR attr)
{
    IPTR value = 0;
    OOP_GetAttr(object, attr, &value);
    return value;
}
#define OOP_GET(object, attr) get_attr(object, attr)
static unsigned super_colors, super_sets, super_disposes, allocations;
static bool super_colors_ok = true, super_new_ok = true, allocation_ok = true;
static IPTR OOP_DoSuperMethod(OOP_Class *cl, OOP_Object *object, OOP_Msg message)
{
    switch (*static_cast<OOP_MethodID *>(message)) {
    case MID_NEW: return super_new_ok ? reinterpret_cast<IPTR>(object) : 0;
    case MID_DISPOSE: ++super_disposes; object->disposed = true; return 0;
    case MID_SET: {
        ++super_sets;
        check(cl->shared->lock_depth > 0, "bitmap superclass Set is serialized");
        auto *state = static_cast<pRoot_Set *>(message)->attrList;
        while (auto *tag = NextTagItem(&state)) {
            // Supply known-valid superclass results, not its clipping algorithm.
            if (tag->ti_Tag == aHidd_BitMap_LeftEdge) object->left = static_cast<LONG>(tag->ti_Data);
            if (tag->ti_Tag == aHidd_BitMap_TopEdge) object->top = static_cast<LONG>(tag->ti_Data);
        }
        return TRUE;
    }
    case MID_COLORS:
        ++super_colors;
        check(cl->shared->lock_depth > 0, "bitmap superclass SetColors is serialized");
        return super_colors_ok;
    default: std::abort();
    }
}
static void HIDD_DMEnum_GetMode(OOP_Object *enumerator, HIDDT_ModeID,
                                OOP_Object **sync, OOP_Object **pixfmt)
{
    *sync = enumerator->sync;
    *pixfmt = enumerator->pixfmt;
}
static OOP_MethodID OOP_GetMethodID(IPTR, IPTR method) { return method; }
static void OOP_CoerceMethod(OOP_Class *, OOP_Object *, OOP_Msg);
static void OOP_SetAttrs(OOP_Object *, TagItem *);
static UBYTE *AllocMem(ULONG size, ULONG flags)
{
    if (!allocation_ok) return nullptr;
    UBYTE *p = static_cast<UBYTE *>(std::malloc(size));
    if (p) { ++allocations; if (flags & MEMF_CLEAR) std::memset(p, 0, size); }
    return p;
}
static void FreeMem(void *p, ULONG) { if (p) { --allocations; std::free(p); } }

struct KernelBase {};
static KernelBase kernel;
static bool kernel_available = true;
static std::array<IPTR, 5> kernel_attrs;
static unsigned detaches, clear_calls, refresh_calls;
static bool suppress_clear = false, require_locked_pixels = false;
static FBGfx_staticdata *pixel_owner = nullptr;
static KernelBase *OpenResource(const char *) { return kernel_available ? &kernel : nullptr; }
static IPTR KrnGetSystemAttr(IPTR attr) { return kernel_attrs.at(attr); }
static void RawPutChar(unsigned char ch) { check(ch == 3, "console detach opcode"); ++detaches; }
static void ClearBuffer(HWData *);
static void fbDoRefreshArea(HWData *, FBGfxBitMapData *, LONG, LONG, LONG, LONG);

#include "production_support.inc"

static void ClearBuffer(HWData *data)
{
    ++clear_calls;
    if (require_locked_pixels) check(pixel_owner->lock_depth > 0, "clear holds framebuffer lock");
    // Record invalid-init clear attempts without dereferencing deliberately
    // impossible framebuffer addresses or enormous invalid geometry.
    if (!suppress_clear) test_clear_pixels(data);
}
static void fbDoRefreshArea(HWData *hw, FBGfxBitMapData *data,
                           LONG x1, LONG y1, LONG x2, LONG y2)
{
    ++refresh_calls;
    if (require_locked_pixels) check(pixel_owner->lock_depth > 0, "refresh holds framebuffer lock");
    test_refresh_pixels(hw, data, x1, y1, x2, y2);
}

#include "production_bitmap.inc"

static void OOP_CoerceMethod(OOP_Class *cl, OOP_Object *object, OOP_Msg message)
{
    check(*static_cast<OOP_MethodID *>(message) == MID_DISPOSE, "constructor disposes through Root");
    FBGfxBM__Root__Dispose(cl, object, message);
}
static void OOP_SetAttrs(OOP_Object *object, TagItem *tags)
{
    pRoot_Set message{MID_SET, tags};
    FBGfxBM__Root__Set(object->bitmap_class, object, &message);
}

#include "production_display.inc"

struct Guarded {
    static constexpr size_t guard = 16;
    size_t size;
    std::vector<UBYTE> bytes;
    explicit Guarded(size_t length) : size(length), bytes(length + 2 * guard, 0xC7) {}
    UBYTE *pixels() { return bytes.data() + guard; }
    void fill(UBYTE value) { std::fill_n(pixels(), size, value); }
    void guards(const char *label) const {
        check(std::all_of(bytes.begin(), bytes.begin() + guard, [](UBYTE x) { return x == 0xC7; }) &&
              std::all_of(bytes.end() - guard, bytes.end(), [](UBYTE x) { return x == 0xC7; }), label);
    }
};
static bool same_bytes(Guarded &buffer, const std::vector<UBYTE> &snapshot)
{
    return buffer.size + 2 * Guarded::guard == snapshot.size() &&
        std::memcmp(buffer.pixels() - Guarded::guard, snapshot.data(), snapshot.size()) == 0;
}
using RGBX = std::array<UBYTE, 4>;
static constexpr RGBX black{0, 0, 0, 0}, red{255, 0, 0, 0}, green{0, 255, 0, 0}, blue{0, 0, 255, 0};
struct Fixture {
    Guarded framebuffer{36 * 6}, source_a{6 * 4}, source_b{3 * 2}, palette_a{768}, palette_b{768};
    FBGfx_staticdata shared;
    OOP_Class cl{&shared};
    FBGfxBitMapData a{}, b{};
    OOP_Object oa, ob, display;
    Fixture() {
        framebuffer.fill(0xA5); source_a.fill(1); source_b.fill(2); palette_a.fill(0); palette_b.fill(0);
        palette_a.pixels()[3] = 255; palette_a.pixels()[7] = 255;
        palette_b.pixels()[7] = 255;
        a.VideoData = source_a.pixels(); a.width = 4; a.height = 4; a.bytesperline = 6;
        a.bytesperpix = 1; a.bpp = 8; a.DAC = palette_a.pixels(); a.disp_width = 8; a.disp_height = 6;
        b.VideoData = source_b.pixels(); b.width = 2; b.height = 2; b.bytesperline = 3;
        b.bytesperpix = 1; b.bpp = 8; b.DAC = palette_b.pixels(); b.disp_width = 8; b.disp_height = 6;
        oa.bitmap = &a; oa.bitmap_class = &cl; ob.bitmap = &b; ob.bitmap_class = &cl;
        shared.data.framebuffer = framebuffer.pixels(); shared.data.fbsize = framebuffer.size;
        shared.data.width = 8; shared.data.height = 6; shared.data.bytesperline = 36;
        shared.data.bytesperpixel = 4;
        pixel_owner = &shared; require_locked_pixels = true;
        clear_calls = refresh_calls = super_colors = super_sets = 0;
        super_colors_ok = true;
    }
    ~Fixture() { require_locked_pixels = false; pixel_owner = nullptr; }
    void colors(OOP_Object &object, Color *colors, ULONG first, ULONG count, bool expected = true) {
        pHidd_BitMap_SetColors message{MID_COLORS, colors, first, count};
        check(bool(FBGfxBM__Hidd_BitMap__SetColors(&cl, &object, &message)) == expected,
              "SetColors result");
        balanced();
    }
    void show(OOP_Object *bitmap) {
        pHidd_Display_Show message{MID_SHOW, bitmap, 0};
        check(FBGfxDisplay__Hidd_Display__Show(&cl, &display, &message) == bitmap, "Show returns requested bitmap");
        check(shared.visible == bitmap, "Show records requested bitmap");
        balanced();
    }
    void move(LONG x, LONG y) {
        TagItem tags[] = {{aHidd_BitMap_LeftEdge, static_cast<IPTR>(x)},
                          {aHidd_BitMap_TopEdge, static_cast<IPTR>(y)}, {TAG_DONE, 0}};
        OOP_SetAttrs(&oa, tags); balanced();
    }
    void update(OOP_Object &object, WORD x, WORD y, WORD width, WORD height) {
        pHidd_BitMap_UpdateRect message{MID_UPDATE, x, y, width, height};
        FBGfxBM__Hidd_BitMap__UpdateRect(&cl, &object, &message); balanced();
    }
    void balanced() { check(shared.lock_depth == 0 && shared.lock_acquires == shared.lock_releases,
                            "framebuffer acquisitions and releases balance"); }
    RGBX pixel(unsigned x, unsigned y) const {
        RGBX value; std::copy_n(framebuffer.bytes.data() + Guarded::guard + 36 * y + 4 * x, 4, value.begin()); return value;
    }
    void rectangle(unsigned x0, unsigned y0, unsigned w, unsigned h, RGBX color, const char *label) const {
        bool ok = true;
        // A fixed small expected rectangle, independent of production clipping.
        for (unsigned y = 0; y < 6; ++y) for (unsigned x = 0; x < 8; ++x)
            ok &= pixel(x, y) == ((x >= x0 && x < x0 + w && y >= y0 && y < y0 + h) ? color : black);
        check(ok, label);
    }
    void guards() {
        framebuffer.guards("framebuffer canaries"); source_a.guards("source A canaries");
        source_b.guards("source B canaries"); palette_a.guards("palette A canaries"); palette_b.guards("palette B canaries");
    }
};

static void test_buffer_sizes()
{
    uint32_t result = 0;
    check(fbgfx_buffer_size(3, 2, 5, 1, &result) && result == 10, "LUT8 full padded allocation size");
    check(fbgfx_buffer_size(3, 2, 16, 4, &result) && result == 32, "RGBX full padded allocation size");
    check(fbgfx_buffer_size(1, 1, UINT32_MAX, 1, &result) && result == UINT32_MAX, "ULONG allocation upper boundary");
    const uint32_t bad[][4] = {{0,1,4,4},{1,0,4,4},{1,1,0,1},{1,1,4,2},
                              {3,1,11,4},{UINT32_MAX,1,UINT32_MAX,4},
                              {1,2,UINT32_MAX,1},{1,0x40000000,8,4}};
    for (const auto &v : bad) {
        result = 0x76543210;
        check(!fbgfx_buffer_size(v[0], v[1], v[2], v[3], &result), "invalid backing geometry refused");
        check(result == 0x76543210, "failed size check preserves output");
    }
    check(!fbgfx_buffer_size(1, 1, 4, 4, nullptr), "null allocation-size output refused");
}

static void test_palette_requests()
{
    Fixture f;
    Color color{0x1234, 0xABCD, 0xEF01, 0, 0};
    auto source_before = f.source_a.bytes;
    auto fb_before = f.framebuffer.bytes;
    f.colors(f.oa, &color, 255, 1);
    check(f.a.DAC[765] == 0x12 && f.a.DAC[766] == 0xAB && f.a.DAC[767] == 0xEF,
          "palette preserves full high eight bits of each component");
    check(same_bytes(f.framebuffer, fb_before) && refresh_calls == 0 && clear_calls == 0,
          "hidden palette update never touches framebuffer");
    const ULONG bad[][2] = {{255,2},{256,1},{UINT32_MAX,2},{0,UINT32_MAX}};
    auto palette_before = f.palette_a.bytes;
    unsigned supers = super_colors;
    for (const auto &range : bad) f.colors(f.oa, &color, range[0], range[1], false);
    f.colors(f.oa, nullptr, 0, 1, false);
    f.colors(f.oa, nullptr, 256, 0);
    check(super_colors == supers && same_bytes(f.palette_a, palette_before) && same_bytes(f.framebuffer, fb_before),
          "bad/null/empty palette requests preserve data and bypass superclass");
    f.show(&f.oa);
    fb_before = f.framebuffer.bytes; palette_before = f.palette_a.bytes;
    unsigned refreshes = refresh_calls, clears = clear_calls;
    super_colors_ok = false;
    f.colors(f.oa, &color, 1, 1, false);
    check(super_colors == supers + 1 && same_bytes(f.palette_a, palette_before) && same_bytes(f.framebuffer, fb_before) &&
          refresh_calls == refreshes && clear_calls == clears, "superclass palette failure is atomic");
    check(same_bytes(f.source_a, source_before), "palette requests never rewrite source indices");
    f.guards();
}

static void test_visibility_scroll_and_partial_updates()
{
    Fixture f;
    auto indices_before = f.source_a.bytes;
    auto before = f.framebuffer.bytes;
    f.update(f.oa, 0, 0, 4, 4);
    check(same_bytes(f.framebuffer, before) && !clear_calls && !refresh_calls, "hidden UpdateRect does not refresh");
    f.show(&f.oa);
    f.rectangle(0, 0, 4, 4, red, "Show publishes prepainted pixels and blacks uncovered bands");
    check(clear_calls == 1 && refresh_calls == 1, "visible transition clears and refreshes once");
    check(f.a.disp && f.shared.lock_max >= 2, "Show supports recursive framebuffer acquisition");
    f.move(1, 1);
    f.rectangle(1, 1, 4, 4, red, "positive offsets move content and clear old bands");
    check(clear_calls == 2 && refresh_calls == 2 && f.a.xoffset == 1 && f.a.yoffset == 1,
          "move stores validated offsets and performs one complete refresh");
    f.move(1, 1);
    check(clear_calls == 2 && refresh_calls == 2, "unchanged offsets do not repaint");
    f.source_a.pixels()[0] = 2;
    f.framebuffer.pixels()[7 * 4] = 0x61; // Deliberate marker outside current bitmap coverage.
    unsigned clears = clear_calls;
    f.update(f.oa, 0, 0, 1, 1);
    check(f.pixel(1, 1) == green && f.pixel(2, 1) == red && f.pixel(7, 0)[0] == 0x61,
          "partial update changes only requested covered pixels");
    check(clear_calls == clears, "partial update preserves uncovered bands");
    Color color_blue{0, 0, 0xFFFF, 0, 0};
    f.colors(f.oa, &color_blue, 1, 1);
    check(f.pixel(2, 1) == blue && f.pixel(1, 1) == green && f.pixel(7, 0)[0] == 0x61,
          "visible palette recolors unchanged indices without changing coverage");
    check(clear_calls == clears, "palette update does not clear framebuffer");
    f.show(&f.ob);
    f.rectangle(0, 0, 2, 2, green, "second bitmap replaces prior coverage");
    before = f.framebuffer.bytes;
    Color color_red{0xFFFF, 0, 0, 0, 0};
    f.colors(f.oa, &color_red, 1, 1);
    f.update(f.oa, 0, 0, 4, 4);
    check(!f.a.disp && f.b.disp && same_bytes(f.framebuffer, before),
          "hidden palette and updates cannot repaint another visible bitmap");
    f.show(nullptr);
    f.rectangle(0, 0, 0, 0, black, "Show NULL blanks all physical pixels");
    check(!f.b.disp, "Show NULL hides old bitmap");
    f.show(&f.oa);
    check(f.pixel(1, 1) == green && f.pixel(2, 1) == red && f.pixel(0, 0) == black,
          "reshow uses hidden palette and retained offsets with existing indices");
    f.source_a.pixels()[0] = 1;
    check(same_bytes(f.source_a, indices_before), "presentation leaves indices, row padding and guards intact");
    check(std::all_of(f.framebuffer.pixels() + 32, f.framebuffer.pixels() + 36,
                      [](UBYTE b) { return b == 0; }), "visible transition clears framebuffer row padding");
    f.guards();
}

static void test_hw_init()
{
    Guarded fb(32);
    auto valid_attrs = [&] {
        kernel_available = true;
        kernel_attrs = {reinterpret_cast<IPTR>(fb.pixels()), 2, 2, 8, 32};
        clear_calls = detaches = 0;
    };
    valid_attrs(); fb.fill(0xA5);
    HWData hw{};
    check(initFBGfxHW(&hw), "valid RGBX kernel framebuffer initializes");
    check(hw.framebuffer == fb.pixels() && hw.fbsize == 16 && hw.width == 2 && hw.height == 2 &&
          hw.bytesperline == 8 && hw.bytesperpixel == 4 && hw.depth == 32,
          "hardware snapshot uses checked geometry");
    check(clear_calls == 1 && detaches == 1 && std::all_of(fb.pixels(), fb.pixels() + 16,
          [](UBYTE b) { return b == 0; }) && fb.pixels()[16] == 0xA5, "hardware init clears exactly validated size");
    auto refuse = [&](const char *label) {
        fb.fill(0xA5); auto before = fb.bytes;
        HWData initial{}; std::memset(&initial, 0x5A, sizeof(initial)); HWData output = initial;
        suppress_clear = true;
        bool ok = initFBGfxHW(&output);
        suppress_clear = false;
        check(!ok && clear_calls == 0 && detaches == 0, label);
        check(std::memcmp(&output, &initial, sizeof(initial)) == 0 && same_bytes(fb, before),
              "hardware refusal preserves snapshot and framebuffer");
    };
    valid_attrs(); kernel_available = false; refuse("absent kernel resource refused before clear");
    for (IPTR pointer : {IPTR(0), UINTPTR_MAX, UINTPTR_MAX - 8}) {
        valid_attrs(); kernel_attrs[KATTR_FrameBuffer] = pointer; refuse("unavailable or wrapping framebuffer pointer refused");
    }
    for (unsigned attr : {KATTR_FrameBufferWidth, KATTR_FrameBufferHeight, KATTR_FrameBufferPitch})
        for (IPTR value : {IPTR(0), UINTPTR_MAX}) {
            valid_attrs(); kernel_attrs[attr] = value; refuse("unknown or zero framebuffer geometry refused");
        }
    valid_attrs(); kernel_attrs[KATTR_FrameBufferPitch] = 7; refuse("undersized RGBX pitch refused");
    valid_attrs(); kernel_attrs[KATTR_FrameBufferHeight] = 0x40000000;
    refuse("framebuffer allocation product exceeding ULONG refused");
    for (IPTR depth : {IPTR(16), UINTPTR_MAX}) {
        valid_attrs(); kernel_attrs[KATTR_FrameBufferDepth] = depth; refuse("unsupported or unknown framebuffer depth refused");
    }
    fb.guards("hardware-init framebuffer canaries");
}

static void test_refresh_geometry_and_truecolor()
{
    Fixture f;
    f.show(&f.oa);
    f.move(-1, -1);
    f.rectangle(0, 0, 3, 3, red, "negative offsets clip source while clearing uncovered bands");
    f.move(7, 5);
    f.rectangle(7, 5, 1, 1, red, "bottom-right visible sliver clips to physical framebuffer");
    f.move(-3, -3);
    f.rectangle(0, 0, 1, 1, red, "top-left visible sliver clips to bitmap edge");

    Guarded rgb(20 * 3);
    rgb.fill(0xCD);
    for (unsigned y = 0; y < 3; ++y) for (unsigned x = 0; x < 4; ++x) {
        UBYTE *pixel = rgb.pixels() + 20 * y + 4 * x;
        pixel[0] = x + 1; pixel[1] = y + 2; pixel[2] = 0x55; pixel[3] = 0xA0 + x;
    }
    auto source_before = rgb.bytes;
    f.framebuffer.fill(0);
    FBGfxBitMapData native{};
    native.VideoData = rgb.pixels(); native.width = 4; native.height = 3; native.bytesperpix = 4;
    native.bytesperline = 20; native.bpp = 24; native.disp = 1;
    native.disp_width = 3; native.disp_height = 2; native.xoffset = 1;
    OOP_Object object; object.bitmap = &native; object.bitmap_class = &f.cl;
    f.update(object, 0, 0, 4, 3);
    bool pixels_ok = true;
    for (unsigned y = 0; y < 6; ++y) for (unsigned x = 0; x < 8; ++x) {
        RGBX expected = black;
        if (x >= 1 && x < 3 && y < 2)
            expected = RGBX{UBYTE(x), UBYTE(y + 2), 0x55, UBYTE(0x9F + x)};
        pixels_ok &= f.pixel(x, y) == expected;
    }
    check(pixels_ok, "native RGBX retains all four bytes and clips to display-mode size");
    check(same_bytes(rgb, source_before), "native refresh preserves source, row padding and canaries");
    auto before = f.framebuffer.bytes;
    auto valid = native;
    const unsigned clears = clear_calls, refreshes = refresh_calls;
    Color ignored{0xFFFF, 0, 0, 0, 0};
    f.colors(object, &ignored, 1, 1);
    check(same_bytes(f.framebuffer, before) && clear_calls == clears && refresh_calls == refreshes,
          "native bitmap palette call delegates without software palette refresh");
    for (unsigned test = 0; test < 11; ++test) {
        native = valid;
        f.shared.data.bytesperpixel = 4; f.shared.data.fbsize = f.framebuffer.size;
        switch (test) {
        case 0: native.width = 0; break;
        case 1: native.height = -1; break;
        case 2: native.height = 0x40000000; break;
        case 3: native.bytesperpix = 2; break;
        case 4: native.bytesperline = 15; break;
        case 5: native.VideoData = nullptr; break;
        case 6: native.disp_width = 0; break;
        case 7: native.disp_height = -1; break;
        case 8: f.shared.data.bytesperpixel = 2; break;
        case 9: f.shared.data.fbsize = 1; break;
        case 10: native.VideoData = f.framebuffer.pixels(); break;
        }
        f.update(object, -3, -2, 100, 100);
        check(same_bytes(f.framebuffer, before) && same_bytes(rgb, source_before),
              "malformed or overlapping refresh is refused without any pixel change");
    }
    rgb.guards("native refresh source canaries");
    f.guards();
}

static void test_constructor()
{
    Guarded source(64);
    FBGfx_staticdata shared;
    OOP_Class cl{&shared};
    OOP_Object pf, sync, enumerator, display;
    pf.attrs = {{aHidd_PixFmt_BytesPerPixel, 1}, {aHidd_PixFmt_BitsPerPixel, 8},
                {aHidd_PixFmt_ColorModel, vHidd_ColorModel_Palette}};
    sync.attrs = {{aHidd_Sync_HDisp, 8}, {aHidd_Sync_VDisp, 6}};
    enumerator.sync = &sync; enumerator.pixfmt = &pf;
    display.attrs[aHidd_Display_DMEnumerator] = reinterpret_cast<IPTR>(&enumerator);
    auto make = [&](FBGfxBitMapData &data) {
        OOP_Object object; object.bitmap = &data; object.bitmap_class = &cl;
        object.attrs = {{aHidd_BitMap_Display, reinterpret_cast<IPTR>(&display)},
            {aHidd_BitMap_PixFmt, reinterpret_cast<IPTR>(&pf)}, {aHidd_BitMap_ModeID, 1},
            {aHidd_ChunkyBM_Buffer, reinterpret_cast<IPTR>(source.pixels())},
            {aHidd_BitMap_Width, 4}, {aHidd_BitMap_Height, 4}, {aHidd_BitMap_BytesPerRow, 6}};
        return object;
    };
    pRoot_New message{MID_NEW, nullptr};
    FBGfxBitMapData data{}; auto object = make(data);
    check(FBGfxBM__Root__New(&cl, &object, &message) == &object && data.DAC && allocations == 1,
          "valid indexed bitmap constructor publishes object and palette");
    check(std::all_of(data.DAC, data.DAC + 768, [](UBYTE b) { return b == 0; }),
          "new palette starts black");
    OOP_MethodID dispose = MID_DISPOSE;
    FBGfxBM__Root__Dispose(&cl, &object, &dispose);
    check(allocations == 0 && object.disposed, "palette is released by constructor's normal disposal path");
    const unsigned cases = 10;
    for (unsigned test = 0; test < cases; ++test) {
        FBGfxBitMapData rejected{}; auto candidate = make(rejected);
        enumerator.sync = &sync; enumerator.pixfmt = &pf;
        display.attrs[aHidd_Display_DMEnumerator] = reinterpret_cast<IPTR>(&enumerator);
        pf.attrs[aHidd_PixFmt_BytesPerPixel] = 1;
        allocation_ok = true;
        switch (test) {
        case 0: display.attrs[aHidd_Display_DMEnumerator] = 0; break;
        case 1: enumerator.sync = nullptr; break;
        case 2: enumerator.pixfmt = nullptr; break;
        case 3: candidate.attrs[aHidd_ChunkyBM_Buffer] = 0; break;
        case 4: candidate.attrs[aHidd_BitMap_Width] = 0; break;
        case 5: candidate.attrs[aHidd_BitMap_Height] = 0; break;
        case 6: candidate.attrs[aHidd_BitMap_BytesPerRow] = 3; break;
        case 7: candidate.attrs[aHidd_BitMap_Height] = 0x40000000; break;
        case 8: pf.attrs[aHidd_PixFmt_BytesPerPixel] = 2; break;
        case 9: allocation_ok = false; break;
        }
        unsigned disposes = super_disposes;
        OOP_Object *result = FBGfxBM__Root__New(&cl, &candidate, &message);
        check(result == nullptr && candidate.disposed &&
              super_disposes == disposes + 1 && !allocations, "invalid constructor state is disposed without leaked palette");
        if (result)
            FBGfxBM__Root__Dispose(&cl, result, &dispose);
    }
    allocation_ok = true;
    FBGfxBitMapData native{}; auto native_object = make(native);
    enumerator.sync = &sync; enumerator.pixfmt = &pf;
    pf.attrs[aHidd_PixFmt_BytesPerPixel] = 4;
    pf.attrs[aHidd_PixFmt_BitsPerPixel] = 24;
    pf.attrs[aHidd_PixFmt_ColorModel] = 2;
    native_object.attrs[aHidd_BitMap_BytesPerRow] = 16;
    check(FBGfxBM__Root__New(&cl, &native_object, &message) == &native_object &&
          native.bytesperpix == 4 && native.DAC == nullptr && !allocations,
          "native bitmap constructor retains truecolor path without palette allocation");
    FBGfxBM__Root__Dispose(&cl, &native_object, &dispose);
    FBGfxBitMapData failed{}; auto failed_object = make(failed);
    unsigned disposes = super_disposes;
    super_new_ok = false;
    check(FBGfxBM__Root__New(&cl, &failed_object, &message) == nullptr &&
          !failed_object.disposed && super_disposes == disposes && !allocations,
          "superclass constructor failure does not dispose a nonexistent object");
    super_new_ok = true;
    source.guards("constructor leaves caller-provided buffer untouched");
}

int main()
{
    test_buffer_sizes();
    test_palette_requests();
    test_visibility_scroll_and_partial_updates();
    test_refresh_geometry_and_truecolor();
    test_hw_init();
    test_constructor();
    std::printf("Framebuffer production methods: %u checks, %u failed\n", checks, failures);
    return failures ? 1 : 0;
}
