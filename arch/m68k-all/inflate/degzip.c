/*
 * degzip_gnu.c
 * 
 * Analyse and extract DEFLATE streams from gzip archives.
 * Original version, for GNU/Linux.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

/* __packed: Given struct should not include ABI-compliant padding. */
#define __packed __attribute__((packed))

/* Our own custom pack header which is written to the output file when
 * -H option is specified. In addition, a CRC16-CCITT is computed over 
 * the entire output file and written in an extra two bytes at the end. 
 * All fields (and the trailing CRC) are big endian. */
struct pack_header {
    /* Total size of this compressed file, in bytes, including this header and 
     * the trailing 2-byte CRC. */
    uint32_t in_bytes;
    /* Size of decompressed output, in bytes. */
    uint32_t out_bytes;
    /* CRC16-CCITT over the decompressed output. */
    uint16_t out_crc;
    /* Leeway which must be given to the decompressor if input and output 
     * share the same memory. If the compressed DEFLATE stream is placed at 
     * the end of the output buffer, then that buffer must be sized 
     * (out_bytes + leeway) to correctly decompress without overwriting 
     * the remaining tail of the input stream. */
    uint16_t leeway;
} __packed;

#define ASSERT(p) do { \
    if (!(p)) errx(1, "Assertion at %u", __LINE__); } while (0)

static uint32_t nr__lit, nr__len, tot__cplen, nr__codes, nr__codebits;
static uint32_t nr__longcodes;

struct gzip_header {
    uint8_t id[2];
    uint8_t cm;
    uint8_t flg;
#define FTEXT    (1u<<0)
#define FHCRC    (1u<<1)
#define FEXTRA   (1u<<2)
#define FNAME    (1u<<3)
#define FCOMMENT (1u<<4)
#define FRSRVD   (0xe0u)
    uint32_t mtime;
    uint8_t xfl;
    uint8_t os;
} __packed;

struct gzip_extra {
    uint16_t xlen;
    uint8_t xdat[0];
} __packed;

static uint32_t crc32_tab[256];
static void crc32_tab_init(void)
{
    unsigned int i, j;
    for (i = 0; i < 256; i++) {
        uint32_t c = i;
        for (j = 0; j < 8; j++)
            c = (c >> 1) ^ ((c & 1) ? 0xedb88320 : 0);
        crc32_tab[i] = c;
    }
}

uint32_t crc32_add(const void *buf, size_t len, uint32_t crc)
{
    unsigned int i;
    const char *b = buf;
    crc = ~crc;
    for (i = 0; i < len; i++)
        crc = crc32_tab[(uint8_t)(crc ^ *b++)] ^ (crc >> 8);
    return ~crc;
}

uint32_t crc32(const void *buf, size_t len)
{
    return crc32_add(buf, len, 0);
}

