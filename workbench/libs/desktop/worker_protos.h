#ifndef WORKER_PROTOS_H
#    define WORKER_PROTOS_H

ULONG           workerEntry(void);
struct WorkerMessage *createWorkerScanMessage(ULONG workerCommand,
                                              ULONG workerAction,
                                              ULONG messageID,
                                              struct MsgPort *replyPort,
                                              BPTR dirLock);
struct MsgPort *startScannerWorker(ULONG id, BPTR dirLock,
                                   struct MsgPort *replyPort);

#endif
