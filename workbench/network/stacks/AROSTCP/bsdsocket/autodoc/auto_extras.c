/****** bsdsocket.library/Errno *********************************************
*
*   NAME
*        Errno - get error value after unsuccessful function call
*
*   SYNOPSIS
*        errno = Errno()
*        D0
*
*        LONG Errno(VOID);
*
*   FUNCTION
*        When  some  function  in  socket  library  return  an  error
*        condition value, they also set a specific error value.  This
*        error value can be extracted by this function.
*
*   RESULT
*        Error value  indicating  the error on  last failure  of some
*        socket function call.
*
*   NOTES
*        Return  value  of  Errno()  is not changed  after successful
*        function so it cannot be used to determine success of any
*        function call  of this library.  Also, another function call
*        to this  library may change  the return value of  Errno() so
*        use it right after error occurred.
*
*   SEE ALSO
*        SetErrnoPtr()
*
*****************************************************************************
*
*/


/****** bsdsocket.library/ObtainSocket **************************************
*
*   NAME
*        ObtainSocket - get a socket from AmiTCP/IP socket list
*
*   SYNOPSIS
*        s = ObtainSocket(id, domain, type, protocol)
*        D0               D0  D1      D2    D3
*
*        LONG ObtainSocket(LONG, LONG, LONG, LONG);
*
*   FUNCTION 
*        When one task wants to give  a socket to  an another one, it
*        releases it (with a key value) to a special socket list held
*        by  AmiTCP/IP.   This  function  requests  that  socket  and
*        receives it if id and other parameters match.
*
*   INPUTS
*        id       - a key value given by the socket donator.
*        domain   - see documentation of socket().
*        type     - see documentation of socket().
*        protocol - see documentation of socket().
*
*   RESULT
*        Non negative socket descriptor on success. On failure, -1 is
*        returned and the errno is set to indicate the error.
*
*   ERRORS
*        EMFILE          - The per-process descriptor table is
*                          full.
*
*        EPROTONOSUPPORT - The protocol type or the specified  pro-
*                          tocol is not supported within this
*                          domain.
*
*        EPROTOTYPE      - The protocol is the wrong type for the
*                          socket.
*
*        EWOULDBLOCK     - Matching socket is not found.
*         
*   SEE ALSO
*        ReleaseCopyOfSocket(), ReleaseSocket(), socket()
*
*****************************************************************************
*
*/

/****** bsdsocket.library/ReleaseCopyOfSocket *******************************
*
*   NAME
*        ReleaseCopyOfSocket - copy given socket to AmiTCP/IP socket list.
*
*   SYNOPSIS
*        id = ReleaseCopyOfSocket(fd, id)
*        D0                       D0  D1
*
*        LONG ReleaseCopyOfSocket(LONG, LONG);
*
*   FUNCTION
*        Make a new reference to a given socket (pointed by its descriptor)
*        and release it to the socket list held by AmiTCP/IP.
*
*   INPUTS
*        fd - descriptor of the socket to release.
*
*        id - the key value to identify use of this socket. It can be
*             unique or not, depending on its  value.  If id value is
*             between 0  and  65535,  inclusively,  it is  considered
*             nonunique  and it can  be  used as a port  number,  for
*             example.   If  id is greater  than  65535 and less than
*             2^31) it  must be unique in currently  held sockets  in
*             AmiTCP/IP socket  list,  Otherwise  an  error  will  be
*             returned  and  socket  is  not  released.    If  id  ==
*             UNIQUE_ID (defined in <sys/socket.h>) an unique id will
*             be generated.
*
*   RESULT
*        id - -1 in case of error and the key value of the socket put
*             in the list. Most useful when an unique id is generated
*             by this routine. 
*
*   ERRORS
*        EINVAL - Requested unique id is already used.
*
*        ENOMEM - Needed memory couldn't be allocated.
*
*   NOTE
*        The socket descriptor is not deallocated.
*
*   SEE ALSO
*        ObtainSocket(), ReleaseSocket()
*
*
*****************************************************************************
*
*/

