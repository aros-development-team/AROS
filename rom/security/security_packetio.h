/* Header file for the FS Enforcer PacketIO
 *
 * This code is ©1998 Wez Furlong, All Rights Reserved
 *
 */

#include <dos/dosextens.h>

/*
*		Internal Function Prototypes
*/

extern struct secExtOwner* GetPktOwner(struct SecurityBase *secBase, struct DosPacket* pkt);
extern struct Task * GetPktTask(struct DosPacket *pkt);
extern LONG GetPktDefProtection(struct SecurityBase *secBase, struct DosPacket *pkt);

extern SIPTR secFSDoPkt(struct secVolume *Vol, LONG act, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5);
extern SIPTR DoPacket(struct secVolume *Vol, struct DosPacket* pkt);

#if (0)
extern BOOL myExamineFH(struct muVolume *Vol,BPTR fh);
extern BPTR myDupLockFromFH(struct muVolume *Vol, LONG arg1);
extern BOOL myExamine(struct muVolume* Vol, BPTR lock);
extern BOOL myChangeMode(struct muVolume *Vol, LONG type, BPTR object, LONG mode);
extern BPTR myParent(struct muVolume*Vol, BPTR lock);
extern BPTR myLock(struct muVolume*Vol, BSTR name, LONG mode, BPTR rlock);
extern BOOL mySetProtect(struct muVolume *Vol, BPTR rlock, BSTR name, LONG prot);

extern BOOL mySetOwner(struct muVolume *Vol, ULONG Owner, BSTR name, BPTR rlock);
extern BOOL myUnLock(struct muVolume* Vol,BPTR lock);

extern BOOL myAltExamine(struct muVolume* Vol, BSTR name);
extern BOOL myExAll(struct muVolume* Vol, BPTR lock, STRPTR buffer, LONG size, struct ExAllControl *eac);
#endif