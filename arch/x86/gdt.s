BITS 32

; macro to make a segment selector
%define selector(index, table, priv) \
    ((index << 3) | (table << 2) | (priv))

; maybe a bit silly, avoids some magic numbers
%define ring(n) (n)

; flags for the 'table' parameter
GDT: equ 0
LDT: equ 1

section .text
global load_gdt
load_gdt:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, selector(2, GDT, ring(0))
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp selector(1, GDT, ring(0)):.flush

.flush:
    ret
