/**
 * Copyright (c) 2014 Raspberry Pi (Trading) Ltd. All rights reserved.
 * Copyright (c) 2010-2012 Broadcom. All rights reserved.
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The ARM side state of FreeBSD's vchiq_arm.c without the character
 * device and without suspend: the VideoCore is always wanted here, so
 * use counts are only kept for the keepalive service, which answers the
 * VideoCore's remote use requests.
 */

#include <aros/debug.h>

#include "interface/vchiq_arm/vchiq_core.h"
#include "interface/vchiq_arm/vchiq_arm.h"

#define KEEPALIVE_VER 1
#define KEEPALIVE_VER_MIN KEEPALIVE_VER

int vchiq_arm_log_level = VCHIQ_LOG_DEFAULT;
int vchiq_susp_log_level = VCHIQ_LOG_ERROR;

VCHIQ_STATE_T g_state;

VCHIQ_STATE_T *vchiq_get_state(void)
{
    if (g_state.remote == NULL)
        printk(KERN_ERR "%s: g_state.remote == NULL\n", __func__);
    else if (g_state.remote->initialised != 1)
        printk(KERN_INFO "%s: g_state.remote->initialised != 1 (%d)\n",
            __func__, g_state.remote->initialised);

    return ((g_state.remote != NULL) &&
        (g_state.remote->initialised == 1)) ? &g_state : NULL;
}

void vchiq_dump(void *dump_context, const char *str, int len)
{
    (void)dump_context;
    bug("%.*s\n", len, str);
}

void vchiq_dump_platform_instances(void *dump_context)
{
    (void)dump_context;
}

void vchiq_dump_platform_service_state(void *dump_context,
    VCHIQ_SERVICE_T *service)
{
    char buf[80];
    int len;

    len = snprintf(buf, sizeof(buf), "  instance %p", service->instance);
    vchiq_dump(dump_context, buf, len + 1);
}

static VCHIQ_STATUS_T vchiq_keepalive_vchiq_callback(VCHIQ_REASON_T reason,
    VCHIQ_HEADER_T *header, VCHIQ_SERVICE_HANDLE_T service_user,
    void *bulk_user)
{
    vchiq_log_error(vchiq_susp_log_level,
        "%s callback reason %d", __func__, reason);
    return 0;
}

static int vchiq_keepalive_thread_func(void *v)
{
    VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)v;
    VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);

    VCHIQ_STATUS_T status;
    VCHIQ_INSTANCE_T instance;
    VCHIQ_SERVICE_HANDLE_T ka_handle;

    VCHIQ_SERVICE_PARAMS_T params = {
        .fourcc      = VCHIQ_MAKE_FOURCC('K', 'E', 'E', 'P'),
        .callback    = vchiq_keepalive_vchiq_callback,
        .version     = KEEPALIVE_VER,
        .version_min = KEEPALIVE_VER_MIN
    };

    status = vchiq_initialise(&instance);
    if (status != VCHIQ_SUCCESS) {
        vchiq_log_error(vchiq_susp_log_level,
            "%s vchiq_initialise failed %d", __func__, status);
        goto exit;
    }

    status = vchiq_connect(instance);
    if (status != VCHIQ_SUCCESS) {
        vchiq_log_error(vchiq_susp_log_level,
            "%s vchiq_connect failed %d", __func__, status);
        goto shutdown;
    }

    status = vchiq_add_service(instance, &params, &ka_handle);
    if (status != VCHIQ_SUCCESS) {
        vchiq_log_error(vchiq_susp_log_level,
            "%s vchiq_open_service failed %d", __func__, status);
        goto shutdown;
    }

    while (1) {
        long rc = 0, uc = 0;

        wait_for_completion_interruptible(&arm_state->ka_evt);

        /* read and clear counters.  Do release_count then use_count to
         * prevent getting more releases than uses */
        rc = atomic_xchg(&arm_state->ka_release_count, 0);
        uc = atomic_xchg(&arm_state->ka_use_count, 0);

        /* Call use/release service the requisite number of times.
         * Process use before release so use counts don't go negative */
        while (uc--) {
            atomic_inc(&arm_state->ka_use_ack_count);
            status = vchiq_use_service(ka_handle);
            if (status != VCHIQ_SUCCESS) {
                vchiq_log_error(vchiq_susp_log_level,
                    "%s vchiq_use_service error %d",
                    __func__, status);
            }
        }
        while (rc--) {
            status = vchiq_release_service(ka_handle);
            if (status != VCHIQ_SUCCESS) {
                vchiq_log_error(vchiq_susp_log_level,
                    "%s vchiq_release_service error %d",
                    __func__, status);
            }
        }
    }

shutdown:
    vchiq_shutdown(instance);
exit:
    return 0;
}

VCHIQ_STATUS_T vchiq_arm_init_state(VCHIQ_STATE_T *state,
    VCHIQ_ARM_STATE_T *arm_state)
{
    if (arm_state) {
        rwlock_init(&arm_state->susp_res_lock);

        init_completion(&arm_state->ka_evt);
        atomic_set(&arm_state->ka_use_count, 0);
        atomic_set(&arm_state->ka_use_ack_count, 0);
        atomic_set(&arm_state->ka_release_count, 0);

        arm_state->vc_suspend_state = VC_SUSPEND_IDLE;
        arm_state->vc_resume_state = VC_RESUME_RESUMED;
        arm_state->first_connect = 0;
    }
    return VCHIQ_SUCCESS;
}

void vchiq_platform_check_suspend(VCHIQ_STATE_T *state)
{
    (void)state;
}

