all: real-all

include $(TOP)/config/make.cfg

CFLAGS := -Wunused -O3

SDFILES := C_c_orig.sd C_h_orig.sd

real-all: $(FLEXCAT) copy_sd_files

$(FLEXCAT) : flexcat.c copy_sd_files
	@$(ECHO) "Compiling $@..."
	@$(CC) $(CFLAGS) $< -o $@
	@$(STRIP) $@

copy_sd_files :
	@$(CP) $(SDFILES) $(TOOLDIR)


clean:
	@$(ECHO) "Cleaning sources..."
	@$(RM) -f $(FLEXCAT)

