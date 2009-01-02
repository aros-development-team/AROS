/* Prototypes for functions defined in amiga_select.c
 */

#if __SASC
LONG __asm _IoctlSocket(register __a6 struct SocketBase * , register __d0 LONG , register __d1 ULONG , register __a0 caddr_t );
#endif

void select_init(void);

#if __SASC
LONG __asm _WaitSelect(register __a6 struct SocketBase * , register __d0 ULONG , register __a0 fd_mask * , register __a1 fd_mask * , register __a2 fd_mask * , register __a3 struct timeval *, register __d1 ULONG *);
#endif


int selscan(struct SocketBase * p,
            fd_mask * in,
            fd_mask * ou,
            fd_mask * ex,
            fd_mask * obits,
            int nfd,
            int * retval,
	    int * selitemcount);

void selenter(struct SocketBase * p,
              struct newselitem **hdr);

void unselect(register struct newselbuf * sb);

void selwakeup(struct newselitem **hdr);

int soo_select(struct socket * so,
               int which,
               struct SocketBase * p);

