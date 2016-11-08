BITS 32

get_eip:
    mov eax, [esp]
    ret

;; usermode_jump defined in ringswitch.s
extern usermode_jump

global sched_do_thread_switch
;; esp+4: thread_t structure of current thread, which will
;;        be saved in a way that can be resumed later
;; esp+8: next thread, which needs to be loaded
sched_do_thread_switch:
    cli
    push esi
    mov esi, esp
    add esi, 4
    pusha

    ; load first argument into edx, and skip to loading routine
    ; if it's zero (as it is when the scheduler is first started)
    mov edx, [esi + 4]
    test edx, edx
    jz .load_state

.save_state:
    ; store ebp, esp and eip respectively
    mov [edx + 0], ebp
    mov [edx + 4], esp
    mov eax, .finished
    mov [edx + 8], eax

.load_state:
    ; load second argument from stack
    mov edx, [esi + 8]

    ; test to see if this thread needs to be switched to usermode,
    ; and if so, do that instead of restoring state here
    mov  eax, [edx + 12]
    test eax, eax
    jnz .do_usermode_switch

    ; load ebp, esp and eip, and jump to new eip
    mov ebp, [edx + 0]
    mov esp, [edx + 4]
    mov ebx, [edx + 8]
    sti
    jmp ebx

.do_usermode_switch:
    mov [edx + 12], dword 0

    ; push stack argument (esp)
    mov eax, [edx + 4]
    push eax

    ; push entry argument (eip)
    mov eax, [edx + 8]
    push eax

    push dword 0
    jmp usermode_jump

.finished:
    ; restore state when resuming from previous thread switch
    popa
    pop esi
    ret
