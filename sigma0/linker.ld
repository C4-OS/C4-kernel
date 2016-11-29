ENTRY (main)

SECTIONS {
	. = 0x3c0de000;

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
