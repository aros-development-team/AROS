
/* DEVICE STUFF */

#define DEVNAME             "dm9601eth.device"

#define DEVBASETYPEPTR struct NepEthDevBase *

/* local protos */

AROS_UFP3(DEVBASETYPEPTR, devInit,
          AROS_UFPA(DEVBASETYPEPTR, base, D0),
          AROS_UFPA(BPTR, seglist, A0),
          AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_LD3(DEVBASETYPEPTR, devOpen,
         AROS_LDA(struct IOSana2Req *, ioreq, A1),
         AROS_LDA(ULONG, unitnum, D0),
         AROS_LDA(ULONG, flags, D1),
         DEVBASETYPEPTR, base, 1, dev);

AROS_LD1(BPTR, devClose,
         AROS_LDA(struct IOSana2Req *, ioreq, A1),
         DEVBASETYPEPTR, base, 2, dev);

AROS_LD1(BPTR, devExpunge,
         AROS_LDA(DEVBASETYPEPTR, extralh, D0),
         DEVBASETYPEPTR, base, 3, dev);

AROS_LD0(DEVBASETYPEPTR, devReserved,
         DEVBASETYPEPTR, base, 4, dev);

AROS_LD1(void, devBeginIO,
         AROS_LDA(struct IOSana2Req *, ioreq, A1),
         DEVBASETYPEPTR, base, 5, dev);

AROS_LD1(LONG, devAbortIO,
         AROS_LDA(struct IOSana2Req *, ioreq, A1),
         DEVBASETYPEPTR, base, 6, dev);

/* Device stuff */

#define deverror(ioerr,wireerr) (((wireerr) << 8) | ((ioerr) & 0xff))

/* Reply the iorequest with success
*/
#define RC_OK         0

/* Magic cookie, don't set error fields & don't reply the ioreq
*/
#define RC_DONTREPLY  -1

struct Unit *Open_Unit(struct IOSana2Req *ioreq,
                       LONG unitnr,
                       struct NepEthDevBase *base);
void Close_Unit(struct NepEthDevBase *base, struct NepClassEth *ncp,
                struct IOSana2Req *ioreq);

WORD cmdNSDeviceQuery(struct NepClassEth *ncp, struct IOStdReq *ioreq);

LONG AbortReq(struct NepClassEth *ncp, struct List *list, struct IOSana2Req *ioreq);
void TermIO(struct NepClassEth *ncp, struct IOSana2Req *ioreq);
void AbortList(struct NepClassEth *ncp, struct List *list, struct BufMan *bufman, WORD error);
void AbortRW(struct NepClassEth *ncp, struct BufMan *bufman, WORD error);
struct Sana2PacketTypeStats * FindPacketTypeStats(struct NepClassEth *ncp, ULONG packettype);
WORD AddMCastRange(struct NepClassEth *ncp, struct IOSana2Req *ioreq, UBYTE *lower, UBYTE *upper);
WORD DelMCastRange(struct NepClassEth *ncp, struct IOSana2Req *ioreq, UBYTE *lower, UBYTE *upper);
void UpdateMulticastHash(struct NepClassEth *ncp);

struct my_NSDeviceQueryResult
{
    ULONG   DevQueryFormat;         /* this is type 0               */
    ULONG   SizeAvailable;          /* bytes available              */
    UWORD   DeviceType;             /* what the device does         */
    UWORD   DeviceSubType;          /* depends on the main type     */
    const UWORD *SupportedCommands; /* 0 terminated list of cmd's   */
};