VCHIQ_STATUS_T vchiq_use_internal(VCHIQ_STATE_T *state,
    VCHIQ_SERVICE_T *service, enum USE_TYPE_E use_type)
{
    VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);
    VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
    int *entity_uc;
    long ack_cnt;

    if (use_type == USE_TYPE_VCHIQ)
        entity_uc = &arm_state->peer_use_count;
    else if (service)
        entity_uc = &service->service_use_count;
    else
        return VCHIQ_ERROR;

    write_lock_bh(&arm_state->susp_res_lock);
    ++arm_state->videocore_use_count;
    ++(*entity_uc);
    write_unlock_bh(&arm_state->susp_res_lock);

    /* Acknowledge the remote uses the keepalive thread took on */
    ack_cnt = atomic_xchg(&arm_state->ka_use_ack_count, 0);
    while (ack_cnt && (status == VCHIQ_SUCCESS)) {
        /* Send the use notify to videocore */
        status = vchiq_send_remote_use_active(state);
        if (status == VCHIQ_SUCCESS)
            ack_cnt--;
        else
            atomic_add(ack_cnt, &arm_state->ka_use_ack_count);
    }

    return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_release_internal(VCHIQ_STATE_T *state,
    VCHIQ_SERVICE_T *service)
{
    VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);
    VCHIQ_STATUS_T ret = VCHIQ_SUCCESS;
    int *entity_uc;

    entity_uc = service ? &service->service_use_count
                        : &arm_state->peer_use_count;

    write_lock_bh(&arm_state->susp_res_lock);
    if (!arm_state->videocore_use_count || !(*entity_uc)) {
        /* Don't use BUG_ON - don't allow user thread to crash kernel */
        WARN_ON(!arm_state->videocore_use_count);
        WARN_ON(!(*entity_uc));
        ret = VCHIQ_ERROR;
    } else {
        --arm_state->videocore_use_count;
        --(*entity_uc);
    }
    write_unlock_bh(&arm_state->susp_res_lock);

    return ret;
}

void vchiq_on_remote_use(VCHIQ_STATE_T *state)
{
    VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);

    atomic_inc(&arm_state->ka_use_count);
    complete(&arm_state->ka_evt);
}

void vchiq_on_remote_release(VCHIQ_STATE_T *state)
{
    VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);

    atomic_inc(&arm_state->ka_release_count);
    complete(&arm_state->ka_evt);
}

void vchiq_on_remote_use_active(VCHIQ_STATE_T *state)
{
    (void)state;
}

VCHIQ_STATUS_T vchiq_use_service_internal(VCHIQ_SERVICE_T *service)
{
    return vchiq_use_internal(service->state, service, USE_TYPE_SERVICE);
}

VCHIQ_STATUS_T vchiq_release_service_internal(VCHIQ_SERVICE_T *service)
{
    return vchiq_release_internal(service->state, service);
}

VCHIQ_STATUS_T vchiq_use_service(VCHIQ_SERVICE_HANDLE_T handle)
{
    VCHIQ_STATUS_T ret = VCHIQ_ERROR;
    VCHIQ_SERVICE_T *service = find_service_by_handle(handle);

    if (service) {
        ret = vchiq_use_internal(service->state, service,
                USE_TYPE_SERVICE);
        unlock_service(service);
    }
    return ret;
}

VCHIQ_STATUS_T vchiq_release_service(VCHIQ_SERVICE_HANDLE_T handle)
{
    VCHIQ_STATUS_T ret = VCHIQ_ERROR;
    VCHIQ_SERVICE_T *service = find_service_by_handle(handle);

    if (service) {
        ret = vchiq_release_internal(service->state, service);
        unlock_service(service);
    }
    return ret;
}

VCHIQ_STATUS_T vchiq_check_service(VCHIQ_SERVICE_T *service)
{
    if (!service || !service->state)
        return VCHIQ_ERROR;

    if (!service->service_use_count) {
        vchiq_log_error(vchiq_susp_log_level,
            "%s ERROR - %c%c%c%c:%8x service count %d", __func__,
            VCHIQ_FOURCC_AS_4CHARS(service->base.fourcc),
            service->client_id, service->service_use_count);
        return VCHIQ_ERROR;
    }

    return VCHIQ_SUCCESS;
}

void vchiq_platform_conn_state_changed(VCHIQ_STATE_T *state,
    VCHIQ_CONNSTATE_T oldstate, VCHIQ_CONNSTATE_T newstate)
{
    VCHIQ_ARM_STATE_T *arm_state = vchiq_platform_get_arm_state(state);

    if (state->conn_state == VCHIQ_CONNSTATE_CONNECTED) {
        write_lock_bh(&arm_state->susp_res_lock);
        if (!arm_state->first_connect) {
            arm_state->first_connect = 1;
            write_unlock_bh(&arm_state->susp_res_lock);
            arm_state->ka_thread = vchiq_thread_create(
                &vchiq_keepalive_thread_func,
                (void *)state,
                "VCHIQka-%d", state->id);
            if (arm_state->ka_thread == NULL) {
                vchiq_log_error(vchiq_susp_log_level,
                    "vchiq: FATAL: couldn't create keepalive thread");
            }
        } else
            write_unlock_bh(&arm_state->susp_res_lock);
    }
}

int vchiq_init(void)
{
    int err;

    err = vchiq_platform_init(&g_state);
    if (err != 0) {
        vchiq_log_warning(vchiq_arm_log_level, "could not load vchiq");
        return err;
    }

    vchiq_log_info(vchiq_arm_log_level,
        "vchiq: initialised - version %d (min %d)",
        VCHIQ_VERSION, VCHIQ_VERSION_MIN);

    return 0;
}
