/****** bsdsocket.library/accept ********************************************
*
*   NAME
*        accept - accept a connection on a socket
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*
*        ns = accept(s, addr, addrlen)
*        D0          D0 A0    A1
*
*        long accept(long, struct sockaddr *, long *);
*
*   FUNCTION
*        The  argument  s  is a  socket  that  has  been created with
*        socket(), bound to an  address  with bind(), and  is listen-
*        ing for connections after a listen().  accept() extracts the
*        first  connection  on  the  queue  of  pending  connections,
*        creates a new socket with the same properties of s and allo-
*        cates a new socket descriptor for the socket.  If no pending
*        connections are present on the  queue, and the socket is not
*        marked  as non-blocking, accept() blocks the caller  until a
*        connection is present.  If the socket is marked non-blocking
*        and  no  pending  connections  are  present  on  the  queue,
*        accept() returns  an error as described below.  The accepted
*        socket is used to read and write data to and from the socket
*        which connected to  this one; it is not used to  accept more
*        connections.  The original socket s remains open for accept-
*        ing further connections.
*
*        The argument addr is a result parameter that  is  filled  in
*        with  the  address of the connecting entity, as known to the
*        communications layer.  The exact format of the addr  parame-
*        ter  is  determined by the domain in which the communication
*        is occurring.  The addrlen is a value-result  parameter;  it
*        should  initially  contain the amount of space pointed to by
*        addr; on return it will contain the actual length (in bytes)
*        of   the   address   returned.    This  call  is  used  with
*        connection-based socket types, currently with SOCK_STREAM.
*
*        It is possible to select() a socket  for  the  purposes  of
*        doing an accept() by selecting it for read.
*
*   RETURN VALUES
*        accept() returns a non-negative descriptor for the  accepted
*        socket on success.  On failure, it returns -1 and sets errno
*        to indicate the error.
*
*   ERRORS
*        EBADF        - The descriptor is invalid.
*
*        EINTR        - The operation was interrupted by a break 
*                       signal.
* 
*        EOPNOTSUPP   - The referenced socket is not of type
*                       SOCK_STREAM.
*
*        EWOULDBLOCK  - The socket is marked non-blocking and no con-
*                       nections are present to be accepted.
*
*   SEE ALSO
*        bind(), connect(), listen(), select(), SetSocketSignals(),
*        socket()
*****************************************************************************
*
*/

/****** bsdsocket.library/bind **********************************************
*
*   NAME
*        bind - bind a name to a socket
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*
*        success = bind(s, name, namelen)
*        D0             D0 A0    D1
*
*        long bind(long, struct sockaddr *, long);
*
*   FUNCTION
*        bind() assigns a name to an unnamed socket.  When  a  socket
*        is created with socket(2) it exists in a name space (address
*        family) but has no name assigned.  bind() requests that  the
*        name pointed to by name be assigned to the socket.
*
*   RETURN VALUES
*        0  - on success.
*
*        -1 - on failure and sets errno to indicate the error.
*
*   ERRORS
*        EACCES            - The requested address is protected,  and
*                            the  current user has inadequate permis-
*                            sion to access it.
*
*        EADDRINUSE        - The specified address is already in use.
*
*        EADDRNOTAVAIL     - The specified address is  not  available
*                            from the local machine.
*
*        EBADF             - s is not a valid descriptor.
*
*        EINVAL            - namelen is  not  the  size  of  a  valid
*                            address  for  the specified address fam-
*                            ily.
*
*                            The  socket  is  already  bound  to   an
*                            address.
*
*   SEE ALSO
*        connect(), getsockname(), listen(), socket()
*
*   NOTES
*        The rules used in name binding  vary  between  communication
*        domains.
*****************************************************************************
*
*/


/****** bsdsocket.library/CloseSocket ***************************************
*
*   NAME
*        CloseSocket - delete a socket descriptor
*
*   SYNOPSIS
*        success = CloseSocket(s)
*        D0                    D0
*
*        long CloseSocket(long);
*
*   FUNCTION 
*        CloseSocket() deletes  a  descriptor  from the  library base
*        socket reference table.   If s is the last reference  to the
*        underlying object, then the object  will  be deactivated and
*        socket  (see socket()),  associated naming  information  and
*        queued data are discarded.
*
*        All sockets are automatically closed when the socket library
*        is closed, but closing sockets as soon as possible is
*        recommended to save system resources.
*
*   RETURN VALUES
*         0   on success.
*
*        -1   on failure and sets errno to indicate the error.
*
*   ERRORS
*        EBADF             - s is not an active socket descriptor.
*
*        EINTR             - linger on close was interrupted.
*                            The socket is closed, however.
*
*   SEE ALSO
*        accept(), SocketBaseTagList(), shutdown(), socket(),
*        exec.library/CloseLibrary()
*****************************************************************************
*
*/



