/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/pngdt.h>
#include <datatypes/pictureclass.h>

#include <png.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

/**************************************************************************************************/

struct PNGStuff
{
    png_structp png_ptr;
    png_infop   png_info_ptr;
    png_infop   png_end_info_ptr;
    png_uint_32 png_width;
    png_uint_32 png_height;
    int         png_bits;
    int         png_type;
    int         png_lace;
    int         png_num_lace_passes;
    int         png_depth;
    int         png_format;
};

struct PNGHandle
{
    struct PNGStuff stuff;
    APTR            data;
    APTR            pal;
};

struct MyMemHandle
{
    UBYTE *address;
    ULONG  pos;
    ULONG  size;
};

/***************************************************************************************************/

png_voidp my_malloc_fn(png_structp png_ptr, png_size_t size);
void my_free_fn(png_structp png_ptr, png_voidp ptr);
void my_error_fn(png_structp png_ptr, png_const_charp error_msg);
void my_warning_fn(png_structp png_ptr, png_const_charp warning_msg);
void my_read_fn(png_structp png_ptr, png_bytep data, png_size_t length);

/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, PNG_CheckSig,

/*  SYNOPSIS */
            AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
            struct Library *, PNGBase, 6, PNG)
/*  FUNCTION
        Open a file, read its first eight bytes, and compare them with the PNG
        signature. Close the file before returning. No image data is decoded.

    INPUTS
        name - Non-NULL, NUL-terminated DOS filename. Relative paths are
            resolved in the calling process's current directory.

    RESULT
        8 if all eight signature bytes match. 0 if eight bytes were read but
        do not match. -1 if the file could not be opened or fewer than eight
        bytes were read. No separate result distinguishes a short file from
        an I/O error.

    NOTES
        Include <proto/pngdt.h> and open datatypes/png.datatype into PNGBase.
        This is the datatype's direct interface, not png.library. Call from
        DOS process context where file I/O is permitted. The call can block
        in Open(), Read() or Close(); it supplies no timeout or cancellation.

        A matching signature is not a complete PNG validation or a promise
        that PNG_LoadImage() will succeed. The function neither returns an
        open handle nor defines a decoder-specific IoErr() result.

    SEE ALSO
        PNG_LoadImage(), PNG_LoadImageFH(), PNG_LoadImageMEM()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR fh;
    LONG ret = -1;
    
    if ((fh = Open(name, MODE_OLDFILE)))
    {
        png_byte header[8];
        
        if (Read(fh, header, sizeof(header)) == sizeof(header))
        {
            if (png_sig_cmp(header, 0, sizeof(header)) == 0)
            {
                ret = 8;
            }
            else
            {
                ret = 0;
            }
        }
        
        Close(fh);
        
    }
    
    return ret;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

#define HANDLETYPE_FILE 0
#define HANDLETYPE_MEM  1

/**************************************************************************************************/

/*
 * Read a requested byte range from the caller-owned memory stream used by
 * libpng. The callback advances the cursor only after a successful copy;
 * png_error() handles a reported end-of-stream failure through libpng's
 * error path. Neither the buffer nor the stream descriptor is owned here.
 * The pos + length range check can overflow, and a NULL source address is
 * not checked before copying. Callers must provide a valid bounded stream.
 */
void my_readmem_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct MyMemHandle *mh = png_get_io_ptr(png_ptr);
    
    if (mh->pos + length > mh->size)
    {
        png_error(png_ptr, "Read mem error!");
    }
    else
    {
        memcpy(data, mh->address + mh->pos, length);
        mh->pos += length;
    }
}

/***************************************************************************************************/

/*
 * Decode a PNG from a borrowed DOS file handle or memory stream. Read and
 * validate the signature, configure libpng's row transformations, allocate
 * an owned image handle and optionally copy selected ancillary chunks into
 * separately owned result slots. The caller retains the input stream and
 * must release any returned chunks independently of the image handle.
 * Errors normally return NULL. Chunk copies already published to the caller
 * are not rolled back, and automatic cleanup pointers modified after
 * setjmp() are not reliable following a decoder longjmp. Image-size and
 * allocation-size arithmetic is not overflow-checked.
 */
