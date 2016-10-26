BITS 32

get_eip:
    mov eax, [esp]
    ret

global sched_do_thread_switch
;; esp+4: thread_t structure of current thread, which will
;;        be saved in a way that can be resumed later
;; esp+8: next thread, which needs to be loaded
sched_do_thread_switch:
    push esi
    mov esi, esp
    add esi, 4
    pusha

    ; load first argument into edx, and skip to loading routine
    ; if it's zero (as it is when the scheduler is first started)
    mov edx, [esi + 4]
    test edx, edx
    je .load_state

.save_state:
    ; store ebp, esp and eip respectively
    mov [edx + 0], ebp
    mov [edx + 4], esp
    mov eax, .finished
    mov [edx + 8], eax

.load_state:
    ; load second argument from stack
    mov edx, [esi + 8]

    ; load ebp, esp and eip, and jump to new eip
    mov ebp, [edx + 0]
    mov esp, [edx + 4]
    mov ebx, [edx + 8]
    sti
    jmp ebx

.finished:
    ; restore state when resuming from previous thread switch
    popa
    pop esi
    ret
