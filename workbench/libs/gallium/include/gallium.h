/*
    Copyright (C) 2010-2019, The AROS Development Team. All rights reserved.
*/

#ifndef GALLIUM_GALLIUM_H
#define GALLIUM_GALLIUM_H

#define GALLIUM_INTERFACE_VERSION   5

/* Tags for CreatePipeScreen() function */
#define CPS_Dummy                       (TAG_USER)
#define CPS_GalliumInterfaceVersion     (CPS_Dummy + 1)
#define CPS_PipeFriendBitMap            (CPS_Dummy + 2)
#define CPS_PipeScreenDriver            (CPS_Dummy + 3)
/* mesa3dgl's GalliumCoreAPI table (struct GalliumCoreAPI *). Forwarded to
 * the pipe hidd as aHidd_Gallium_CoreAPI so a runtime-bound pipe driver
 * can call the one Mesa compiler core. 0 when the build carries no
 * table; drivers that need it refuse CreatePipeScreen. */
#define CPS_GalliumCoreAPI              (CPS_Dummy + 4)

/* A special version of CreatePipe function with version embeded in call */
#define CreatePipeV(tags)                                               \
    ({                                                                  \
        struct TagItem cpsvtags [] =                                    \
        {                                                               \
            { CPS_GalliumInterfaceVersion, GALLIUM_INTERFACE_VERSION }, \
            { TAG_MORE, (IPTR)tags }                                    \
        };                                                              \
        CreatePipe(cpsvtags);                                           \
    })

typedef APTR PipeHandle_t;
typedef APTR PipeScreen_t;

/*
 * Scan-out framebuffers (3D scan-out): a set of 2..3 colour buffers in
 * display memory that the screen's display can show through a hardware
 * plane (or that its compositor blits), each available to the pipe as a
 * struct pipe_resource. Created for a window; gallium.library tracks the
 * window's layer and keeps the surface's position and visible shape
 * current. Only available when the display answers SCF_PLANE for the
 * format and the pipe driver can import a scan-out bitmap buffer
 * (moHidd_Gallium_ResourceFromBitMap); otherwise CreateScanoutFrameBuffer()
 * returns NULL and the client keeps BltPipeResourceRastPort().
 *
 * Resize: there is no resize call; destroy the framebuffer and create a
 * new one for the new rectangle.
 */

/* Tags for CreateScanoutFrameBuffer() */
#define SFB_Dummy           (TAG_USER + 0x100)
/* The rectangle rendered to, relative to window->WLayer (RastPort
 * coordinates: for a GimmeZeroZero window it starts at (0,0), otherwise
 * at the window's border). All four are required. */
#define SFB_Left            (SFB_Dummy + 1)
#define SFB_Top             (SFB_Dummy + 2)
#define SFB_Width           (SFB_Dummy + 3)
#define SFB_Height          (SFB_Dummy + 4)
/* Number of colour buffers wanted, 2 or 3 (default 2); clamped to what the
 * display allows, creation fails below 2. */
#define SFB_Buffers         (SFB_Dummy + 5)
/* enum pipe_format of the colour buffers (required). Supported:
 * PIPE_FORMAT_B8G8R8A8_UNORM, PIPE_FORMAT_B8G8R8X8_UNORM,
 * PIPE_FORMAT_B5G6R5_UNORM. */
#define SFB_PipeFormat      (SFB_Dummy + 6)
/* PipeScreen_t (from CreatePipeScreen() on the same pipe) that the
 * resources belong to (required). */
#define SFB_PipeScreen      (SFB_Dummy + 7)
/* BOOL: SwapScanoutFrameBuffer() never waits for the vertical blank.
 * Default: wait with 2 buffers (the displaced buffer is rendered next),
 * do not wait with 3. */
#define SFB_NoWait          (SFB_Dummy + 8)

/* GetScanoutFrameBufferAttr() */
#define SFBA_Dummy          (TAG_USER + 0x180)
#define SFBA_Buffers        (SFBA_Dummy + 1)    /* number of colour buffers           */
#define SFBA_BackBuffer     (SFBA_Dummy + 2)    /* index of the buffer to render next */
#define SFBA_FrontBuffer    (SFBA_Dummy + 3)    /* index of the buffer being shown    */
#define SFBA_Width          (SFBA_Dummy + 4)
#define SFBA_Height         (SFBA_Dummy + 5)
#define SFBA_OnPlane        (SFBA_Dummy + 6)    /* TRUE while a hardware plane scans the surface out (else blitted) */
#define SFBA_Attached       (SFBA_Dummy + 7)    /* FALSE once the window closed under it */
#define SFBA_Window         (SFBA_Dummy + 8)    /* struct Window *, NULL when detached */
#define SFBA_Swaps          (SFBA_Dummy + 9)    /* SwapScanoutFrameBuffer() calls so far */
#define SFBA_PipeFormat     (SFBA_Dummy + 10)   /* enum pipe_format of the buffers    */

#endif
