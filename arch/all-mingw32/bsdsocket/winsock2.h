#ifdef __x86_64__
#define __stdcall
#else
#define __stdcall __attribute__((stdcall))
#endif

#define WSABASEERR		10000

struct PROTOENT
{
    char *p_name;
    char **p_aliases;
    short p_proto;
};
