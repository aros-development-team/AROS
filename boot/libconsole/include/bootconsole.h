struct multiboot;
struct vbe_mode;

/* Initialization */
void con_InitMultiboot(struct multiboot *mb);
void con_InitVESA(unsigned short version, struct vbe_mode *mode);
void con_InitVGA(void);
void con_InitSerial(char *cmdline);

/* Output */
void con_Clear(void);
void con_Putc(char);

/* Serial I/O */
void serial_Init(char *opts);
void serial_Putc(unsigned char data);

/* Memory allocation */
void *boot_AllocMem(unsigned long len);
