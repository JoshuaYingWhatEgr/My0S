TI_GDT equ 0
RPL0   equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

section .data
put_int_buffer dp 0     ;定义8字节的缓冲区用于int向char的转换

[bits 32]
section .text

;----------------- put_str字符串函数实现 -----------------------
global put_str
put_str:
    push ebx
    push ecx
    xor  ecx,ecx
    mov  ebx,[esp+12]  ;获取打印的字符串地址

.goon:
    mov cl,[ebx]    ;通过寻址访问1字节内存数据存储到cl中
    cmp cl,0
    jz  .str_over ;如果字符处理到了结尾0 则跳到结束处返回
    push ecx      ;为put_char函数传递参数 cx:ch-cl ecx:
    call put_char
    add esp,4
    inc ebx    ;ebx指向下一个字符
    jmp .goon

.str_over:
    pop ecx
    pop ebx
    ret


;------------------ put_char打印单个字符函数实现 --------------------
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
    out dx,al        ;将0x0e索引写入0x03d4中
    mov dx,0x3d5     ;通过读写数据端口0x3d5来获得或设置光标位置
    in al,dx
    mov ah,al

     ;再获取低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx       ;现在寄存器ax中是16位的光标完整坐标值

    ;将光标存入bx
    mov bx,ax      ;这么做主要是用bx做16位的基址寻址 bx现在已经是光标坐标值了它是下一个可打印字符的位置
    ;获取待打印的字符
    mov ecx,[esp+36] ;pushad压入的8个通用寄存器4×8＝32字节,加上put_char的返回地址4字节,故esp+36字节
                     ;ecx就是put_char(uint8_t char_asci)中char_asci的数值
    ;判断输入的字符如果是特殊字符还需要做相应的处理
    cmp cl,0xd       ;CR是0x0d,LF是0x0a 回车符CR,换行符LF和backspace当作不可见字符
    jz  .is_carriage_return
    cmp cl,0xa       ;换行符LF
    jz .is_line_feed


    cmp cl,0x8 ;BS(backspace)的asc码是8
    jz .is_backspace
    jmp .put_other
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.is_backspace:
;;;;;;;;;;;;;; backspce 回退键 ;;;;;;;;;;;;;;;;
    dec bx
    shl bx,1        ;将光标左移一位*2

    mov byte [gs:bx],0x20 ;将低字节设置为空格 处理回退字符的ascii码
    inc bx                ;将bx加上1 这样就可以处理回退字符的属性位
    mov byte [gs:bx],0x07 ;设置黑底白字
    shr bx,1               ;右移一位恢复光标坐标，此时bx覆盖了新的空格
    jmp .set_cursor        ;获取到了光标的坐标后,将光标设置到新的位置

;;;;;;;;;;;;;;;;; 处理可见字符 ;;;;;;;;;;;;;;;;;;;;;;;;
.put_other:
    shl bx,1
    mov [gs:bx],cl      ;ascii字符本身
    inc bx
    mov byte [gs:bx],0x07 ;字符属性
    shr bx,1            ;恢复老的光标值
    inc bx              ;下一个光表值
    cmp bx,2000
    jl  .set_cursor     ; 若光标值小于2000,表示未写到显存的最后,则去设置新的光标值
                        					  ; 若超出屏幕字符数大小(2000)则换行处理
.is_line_feed:          ;处理换行符
.is_carriage_return:    ;处理回车符
                        ;将光标切换到行首 然后换到下一行

    xor dx,dx
    mov ax,bx
    mov si,80
    div si
    sub bx,dx

.is_carriage_return_end:    ;回车符cr处理完毕
    add bx,80
    cmp bx,2000
.is_line_feed_end:
    jl .set_cursor

.roll_screen:       ;若超出屏幕大小，开始滚屏
    cld
    mov ecx,960
    mov esi,0xb80a0
    mov edi,0xb8000
    rep movsd

;;;;;;;;;;;; 将最后一行填充为空白
    mov ebx,3840
    mov ecx,80
.cls:
    mov word [gs:ebx],0x0720    ;黑底白字空格键
    add ebx,2
    loop .cls
    mov bx,1920

.set_cursor:
    ;;;将光标设置为bx值
;;;;;;;; 先设置高8位 ;;;;;;;;;;;;;;;
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x03d5       ;通过读写数据端口0x3d5来获得或设置光标位置
    mov al,bh
    out dx,al



;;;;;;;;;;;; 设置低8位 ;;;;;;;;;;;;;;;;;;;;;;;
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl
    out dx,al
.put_char_done:
    popad
    ret

;;;;;;;;;;;;  put_int ;;;;;;;;;;;;;;;;;;;;;;;;;
global put_int
put_int:
    pushad
    mov ebp,esp
    mov eax,[ebp+4*9]
    mov edx,eax
    mov edi,7
    mov ecx,8
    mov ebx,put_int_buffer

.16based_4bits:
    add edx,0x0000000F
    cmp edx,9
    jp  .is_A2F
    add edx,'0'
    jmp .store
.is_A2F:
    sub edx,10

    add edx,'A'
.store:
    mov [ebx+edi],dl
    dec edi
    shr eax,4
    mov edx,eax
    loop .16based_4bits
.ready_to_print:
    inc edi
.skip_prefix_0:
    cmp edi,8
    je .full0
.go_on_skip:
    mov cl,[put_int_buffer+edi]
    inc edi
    cmp cl,'0'
    je  .skip_prefix_0
    dec edi
    jmp .put_each_num

.full0:
    mov cl,'0'
.put_each_num:
    push ecx
    call put_char
    add esp,4
    inc edi
    mov cl,[put_int_buffer+edi]
    cmp edi,8
    jl  .put_each_num
    popad
    ret













