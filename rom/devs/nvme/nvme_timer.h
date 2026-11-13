/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

    Desc:
*/

struct IORequest *nvme_OpenTimer(struct NVMEBase *base);

void nvme_CloseTimer(struct IORequest *tmr);

ULONG nvme_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs);