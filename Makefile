ARCH    = x86
KERN_CC = ./cross/bin/i586-elf-

ALL_TARGETS = 
ALL_CLEAN   =

include objs.mk

.PHONY: all
all: $(ALL_TARGETS)

.PHONY: clean
clean: $(ALL_CLEAN)

.PHONY: test
test:
	qemu-system-i386 -kernel ./i586-c4