/****** bsdsocket.library/connect *******************************************
*
*   NAME
*        connect - initiate a connection on a socket
*   
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*   
*        success = connect(s, name, namelen)
*        D0                D0 A0    D1
*   
*        long connect(long, struct sockaddr *, long);
*   
*   FUNCTION
*        The parameter s is a socket.  If it  is of  type SOCK_DGRAM,
*        then  this call specifies the peer with which the  socket is
*        to be associated;  this address  is that  to which datagrams
*        are  to be sent, and  the only address from which  datagrams
*        are to be received.  If it is of type SOCK_STREAM, then this
*        call attempts  to make a connection  to another socket.  The
*        other socket is specified by name which is an address in the
*        communications space of  the  socket.   Each  communications
*        space interprets the  name parameter in  its  own way.  Gen-
*        erally, stream sockets may successfully connect() only once;
*        datagram sockets may use connect() multiple  times to change
*        their association.  Datagram sockets may dissolve the  asso-
*        ciation by connecting to  an invalid address, such as a null
*        address.
*   
*   RETURN VALUES
*         0   on success.
*   
*        -1   on failure and sets errno to indicate the error.
*   
*   ERRORS
*        EADDRINUSE        - The address is already in use.
*   
*        EADDRNOTAVAIL     - The specified address is  not  available
*                            on the remote machine.
*   
*        EAFNOSUPPORT      - Addresses in the specified address  fam-
*                            ily cannot be used with this socket.
*   
*        EALREADY          - The socket is non-blocking and a  previ-
*                            ous  connection attempt has not yet been
*                            completed.
*   
*        EBADF             - s is not a valid descriptor.
*   
*        ECONNREFUSED      - The attempt to  connect  was  forcefully
*                            rejected.   The  calling  program should
*                            CloseSocket() the socket descriptor, and
*                            issue another socket()  call to obtain a
*                            new descriptor before attempting another
*                            connect() call.
*   
*        EINPROGRESS       - The socket is non-blocking and the  con-
*                            nection cannot be completed immediately.
*                            It is possible to select()  for  comple-
*                            tion  by  selecting the socket for writ-
*                            ing.
*   
*        EINTR             - The operation was interrupted by a break 
*                            signal.
* 
*        EINVAL            - namelen is  not  the  size  of  a  valid
*                            address  for  the specified address fam-
*                            ily.
*   
*        EISCONN             The socket is already connected.
*   
*        ENETUNREACH       - The network is not reachable  from  this
*                            host.
*   
*        ETIMEDOUT         - Connection   establishment   timed   out
*                            without establishing a connection.
*        
*   SEE ALSO
*        accept(), CloseSocket(), connect(), getsockname(), select(),
*        socket()
*****************************************************************************
*
*/


/****** bsdsocket.library/Dup2Socket *****************************************
*
*   NAME
*       Dup2Socket - duplicate a socket descriptor
*
*   SYNOPSIS
*
*       newfd = Dup2Socket(fd1, fd2)
*       D0                 D0   D1
*
*       long Dup2Socket(long, long);
*
*   DESCRIPTION
*       Dup2Socket() duplicates an existing socket descriptor. 
*       the argument fd1 is small non-negative value that indexes
*       the socket on SocketBase descriptor table. The value must
*       be less than the size of the table, which is returned by
*       getdtablesize(). fd2 specifies the desired value of the new
*       descriptor. If descriptor fd2 is already in use, it is 
*       first deallocated as if it were closed by CloseSocket(). If
*       the value if fd2 is -1, the new descriptor used and returned
*       is the lowest numbered descriptor that is not currently in 
*       use by the SocketBase.
*
*       Dup2Socket() has also an feature to mark a file descriptor as
*       being used. If fd1 is given as -1, fd2 is marked as being used
*       socket descriptor table and it won't be allocated for any
*       socket. This mark can be removed using CloseSocket() call.
*
*   RETURN VALUES
*       Dup2Socket() returns a new descriptor on  success. On failure
*       -1 is returned and errno is set to indicate the error.
*
*   ERRORS
*       EBADF          fd1 or fd2 is not a valid active descriptor.
*
*       EMFILE         Too many descriptors are active.
*
*   SEE ALSO
*       accept(), CloseSocket(), getdtablesize(), SocketBaseTagList(),
*       socket()
*
*****************************************************************************
*
*/


/****** bsdsocket.library/getpeername ***************************************
*
*   NAME
*        getpeername - get name of connected peer
*
*   SYNOPSIS
*        success =  getpeername(s, name, namelen)
*        D0                     D0 A0    A1
*
*        long getpeername(long, struct sockaddr *, long *);
*
*   FUNCTION
*        getpeername() returns the name  of  the  peer  connected  to
*        socket  s.   The long  pointed  to  by the namelen parameter
*        should be  initialized  to  indicate  the  amount  of  space
*        pointed  to  by name.  On return it contains the actual size
*        of the name returned (in bytes).  The name is  truncated  if
*        the buffer provided is too small.
*
*   RETURN VALUE
*        A 0 is returned if the call succeeds, -1 if it fails.
*
*   ERRORS
*        EBADF        - The argument s is not a valid descriptor.
*
*        ENOBUFS      - Insufficient resources were available in  the
*                       system to perform the operation.
*
*        ENOTCONN     - The socket is not connected.
*
*   SEE ALSO
*        accept(), bind(), getsockname(), socket()
*****************************************************************************
*
*/


