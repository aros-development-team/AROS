#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""
theme-convert.py - convert an AROS theme to the MUI style image layout.

Themes originally stored their images as

    <theme>/Images/Gadgets/<Name>            button images
    <theme>/System/<Name>/Default            decoration images, with
    <theme>/Menu/<Name>/Default              every state side by side

The current layout follows the conventions of MUI image sets:

    <theme>/Images/<size>pt/<Name>.mbr       a single state image
    <theme>/Images/<size>pt/<Name>.mf0       state 0 (normal)
    <theme>/Images/<size>pt/<Name>.mf1       state 1 (selected)
    <theme>/Images/<size>pt/<Name>.mf2 ...   further states

This script reads a theme in the original layout and writes the images
in the current one, splitting the multi state images into one file per
state and creating the smaller size set by scaling. The theme's Config
files are not changed, and no files are removed unless asked for.

    theme-convert.py [options] <theme directory> ...

Only the Python standard library is needed. Images have to be PNG.
"""

import argparse
import os
import struct
import sys
import zlib

PNG_SIG = b"\x89PNG\r\n\x1a\n"

# --------------------------------------------------------------------------
# Minimal PNG support: any non interlaced PNG in, 8 bit RGBA out.
# --------------------------------------------------------------------------


class Image:
    """A width x height image of 8 bit RGBA pixels, stored row by row."""

    def __init__(self, width, height, data=None):
        self.width = width
        self.height = height
        self.data = data if data is not None else bytearray(width * height * 4)

    def crop(self, x, y, w, h):
        out = Image(w, h)
        for row in range(h):
            src = ((y + row) * self.width + x) * 4
            out.data[row * w * 4:(row + 1) * w * 4] = self.data[src:src + w * 4]
        return out

    def same(self, other):
        return (self.width == other.width and self.height == other.height
                and self.data == other.data)

    def scaled(self, w, h):
        """Area averaged resize, weighting colour by alpha."""
        out = Image(w, h)
        sw, sh = self.width, self.height
        for dy in range(h):
            y0, y1 = dy * sh / h, (dy + 1) * sh / h
            for dx in range(w):
                x0, x1 = dx * sw / w, (dx + 1) * sw / w
                acc = [0.0, 0.0, 0.0, 0.0]
                area = 0.0
                sy = int(y0)
                while sy < y1 and sy < sh:
                    wy = min(y1, sy + 1) - max(y0, sy)
                    sx = int(x0)
                    while sx < x1 and sx < sw:
                        wx = min(x1, sx + 1) - max(x0, sx)
                        wgt = wx * wy
                        if wgt > 0:
                            p = (sy * sw + sx) * 4
                            a = self.data[p + 3]
                            acc[0] += self.data[p] * a * wgt
                            acc[1] += self.data[p + 1] * a * wgt
                            acc[2] += self.data[p + 2] * a * wgt
                            acc[3] += a * wgt
                            area += wgt
                        sx += 1
                    sy += 1
                o = (dy * w + dx) * 4
                if acc[3] > 0:
                    out.data[o] = min(255, int(acc[0] / acc[3] + 0.5))
                    out.data[o + 1] = min(255, int(acc[1] / acc[3] + 0.5))
                    out.data[o + 2] = min(255, int(acc[2] / acc[3] + 0.5))
                    out.data[o + 3] = min(255, int(acc[3] / area + 0.5))
        return out


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def read_png(path):
    with open(path, "rb") as fh:
        raw = fh.read()
    if raw[:8] != PNG_SIG:
        raise ValueError("%s: not a PNG image" % path)

    pos = 8
    idat = []
    palette = None
    trns = None
    header = None
    while pos + 8 <= len(raw):
        length, ctype = struct.unpack(">I4s", raw[pos:pos + 8])
        body = raw[pos + 8:pos + 8 + length]
        pos += 12 + length
        if ctype == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif ctype == b"PLTE":
            palette = body
        elif ctype == b"tRNS":
            trns = body
        elif ctype == b"IDAT":
            idat.append(body)
        elif ctype == b"IEND":
            break
    if header is None:
        raise ValueError("%s: no image header" % path)

    width, height, depth, ctype, _, _, interlace = header
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(ctype)
    if channels is None or depth not in (1, 2, 4, 8, 16) or interlace > 1:
        raise ValueError("%s: unsupported pixel format" % path)

    bpp = max(1, channels * depth // 8)
    data = zlib.decompress(b"".join(idat))
    img = Image(width, height)
    out = img.data
    pos = 0

    def unfilter(w, h):
        """Undo the row filters of a w x h (sub)image starting at 'pos'."""
        nonlocal pos
        stride = (w * channels * depth + 7) // 8
        rows = []
        prev = bytearray(stride)
        for _ in range(h):
            ftype = data[pos]
            line = bytearray(data[pos + 1:pos + 1 + stride])
            pos += 1 + stride
            if ftype == 1:
                for i in range(bpp, stride):
                    line[i] = (line[i] + line[i - bpp]) & 255
            elif ftype == 2:
                for i in range(stride):
                    line[i] = (line[i] + prev[i]) & 255
            elif ftype == 3:
                for i in range(stride):
                    left = line[i - bpp] if i >= bpp else 0
                    line[i] = (line[i] + ((left + prev[i]) >> 1)) & 255
            elif ftype == 4:
                for i in range(stride):
                    left = line[i - bpp] if i >= bpp else 0
                    upleft = prev[i - bpp] if i >= bpp else 0
                    line[i] = (line[i] + _paeth(left, prev[i], upleft)) & 255
            elif ftype != 0:
                raise ValueError("%s: corrupt image data" % path)
            rows.append(line)
            prev = line
        return rows

    def pixels(line, w):
        """Convert one unfiltered row of w pixels to RGBA tuples."""
        if depth == 16:
            # keep the most significant byte of every sample
            samples = line[0::2]
        elif depth < 8:
            samples = bytearray()
            mask = (1 << depth) - 1
            for x in range(w * channels):
                byte = line[(x * depth) >> 3]
                shift = 8 - depth - ((x * depth) & 7)
                value = (byte >> shift) & mask
                if ctype != 3:
                    value = value * 255 // mask
                samples.append(value)
        else:
            samples = line
        res = []
        for x in range(w):
            if ctype == 6:
                res.append(tuple(samples[x * 4:x * 4 + 4]))
            elif ctype == 2:
                rgb = tuple(samples[x * 3:x * 3 + 3])
                alpha = 255
                if trns and len(trns) >= 6 and depth == 8:
                    if rgb == (trns[1], trns[3], trns[5]):
                        alpha = 0
                res.append(rgb + (alpha,))
            elif ctype == 0:
                g = samples[x]
                alpha = 255
                if trns and len(trns) >= 2 and depth == 8 and g == trns[1]:
                    alpha = 0
                res.append((g, g, g, alpha))
            elif ctype == 4:
                g = samples[x * 2]
                res.append((g, g, g, samples[x * 2 + 1]))
            else:
                idx = samples[x]
                if palette is None or idx * 3 + 2 >= len(palette):
                    raise ValueError("%s: bad palette index" % path)
                alpha = trns[idx] if trns and idx < len(trns) else 255
                res.append(tuple(palette[idx * 3:idx * 3 + 3]) + (alpha,))
        return res

    if interlace:
        # Adam7: seven reduced images, each covering part of the grid
        passes = ((0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4),
                  (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2))
    else:
        passes = ((0, 0, 1, 1),)

    for x0, y0, dx, dy in passes:
        pw = (width - x0 + dx - 1) // dx
        ph = (height - y0 + dy - 1) // dy
        if pw <= 0 or ph <= 0:
            continue
        for row, line in enumerate(unfilter(pw, ph)):
            y = y0 + row * dy
            for col, pix in enumerate(pixels(line, pw)):
                o = (y * width + x0 + col * dx) * 4
                out[o:o + 4] = bytes(pix)
    return img


def write_png(path, img):
    def chunk(ctype, body):
        return (struct.pack(">I", len(body)) + ctype + body
                + struct.pack(">I", zlib.crc32(ctype + body) & 0xffffffff))

    stride = img.width * 4
    raw = bytearray()
    for y in range(img.height):
        raw.append(0)
        raw += img.data[y * stride:(y + 1) * stride]
    out = (PNG_SIG
           + chunk(b"IHDR", struct.pack(">IIBBBBB", img.width, img.height, 8, 6, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(out)


# --------------------------------------------------------------------------
# Theme layout
# --------------------------------------------------------------------------

# Decoration images: original file, new name, states across, states down.
# 'G' stands for the number of window gadget states, which is 4 unless
# the theme sets "NoInactiveSelected = Yes" in System/Config.
DECORATION = [
    ("System/SDepth/Default", "ScrDepth", 2, 1),
    ("System/STitlebar/Default", "ScrBar", 1, 1),
    ("System/SBarLogo/Default", "ScrBarLogo", 1, 1),
    ("System/Size/Default", "WinSize", "G", 1),
    ("System/Close/Default", "WinClose", "G", 1),
    ("System/Depth/Default", "WinDepth", "G", 1),
    ("System/Zoom/Default", "WinZoom", "G", 1),
    ("System/MUI/Default", "WinMUI", "G", 1),
    ("System/PopUp/Default", "WinPopUp", "G", 1),
    ("System/Snapshot/Default", "WinSnapshot", "G", 1),
    ("System/Iconify/Default", "WinIconify", "G", 1),
    ("System/Lock/Default", "WinLock", "G", 1),
    ("System/ArrowUp/Default", "ArrowUp", "G", 1),
    ("System/ArrowDown/Default", "ArrowDown", "G", 1),
    ("System/ArrowLeft/Default", "ArrowLeft", "G", 1),
    ("System/ArrowRight/Default", "ArrowRight", "G", 1),
    ("System/Titlebar/Default", "WinBar", 1, 2),
    ("System/Container/Vertical", "PropBackVert", 2, 1),
    ("System/Knob/Vertical", "PropKnobVert", 3, 1),
    ("System/Container/Horizontal", "PropBackHoriz", 1, 2),
    ("System/Knob/Horizontal", "PropKnobHoriz", 1, 3),
    ("Menu/Background/Default", "MenuBack", 1, 1),
    ("Menu/AmigaKey/Default", "MenuAmigaKey", 1, 1),
    ("Menu/Checkmark/Default", "MenuCheck", 1, 1),
    ("Menu/SubMenu/Default", "MenuSubMenu", 1, 1),
]

# Images that are a single state of a multi state image: original file,
# new name, state number.
DECORATION_STATES = [
    ("System/Borders/Default", "WinBorder", 0),
    ("System/Borders/Default_Deactivated", "WinBorder", 1),
]

# Images for MUI's own gadgets. Some themes carry these with the
# decoration images, holding normal, selected, disabled and disabled
# selected states.
MUI_IMAGES = [
    (("System/Checkmark/Default.png", "System/Checkmark/Default"), "CheckMark", 4, 1),
    (("System/Radio/Default.png", "System/Radio/Default"), "RadioButton", 4, 1),
    (("System/Cycle/Default.png", "System/Cycle/Default"), "Cycle", 4, 1),
]

# Button images that are stored under a different name.
GADGET_NAMES = {
    "ok": "OK",
    # these are the arrows of MUI; a theme that has arrow images for its
    # window borders uses those, and the button image is not converted
    "previous": "ArrowLeft",
    "next": "ArrowRight",
    "below": "ArrowDown",
}

# Version of the theme layout written by this script; the decoration
# does not load themes that state another version, or none.
THEME_VERSION = 2

GADGET_DIR = "Images/Gadgets"
SMALL_GADGET_DIR = "small"
SMALL_SUFFIX = "Small"
IMAGE_DIR = "Images"


def is_png(path):
    with open(path, "rb") as fh:
        return fh.read(8) == PNG_SIG


def find_path(base, rel):
    """Locate 'rel' beneath 'base' ignoring case; None if it is not there."""
    cur = base
    for part in rel.split("/"):
        try:
            entries = os.listdir(cur)
        except OSError:
            return None
        match = [e for e in entries if e.lower() == part.lower()]
        if not match:
            return None
        cur = os.path.join(cur, match[0])
    return cur


def gadget_states(theme):
    cfg = find_path(theme, "System/Config")
    if cfg:
        with open(cfg, "r", errors="replace") as fh:
            for line in fh:
                if line.startswith("NoInactiveSelected") and "Yes" in line:
                    return 3
    return 4


class Converter:
    def __init__(self, theme, opts):
        self.theme = theme
        self.opts = opts
        self.removed = []
        self.written = []
        self.imagedir = find_path(theme, IMAGE_DIR) or os.path.join(theme, IMAGE_DIR)

    def log(self, text):
        if not self.opts.quiet:
            print(text)

    def out_path(self, size, name):
        return os.path.join(self.imagedir, "%dpt" % size, name)

    def write(self, size, name, img):
        path = self.out_path(size, name)
        self.written.append(path)
        if self.opts.dry_run:
            return
        write_png(path, img)

    def copy(self, size, name, src):
        """Store an unmodified image; the original bytes are kept."""
        path = self.out_path(size, name)
        self.written.append(path)
        if self.opts.dry_run:
            return
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(src, "rb") as fh:
            body = fh.read()
        with open(path, "wb") as fh:
            fh.write(body)

    def convert_decoration(self):
        size = self.opts.size
        gstates = gadget_states(self.theme)

        for rels, name, cols, rows in DECORATION + MUI_IMAGES:
            if isinstance(rels, str):
                rels = (rels,)
            src = rel = None
            for rel in rels:
                src = find_path(self.theme, rel)
                if src and os.path.isfile(src) and is_png(src):
                    break
                src = None
            if not src:
                continue
            cols = gstates if cols == "G" else cols
            states = cols * rows
            if states == 1:
                self.copy(size, name + ".mbr", src)
                self.log("  %-32s -> %s.mbr" % (rel, name))
                self.removed.append(src)
                continue

            img = read_png(src)
            if img.width < cols or img.height < rows:
                print("%s: %dx%d image cannot hold %dx%d states - skipped"
                      % (src, img.width, img.height, cols, rows), file=sys.stderr)
                continue
            # as when the states are read from one image, pixels beyond a
            # whole number of states are ignored
            w, h = img.width // cols, img.height // rows
            parts = [img.crop((k % cols) * w, (k // cols) * h, w, h)
                     for k in range(states)]
            kept = []
            for k, part in enumerate(parts):
                # A state that matches the one used when it is missing
                # does not need to be stored.
                fallback = 1 if (k == 3 and 1 in kept) else 0
                if k > 0 and not self.opts.all_states and part.same(parts[fallback]):
                    continue
                kept.append(k)
                self.write(size, "%s.mf%d" % (name, k), part)
            self.log("  %-32s -> %s.mf{%s}" % (rel, name, ",".join(map(str, kept))))
            self.removed.append(src)

        for rel, name, state in DECORATION_STATES:
            src = find_path(self.theme, rel)
            if not src or not os.path.isfile(src):
                continue
            self.copy(size, "%s.mf%d" % (name, state), src)
            self.log("  %-32s -> %s.mf%d" % (rel, name, state))
            self.removed.append(src)

    def convert_gadgets(self):
        size = self.opts.size
        gdir = find_path(self.theme, GADGET_DIR)
        if not gdir:
            return
        for entry in sorted(os.listdir(gdir)):
            src = os.path.join(gdir, entry)
            if not os.path.isfile(src) or not is_png(src):
                continue
            name = GADGET_NAMES.get(entry.lower(), entry)
            if os.path.exists(self.out_path(size, name + ".mf0")) or \
                    self.out_path(size, name + ".mf0") in self.written:
                self.log("  %-32s    (theme has %s.mf0)"
                         % (GADGET_DIR + "/" + entry, name))
                self.removed.append(src)
                continue
            self.copy(size, name + ".mbr", src)
            note = ""
            for small in self.opts.scaled:
                img = read_png(src)
                w = max(1, (img.width * small + size // 2) // size)
                h = max(1, (img.height * small + size // 2) // size)
                self.write(small, name + ".mbr", img.scaled(w, h))
                note += ", %dpt %dx%d" % (small, w, h)
            self.log("  %-32s -> %s.mbr%s" % (GADGET_DIR + "/" + entry, name, note))
            self.removed.append(src)

        # Images drawn for small buttons are images of their own, named
        # after the button image they are the small version of
        sdir = find_path(gdir, SMALL_GADGET_DIR)
        if not sdir:
            return
        for entry in sorted(os.listdir(sdir)):
            src = os.path.join(sdir, entry)
            if not os.path.isfile(src) or not is_png(src):
                continue
            name = GADGET_NAMES.get(entry.lower(), entry) + SMALL_SUFFIX
            self.copy(size, name + ".mbr", src)
            note = ""
            for small in self.opts.scaled:
                img = read_png(src)
                w = max(1, (img.width * small + size // 2) // size)
                h = max(1, (img.height * small + size // 2) // size)
                self.write(small, name + ".mbr", img.scaled(w, h))
                note += ", %dpt %dx%d" % (small, w, h)
            self.log("  %-32s -> %s.mbr%s" % (
                "%s/%s/%s" % (GADGET_DIR, SMALL_GADGET_DIR, entry), name, note))
            self.removed.append(src)

    def set_version(self):
        """State the theme layout version in System/Config."""
        cfg = find_path(self.theme, "System/Config")
        if not cfg:
            return
        with open(cfg, "rb") as fh:
            body = fh.read()
        nl = b"\r\n" if b"\r\n" in body else b"\n"
        lines = body.split(nl)
        entry = ("DecorVers           = %d          /* version of the theme "
                 "layout this theme is written for */" % THEME_VERSION).encode()
        for i, line in enumerate(lines):
            if line.startswith(b"DecorVers "):
                if line == entry:
                    return
                lines[i] = entry
                break
        else:
            first = lines[0].lstrip() if lines else b""
            lines.insert(1 if first.startswith(b"/*") and b"*/" in first else 0,
                         entry)
        self.log("  System/Config: DecorVers = %d" % THEME_VERSION)
        if not self.opts.dry_run:
            with open(cfg, "wb") as fh:
                fh.write(nl.join(lines))

    def run(self):
        self.log("%s:" % self.theme)
        self.convert_decoration()
        self.convert_gadgets()
        self.set_version()
        if self.opts.remove and not self.opts.dry_run:
            for path in self.removed:
                if not os.path.exists(path):
                    continue
                os.remove(path)
                # drop directories this leaves empty
                parent = os.path.dirname(path)
                while parent != self.theme and not os.listdir(parent):
                    os.rmdir(parent)
                    parent = os.path.dirname(parent)


def main():
    ap = argparse.ArgumentParser(
        description="Convert AROS themes to the MUI style image layout.")
    ap.add_argument("themes", nargs="+", metavar="theme",
                    help="theme directory to convert")
    ap.add_argument("--size", type=int, default=16,
                    help="size the theme's images were drawn for (default 16)")
    ap.add_argument("--scaled", type=int, action="append", default=None,
                    metavar="SIZE",
                    help="also create scaled button images for SIZE "
                         "(default 11; may be given more than once)")
    ap.add_argument("--no-scaled", action="store_true",
                    help="do not create scaled button images")
    ap.add_argument("--all-states", action="store_true",
                    help="write every state, even ones that match their fallback")
    ap.add_argument("--remove", action="store_true",
                    help="remove the original files once converted")
    ap.add_argument("-n", "--dry-run", action="store_true",
                    help="show what would be done without writing anything")
    ap.add_argument("-q", "--quiet", action="store_true")
    opts = ap.parse_args()
    if opts.no_scaled:
        opts.scaled = []
    elif opts.scaled is None:
        opts.scaled = [11]

    status = 0
    for theme in opts.themes:
        if not os.path.isdir(theme):
            print("%s: not a directory" % theme, file=sys.stderr)
            status = 1
            continue
        try:
            Converter(theme.rstrip("/"), opts).run()
        except (ValueError, OSError, zlib.error) as err:
            print("%s: %s" % (theme, err), file=sys.stderr)
            status = 1
    return status


if __name__ == "__main__":
    sys.exit(main())
