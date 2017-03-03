/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/tasks.h>
#include <exec/ports.h>

struct SMPMaster
{
    IPTR                        smpm_WorkerCount;
    struct List                 smpm_Workers;
    struct Task                 *smpm_Master;
    struct MsgPort              *smpm_MasterPort;
    UBYTE                       *smpm_WorkBuffer;
    UWORD                       smpm_Width;
    UWORD                       smpm_Height;    
};

struct SMPWorker
{
    struct Node                 smpw_Node;
    struct Task                 *smpw_Task;
    struct MsgPort              *smpw_MasterPort;
    struct MsgPort              *smpw_MsgPort;
    struct Task                 *smpw_SyncTask;
};

struct SMPWorkMessage
{
    struct Message              smpwm_Msg;
    IPTR                        smpwm_Type;
    UBYTE                       *smpwm_Buffer;
    UWORD                       smpwm_Width;
    UWORD                       smpwm_Height;
    UWORD                       smpwm_Start;
    UWORD                       smpwm_End;
};

#define SPMWORKTYPE_FINISHED    (1 << 0)
#define SPMWORKTYPE_PROCESS     (1 << 1)

void SMPTestMaster(struct ExecBase *);
void SMPTestWorker(struct ExecBase *);
