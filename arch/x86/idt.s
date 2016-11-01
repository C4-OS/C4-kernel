BITS 32

%include "c4/arch/asm.s"

;; ISR_NOERROR-generated procedures push a null byte
;; onto the stack where the error would be in ISR_ERROR,
;; to keep a consistent stack frame between different types
;; of interrupts in isr_common
%macro ISR_NOERROR 1
    align 4
    isr%1:
        cli
        push dword 0
        push dword %1
        jmp isr_common
%endmacro

%macro ISR_ERROR 1
    align 4
    isr%1:
        cli
        ;; TODO: figure out if this is really right, error code
        ;; doesn't seem to be pushed when it should be?
        ;push dword 0
        push dword %1
        jmp isr_common
%endmacro

%macro IRQ 1
    align 4
    isr%1:
        cli
        push dword 0
        push dword %1
        jmp irq_common
%endmacro

section .text

global load_idt
load_idt:
    mov eax, [esp + 4]
    lidt [eax]
    ret

extern isr_dispatch
isr_common:
    pusha

    ;mov ax, ds
    ;push eax
    ;SET_DATA_SELECTORS selector(2, GDT, ring(0))
    push esp

    call isr_dispatch

    ;pop eax
    ;SET_DATA_SELECTORS ax

    ; remove old esp value
    pop eax

    popa
    add esp, 8
    sti
    iret

extern irq_dispatch
irq_common:
    pusha

    ;mov ax, ds
    ;push eax
    ;SET_DATA_SELECTORS selector(2, GDT, ring(0))
    push esp

    call irq_dispatch

    ;pop eax
    ;SET_DATA_SELECTORS ax

    ; remove old esp value
    pop eax

    popa
    add esp, 4
    sti
    iret

ISR_NOERROR 0
ISR_NOERROR 1
ISR_NOERROR 2
ISR_NOERROR 3
ISR_NOERROR 4
ISR_NOERROR 5
ISR_NOERROR 6
ISR_NOERROR 7
ISR_ERROR   8
ISR_NOERROR 9
ISR_ERROR   10
ISR_ERROR   11
ISR_ERROR   12
ISR_ERROR   13
ISR_ERROR   14
ISR_NOERROR 15
ISR_NOERROR 16
ISR_NOERROR 17
ISR_NOERROR 18
ISR_NOERROR 19
ISR_NOERROR 20
ISR_NOERROR 21
ISR_NOERROR 22
ISR_NOERROR 23
ISR_NOERROR 24
ISR_NOERROR 25
ISR_NOERROR 26
ISR_NOERROR 27
ISR_NOERROR 28
ISR_NOERROR 29
ISR_NOERROR 30
ISR_NOERROR 31

%assign i 32
%rep 16
    IRQ i
    %assign i i+1
%endrep

%assign i 48
%rep 256 - 48
    ISR_NOERROR i
    %assign i i+1
%endrep

section .data
global isr_stubs
isr_stubs:
    %assign i 0
    ;%rep 32
    %rep 256
        dd isr %+ i
        %assign i i+1
    %endrep
