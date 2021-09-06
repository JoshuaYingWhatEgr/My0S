[bits 32]
%define ERROR_CODE nop

%define ZERO push 0

extern idt_table    ;c语言中定义的中断处理程序数组

extern put_str      ;声明外部函数

section .data
global intr_entry_table
intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry:

    %2                  ;push 0

    ;保存上下文环境
    push ds
    push es
    push fs
    push gs
    pushad

    mov al,0x20
    out 0xa0,al         ;向从片发送eoi
    out 0x20,al         ;向主片发送eoi

    push %1             ;push 中断号
    call [idt_table+%1 * 4]

    jmp intr_exit

section .data
    dd  intr%1entry
%endmacro

section .text
global intr_exit
intr_exit:

     ;恢复上下文环境
    add esp,4    ;跳过中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp,4    ;跳过ERROR_CODE或者ZERO
    iretd

VECTOR 0x00,ZERO ;vector有两个参数 %2主要用的是第二个参数ZERO= push 0
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO
VECTOR 0x20,ZERO

