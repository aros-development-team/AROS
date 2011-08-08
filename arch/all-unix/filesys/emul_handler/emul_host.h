#ifndef RESOURCES_EMUL_HOST_H
#define RESOURCES_EMUL_HOST_H

/* avoid conflicts between our __unused define and the ones that might come in
   via sys/stat.h */
#undef __unused

struct LibCInterface;

struct PlatformHandle
{
    ULONG dirpos; /* Directory search position for Android */
};

struct Emul_PlatformData
{
    void		   *libcHandle;
    struct LibCInterface   *SysIFace;
    int			   *errnoPtr;	/* Pointer to host's errno		 */
    int			    my_pid;	/* AROS process ID			 */
    APTR		    em_UtilityBase;
};

/* Remove this hack later in the ABIv1 development cycle */
#define UtilityBase (emulbase->pdata.em_UtilityBase)

#endif /* RESOURCES_EMUL_HOST_H */