static APTR PNG_LoadImageInternal(APTR handle, CONST_STRPTR const *chunkstoread, APTR *chunkstore,
                                  BOOL makeARGB, LONG handletype)
{
    struct PNGStuff   png;
    struct PNGHandle *pnghandle = NULL;
    APTR              buffer = NULL;
    UBYTE             header[8];
    APTR              retval = NULL;
    BOOL              ok = TRUE;
    
    if (!handle) return NULL;
    
    D(bug("PNG_LoadImageInternal: Handle type %d\n", handletype));
    
    if (handletype == HANDLETYPE_FILE)
    {
        if (Read(MKBADDR(handle), header, sizeof(header)) != sizeof(header)) ok = FALSE;
    }
    else if (handletype == HANDLETYPE_MEM)
    {
        struct MyMemHandle *mh = (struct MyMemHandle *)handle;
        
        if (mh->size < 8)
        {
            D(bug("PNG_LoadImageInternal: Memory file too small: %d\n", mh->size));
            ok = FALSE;
        }
        else
        {
            memcpy(header, mh->address, 8);
            mh->pos = 8;
        }
    }
    else
    {
        ok = FALSE;
    }
    
    D(bug("PNG_LoadImageInternal: Header %02x%02x%02x%02x%02x%02x%02x%02x\n",
          header[0], header[1], header[2], header[3],
          header[4], header[5], header[6], header[7]));
          
    memset(&png, 0, sizeof(png));
    
    if (ok)
    {
        if (png_sig_cmp(header, 0, sizeof(header)) != 0) ok = FALSE;
    }
    
    if (ok)
    {
        D(bug("PNG_LoadImageInternal: signature okay\n"));
        memset(&png, 0, sizeof(png));
            
        png.png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
                                               0,               /* error ptr */
                                               my_error_fn,     /* error function */
                                               my_warning_fn,   /* warning function */
                                               0,               /* mem ptr */
                                               my_malloc_fn,    /* malloc function */
                                               my_free_fn       /* free function */
                                               );
            
        if (!png.png_ptr) ok = FALSE;
    }
    
    if (ok)
    {
        png.png_info_ptr = png_create_info_struct(png.png_ptr);
        if (!png.png_info_ptr) ok = FALSE;
    }
    
    
    if (ok)
    {
        png.png_end_info_ptr = png_create_info_struct(png.png_ptr);
        if (!png.png_end_info_ptr) ok = FALSE;
    }
    
    if (ok)
    {
        png_set_read_fn(png.png_ptr,
                        handle,
                        (handletype == HANDLETYPE_FILE) ? my_read_fn : my_readmem_fn);
                        
        png_set_sig_bytes(png.png_ptr, sizeof(header));
        
    }
    
    if (ok)
    {
        if (setjmp(png_jmpbuf(png.png_ptr)))
        {
            /* Error */

            ok = FALSE;
        }
    }
    
    if (ok)
    {
        if (chunkstoread)
        {
            int i;

            /* CHECKME:
            
               Do this first, because it doesn't work otherwise. Maybe
               a bug in libpng. The problem is that libpng/pngrutil.c
               in png_handle_unknown() checks PNG_FLAG_KEEP_UNKNOWN_CHUNKS
               flag. But this flag is set only when num_chunks param passed
               to png_set_keep_unknown_chunks() is 0.
               
            */
               
            png_set_keep_unknown_chunks(png.png_ptr, 3, 0, 0);
            
            for(i = 0; chunkstoread[i]; i++)
            {
                png_set_keep_unknown_chunks(png.png_ptr, 3, chunkstoread[i], 1);
            }
                                                            
        }

        png_read_info(png.png_ptr, png.png_info_ptr);

        png_get_IHDR(png.png_ptr, png.png_info_ptr,
                     &png.png_width, &png.png_height, &png.png_bits,
                     &png.png_type, &png.png_lace, NULL, NULL);


        if (png.png_bits == 16)
        {
            png_set_strip_16(png.png_ptr);
            png.png_bits = 8;
        }
        else if (png.png_bits < 8)
        {
            png_set_packing(png.png_ptr);
            if (png.png_type == PNG_COLOR_TYPE_GRAY)
            {
                png_set_expand_gray_1_2_4_to_8(png.png_ptr);
            }
            png.png_bits = 8;
        }

        if (png.png_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        {
            png_set_strip_alpha(png.png_ptr);
            png.png_type = PNG_COLOR_TYPE_GRAY;
        }

        if (makeARGB)
        {
            if (png.png_type == PNG_COLOR_TYPE_RGB)
            {
                png_set_filler(png.png_ptr, 0xFF, PNG_FILLER_BEFORE);
            }
            else if (png.png_type == PNG_COLOR_TYPE_GRAY)
            {
                png_set_gray_to_rgb(png.png_ptr);
                png_set_filler(png.png_ptr, 0xFF, PNG_FILLER_BEFORE);
            }
            else if (png.png_type == PNG_COLOR_TYPE_PALETTE)
            {
                png_set_palette_to_rgb(png.png_ptr);
                if (png_get_valid(png.png_ptr, png.png_info_ptr, PNG_INFO_tRNS))
                {
                    png_set_tRNS_to_alpha(png.png_ptr);
                }
                else
                {
                    png_set_filler(png.png_ptr, 0xFF, PNG_FILLER_BEFORE);
                }
            }

            png.png_type = PNG_COLOR_TYPE_RGB_ALPHA;
        }

        {
            double png_file_gamma;
            double png_screen_gamma = 2.2;

            if (!(png_get_gAMA(png.png_ptr, png.png_info_ptr, &png_file_gamma)))
            {
                png_file_gamma = 0.45455;
            }

            png_set_gamma(png.png_ptr, png_file_gamma, png_screen_gamma);
        }

        png.png_num_lace_passes = png_set_interlace_handling(png.png_ptr);

        switch(png.png_type)
        {
            case PNG_COLOR_TYPE_GRAY:
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                png.png_depth = 8;
                png.png_format = PBPAFMT_GREY8;
                break;

            case PNG_COLOR_TYPE_PALETTE:
                png.png_depth = 8;
                png.png_format = PBPAFMT_LUT8;
                break;

            case PNG_COLOR_TYPE_RGB:
                png.png_depth = 24;
                png.png_format = PBPAFMT_RGB;
                break;

            case PNG_COLOR_TYPE_RGB_ALPHA:
                png.png_depth = 32;
                if (!makeARGB)
                    png.png_format = PBPAFMT_RGBA;
                else
                {
                    png.png_format = PBPAFMT_ARGB;
                    png_set_swap_alpha(png.png_ptr);
                }

                break;

            default:
                png_error(png.png_ptr, "Unknown PNG Color Type!");
                break;
        }

        png_read_update_info(png.png_ptr, png.png_info_ptr);
        
        {
            ULONG buffersize, modulo, y, bpr, bpp;
            UBYTE *buf;

            buffersize = png_get_rowbytes(png.png_ptr, png.png_info_ptr);
            if (png.png_lace == PNG_INTERLACE_NONE)
            {
                modulo = 0;
            }
            else
            {
                modulo = buffersize; buffersize *= png.png_height;
            }

            buffer = AllocVec(buffersize, MEMF_ANY);
            if (!buffer) png_error(png.png_ptr, "Out of memory!");

            bpp = png.png_depth / 8;
            bpr = bpp * png.png_width;
            buffersize = bpr * png.png_height;
            
            pnghandle = AllocVec(sizeof(*pnghandle) + buffersize, MEMF_ANY);
            if (!pnghandle) png_error(png.png_ptr, "Out of memory!");
            
            pnghandle->data = ((UBYTE *)pnghandle) + sizeof(*pnghandle);
            
            while(png.png_num_lace_passes--)
            {
                for(y = 0, buf = buffer; y < png.png_height; y++, buf += modulo)
                {
                    png_read_row(png.png_ptr, buf, NULL);

                    if (png.png_num_lace_passes == 0)
                    {
                        memcpy(pnghandle->data + y * bpr, buf, bpr);
                    }

                } /* for(y = 0, buf = buffer; y < png.png_height; y++, buf += modulo) */

            } /* while(png.png_num_lace_passes--) */

            png_read_end(png.png_ptr, png.png_end_info_ptr);

            if (chunkstore && chunkstoread)
            {
                png_unknown_chunkp entries;
                png_infop info;
                
                int infoloop;
                
                info = png.png_info_ptr;
                
                for(infoloop = 0; infoloop < 2; infoloop++)
                {
                    int numchunks = png_get_unknown_chunks(png.png_ptr, info, &entries);

                    while(numchunks--)
                    {
                        int i;
                        
                        for(i = 0; chunkstoread[i]; i++)
                        {
                            if (chunkstore[i]) continue;
                            
                            if (memcmp(entries->name, chunkstoread[i], 4) == 0)
                            {
                                png_unknown_chunkp p;
                                
                                chunkstore[i] = AllocVec(sizeof(*p) + entries->size, MEMF_ANY);
                                if (!chunkstore[i]) png_error(png.png_ptr, "Out of memory!");
                                
                                p = (png_unknown_chunkp)chunkstore[i];
                                
                                *p = *entries;
                                p->data = (png_byte *)(p + 1);
                                memcpy(p->data, entries->data, entries->size);
                                break;
                            }

                        } /* for(i = 0; chunkstoread[i]; i++) */

                        entries++;
                        
                    } /* while(numchunks--) */
                    
                    info = png.png_end_info_ptr;
                    
                } /* for(infoloop = 0, infoloop < 2; infoloop++) */
                
            } /* if (chunkstore && chunkstoread) */

            pnghandle->pal = 0;
            pnghandle->stuff = png;
                    
            /* We're done :-) */
            
            retval = pnghandle;
            
        } /**/
        
    } /* if (ok) */
    
    if (png.png_ptr)
    {
        FreeVec(buffer);
        png_destroy_read_struct(&png.png_ptr, &png.png_info_ptr, &png.png_end_info_ptr);
    }
        
    if (!ok)
    {
        FreeVec(pnghandle);
    }
            
    return retval;

}