/****** bsdsocket.library/getsockname ***************************************
*
*   NAME
*        getsockname - get socket name
*
*   SYNOPSIS
*
*        success = getsockname(s, name, namelen)
*        D0                    D0 A0    A1
*
*        long getsockname(long, struct sockaddr *, long *);
*
*   FUNCTION
*        getsockname() returns the current  name  for  the  specified
*        socket.   The  namelen  parameter  should  be initialized to
*        indicate the amount of space pointed to by name.  On  return
*        it contains the actual size of the name returned (in bytes).
*
*   DIAGNOSTICS
*        A 0 is returned if the call succeeds, -1 if it fails.
*
*   ERRORS
*        The call succeeds unless:
*
*        EBADF          s is not a valid descriptor.
*
*        ENOBUFS        Insufficient resources were available in  the
*                       system to perform the operation.
*
*   SEE ALSO
*        bind(), getpeername(), socket()
*****************************************************************************
*
*/


/****** bsdsocket.library/getsockopt ****************************************
*
*   NAME
*        getsockopt, setsockopt - get and set options on sockets
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*
*        success =  getsockopt(s, level, optname, optval, optlen)
*        D0                    D0 D1     D2       A0      A1
*        
*        long getsockopt(long, long, long, caddr_t, long *);
*
*        success =  setsockopt(s, level, optname, optval, optlen)
*        D0                    D0 D1     D2       A0      D3
*        
*        long setsockopt(long, long, long, caddr_t, long);
*
*   FUNCTION
*        getsockopt() and setsockopt() manipulate options  associated
*        with  a socket.  Options may exist at multiple protocol lev-
*        els; they are always present  at  the  uppermost  ``socket''
*        level.
*
*        When manipulating socket options  the  level  at  which  the
*        option resides and the name of the option must be specified.
*        To manipulate options at  the  ``socket''  level,  level  is
*        specified as SOL_SOCKET.  To manipulate options at any other
*        level the protocol number of the appropriate  protocol  con-
*        trolling  the  option is supplied.  For example, to indicate
*        that an option is to be interpreted  by  the  TCP  protocol,
*        level  should  be  set  to  the  protocol number of TCP.
*
*        The parameters optval and optlen are used to  access  option
*        values  for  setsockopt().  For getsockopt() they identify a
*        buffer in which the value for the requested option(s) are to
*        be  returned.   For  getsockopt(),  optlen is a value-result
*        parameter, initially  containing  the  size  of  the  buffer
*        pointed to by optval, and modified on return to indicate the
*        actual size of the value returned.  If no option value is to
*        be supplied or returned, optval may be supplied as 0.
*
*        optname and any specified options are  passed  uninterpreted
*        to  the appropriate protocol module for interpretation.  The
*        include  file  <sys/socket.h>   contains   definitions   for
*        ``socket'' level options, described below.  Options at other
*        protocol  levels  vary  in  format  and  name.
*
*        Most socket-level options take an int parameter for  optval.
*        For setsockopt(), the parameter should be non-zero to enable
*        a boolean option, or zero if the option is to  be  disabled.
*
*        SO_LINGER   uses  a  struct  linger  parameter,  defined  in
*        <sys/socket.h>, which specifies the  desired  state  of  the
*        option and the linger interval (see below).
*
*        The following options are recognized at  the  socket  level.
*        Except  as noted, each may be examined with getsockopt() and
*        set with setsockopt().
*
*             SO_DEBUG          - toggle   recording   of   debugging
*                                 information
*             SO_REUSEADDR      - toggle local address reuse
*             SO_KEEPALIVE      - toggle keep connections alive
*             SO_DONTROUTE      - toggle routing bypass for  outgoing
*                                 messages
*             SO_LINGER         - linger on close if data present
*             SO_BROADCAST      - toggle   permission   to   transmit
*                                 broadcast messages
*             SO_OOBINLINE      - toggle  reception  of   out-of-band
*                                 data in band
*             SO_SNDBUF         - set buffer size for output
*             SO_RCVBUF         - set buffer size for input
*             SO_TYPE           - get the type  of  the  socket  (get
*                                 only)
*             SO_ERROR          - get and clear error on  the  socket
*                                 (get only)
*
*        SO_DEBUG  enables  debugging  in  the  underlying   protocol
*        modules.   SO_REUSEADDR  indicates  that  the  rules used in
*        validating addresses supplied in a bind() call should allow
*        reuse of local addresses.  SO_KEEPALIVE enables the periodic
*        transmission of messages on a connected socket.  Should  the
*        connected  party fail to respond to these messages, the con-
*        nection is considered broken. If  the  process  is
*        waiting in select() when the connection is broken, select()
*        returns true for any read or write events selected  for  the
*        socket.    SO_DONTROUTE  indicates  that  outgoing  messages
*        should bypass the  standard  routing  facilities.   Instead,
*        messages  are  directed to the appropriate network interface
*        according to the network portion of the destination address.
*
*        SO_LINGER controls the action taken when unsent messags  are
*        queued  on  socket and a CloseSocket() is performed.  If the
*        socket promises reliable delivery of data and  SO_LINGER  is
*        set,  the  system  will  block  the  process  on the close
*        attempt until it is able to transmit the data  or  until  it
*        decides  it  is unable to deliver the information (a timeout
*        period, in seconds, termed the linger interval, is specified
*        in the set- sockopt() call when SO_LINGER is requested).  If
*        SO_LINGER  is  disabled and a CloseSocket()  is  issued, the
*        system will process the  close  in  a manner that allows the
*        process to continue as quickly as possible.
*
*        The option SO_BROADCAST requests permission to  send  broad-
*        cast  datagrams  on  the socket.  Broadcast was a privileged
*        operation in earlier versions of the system.  With protocols
*        that  support  out-of-band  data,  the  SO_OOBINLINE  option
*        requests that out-of-band data be placed in the normal  data
*        input  queue  as  received;  it will then be accessible with
*        recv() or read() calls without the MSG_OOB flag.   SO_SNDBUF
*        and  SO_RCVBUF are options to adjust the normal buffer sizes
*        allocated for output and input buffers,  respectively.   The
*        buffer size may be increased for high-volume connections, or
*        may be decreased to limit the possible backlog  of  incoming
*        data.   The system places an absolute limit on these values.
*        Finally, SO_TYPE and SO_ERROR are  options  used  only  with
*        getsockopt().   SO_TYPE returns the type of the socket, such
*        as SOCK_STREAM; it is useful for servers that inherit  sock-
*        ets  on  startup.  SO_ERROR returns any pending error on the
*        socket and clears the error status.  It may be used to check
*        for asynchronous errors on connected datagram sockets or for
*        other asynchronous errors.
*
*   RETURN VALUES
*         0 - on success.
*
*        -1 - on failure and set errno to indicate the error.
*
*   ERRORS
*        EBADF             - s is not a valid descriptor.
*
*        ENOPROTOOPT       - The option is unknown at the level indi-
*                            cated.
*
*   SEE ALSO
*        IoctlSocket(), socket()
*
*   BUGS
*        Several of the socket options should  be  handled  at  lower
*        levels of the system.
*****************************************************************************
*
*/


