/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: IPMI Keyboard Controller Style (KCS) system interface,
          IPMI v2.0 specification section 9. Polled operation.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "ipmi_transport.h"

/* Registers (index, scaled by the register spacing) */
#define KCS_REG_DATA        0       /* Data_In (write) / Data_Out (read) */
#define KCS_REG_STATUS      1       /* Status (read) / Command (write) */

/* Status register */
#define KCS_STATUS_OBF      0x01    /* Output Buffer Full: data from BMC ready */
#define KCS_STATUS_IBF      0x02    /* Input Buffer Full: BMC has not taken our byte yet */
#define KCS_STATUS_SMS_ATN  0x04
#define KCS_STATUS_CD       0x08
#define KCS_STATE_MASK      0xC0
#define KCS_STATE_IDLE      0x00
#define KCS_STATE_READ      0x40
#define KCS_STATE_WRITE     0x80
#define KCS_STATE_ERROR     0xC0

/* Control codes written to the Command register */
#define KCS_CMD_GET_STATUS_ABORT    0x60
#define KCS_CMD_WRITE_START         0x61
#define KCS_CMD_WRITE_END           0x62
#define KCS_CMD_READ                0x68

/* Per-step timeouts (microseconds). The BMC may be slow. */
#define KCS_IBF_TIMEOUT     5000000
#define KCS_OBF_TIMEOUT     5000000

#define KCS_ERROR_RETRIES   2

static inline UBYTE kcs_status(struct ipmi_io *io)
{
    return ipmi_in(io, KCS_REG_STATUS);
}

static inline void kcs_write_cmd(struct ipmi_io *io, UBYTE cmd)
{
    ipmi_out(io, KCS_REG_STATUS, cmd);
}

static inline void kcs_write_data(struct ipmi_io *io, UBYTE data)
{
    ipmi_out(io, KCS_REG_DATA, data);
}

static inline UBYTE kcs_read_data(struct ipmi_io *io)
{
    return ipmi_in(io, KCS_REG_DATA);
}

static inline BOOL kcs_wait_ibf_clear(struct ipmi_io *io)
{
    return ipmi_wait(io, KCS_REG_STATUS, KCS_STATUS_IBF, 0, KCS_IBF_TIMEOUT);
}

static inline BOOL kcs_wait_obf_set(struct ipmi_io *io)
{
    return ipmi_wait(io, KCS_REG_STATUS, KCS_STATUS_OBF, KCS_STATUS_OBF, KCS_OBF_TIMEOUT);
}

/* Reading Data_Out clears OBF; the BMC expects the host to keep it clear while writing */
static inline void kcs_clear_obf(struct ipmi_io *io)
{
    if (kcs_status(io) & KCS_STATUS_OBF)
        (void)kcs_read_data(io);
}

/*
 * Write phase (spec 9.15, "Write Processing"). Returns FALSE on any
 * timeout or unexpected state; the caller then runs the abort sequence.
 */
static BOOL kcs_write(struct ipmi_io *io, const UBYTE *buf, ULONG len)
{
    ULONG i;

    if (!kcs_wait_ibf_clear(io))
        return FALSE;
    kcs_clear_obf(io);

    kcs_write_cmd(io, KCS_CMD_WRITE_START);
    if (!kcs_wait_ibf_clear(io))
        return FALSE;
    if ((kcs_status(io) & KCS_STATE_MASK) != KCS_STATE_WRITE)
        return FALSE;
    kcs_clear_obf(io);

    for (i = 0; i + 1 < len; i++)
    {
        kcs_write_data(io, buf[i]);
        if (!kcs_wait_ibf_clear(io))
            return FALSE;
        if ((kcs_status(io) & KCS_STATE_MASK) != KCS_STATE_WRITE)
            return FALSE;
        kcs_clear_obf(io);
    }

    kcs_write_cmd(io, KCS_CMD_WRITE_END);
    if (!kcs_wait_ibf_clear(io))
        return FALSE;
    if ((kcs_status(io) & KCS_STATE_MASK) != KCS_STATE_WRITE)
        return FALSE;
    kcs_clear_obf(io);

    kcs_write_data(io, buf[len - 1]);

    return TRUE;
}

