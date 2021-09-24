#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"

/**
 * 负责初始化所有模块
 * */
void init_all() {
    put_str("init_all\n");

    /**
     * 初始化中断
     */
    idt_init();

    /**
     * 启动计数器
     */
    timer_init();

    /**
     * 初始化内存池
     */
    mem_init();

    /**
     * 初始化线程
     */
     thread_init();
}
