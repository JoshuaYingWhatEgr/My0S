TI_GDT equ 0
RPL0   equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

[bits 32]
section .text
;------------------ put_char函数实现 --------------------
;把栈中的一个字符写入光标所在处
;-------------------------------------------------------
global put_char ;外部文件可以调用put_char函数
put_char:
    pushad              ;备份32位环境寄存器

    mov ax,SELECTOR_VIDEO ;为保证gs中的为正确的段选择子 所以每次都为gs赋值
    mov gs,ax

    ;;;;;;;;;;;; 获取当前光标位置 ;;;;;;;;;;;;;;;;;;;;;;;;;
    ;先获得高8位
    mov dx,0x03d4    ;获取显存中的索引寄存器
    mov al,0x0e      ;用于提供光标位置的高8位
    out dx,al
    mov dx,0x3d5     ;通过读写数据端口0x3d5来获得或设置光标位置

    in al,dx
    mov ah,al

     ;再获取低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx

    ;将光标存入bx
    mov bx,ax
    ;获取待打印的字符
    mov ecx,[esp+36] ;pushad压入4×8＝32字节,加上主调函数的返回地址4字节,故esp+36字节
    cmp cl,0xd       ;CR是0x0d,LF是0x0a
    jz  .is_carriage_return
    cmp cl,0xa
    jz .is_line_feed


    cmp cl,0x8 ;BS(backspace)的asc码是8
    jz .is_backspace
    jmp .put_other
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


