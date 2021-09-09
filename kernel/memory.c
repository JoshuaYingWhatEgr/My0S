#include "memory.h"
#include "stdint.h"
#include "print.h"

#define PG_SIZE 4096 //4k

#define MEM_BITMAP_BASE 0xc009a000 //位图起始位置

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

    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;

    uint32_t kp_start = used_mem;
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    kernel_pool.pool_bitmap.bits = (void *) MEM_BITMAP_BASE;

    user_pool.pool_bitmap.bits = (void *) (MEM_BITMAP_BASE + kbm_length);
}

/**
 * 初始化内存池
 */
void mem_init(void) {

}