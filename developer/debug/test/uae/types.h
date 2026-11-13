#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

#include <netinet/in.h>

typedef char BYTE;
typedef unsigned char UBYTE;

class WORD
{
/* This is the data. It's defined in protected mode so the child classes
   can directly access it. Note that the value is encoded in *big endian*. */
protected:
    short data;

/* Now the initializations and the conversions. They are defined in public
   mode because everyone must be able to access them. */
public:
    /* Convert the data to int in host-endianess */
    inline operator int ()
    {
	return (int) ntohs (data);
    }

    /* Same for short */
    inline operator short ()
    {
	return ntohs (data);
    }

    /* Create a variable of type WORD from a short */
    inline WORD (short v)
    {
	data = htons (v);
    }

    /* Same but from an int */
    inline WORD (int v)
    {
	//printf ("WORD(int): v=%d\n", v);
	data = htons (((short)v));
	//printf ("WORD(int): data=%d\n", data);
    }

    /* How to copy a variable of type WORD */
    inline WORD (const WORD& v)
    {
	data = v.data;
    }

    /* How to create an uninitilized WORD variable */
    inline WORD ()
    {
	return;
    }
};

/* This is pretty much the same but different types */
class APTR
{
protected:
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
	data = htonl (((long)v));
    }

    inline APTR (const APTR& v)
    {
	data = v.data;
    }

    inline APTR ()
    {
	return;
    }

#ifdef DEBUG
    /* Debugging */
    inline void print ()
    {
	printf ("%08lx", data);
    }
#endif
};

/* This is an example how to inhert the functionality for other
   pointers. */
class STRPTR : APTR
{
public:
    inline operator char * ()
    {
	return (char *) ntohl (data);
    }

    inline operator const char * ()
    {
	return (const char *) ntohl (data);
    }

    inline STRPTR ()
    {
	return;
    }

    inline STRPTR (char * v)
    {
	data = htonl ((long)v);
    }
};

#endif /* EXEC_TYPES_H */

