/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>
#include <aros/atomic.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/rawfmt.h>

#include "renderer.h"

extern void render_tile(int w, int h, int samps, int tile_x, int tile_y, ULONG *buffer, struct Task *gfx);
extern void RenderTile(struct ExecBase *SysBase, struct MsgPort *masterPort);

struct Worker {
    struct Task *   task;
    char *          name;
};

void Renderer(struct ExecBase *ExecBase, struct MsgPort *ParentMailbox)
{
    ULONG width = 0;
    ULONG height = 0;
    ULONG *bitmap = NULL;
    struct MsgPort *guiPort = NULL;
    struct MsgPort *port;
    struct MyMessage *messages;
    ULONG numberOfCores = 0;

    D(bug("[SMP-Smallpt-Renderer] Renderer started, ParentMailBox = %p\n", ParentMailbox));

    port = CreateMsgPort();
    
    if (port)
    {
        struct Message startup;
        int stayAlive = TRUE;
        struct MinList workList;
        struct MinList doneList;
        struct MinList msgPool;
        struct tileWork *workPackages;
        struct Worker *workers;

        NEWLIST(&workList);
        NEWLIST(&doneList);
        NEWLIST(&msgPool);

        /* Prepare initial message and wait for startup msg */
        startup.mn_Length = sizeof(startup);
        startup.mn_ReplyPort = port;
 
        D(bug("[SMP-Smallpt-Renderer] Sending welcome msg to parent\n"));
        PutMsg(ParentMailbox, &startup);

        while(width == 0)
        {
            struct MyMessage *msg;
            
            WaitPort(port);

            while ((msg = (struct MyMessage *)GetMsg(port)))
            {
                if (msg->mm_Message.mn_Node.ln_Type == NT_REPLYMSG)
                {
                    D(bug("[SMP-Smallpt-Renderer] Parent replied to welcome msg\n"));
                }
                else if (msg->mm_Message.mn_Length >= sizeof(struct Message))
                {
                    if (msg->mm_Type == MSG_STARTUP)
                    {
                        D(bug("[SMP-Smallpt-Renderer] recieved startup message at %p\n", msg));

                        guiPort = msg->mm_Message.mn_ReplyPort;
                        width = msg->mm_Body.Startup.Width;
                        height = msg->mm_Body.Startup.Height;
                        bitmap = msg->mm_Body.Startup.ChunkyBM;
                        //mainTask = msg->mm_Message.mn_ReplyPort->mp_SigTask;
                        numberOfCores = msg->mm_Body.Startup.coreCount;
                    }
                    ReplyMsg(&msg->mm_Message);
                }
            }
        }

        D(bug("[SMP-Smallpt-Renderer] Bitmap size %dx%d, chunky buffer at %p\n", width, height, bitmap));

        ULONG tile_count = (width / TILE_SIZE) * (height / TILE_SIZE);

        workPackages = AllocMem(tile_count * sizeof(struct tileWork), MEMF_ANY);

        D(bug("[SMP-Smallpt-Renderer] Preparing work packages\n"));

        for (ULONG i=0; i < tile_count; i++)
        {
            workPackages[i].x = i % (width / TILE_SIZE);
            workPackages[i].y = i / (width / TILE_SIZE);
            ADDHEAD(&workList, &workPackages[i].node);
        }

        messages = AllocMem(sizeof(struct MyMessage) * numberOfCores * 5, MEMF_PUBLIC | MEMF_CLEAR);
        for (int i=0; i < numberOfCores * 5; i++)
            ADDHEAD(&msgPool, &messages[i].mm_Message);

        D(bug("[SMP-Smallpt-Renderer] creating %d workers\n", numberOfCores));
        workers = AllocMem(sizeof(struct Worker) * numberOfCores, MEMF_PUBLIC | MEMF_CLEAR);

        for (ULONG i=0; i < numberOfCores; i++)
        {
            void *coreAffinity = KrnAllocCPUMask();
            KrnGetCPUMask(i, coreAffinity);

            workers[i].name = AllocMem(30, MEMF_PUBLIC | MEMF_CLEAR);
            NewRawDoFmt("SMP-SmallPT Worker.#%03u", RAWFMTFUNC_STRING, workers[i].name, i);

            workers[i].task = NewCreateTask(TASKTAG_NAME,   workers[i].name,
                                    TASKTAG_AFFINITY,       coreAffinity,
                                    TASKTAG_PRI,            -1,
                                    TASKTAG_PC,             RenderTile,
                                    TASKTAG_ARG1,           SysBase,
                                    TASKTAG_ARG2,           port,
                                    TASKTAG_STACKSIZE,      10000000,
                                    TAG_DONE);
        }

        D(bug("[SMP-Smallpt-Renderer] all set up, doing work\n"));

        while(stayAlive)
        {
            struct MyMessage *msg;
            WaitPort(port);

            while ((msg = (struct MyMessage *)GetMsg(port)))
            {
                if (msg->mm_Message.mn_Node.ln_Type == NT_REPLYMSG)
                    ADDHEAD(&msgPool, msg);

                if (msg->mm_Type == MSG_DIE)
                {
                    D(bug("[SMP-Smallpt-Renderer] time to die...\n"));
                    stayAlive = FALSE;

                    for (ULONG i=0; i < numberOfCores; i++)
                    {
                        APTR ptr = workers[i].task->tc_Node.ln_Name;
                        D(bug("[SMP-Smallpt-Renderer] telling task %s to shut down\n", ptr));
                        Signal(workers[i].task, SIGBREAKF_CTRL_C);
                        Wait(SIGBREAKF_CTRL_C);
                        FreeMem(ptr, 30);
                    }

                    ReplyMsg(&msg->mm_Message);
                }
                else if (msg->mm_Type == MSG_HUNGRY)
                {
                    struct MsgPort *workerPort = msg->mm_Message.mn_ReplyPort;
                    ReplyMsg(&msg->mm_Message);

                    if (!IsMinListEmpty(&workList))
                    {
                        struct tileWork *work = (struct tileWork *)REMHEAD(&workList);
                        struct MyMessage *m = (struct MyMessage *)REMHEAD(&msgPool);

                        m->mm_Type = MSG_RENDERTILE;
                        m->mm_Message.mn_Length = sizeof(m);
                        m->mm_Message.mn_ReplyPort = port;
                        m->mm_Body.RenderTile.tile = work;
                        m->mm_Body.RenderTile.buffer = bitmap;
                        m->mm_Body.RenderTile.guiPort = guiPort;
                        m->mm_Body.RenderTile.width = width;
                        m->mm_Body.RenderTile.height = height;
                        m->mm_Body.RenderTile.numberOfSamples = 16;

                        PutMsg(workerPort, &m->mm_Message);
                    }
                }
                else if (msg->mm_Type == MSG_RENDERREADY)
                {
                    struct tileWork *work = msg->mm_Body.RenderTile.tile;
                    ReplyMsg(&msg->mm_Message);
                    ADDHEAD(&doneList, work);
                }
            }
        }

        FreeMem(workPackages, tile_count * sizeof(struct tileWork));
        FreeMem(messages, sizeof(struct MyMessage) * numberOfCores * 5);
        FreeMem(workers, sizeof(struct Worker) * numberOfCores);
        DeleteMsgPort(port);
    }
    D(bug("[SMP-Smallpt-Renderer] goodbye!\n"));
    Signal((struct Task *)guiPort->mp_SigTask, SIGBREAKF_CTRL_C);
}