/****** bsdsocket.library/ReleaseSocket *************************************
*
*   NAME
*        ReleaseSocket - release given socket to AmiTCP/IP socket list.
*
*   SYNOPSIS
*        id = ReleaseSocket(fd, id)
*        D0                 D0  D1
*
*        LONG ReleaseSocket(LONG, LONG);
*
*   FUNCTION
*        Release the reference of given socket (via  its  descriptor)
*        and move the socket to the  socket  list held by  AmiTCP/IP.
*        The socket descriptor is deallocated in this procedure.
*
*   INPUTS
*        fd - descriptor of the socket to release.
*
*        id - the key value to identify use of this socket. It can be
*             unique or not, depending on its  value.  If id value is
*             between 0  and  65535,  inclusively,  it is  considered
*             nonunique  and it can  be  used as a port  number,  for
*             example.   If  id is greater  than  65535 and less than
*             2^31) it  must be unique in currently  held sockets  in
*             AmiTCP/IP socket  list,  Otherwise  an  error  will  be
*             returned  and  socket  is  not  released.    If  id  ==
*             UNIQUE_ID (defined in <sys/socket.h>) an unique id will
*             be generated.
*
*   RESULT
*        id - -1 in case of error and the key value of the socket put
*             in the list. Most useful when an unique id is generated
*             by this routine. 
*
*   ERRORS
*        EINVAL - Requested unique id is already used.
*
*        ENOMEM - Needed memory couldn't be allocated.
*
*   SEE ALSO
*        ObtainSocket(), ReleaseCopyOfSocket()
*
*****************************************************************************
*
*/

/****** bsdsocket.library/SetErrnoPtr ***************************************
*
*   NAME
*        SetErrnoPtr - set new place where the error value will be written
*
*   SYNOPSIS
*        SetErrnoPtr(ptr, size)
*                    A0   D0
*
*        VOID SetErrnoPtr(VOID *, UBYTE);
*
*   FUNCTION
*        This functions allows caller to redirect error variable inside
*        scope of  caller task.   Usually this is  used to make  task's
*        global variable errno as error variable.
*
*   INPUTS
*        ptr     - pointer to error variable that is to be modified on
*                  every error condition on this library function.
*        size    - size of the error variable.
* 
*   EXAMPLE
*        #include <errno.h>
*
*        struct Library;
*        struct Library * SocketBase = NULL;
*
*        int main(void)
*        {
*           ...
*          if ((SocketBase = OpenLibrary("bsdsocket.library", 2))
*              != NULL) {
*            SetErrnoPtr(&errno, sizeof(errno));
*           ...
*          }
*        }
*
*   NOTES
*        Be sure that this new error variable exists until library base
*        is finally closed or SetErrnoPtr() is called again for another
*        variable.
*
*   SEE ALSO
*        Errno()
*
*****************************************************************************
*
*/

/****** bsdsocket.library/SetSocketSignals **********************************
*
*   NAME
*        SetSocketSignals - inform AmiTCP/IP of INTR, IO and URG signals
*
*   SYNOPSIS
*        SetSocketSignals(sigintrmask, sigiomask, sigurgmask)
*                         D0           D1         D2
*
*        VOID SetSocketSignals(ULONG, ULONG, ULONG);
*
*   FUNCTION
*        SetSocketSignals() tells  the  AmiTCP/IP which signal  masks
*        corresponds UNIX SIGINT, SIGIO and SIGURG signals to be used
*        in   this implementation.  The  sigintrmask  mask is used to
*        determine which  Amiga  signals interrupt  blocking  library
*        calls.
*
*        The sigiomask  is sent  when  asynchronous  notification  of
*        socket   events   is  done,  the sigurgmask    is  sent when
*        out-of-band   data  arrives, respectively.   The signals are
*        sent only  to  the owning task  of   particular socket.  The
*        socket  has got no owner   by default; the  owner  is set by
*        FIOSETOWN (SIOCSPGRP) ioctl call.
*
*        Note that the supplied  values write over  old ones. If this
*        function is used and CTRL-C is still wanted to interrupt the
*        calls (the default behaviour),  the value BREAKF_CTRL_C must
*        be explicitly given.
*
*   NOTES
*        The function SetSocketSignals() is obsoleted by the function
*        SocketBaseTags().
*
*   SEE ALSO
*        IoctlSocket(), recv(), send(), select(), WaitSelect()
*
*****************************************************************************
* 
*/
