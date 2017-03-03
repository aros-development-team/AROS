/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <exec/tasks.h>
#include <exec/ports.h>

struct SMPMaster
{
    IPTR                        smpm_WorkerCount;
    struct List                 smpm_Workers;
    struct Task                 *smpm_Master;
    struct MsgPort              *smpm_MasterPort;
    ULONG                       *smpm_WorkBuffer;
    UWORD                       smpm_Width;
    UWORD                       smpm_Height;    
    spinlock_t                  smpm_Lock;
};

struct SMPWorker
{
    struct Node                 smpw_Node;
    struct Task                 *smpw_Task;
    struct MsgPort              *smpw_MasterPort;
    struct MsgPort              *smpw_MsgPort;
    struct Task                 *smpw_SyncTask;
    spinlock_t                  *smpw_Lock;
};

struct SMPWorkMessage
{
    struct Message              smpwm_Msg;
    IPTR                        smpwm_Type;
    ULONG                       *smpwm_Buffer;
    ULONG                       smpwm_Width;
    ULONG                       smpwm_Height;
    ULONG                       smpwm_Start;
    ULONG                       smpwm_End;
    spinlock_t                  *smpwm_Lock;
};

#define SPMWORKTYPE_FINISHED    (1 << 0)
#define SPMWORKTYPE_PROCESS     (1 << 1)

void SMPTestMaster(struct ExecBase *);
void SMPTestWorker(struct ExecBase *);
