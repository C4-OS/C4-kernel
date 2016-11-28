SIGMA0_INCLUDE = ./sigma0/include/
MINIFT_INCLUDE = ./sigma0/miniforth/include/
SIGMA0_CFLAGS  = $(K_CFLAGS) -fpie -fpic -I$(SIGMA0_INCLUDE) -I$(MINIFT_INCLUDE)

sig-objs = sigma0/sigma0.o sigma0/display.o sigma0/miniforth/out/miniforth.a

sigma0/%.o: sigma0/%.c
	@echo CC $< -c -o $@
	@$(KERN_CC) $(SIGMA0_CFLAGS) $< -c -o $@

sigma0/miniforth/out/miniforth.a:
	@cd ./sigma0/miniforth; make lib CC=$(KERN_CC) CFLAGS="$(SIGMA0_CFLAGS)";

sigma0/sigma0-$(ARCH).elf: $(sig-objs)
	@$(KERN_CC) $(SIGMA0_CFLAGS) -T sigma0/linker.ld $(sig-objs) -o $@

c4-$(ARCH)-sigma0: sigma0/sigma0-$(ARCH).elf
	@echo CC $< -o $@
	@$(CROSS)objcopy -O binary $< $@

ALL_TARGETS += c4-$(ARCH)-sigma0
ALL_CLEAN   += c4-$(ARCH)-sigma0 $(sig-objs) sigma0/sigma0-$(ARCH).elf
