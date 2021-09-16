;任务切换
[bits 32]
section .text
global switch_to
switch_to:
;;;;;;;;;;; 备份当前线程的环境 ;;;;;;;;;;;;;;;
    ;栈中此处是返回地址
    push esi
    push edi
    push ebx
    push ebp

    mov eax,[esp+20];得到栈中的参数cur = esp+20
    mov [eax],esp

;;;;;;;;;;;; 恢复当前线程的环境 ;;;;;;;;;;;;;;;
    mov eax,[esp+24];得到栈中参数 next = [esp+24]
    mov esp,[eax];pcb的第一个成员 self_kstack

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret  ; 返回到上面switch_to下面的那句注释的返回地址,
        				 ; 未由中断进入,第一次执行时会返回到kernel_thread
