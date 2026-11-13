
/* DEVICE STUFF */

#define DEVNAME             "usbscsi.device"

#define DEVBASETYPEPTR struct NepMSDevBase *

/* local protos */

AROS_UFP3(DEVBASETYPEPTR, GM_UNIQUENAME(devInit),
          AROS_UFPA(DEVBASETYPEPTR, base, D0),
          AROS_UFPA(BPTR, seglist, A0),
          AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_LD3(DEVBASETYPEPTR, devOpen,
         AROS_LDA(struct IORequest *, ioreq, A1),
         AROS_LDA(ULONG, unitnum, D0),
         AROS_LDA(ULONG, flags, D1),
         DEVBASETYPEPTR, base, 1, usbscsidev);

AROS_LD1(BPTR, devClose,
         AROS_LDA(struct IORequest *, ioreq, A1),
         DEVBASETYPEPTR, base, 2, usbscsidev);

AROS_LD1(BPTR, devExpunge,
         AROS_LDA(DEVBASETYPEPTR, extralh, D0),
         DEVBASETYPEPTR, base, 3, usbscsidev);

AROS_LD0(DEVBASETYPEPTR, devReserved,
         DEVBASETYPEPTR, base, 4, usbscsidev);

AROS_LD1(void, devBeginIO,
         AROS_LDA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 5, usbscsidev);

AROS_LD1(LONG, devAbortIO,
         AROS_LDA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 6, usbscsidev);

/* Device stuff */

/* Reply the iorequest with success */
#define RC_OK         0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY  -1

WORD GM_UNIQUENAME(cmdNSDeviceQuery)(struct IOStdReq *ioreq, struct NepClassMS *unit, struct NepMSDevBase *base);

void GM_UNIQUENAME(TermIO)(struct IOStdReq *ioreq, struct NepMSDevBase *base);

struct my_NSDeviceQueryResult
{
    ULONG   DevQueryFormat;         /* this is type 0               */
    ULONG   SizeAvailable;          /* bytes available              */
    UWORD   DeviceType;             /* what the device does         */
    UWORD   DeviceSubType;          /* depends on the main type     */
    const UWORD *SupportedCommands; /* 0 terminated list of cmd's   */
};
