/**
 * @file   timer.c
 *
 * @brief  Implementation of routines for interacting with ARM timer
 *
 * @date   03.07.2018
 * @author yanyingz
 */

#include <timer.h>
#include <BCM2836.h>
#include <kstdint.h>
#include <printk.h>

/**@brief define the base register for timer*/
#define INTERRUPT_REG_BASE (MMIO_BASE_PHYSICAL + 0xb000)
/**@brief define the irq pending register*/
#define IRQ_PENDING (volatile uint32_t *) (INTERRUPT_REG_BASE + 0x200)
/**@brief define the irq enable register*/
#define IRQ_ENABLE (volatile uint32_t *) (INTERRUPT_REG_BASE + 0x218)
/**@brief define the irq disabler register*/
#define IRQ_DISABLE (volatile uint32_t *) (INTERRUPT_REG_BASE + 0x224)
/**@brief define the load register*/
#define LOAD_REG (volatile uint32_t *) (INTERRUPT_REG_BASE + 0x400)
/**@brief define the control register*/
#define CONTROL_REG (volatile uint32_t *) (INTERRUPT_REG_BASE + 0x408)
/**@brief define the irq clear register*/
#define IRQ_CLEAR_REG (volatile uint32_t *) (INTERRUPT_REG_BASE + 0x40c)

void timer_start(int freq) {
  *IRQ_ENABLE |= 0x1;
  *LOAD_REG = freq; 
  *CONTROL_REG |= ((1<<7)|(1<<5)|(1<<1));
  return;
}


void timer_stop(void) {
  *IRQ_DISABLE |= 0x1;
  *CONTROL_REG &= ~(1<<7);
  return;
}


int timer_is_pending(void) {
  return (*IRQ_PENDING & 0x1);
}


void timer_clear_pending(void) {
  *IRQ_CLEAR_REG = 0x1;
  return;
}
