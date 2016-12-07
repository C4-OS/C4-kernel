SIGMA0_INCLUDE = ./sigma0/include/
MINIFT_INCLUDE = ./sigma0/miniforth/include/
SIGMA0_CFLAGS  = $(K_CFLAGS) -fpie -fpic -I$(SIGMA0_INCLUDE) -I$(MINIFT_INCLUDE)

sig-objs  = sigma0/sigma0.o sigma0/display.o sigma0/tar.o sigma0/elf.o
sig-objs += sigma0/miniforth/out/miniforth.a
sig-objs += sigma0/init_commands.o
sig-objs += sigma0/initfs.o

user-src = $(wildcard sigma0/initfs/src/*.c)
user-obj = $(subst src,bin,$(user-src:.c=))

sigma0/%.o: sigma0/%.c
	@echo CC $< -c -o $@
	@$(KERN_CC) $(SIGMA0_CFLAGS) $< -c -o $@

sigma0/initfs/bin/%: sigma0/initfs/src/%.c
	@echo CC $< -c -o $@
	@$(KERN_CC) $(SIGMA0_CFLAGS) $< -o $@

sigma0/%.o: sigma0/%.fs
	@echo LD $< -o $@
	@$(KERN_LD) -r -b binary -o $@ $<

sigma0/%.o: sigma0/%.tar
	@echo LD $< -o $@
	@$(KERN_LD) -r -b binary -o $@ $<

sigma0/initfs.tar: $(user-obj) $(wildcard sigma0/initfs/*)
	tar c sigma0/initfs > $@

sigma0/miniforth/out/miniforth.a:
	@cd ./sigma0/miniforth; make lib CC=$(KERN_CC) CFLAGS="$(SIGMA0_CFLAGS)";

sigma0/sigma0-$(ARCH).elf: $(sig-objs)
	@$(KERN_CC) $(SIGMA0_CFLAGS) -T sigma0/linker.ld $(sig-objs) -o $@

c4-$(ARCH)-sigma0: sigma0/sigma0-$(ARCH).elf
	@echo CC $< -o $@
	@$(CROSS)objcopy -O binary $< $@

ALL_TARGETS += c4-$(ARCH)-sigma0
ALL_CLEAN   += c4-$(ARCH)-sigma0 $(sig-objs) sigma0/sigma0-$(ARCH).elf
ALL_CLEAN   += sigma0/initfs.tar sigma0/miniforth/out/miniforth.a
ALL_CLEAN   += $(user-obj)