/****** bsdsocket.library/IoctlSocket ***************************************
*
*   NAME
*        IoctlSocket - control sockets
*
*   SYNOPSIS
*
*        #include <sys/types.h>
*        #include <sys/ioctl.h>
*
*        value = IoctlSocket(fd, request, arg)
*        D0            D0  D1       A0
*
*        long IoctlSocket(long, long, caddr_t);
*
*   FUNCTION
*        IoctlSocket() performs a special function on the object referred
*        to by the open  socket descriptor fd. Note: the setsockopt()
*        call (see getsockopt()) is the primary  method for operating
*        on sockets  as such, rather than on the underlying  protocol
*        or network interface.
*
*        For most IoctlSocket() functions, arg is a pointer to data to
*        be used by the  function  or to be filled in by the function.
*        Other functions may ignore arg or may treat it directly as a
*        data item; they may, for example, be passed an int value.
*
*        The following requests are supported:
*
*
*        FIOASYNC            The argument is a  pointer  to  a  long.
*                            Set  or  clear asynchronous I/O.  If the
*                            value of that  long is  a  1  (one)  the
*                            descriptor  is set for asynchronous I/O.
*                            If the value of that long is a  0 (zero)
*                            the  descriptor is cleared for asynchro-
*                            nous I/O.
*
*        FIOCLEX
*        FIONCLEX            Ignored, no use for close-on-exec flag
*                            in Amiga.
*
*        FIOGETOWN
*        SIOCGPGRP           The argument is pointer to struct Task*.
*                            Set the value of that pointer to the 
*                            Task that  is  receiving SIGIO or SIGURG
*                            signals for  the  socket  referred to by
*                            the descriptor passed to IoctlSocket().
*
*        FIONBIO             The argument is a  pointer  to  a  long.
*                            Set  or  clear non-blocking I/O.  If the
*                            value of that  long is  a  1  (one)  the
*                            descriptor  is set for non-blocking I/O.
*                            If the value of that long is a  0 (zero)
*                            the   descriptor  is  cleared  for  non-
*                            blocking I/O.
*
*        FIONREAD            The argument is a  pointer  to  a  long.
*                            Set the value of that long to the number
*                            of immediately readable characters  from
*                            the socket fd.
*
*        FIOSETOWN
*        SIOCSPGRP           The argument is pointer to struct Task*,
*                            pointer  to  the task  that will subseq-
*                            uently receive  SIGIO or  SIGURG signals
*                            for   the  socket  referred  to  by  the
*                            descriptor passed.
*
*        SIOCCATMARK         The argument is a pointer to a long.
*                            Set the value of that long to 1 if the
*                            read pointer for the socket referred to
*                            by the descriptor passed to
*                            IoctlSocket() points to a mark in the
*                            data stream for an out-of-band message,
*                            and to 0 if it does not point to a mark.
*
*
*   RETURN VALUES
*        IoctlSocket() returns 0 on success for most requests.   Some 
*        specialized requests may return non-zero values on success; On  
*        failure,  IoctlSocket() returns -1 and sets errno to indicate
*        the error.
*
*   ERRORS
*        EBADF          fd is not a valid descriptor.
*
*        EINVAL         request or arg is not valid.
*
*        IoctlSocket() will also fail if the object on which the function
*        is  being performed detects an error. In this case, an error
*        code specific  to  the  object  and  the  function  will  be
*        returned.
*
*   SEE ALSO
*        getsockopt(), SocketBaseTagList(), setsockopt()
*****************************************************************************
*
*/


