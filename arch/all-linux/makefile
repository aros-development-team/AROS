TOP=../..

include $(TOP)/config/make.cfg

all: $(TOP)/graphics/graphics_driver.c \
	$(TOP)/intuition/intuition_driver.c

$(TOP)/graphics/graphics_driver.c:
	$(TOP)/scripts/makelinks . $(TOP)/graphics/ graphics_driver.c

$(TOP)/intuition/intuition_driver.c:
	$(TOP)/scripts/makelinks . $(TOP)/intuition/ intuition_driver.c

clean:
	$(RM) $(TOP)/graphics/graphics_driver.c \
	    $(TOP)/intuition/intuition_driver.c

config:
	@echo "Configuring for Linux..."
