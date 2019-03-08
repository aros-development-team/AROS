#ifndef DEVICES_SMART_H
#define DEVICES_SMART_H

/*
**
** $VER: smart.h 1.1 (24.02.2019)
**
** S.M.A.R.T. definitions (V1)
**
** Rupert Hausberger
**
*/

/* SMART ATA sub-commands                   */
/* (passed in io_Reserved1 && io_Reserved2) */
# define SMARTC_TEST_AVAIL                      ATAFEATURE_TEST_AVAIL

#define SMARTC_READ_VALUES                      0xD0
#define SMARTC_READ_THRESHOLDS                  0xD1
#define SMARTC_AUTOSAVE                         0xD2
#define SMARTC_SAVE                             0xD3
#define SMARTC_IMMEDIATE_OFFLINE                0xD4
#define SMARTC_READ_LOG_SECTOR                  0xD5
#define SMARTC_WRITE_LOG_SECTOR                 0xD6
#define SMARTC_WRITE_THRESHOLDS                 0xD7
#define SMARTC_ENABLE                           0xD8
#define SMARTC_DISABLE                          0xD9
#define SMARTC_STATUS                           0xDA
#define SMARTC_AUTO_OFFLINE                     0xDB

/* Legacy attributes */
#define SMARTA_RAW_READ_ERROR_RATE              0x01
#define SMARTA_THROUGHPUT_PERFORMANCE           0x02
#define SMARTA_SPINUP_TIME                      0x03
#define SMARTA_START_STOP_COUNT                 0x04
#define SMARTA_REALLOCATED_SECTOR_COUNT         0x05
#define SMARTA_READ_CHANNEL_MARGIN              0x06
#define SMARTA_SEEK_ERROR_RATE                  0x07
#define SMARTA_SEEK_TIMER_PERFORMANCE           0x08
#define SMARTA_POWERON_HOURS_COUNT              0x09
#define SMARTA_SPINUP_RETRY_COUNT               0x0A
#define SMARTA_CALIBRATION_RETRY_COUNT          0x0B
#define SMARTA_POWER_CYCLE_COUNT                0x0C
#define SMARTA_SOFT_READ_ERROR_RATE             0x0D
#define SMARTA_G_SENSE_ERROR_RATE               0xBF
#define SMARTA_POWEROFF_RETRACT_COUNT           0xC0
#define SMARTA_LOAD_UNLOAD_CYCLE_COUNT          0xC1
#define SMARTA_HDA_TEMPERATURE                  0xC2
#define SMARTA_HARDWARE_ECC_RECOVERED           0xC3
#define SMARTA_REALLOCATION_COUNT               0xC4
#define SMARTA_CURRENT_PENDING_SECTOR_COUNT     0xC5
#define SMARTA_OFFLINE_SCAN_UNCORRECTABLE_COUNT 0xC6
#define SMARTA_UDMA_CRC_ERROR_RATE              0xC7
#define SMARTA_WRITE_ERROR_RATE                 0xC8
#define SMARTA_SOFT_READ_ERROR_RATE_2           0xC9
#define SMARTA_DATA_ADDRESS_MARK_ERRORS         0xCa
#define SMARTA_RUN_OUT_CANCEL                   0xCB
#define SMARTA_SOFT_ECC_CORRECTION              0xCC
#define SMARTA_THERMAL_ASPERITY_RATE            0xCD
#define SMARTA_FLYING_HEIGHT                    0xCE
#define SMARTA_SPIN_HIGH_CURRENT                0xCF
#define SMARTA_SPIN_BUZZ                        0xD0
#define SMARTA_OFFLINE_SEEK_PERFORMANCE         0xD1
#define SMARTA_DISK_SHIFT                       0xDC
#define SMARTA_G_SENSE_ERROR_RATE_2             0xDD
#define SMARTA_LOADED_HOURS                     0xDE
#define SMARTA_LOAD_UNLOAD_RETRY_COUNT          0xDF
#define SMARTA_LOAD_FRICTION                    0xE0
#define SMARTA_LOAD_UNLOAD_CYCLE_COUNT_2        0xE1
#define SMARTA_LOAD_IN_TIME                     0xE2
#define SMARTA_TORQUE_AMPLIFICATION_COUNT       0xE3
#define SMARTA_POWER_OFF_RETRACT_COUNT          0xE4
#define SMARTA_GMR_HEAD_AMPLITUDE               0xE6
#define SMARTA_TEMPERATURE                      0xE7
#define SMARTA_HEAD_FLYING_HOURS                0xF0
#define SMARTA_READ_ERROR_RETRY_RATE            0xFA

/* Attribute flags */
#define SMARTF_PREFAILURE                       0x01
#define SMARTF_ONLINE                           0x02
/* Vendor specific */
#define SMARTF_PERFORMANCE                      0x04
#define SMARTF_ERRORRATE                        0x08
#define SMARTF_EVENTCOUNT                       0x10
#define SMARTF_SELFPRESERVING                   0x20
#define SMARTF_OTHER                            0xffc0

/* struct SMARTAttributes and SMARTThresholds */
#define SMART_DATA_LENGTH                       512

/* Maximum entries in the table */
#define SMART_MAX_ATTRIBUTES                    30

/* Validate an attribute-id */
#define SMART_ATTRIBUTE_ID_VALID(id)            ((id)>0 && (id)<0xFF)

struct SMARTAttributeEntry
{
   UBYTE                        sae_ID;
   UWORD                        sae_Flags;
   UBYTE                        sae_Value;
   UBYTE                        sae_Worst;
   UBYTE                        sae_Raw[6];
   UBYTE                        sae_Reserved;
} __attribute__((packed));

struct SMARTThresholdEntry
{
   UBYTE                        ste_ID;
   UBYTE                        ste_Value;
   UBYTE                        ste_Reserved[10];
} __attribute__((packed));

/* The following structures are 512 bytes each */
struct SMARTAttributes
{
   UWORD                        sa_Revision;
   struct SMARTAttributeEntry   sa_Attribute[SMART_MAX_ATTRIBUTES];
   UBYTE                        sa_Reserved[6];
   UWORD                        sa_Capability;
   UBYTE                        sa_Reserved2[16];
   UBYTE                        sa_VendorSpecific[125];
   UBYTE                        sa_CheckSum;
} __attribute__((packed));

struct SMARTThresholds
{
   UWORD                        st_Revision;
   struct SMARTThresholdEntry   st_Threshold[SMART_MAX_ATTRIBUTES];
   UBYTE                        st_Reserved[18];
   UBYTE                        st_VendorSpecific[131];
   UBYTE                        st_CheckSum;
} __attribute__((packed));

#endif /* DEVICES_SMART_H */