/****** bsdsocket.library/listen ********************************************
*   NAME
*        listen - listen for connections on a socket
*
*   SYNOPSIS
*        success = listen(s, backlog)
*        D0               D0 D1
*
*        long listen(long, long);
*
*   FUNCTION
*        To accept  connections,  a  socket  is  first  created  with
*        socket(),  a  backlog for incoming connections is specified
*        with listen() and then the  connections  are  accepted  with
*        accept().   The  listen()  call  applies only to socket of
*        type SOCK_STREAM.
*
*        The backlog parameter defines the maximum length  the  queue
*        of pending connections may grow to.  If a connection request
*        arrives with the queue full the client will receive an error
*        with an indication of ECONNREFUSED.
*
*   RETURN VALUES
*         0    on success.
*
*        -1    on failure and sets errno to indicate the error.
*
*   ERRORS
*        EBADF             - s is not a valid descriptor.
*
*        EOPNOTSUPP        - The socket is not of a  type  that  sup-
*                            ports listen().
*
*   SEE ALSO
*        accept(), connect(), socket()
*
*   BUGS
*        The backlog is currently limited (silently) to 5.
*****************************************************************************
*
*/


/****** bsdsocket.library/recv **********************************************
*   NAME
*        recv, recvfrom, - receive a message from a socket
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*
*       nbytes = recv(s, buf, len, flags)
*       D0            D0 A0   D1   D2
*
*       long recv(long, char *, long, long);
*
*       nbytes = recvfrom(s, buf, len, flags, from, fromlen)
*       D0                D0 A0   D1   D2     A1    A2
*
*       long recvfrom(long, char *, long, long, 
*                        struct sockaddr *, long *);
*
*   FUNCTION
*        s is a socket created with socket().  recv() and recvfrom(),
*        are used  to  receive messages from  another socket.  recv()
*        may  be  used  only on a connected socket  (see  connect()),
*        while  recvfrom() may be used  to receive data  on a  socket
*        whether it is in a connected state or not.
*
*        If from is not a NULL pointer, the  source  address  of  the
*        message  is filled in.  fromlen is a value-result parameter,
*        initialized to the size of the buffer associated with  from,
*        and  modified  on  return to indicate the actual size of the
*        address  stored  there.   The  length  of  the  message   is
*        returned.   If  a message is too long to fit in the supplied
*        buffer, excess bytes may be discarded depending on the  type
*        of socket the message is received from (see socket()).
*
*        If no messages are available at the socket, the receive call
*        waits  for  a  message  to arrive, unless the socket is non-
*        blocking (see IoctlSocket()) in which case -1  is  returned
*        with the external variable errno set to EWOULDBLOCK.
*
*        The select() call may be used to determine when  more  data
*        arrive.
*
*        The flags parameter is formed by ORing one or  more  of  the
*        following:
*
*        MSG_OOB      - Read any "out-of-band" data  present  on  the
*                       socket,  rather  than  the  regular "in-band"
*                       data.
*
*        MSG_PEEK     - "Peek" at the data present on the socket; the
*                       data  are returned, but not consumed, so that
*                       a subsequent receive operation will  see  the
*                       same data.
*
*   RETURN VALUES
*        These calls return the number of bytes received, or -1 if an
*        error occurred.
*
*   ERRORS
*        EBADF             - s is an invalid descriptor.
*
*        EINTR             - The operation was interrupted by a break 
*                            signal.
* 
*        EWOULDBLOCK       - The socket is  marked  non-blocking  and
*                            the requested operation would block.
*
*   SEE ALSO
*        connect(), getsockopt(), IoctlSocket(), select(), send(),
*        SocketBaseTagList(), socket()
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/recvfrom ******************************************
*
*   SEE ALSO
*        recv()
*****************************************************************************
*
*/


