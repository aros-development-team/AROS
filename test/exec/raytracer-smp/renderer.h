#ifndef _RENDERER_H
#define _RENDERER_H

#include <exec/ports.h>
#include <exec/tasks.h>

#define TILE_SIZE   32

struct tileWork {
    struct MinNode  node;
    ULONG           x;
    ULONG           y;
};

struct MyMessage {
    struct Message      mm_Message;
    ULONG               mm_Type;
    union {
        struct {
            ULONG *             ChunkyBM;
            ULONG               Width;
            ULONG               Height;
            ULONG               coreCount;
            ULONG               numberOfSamples;
            BYTE                explicitMode;
        } Startup;

        struct {
            ULONG               TileX;
            ULONG               TileY;
        } RedrawTile;

        struct {
            struct tileWork *   tile;
            struct MsgPort *    guiPort;
            ULONG *             buffer;
            ULONG               width;
            ULONG               height;
            ULONG               numberOfSamples;
            BYTE                explicitMode;
        } RenderTile;

        struct {
            ULONG   tasksIn;
            ULONG   tasksOut;
            ULONG   tasksWork;
        } Stats;
    } mm_Body;
};

extern int maximal_ray_depth;

#define MSG_STARTUP     0
#define MSG_DIE         1
#define MSG_REDRAWTILE  2
#define MSG_RENDERTILE  3
#define MSG_HUNGRY      4
#define MSG_RENDERREADY 5
#define MSG_STATS       6

void Renderer(struct ExecBase *SysBase, struct MsgPort *ParentMailbox);

#endif /* _RENDERER_H */
