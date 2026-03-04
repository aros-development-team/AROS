/****** bsdsocket.library/inet_addr *****************************************
*
*   NAME
*        inet_addr,  inet_network,  Inet_MakeAddr,  Inet_LnaOf,
*        Inet_NetOf, Inet_NtoA - Internet address manipulation
*
*        inet_makeaddr, inet_lnaof, inet_netof,
*        inet_ntoa -- inline/stub functions to handle structure arguments
*
*   SYNOPSIS
*        #include <netinet/in.h>
*
*        addr = inet_addr(cp)
*        D0               A0
*
*        unsigned long inet_addr(char *);
*
*        net = inet_network(cp)
*        D0                 A0
*        
*        unsigned long inet_network(char *);
*
*        in_addr = Inet_MakeAddr(net, lna)
*        D0                      D0   D1
*
*        unsigned long Inet_MakeAddr(long, long);
*
*        lna = Inet_LnaOf(in)
*        D0               D0
*
*        long Inet_LnaOf(unsigned long);
*
*        net = Inet_NetOf(in)
*        D0               D0
*
*        long Inet_NetOf(unsigned long);
*
*        addr = Inet_NtoA(in)
*        DO               D0
*
*        char * Inet_NtoA(unsigned long);
*
*        
*        in_addr = inet_makeaddr(net, lna)
*
*        struct in_addr inet_makeaddr(long, long);
*
*        lna = inet_lnaof(in)
*
*        int inet_lnaof(struct in_addr);
*
*        net = inet_netof(in)
*
*        int inet_netof(struct in_addr);
*
*        addr = inet_ntoa(in)
*
*        char * inet_ntoa(struct in_addr);
*
*   IMPLEMENTATION NOTE
*        Return  value  of  Inet_MakeAddr()  and  argument  types  of
*        Inet_LnaOf(), Inet_NetOf() and Inet_NtoA() are longs instead
*        of  struct  in_addr.  The original behaviour  is achieved by
*        using  included  stub  functions (lower case function names)
*        which handle structure arguments.
*
*   DESCRIPTION
*        The routines inet_addr() and inet_network()  each  interpret
*        character  strings  representing  numbers  expressed  in the
*        Internet standard `.' notation, returning  numbers  suitable
*        for  use as Internet addresses and Internet network numbers,
*        respectively.  The routine inet_makeaddr() takes an Internet
*        network number and a local network address and constructs an
*        Internet address from it.   The  routines  inet_netof()  and
*        inet_lnaof()  break apart Internet host addresses, returning
*        the network number and local network address  part,  respec-
*        tively.
*
*        The routine inet_ntoa() returns a pointer to a string in the
*        base 256 notation ``d.d.d.d'' described below.
*
*        All Internet address are returned in  network  order  (bytes
*        ordered  from left to right).  All network numbers and local
*        address parts are returned as machine format integer values.
*
*   INTERNET ADDRESSES
*        Values specified using the `.'  notation  take  one  of  the
*
*        following forms:
*
*             a.b.c.d
*             a.b.c
*             a.b
*             a
*
*        When four parts are specified, each is interpreted as a byte
*        of data and assigned, from left to right, to  the four bytes
*        of  an Internet address.  Note: when  an Internet address is
*        viewed  as  a  32-bit  integer  quantity  on  little  endian
*        systems,  the  bytes referred to  above appear  as  d.c.b.a.
*        bytes are ordered from right to left.
*
*        When a three part address is specified,  the  last  part  is
*        interpreted  as  a  16-bit  quantity and placed in the right
*        most two bytes of the network address.  This makes the three
*        part  address  format convenient for specifying Class B net-
*        work addresses as "128.net.host".
*
*        When a two part address is supplied, the last part is inter-
*        preted  as  a  24-bit  quantity and placed in the right most
*        three bytes of the network address.  This makes the two part
*        address  format  convenient  for  specifying Class A network
*        addresses as "net.host".
*
*        When only one part is given, the value is stored directly in
*        the network address without any byte rearrangement.
*
*        All numbers supplied as ``parts'' in a `.' notation  may  be
*        decimal,  octal,  or  hexadecimal,  as  specified  in  the C
*        language (that is, a leading 0x or 0X  implies  hexadecimal;
*        otherwise,  a leading 0 implies octal; otherwise, the number
*        is interpreted as decimal).
*
*   RETURN VALUE
*        The value -1 is returned by inet_addr()  and  inet_network()
*        for malformed requests.
*
*   BUGS
*        The problem of host byte ordering versus network byte order-
*        ing  is  confusing.  A simple way to specify Class C network
*        addresses in a manner similar to that for Class B and  Class
*        A is needed.
*
*        The return value from inet_ntoa() points to static buffer
*        which  is  overwritten  in  each inet_ntoa() call.
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/Inet_LnaOf ****************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/inet_lnaof ****************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/inet_MakeAddr *************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/inet_makeaddr *************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/Inet_NetOf ****************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/inet_netof ****************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/inet_network **************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/Inet_NtoA *****************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/inet_ntoa *****************************************
*
*   SEE ALSO
*       inet_addr()
*
*****************************************************************************
*
*/
