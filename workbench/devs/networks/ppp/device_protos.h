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
VOID QueueSerRequest(LIBBASETYPEPTR LIBBASE , LONG length);

BOOL OpenSerial(LIBBASETYPEPTR LIBBASE);
VOID CloseSerial(LIBBASETYPEPTR LIBBASE);
VOID DoStr(LIBBASETYPEPTR LIBBASE,const STRPTR str);
BOOL SendStr(LIBBASETYPEPTR LIBBASE,const STRPTR str ,LONG timeout);
void DoBYTES(LIBBASETYPEPTR LIBBASE, BYTE *p,ULONG len);
void SendBYTES(LIBBASETYPEPTR LIBBASE, BYTE *p,ULONG len);
void SerDelay(LIBBASETYPEPTR LIBBASE, LONG timeout);
BOOL WaitStr(LIBBASETYPEPTR LIBBASE,const STRPTR str, LONG timeout);

void SetTimer(LIBBASETYPEPTR LIBBASE,const ULONG t);

void init_ppp(LIBBASETYPEPTR LIBBASE);
void bytes_received( UBYTE *bytes,ULONG len );
void send_IP_packet( BYTE *ptr ,ULONG len );
void SendTerminateReq();
void RunRoute();
BYTE Phase();
void Set_phase(UBYTE ph);
void ppp_timer(int dt);

