#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"


#define PG_SIZE 4096 //4k

#define MEM_BITMAP_BASE 0xc009a000 //位图起始位置

#define K_HEAP_START 0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)

#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

struct pool {
    struct bitmap pool_bitmap; //用于管理物理内存
    uint32_t phy_addr_start;    //所管理物理内存的起始地址
    uint32_t pool_size;//内存池字节容量
};

struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;//用来给内核分配虚拟地址

/**
 * 在pf表示的虚拟内存池中申请pg_cnt个虚拟页,成功则返回虚拟页的起始地址 否则返回NULL
 */
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if (pf == PF_KERNEL) {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1) {
            return NULL;
        }
        while (cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    } else {
        //用户内存池
    }
    return (void *) vaddr_start;
}

/***
 * 得到虚拟地址addr对应的pte指针
 */
uint32_t *pte_ptr(uint32_t vaddr) {
    /**
     * 先访问到页表自己 再用页目录项pde(页目录内页表的索引)做为pte的索引访问到页表
     * 再用pte的索引作为页内偏移
     */
    uint32_t *pte = (uint32_t *) (0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

/**
 * 得到虚拟地址vaddr对应的pde的指针
 */
uint32_t *pde_ptr(uint32_t vaddr) {
    uint32_t *pde = (uint32_t *) ((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}

/**
 * 再m_pool指向的物理内存池中分配1个物理页
 * 成功则返回页框的物理地址,失败则返回NULL
 */
static void *palloc(struct pool *m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1) {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);//将bit_idx位置为1
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void *) page_phyaddr;
}

/**
 * 页表中添加虚拟地址vaddr和物理地址page_phyaddr的映射
 */
static void page_table_add(void *_vaddr, void *_page_phyaddr) {
    uint32_t vaddr = (uint32_t) _vaddr, page_phyaddr = (uint32_t) _page_phyaddr;
    uint32_t *pde = pde_ptr(vaddr);
    uint32_t *pte = pte_ptr(vaddr);
    /**
     * 先在页目录内判断目录项的P位若为1则表示该表存在
     */
    if (*pde & 0x00000001) {
        //页目录项和页表项的第0位为p,判断目录项是否存在
        ASSERT(!(*pte & 0x00000001));

        if (!(*pte & 0x00000001)) {
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        } else {
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    } else {
        //页目录项不存在 先创建页目录在创建页表项
        /**
         * 页表中用到的页框一律从内核空间分配
         */
        uint32_t pde_phyaddr = (uint32_t) palloc(&kernel_pool);

        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

        /* 分配到的物理页地址pde_phyaddr对应的物理内存清0,
       * 避免里面的陈旧数据变成了页表项,从而让页表混乱.
       * 访问到pde对应的物理地址,用pte取高20位便可.
       * 因为pte是基于该pde对应的物理地址内再寻址,
       * 把低12位置0便是该pde对应的物理页的起始*/
        memset((void *) ((int) pte & 0xfffff000), 0, PG_SIZE);

        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

/**
 * 分配pg_cnt个页空间,成功则返回起始的虚拟地址 否则返回NULL
 */
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    /***********   malloc_page的原理是三个动作的合成:   ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
***************************************************************/
    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL) {
        return NULL;
    }

    uint32_t vaddr = (uint32_t) vaddr_start, cnt = pg_cnt;
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    /**
     * 因为虚拟地址是连续的 而物理地址是不连续的 所以需要逐个映射
     */
    while (cnt-- > 0) {
        void *page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL) {
            return NULL;
        }
        page_table_add((void *) vaddr, page_phyaddr);// 在页表中做映射
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}

/**
 * 从内核物理内存池中申请pg_cnt页内存,成功则返回其虚拟地址 否则返回NULL
 */
void* get_kernel_pages(uint32_t pg_cnt) {
    void* vaddr = malloc_page(PF_KERNEL,pg_cnt);
    if(vaddr != NULL) { // 若分配的地址不为空,将页框清0后返回
        memset(vaddr,0,pg_cnt*PG_SIZE);
    }
    return vaddr;
}

/**
 * 初始化内存池
 */
static void mem_pool_init(uint32_t all_mem) {
    put_str(" mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;
    uint32_t used_mem = page_table_size + 0x100000;
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;//剩余物理页数

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

    put_str("kernel_pool_bitmap_start:");
    put_int((int) kernel_pool.pool_bitmap.bits);
    put_str("\n");
    put_str("      kernel_pool_bitmap_end:");
    put_int((int) kernel_pool.pool_bitmap.bits + kernel_pool.pool_bitmap.btmp_bytes_len);
    put_str("\n");
    put_str("       kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("       kernel_pool_phy_addr_end:");
    put_int(kernel_pool.phy_addr_start + kernel_pool.pool_size);
    put_str("\n");
    put_str("      user_pool_bitmap_start:");

    put_int((int) user_pool.pool_bitmap.bits);
    put_str("\n");
    put_str("      user_pool_bitmap_end:");
    put_int((int) user_pool.pool_bitmap.bits + user_pool.pool_bitmap.btmp_bytes_len);
    put_str("\n");
    put_str("       user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_str("\n");
    put_str("       user_pool_phy_addr_end:");
    put_int(user_pool.phy_addr_start + user_pool.pool_size);
    put_str("\n");

    /**
     *  将位图置0
     */
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;

    kernel_vaddr.vaddr_bitmap.bits = (void *) (MEM_BITMAP_BASE + kbm_length + ubm_length);
    kernel_vaddr.vaddr_start = K_HEAP_START;
    put_str("     kernel_vaddr.vaddr_bitmap.start:");
    put_int((int) kernel_vaddr.vaddr_bitmap.bits);
    put_str("\n");
    put_str("     kernel_vaddr.vaddr_bitmap.end:");
    put_int((int) kernel_vaddr.vaddr_bitmap.bits + kernel_vaddr.vaddr_bitmap.btmp_bytes_len);
    put_str("\n");

    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}

/**
 * 初始化内存池
 */
void mem_init(void) {
    put_str("mem_init start \n");
    uint32_t mem_bytes_total = (*(uint32_t *) (0xb00));
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done \n");
}