/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH4(APTR, PNG_LoadImageFH,

/*  SYNOPSIS */
            AROS_LHA(BPTR, fh, A0),
            AROS_LHA(CONST_STRPTR const *, chunkstoread, A1),
            AROS_LHA(APTR *,  chunkstore, A2),
            AROS_LHA(BOOL, makeARGB, D0),

/*  LOCATION */
            struct Library *, PNGBase, 7, PNG)
/*  FUNCTION
        Decode one PNG image from the current position of an open DOS file.
        Read the signature, image and trailing chunks through the PNG end
        marker. The operation is synchronous and does not close the file.

    INPUTS
        fh - Open, readable DOS BPTR positioned at the PNG signature. Zero
            returns NULL. The handle and its file position are borrowed;
            exclude concurrent reads, seeks and closure during this call.

        chunkstoread - NULL, or a NULL-terminated array of pointers to distinct
            four-character, NUL-terminated PNG chunk names. The names and
            array must remain readable until the call returns. Intended for
            application-specific ancillary chunks such as "icOn".
        chunkstore - NULL, or a writable APTR array with one element per
            requested name. Initialize every element to NULL for a new load.
            Existing non-NULL elements are left untouched and skipped.
        makeARGB - TRUE to request four-byte A,R,G,B pixels; FALSE to retain
            grayscale, palette-index, RGB or RGBA layout as described below.

    RESULT
        An opaque image handle on success, owned by the caller and released
        with PNG_FreeImage(). NULL on a normally completed failure path, such
        as an invalid signature, read failure, decoder error or allocation
        failure. The error-recovery limitations below still apply. No detailed
        decoder error code is returned; IoErr() is not a complete error channel.

    NOTES
        Include <proto/pngdt.h> and open datatypes/png.datatype into PNGBase.
        Use DOS process context. Reads and allocation can block, with no
        timeout or cancellation interface. The stream must start at the PNG
        signature; do not consume it first with another reader.

        File position advances on both success and failure and is not restored.
        Keep the datatype open while using its handles and release those handles
        before closing it. No source bytes are retained after decoding.

        Output rows are tightly packed, in image order, with eight-bit samples.
        Sixteen-bit samples are stripped to eight bits. Grayscale samples below
        eight bits are expanded to eight-bit intensity; palette indices below
        eight bits are unpacked to one byte per pixel.

        With makeARGB FALSE, grayscale uses one byte, palette images one index
        byte, RGB three bytes and RGBA four bytes per pixel. No palette is
        returned. With makeARGB TRUE, palette indices and grayscale are expanded
        to RGB, and pixels occupy four bytes in A,R,G,B order. These are byte
        layouts, not host-endian ULONG color values. Alpha is not premultiplied.

        Grayscale alpha is discarded in both modes. Palette tRNS transparency
        is expanded only with makeARGB TRUE. Grayscale and RGB tRNS transparency
        are not expanded. Opaque filler bytes are 255. The loader requests gamma
        processing; callers cannot select a gamma or color-management policy.


        When both chunk arrays are supplied, the loader copies matching retained
        chunks before returning. It visits chunks before IDAT first, then chunks
        after IDAT, and fills only empty output slots. An absent requested chunk
        leaves its slot unchanged. Retained chunks are separate allocations:
        release each newly returned non-NULL handle with PNG_FreeChunk(), even
        if the image result is NULL. PNG_FreeImage() does not release them.

        The chunk arrays themselves are borrowed, not retained. Requesting a
        chunk normally interpreted by libpng can change how that chunk is
        decoded. Do not use the retention list as an unrestricted metadata API.

    BUGS
        Image-size products and allocation-size additions are not checked for
        overflow. Paletted output has no accompanying palette, grayscale alpha
        is lost, and RGB/grayscale tRNS is not preserved. Gamma arguments are
        passed in the reverse order from libpng's declared interface.

        Error recovery reads automatic allocation pointers modified after
        setjmp() without volatile qualification. Cleanup after a decoder error
        is therefore not reliable. Partial chunk results are not rolled back.
        A NULL result is not a guarantee of safe or complete error recovery.

    SEE ALSO
        PNG_CheckSig(), PNG_LoadImage(), PNG_LoadImageMEM(), PNG_GetImageInfo(), PNG_GetImageData(), PNG_FreeImage(), PNG_FreeChunk()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
        
    return PNG_LoadImageInternal(BADDR(fh), chunkstoread, chunkstore, makeARGB, HANDLETYPE_FILE);
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH4(APTR, PNG_LoadImage,

/*  SYNOPSIS */
            AROS_LHA(CONST_STRPTR, name, A0),
            AROS_LHA(CONST_STRPTR const *, chunkstoread, A1),
            AROS_LHA(APTR *,  chunkstore, A2),
            AROS_LHA(BOOL, makeARGB, D0),

