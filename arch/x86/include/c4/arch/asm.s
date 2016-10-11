; some macros and definitions for assembly code

; macro to make a segment selector
%define selector(index, table, priv) \
    ((index << 3) | (table << 2) | (priv))

; maybe a bit silly, avoids some magic numbers
%define ring(n) (n)

; flags for the 'table' parameter
GDT: equ 0
LDT: equ 1

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
    ret
%endmacro
