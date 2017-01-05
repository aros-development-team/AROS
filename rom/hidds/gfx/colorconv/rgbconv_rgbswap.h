#define SWAP32CODE                                  \
    ULONG *src = (ULONG *)srcPixels;                \
    ULONG *dst = (ULONG *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            ULONG s = src[x];                       \
                                                    \
            dst[x] = AROS_SWAP_BYTES_LONG(s);       \
        }                                           \
        src = (ULONG *)(((UBYTE *)src) + srcMod);   \
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP16CODE                                  \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            dst[x] = AROS_SWAP_BYTES_WORD(s);       \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP2432CODE                                \
    UBYTE *src = (UBYTE *)srcPixels;                \
    ULONG *dst = (ULONG *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                                \
        for(x = 0; x < width; x++)                  \
        {                                           \
            ULONG s = GET24_INV;                    \
                                                    \
            dst[x] = s;                             \
        }                                           \
        src = (UBYTE *)(((UBYTE *)src) + srcMod);   \
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP2424CODE                                \
    UBYTE *src = (UBYTE *)srcPixels;                \
    UBYTE *dst = (UBYTE *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            dst[x * 3]     = src[x * 3 + 2];        \
            dst[x * 3 + 1] = src[x * 3 + 1];        \
            dst[x * 3 + 2] = src[x * 3];            \
        }                                           \
        src = (UBYTE *)(((UBYTE *)src) + srcMod);   \
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1515CODE                                \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            dst[x] = ((s & RGB15_RMASK) >> 10) |    \
                     ((s & RGB15_GMASK)) |          \
                     ((s & RGB15_BMASK) << 10);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP15OE15CODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            dst[x] = ((s & RGB15_RMASK) >> 10) |    \
                     ((s & RGB15_GMASK)) |          \
                     ((s & RGB15_BMASK) << 10);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1515OECODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            s = ((s & RGB15_RMASK) >> 10) |         \
                     ((s & RGB15_GMASK)) |          \
                     ((s & RGB15_BMASK) << 10);     \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP15OE15OECODE                            \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            s = ((s & RGB15_RMASK) >> 10) |         \
                     ((s & RGB15_GMASK)) |          \
                     ((s & RGB15_BMASK) << 10);     \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1616CODE                                \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            dst[x] = ((s & RGB16_RMASK) >> 11) |    \
                     ((s & RGB16_GMASK)) |          \
                     ((s & RGB16_BMASK) << 11);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP16OE16CODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            dst[x] = ((s & RGB16_RMASK) >> 11) |    \
                     ((s & RGB16_GMASK)) |          \
                     ((s & RGB16_BMASK) << 11);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1616OECODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            s = ((s & RGB16_RMASK) >> 11) |         \
                ((s & RGB16_GMASK)) |               \
                ((s & RGB16_BMASK) << 11);          \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP16OE16OECODE                            \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            s = ((s & RGB16_RMASK) >> 11) |         \
                ((s & RGB16_GMASK)) |               \
                ((s & RGB16_BMASK) << 11);          \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1516CODE                                \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            dst[x] = ((s & RGB15_RMASK) >> 10) |    \
                     ((s & RGB15_GMASK) << 1) |     \
                     ((s & RGB15_BMASK) << 11);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP15OE16CODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            dst[x] = ((s & RGB15_RMASK) >> 10) |    \
                     ((s & RGB15_GMASK) << 1) |     \
                     ((s & RGB15_BMASK) << 11);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1516OECODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            s = ((s & RGB15_RMASK) >> 10) |         \
                ((s & RGB15_GMASK) << 1) |          \
                ((s & RGB15_BMASK) << 11);          \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP15OE16OECODE                            \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            s = ((s & RGB15_RMASK) >> 10) |         \
                ((s & RGB15_GMASK) << 1) |          \
                ((s & RGB15_BMASK) << 11);          \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1615CODE                                \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            dst[x] = ((s >> 11) & RGB15_BMASK) |    \
                     ((s >> 1) & RGB15_GMASK) |     \
                     ((s << 10) & RGB15_RMASK);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP16OE15CODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            dst[x] = ((s >> 11) & RGB15_BMASK) |    \
                     ((s >> 1) & RGB15_GMASK) |     \
                     ((s << 10) & RGB15_RMASK);     \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

#define SWAP1615OECODE                              \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = src[x];                       \
                                                    \
            s = ((s >> 11) & RGB15_BMASK) |         \
                ((s >> 1) & RGB15_GMASK) |          \
                ((s << 10) & RGB15_RMASK);          \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;


#define SWAP16OE15OECODE                            \
    UWORD *src = (UWORD *)srcPixels;                \
    UWORD *dst = (UWORD *)dstPixels;                \
    ULONG x, y;                                     \
                                                    \
    for(y = 0; y < height; y++)                     \
    {                                               \
        for(x = 0; x < width; x++)                  \
        {                                           \
            UWORD s = INV16(src[x]);                \
                                                    \
            s = ((s >> 11) & RGB15_BMASK) |         \
                ((s >> 1) & RGB15_GMASK) |          \
                ((s << 10) & RGB15_RMASK);          \
            dst[x] = INV16(s);                      \
        }                                           \
        src = (UWORD *)(((UBYTE *)src) + srcMod);   \
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);   \
    }                                               \
                                                    \
    return 1;