/*  LOCATION */
            struct Library *, PNGBase, 8, PNG)
/*  FUNCTION
        Open a named file for reading, decode its PNG image, and close the file
        before returning. The decoding and chunk-retention rules are those of
        PNG_LoadImageFH().

    INPUTS
        name - Non-NULL, NUL-terminated DOS filename, valid through this call.
            Relative names use the calling process's current directory.

        chunkstoread - NULL, or a NULL-terminated array of pointers to distinct
            four-character, NUL-terminated PNG chunk names. The names and
            array must remain readable until the call returns. Intended for
            application-specific ancillary chunks such as "icOn".
        chunkstore - NULL, or a writable APTR array with one element per
            requested name. Initialize every element to NULL for a new load.
            Existing non-NULL elements are left untouched and skipped.
        makeARGB - TRUE to request four-byte A,R,G,B pixels; FALSE to retain
            grayscale, palette-index, RGB or RGBA layout as described below.

    RESULT
        An opaque image handle on success, owned by the caller and released
        with PNG_FreeImage(). NULL on a normally completed failure path, such
        as an invalid signature, read failure, decoder error or allocation
        failure. The error-recovery limitations below still apply. No detailed
        decoder error code is returned; IoErr() is not a complete error channel.

        Failure to open the named file also returns NULL without decoding.

    NOTES
        Include <proto/pngdt.h> and open datatypes/png.datatype into PNGBase.
        Call from DOS process context. File I/O, decoding and allocation are
        synchronous and have no timeout or cancellation interface. The image
        handle owns a copy of the decoded pixels, not the source file.

        Release the image with PNG_FreeImage() and every separately returned
        chunk with PNG_FreeChunk() before closing the datatype. Inspect and
        release partial chunk results even when this call returns NULL.

        See PNG_LoadImageFH() for the exact pixel layouts, transparency losses,
        gamma behavior, chunk selection and output-slot rules.

    BUGS
        Image-size products and allocation-size additions are not checked for
        overflow. Paletted output has no accompanying palette, grayscale alpha
        is lost, and RGB/grayscale tRNS is not preserved. Gamma arguments are
        passed in the reverse order from libpng's declared interface.

        Error recovery reads automatic allocation pointers modified after
        setjmp() without volatile qualification. Cleanup after a decoder error
        is therefore not reliable. Partial chunk results are not rolled back.
        A NULL result is not a guarantee of safe or complete error recovery.

    EXAMPLE
        // PNGBase is open. The source must meet the loader's size constraints.
        CONST_STRPTR wanted[] = { "icOn", NULL };
        APTR chunks[1] = { NULL };
        APTR image = PNG_LoadImage("RAM:sample.png", wanted, chunks, TRUE);

        if (image != NULL) {
            // Use the image; borrowed pixel data expires at PNG_FreeImage().
            PNG_FreeImage(image);
        }
        // Chunk ownership is independent of image success and lifetime.
        PNG_FreeChunk(chunks[0]);

    SEE ALSO
        PNG_LoadImageFH(), PNG_LoadImageMEM(), PNG_GetImageInfo(), PNG_GetImageData(), PNG_FreeImage(), PNG_FreeChunk()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    BPTR fh;
    APTR retval = 0;
    
    if ((fh = Open(name, MODE_OLDFILE)))
    {
        retval = PNG_LoadImageInternal(BADDR(fh), chunkstoread, chunkstore, makeARGB, HANDLETYPE_FILE);
        Close(fh);
    }
   
    return retval;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH5(APTR, PNG_LoadImageMEM,

/*  SYNOPSIS */
            AROS_LHA(APTR, mem, A0),
            AROS_LHA(ULONG, memsize, D0),
            AROS_LHA(CONST_STRPTR const *, chunkstoread, A1),
            AROS_LHA(APTR *,  chunkstore, A2),
            AROS_LHA(BOOL, makeARGB, D1),

