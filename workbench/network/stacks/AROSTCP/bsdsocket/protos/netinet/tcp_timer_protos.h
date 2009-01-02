/* Prototypes for functions defined in
tcp_timer.c
 */

void tcp_fasttimo(void);

void tcp_slowtimo(void);

void tcp_canceltimers(struct tcpcb * tp);

struct tcpcb * tcp_timers(register struct tcpcb * tp,
                          int timer);

