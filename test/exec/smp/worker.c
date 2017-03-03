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

#include "work.h"

#define MAXITERATIONS   100000

#define DWORK(x)

/*
 * define a complex number with
 * real and imaginary parts
 */
typedef struct {
	double r;
	double i;
} complexno_t;

struct WorkersWork
{
    ULONG         workMax;
    complexno_t workTrajectories[];  
};

ULONG calculateTrajectory(struct WorkersWork *workload, double r, double i)
{
    double realNo, imaginaryNo, realNo2, imaginaryNo2;
    ULONG trajectory;

    /* Calculate trajectory */
    realNo = 0;
    imaginaryNo = 0;

    for(trajectory = 0; trajectory < workload->workMax; trajectory++)
    {
        /* Check if it's out of circle with radius 2 */
        realNo2 = realNo * realNo;
        imaginaryNo2 = imaginaryNo * imaginaryNo;

        if (realNo2 + imaginaryNo2 > 2.0)
            return trajectory;

        /* Next */
        realNo = realNo2 - imaginaryNo2 + r;
        imaginaryNo = 2 * realNo * imaginaryNo + i;

        /* Store */
        workload->workTrajectories[trajectory].r = realNo;
        workload->workTrajectories[trajectory].i = imaginaryNo;
    }

    return 0;
}

void processWork(struct WorkersWork *workload, ULONG *workBuffer, UWORD workWidth, UWORD workHeight, UWORD workStart, UWORD workEnd)
{

    double xlo = -1.0349498063694267;
    double ylo = -0.36302123503184713;
    double xhi = -0.887179105732484;
    double yhi = -0.21779830509554143;
    ULONG trajectoryLength;
    UWORD current;
    UWORD x, y;

    DWORK(
        bug("[SMP-Test:Worker] %s: Buffer @ 0x%p\n", __func__, workBuffer);
        bug("[SMP-Test:Worker] %s:           %dx%d\n", __func__, workWidth, workHeight);
        bug("[SMP-Test:Worker] %s: start : %d, end %d\n", __func__, workStart, workEnd);
    )

    for (current = workStart; current < workEnd; current++)
    {
        ULONG val;

        /* Locate the point on the complex plane */
        x = (current % workWidth);
        y = (current / workWidth);

        /* Calculate the points trajectory ... */
        trajectoryLength = calculateTrajectory(workload, (double)(xlo + (xhi - xlo) * x / workWidth), (double)(ylo + (yhi - ylo) * y / workHeight));

#if (0)
        /* Update the display if it escapes */
        if (trajectoryLength > 0)
        {
            UQUAD pos;
            int i;

            for(i = 0; i < trajectoryLength; i++)
            {
                pos = (workload->workTrajectories[i].i  * workWidth) + workload->workTrajectories[i].r;
                if (pos > 0 && pos < (workWidth * workHeight))
                {
                    val = (trajectoryLength / (workload->workMax/ 255));
                    workBuffer[pos] = workBuffer[pos] + ((val << 16) | (val << 8) | val);
                }
            }
        }
#else
        val = (trajectoryLength / (workload->workMax/ 255));
        workBuffer[current] = ((val << 16) | (val << 8) | val);
#endif
    }
}

/*
 * This Task processes work received
 */
void SMPTestWorker(struct ExecBase *SysBase)
{
    struct Task *thisTask = FindTask(NULL);
    D(
        int cpunum = KrnGetCPUNumber();

        bug("[SMP-Test:Worker.%03d] %s: %s task started\n", cpunum, __func__, thisTask->tc_Node.ln_Name);
    )
    struct SMPWorker *worker = thisTask->tc_UserData;
    struct SMPWorkMessage *workMsg;
    struct WorkersWork    *workPrivate;
    BOOL doWork = TRUE;

    D(bug("[SMP-Test:Worker.%03d] %s: worker data @ 0x%p\n", cpunum, __func__, worker);)
    
    if ((worker) && (worker->smpw_MasterPort))
    {
        worker->smpw_Node.ln_Type = 0;

        D(bug("[SMP-Test:Worker.%03d] %s: work Master MsgPort @ 0x%p\n", cpunum, __func__, worker->smpw_MasterPort);)
        worker->smpw_MsgPort = CreateMsgPort();
        D(bug("[SMP-Test:Worker.%03d] %s: work MsgPort @ 0x%p\n", cpunum, __func__, worker->smpw_MsgPort);)

        workPrivate = AllocMem(sizeof(struct WorkersWork) + (MAXITERATIONS * sizeof(complexno_t)), MEMF_CLEAR|MEMF_ANY);
        
        D(bug("[SMP-Test:Worker.%03d] %s: worker private data @ 0x%p\n", cpunum, __func__, workPrivate);)

        if (workPrivate)
        {
            workPrivate->workMax = MAXITERATIONS;

            while (doWork)
            {
                UWORD workWidth, workHeight, workStart, workEnd;
                ULONG *workBuffer;
                IPTR workType;

                /* we are ready for work .. */
                worker->smpw_Node.ln_Type = 1;
                Signal(worker->smpw_SyncTask, SIGBREAKF_CTRL_C);
                WaitPort(worker->smpw_MsgPort);

                while((workMsg = (struct SMPWorkMessage *) GetMsg(worker->smpw_MsgPort)))
                {
                    D(bug("[SMP-Test:Worker.%03d] %s: work received (Msg @ 0x%p)\n", cpunum, __func__, workMsg);)

                    /* cache the requested work and free the message ... */
                    workType = workMsg->smpwm_Type;
                    workBuffer = workMsg->smpwm_Buffer;
                    workWidth = workMsg->smpwm_Width;
                    workHeight = workMsg->smpwm_Height;
                    workStart = workMsg->smpwm_Start;
                    workEnd = workMsg->smpwm_End;

                    FreeMem(workMsg, sizeof(struct SMPWorkMessage));
                    
                    /* now process it .. */
                    if (workType == SPMWORKTYPE_FINISHED)
                    {
                        D(bug("[SMP-Test:Worker.%03d] %s: Finished! exiting ...\n", cpunum, __func__);)

                        doWork = FALSE;
                        break;
                    }
                    else if (workType == SPMWORKTYPE_PROCESS)
                    {
                        /*
                         * Lets do some work!
                         */
                        D(bug("[SMP-Test:Worker.%03d] %s: Processing requested work!\n", cpunum, __func__);)

                        processWork(workPrivate, workBuffer, workWidth, workHeight, workStart, workEnd);

                        /* let our "parent" know we are done .. */
                        Signal(worker->smpw_MasterPort->mp_SigTask, SIGBREAKF_CTRL_D);
                    }
                }
            }
            FreeMem(workPrivate, sizeof(struct WorkersWork) + (workPrivate->workMax * sizeof(complexno_t)));
        }
        Remove(&worker->smpw_Node);
        DeleteMsgPort(worker->smpw_MsgPort);
        Signal(worker->smpw_MasterPort->mp_SigTask, SIGBREAKF_CTRL_C);
    }
}
