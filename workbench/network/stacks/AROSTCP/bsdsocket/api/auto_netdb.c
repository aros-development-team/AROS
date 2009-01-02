#define NOTEX
/****** bsdsocket.library/gethostbyaddr *************************************
*
*   SEE ALSO
*        gethostbyname()
*
*****************************************************************************
*
*/

/****** bsdsocket.library/gethostbyname *************************************
*
*   NAME
*        gethostbyname, gethostbyaddr  - get network host entry
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*        #include <netdb.h>
*
*        hostent = gethostbyname(name)
*        D0                      A0
*
*        struct hostent *gethostbyname(char *);
*
*        hostent = gethostbyaddr(addr, len, type)
*        D0                      A0    D0   D1
*
*        struct hostent *gethostbyaddr(caddr_t, LONG, LONG);
*
*
*   DESCRIPTION 
*        gethostbyname() and gethostbyaddr() both return a pointer
*        to an object with the following structure containing the
*        data received from a name server or the broken-out fields
*        of a line in netdb configuration file.  In the case of
*        gethostbyaddr(), addr is a pointer to the binary format
*        address of length len (not a character string) and type is
*        an address family as defined in <sys/socket.h>. 
*
*          struct hostent {
*            char *h_name;       \* official name of host *\
*            char **h_aliases;   \* alias list *\
*            int  h_addrtype;    \* address type *\
*            int  h_length;      \* length of address *\
*            char **h_addr_list; \* list of addresses from name server *\
*          };
*
*        The members of this structure are:
*
*        h_name              Official name of the host.
*
*        h_aliases           A zero  terminated  array  of  alternate
*                            names for the host.
*
*        h_addrtype          The  type  of  address  being  returned;
*                            currently always AF_INET.
*
*        h_length            The length, in bytes, of the address.
*
*        h_addr_list         A pointer to a list of network addresses
*                            for  the named host.  Host addresses are
*                            returned in network byte order.
*
*   DIAGNOSTICS
*        A NULL pointer is returned if no matching entry was found or 
*        error occured.
*
*   BUGS
*        All information is contained in a static area so it must  be
*        copied if it is to be saved.  Only the Internet address for-
*        mat is currently understood.
*
*   SEE ALSO
*        AmiTCP/IP configuration
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/getnetbyaddr **************************************
*
*   SEE ALSO
*        getnetbyname()
*
*****************************************************************************
*
*/

/****** bsdsocket.library/getnetbyname **************************************
*
*   NAME
*        getnetbyname, getnetbyaddr - get network entry
*
*   SYNOPSIS
*        #include <netdb.h>
*
*        netent = getnetbyname(name)
*        D0                    A0
*
*        struct netent *getnetbyname(char *);
*
*        netent = getnetbyaddr(net, type)
*        D0                    D0   D1
*
*        struct netent *getnetbyaddr(long, long);
*
*   DESCRIPTION
*        getnetbyname(), and getnetbyaddr() both return  a  pointer to
*        an  object  with  the  following  structure  containing   the
*        broken-out fields of a line in netdb configuration file.
*
*          struct netent {
*            char *n_name;       \* official name of net *\
*            char **n_aliases;   \* alias list *\
*            int  n_addrtype;    \* net number type *\
*            long n_net;         \* net number *\
*          };
*
*        The members of this structure are:
*
*        n_name              The official name of the network.
*
*        n_aliases           A  zero  terminated  list  of  alternate
*                            names for the network.
*
*        n_addrtype          The type of the network number returned;
*                            currently only AF_INET.
*
*        n_net               The network number.  Network numbers are
*                            returned in machine byte order.
*
*        Network numbers are supplied in host order.
*
*        Type specifies the address type to use, currently only
*        AF_INET is supported.
*
*   DIAGNOSTICS
*        A NULL pointer is returned if no matching entry was found or 
*        error occured.
*
*   BUGS
*        All information is contained in a static area so it must  be
*        copied if it is to be saved.
*
*        Only Internet network numbers are currently understood.
*
*   SEE ALSO
*        AmiTCP/IP configuration
*
*****************************************************************************
*
*/

/****** bsdsocket.library/getprotobyname ************************************
*
*   NAME
*        getprotobyname, getprotobynumber - get protocol entry
*
*   SYNOPSIS
*        #include <netdb.h>
*
*        protoent = getprotobyname(name)
*        D0                        A0
*
*        struct protoent *getprotobyname(char *);
*
*        protoent = getprotobynumber(proto)
*        D0                          D0
*
*        struct protoent *getprotobynumber(long);
*
*   DESCRIPTION
*        getprotobyname() and getprotobynumber() both return a pointer
*        to  an  object with the  following structure  containing  the
*        broken-out fields of a line in netdb configuration file
*
*          struct    protoent {
*            char *p_name;       \* official name of protocol *\
*            char **p_aliases;   \* alias list *\
*            int  p_proto;       \* protocol number *\
*         };
*
*        The members of this structure are:
*
*        p_name              The official name of the protocol.
*        p_aliases           A  zero  terminated  list  of  alternate
*                            names for the protocol.
*        p_proto             The protocol number.
*
*
*   DIAGNOSTICS
*        A NULL pointer is returned if no matching entry was found or 
*        error occured.
*
*   BUGS
*        All information is contained in a static area so it must  be
*        copied  if  it  is to be saved.  Only the Internet protocols
*        are currently understood.
*
*   SEE ALSO
*        AmiTCP/IP configuration
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/getprotobynumber **********************************
*
*   SEE ALSO
*        getprotobyname()
*
*****************************************************************************
*
*/

/****** bsdsocket.library/getservbyname *************************************
*
*   NAME
*        getservbyname, getservbyport - get service entry
*
*   SYNOPSIS
*        #include <netdb.h>
*
*        servent = getservbyname(name, proto)
*        D0                      A0    A1
*
*        struct servent *getservbyname(char *, char *)
*
*        servent = getservbyport(port, proto)
*        D0                      D0    A0
*
*        struct servent *getservbyport(long, char *);
*
*   DESCRIPTION
*        getservbyname() and getservbyport() both return a pointer  to
*        an   object  with  the  following  structure  containing  the
*        broken-out fields of a line in netdb configuration file.
*
*          struct    servent {
*            char *s_name;       \* official name of service *\
*            char **s_aliases;   \* alias list *\
*            int  s_port;        \* port service resides at *\
*            char *s_proto;      \* protocol to use *\
*          };
*
*        The members of this structure are:
*             s_name              The official name of the service.
*             s_aliases           A zero terminated list of alternate
*                                 names for the service.
*             s_port              The port number at which  the  ser-
*                                 vice  resides.   Port  numbers  are
*                                 returned  in  network  short   byte
*                                 order.
*             s_proto             The name of  the  protocol  to  use
*                                 when contacting the service.
*
*        The proto argument specifies the protocol for which to the
*        sercive is to use. It is a normal C string, e.g. "tcp" or
*        "udp".
*
*   DIAGNOSTICS
*        A NULL pointer is returned if no matching entry was found or 
*        error occured.
*
*   BUGS
*        All information is contained in a static area so it must  be
*        copied  if it is to be saved.  Expecting port numbers to fit
*        in a 32 bit quantity is probably naive.
*
*   SEE ALSO
*        AmiTCP/IP configuration
*
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/getservbyport *************************************
*
*   SEE ALSO
*        getservbyname()
*
*****************************************************************************
*
*/
