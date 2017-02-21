/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/tasks.h>
#include <exec/ports.h>

struct SMPMaster
{
    struct List smpm_Workers;
    struct Task *smpm_Master;
    struct MsgPort *smpm_MasterPort;
};

struct SMPWorker
{
    struct Node smpw_Node;
    struct Task *smpw_Task;
    struct MsgPort *smpw_MasterPort;
    struct MsgPort *smpw_MsgPort;
};

struct SMPWorkMessage
{
    struct Message smpwm_Msg;
    IPTR           smpwm_Type;
};

#define SPMWORKTYPE_FINISHED    (1 << 0)
#define SPMWORKTYPE_PROCESS     (1 << 1)

void SMPTestMaster(struct ExecBase *);
void SMPTestWorker(struct ExecBase *);