static uint16_t crc16tab[256];
static void crc16_gentable(void)
{
    uint16_t crc, i, j;
    for (i = 0; i < 256; i++) {
        crc = i << 8;
        for (j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? 0x1021 ^ (crc << 1) : (crc << 1);
        crc16tab[i] = crc;
    }
}

static uint16_t crc16_ccitt(const void *buf, size_t len, uint16_t crc)
{
    unsigned int i;
    const uint8_t *b = buf;
    for (i = 0; i < len; i++)
        crc = crc16tab[(crc>>8)^*b++] ^ (crc<<8);
    return crc;
}

struct deflate_stream {
    struct {
        uint8_t *p, *end;
    } in, out;
    uint32_t cur, nr; /* input bit tracking */
    uint32_t leeway;
};

static uint32_t deflate_stream_next_bits(
    struct deflate_stream *s, unsigned int nr)
{
    uint32_t w;
    while (s->nr < nr) {
        ASSERT(s->in.p < s->in.end);
        s->cur |= *(s->in.p++) << s->nr;
        s->nr += 8;
    }
    w = s->cur & ((1u << nr) - 1);
    s->cur >>= nr;
    s->nr -= nr;
    return w;
}

/* Longest possible code. */
#define MAX_CODE_LEN   16

/* (Maximum) alphabet sizes. */
#define nr_codelen_symbols  19
#define nr_litlen_symbols  288
#define nr_distance_symbols 32

/* Order of code lengths for the code length alphabet. */
static const uint8_t codelen_order[nr_codelen_symbols] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

/* Base values and extra bits for lit/len symbols 257+. */
static const uint16_t length_base[] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const uint8_t length_extrabits[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };

/* Base values and extra bits for distance symbols. */
static const uint16_t dist_base[] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
    769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const uint8_t dist_extrabits[] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

struct node {
    uint16_t l, r;
#define LEAF 0x1000
};

static uint32_t deflate_stream_next_symbol(
    struct deflate_stream *s, struct node *tree)
{
    struct node *node = tree;
    uint16_t idx, bits = 0;

    for (;;) {
        bits++;
        idx = deflate_stream_next_bits(s, 1) ? node->r : node->l;
        if (idx & LEAF)
            break;
        if (idx == 0)
            errx(1, "Bad symbol decode");
        node = &tree[idx];
    }

    nr__codebits += bits;
    nr__codes++;
    if (bits > 8)
        nr__longcodes++;

    return idx & 0xfffu;
}

static void build_code(uint8_t *len, struct node *nodes, uint16_t nr_symbols)
{
    struct node *node;
    uint16_t *pnode, next_node;
    uint16_t i, code, mask, bl_count[MAX_CODE_LEN+1];
    uint16_t next_code[MAX_CODE_LEN+1]; /* XXX merge with bl_count */
    uint8_t l;

    memset(bl_count, 0, (MAX_CODE_LEN+1)*2);
    for (i = 0; i < nr_symbols; i++)
        bl_count[len[i]]++;
    bl_count[0] = 0; /* zero-length codes are not included */

    code = 0;
    for (i = 1; i <= MAX_CODE_LEN; i++) {
        code = (code + bl_count[i-1]) << 1;
        next_code[i] = code;
    }

    memset(nodes, 0, nr_symbols*sizeof(*nodes));
    next_node = 1;
    for (i = 0; i < nr_symbols; i++) {
        if (!(l = len[i]))
            continue;
        code = next_code[l]++;
        mask = 1u << (l - 1);
        node = &nodes[0];
        for (;;) {
            pnode = (code & mask) ? &node->r : &node->l;
            if (!(mask >>= 1))
                break;
            if (!*pnode)
                *pnode = next_node++;
            node = &nodes[*pnode];
        }
        *pnode = LEAF | i;
    }
}

static void huffman(struct deflate_stream *main, struct deflate_stream *prefix)
{
    uint16_t hlit, i, litlen_sym;
    uint8_t hdist, hclen, len[nr_litlen_symbols + nr_distance_symbols];
    struct node codelen_tree[nr_codelen_symbols];
    struct node litlen_tree[nr_litlen_symbols];
    struct node dist_tree[nr_distance_symbols];
    struct deflate_stream *s = prefix ?: main;

    memset(codelen_tree, 0, sizeof(codelen_tree));
    memset(litlen_tree, 0, sizeof(litlen_tree));

    hlit = deflate_stream_next_bits(s, 5) + 257;
    ASSERT(hlit <= nr_litlen_symbols);
    hdist = deflate_stream_next_bits(s, 5) + 1;
    ASSERT(hdist <= nr_distance_symbols);
    hclen = deflate_stream_next_bits(s, 4) + 4;
    ASSERT(hclen <= nr_codelen_symbols);

    for (i = 0; i < hclen; i++)
        len[codelen_order[i]] = deflate_stream_next_bits(s, 3);
    for (; i < nr_codelen_symbols; i++)
        len[codelen_order[i]] = 0;
    build_code(len, codelen_tree, nr_codelen_symbols);

    /* Literal & distance code lengths stored in a single coded stream. */
    for (i = 0; i < hlit+hdist;) {
        switch (len[i] = deflate_stream_next_symbol(s, codelen_tree)) {
        case 16: {
            uint8_t l = len[i-1], rep = deflate_stream_next_bits(s, 2) + 3;
            while (rep--) len[i++] = l;
            break;
        }
        case 17: {
            uint8_t rep = deflate_stream_next_bits(s, 3) + 3;
            while (rep--) len[i++] = 0;
            break;
        }
        case 18: {
            uint8_t rep = deflate_stream_next_bits(s, 7) + 11;
            while (rep--) len[i++] = 0;
            break;
        }
        default: /* 0..15 */
            i++;
            break;
        }
    }
    build_code(&len[0], litlen_tree, hlit);
    build_code(&len[hlit], dist_tree, hdist);

    s = main;
    while ((litlen_sym = deflate_stream_next_symbol(s, litlen_tree)) != 256) {
        int32_t overtake;
        uint16_t dist_sym, cplen, cpdst;
        uint8_t *p;

        if (litlen_sym < 256) {
            ASSERT(s->out.p < s->out.end);
            *(s->out.p++) = litlen_sym;
            nr__lit++;
        } else {
            /* 257+ */
            nr__len++;
            cplen = length_base[litlen_sym-257] +
                deflate_stream_next_bits(s, length_extrabits[litlen_sym-257]);
            dist_sym = deflate_stream_next_symbol(s, dist_tree);
            cpdst = dist_base[dist_sym] +
                deflate_stream_next_bits(s, dist_extrabits[dist_sym]);
            p = s->out.p - cpdst;
            ASSERT((s->out.p + cplen) <= s->out.end);
            tot__cplen += cplen;
            while (cplen--)
                *(s->out.p++) = *p++;
        }
        /* Calculate the amount by which the output has overtaken the 
         * remaining input stream, if the input stream were located at the 
         * far end of the output buffer. Track the max of that value. */
        overtake = (s->in.end - s->in.p) - (s->out.end - s->out.p);
        if (overtake > (int32_t)s->leeway)
            s->leeway = overtake;
    }
}

static void uncompressed_block(struct deflate_stream *s)
{
    uint16_t len, nlen;
    s->cur = s->nr = 0; /* byte boundary */
    len = deflate_stream_next_bits(s, 16);
    nlen = deflate_stream_next_bits(s, 16);
    ASSERT(nlen == (uint16_t)~len);
    ASSERT((s->in.p + len) <= s->in.end);
    ASSERT((s->out.p + len) <= s->out.end);
    memcpy(s->out.p, s->in.p, len);
    s->in.p += len;
    s->out.p += len;
}

static void deflate_process_block(struct deflate_stream *s)
{
    uint8_t bfinal, btype;

    do {
        bfinal = deflate_stream_next_bits(s, 1);
        btype = deflate_stream_next_bits(s, 2);

        switch (btype) {
        case 0:
            uncompressed_block(s);
            break;
        case 1: {
            static const uint8_t static_prefix[] = {
                0xff, 0x5b, 0x00, 0x6c, 0x03, 0x36, 0xdb,
                0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb, 0xb6,
                0xcd, 0xdb, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d,
                0xdb, 0xa8, 0x6d, 0xce, 0x8b, 0x6d, 0x3b };
            struct deflate_stream dfls_prefix = {
                .in.p = (uint8_t *)static_prefix,
                .in.end = (uint8_t *)static_prefix + sizeof(static_prefix) };
            huffman(s, &dfls_prefix);
            break;
        }
        case 2:
            huffman(s, NULL);
            break;
        default:
            errx(1, "Unknown DEFLATE block type %u", btype);
        }
    } while (!bfinal);
}

static void usage(int rc)
{
    printf("Usage: degzip [options] in_file out_file\n");
    printf("Options:\n");
    printf("  -h, --help    Display this information\n");
    printf("  -H, --header  Output DEFLATE stream with a useful header\n");
    printf("  -r, --raw     Output raw DEFLATE stream [Default]\n");
    printf("  -u, --uncompressed  Output original uncompressed data\n");
    exit(rc);
}

int main(int argc, char **argv)
{
    struct deflate_stream dfls;
    struct gzip_header *hdr;
    unsigned char *buf, *p, *outbuf, *dflp;
    int ch, fd, i;
    uint32_t insz, outsz, crc = 0;
    char *in, *out;
    enum {
        output_header, output_raw, output_uncompressed
    } output_type = output_raw;

    const static char sopts[] = "hHru";
    const static struct option lopts[] = {
        { "help", 0, NULL, 'h' },
        { "header", 0, NULL, 'H' },
        { "raw", 0, NULL, 'r' },
        { "uncompressed", 0, NULL, 'u' },
        { 0, 0, 0, 0 }
    };

    while ((ch = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ch) {
        case 'h':
            usage(0);
            break;
        case 'H':
            output_type = output_header;
            break;
        case 'r':
            output_type = output_raw;
            break;
        case 'u':
            output_type = output_uncompressed;
            break;
        default:
            usage(1);
            break;
        }
    }

    if (argc != (optind + 2))
        usage(1);
    in = argv[optind];
    out = argv[optind+1];

    crc16_gentable();

    fd = open(in, O_RDONLY);
    if (fd == -1)
        err(1, "%s", in);
    if ((insz = lseek(fd, 0, SEEK_END)) < 0)
        err(1, NULL);
    lseek(fd, -4, SEEK_END);
    if (read(fd, &outsz, 4) != 4)
        err(1, NULL);
    outsz = le32toh(outsz);
    if ((buf = malloc(insz)) == NULL)
        err(1, NULL);
    if ((outbuf = malloc(outsz)) == NULL)
        err(1, NULL);
    lseek(fd, 0, SEEK_SET);
    if (read(fd, buf, insz) != insz)
        err(1, NULL);
    close(fd);
    p = buf;

    hdr = (struct gzip_header *)p;
    p += sizeof(struct gzip_header);
    if ((hdr->id[0] != 0x1f) || (hdr->id[1] != 0x8b))
        errx(1, "Not a GZIP file");
    if (hdr->cm != 8)
        errx(1, "Compression method %u unsupported", hdr->cm);

    if (hdr->flg & FRSRVD)
        errx(1, "Unrecognised header flags %02x", hdr->flg);

    if (hdr->flg & FEXTRA) {
        struct gzip_extra *extra = (struct gzip_extra *)p;
        extra->xlen = le16toh(extra->xlen);
        printf("Skipping %u bytes extra data\n", extra->xlen);
        p += sizeof(struct gzip_extra) + extra->xlen;
    }

    if (hdr->flg & FNAME) {
        printf("Filename: '%s'\n", p);
        p += strlen((char *)p) + 1;
    }

    if (hdr->flg & FCOMMENT) {
        printf("Comment: '%s'\n", p);
        p += strlen((char *)p) + 1;
    }

    if (hdr->flg & FHCRC) {
        printf("CRC16\n");
        p += 2;
    }

    printf("Compressed size: %u; Original size: %u\n", insz, outsz);

    dfls.cur = dfls.nr = 0;
    dfls.in.p = dflp = p;
    dfls.in.end = buf + insz - 8;
    dfls.out.p = outbuf;
    dfls.out.end = outbuf + outsz;
    dfls.leeway = 0;
    deflate_process_block(&dfls);
    ASSERT(dfls.in.p == dfls.in.end);
    ASSERT(dfls.out.p == dfls.out.end);

    for (i = 3; i >= 0; i--) {
        crc <<= 8;
        crc |= dfls.in.p[i];
    }
    crc32_tab_init();
    if (crc != crc32(outbuf, outsz))
        errx(1, "CRC32 mismatch on original data");

    printf("Nr lits: %u, Nr lens: %u, Av len: %u\n",
           nr__lit, nr__len, nr__len ? tot__cplen / nr__len : 0);
    printf("Lit/Len Ratio: %f\n", nr__len ? (float)nr__lit/nr__len : 0.0);
    printf("Nr Codes: %u, Av bits: %f, Longcodes Ratio: %f\n",
           nr__codes, nr__codes ? (float)nr__codebits/nr__codes : 0.0,
           nr__codes ? (float)nr__longcodes/nr__codes : 0.0);
    printf("Decompression requires leeway of %u bytes\n", dfls.leeway);

    fd = open(out, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd == -1)
        err(1, "%s", out);

    switch (output_type) {
    case output_header: {
        uint16_t crc16;
        struct pack_header hdr;
        if (dfls.leeway != (uint16_t)dfls.leeway)
            errx(1, "Leeway too lareg to fit in pack header");
        hdr.leeway = htobe16(dfls.leeway);
        hdr.out_bytes = htobe32(outsz);
        outsz = dfls.in.end-dflp;
        hdr.in_bytes = htobe32(outsz + sizeof(hdr) + 2/*crc16*/);
        hdr.out_crc = htobe16(crc16_ccitt(outbuf, be32toh(hdr.out_bytes),
                                          0xffff));
        crc16 = crc16_ccitt(&hdr, sizeof(hdr), 0xffff);
        crc16 = crc16_ccitt(dflp, outsz, crc16);
        crc16 = htobe16(crc16);
        if ((write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) ||
            (write(fd, dflp, outsz) != outsz) ||
            (write(fd, &crc16, 2) != 2))
            err(1, NULL);
        break;
    }
    case output_raw: {
        outsz = dfls.in.end-dflp;
        if (write(fd, dflp, outsz) != outsz)
            err(1, NULL);
        break;
    }
    case output_uncompressed: {
        /* Output original uncompressed data. */
        if (write(fd, outbuf, outsz) != outsz)
            err(1, NULL);
        break;
    }
    }

    close(fd);

    return 0;
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */