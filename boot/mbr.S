;主引导程序
;---------------------------------------------
SECTION MBR  vstart=0x7c00 ;告诉编译器将mbr地址编译成0x7c00
        mov  ax,cs
        mov  ds,ax
        mov  es,ax
        mov  ss,ax
        mov  fs,ax
        mov  sp,0x7c00
        mov  ax,0xb800;显卡文本模式显示器的起始地址b8000  结束地址bffff
        mov  gs,ax;往gs中存入段基址

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
       mov  ax,0600h
       mov  bx,0700h
       mov  cx,0
       mov  dx,184fh

       int  10h

;往显存中写入字符
       mov  byte [gs:0x00],'1'
       mov  byte [gs:0x01],0xA4

       mov  byte [gs:0x02],' '
       mov  byte [gs:0x03],0xA4

       mov  byte [gs:0x04],'M'
       mov  byte [gs:0x05],0xA4

       mov  byte [gs:0x06],'B'
       mov  byte [gs:0x07],0xA4

       mov  byte [gs:0x08],'R'
       mov  byte [gs:0x09],0xA4

       ;mov  ah,3
       ;mov  bh,0

       ;int  0x10

;;;;;;;;;  获取光标位置  ;;;;;;;;;

;;;;;;;;   打印字符串   ;;;;;;;;;
      ; mov  ax, message
      ; mov  bp, ax


      ; mov  cx,5
      ; mov  ax, 0x1301

      ; mov  bx, 0x2

       ;int  0x10


       jmp  $;//死循环 程序会卡在这里不动

      ; message db "1 MBR";将通向bios的输入改为通向显存的输入
       times   510-($-$$) db 0;将最后两个字节填充为0
       db      0x55,0xaa  ;填充512字节最后两个字节为0x55,0xaa 主要是为了bios检查到0盘0道1扇区的最后511和512字节是0x550xaa然后将主动权交给mbr并跳转到0x7c00出执行mbr的程序


