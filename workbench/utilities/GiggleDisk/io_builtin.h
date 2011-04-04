/*
** device struct
*/

struct MDevice
{
     struct IORequest *IORequest;
     struct MsgPort *MessagePort;
     BOOL DeviceOpen;
};
