#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic_spin(char* file_name, int line, const char* func, const char* condition) {

    intr_disable(); //先关中断

    put_str("\n\n\n!!!!! error !!!!!\n");
    put_str("filename:");put_str(file_name);put_str("\n");
    put_str("line:0x");put_int(line);put_str("\n");
    put_str("function:");put_str((char*)func);put_str("\n");
    put_str("condition:");put_str((char*)condition);put_str("\n");
    while(1);//死循环悬停在这里
}