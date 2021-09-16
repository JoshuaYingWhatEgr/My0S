#include "thread.h"
#include "string.h"
#include "stdint.h"
#include "global.h"
#include "memory.h"
#include "interrupt.h"
#include "debugg.h"

#define PG_SIZE 4096

struct task_struct *main_thread;//主线程的pcb
struct list thread_ready_list;//就绪队列
struct list thread_all_list;//所有任务队列
static struct list_elem *thread_tag;//用于保存队列中的线程节点

extern void switch_to(struct task_struct *cur, struct task_struct *next);

/* 获取当前线程的pcb指针 */
struct task_struct *running_thread() {
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g" (esp));
    /* 取esp整数部分,即pcb起始地址 */
    return (struct task_struct *) (esp & 0xfffff000);
}

/**
 * 由kernel_thread去执行function(func_arg);
 */
static void kernel_thread(thread_func *function, void *func_arg) {
    /* 首先开中断,避免后面的时钟中断被屏蔽*/
    intr_disable();
    function(func_arg);
}

/**
 * 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置
 * */
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg) {
    /*先预留中断使用栈的空间 */
    pthread->self_kstack -= sizeof(struct intr_stack);

    /*留出线程栈空间 */
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack *kthread_stack = (struct thread_stack *) pthread->self_kstack;
    kthread_stack->eip = kernel_thread;//需要执行的函数的地址
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    //线程的具体逻辑还没执行 所以在线程执行前将寄存器初始化为0
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

/**
 * 初始化线程基本信息
 */
void init_thread(struct task_struct *pthread, char *name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);

    if (pthread == main_thread) {//如果当前线程是主线程
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }

    /*self_stack是线程自己在内核态下使用的栈顶地址*/
    pthread->self_kstack = (uint32_t) ((uint32_t) pthread + PG_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->stack_magic = 0x19870916;
}

/**
 * 创建优先级为prio的线程,线程名为name,
 * 线程所执行的函数是function(func_arg);
 */
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg) {
    /*内核进程和用户进程的pcb都位于内核中*/
    struct task_struct *thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    /* 确保之前不在队列中 */
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));

    /* 加入就绪队列 */
    list_append(&thread_ready_list, &thread->general_tag);

    /* 确保之前不在队列中 */
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    /* 加入全部线程队列 */
    list_append(&thread_all_list, &thread->all_list_tag);
    return thread;
}

/**
 * 实现任务调度
 * */
void schedule() {

    ASSERT(intr_get_status == INTR_OFF);

    struct task_struct *cur = running_thread();
    if (cur->status == TASK_RUNNING) {
        //若此线程的时间片到了,就将其加入对尾
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    } else {
        /* 若此线程需要某事件发生后才能继续上cpu运行,
     不需要将其加入队列,因为当前线程不在就绪队列中。*/
    }

    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;
    /* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct *next = elem2entry(
    struct task_struct,general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur, next);//切换任务
}


/* 将kernel中的main函数完善为主线程 */
static void make_main_thread(void) {
    /* 因为main线程早已运行,咱们在loader.S中进入内核时的mov esp,0xc009f000,
就是为其预留了tcb,地址为0xc009e000,因此不需要通过get_kernel_page另分配一页*/
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    /* main函数是当前线程,当前线程不在thread_ready_list中,
 * 所以只将其加在thread_all_list中. */
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void thread_init(void) {
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    /* 将当前main函数创建为线程 */
    make_main_thread();
    put_str("thread_init down\n");
}



