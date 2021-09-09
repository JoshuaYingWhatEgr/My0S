#include "memory.h"
#include "stdint.h"
#include "print.h"

#define PG_SIZE 4096 //4k

#define MEM_BITMAP_BASE 0xc009a00 //位图起始位置

#define K_HEAP_START 0xc0100000

struct pool {
    struct bitmap pool_bitmap; //用于管理物理内存
    uint32_t phy_addr_start;    //所管理物理内存的起始地址
    uint32_t pool_size;//内存池字节容量
};

struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;//用来给内核分配虚拟地址

/**
 * 初始化内存池
 */
static void mem_pool_init(uint32_t all_mem) {
    put_str(" mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;
    uint32_t used_mem = page_table_size + 0x100000;
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;
}

/**
 * 初始化内存池
 */
void mem_init(void) {

}