/*  LOCATION */
            struct Library *, PNGBase, 9, PNG)
/*  FUNCTION
        Decode a PNG image from a contiguous memory buffer beginning with its
        eight-byte signature. Copy the decoded pixels and requested retained
        chunks into separate allocations. The source buffer is not retained.

    INPUTS
        mem - Readable buffer containing the PNG stream. Must be non-NULL when
            memsize is at least eight; that pointer is not validated internally.
            Keep the bytes unchanged until the synchronous call returns.
        memsize - Actual readable buffer length in bytes, including the
            signature. Sizes below eight return NULL without reading mem.
            A value of (ULONG)-1 is not a supported unknown-length sentinel.

        chunkstoread - NULL, or a NULL-terminated array of pointers to distinct
            four-character, NUL-terminated PNG chunk names. The names and
            array must remain readable until the call returns. Intended for
            application-specific ancillary chunks such as "icOn".
        chunkstore - NULL, or a writable APTR array with one element per
            requested name. Initialize every element to NULL for a new load.
            Existing non-NULL elements are left untouched and skipped.
        makeARGB - TRUE to request four-byte A,R,G,B pixels; FALSE to retain
            grayscale, palette-index, RGB or RGBA layout as described below.

    RESULT
        An opaque image handle on success, owned by the caller and released
        with PNG_FreeImage(). NULL on a normally completed failure path, such
        as an invalid signature, read failure, decoder error or allocation
        failure. The error-recovery limitations below still apply. No detailed
        decoder error code is returned; IoErr() is not a complete error channel.

    NOTES
        Include <proto/pngdt.h> and open datatypes/png.datatype into PNGBase.
        Use task context where allocation and decoder work are permitted.
        This entry point performs no DOS file reads. It provides no timeout,
        progress callback or cancellation interface and reports no consumed
        byte count. Trailing bytes beyond the decoded PNG are not returned.

        The caller retains ownership of mem and may release it after this call.
        Image and chunk lifetimes are independent of that source. Release the
        image with PNG_FreeImage() and all returned chunks with PNG_FreeChunk(),
        including partial chunks returned alongside a NULL image result.

        See PNG_LoadImageFH() for pixel layouts, transparency, gamma and chunk
        selection. Keep the datatype open until its handles have been released.

    BUGS
        The read callback checks pos + length against memsize without checking
        the addition for overflow. On a 32-bit size_t target this sum can wrap.
        An overstated length also permits reads beyond the real allocation;
        the loader cannot infer its true size. NULL mem with memsize >= 8
        reaches memcpy() rather than a reported failure.


        Image-size products and allocation-size additions are not checked for
        overflow. Paletted output has no accompanying palette, grayscale alpha
        is lost, and RGB/grayscale tRNS is not preserved. Gamma arguments are
        passed in the reverse order from libpng's declared interface.

        Error recovery reads automatic allocation pointers modified after
        setjmp() without volatile qualification. Cleanup after a decoder error
        is therefore not reliable. Partial chunk results are not rolled back.
        A NULL result is not a guarantee of safe or complete error recovery.

    SEE ALSO
        PNG_LoadImageFH(), PNG_LoadImage(), PNG_GetImageInfo(), PNG_GetImageData(), PNG_FreeImage(), PNG_FreeChunk()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct MyMemHandle mh;
    APTR retval = 0;
    
    mh.address = mem;
    mh.size    = memsize;
    mh.pos     = 0;

    D(bug("PNG_LoadImageMEM: address 0x%x size %d\n", mem, memsize));
        
    retval = PNG_LoadImageInternal(&mh, chunkstoread, chunkstore, makeARGB, HANDLETYPE_MEM);
   
    return retval;
    
    AROS_LIBFUNC_EXIT
}
    
/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH5(void, PNG_GetImageInfo,

/*  SYNOPSIS */
            AROS_LHA(APTR, pnghandle, A0),
            AROS_LHA(LONG *, widthptr, A1),
            AROS_LHA(LONG *, heightptr, A2),
            AROS_LHA(LONG *, depthptr, A3),
            AROS_LHA(LONG *, typeptr, A4),

