ENTRY (main)

SECTIONS {
	. = 0x10000;

	.text ALIGN (0x1000) : {
		*(.text.startup)
		*(.text)
	}

	.rodata ALIGN (0x1000) : {
		*(.rodata*)
	}

	.data ALIGN (0x1000) : {
		*(.data*)
	}

	.bss ALIGN (0x1000) : {
		*(COMMON)
		*(.bss)
	}
}
