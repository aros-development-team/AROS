/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _GPU_SCHED_TRACE_H_
#define _GPU_SCHED_TRACE_H_

/* no tracepoints here */
#define trace_drm_sched_job_queue(job, entity)              do { } while (0)
#define trace_drm_sched_job_run(job, entity)                do { } while (0)
#define trace_drm_sched_job_done(fence)                     do { } while (0)
#define trace_drm_sched_job_add_dep(job, fence)             do { } while (0)
#define trace_drm_sched_job_add_dep_enabled()               (false)
#define trace_drm_sched_job_unschedulable(job, fence)       do { } while (0)
#define trace_drm_sched_job_unschedulable_enabled()         (false)

#endif /* _GPU_SCHED_TRACE_H_ */