/*  LOCATION */
            struct Library *, PNGBase, 10, PNG)
/*  FUNCTION
        Copy the dimensions, decoded bits per pixel and normalized PNG color
        type from an image handle. Each output is optional.

    INPUTS
        pnghandle - Live handle returned by a PNG_LoadImage*() call, or NULL.
        widthptr - Writable LONG output for pixel width, or NULL.
        heightptr - Writable LONG output for pixel height, or NULL.
        depthptr - Writable LONG output for decoded bits per pixel: 8, 24 or 32;
            this is not the original PNG sample depth. May be NULL.
        typeptr - Writable LONG output for the normalized PNG color type:
            0 (gray), 2 (RGB), 3 (palette) or 6 (RGB with alpha). May be NULL.

    RESULT
        No return value. With a non-NULL handle, the requested outputs are
        written. With NULL pnghandle, every output is left unchanged.

    NOTES
        Include <proto/pngdt.h>; PNGBase must refer to the open png.datatype.
        This is a metadata copy with no allocation, I/O or ownership transfer.
        Do not free the image concurrently. Distinct output locations are
        required if all requested results must be retained.

        typeptr is a PNG color-type value, not a PBPAFMT_* value. Type 6 can
        describe RGBA or ARGB bytes; retain the makeARGB choice used when
        loading. TRUE makeARGB reports type 6 and depth 32 for every successful
        conversion. Grayscale alpha was already discarded by the loader.

    BUGS
        Non-NULL handles and output addresses are not validated. The stored
        pixel-format field is not exposed, and the original sample depth and
        original color type cannot be recovered through this function.

    SEE ALSO
        PNG_GetImageData(), PNG_LoadImageFH(), PNG_FreeImage()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PNGHandle *h = (struct PNGHandle *)pnghandle;
    
    if (!h) return;
    
    if (widthptr)  *widthptr  = h->stuff.png_width;
    if (heightptr) *heightptr = h->stuff.png_height;
    if (depthptr)  *depthptr  = h->stuff.png_depth;
    if (typeptr)   *typeptr   = h->stuff.png_type;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH3(void, PNG_GetImageData,

/*  SYNOPSIS */
            AROS_LHA(APTR, pnghandle, A0),
            AROS_LHA(APTR *, gfxdataptr, A1),
            AROS_LHA(APTR *, paldataptr, A2),

