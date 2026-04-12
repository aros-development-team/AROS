#ifndef _LPE32002_HW_H_
#define _LPE32002_HW_H_
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA.
*/

/*
 * Emulex/Broadcom Lancer-G7 SLI-4 Register Definitions
 *
 * The LPe32002 uses the SLI-4 (Service Level Interface v4) programming
 * model which communicates through memory-mapped registers and DMA-based
 * command/completion queues.
 */

#include <exec/types.h>

/* ===== PCI Identification ===== */

#define PCI_VENDOR_ID_EMULEX                0x10DF

#define PCI_DEVICE_ID_LPE32002_M2           0xE300  /* 2-Port 32Gb FC */
#define PCI_DEVICE_ID_LPE32000_M2           0xE301  /* 1-Port 32Gb FC */
#define PCI_DEVICE_ID_LPE32002_M2_D         0xE320  /* 2-Port 32Gb FC (diag) */
#define PCI_DEVICE_ID_LPE35002_M2           0xE330  /* 2-Port 32Gb FC */

/* ===== SLI-4 Configuration Registers (BAR0) ===== */

#define SLI4_SLIPORT_STATUS                 0x0404
#define SLI4_SLIPORT_CONTROL                0x0408
#define SLI4_SLIPORT_ERROR1                 0x040C
#define SLI4_SLIPORT_ERROR2                 0x0410
#define SLI4_PHYSDEV_CONTROL                0x0414

/* SLIPORT_STATUS bits */
#define SLI4_STATUS_ERR                     (1 << 31)
#define SLI4_STATUS_RN                      (1 << 24)
#define SLI4_STATUS_RDY                     (1 << 23)
#define SLI4_STATUS_FDP                     (1 << 22)
#define SLI4_STATUS_DIP                     (1 << 21)
#define SLI4_STATUS_OTI                     (1 << 20)

/* SLIPORT_CONTROL bits */
#define SLI4_CONTROL_IP                     (1 << 27)
#define SLI4_CONTROL_IDREQ                  (1 << 18)
#define SLI4_CONTROL_FDD                    (1 << 2)
#define SLI4_CONTROL_END                    (1 << 30)

/* PHYSDEV_CONTROL bits */
#define SLI4_PHYSDEV_FW_RESET               (1 << 1)
#define SLI4_PHYSDEV_DD                     (1 << 0)

/* ===== SLI-4 Control Registers (BAR1) ===== */

#define SLI4_MPU_EP_SEMAPHORE               0x00AC
#define SLI4_BOOTSTRAP_MBOX_DB              0x0160
#define SLI4_EQ_DOORBELL                    0x0120
#define SLI4_CQ_DOORBELL                    0x0120
#define SLI4_WQ_DOORBELL                    0x0100
#define SLI4_RQ_DOORBELL                    0x0108
#define SLI4_MQ_DOORBELL                    0x0140

/* MPU_EP_SEMAPHORE bits */
#define SLI4_MPU_EP_SEM_STAGE_MASK          0x0000FFFF
#define SLI4_MPU_EP_SEM_ERROR               (1 << 31)

/* Firmware ready stages */
#define SLI4_FW_STAGE_READY                 0xC000
#define SLI4_FW_STAGE_ARM_READY             0xC001
#define SLI4_FW_STAGE_ERROR                 0xF000

/* ===== Doorbell Register Formats ===== */

/* EQ/CQ Doorbell (shared register) */
#define SLI4_EQCQ_DB_EVENT                  (1 << 31)  /* re-arm */
#define SLI4_EQCQ_DB_CLEAR                  (1 << 29)
#define SLI4_EQCQ_DB_QID_SHIFT              16
#define SLI4_EQCQ_DB_QID_MASK              (0x3FF << 16)
#define SLI4_EQCQ_DB_NUM_SHIFT              0
#define SLI4_EQCQ_DB_NUM_MASK              0x1FFF

/* WQ Doorbell */
#define SLI4_WQ_DB_QID_SHIFT                16
#define SLI4_WQ_DB_QID_MASK                (0xFFFF << 16)
#define SLI4_WQ_DB_IDX_SHIFT               0
#define SLI4_WQ_DB_IDX_MASK                0x00FF

/* RQ Doorbell */
#define SLI4_RQ_DB_QID_SHIFT                16
#define SLI4_RQ_DB_QID_MASK                (0xFFFF << 16)
#define SLI4_RQ_DB_NUM_SHIFT               0
#define SLI4_RQ_DB_NUM_MASK                0x3FFF

