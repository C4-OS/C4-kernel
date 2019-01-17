k-obj += arch/x86/entry.o
k-obj += arch/x86/init.o
k-obj += arch/x86/segments.o
k-obj += arch/x86/debug.o
k-obj += arch/x86/interrupts.o
k-obj += arch/x86/paging.o
k-obj += arch/x86/earlyheap.o
k-obj += arch/x86/pic.o
k-obj += arch/x86/gdt.o
k-obj += arch/x86/idt.o
k-obj += arch/x86/thread.o
k-obj += arch/x86/scheduler.o
k-obj += arch/x86/ringswitch.o
k-obj += arch/x86/syscall.o

k-obj += arch/x86/mp.o
k-obj += arch/x86/apic.o
k-obj += arch/x86/ioapic.o
k-obj += arch/x86/pit.o
k-obj += arch/x86/bios.o
k-obj += arch/x86/apic-utils.o

arch/x86/%.bin: arch/x86/%.s
	$(KERN_AS) -fbin $< -o $@

arch/x86/smp_patch.o: arch/x86/smp.bin
	$(KERN_LD) -melf_i386 -r -b binary -o $@.tmp.o $<
	binary_name="_binary_$$(echo $< | tr '\\/\-. ' _)"; \
	$(KERN_OBJCOPY) --redefine-sym $${binary_name}_start=smp_boot_start \
	                --redefine-sym $${binary_name}_end=smp_boot_end \
	                --redefine-sym $${binary_name}_size=smp_boot_size \
	                $@.tmp.o $@
	rm $@.tmp.o

k-obj += arch/x86/smp_patch.o

ALL_CLEAN += arch/x86/smp.bin
