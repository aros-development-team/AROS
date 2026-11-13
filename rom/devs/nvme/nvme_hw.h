
extern int nvme_submit_cmd(struct nvme_queue *, struct nvme_command *);
extern struct nvme_queue *nvme_alloc_queue(device_t, int, int, int);
extern void nvme_free_queue(struct nvme_queue *);
extern int nvme_process_cq(struct nvme_queue *);
extern int nvme_alloc_cmdid(struct nvme_queue *);
extern void nvme_release_cmdid(struct nvme_queue *, UWORD);
extern void nvme_stop_completions(struct nvme_queue *, struct IORequest *);
extern BOOL nvme_quiesce(device_t, struct IORequest *);
extern void nvme_release_queues(device_t);