/****** bsdsocket.library/select ********************************************
*
*   NAME
*        select -- synchronous I/O multiplexing (stub/inline function)
*        WaitSelect -- select() with Amiga Wait() function.
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/time.h>
*
*        n = select (nfds, readfds, writefds, exceptfds, timeout)
*
*        long select(long, fd_set *, fd_set *, fd_set *, 
*                    struct timeval *);
*
*        n = WaitSelect (nfds, readfds, writefds, exceptfds, timeout,
*        D0              D0    A0       A1        A2         A3
*                    sigmp)
*                    D1
*
*        long WaitSelect(long, fd_set *, fd_set *, fd_set *, 
*                        struct timeval *, long *);
*
*        FD_SET (fd, &fdset)
*        FD_CLR (fd, &fdset)
*        FD_ISSET (fd, &fdset)
*        FD_ZERO (&fdset)
*        long fd;
*        fd_set fdset;
*
*   DESCRIPTION
*        select() examines the socket descriptor sets whose addresses
*        are passed in readfds,  writefds,  and exceptfds  to  see if
*        some of their descriptors are ready  for reading,  ready for
*        writing,  or have an exceptional condition pending.  nfds is
*        the  number  of bits to be checked in  each  bit  mask  that
*        represent a file descriptor; the descriptors from 0  through
*        (nfds - 1) in the descriptor sets  are examined.  On return,
*        select()  replaces  the  given descriptor sets  with subsets
*        consisting of  those descriptors  that  are  ready  for  the
*        requested operation.  The total number  of ready descriptors
*        in all the sets is returned.
*
*        WaitSelect() also takes a signal mask which is waited during
*        normal select() operation. If one of these singals is recei-
*        ved,  WaitSelect() returns  and has  re-set  the signal mask
*        to return those signals that  have arrived.  Normal select()
*        return values are returned.
*
*        The descriptor sets are stored as bit fields  in  arrays  of
*        integers.   The following macros are provided for manipulat-
*        ing such descriptor sets:  FD_ZERO  (&fdset)  initializes  a
*        descriptor  set  fdset to the null set.  FD_SET(fd, &fdset )
*        includes a particular descriptor fd  in  fdset.   FD_CLR(fd,
*        &fdset)  removes  fd  from  fdset.   FD_ISSET(fd, &fdset) is
*        nonzero if fd is a member of  fdset,  zero  otherwise.   The
*        behavior  of these macros is undefined if a descriptor value
*        is less than zero or greater than or  equal  to  FD_SETSIZE,
*        which  is  normally  at least equal to the maximum number of
*        descriptors supported by the system.
*
*        If timeout is not a NULL pointer,  it  specifies  a  maximum
*        interval  to wait for the selection to complete.  If timeout
*        is a NULL  pointer,  the  select  blocks  indefinitely.   To
*        effect  a  poll,  the  timeout argument should be a non-NULL
*        pointer, pointing to a zero-valued timeval structure.
*
*        Any of readfds, writefds, and exceptfds may be given as NULL
*        pointers if no descriptors are of interest.
*
*        Selecting true for reading on a socket descriptor upon which
*        a  listen() call has been performed indicates that a subse-
*        quent accept() call on that descriptor will not block.
*
*   RETURN VALUES
*        select() returns a non-negative value on success. A positive
*        value indicates the number of ready descriptors in the
*        descriptor sets. 0 indicates that the time limit referred to
*        by timeout expired or that the operation was interrupted
*        either by a break signal or by arrival of a signal specified
*        in *sigmp. On failure, select() returns -1, sets errno to
*        indicate the error, and the descriptor sets are not changed.
*
*   ERRORS
*        EBADF        - One  of  the  descriptor  sets  specified  an
*                       invalid descriptor.
*
*        EINTR        - one of the signals in SIGINTR  mask (see Set-
*                       SocketSignals())   is  set  and  it  was  not
*                       requested in WaitSelect() call.
*
*        EINVAL       - A component of the pointed-to time  limit  is
*                       outside  the  acceptable range: t_sec must be
*                       between 0 and 10^8, inclusive. t_usec must be
*                       greater  than  or  equal  to 0, and less than
*                       10^6.
*
*   SEE ALSO
*        accept(),  connect(), getdtablesize(), listen(), recv(),
*        send(), SetDTableSize(), SetSocketSignals()
*
*   NOTES
*        Under rare  circumstances,  select()  may  indicate  that  a
*        descriptor  is  ready for writing when in fact an attempt to
*        write would block.  This  can  happen  if  system  resources
*        necessary  for  a  write are exhausted or otherwise unavail-
*        able.  If an application deems it critical that writes to  a
*        file  descriptor not block, it should set the descriptor for
*        non-blocking I/O using the FIOASYNC request to IoctlSocket().
*
*        Default   system   limit  for  open  socket  descriptors  is
*        currently  64. However,  in  order  to accommodate  programs
*        which might  potentially  use  a larger number of open files
*        with select, it is possible  to  increase this size within a
*        program  by  providing  a  larger definition  of  FD_SETSIZE
*        before    the   inclusion    of   <sys/types.h>    and   use
*        SocketBaseTags(SBTM_SETVAL(SBTC_DTABLESIZE), FD_SETSIZE);
*        call directly after OpenLibrary().
*
*   BUGS
*        select() should probably return the time remaining from  the
*        original  timeout,  if  any,  by modifying the time value in
*        place.  This may be implemented in future  versions  of  the
*        system.   Thus,  it  is  unwise  to  assume that the timeout
*        pointer will be unmodified by the select() call.
*****************************************************************************
*
*/