/*  LOCATION */
            struct Library *, PNGBase, 11, PNG)
/*  FUNCTION
        Return borrowed pointers to the decoded pixel buffer and palette
        field. No pixel or palette data is copied.

    INPUTS
        pnghandle - Live handle returned by a PNG_LoadImage*() call, or NULL.
        gfxdataptr - Writable APTR output for the first pixel byte, or NULL.
        paldataptr - Writable APTR output for the palette pointer, or NULL.

    RESULT
        No return value. For a valid handle, gfxdataptr receives the pixel
        pointer and paldataptr receives NULL. With NULL pnghandle, outputs
        remain unchanged. Do not use the same location for both outputs.

    NOTES
        Include <proto/pngdt.h>; PNGBase must refer to the open png.datatype.
        The pixel pointer remains valid only until PNG_FreeImage(pnghandle).
        Do not free it separately. The call allocates nothing and performs no
        I/O. Synchronize all consumers against image destruction or mutation.

        Rows are contiguous, top to bottom, with no padding. Row size is width
        multiplied by depth / 8; obtain those values with PNG_GetImageInfo().
        Recompute sizes with checked arithmetic in the caller. Byte layouts
        and conversion limits are described by PNG_LoadImageFH().

    BUGS
        The loader always stores NULL in its palette field. With makeARGB FALSE,
        paletted images therefore expose index bytes without their color table.
        Use makeARGB TRUE when a self-contained decoded color buffer is needed.
        The returned pointers do not establish that image-size arithmetic was
        checked during loading.

    SEE ALSO
        PNG_GetImageInfo(), PNG_LoadImageFH(), PNG_FreeImage()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PNGHandle *h = (struct PNGHandle *)pnghandle;
    
    if (!h) return;
    
    if (gfxdataptr) *gfxdataptr = h->data;
    if (paldataptr) *paldataptr = h->pal;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH1(void, PNG_FreeImage,

/*  SYNOPSIS */
            AROS_LHA(APTR, pnghandle, A0),

/*  LOCATION */
            struct Library *, PNGBase, 12, PNG)
