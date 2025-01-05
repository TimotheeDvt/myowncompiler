global _start
_start:
    mov rax, 54
    push rax
    mov rax, 60
    pop rdi
    syscall
