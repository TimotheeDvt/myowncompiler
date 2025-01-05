global _start
_start:
    mov rax, 4
    push rax
    mov rax, 60
    mov rdi, 0
    syscall