/* Prototypes for functions defined in
tcp_debug.c
 */

void tcp_trace(int act,
              int ostate,
              struct tcpcb * tp,
              struct tcpiphdr * ti,
              int req);

