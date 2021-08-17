%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR

LOADER_STACK_TOP equ LOADER_BASE_ADDR

       ;jmp  loader_start;

;构建gdt及其内部的描述符
GDT_BASE: dd  0x00000000      ;

          dd  0x00000000

CODE_DESC: dd 0x0000FFFF      ;代码段 0x00cf9800-0x0000FFFF =0x00cf98000000FFFF

           dd DESC_CODE_HIGH4 ;0x00cf9800

DATA_STACK_DESC: dd 0x0000FFFF ;数据段和栈段共用一个描述符 0x00cf9800-0x0000FFFF = 0x00cf98000000FFFF

                 dd DESC_DATA_HIGH4;0x00cf9800

VIDEO_DESC: dd  0x80000007 ;显存段

            dd  DESC_VIDEO_HIGH4


GDT_SIZE equ $ - GDT_BASE;获取gdt大小
GDT_LIMIT equ GDT_SIZE -1;获取gdt段界限

;预留60个8字节空位
times 60 dq 0

;total_mem_bytes保存获取到的内存容量以字节位单位 total_mem_bytes的起始地址为0xb00=((前面4个段描述符)+(times 60个段描述符的槽位))*8=521
;512=0x200 +0x900 = 0xb00
total_mem_bytes dd 0

;----------------- 定义段选择子 ---------------------
SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

;gdt指针 前2字节是gdt界限 后4字节是gdt起始地址
gdt_ptr dw GDT_LIMIT

        dd GDT_BASE

ards_buf times 244 dd 0
ards_nr  dw  0

loadermsg db '2 loader in real.'

loader_start:

        ;int 15h eax=0000e820h, edx=534d4150h 获取内存布局
        xor  ebx,ebx
        mov  edx,0x534d4150
        mov  di,ards_buf
.e820_mem_get_loop:           ;循环获取每个ards内存范围描述结构
        mov  eax,0x0000e820   ;执行int 0x15后 eax的值变为0x534d4150

        mov  ecx,20           ;ards结构范围大小是20字节

        int  0x15
        jc   .e820_failed_so_try_e801;如果0xe802的cf位为1出错就用e801子功能

        ;如果cf位为1则有错误发生,尝试用0xe801子功能获取内存
        add  di,cx
        inc  word[ards_nr]

        cmp  ebx,0
        jnz  .e820_mem_get_loop
        mov  cx,[ards_nr]

        mov  ebx,ards_buf
        xor  edx,edx
.find_max_mem_area:
        mov  eax,[ebx]
        add  eax,[ebx+8]
        add  ebx,20
        cmp  edx,eax
        jge  .nex_ards
        mov  edx,eax
.nex_ards:
        loop .find_max_mem_area
        jmp  .mem_get_ok

        ;-------- int =15h ax=e801h 获取内存大小，最大支持4g-----------
.e820_failed_so_try_e801:
        mov ax,0xe801
        int 0x15
        jc  .e801_fialed_so_try88

        mov cx,0x400
        mul cx
        shl edx,16
        add eax,0x0000FFFF
        or edx,eax
        add edx,0x100000
        mov esi,edx


        xor eax,eax
        mov ax,bx
        mov ecx,0x10000
        mul ecx

        add esi,eax
        mov edx,esi
        jmp .mem_get_ok

;-------------  int 15h ah=0x88 获取内存大小只能获取64M ------------------
.e801_fialed_so_try88:
        mov ah,0x88
        int 0x15
        jc .error_hlt
        add eax,0x0000FFFF

        mov  cx,0x400
        mul  cx
        shl  edx,16
        or   edx,eax
        add  edx,0x100000
.mem_get_ok:
        mov [total_mem_bytes],edx ;将获取的内存容量转换为byte存入total_mem_bytes


        ;mov  sp, LOADER_BASE_ADDR
        ;mov  bp, loadermsg
        ;mov  cx, 17
        ;mov  ax,0x1301
        ;mov  bx,0x001f
        ;mov  dx,0x1800

        ;int 0x10  ; 10h号 中断

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

.error_hlt: ;出错挂起
    hlt
[bits 32]
p_mode_start:
        mov ax,SELECTOR_DATA
        mov ds,ax
        mov es,ax
        mov ss,ax
        mov esp,LOADER_STACK_TOP
        mov ax,SELECTOR_VIDEO
        mov gs,ax

        mov byte [gs:160], 'P' ;进入保护模式后显示字符串P

        jmp $





