
#include <netinet/in.h>

typedef char BYTE;
typedef unsigned char UBYTE;

class WORD
{
private:
    short data;

public:
    inline operator int ()
    {
	return (int) ntohs (data);
    }

    inline operator short ()
    {
	return ntohs (data);
    }

    inline WORD (short v)
    {
	data = htons (v);
    }

    inline WORD (int v)
    {
	printf ("WORD(int): v=%d\n", v);
	data = htons (((short)v));
	printf ("WORD(int): data=%d\n", data);
    }

    inline WORD (const WORD& v)
    {
	data = v.data;
    }

    inline WORD ()
    {
	return;
    }
};

class APTR
{
private:
    long data;

public:
    inline operator int ()
    {
	return (int) ntohl (data);
    }

    inline operator void * ()
    {
	return (void *) ntohl (data);
    }

    inline APTR (int v)
    {
	data = htonl (v);
    }

    inline APTR (void * v)
    {
	printf ("APTR(void *): v=%p\n", v);
	data = htonl (((long)v));
	printf ("APTR(void *): data=%p\n", (void *)data);
    }

    inline APTR (const APTR& v)
    {
	data = v.data;
    }

    inline APTR ()
    {
	return;
    }
};

