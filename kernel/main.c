#include "init.h"
#include "../lib/kernel/print.h"
#include "debugg.h"
#include "memory.h"
#include "../thread/thread.h"
#include "interrupt.h"
#include "../device/console.h"

void k_thread_a(void *);

void k_thread_b(void *);

/**
 * 程序运行开中断以及线程调度导致字符打印混乱
 * @return
 */
int main(void) {
    put_str("I am kernel\n");

    init_all();

//    void* addr = get_kernel_pages(3);
//    put_str("\n get_kernel_pages start vaddr is ");
//
//    put_int((uint32_t) addr); //打印分配的虚拟页地址
//    put_str("\n");

    thread_start("k_thread_a", 31, k_thread_a, "argA");
    thread_start("k_thread_b", 8, k_thread_b, "argB");

    intr_enable();//打开时钟中断
    while (1) {
        console_put_str("Main ");
    }
    return 0;
}

void k_thread_a(void *arg) {
    char *par = arg;

    while (1) {
        console_put_str(par);
    }
}

void k_thread_b(void *arg) {
    char *par = arg;

    while (1) {
        console_put_str(par);
    }
}

