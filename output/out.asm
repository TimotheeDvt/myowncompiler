global _start
_start:
    mov rax, 7
    push rax
    mov rax, 8
    push rax
    mov rax, 1
    push rax
    pop rax
    pop rbx
    add rax, rbx
    push rax
    mov rax, 1
    mov rdi, 1
    mov rsi, message
    mov rdx, msg_len
    syscall
    push QWORD [rsp + 0]

    mov rax, 60
    pop rdi
    syscall

section .data
    message db "7", 0xA
    msg_len equ $ - message
