SRC	:= hiddtool.c
EXE	:= ./hiddtool

CONFIGFILE	:= ./hidd.conf
DEPFILE		:= ./hidds.dep
all	: $(EXE)

$(EXE)	: $(SRC)
	@gcc -g -Wall $< -o $@
	
clean:
	@rm hiddtool	

demo	: $(EXE)
	@echo "Demo of hidd tool"
	@echo ""
	@echo "This tool uses the files hidd.conf and hidds.dep"
	@echo "hidd.conf is a config file that tells what hidds to"
	@echo "build and if they go into kernel or on disk."
	@echo "This file would be in the config/arch/cpu directory"
	@echo ""
	@echo "Example hidd.conf file:"
	@echo ""
	@cat $(CONFIGFILE)
	@echo ""
	@echo ""
	@echo "Press a key"
	@read
	@echo ""
	@echo ""
	@echo "The hidd.deps file contain the runtime dependancies"
	@echo "between the hidds"
	@echo ""
	@echo "Example hidd.deps file:"
	@echo ""
	@echo ""
	@cat $(DEPFILE)
	@echo ""
	@echo ""
	@echo "Press a key"
	@read
	@echo ""
	@echo "The hiddtool currently has three ways of invocation:"
	@echo ""
	@echo "hiddtool -k configfile"
	@echo ""
	@echo "This will result in a list of the kernel hidds sorted"
	@echo "by dependancy like this:"
	@echo ""
	@$(EXE) -k $(CONFIGFILE)
	@echo ""
	@echo ""
	@echo "hiddtool -d configfile"
	@echo ""
	@echo "This will result in a list of the disk hidds sorted"
	@echo "by dependancy like this:"
	@echo ""
	@$(EXE) -d $(CONFIGFILE)
	@echo ""
	@echo ""
	@echo "Press a key"
	@read
	@echo "hiddtool -m configfile"
	@echo ""
	@echo "This will generate a set of metamake dependencies to build"
	@echo "the necessary hidds"
	@echo ""
	@$(EXE) -m $(CONFIGFILE)
	@echo ""
	@echo ""
	@echo "These can be inserted in the config/arch/cpu mmakefile"
	@echo "with a little change in genmf:"
	@echo "One must have some builtin %shell template that"
	@echo "executes a supplied shell command and where stdout"
	@echo "will be generated into the mmakefile"
	@echo "This again means that when config/arch/cpu/hidd.conf"
	@echo "is changed, genmf should be called to regenrate the mmakefile"
	@echo "One could alternatively put the rules into"
	@echo "mmakefile.src manually, but then you are writing the"
	@echo "same information twice"
	@echo ""
	@echo "Press a key"
	@read
	@echo ""
	@echo "Dunno if this is 100% correct metamake syntax, but the point"
	@echo "is that the first #MM hidd-kernel: target should call"
	@echo "makes all the dependencies be built in the different"
	@echo "hidd directories. The hidds are built into xxx.a libs"
	@echo "like for example libhidd-base.a"
	@echo "Then the user has to supply the rules"
	@echo "hidd-kernel-local and hidd-disk-local"
	@echo "to compile the hidds into kernel and disk hidds"
	@echo "To obtain a list of all kernel hidd libs"
	@echo "one can just use the shell inbuilt make command to"
	@echo "get the hidds into a make variable"
	@echo "and the use foreach to get libxxx-yyy.a."
	@echo ""
	@echo "One could also later add options to this tool"
	@echo "to automatically build resident tables for the hidds"
	@echo "The point is that you only need to change the hidd.conf file"
	@echo "and recompile"
	@echo ""
	@echo "Also it could maybe be expanded to work for libs, devs etc."
	@echo ""

	
test:	
	@echo $(KERNELHIDDS)
	@echo $(KERNELHIDDLIBS)
	
