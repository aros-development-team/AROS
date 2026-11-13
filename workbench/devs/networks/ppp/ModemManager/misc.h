/*
 * $Id$
 */

 struct EasyTimer{
	struct MsgPort  *TimeMsg;
	struct timerequest *TimeReq;
};

 struct EasySerial{
	struct IOExtSer	*SerRx;   /* Serial IORequest for CMD_READ's */
	struct IOExtSer	*SerTx;   /* Serial IORequest for CMD_WRITE's */
	struct MsgPort	*RxPort;  /* Serial CMD_READ IORequest reply port */
	struct MsgPort	*TxPort;  /* Serial CMD_WRITE IORequest reply port */
	UBYTE			*RxBuff;	/* Buffer for holding incoming data */
	UBYTE			*TxBuff;	/* Buffer for hold outgoing packets */
	BOOL Ok; // is device ok (= not unplugged)
};

struct Conf{

	BYTE InterfaceName[PPP_MAXARGLEN];
	BYTE DeviceName[PPP_MAXARGLEN];
	BYTE SerUnitNum;

	struct List  atcl;

	BYTE username[PPP_MAXARGLEN];
	BYTE password[PPP_MAXARGLEN];

	BYTE modemmodel[PPP_MAXARGLEN];
	BYTE signal;
	BYTE AccessType;

	ULONG CommandTimeOut;

	UBYTE state;

};

struct EasySerial * OpenSerial(BYTE *name,ULONG unit);
VOID _CloseSerial(struct EasySerial *s);
VOID QueueSerRequest(struct EasySerial *s , LONG length);
VOID DoStr(struct EasySerial *s,const STRPTR str);
void DoBYTES(struct EasySerial *s, BYTE *p,ULONG len);
void SendBYTES(struct EasySerial *s, BYTE *p,ULONG len);
void DrainSerial(struct EasySerial *s);
BOOL GetResponse(struct EasySerial *s,UBYTE *Buffer,ULONG maxbuffer,LONG timeout);
#define CloseSerial(x) if(x){bug("CloseSerial\n");_CloseSerial(x);x=NULL;}

void SetTimer(struct EasyTimer* t,const ULONG s);
void CloseTimer(struct EasyTimer* t);
struct EasyTimer* OpenTimer();

BOOL ReadConfig(struct Conf *c);
BOOL DialUp(struct EasySerial *s,struct Conf *c);
BOOL TestModem(struct EasySerial *s,struct Conf *c);

BOOL StartStack();

