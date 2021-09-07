#ifndef __KERNEL_DEBUGG_H
#define __KERNEL_DEBUGG_H

void panic_spin(char* file_name, int line, const char* func, const char* condition);

/**
 * __VA_ARGS__代表所有与省略号相应的参数
 */
#define PANIC(...) panic_spin(__FILE__,__LINE__,__func__,__VA_ARGS__);

#ifdef NDEBUG
    #define  ASSERT(CONDITION)((void)0)
#else
    #define ASSERT(CONDITION)
    /* CONDITION条件为真时什么也不做 */
        if(CONDITION) {} else {
        PANIC(#CONDITION);
    }
#endif

#endif