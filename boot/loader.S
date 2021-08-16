%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
       ;mov  byte [gs:0x00],'2'
       ;mov  byte [gs:0x01],0xA4

       ;mov  byte [gs:0x02],' '
       ;mov  byte [gs:0x03],0xA4

       ;mov  byte [gs:0x04],'L'
       ;mov  byte [gs:0x05],0xA4

       ;mov  byte [gs:0x06],'O'
       ;mov  byte [gs:0x07],0xA4

       ;mov  byte [gs:0x08],'A'
       ;mov  byte [gs:0x09],0xA4

       ;mov  byte [gs:0x0a],'D'
       ;mov  byte [gs:0x0b],0xA4

       ;mov  byte [gs:0x0c],'E'
       ;mov  byte [gs:0x0d],0xA4

       ;mov  byte [gs:0x0E],'R'
       ;mov  byte [gs:0x0F],0xA4

LOADER_STACK_TOP equ LOADER_BASE_ADDR

       jmp  loader_start;

;构建gdt及内部的描述符
GDT_BASE: dd  0x00000000

          dd  0x00000000

CODE_DESC: dd 0x0000FFFF

           dd DESC_CODE_HIGH4

DATA_STACK_DESC: dd 0x0000FFFF

                 dd DESC_DATA_HIGH4

VIDEO_DESC: dd  0x80000007

            dd  DESC_VIDEO_HIGH4


GDT_SIZE equ $ - GDT_BASE
GDT_LIMIT equ GDT_SIZE -1

;预留60个8字节空位
times 60 dq 0

;----------------- 定义段选择子 ---------------------
SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

;gdt指针 前2字节是gdt界限 后4字节是gdt起始地址
gdt_ptr dw GDT_LIMIT

        dd GDT_BASE

loadermsg dd '2 loader in rel.'

loader_start:
        mov  sp, LOADER_BASE_ADDR
        mov  bp, loadermsg
        mov  cx, 16
        mov  ax,0x1301
        mov  bx,0x001f
        mov  dx,0x1800

        int 0x10  ; 10h号 中断

;------------------- 进入保护模式 -----------------------------
;1.打开地址线 A20
;加载gdt
;将cr0的pe位设置为1

        ;----------------------  A20 ------------------------

        in  al,0x92
        or  al,0000_0010B
        out 0x92,al

        ;--------------------  加载gdt ------------------------
        lgdt  [gdt_ptr]

        ;-------------------- cr0 第0位值为1 -------------------
        mov  eax, cr0
        or   eax, 0x00000001
        mov  cr0,eax

        jmp  dword SELECTOR_CODE:p_mode_start ;刷新流水线

[bits 32]
p_mode_start:
        mov ax,SELECTOR_DATA
        mov ds,ax
        mov es,ax
        mov ss,ax
        mov esp,LOADER_STACK_TOP
        mov ax,SELECTOR_VIDEO
        mov gs,ax

        mov byte [gs:160], 'P'

        jmp $





