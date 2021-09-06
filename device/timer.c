#include "timer.h"
#include "io.h"
#include "stdint.h"
#include "print.h"

#define IRQ0_FREQUENCY 1        //初始计数值
#define INPUT_FREQUENCY 1193180 //计数器的工作频率
#define COUNTER0_VALUE  INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT  40
#define COUNTER0_NO     0
#define COUNTER_MODE    2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

static void frequency_set(uint8_t counter_port, uint8_t count_no,
                          uint8_t rwl, uint8_t counter_mode, uint16_t counter_value) {

    /**
     * 往控制字寄存器端口0x43中写入控制字
     */
    outb(PIT_CONTROL_PORT, (uint8_t) (counter_no << 6 | rwl << 4 | counter_mode << 1));

    /**
     * 先写入counter_value的低8位
     */
    outb(counter_port, (uint8_t) counter_value)

    /**
     * 再写入高8位
     */
    outb(counter_port, (uint8_t) counter_value >> 8);
}

/**
 * 初始化pit计数器
 */
void timer_init() {

    put_str("timer_init start\n");
    /**
     * 设置8253位定时周期
     */
    frequency_set(CONTRER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    put_str("timer_init done\n");
}