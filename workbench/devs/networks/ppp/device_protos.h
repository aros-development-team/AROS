/*
 * $Id$
 */

struct PPP_DevUnit *InitPPPUnit(LIBBASETYPEPTR LIBBASE,ULONG s2unit);
VOID ExpungeUnit(LIBBASETYPEPTR LIBBASE);
BOOL ReadConfig(LIBBASETYPEPTR LIBBASE);

VOID PerformIO(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID TermIO(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);

ULONG AbortReq(LIBBASETYPEPTR LIBBASE,struct MinList *minlist,struct IOSana2Req *ios2);
VOID ConfigInterface(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID GetStationAddress(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID DeviceQuery(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID WritePacket(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);

VOID SendPacket( LIBBASETYPEPTR LIBBASE ,struct IOSana2Req *ios2 );
VOID ReadPacket(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);

VOID Online(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID Offline(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);

VOID CMD_WRITE_Ready(LIBBASETYPEPTR LIBBASE);
VOID CMD_READ_Ready(LIBBASETYPEPTR LIBBASE,struct IOExtSer *ioSer);
VOID Incoming_IP_Packet(LIBBASETYPEPTR LIBBASE, BYTE *p , ULONG length);

struct EasySerial * OpenSerial(BYTE *name,ULONG unit);
VOID _CloseSerial(struct EasySerial *s);
VOID QueueSerRequest(struct EasySerial *s , LONG length);
VOID DoStr(struct EasySerial *s,const STRPTR str);
void DoBYTES(struct EasySerial *s, BYTE *p,ULONG len);
void SendBYTES(struct EasySerial *s, BYTE *p,ULONG len);
void DrainSerial(struct EasySerial *s);
BOOL GetResponse(struct EasySerial *s,UBYTE *Buffer,ULONG maxbuffer,LONG timeout);
#define CloseSerial(x) if(x){ bug("CloseSerial\n");_CloseSerial(x);x=NULL;}

void SetTimer(struct EasyTimer* t,const ULONG s);
void CloseTimer(struct EasyTimer* t);
struct EasyTimer* OpenTimer();

void init_ppp(LIBBASETYPEPTR LIBBASE);
void bytes_received( UBYTE *bytes,ULONG len );
void send_IP_packet( BYTE *ptr ,ULONG len );
void SendTerminateReq();

BYTE Phase();
void Set_phase(UBYTE ph);
void ppp_timer(int dt);

