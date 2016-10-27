BITS 32

%include "c4/arch/asm.s"

section .text
global load_gdt
load_gdt:
    mov eax, [esp + 4]
    lgdt [eax]

    SET_DATA_SELECTORS selector(2, GDT, ring(0))
    SET_CODE_SELECTOR  selector(1, GDT, ring(0))

    ret

global load_tss
load_tss:
    mov eax, [esp + 4]
    ltr ax
    ret
