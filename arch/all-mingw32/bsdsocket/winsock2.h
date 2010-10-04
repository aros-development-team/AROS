#ifdef __x86_64__
#define __stdcall
#else
#define __stdcall __attribute__((stdcall))
#endif

#define FD_READ_BIT      0
#define WS_FD_READ       (1 << FD_READ_BIT)
#define FD_WRITE_BIT     1
#define WS_FD_WRITE      (1 << FD_WRITE_BIT)
#define FD_OOB_BIT       2
#define WS_FD_OOB        (1 << FD_OOB_BIT)
#define FD_ACCEPT_BIT    3
#define WS_FD_ACCEPT     (1 << FD_ACCEPT_BIT)
#define FD_CONNECT_BIT   4
#define WS_FD_CONNECT    (1 << FD_CONNECT_BIT)
#define FD_CLOSE_BIT     5
#define WS_FD_CLOSE      (1 << FD_CLOSE_BIT)

#define WSABASEERR		10000

struct PROTOENT
{
    char *p_name;
    char **p_aliases;
    short p_proto;
};

struct WSsockaddr
{
	u_short sa_family;
	char	sa_data[14];
};
