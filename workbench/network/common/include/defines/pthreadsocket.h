
#ifndef PTHREAD_SOCKET_H
#define PTHREAD_SOCKET_H

#ifdef accept
#undef accept
#endif
#define accept(...) (pthread_testcancel(), __accept_WB(SocketBase, __VA_ARGS__))

#ifdef connect
#undef connect
#endif
#define connect(...) (pthread_testcancel(), __connect_WB(SocketBase, __VA_ARGS__))

#ifdef CloseSocket
#undef CloseSocket
#endif
#define CloseSocket(...) (pthread_testcancel(), __CloseSocket_WB(SocketBase, __VA_ARGS__))

#ifdef recv
#undef recv
#endif
#define recv(...) (pthread_testcancel(), __recv_WB(SocketBase, __VA_ARGS__))

#ifdef recvfrom
#undef recvfrom
#endif
#define recvfrom(...) (pthread_testcancel(), __recvfrom_WB(SocketBase, __VA_ARGS__))

#ifdef recvmsg
#undef recvmsg
#endif
#define recvmsg(...) (pthread_testcancel(), __recvmsg_WB(SocketBase, __VA_ARGS__))

#ifdef __AROS__
#ifdef select
#undef select
#endif
#define select(nfds,rfds,wfds,efds,timeout) (pthread_testcancel(), __WaitSelect_WB(SocketBase, nfds,rfds,wfds,efds,timeout, NULL))

#ifdef WaitSelect
#undef WaitSelect
#endif
#define WaitSelect(...) (pthread_testcancel(), __WaitSelect_WB(SocketBase, __VA_ARGS__))
#endif

#ifdef send
#undef send
#endif
#define send(...) (pthread_testcancel(), __send_WB(SocketBase, __VA_ARGS__))

#ifdef sendmsg
#undef sendmsg
#endif
#define sendmsg(...) (pthread_testcancel(), __sendmsg_WB(SocketBase, __VA_ARGS__))

#ifdef sendto
#undef sendto
#endif
#define sendto(...) (pthread_testcancel(), __sendto_WB(SocketBase, __VA_ARGS__))

#endif /* PTHREAD_SOCKET_H */
