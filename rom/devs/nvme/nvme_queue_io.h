
#define DMAFLAGS_PREREAD     0
#define DMAFLAGS_PREWRITE    DMA_ReadFromRAM
#define DMAFLAGS_POSTREAD    (1 << 31)
#define DMAFLAGS_POSTWRITE   (1 << 31) | DMA_ReadFromRAM

int nvme_submit_iocmd(struct nvme_queue *nvmeq, struct nvme_command *cmd, struct completionevent_handler *handler);
