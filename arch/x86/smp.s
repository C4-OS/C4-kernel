; gdt struct left here for reference
struc gdt_entry_struct
    limit_low:   resb 2
    base_low:    resb 2
    base_middle: resb 1
    access:      resb 1
    granularity: resb 1
    base_high:   resb 1
endstruc

; GDT_ENTRY base, limit, access, granularity
%macro GDT_ENTRY 4+
    dw (%2 & 0xffff)
    dw (%1 & 0xffff)
    db ((%1 >> 16) & 0xff)
    db (%3 & 0xff)
    db (%4 & 0xff) | ((%2 >> 16) & 0xf)
    db ((%1 >> 24) & 0xff)
%endmacro

%macro SET_DATA_SELECTORS 1+
    mov ax, %1
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
%endmacro

%macro SET_CODE_SELECTOR 1+
    jmp %1:%%alabel
  %%alabel:
%endmacro

%define selector(index, table, priv) \
    ((index << 3) | (table << 2) | (priv))

GDT_FLAG_PRESENT equ 0x80

GDT_GRANULARITY_BYTE equ 0
GDT_GRANULARITY_PAGE equ 1
GDT_OP_16BIT         equ 0
GDT_OP_32BIT         equ 1

GDT_TYPE_DATA       equ 0
GDT_TYPE_CODE       equ 0x8
GDT_TYPE_CODE_READ  equ 0x2
GDT_TYPE_DATA_WRITE equ 0x2

GDT: equ 0
LDT: equ 1

CR0_FLAG_PE equ (1 << 0)
CR0_FLAG_PG equ (1 << 31)

%define gdt_access(type, priv) \
    (GDT_FLAG_PRESENT | (priv << 5) | (1 << 4) | type)

%define gdt_granularity(granularity, op_size, is_longmode) \
    ((granularity << 7) | (op_size << 6) | (is_longmode << 5)) 

%define ring(n) (n)

SMP_KERNEL_ENTRY equ 0x101000
org 0xa000

section .text
global smp_boot_init
smp_boot_init:
    BITS 16

;    cli
;.hang:
;    jmp .hang

    ; load GDT
    lgdt [gdtptr]
    ; enabled protected mode
    mov eax, cr0
    or  eax, CR0_FLAG_PE
    mov cr0, eax

    ; load 32-bit segments
    SET_CODE_SELECTOR selector(1, GDT, ring(0))
    BITS 32
    mov esp, tempstack
    SET_DATA_SELECTORS selector(2, GDT, ring(0))

    ; now we're running in a proper 32-bit protected mode environment,
    ; jump into SMP entry routine
    jmp SMP_KERNEL_ENTRY 

section .bss
tempstack: resb 128

section .data
align 16
gdtptr:
    dw 5 * 8
    dd gdtdesc

align 16
gdtdesc:
    ; null descriptor
    db 0, 0, 0, 0, 0, 0, 0, 0

    ;GDT_ENTRY 0x0, 0xfffff, \
    ;          gdt_access(GDT_TYPE_CODE | GDT_TYPE_CODE_READ, 0), \
    ;          gdt_granularity(GDT_GRANULARITY_BYTE, GDT_OP_16BIT, 0)
    ;GDT_ENTRY 0x0, 0xfffff, \
    ;          gdt_access(GDT_TYPE_DATA | GDT_TYPE_DATA_WRITE, 0), \
    ;          gdt_granularity(GDT_GRANULARITY_BYTE, GDT_OP_16BIT, 0)
    GDT_ENTRY 0x0, 0xfffff, \
              gdt_access(GDT_TYPE_CODE | GDT_TYPE_CODE_READ, 0), \
              gdt_granularity(GDT_GRANULARITY_PAGE, GDT_OP_32BIT, 0)
    GDT_ENTRY 0x0, 0xfffff, \
              gdt_access(GDT_TYPE_DATA | GDT_TYPE_DATA_WRITE, 0), \
              gdt_granularity(GDT_GRANULARITY_PAGE, GDT_OP_32BIT, 0)
    GDT_ENTRY 0x0, 0xfffff, \
              gdt_access(GDT_TYPE_CODE | GDT_TYPE_CODE_READ, 0), \
              gdt_granularity(GDT_GRANULARITY_PAGE, GDT_OP_32BIT, 3)
    GDT_ENTRY 0x0, 0xfffff, \
              gdt_access(GDT_TYPE_DATA | GDT_TYPE_DATA_WRITE, 0), \
              gdt_granularity(GDT_GRANULARITY_PAGE, GDT_OP_32BIT, 3)