/*  FUNCTION
        Release an image handle and its inline decoded pixel buffer in one
        FreeVec() call.

    INPUTS
        pnghandle - Owned live handle from a PNG_LoadImage*() call, or NULL.
            Pass the handle itself, not a pointer obtained from PNG_GetImageData().

    RESULT
        No return value. NULL has no effect. A non-NULL handle and all borrowed
        pixel pointers become invalid when freed.

    NOTES
        Include <proto/pngdt.h>; keep png.datatype open for this call. Release
        an image exactly once, after every consumer has finished. This call
        does not wait for consumers and does not clear caller variables.
        It performs no image decoding or file I/O.

        Chunks returned through a loader's chunkstore array are separately
        allocated and are not released here. Free each with PNG_FreeChunk().
        The caller's original file or memory buffer is not released.

    BUGS
        Invalid handles, double frees and concurrent use are not detected.

    SEE ALSO
        PNG_GetImageData(), PNG_FreeChunk(), PNG_LoadImageFH()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeVec(pnghandle);
    
    AROS_LIBFUNC_EXIT
}
     
/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH1(void, PNG_FreeChunk,

/*  SYNOPSIS */
            AROS_LHA(APTR, chunk, A0),

/*  LOCATION */
            struct Library *, PNGBase, 13, PNG)
/*  FUNCTION
        Release one retained chunk handle and its inline payload in one
        FreeVec() call.

    INPUTS
        chunk - Owned live chunk handle returned in a loader's chunkstore
            array, or NULL. Pass the handle, not its payload pointer.

    RESULT
        No return value. NULL has no effect. A non-NULL chunk handle and any
        payload pointer returned by PNG_GetChunkInfo() become invalid.

    NOTES
        Include <proto/pngdt.h>; keep png.datatype open for this call. Release
        each chunk exactly once, after every payload consumer has finished.
        No reference count or consumer wait is provided, and caller variables
        are not cleared. The call performs no decoding or file I/O.

        A retained chunk is independent of its image handle and source bytes.
        It can outlive PNG_FreeImage(). Loaders can leave partial chunks in
        chunkstore even when their image result is NULL; those chunks still
        require release. Existing non-NULL slots skipped by a loader retain
        their previous ownership.

    BUGS
        Invalid handles, double frees and concurrent use are not detected.

    SEE ALSO
        PNG_GetChunkInfo(), PNG_FreeImage(), PNG_LoadImageFH()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeVec(chunk);
    
    AROS_LIBFUNC_EXIT
}
 
/***************************************************************************************************/

/*****************************************************************************

    NAME */
        AROS_LH3(void, PNG_GetChunkInfo,

/*  SYNOPSIS */
            AROS_LHA(APTR, chunk, A0),
            AROS_LHA(APTR *, dataptr, A1),
            AROS_LHA(ULONG *, sizeptr, A2),

/*  LOCATION */
            struct Library *, PNGBase, 14, PNG)
/*  FUNCTION
        Return a retained chunk's borrowed payload pointer and payload length.
        The payload contains the chunk data bytes, without its PNG length,
        type or CRC fields.

    INPUTS
        chunk - Live non-NULL chunk handle returned in a loader's chunkstore
            array. It is not an image handle or a pointer into the source PNG.
        dataptr - Writable APTR output for the payload pointer, or NULL.
        sizeptr - Writable ULONG output for the payload length in bytes,
            or NULL. A zero-length payload is possible.

    RESULT
        No return value. Each non-NULL output is written from the chunk handle.
        No validation or independent success indicator is provided.

    NOTES
        Include <proto/pngdt.h>; PNGBase must refer to the open png.datatype.
        The call allocates nothing, copies no payload and performs no I/O.
        Keep the chunk alive until all borrowed-payload users have finished.
        Free the chunk handle with PNG_FreeChunk(), never the payload pointer.

        Interpret and bounds-check the payload according to the requested
        chunk type. No chunk-name or source-position output is provided; the
        loader's request-array position identifies the requested type.

    BUGS
        Unlike PNG_GetImageInfo(), this function has no NULL-handle check.
        NULL chunk with a requested output is dereferenced. A payload pointer
        and length do not validate the chunk's application-specific structure.

    SEE ALSO
        PNG_FreeChunk(), PNG_LoadImageFH(), PNG_LoadImageMEM()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    png_unknown_chunkp p = (png_unknown_chunkp)chunk;
    if (dataptr) *dataptr = p->data;
    if (sizeptr) *sizeptr = p->size;
    
    AROS_LIBFUNC_EXIT
}
 
/***************************************************************************************************/
