/* Prototypes for functions defined in raw_cb.c
 */

int raw_attach(register struct socket * so,
               int proto);

void raw_detach(register struct rawcb * rp);

void raw_disconnect(struct rawcb * rp);

