BITS 32

%include "c4/arch/asm.s"

global switch_to_usermode
;; this sets up the stack as though an interrupt with a privilege change
;; happened, then `iret`s to apply the change to the higher CPL.
switch_to_usermode:
    push ebp
    mov ebp, esp

    SET_DATA_SELECTORS selector( 4, GDT, ring(3) ) ;; see segments.c

    mov eax, esp
    push dword selector( 4, GDT, ring(3) ) ;; ss
    push dword eax                         ;; esp
    pushf                                  ;; eflags
    push dword selector( 3, GDT, ring(3) ) ;; cs
    mov ebx, [ebp + 8]                     ;; eip, from first argument on stack
    push dword ebx

    iret                                   ;; and return

.done:
;   jmp .done
    mov esp, ebp
    pop ebp
    ret
