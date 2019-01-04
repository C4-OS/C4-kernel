ENTRY (loader)

SECTIONS {
	/* . = 0x00100000; */
	. = 0xfd100000;

	.__mbHeader : AT(ADDR(.__mbHeader) - 0xfd000000) {
		*(.__mbHeader)
	}

	. = 0xfd101000;
	.__smp_entry : AT(ADDR(.__smp_entry) - 0xfd000000) {
		*(.__smp_entry)
	}

	.text ALIGN (0x1000) : AT(ADDR(.text) - 0xfd000000) {
		*(.text)
	}

	.rodata ALIGN (0x1000) : AT(ADDR(.rodata) - 0xfd000000) {
		*(.rodata*)
	}

	.data ALIGN (0x1000) : AT(ADDR(.data) - 0xfd000000) {
		*(.data)
	}

	.bss ALIGN (0x1000) : AT(ADDR(.bss) - 0xfd000000) {
		sbss = .;
		*(COMMON)
		*(.bss)
		ebss = .;
	}

	end = .; _end = .; __end = .;
}
