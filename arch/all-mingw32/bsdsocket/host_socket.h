/* WinSock virtual hardware */

struct SocketController
{
    unsigned char  SocketIRQ;
    unsigned char  ResolverIRQ;
    void 	  *SocketEvent;
    void	  *ResolverEvent;
    unsigned char  Command;
};

#define SOCK_CMD_SHUTDOWN 0x01
