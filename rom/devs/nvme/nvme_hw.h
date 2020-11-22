
extern int nvme_submit_cmd(struct nvme_queue *, struct nvme_command *);
extern struct nvme_queue *nvme_alloc_queue(device_t, int, int, int);
extern void nvme_process_cq(struct nvme_queue *);
extern int nvme_alloc_cmdid(struct nvme_queue *);
