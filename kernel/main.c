#include "init.h"
#include "../lib/kernel/print.h"
#include "debugg.h"

int main(void) {
    put_str("I am kernel\n");

    init_all();
//    asm volatile("sti");//临时开中断
    ASSERT(1 == 2);
    while (1);
    return 0;
}