/*
 * Read phase (spec 9.15, "Read Processing"). Collects at most 'max'
 * bytes into 'buf' (the rest is drained and dropped) and stores the
 * full length in *len.
 */
static BOOL kcs_read(struct ipmi_io *io, UBYTE *buf, ULONG max, ULONG *len)
{
    ULONG n = 0;

    for (;;)
    {
        UBYTE state;

        if (!kcs_wait_ibf_clear(io))
            return FALSE;

        state = kcs_status(io) & KCS_STATE_MASK;
        if (state == KCS_STATE_READ)
        {
            UBYTE b;

            if (!kcs_wait_obf_set(io))
                return FALSE;
            b = kcs_read_data(io);
            if (n < max)
                buf[n] = b;
            n++;
            kcs_write_data(io, KCS_CMD_READ);
        }
        else if (state == KCS_STATE_IDLE)
        {
            if (!kcs_wait_obf_set(io))
                return FALSE;
            (void)kcs_read_data(io);
            *len = n;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

/*
 * Abort sequence (spec 9.15, "Error Processing"). Returns the interface
 * to IDLE and retrieves the BMC's status code, which we only log.
 */
static void kcs_abort(struct ipmi_io *io)
{
    UBYTE code;

    kcs_write_cmd(io, KCS_CMD_GET_STATUS_ABORT);
    if (!kcs_wait_ibf_clear(io))
        return;
    kcs_clear_obf(io);

    kcs_write_data(io, 0x00);
    if (!kcs_wait_ibf_clear(io))
        return;
    if ((kcs_status(io) & KCS_STATE_MASK) != KCS_STATE_READ)
        return;
    if (!kcs_wait_obf_set(io))
        return;
    code = kcs_read_data(io);
    D(bug("[IPMI:KCS] abort: BMC status code 0x%02x\n", code));
    (void)code;

    kcs_write_data(io, KCS_CMD_READ);
    if (!kcs_wait_ibf_clear(io))
        return;
    if ((kcs_status(io) & KCS_STATE_MASK) != KCS_STATE_IDLE)
        return;
    if (!kcs_wait_obf_set(io))
        return;
    (void)kcs_read_data(io);
}

BOOL ipmi_kcs_transact(struct ipmi_io *io, struct ipmi_msg *msg)
{
    UBYTE req[IPMI_MAX_MSG];
    UBYTE resp[IPMI_MAX_MSG];
    ULONG reqLen, respLen = 0;
    int attempt;

    if (msg->msg_DataLen + 2 > IPMI_MAX_MSG)
        return FALSE;

    req[0] = msg->msg_NetFn;
    req[1] = msg->msg_Cmd;
    if (msg->msg_DataLen)
        CopyMem((APTR)msg->msg_Data, &req[2], msg->msg_DataLen);
    reqLen = msg->msg_DataLen + 2;

    for (attempt = 0; attempt <= KCS_ERROR_RETRIES; attempt++)
    {
        if (attempt)
            D(bug("[IPMI:KCS] retry %d, status 0x%02x\n", attempt, kcs_status(io)));

        if (!kcs_write(io, req, reqLen) || !kcs_read(io, resp, IPMI_MAX_MSG, &respLen))
        {
            D(bug("[IPMI:KCS] transaction failed, status 0x%02x\n", kcs_status(io)));
            kcs_abort(io);
            continue;
        }

        /* Response: netfn|lun (request netfn + 1), cmd, completion code, data... */
        if (respLen < 3 || resp[1] != msg->msg_Cmd
            || (resp[0] & 0xFC) != ((msg->msg_NetFn & 0xFC) + 0x04))
        {
            D(bug("[IPMI:KCS] malformed response, %u bytes, netfn 0x%02x cmd 0x%02x\n",
                respLen, respLen ? resp[0] : 0, respLen > 1 ? resp[1] : 0));
            continue;
        }

        msg->msg_RespLen = respLen - 2;
        if (respLen > IPMI_MAX_MSG)
            msg->msg_RespLen = IPMI_MAX_MSG - 2;
        CopyMem(&resp[2], msg->msg_Resp, msg->msg_RespLen);
        return TRUE;
    }

    return FALSE;
}
