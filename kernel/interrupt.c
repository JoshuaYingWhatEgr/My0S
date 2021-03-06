#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define PIC_M_CTRL 0x20           // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21           // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0           // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1           // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x21     // 目前总共支持的中断数

#define EFLAGS_IF 0x0000020     //将eflag寄存器的if位置为1
#define GET_EFLAGS(EFLAG_VAR)  asm volatile("pushfl; popl %0" : "=g" (EFLAG_VAR))

/*中断门描述符结构体*/
struct gate_desc {
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;   //此项为双字计数字段，是门描述符中的第4字节。此项固定值，不用考虑
    uint8_t attribute;
    uint16_t func_offset_high_word;
};

// 静态函数声明,非必须
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function);

static struct gate_desc idt[IDT_DESC_CNT];   // idt是中断描述符表,本质上就是个中断门描述符数组

char *intr_name[IDT_DESC_CNT];
intr_handler idt_table[IDT_DESC_CNT];
extern intr_handler intr_entry_table[IDT_DESC_CNT];        // 声明引用定义在kernel.S中的中断处理函数入口数组

/* 初始化可编程中断控制器8259A */
static void pic_init(void) {

    /* 初始化主片 */
    outb(PIC_M_CTRL, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_M_DATA, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
    outb(PIC_M_DATA, 0x04);   // ICW3: IR2接从片.
    outb(PIC_M_DATA, 0x01);   // ICW4: 8086模式, 正常EOI

    /* 初始化从片 */
    outb(PIC_S_CTRL, 0x11);    // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_S_DATA, 0x28);    // ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
    outb(PIC_S_DATA, 0x02);    // ICW3: 设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA, 0x01);    // ICW4: 8086模式, 正常EOI

    /* 打开主片上IR0,也就是目前只接受时钟产生的中断 */
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    put_str("   pic_init done\n");
}

/* 创建中断门描述符 */
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t) function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t) function & 0xFFFF0000) >> 16;
}

/*初始化中断描述符表*/
static void idt_desc_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("   idt_desc_init done\n");
}


/**
 * 通用中断处理函数 用在异常出现时的处理
 * @param vec_nr 中断向量号
 */
static void general_intr_handler(uint8_t vec_nr) {

    if (vec_nr == 0x27 || vec_nr == 0x2f) { //处理伪中断
        return;
    }

    /* 将光标设置为0,清出一片打印的区域方便阅读 */
    set_cursor(0);
    int cursor_pos = 0;
    while (cursor_pos < 320) {
        put_char(' ');
        cursor_pos++;
    }
    /* 重置光标为屏幕左上角 */
    set_cursor(0)
    put_str("!!!!!!!      excetion message begin  !!!!!!!!\n");
    set_cursor(88);//从第二行第8个字符开始打印
    put_str(intr_name[vec_nr]);
    if (vec_nr == 14) {
        //缺页异常
        int page_fault_vaddr = 0;
        asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));      // cr2是存放造成page_fault的地址
        put_str("\npage fault addr is ");
        put_int(page_fault_vaddr);
    }
    put_str("\n!!!!!!!      excetion message end    !!!!!!!!\n");
    //死循环悬停在这里
    while (1);
}

/**
 * 完成一般中断处理函数和异常名称注册
 */
static void exception_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        idt_table[i] = general_intr_handler;

        intr_name[i] = "unknow";//intr_name[0]~intr_name[19]都定义正确的异常名
        //intr_name[20]~intr_name[32]一律为unknnow不指向空
    }

    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

/**
 * 开中断并返回开中断前的状态
 */
enum intr_status intr_enable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        return old_status;
    } else {
        old_status = INTR_OFF;
        asm volatile("sti"); //开中断
        return old_status;
    }
}

/**
 * 关中断并返回之前关中断的状态
 */
enum intr_status intr_disable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        asm volatile("cli" : : : "memory"); //将if位置为0
        return old_status;
    } else {
        old_status = INTR_OFF;
        return old_status;
    }
}

/**
 * 将中断状态设置为status
 */
enum intr_status intr_set_status(enum intr_status status) {
    return status & INTR_ON ? intr_enable() : intr_disable();
}

/**
 * 获取当前中断状态
 */
enum intr_status intr_get_status() {
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

/* 注册时钟中断处理程序 */
static void register_handler(uint8_t vector_no, intr_handler function) {
    idt_table[vector_no] = function;
}

/*完成有关中断的所有初始化工作*/
void idt_init() {
    put_str("idt_init start\n");
    idt_desc_init();       // 初始化中断描述符表

    exception_init();      //注册通用的中断处理函数
    pic_init();           // 初始化8259A

    /* 加载idt */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t) (uint32_t) idt << 16));
    asm volatile("lidt %0" : : "m" (idt_operand));
    put_str("idt_init done\n");
}

