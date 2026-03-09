/* Prototypes for functions defined in
tcp_cc_cubic.c
 */

u_long tcp_cubic_root(u_long x);

void tcp_cc_init(void);

/* The following are declared in tcp_cc.h:
 * extern struct tcp_cc_algo tcp_cc_cubic;
 * extern struct tcp_cc_algo *tcp_cc_algos[];
 * extern int tcp_cc_default;
 */
