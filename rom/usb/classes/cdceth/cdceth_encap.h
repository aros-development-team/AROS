#ifndef CDCETH_ENCAP_H
#define CDCETH_ENCAP_H

#include <exec/types.h>

struct NepClassEth;

BOOL cdceth_encap_configure(struct NepClassEth *ncp);
BOOL cdceth_prepare_tx(struct NepClassEth *ncp, ULONG payload_len, ULONG buf_size,
                       ULONG *payload_offset, ULONG *total_len);
void cdceth_finalize_tx(struct NepClassEth *ncp, UBYTE *buf, ULONG payload_offset,
                        ULONG payload_len, ULONG total_len);
BOOL cdceth_handle_rx(struct NepClassEth *ncp, UBYTE *pktptr, ULONG pktlen);

#endif
