# $Id$
TOP =../../../..

include $(TOP)/config/make.cfg

all: setup

setup: 
	@echo ""
	@echo "Make sure you run with the correct Linux-Kernel, otherwise AROS"
	@echo "will definitly crash! The Files for this new Linux-Kernel are in"
	@echo "this directory (config/unix/exec/m68k). Read the README-file!!"
	@echo ""
	@$(CP) -p stackswap.c ../
	@$(CP) -p sigcore.h ../ 
