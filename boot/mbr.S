;主引导程序
;---------------------------------------------
SECTION MBR  vstart=0x7c00 ;告诉编译器将mbr地址编译成0x7c00
        mov  ax,cs
        mov  ds,ax
        mov  es,ax
        mov  ss,ax
        mov  fs,ax
        mov  sp,0x7c00

;清屏用0x06功能,上卷全部行,则可清屏
;---------------------------------------------
;INT 0X10   功能号:0x60 上卷窗口
;---------------------------------------------
;输入:
;AH 功能号= 0X60
;AL = 上卷的行数(如果为0,表示全部)
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(x,y)位置
;(DL,DH) = 窗口右上角的(x,y)位置
;无返回值:
       mov  ax,0x600
       mov  bx,0x700
       mov  cx,0
       mov  dx,0x184f

       int  0x10

       mov  ah,3
       mov  bh,0

       int  0x10

;;;;;;;;;  获取光标位置  ;;;;;;;;;

;;;;;;;;   打印字符串   ;;;;;;;;;
       mov  ax, message
       mov  bp, ax


       mov  cx,5
       mov  ax, 0x1301

       mov  bx, 0x2

       int  0x10


       jmp  $

       message db "1 MBR"
       times   510-($-$$) db 0;将最后两个字节填充为0
       db      0x55,0xaa  ;填充512字节最后两个字节为0x55,0xaa


