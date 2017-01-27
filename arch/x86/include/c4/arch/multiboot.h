#ifndef _C4_ARCH_MULTIBOOT_H
#define _C4_ARCH_MULTIBOOT_H 1
#include <stdint.h>

enum {
	MULTIBOOT_FLAG_MEM     = 1 << 0,
	MULTIBOOT_FLAG_DEVICE  = 1 << 1,
	MULTIBOOT_FLAG_CMDLINE = 1 << 2,
	MULTIBOOT_FLAG_MODS    = 1 << 3,
	MULTIBOOT_FLAG_AOUT    = 1 << 4,
	MULTIBOOT_FLAG_ELF     = 1 << 5,
	MULTIBOOT_FLAG_MMAP    = 1 << 6,
	MULTIBOOT_FLAG_DRIVES  = 1 << 7,
	MULTIBOOT_FLAG_CONFIG  = 1 << 8,
	MULTIBOOT_FLAG_LOADER  = 1 << 9,
	MULTIBOOT_FLAG_APM     = 1 << 10,
	MULTIBOOT_FLAG_VBE     = 1 << 11,
};

enum { 
	MULTIBOOT_MEM_AVAILABLE = 1,
	MULTIBOOT_MEM_RESERVED  = 2
};

typedef struct multiboot_mem_map {
	uint32_t size;
	uint32_t addr_high;
	uint32_t addr_low;
	uint32_t len_high;
	uint32_t len_low;
	uint32_t type;
} __attribute__((packed)) multiboot_mem_map_t;

typedef struct multiboot_elf_sheaders {
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
} __attribute__((packed)) multiboot_elf_t;

typedef struct multiboot_module {
	uint32_t start;
	uint32_t end;
	uint32_t string;
	uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

typedef struct multiboot_header {
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	uint32_t cmdline;
	uint32_t mods_count;
	uint32_t mods_addr;
	
	multiboot_elf_t elf_headers;
	uint32_t mmap_length;
	uint32_t mmap_addr;
	uint32_t drives_length;
	uint32_t drives_addr;
	uint32_t config_table;
	uint32_t boot_loader_name;
	uint32_t apm_table;
	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint32_t vbe_mode;
	uint32_t vbe_interface_seg;
	uint32_t vbe_interface_off;
	uint32_t vbe_interface_len;
} __attribute__((packed)) multiboot_header_t;

typedef struct vbe_info_block {
    uint8_t  vbe_sig[4];
    uint16_t version;
    uint16_t oem_str_ptr[2];
    uint8_t  capabilities[4];
    uint16_t mode_ptr[2];
    uint16_t total_mem;
} __attribute__((packed)) vbe_info_block_t;

typedef struct vbe_mode {
    uint16_t attributes;
    uint8_t  win_a, win_b;
    uint16_t granularity;
    uint16_t winsize;
    uint16_t segment_a, segment_b;
    uint16_t real_fct_ptr[2];
    uint16_t pitch;

    uint16_t x_res, y_res;
    uint8_t  w_char, y_char, planes, bpp, banks;
    uint8_t  mem_model, bank_size, image_pages;
    uint8_t  reserved0;

    uint8_t  red_mask, red_position;
    uint8_t  green_mask, green_position;
    uint8_t  blue_mask, blue_position;
    uint8_t  rsv_mask, rsv_position;
    uint8_t  direct_color_attributes;

    uint32_t physbase;
    uint32_t reserved1;
    uint16_t reserved2;
} __attribute__((packed)) vbe_mode_t;

#endif