/* MQ Doorbell */
#define SLI4_MQ_DB_QID_SHIFT                16
#define SLI4_MQ_DB_QID_MASK                (0xFFFF << 16)
#define SLI4_MQ_DB_NUM_SHIFT               0
#define SLI4_MQ_DB_NUM_MASK                0x3FFF

/* ===== Bootstrap Mailbox ===== */

#define SLI4_BMBX_SIZE                      256

/* Bootstrap mailbox register bits */
#define SLI4_BMBX_RDY                       (1 << 0)
#define SLI4_BMBX_HI                        (1 << 1)
#define SLI4_BMBX_ADDR_MASK                 0xFFFFFFFC

/* ===== Mailbox Command Opcodes ===== */

#define SLI4_MBOX_CMD_READ_REV              0x0011
#define SLI4_MBOX_CMD_READ_STATUS           0x000E
#define SLI4_MBOX_CMD_READ_CONFIG           0x000B
#define SLI4_MBOX_CMD_READ_SPARM            0x008D
#define SLI4_MBOX_CMD_READ_LINK_STAT        0x0012
#define SLI4_MBOX_CMD_REG_FCFI              0x00A0
#define SLI4_MBOX_CMD_REG_VFI              0x009F
#define SLI4_MBOX_CMD_REG_RPI              0x0093
#define SLI4_MBOX_CMD_REG_VPI              0x0096
#define SLI4_MBOX_CMD_INIT_VFI             0x00A3
#define SLI4_MBOX_CMD_INIT_VPI             0x00A4
#define SLI4_MBOX_CMD_CONFIG_PORT           0x0088
#define SLI4_MBOX_CMD_REQUEST_FEATURES      0x009D
#define SLI4_MBOX_CMD_CREATE_EQ             0x000D
#define SLI4_MBOX_CMD_CREATE_CQ             0x0015
#define SLI4_MBOX_CMD_CREATE_WQ             0x0081
#define SLI4_MBOX_CMD_CREATE_RQ             0x0085
#define SLI4_MBOX_CMD_CREATE_MQ             0x0015
#define SLI4_MBOX_CMD_DESTROY_EQ            0x000E
#define SLI4_MBOX_CMD_DESTROY_CQ            0x0036
#define SLI4_MBOX_CMD_DESTROY_WQ            0x0082
#define SLI4_MBOX_CMD_DESTROY_RQ            0x0086
#define SLI4_MBOX_CMD_DESTROY_MQ            0x0035
#define SLI4_MBOX_CMD_NOP                   0x003B
#define SLI4_MBOX_CMD_DUMP                  0x0017

/* Mailbox status codes */
#define SLI4_MBOX_STATUS_SUCCESS            0x0000
#define SLI4_MBOX_STATUS_FAILED             0x0001
#define SLI4_MBOX_STATUS_ILLEGAL_CMD        0x0002
#define SLI4_MBOX_STATUS_ILLEGAL_FIELD      0x0003
#define SLI4_MBOX_STATUS_INSUFFICIENT_BUF   0x0004

/* ===== Mailbox Command Header ===== */

struct sli4_mbox_hdr {
    ULONG   opcode:8;
    ULONG   subsystem:8;
    ULONG   port_number:2;
    ULONG   domain:5;
    ULONG   rsvd0:1;
    ULONG   timeout:8;
    ULONG   request_length;
    ULONG   status:8;
    ULONG   additional_status:8;
    ULONG   rsvd1:16;
    ULONG   response_length;
} __attribute__((packed));

/* Generic mailbox command wrapper */
struct sli4_mbox_cmd {
    struct sli4_mbox_hdr hdr;
    ULONG   payload[58];  /* up to 232 bytes of payload */
} __attribute__((packed));

/* ===== Queue Entry Structures ===== */

/* Queue entry sizes */
#define SLI4_EQ_ENTRY_SIZE                  4       /* bytes */
#define SLI4_CQ_ENTRY_SIZE                  16      /* bytes */
#define SLI4_WQ_ENTRY_SIZE                  64      /* bytes */
#define SLI4_RQ_ENTRY_SIZE                  8       /* bytes */
#define SLI4_MQ_ENTRY_SIZE                  256     /* bytes */

/* Default queue depths */
#define LPE_EQ_DEPTH                        256
#define LPE_CQ_DEPTH                        256
#define LPE_WQ_DEPTH                        64
#define LPE_RQ_DEPTH                        128
#define LPE_MQ_DEPTH                        16

/* Event Queue Entry */
struct sli4_eq_entry {
    ULONG   major_code:3;
    ULONG   rsvd0:28;
    ULONG   valid:1;
} __attribute__((packed));

#define SLI4_EQ_MAJOR_CODE_CQ              0x00
#define SLI4_EQ_VALID                      (1 << 0)

