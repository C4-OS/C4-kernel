BITS 32

; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN  equ 1<<0                          ; align loaded modules on page boundaries
MEMINFO      equ 1<<1                          ; provide memory map
VIDEO        equ 1<<2                          ; request video mode
FLAGS        equ MODULEALIGN | MEMINFO | VIDEO ; this is the Multiboot 'flag' field
MAGIC        equ 0x1BADB002                    ; 'magic number' lets bootloader find the header
CHECKSUM     equ -(MAGIC + FLAGS)              ; checksum required

;KERNEL_VBASE    equ 0xc0000000
KERNEL_VBASE    equ 0xfd000000
KERNEL_PAGE_NUM equ (KERNEL_VBASE >> 22)
;STACKSIZE       equ 0x4000
STACKSIZE       equ 0x400
MAX_CPUS        equ 32

section .__mbHeader
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0    ; unused
    dd 0
    dd 0
    dd 0
    dd 0
    dd 1    ; prefered video type, 0 for linear and 1 for text
    dd 1024 ; prefered video width
    dd 768  ; prefered video height
    dd 32   ; prefered video bpp

section .data
; reserve some space for the kernel page directory,
; with a 4MB page at 0x0 and at 0xc0000000.
; identity-mapped 0x0 is needed because 'loader' will still
; be running at around 0x100000 after paging is initialized
;
; NOTE: this is re-used as the kernel page directory after boot
; TODO: maybe consider cloning the page directory in kernel after boot,
;       although in terms of memory it's no more efficient than having two
;       copies here
align 0x1000
global boot_page_dir
boot_page_dir:
    dd 0x00000083
    times (KERNEL_PAGE_NUM - 1) dd 0
    dd 0x00000083
    times (1024 - KERNEL_PAGE_NUM - 1) dd 0

; copy of the initial boot_page_dir, however this one isn't changed after
; enabling paging. After leaving the SMP entry stub, the new CPU will switch
; to the main kernel directory, which is the mutable boot_page_dir above.
align 0x1000
smp_boot_page_dir:
    dd 0x00000083
    times (KERNEL_PAGE_NUM - 1) dd 0
    dd 0x00000083
    times (1024 - KERNEL_PAGE_NUM - 1) dd 0

section .text
align 4

; initialize paging so we can jump to the real init code
; note that this doesn't access any memory until after paging is set up.
global loader
loader:
    mov ecx, boot_page_dir - KERNEL_VBASE
    mov cr3, ecx

    mov ecx, cr4
    or ecx, 0x10
    mov cr4, ecx

    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    lea ecx, [higher_half_start]
    jmp ecx

global higher_half_start
extern arch_init
higher_half_start:
    ; unmap page at 0x0, now that the page init stub has finished
    mov dword [boot_page_dir], 0
    invlpg [0]

    mov esp, stack + STACKSIZE                 ; set up the stack
    mov ebp, 0

    push eax                                   ; Multiboot magic number
    push esp                                   ; Current stack pointer

    add ebx, KERNEL_VBASE
    push ebx                                   ; Multiboot info structure
    call arch_init                             ; call kernel proper

    cli
.hang:
    hlt                                        ; halt machine should kernel return
    jmp  .hang

global smp_higher_half_entry
extern smp_thing
smp_higher_half_entry:

section .__smp_entry
; stub for APs to call after being brought up and switched to protected mode
global smp_entry
extern smp_thing
smp_entry:
    cli
    ;; same as ``loader'' stub here
    mov ecx, smp_boot_page_dir - KERNEL_VBASE
    mov cr3, ecx

    mov ecx, cr4
    or ecx, 0x10
    mov cr4, ecx

    ;; TODO: make an asm includes file for better readability
    ;;       oh wait there already is one, why aren't we using it :V
    mov ecx, 0x80000001
    mov cr0, ecx

    ;; jump to the proper higher half kernel address space
    lea ecx, [.smp_entry_higher_half]
    jmp ecx

.smp_entry_higher_half:
    ;; immediately change the page directory to the real kernel page directory,
    ;; which by this point should already have recursive page mappings
    ;; and I/O maps and everything set up
    mov ecx, boot_page_dir - KERNEL_VBASE
    mov cr3, ecx

    ; get a fresh new kernel stack for the CPU
    ; TODO: there should be some locking here, so CPUs
    ;       don't end up sharing stacks by mistake, although
    ;       the timing between initializations makes this pretty
    ;       improbable (for now)
    mov eax, [stack_offset]
    add eax, STACKSIZE
    mov [stack_offset], eax
    add eax, STACKSIZE
    add eax, stack
    mov esp, eax
    mov ebp, 0

    inc dword [cpu_count]
    mov eax,  [cpu_count]

    push eax
    call smp_thing

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 4
stack:        resb STACKSIZE * MAX_CPUS          ; reserve stacks for CPUs
stack_offset: resd 1
cpu_count:    resd 1
