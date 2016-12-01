ENTRY (main)

SECTIONS {
	. = 0xc0000000;

	.text ALIGN (0x1000) : {
		*(.text.startup)
		*(.text)
	}

	.rodata : {
		*(.rodata*)
	}

	.data : {
		*(.data*)
	}

	.bss : {
		*(COMMON)
		*(.bss)
	}
}
