K_INCLUDE = -I./include/ -I./arch/$(ARCH)/include/
K_CFLAGS  = -Wall -g -O2 -ffreestanding -nostdlib -nodefaultlibs \
	    -nostartfiles -fno-builtin $(K_INCLUDE)
K_ASFLAGS = -f elf $(K_INCLUDE)

k-obj =

%.o: %.c
	@echo CC $< -c -o $@
	@$(KERN_CC) $(K_CFLAGS) $< -c -o $@

%.o: %.s
	@echo AS $< -o $@
	@$(KERN_AS) $(K_ASFLAGS) $< -o $@

include arch/$(ARCH)/objs.mk
include src/objs.mk

c4-$(ARCH): $(k-obj)
	@echo CC $< -o $@
	@$(KERN_CC) $(K_CFLAGS) -T arch/$(ARCH)/linker.ld $(k-obj) -o $@

ALL_CLEAN += $(k-obj)
