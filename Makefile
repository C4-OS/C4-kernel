CROSS   = $(PWD)/cross/bin/i586-elf-
ARCH    = x86
KERN_AS      = nasm
KERN_CC      = $(CROSS)gcc
KERN_LD      = $(CROSS)ld
KERN_OBJCOPY = $(CROSS)objcopy

ALL_TARGETS = c4-$(ARCH)
ALL_CLEAN   = c4-$(ARCH)

.PHONY: all
all: do_all

include objs.mk

.PHONY: do_all
do_all: $(ALL_TARGETS)

.PHONY: clean
clean:
	rm -f $(ALL_CLEAN)
