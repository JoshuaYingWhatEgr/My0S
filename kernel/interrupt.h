#ifndef _KERNEL_INTERRUPT_H
#define _KERNEL_INTERRUPT_H

void idt_init();

typedef void *intr_handler;
#endif