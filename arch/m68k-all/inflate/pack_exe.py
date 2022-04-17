# pack_exe.py
# 
# Convert an Amiga load file into a self-unpacking executable.
# 
# Written & released by Keir Fraser <keir.xen@gmail.com>
# 
# This is free and unencumbered software released into the public domain.
# See the file COPYING for more details, or visit <http://unlicense.org>.

import crcmod.predefined
import struct, sys, os

st_source = 1
st_dest = 2
st_tools = 3
st_frags = 4
st_gzip = 5

# Hunk/Block identifiers.
HUNK_HEADER  = 0x3f3
HUNK_CODE    = 0x3e9
HUNK_DATA    = 0x3ea
HUNK_BSS     = 0x3eb
HUNK_RELOC32 = 0x3ec
HUNK_END     = 0x3f2

# Dictionary of Hunk/Block names.
hname = {
    HUNK_CODE: 'Code',
    HUNK_DATA: 'Data',
    HUNK_BSS:  'BSS ',
}

memname = {
    0: '',
    1: 'Chip',
    2: 'Fast',
    3: 'Extra-Attr',
}

# Minimum number of bytes that compression must save.
MIN_COMPRESSION = 8

# Prefix for intermediate (temporary) files.
PREFIX = '_pack'

# Relocation table:
# u16 hunk (0xffff = sentinel)
# u16 nr (0 = sentinel)
# u16 target
# u8 delta1[, delta3[3]] (delta3 iff delta1==0)
# delta = (deltaN + 1) * 2
relocs = bytes()

# List of allocation sizes and attributes (HUNK_HEADER).
allocs = []

# DEFLATE stream size (in bytes) for each hunk.
# If 0 then that hunk is stored uncompressed.
stream_sizes = []

# List of output hunks
hunks = []

# Summary information about each hunk's processing
infos = []

# Delta-encode a list of Amiga RELOC32 offsets:
# Delta_n = (Off_n - Off_n-1) / 2 - 1; Off_0 = -4
def process_relocs(target, offs):
    global relocs
    offs.sort()
    relocs += struct.pack('>2H', len(offs), target)
    p = -4
    for o in offs:
        assert o > p and -(o-p)&1 == 0
        delta = ((o - p) >> 1) - 1
        assert 0 <= delta <= ((1<<24)-1)
        relocs += struct.pack('>B' if 1 <= delta <= 255 else '>I', delta)
        p = o
    relocs += bytes(-len(relocs)&1)

# Get the (one) position-independent code hunk from an Amiga load file.
def get_code(fragdir, name):
    with open(fragdir + name, 'rb') as f:
        (id, x, nr, first, last) = struct.unpack('>5I', f.read(5*4))
        assert id == HUNK_HEADER and x == 0
        assert nr == 1 and first == 0 and last == 0
        (x, id, nr) = struct.unpack('>3I', f.read(3*4))
        assert id == HUNK_CODE and nr == x
        code = bytes(f.read(nr * 4))
        (id,) = struct.unpack('>I', f.read(4))
        assert id == HUNK_END
    #print('"%s": %u bytes (%u longs)' % (name, len(code), len(code)//4))
    return code

# Compress the given raw byte sequence.
def pack(tooldir, tmpname, gzcmd, raw):
    # Compress to a DEFLATE stream.
    with open(tmpname + PREFIX, 'wb') as f:
        f.write(raw)
    os.system(tooldir + gzcmd + ' ' + tmpname + PREFIX)
    os.system(tooldir + 'degzip -H ' + tmpname + PREFIX + '.gz ' + tmpname + PREFIX + '.raw'
              ' >/dev/null')
    with open(tmpname + PREFIX + '.raw', 'rb') as f:
        packed = f.read()
    (inb, outb, crc, leeway) = struct.unpack('>2I2H', packed[:12])
    # Extract the DEFLATE stream and check the header's length fields.
    inb -= 14
    packed = packed[12:-2]
    assert inb == len(packed) and outb == len(raw)
    # Check the header CRC.
    crc16 = crcmod.predefined.Crc('crc-ccitt-false')
    crc16.update(raw)
    assert crc == crc16.crcValue
    # Pad the DEFLATE stream.
    padding = -len(packed) & 3
    packed += bytes(padding)
    # Extend and pad leeway.
    leeway += padding
    leeway += -leeway & 3
    return (packed, crc, leeway)

