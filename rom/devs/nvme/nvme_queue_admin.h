
int nvme_submit_admincmd(device_t dev, struct nvme_command *cmd, struct completionevent_handler *handler);

BOOL nvme_exec_admincmd(device_t, struct IORequest *, struct nvme_command *, struct completionevent_handler *);
