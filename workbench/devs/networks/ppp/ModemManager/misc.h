/*
 * $Id: misc.h 35800 2010-11-27 15:54:18Z Sami $
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
	
	BYTE DeviceName[PPP_MAXARGLEN];
	BYTE SerUnitNum;

	struct List  atcl;
	
	BYTE username[PPP_MAXARGLEN];
	BYTE password[PPP_MAXARGLEN];
	BOOL enable_dns;
	BYTE modemmodel[PPP_MAXARGLEN];
	ULONG CommandTimeOut;

	UBYTE state;
	
};


struct EasySerial * OpenSerial(BYTE *name,ULONG unit);
VOID CloseSerial(struct EasySerial *s);
VOID QueueSerRequest(struct EasySerial *s , LONG length);
VOID DoStr(struct EasySerial *s,const STRPTR str);
void DoBYTES(struct EasySerial *s, BYTE *p,ULONG len);
void SendBYTES(struct EasySerial *s, BYTE *p,ULONG len);
void DrainSerial(struct EasySerial *s);
BOOL GetResponse(struct EasySerial *s,UBYTE *Buffer,ULONG maxbuffer,LONG timeout);
#define CLOSESERIAL(x) do{ CloseSerial(x);x=NULL;}while(0);

void SetTimer(struct EasyTimer* t,const ULONG s);
void CloseTimer(struct EasyTimer* t);
struct EasyTimer* OpenTimer();

BOOL ReadConfig(struct Conf *c);
BOOL DialUp(struct EasySerial *s,struct Conf *c);
BOOL TestModem(struct EasySerial *s,struct Conf *c);