# Read next hunk from the input file, compress it if appropriate, and appends
# the encoded output hunk to hunks[], updates allocs[i], and appends
# delta-encoded RELOC32 blocks to relocs[].
def process_hunk(tooldir, fragdir, tmpname, gzcmd, f, i):
    global allocs, stream_sizes, relocs, hunks, infos
    old_pos = f.tell()
    alloc = _alloc = (allocs[i] & 0x3fffffff) * 4
    first_reloc = True # Have we seen a RELOC32 block yet?
    seen_dat = False   # Have we seen CODE/DATA/BSS yet?
    hunk = bytes() # Full encoding of this hunk
    while True:
        (_id,) = struct.unpack('>I', f.read(4))
        id = _id & 0x3fffffff
        if id == HUNK_CODE or id == HUNK_DATA:
            if seen_dat:
                f.seek(-4, os.SEEK_CUR)
                break # Done with this hunk!
            seen_dat = id
            (_nr,) = struct.unpack('>I', f.read(4))
            nr = _nr & 0x3fffffff
            raw = f.read(nr*4)
            assert alloc >= len(raw)
            (packed, crc, leeway) = pack(tooldir, tmpname, gzcmd, raw)
            if i != 0 and nr*4 < len(packed)+MIN_COMPRESSION:
                # This hunk is not worth compressing. Store it as is.
                packed = raw
                stream_sizes.append(0) # No compression
            else:
                if i == 0:
                    # First hunk must be code: we inject our entry/exit code
                    assert id == HUNK_CODE
                    packed = get_code(fragdir, 'depacker_entry') + packed
                    # Allocate explicit extra space for the final exit code
                    # This must always extend the allocation as these bytes
                    # will not be zeroed before we jump to the original exe.
                    alloc += 8
                # Extend the hunk allocation for depacker leeway.
                # We also deal with compression making the hunk longer here
                # (hunk 0 only, as other hunks we would store uncompressed).
                alloc = max(alloc, len(raw) + leeway, len(packed))
                stream_sizes.append(len(packed)) # DEFLATE stream size
            # Write out this block.
            hunk += struct.pack('>2I', id, len(packed)//4) + packed
        elif id == HUNK_BSS:
            assert i != 0
            if seen_dat:
                f.seek(-4, os.SEEK_CUR)
                break # Done with this hunk!
            seen_dat = id
            (_nr,) = struct.unpack('>I', f.read(4))
            nr = _nr & 0x3fffffff
            assert alloc >= nr*4
            stream_sizes.append(0) # No compression
            # Write out this block as is.
            hunk += struct.pack('>2I', id, alloc//4)
        elif id == HUNK_END:
            assert seen_dat
            break # Done with this hunk!
        elif id == HUNK_RELOC32:
            while True:
                (nr,) = struct.unpack('>I', f.read(4))
                if nr == 0:
                    break # Done with RELOC32
                (h,) = struct.unpack('>I', f.read(4))
                offs = list(struct.unpack('>%dI' % nr, f.read(nr*4)))
                if first_reloc:
                    # Write out this hunk's number.
                    relocs += struct.pack('>H', i)
                    first_reloc = False
                # Write out the target hunk number and delta-encoded offsets.
                process_relocs(h, offs)
        else:
            print("Unexpected hunk %04x" % (id))
            assert False
    # Generate HUNK_END (optional?)
    # hunk += struct.pack('>I', HUNK_END)
    if not first_reloc:
        # There were relocations for this hunk: Write the sentinel value.
        relocs += struct.pack('>H', 0)
    # Update the allocation size for this hunk.
    assert alloc&3 == 0
    allocs[i] &= 0xc0000000
    allocs[i] |= alloc // 4
    # Add this hunk to the global list for later write-out.
    hunks.append(hunk)
    infos.append((seen_dat, _alloc, alloc, f.tell()-old_pos, len(hunk)))

# Generate the final hunk, which contains the depacker, the packed-stream
# size table, the delta-encoded relocation tables, and the relocator.
# Everything except the depacker itself is stored compressed.
def generate_final_hunk(tooldir, fragdir, tmpname, gzcmd):
    global allocs, relocs, hunks, infos
    depacker = get_code(fragdir, 'depacker_main')
    # Generate the raw byte sequence for compression:
    # 1. Table of stream sizes;
    stream_sizes.append(0) # Sentinel value for the stream-size table
    raw = struct.pack('>%dI' % len(stream_sizes), *stream_sizes)
    # 2. Relocation and epilogue code; and 3. Relocation tables.
    raw += get_code(fragdir, 'depacker_packed') + relocs
    # Ensure everything is a longword multiple.
    raw += bytes(-len(raw) & 3)
    assert len(depacker)&3 == 0 and len(raw)&3 == 0
    # Allocation size covers the depacker and the depacked stream.
    alloc = len(depacker) + 4 + len(raw)
    # Compress the raw byte sequence.
    (packed, crc, leeway) = pack(tooldir, tmpname, gzcmd, raw)
    if len(raw) < len(packed)+MIN_COMPRESSION:
        # Not worth compressing, so don't bother.
        hunk = depacker + bytes(4) + raw # 'bytes(4)' means not packed
    else:
        # Compress everything except the depacker itself (and the
        # longword containing the size of the compressed stream).
        packed_len = len(depacker) + len(packed)
        alloc += leeway # Include depacker leeway in the hunk allocation
        hunk = depacker + struct.pack('>I', len(packed)) + packed
    assert alloc&3 == 0 and len(hunk)&3 == 0
    # Add the hunk header/footer metadata to the code block.
    hunk = struct.pack('>2I', HUNK_CODE, len(hunk)//4) + hunk
    hunk += struct.pack('>I', HUNK_END)
    # Add this hunk and its allocation size to the global lists.
    hunks.append(hunk)
    allocs.append(alloc//4)
    infos.append((0, 0, alloc, 0, len(hunk)))

def process(tooldir, fragdir, f, out_f, tmpname, gzcmd):
    global allocs, relocs, hunks

    # Read the load-file header of the input file, including every
    # hunk's original allocation size.
    (id, x, nr, first, last) = struct.unpack('>5I', f.read(5*4))
    assert id == HUNK_HEADER and x == 0
    assert first == 0 and last == nr-1 and nr > 0
    allocs = list(struct.unpack('>%dI' % nr, f.read(nr*4)))

    # Read and process each input hunk.
    for i in range(nr):
        process_hunk(tooldir, fragdir, tmpname, gzcmd, f, i)

    # Append the final sentinel value to the relocation table.
    relocs += struct.pack('>H', 0xffff)

    # Generate the depacker hunk.
    generate_final_hunk(tooldir, fragdir, tmpname, gzcmd)
    nr += 1

    # Remove intermediate temporary files.
    os.system('rm ' + tmpname + PREFIX + '*')

    # Write out the compressed executable: HUNK_HEADER, then each hunk in turn.
    out_f.write(struct.pack('>5I', HUNK_HEADER, 0, nr, 0, nr-1))
    out_f.write(struct.pack('>%dI' % nr, *allocs))
    [out_f.write(hunk) for hunk in hunks]

    # Return the original- and compressed-file sizes to the caller.
    f.seek(0, os.SEEK_END)
    in_sz = f.tell()
    out_sz = out_f.tell()
    return (in_sz, out_sz)

def usage(argv):
    print("%s -s <input-file> -d <output-file> [-q -g <gzip command> -t <tool dir> -f <fragment dir>]" % argv[0])
    sys.exit(1)

def main(argv):
    state = 0
    # The unpacker code fragments and the degzip binary are in the
    # same directory as the script by default.
    tooldir = os.path.split(os.path.abspath(__file__))[0] + '/'
    fragdir = tooldir
    gzcmd = "zopfli"
    source = ""
    dest = ""
    quiet = 0
    if sys.version_info[0] < 3:
        print("** Requires Python 3")
        usage(argv)
    for arg in argv:
        if arg == "-s":
            state = st_source
        elif arg == "-d":
            state = st_dest
        elif arg == "-t":
            state = st_tools
        elif arg == "-f":
            state = st_frags
        elif arg == "-g":
            state = st_gzip
        elif arg == "-q":
            quiet = 1
        elif arg == "-h":
            usage(argv)
        elif arg[0] == "-":
            print("pack_exe: unknown argument %s" % arg)
            sys.exit(1)
        else:
            if state == st_source:
                source = arg
            elif state == st_dest:
                dest = arg
            elif state == st_tools:
                tooldir = arg
            elif state == st_frags:
                fragdir = arg
            elif state == st_gzip:
                gzcmd = arg

    (in_sz, out_sz) = process(tooldir, fragdir, open(source, 'rb'), open(dest, 'wb'), os.path.basename(dest), gzcmd)

    if quiet == 0:
        tot_old_alloc = tot_new_alloc = tot_old_store = tot_new_store = 0
        for (id, old_alloc, new_alloc, old_store, new_store) in infos:
            tot_old_alloc += old_alloc
            tot_new_alloc += new_alloc
            tot_old_store += old_store
            tot_new_store += new_store

        
        print(' [Nr] Type      File (   delta,      %)    Memory (delta)')
        print('-----------------------------------------------------------')

        # Account for HUNK_HEADER: We grew it by 4 bytes (one extra hunk).
        hlen = (len(infos)+5)*4
        tot_old_store += hlen-4
        tot_new_store += hlen
        print(' [--] Header %7u (%+8u, %+5.1f%%)' % (hlen, 4, 400/(hlen-4)))

        # Stats summary for all the original hunks.
        for i in range(len(infos)-1):
            (id, old_alloc, new_alloc, old_store, new_store) = infos[i]
            if new_store != 0:
                print(' [%02u] %s %9u (%+8u, %+5.1f%%)   %7u (%+5u) %s' %
                      (i, hname[id], new_store, new_store-old_store,
                       (new_store-old_store)*100/old_store,
                       new_alloc, new_alloc-old_alloc,
                       memname[allocs[i] >> 30]))

        # Summarise the new depacker/relocation hunk.
        (id, old_alloc, new_alloc, old_store, new_store) = infos[-1]
        print(' [%02u] DEPACK %7u %20s %7u' %
              (len(infos)-1, new_store, '', new_alloc))

        # Print totals. Note that extra allocation reduces after unpacking:
        # The depacker/relocation hunk is freed before running the original exe.
        print('-----------------------------------------------------------')
        print('%20u (%+8u, %+5.1f%%)   %7u (%+5u)' %
              (tot_new_store, tot_new_store-tot_old_store,
               (tot_new_store-tot_old_store)*100/tot_old_store,
               tot_new_alloc, tot_new_alloc-tot_old_alloc))
        print('After depack:   %25s %7u (%+5u)' %
              ('', tot_new_alloc-new_alloc,
               tot_new_alloc-new_alloc-tot_old_alloc))

        # A very final summary.
        print('\n** RESULT:\n** Original: {} = {} bytes\n'
              '** Compressed: {} = {} bytes\n'
              '** Shrunk {} bytes ({:.1f}%)'
              .format(source, in_sz, dest, out_sz, in_sz - out_sz,
                      (in_sz-out_sz)*100/in_sz))

if __name__ == "__main__":
    main(sys.argv)