/****** bsdsocket.library/send **********************************************
*
*   NAME
*        send, sendto - send a message from a socket
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*
*        nbytes = send(s, msg, len, flags)
*        D0            D0 A0   D1   D2
*
*        int send(int, char *, int, int);
*
*        nbytes = sendto(s, msg, len, flags, to, tolen)
*        D0              D0 A0   D1   D2     A1  D3
*
*        int send(int, char *, int, int, struct sockaddr *, int);
*
*   FUNCTION
*        s is a socket created with socket().  send() and sendto() are
*        used  to transmit a message to  another socket. send() may be
*        used  only when the socket  is  in a connected  state,  while
*        sendto() may be used at any time.
*
*        The address of the target  is given by to with tolen specify-
*        ing its size.  The length of the message is given by len.  If
*        the  message is  too  long  to  pass  atomically  through the
*        underlying protocol, then the error EMSGSIZE is returned, and
*        the message is not transmitted.
*
*        No indication of failure to deliver is implicit  in a send().
*        Return values of -1 indicate some locally detected errors.
*
*        If no buffer space is  available at the socket  to  hold  the
*        message  to  be  transmitted,  then  send() normally  blocks,
*        unless the  socket  has been placed in non-blocking I/O mode.
*        The select() call may be used to determine when  it  is  pos-
*        sible to send more data.
*
*        The flags parameter is formed  by ORing  one  or  more of the
*        following:
*
*        MSG_OOB           - Send  ``out-of-band''  data  on  sockets
*                            that  support this notion.  The underly-
*                            ing protocol must  also  support  ``out-
*                            of-band''    data.     Currently,   only
*                            SOCK_STREAM  sockets  created   in   the
*                            AF_INET  address  family support out-of-
*                            band data.
*
*        MSG_DONTROUTE     - The SO_DONTROUTE option is turned on for
*                            the  duration of the operation.  This is
*                            usually used only by diagnostic or rout-
*                            ing programs.
*
*   RETURN VALUES
*        On success, these functions return the number of bytes sent.
*        On  failure,  they  return  -1 and set errno to indicate the
*        error.
*
*   ERRORS
*        EBADF             - s is an invalid descriptor.
*
*        EINTR             - The operation was interrupted by a break 
*                             signal.
* 
*        EINVAL            - len is not the size of a  valid  address
*                            for the specified address family.
*
*        EMSGSIZE          - The socket requires that message be sent
*                            atomically,  and the size of the message
*                            to be sent made this impossible.
*
*        ENOBUFS           - The system was  unable  to  allocate  an
*                            internal   buffer.   The  operation  may
*                            succeed when buffers become available.
*
*        ENOBUFS           - The output queue for a network interface
*                            was full.  This generally indicates that
*                            the interface has stopped  sending,  but
*                            may be caused by transient congestion.
*
*        EWOULDBLOCK       - The socket is  marked  non-blocking  and
*                            the requested operation would block.
*
*   SEE ALSO
*        connect(), getsockopt(), recv(), select(), socket()
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/sendto ********************************************
*
*   SEE ALSO
*        send()
*****************************************************************************
*
*/

#define NOTEX
/****** bsdsocket.library/setsockopt ****************************************
*
*   SEE ALSO
*        getsockopt()
*****************************************************************************
*
*/


