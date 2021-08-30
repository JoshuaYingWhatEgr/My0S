/*
 * 调用print.S 中的put_char
 * */
#ifndef  _LIB_KERNEL_PRINT_H
#define  _LIB_KERNEL_PRINT_H
#include "stdint.h"
void put_char(uint8_t char_asci); //从栈中获取参数 char_asci
void put_str(char* str);
#endif