/* Completion Queue Entry for WQ/RQ completions */
struct sli4_cq_entry {
    ULONG   word0;
    ULONG   word1;
    ULONG   word2;
    ULONG   word3;        /* bits[31]=valid, bits[30:29]=code */
} __attribute__((packed));

#define SLI4_CQ_ENTRY_VALID               (1UL << 31)
#define SLI4_CQ_ENTRY_CODE_SHIFT          29
#define SLI4_CQ_ENTRY_CODE_MASK           (0x7UL << 29)
#define SLI4_CQ_ENTRY_CODE_WQ             0x01
#define SLI4_CQ_ENTRY_CODE_RQ             0x04
#define SLI4_CQ_ENTRY_CODE_XRI_ABORT      0x05
#define SLI4_CQ_ENTRY_STATUS_SHIFT        16
#define SLI4_CQ_ENTRY_STATUS_MASK         (0xFFUL << 16)

/* Work Queue Entry (Send Frame) */
struct sli4_wq_entry {
    ULONG   words[16];    /* 64 bytes total */
} __attribute__((packed));

/* WQE opcodes */
#define SLI4_WQE_SEND_FRAME               0x09
#define SLI4_WQE_ELS_REQUEST               0x0A
#define SLI4_WQE_ELS_RSP                   0x0B
#define SLI4_WQE_ABORT                     0x0F
#define SLI4_WQE_FCP_IWRITE                0x11
#define SLI4_WQE_FCP_IREAD                 0x12
#define SLI4_WQE_FCP_ICMND                 0x13

/* Receive Queue Entry (buffer descriptor) */
struct sli4_rq_entry {
    ULONG   address_lo;
    ULONG   address_hi;
} __attribute__((packed));

/* ===== Queue Descriptor ===== */

struct sli4_queue {
    UWORD   id;            /* queue ID assigned by firmware */
    UWORD   type;          /* queue type */
    UWORD   size;          /* entry count */
    UWORD   entry_size;    /* bytes per entry */
    UWORD   index;         /* current producer/consumer index */
    UWORD   num_proc;      /* entries processed since last doorbell */
    APTR    base;          /* virtual base of queue memory */
    APTR    base_dma;      /* DMA (physical) base address */
    ULONG   total_size;    /* total allocated bytes */
};

/* Queue types */
#define SLI4_QTYPE_EQ                      0
#define SLI4_QTYPE_CQ                      1
#define SLI4_QTYPE_WQ                      2
#define SLI4_QTYPE_RQ                      3
#define SLI4_QTYPE_MQ                      4

/* ===== Fibre Channel Frame Definitions ===== */

#define FC_ADDR_SIZE                        8       /* WWPN size in bytes */
#define FC_HEADER_SIZE                      24      /* FC frame header */
#define FC_MAX_PAYLOAD                      2112    /* max data field */
#define FC_CRC_SIZE                         4
#define FC_MTU                              FC_MAX_PAYLOAD
#define FC_MAX_FRAME_SIZE                   (FC_HEADER_SIZE + FC_MAX_PAYLOAD + FC_CRC_SIZE)

/* FC Frame Header */
struct fc_frame_hdr {
    UBYTE   r_ctl;          /* routing control */
    UBYTE   d_id[3];        /* destination N_Port ID */
    UBYTE   cs_ctl;         /* class / service control */
    UBYTE   s_id[3];        /* source N_Port ID */
    UBYTE   type;           /* data structure type */
    UBYTE   f_ctl[3];       /* frame control */
    UBYTE   seq_id;
    UBYTE   df_ctl;
    UWORD   seq_cnt;
    UWORD   ox_id;
    UWORD   rx_id;
    ULONG   parameter;
} __attribute__((packed));

/* FC R_CTL values */
#define FC_RCTL_DATA_DEVICE                0x00
#define FC_RCTL_ELS_REQ                    0x22
#define FC_RCTL_ELS_RSP                    0x23
#define FC_RCTL_BLS_ABTS                   0x81

/* FC Frame Types */
#define FC_TYPE_BLS                        0x00
#define FC_TYPE_ELS                        0x01
#define FC_TYPE_FCP                        0x08
#define FC_TYPE_IP                         0x05    /* IP over FC */
#define FC_TYPE_CT                         0x20

/* FC Send/Receive buffer structure */
struct lpe_buffer {
    APTR    virt;           /* virtual address */
    APTR    dma;            /* DMA-mapped address */
    ULONG   size;           /* buffer size */
};

/* Per-frame TX descriptor */
struct lpe_tx_desc {
    struct lpe_buffer   buf;
    UWORD               length;
    UWORD               next_to_watch;
};

