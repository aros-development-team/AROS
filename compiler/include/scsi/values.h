/* scsi/values.h

	Error codes for SCSI-2 commands

   20-Mar-02   Chris Hodges     Created files

*/


#define SCSI_GOOD                 0x00
#define SCSI_CHECK_CONDITION      0x02
#define SCSI_CONDITION_MET        0x04
#define SCSI_BUSY                 0x08
#define SCSI_INTERMEDIATE         0x10
#define SCSI_IM_CONDITION_MET     0x14
#define SCSI_RESERVATION_CONFLICT 0x18
#define SCSI_TASK_SET_FULL        0x28
#define SCSI_ACA_ACTIVE           0x30
#define SCSI_TASK_ABORTED         0x40

/* Peripheral qualifiers */

#define PQ_MASK                   0xe0
#define PQ_CONNECTED              0x00
#define PQ_NOT_CONNECTED          0x20
#define PQ_DEAD_LUN               0x60

/* Peripheral device type */

#define PDT_MASK                  0x1f
#define PDT_DIRECT_ACCESS         0x00
#define PDT_SEQUENTIAL_ACCESS     0x01
#define PDT_PRINTER               0x02
#define PDT_PROCESSOR             0x03
#define PDT_WORM                  0x04
#define PDT_CDROM                 0x05
#define PDT_SCANNER               0x06
#define PDT_OPTICAL               0x07
#define PDT_MEDIUM_CHANGER        0x08
#define PDT_COMMUNICATIONS        0x09
#define PDT_RAID                  0x0c
#define PDT_ENCLOSURE             0x0d
#define PDT_SIMPLE_DIRECT_ACCESS  0x0e
#define PDT_OPTICAL_CARD          0x0f
#define PDT_OBJECT_BASED          0x11
#define PDT_UNKNOWN               0x1f

/* Sense keys */
#define SK_MASK                   0x0f
#define SK_NO_SENSE               0x00
#define SK_RECOVERED_ERROR        0x01
#define SK_NOT_READY              0x02
#define SK_MEDIUM_ERROR           0x03
#define SK_HARDWARE_ERROR         0x04
#define SK_ILLEGAL_REQUEST        0x05
#define SK_UNIT_ATTENTION         0x06
#define SK_DATA_PROTECT           0x07
#define SK_BLANK_CHECK            0x08
#define SK_VENDOR_SPECIFIC        0x09
#define SK_COPY_ABORTED           0x0a
#define SK_ABORTED_COMMAND        0x0b
#define SK_VOLUME_OVERFLOW        0x0d
#define SK_MISCOMPARE             0x0e




