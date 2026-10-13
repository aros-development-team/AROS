
int nvme_submit_iocmd(struct nvme_queue *nvmeq,
                                    struct nvme_command *cmd,
                                    struct completionevent_handler *handler);

void nvme_finish_ioevent(struct completionevent_handler *slot);
