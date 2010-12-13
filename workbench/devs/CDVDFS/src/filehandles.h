#ifndef FILEHANDLES_H
#define FILEHANDLES_H

struct ACDRHandle {
	void *handle;	/* pointer to FileHandle or Lock */
	ULONG flags;
	struct ACDRDeviceInfo *device;
};

#define AHF_IS_LOCK (1<<0)
#define AHF_IS_FH   (1<<1)

#endif /* FILEHANDLES_H */

