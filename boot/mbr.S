;主引导程序
;让mbr可以读取硬盘(查询传输和中断传输方式)从磁盘把loader加载到内存
;---------------------------------------------
%include "boot.inc"
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

       mov  eax,LOADER_START_SECTOR;起始扇区lba的编号
       mov  bx,LOADER_BASE_ADDR;写入的地址0x900 bootloader加载到内存的起始地址
       mov  cx,4;现在的bootloader还是不是很大肯定小于512字节所以先读1个扇区
       call rd_disk_m_16;读取程序的起始部分(一个扇区)

       jmp  LOADER_BASE_ADDR+0x300;跳转到0x900bootloader处执行

rd_disk_m_16:

       mov  esi,eax
       mov  di,cx
;读写磁盘第一步,设置要读取的扇区数
       mov dx,0x1f2
       mov al,cl
       out dx,al

       mov eax,esi
;第二步:将lba地址存入0x1f3-0x1f6
       ;lba地址7-0位写入端口 0x1f3
       mov dx,0x1f3
       out dx,al

       ;lba地址15-8位写入端口0x1f4
       mov cl,8
       shr eax,cl
       mov dx,0x1f4
       out dx,al

       ;lba地址23-16位写入端口0x1f5
       shr eax,cl
       mov dx,0x1f5
       out dx,al

       shr eax,cl
       and al,0x0f
       or  al,0xe0
       mov dx,0x1f6
       out dx,al

;第三步:向0x1f7端口写入读命令,0x20
       mov dx,0x1f7
       mov al,0x20
       out dx,al

;第四步:检查硬盘状态
    .not_ready:
       nop
       in al,dx
       and al,0x88

       cmp al,0x08
       jnz .not_ready

;第五步:从0x1f0端口读数据
       mov ax,di
       mov dx,256
       mul dx
       mov cx,ax

       mov dx,0x1f0

    .go_on_ready:
       in ax,dx
       mov [bx],ax
       add bx,2
       loop .go_on_ready
       ret


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


       ;jmp  $;//死循环 程序会卡在这里不动

      ;message db "1 MBR";将通向bios的输入改为通向显存的输入
       times   510-($-$$) db 0;将最后两个字节填充为0
       db      0x55,0xaa  ;填充512字节最后两个字节为0x55,0xaa主要是为了bios检查到0盘0道1扇区的最后511和512字节是0x550xaa然后将主动权交给mbr并跳转到0x7c00出执行mbr的程序