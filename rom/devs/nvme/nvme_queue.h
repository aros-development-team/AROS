
struct nvme_queue *nvme_alloc_queue(device_t dev, int qid, int depth, int vector);
void nvme_free_queue(struct nvme_queue *nvmeq);
void nvme_stop_iotask(struct nvme_queue *nvmeq);
