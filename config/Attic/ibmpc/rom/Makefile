# (C) 1997-1998 AROS - The Amiga Research OS
# $Id: Makefile
#
# Desc: Makefile for kernel

$(CORE):
	@echo -n Compiling kernel core...
	@$(MAKE) -s -C core $(CORE)
	@echo Done!

$(EXEC):
	@$(MAKE) -s -C exec $(EXEC)

$(EXPANSION):
	@$(MAKE) -s -C expansion $(EXPANSION)

$(UTILITY):
	@$(MAKE) -s -C utility $(UTILITY)

$(AROS):
	@$(MAKE) -s -C aros $(AROS)

$(OOP):
	@$(MAKE) -s -C oop $(OOP)

$(BOOPSI):
	@$(MAKE) -s -C boopsi $(BOOPSI)

$(MATHIEEESINGBAS):
	@$(MAKE) -s -C mathieeesingbas $(MATHIEEESINGBAS)

clean:
	@$(MAKE) -s -C core clean
	@$(MAKE) -s -C exec clean
	@$(MAKE) -s -C expansion clean
	@$(MAKE) -s -C utility clean
	@$(MAKE) -s -C aros clean
	@$(MAKE) -s -C boopsi clean
	@$(MAKE) -s -C mathieeesingbas clean