/****** bsdsocket.library/shutdown ******************************************
*
*   NAME
*        shutdown - shut down part of a full-duplex connection
*
*   SYNOPSIS
*        success = shutdown(s, how)
*        D0                 D0 D1
*
*        long shutdown(long, long);
*
*   DESCRIPTION
*        The shutdown() call causes all or part of a full-duplex con-
*        nection on the socket associated with s to be shut down.  If
*        how is 0, then further receives will be disallowed.  If  how
*        is  1,  then further sends will be disallowed.  If how is 2,
*        then further sends and receives will be disallowed.
*
*   RETURN VALUES
*         0 - on success.
*
*        -1 - on failure and sets errno to indicate the error.
*
*   ERRORS
*        EBADF        - s is not a valid descriptor.
*
*        ENOTCONN     - The specified socket is not connected.
*
*   SEE ALSO
*        connect(), socket()
*
*   BUGS
*        The how values should be defined constants.
*****************************************************************************
*
*/


/****** bsdsocket.library/socket ********************************************
*
*   NAME
*        socket - create an endpoint for communication
*
*   SYNOPSIS
*        #include <sys/types.h>
*        #include <sys/socket.h>
*
*        s = socket(domain, type, protocol)
*        D0          D0     D1    D2
*
*        long socket(long, long, long);
*
*   FUNCTION
*        socket() creates an endpoint for communication and returns a
*        descriptor.
*
*        The  domain  parameter  specifies  a  communications  domain
*        within which communication will take place; this selects the
*        protocol  family  which should be used.  The protocol family
*        generally  is  the  same  as  the  address  family  for  the
*        addresses supplied in later operations on the socket.  These
*        families  are defined  in the  include file  <sys/socket.h>.
*        The currently understood formats are
*
*                PF_INET - (ARPA Internet protocols)
*
*        The socket has the indicated type, which specifies the
*        semantics of communication.  Currently defined types are:
*
*                SOCK_STREAM
*                SOCK_DGRAM
*                SOCK_RAW
*
*        A  SOCK_STREAM type  provides  sequenced,  reliable, two-way
*        connection  based   byte  streams.    An   out-of-band  data
*        transmission  mechanism  may  be  supported.   A  SOCK_DGRAM
*        socket supports datagrams  (connectionless, unreliable  mes-
*        sages  of  a  fixed   (typically   small)  maximum  length).
*        SOCK_RAW   sockets   provide  access  to   internal  network
*        interfaces.
*
*        The protocol specifies a particular protocol to be used with
*        the socket.  Normally  only a single protocol exists to sup-
*        port a particular socket type  within a given  protocol fam-
*        ily.  However, it is possible that many protocols may exist,
*        in which case a  particular protocol  must be  specified  in
*        this manner.  The  protocol number to use  is  particular to
*        the "communication domain" in which communication is to take
*        place.
*
*        Sockets of type SOCK_STREAM  are  full-duplex byte  streams,
*        similar to pipes.   A  stream socket must be in  a connected
*        state before any data may be sent or received on it.  A con-
*        nection  to another socket is created with a connect() call.
*        Once  connected, data  may be  transferred using send()  and
*        recv()  or their variant calls.   When  a  session  has been
*        completed a CloseSocket()  may  be  performed.   Out-of-band
*        data may also  be transmitted  as  described  in  send() and
*        received as described in recv().
*
*        The communications protocols used to implement a SOCK_STREAM
*        insure that data is  not lost or  duplicated.  If a piece of
*        data for  which the peer protocol has buffer space cannot be
*        successfully transmitted within a reasonable length of time,
*        then the  connection  is  considered broken  and  calls will
*        indicate an error with -1 returns and with ETIMEDOUT  as the
*        specific error code (see Errno()).  The protocols optionally
*        keep sockets "warm" by  forcing transmissions roughly  every
*        minute in the absence of other activity.
*
*        SOCK_DGRAM  and SOCK_RAW sockets allow sending of  datagrams
*        to  correspondents  named in send()  calls.   Datagrams  are
*        generally  received  with  recv(), which  returns  the  next
*        datagram with its return address.
*
*        The operation of  sockets  is  controlled  by  socket  level
*        options.   These  options  are defined in the file socket.h.
*        getsockopt() and  setsockopt()  are  used  to  get  and  set
*        options, respectively.
*
*   RETURN VALUES
*        socket() returns a non-negative descriptor on  success.   On
*        failure, it returns -1 and sets errno to indicate the error.
*
*   ERRORS
*        EACCES          - Permission to create  a  socket  of  the
*                          specified   type   and/or   protocol  is
*                          denied.
*
*        EMFILE          - The per-process descriptor table is
*                          full.
*
*        ENOBUFS         - Insufficient buffer space is available.
*                          The socket cannot be created until suf-
*                          ficient resources are freed.
*
*        EPROTONOSUPPORT - The protocol type or the specified  pro-
*                          tocol is not supported within this
*                          domain.
*
*        EPROTOTYPE      - The protocol is the wrong type for the
*                          socket.
*
*   SEE ALSO
*        accept(), bind(), CloseSocket(), connect(), getsockname(),
*        getsockopt(), IoctlSocket(), listen(), recv(), select(), 
*        send(), shutdown(), WaitSelect()
*****************************************************************************
*
*/