/* Per-frame RX descriptor */
struct lpe_rx_desc {
    struct lpe_buffer   hdr_buf;    /* header buffer (RQ pair entry 0) */
    struct lpe_buffer   data_buf;   /* data buffer (RQ pair entry 1) */
};

/* ===== SLI-4 Port Parameters ===== */

struct sli4_params {
    ULONG   fw_rev[2];     /* firmware revision */
    ULONG   sli_rev;       /* SLI revision */
    ULONG   sli_family;
    ULONG   feature_flags;
    ULONG   max_eq;
    ULONG   max_cq;
    ULONG   max_wq;
    ULONG   max_rq;
    ULONG   max_xri;
    ULONG   max_rpi;
    ULONG   max_vpi;
    ULONG   max_vfi;
    ULONG   max_fcfi;
    UBYTE   wwnn[8];       /* World Wide Node Name */
    UBYTE   wwpn[8];       /* World Wide Port Name */
    ULONG   link_speed;    /* current link speed in Mbps */
    UBYTE   topology;      /* fabric, p2p, loop */
};

/* Link speed values (Mbps) */
#define FC_LINK_SPEED_1G                   1000
#define FC_LINK_SPEED_2G                   2000
#define FC_LINK_SPEED_4G                   4000
#define FC_LINK_SPEED_8G                   8000
#define FC_LINK_SPEED_16G                  16000
#define FC_LINK_SPEED_32G                  32000

/* Topology types */
#define FC_TOPO_P2P                        0x01
#define FC_TOPO_FABRIC                     0x02
#define FC_TOPO_LOOP                       0x04

/* ===== Hardware Operations ===== */

/* Register I/O macros */
#define LPE_REG_READ32(base, off) \
    (*(volatile ULONG *)((UBYTE *)(base) + (off)))

#define LPE_REG_WRITE32(base, off, val) \
    do { *(volatile ULONG *)((UBYTE *)(base) + (off)) = (val); } while(0)

#define LPE_WRITE_FLUSH(base) \
    do { (void)LPE_REG_READ32(base, SLI4_SLIPORT_STATUS); } while(0)

/* Forward declaration — full definition is in lpe32002.h */
struct LPe32002Unit;

/* Hardware function prototypes */
int  lpe_hw_init(struct LPe32002Unit *unit);
void lpe_hw_reset(struct LPe32002Unit *unit);
int  lpe_fw_wait_ready(struct LPe32002Unit *unit);
int  lpe_bmbx_command(struct LPe32002Unit *unit, struct sli4_mbox_cmd *cmd);
int  lpe_hw_read_rev(struct LPe32002Unit *unit);
int  lpe_hw_read_config(struct LPe32002Unit *unit);
int  lpe_hw_read_sparm(struct LPe32002Unit *unit);
int  lpe_hw_request_features(struct LPe32002Unit *unit);
int  lpe_hw_config_port(struct LPe32002Unit *unit);
int  lpe_hw_read_link_stat(struct LPe32002Unit *unit);

int  lpe_queue_create_eq(struct LPe32002Unit *unit, struct sli4_queue *eq);
int  lpe_queue_create_cq(struct LPe32002Unit *unit, struct sli4_queue *cq, struct sli4_queue *eq);
int  lpe_queue_create_wq(struct LPe32002Unit *unit, struct sli4_queue *wq, struct sli4_queue *cq);
int  lpe_queue_create_rq(struct LPe32002Unit *unit, struct sli4_queue *rq_hdr, struct sli4_queue *rq_data, struct sli4_queue *cq);
int  lpe_queue_create_mq(struct LPe32002Unit *unit, struct sli4_queue *mq, struct sli4_queue *cq);

void lpe_queue_destroy(struct LPe32002Unit *unit, struct sli4_queue *q);
int  lpe_setup_queues(struct LPe32002Unit *unit);
void lpe_teardown_queues(struct LPe32002Unit *unit);

void lpe_eq_arm(struct LPe32002Unit *unit, struct sli4_queue *eq, BOOL arm);
void lpe_cq_arm(struct LPe32002Unit *unit, struct sli4_queue *cq, BOOL arm);
void lpe_irq_enable(struct LPe32002Unit *unit);
void lpe_irq_disable(struct LPe32002Unit *unit);
int  lpe_request_irq(struct LPe32002Unit *unit);
void lpe_free_irq(struct LPe32002Unit *unit);

int  lpe_tx_frame(struct LPe32002Unit *unit, APTR data, ULONG len);
int  lpe_process_cq(struct LPe32002Unit *unit, struct sli4_queue *cq);
void lpe_set_mac(struct LPe32002Unit *unit);

#endif /* _LPE32002_HW_H_ */
