K_INCLUDE = -I./include -I./arch/$(ARCH)/include
K_CFLAGS  = -Wall -g -O2 -ffreestanding -nostdlib -nodefaultlibs \
	    -nostartfiles -fno-builtin
K_ASFLAGS = -f elf

k-obj =

%.o: %.c
	$(KERN_CC) $(K_CFLAGS) $< -c -o $@

%.o: %.s
	$(KERN_AS) $(K_ASFLAGS) $< -o $@

include arch/$(ARCH)/objs.mk

c4-$(ARCH): $(k-obj)
	$(KERN_CC) $(K_CFLAGS) -T arch/$(ARCH)/linker.ld $(k-obj) -o $@

ALL_CLEAN += $(k-obj)
