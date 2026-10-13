/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: IPMI Block Transfer (BT) system interface,
          IPMI v2.0 specification section 11. Polled operation.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "ipmi_transport.h"

/* Registers (index, scaled by the register spacing) */
#define BT_REG_CTRL         0       /* BT_CTRL */
#define BT_REG_DATA         1       /* HOST2BMC (write) / BMC2HOST (read) */
#define BT_REG_INTMASK      2       /* BT_INTMASK */

/* BT_CTRL bits. Writing 1 to CLR_*_PTR, H2B_ATN and B2H_ATN performs the
   action; writing 1 to H_BUSY toggles it; B_BUSY is read-only. */
#define BT_CTRL_CLR_WR_PTR  0x01
#define BT_CTRL_CLR_RD_PTR  0x02
#define BT_CTRL_H2B_ATN     0x04    /* host has written a request */
#define BT_CTRL_B2H_ATN     0x08    /* BMC has written a response */
#define BT_CTRL_SMS_ATN     0x10
#define BT_CTRL_OEM0        0x20
#define BT_CTRL_H_BUSY      0x40
#define BT_CTRL_B_BUSY      0x80

/* BT_INTMASK bits */
#define BT_INTMASK_B2H_IRQ_EN   0x01
#define BT_INTMASK_B2H_IRQ      0x02    /* write 1 to clear */
#define BT_INTMASK_BMC_HWRST    0x80

#define BT_READY_TIMEOUT    5000000     /* wait for B_BUSY/H2B_ATN to clear */
#define BT_RESPONSE_TIMEOUT 5000000     /* wait for B2H_ATN */

#define BT_ERROR_RETRIES    2

static inline UBYTE bt_ctrl(struct ipmi_io *io)
{
    return ipmi_in(io, BT_REG_CTRL);
}

static inline void bt_ctrl_set(struct ipmi_io *io, UBYTE bits)
{
    ipmi_out(io, BT_REG_CTRL, bits);
}

/* Bring the interface to a known state: host not busy, no stale response pending */
static void bt_reset(struct ipmi_io *io)
{
    UBYTE ctrl = bt_ctrl(io);

    if (ctrl & BT_CTRL_H_BUSY)
        bt_ctrl_set(io, BT_CTRL_H_BUSY);
    if (ctrl & BT_CTRL_B2H_ATN)
        bt_ctrl_set(io, BT_CTRL_B2H_ATN);
    bt_ctrl_set(io, BT_CTRL_CLR_WR_PTR | BT_CTRL_CLR_RD_PTR);
    ipmi_out(io, BT_REG_INTMASK, BT_INTMASK_B2H_IRQ);   /* interrupts off, clear pending */
}

static BOOL bt_write(struct ipmi_io *io, const UBYTE *buf, ULONG len)
{
    ULONG i;

    if (!ipmi_wait(io, BT_REG_CTRL, BT_CTRL_B_BUSY | BT_CTRL_H2B_ATN, 0, BT_READY_TIMEOUT))
        return FALSE;

    bt_ctrl_set(io, BT_CTRL_CLR_WR_PTR);
    for (i = 0; i < len; i++)
        ipmi_out(io, BT_REG_DATA, buf[i]);
    bt_ctrl_set(io, BT_CTRL_H2B_ATN);

    return TRUE;
}

static BOOL bt_read(struct ipmi_io *io, UBYTE *buf, ULONG max, ULONG *len)
{
    ULONG n, i;

    if (!ipmi_wait(io, BT_REG_CTRL, BT_CTRL_B2H_ATN, BT_CTRL_B2H_ATN, BT_RESPONSE_TIMEOUT))
        return FALSE;

    bt_ctrl_set(io, BT_CTRL_H_BUSY);        /* claim the buffer */
    bt_ctrl_set(io, BT_CTRL_B2H_ATN);       /* acknowledge */
    bt_ctrl_set(io, BT_CTRL_CLR_RD_PTR);

    n = ipmi_in(io, BT_REG_DATA);           /* length of what follows */
    for (i = 0; i < n; i++)
    {
        UBYTE b = ipmi_in(io, BT_REG_DATA);

        if (i < max)
            buf[i] = b;
    }

    bt_ctrl_set(io, BT_CTRL_H_BUSY);        /* release the buffer */
    *len = n;

    return TRUE;
}

BOOL ipmi_bt_transact(struct ipmi_io *io, struct ipmi_msg *msg)
{
    UBYTE req[IPMI_MAX_MSG];
    UBYTE resp[IPMI_MAX_MSG];
    ULONG reqLen, respLen = 0;
    int attempt;

    /* Length byte, netfn/lun, seq, cmd, data */
    if (msg->msg_DataLen + 4 > IPMI_MAX_MSG || msg->msg_DataLen + 3 > 255)
        return FALSE;

    for (attempt = 0; attempt <= BT_ERROR_RETRIES; attempt++)
    {
        UBYTE seq = io->io_Data->ipmi_BTSeq++;

        req[0] = msg->msg_DataLen + 3;
        req[1] = msg->msg_NetFn;
        req[2] = seq;
        req[3] = msg->msg_Cmd;
        if (msg->msg_DataLen)
            CopyMem((APTR)msg->msg_Data, &req[4], msg->msg_DataLen);
        reqLen = msg->msg_DataLen + 4;

        if (attempt)
            D(bug("[IPMI:BT] retry %d, ctrl 0x%02x\n", attempt, bt_ctrl(io)));
        bt_reset(io);

        if (!bt_write(io, req, reqLen) || !bt_read(io, resp, IPMI_MAX_MSG, &respLen))
        {
            D(bug("[IPMI:BT] transaction failed, ctrl 0x%02x\n", bt_ctrl(io)));
            continue;
        }

        /* Response: netfn|lun (request netfn + 1), seq, cmd, completion code, data... */
        if (respLen < 4 || resp[1] != seq || resp[2] != msg->msg_Cmd
            || (resp[0] & 0xFC) != ((msg->msg_NetFn & 0xFC) + 0x04))
        {
            D(bug("[IPMI:BT] malformed response, %u bytes, netfn 0x%02x seq 0x%02x cmd 0x%02x\n",
                respLen, respLen ? resp[0] : 0, respLen > 1 ? resp[1] : 0, respLen > 2 ? resp[2] : 0));
            continue;
        }

        msg->msg_RespLen = respLen - 3;
        if (respLen > IPMI_MAX_MSG)
            msg->msg_RespLen = IPMI_MAX_MSG - 3;
        CopyMem(&resp[3], msg->msg_Resp, msg->msg_RespLen);
        return TRUE;
    }

    return FALSE;
}
