/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef UVC_JPEG_H
#define UVC_JPEG_H
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>
#ifdef __AROS__
#include <defines/jfif_LVO.h>
#include <aros/genmodule.h>
/* Stack-call libraries require the generated thunk, which supplies the
 * task's library base to its entry wrapper. Serialize this private stub base
 * across cameras; the library itself is opened in the capturing task. */
struct Library *UvcJpegBase;
static struct SignalSemaphore uvc_jpeg_lock;
AROS_GM_LIBFUNCSTUB(jpeg_std_error, UvcJpegBase, LVOjpeg_std_error)
AROS_GM_LIBFUNCSTUB(jpeg_CreateDecompress, UvcJpegBase, LVOjpeg_CreateDecompress)
AROS_GM_LIBFUNCSTUB(jpeg_mem_src, UvcJpegBase, LVOjpeg_mem_src)
AROS_GM_LIBFUNCSTUB(jpeg_read_header, UvcJpegBase, LVOjpeg_read_header)
AROS_GM_LIBFUNCSTUB(jpeg_start_decompress, UvcJpegBase, LVOjpeg_start_decompress)
AROS_GM_LIBFUNCSTUB(jpeg_read_scanlines, UvcJpegBase, LVOjpeg_read_scanlines)
AROS_GM_LIBFUNCSTUB(jpeg_finish_decompress, UvcJpegBase, LVOjpeg_finish_decompress)
AROS_GM_LIBFUNCSTUB(jpeg_destroy_decompress, UvcJpegBase, LVOjpeg_destroy_decompress)
#endif
#define UVC_JPEG_CALL(name) name
struct uvc_jpeg_error {
    struct jpeg_error_mgr pub;
    jmp_buf jump;
    int code;
};
struct uvc_jpeg_state {
    struct jpeg_decompress_struct image;
    struct uvc_jpeg_error error;
};
static void uvc_jpeg_error_exit(j_common_ptr image)
{
    struct uvc_jpeg_error *error = (struct uvc_jpeg_error *)image->err;
    error->code = image->err->msg_code;
    longjmp(error->jump, 1);
}
static void uvc_jpeg_warning(j_common_ptr image, int level)
{
    /* Do not turn a truncated/corrupt USB frame into an apparently valid one. */
    if (level < 0) uvc_jpeg_error_exit(image);
}
static int uvc_jpeg_rgb(const unsigned char *data, size_t length,
                        unsigned char *rgb, size_t capacity,
                        unsigned int width, unsigned int height, int *reason)
{
    struct uvc_jpeg_state *state;
    volatile int ok = 0;
#ifdef __AROS__
    struct Library *base;
#endif
    if (reason) *reason = -1;
    /* UVC cameras may align an MJPEG frame to four bytes with zero padding.
     * Strip at most the three alignment bytes; never scan for an arbitrary
     * earlier EOI that could conceal a damaged or truncated frame. */
    if (data) {
        unsigned int padding = 0;
        while (length > 4 && padding < 3 && data[length-1] == 0) {
            length--; padding++;
        }
    }
    if (!data || !rgb || !width || !height || width > 3840 || height > 2160 ||
        capacity < (size_t)width*height*3 || length < 4 ||
        data[0] != 0xff || data[1] != 0xd8 ||
        data[length-2] != 0xff || data[length-1] != 0xd9) return 0;
#ifdef __AROS__
    ObtainSemaphore(&uvc_jpeg_lock);
    base = OpenLibrary("jfif.library", 52);
    if (!base) { ReleaseSemaphore(&uvc_jpeg_lock); return 0; }
    UvcJpegBase = base;
    state = AllocVec(sizeof(*state), MEMF_PUBLIC | MEMF_CLEAR);
#else
    state = calloc(1, sizeof(*state));
#endif
    if (!state) goto done;
    state->image.err = UVC_JPEG_CALL(jpeg_std_error)(&state->error.pub);
    state->error.pub.error_exit = uvc_jpeg_error_exit;
    state->error.pub.emit_message = uvc_jpeg_warning;
    if (setjmp(state->error.jump)) goto destroy;
    UVC_JPEG_CALL(jpeg_CreateDecompress)(&state->image, JPEG_LIB_VERSION, sizeof(state->image));
    UVC_JPEG_CALL(jpeg_mem_src)(&state->image, data, length);
    if (UVC_JPEG_CALL(jpeg_read_header)(&state->image, TRUE) != JPEG_HEADER_OK ||
        state->image.image_width != width || state->image.image_height != height) goto destroy;
    state->image.out_color_space = JCS_RGB;
    if (!UVC_JPEG_CALL(jpeg_start_decompress)(&state->image) ||
        state->image.output_width != width || state->image.output_height != height ||
        state->image.output_components != 3) goto destroy;
    while (state->image.output_scanline < height) {
        JSAMPROW row = rgb + (size_t)state->image.output_scanline*width*3;
        if (UVC_JPEG_CALL(jpeg_read_scanlines)(&state->image, &row, 1) != 1) goto destroy;
    }
    ok = UVC_JPEG_CALL(jpeg_finish_decompress)(&state->image);
destroy:
    if (reason) *reason = ok ? 0 : (state->error.code ? state->error.code : -2);
    if (state->image.mem) UVC_JPEG_CALL(jpeg_destroy_decompress)(&state->image);
#ifdef __AROS__
    FreeVec(state);
#else
    free(state);
#endif
done:
#ifdef __AROS__
    CloseLibrary(base);
    UvcJpegBase = NULL;
    ReleaseSemaphore(&uvc_jpeg_lock);
#endif
    return ok;
}
#undef UVC_JPEG_CALL
#endif
