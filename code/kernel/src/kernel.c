/**
 * @file   kernel.c
 *
 * @brief  entry point to the raspberry pi kernel
 *
 * @date   03.07.2018
 * @author yanyingz
 */

#include <kstdint.h>
#include <printk.h>
#include <uart.h>
#include <arm.h>
#include <uart.h>
#include <timer.h>
#include <supervisor.h>
#include <swi_num.h>
#include <syscalls.h>
/**
 * @brief The kernel entry point
 */
void kernel_main(void) {

  uart_init();
  install_interrupt_table();
  while (1){
    enter_user_mode();
  }
}


/**
 * @brief Handler called when an IRQ occurs
 * @param sp is a pointer that points to the current context stack
 * @return the pointer to the new context to resume
 */
uint32_t *irq_c_handler(uint32_t *sp) {
  timer_clear_pending();
  return call_scheduler(sp);
}


/**
 * @brief Handles the given swi_num
 *
 * @param swi_num the swi number passed in from swi_asm_handler
 * @param args pointer to arguments array, set up by the swi_asm_handler
 *
 * @return the return result of the syscall
 */
void *swi_c_handler(int swi_num, int *args, int more) {
  switch(swi_num){
    case (SWI_SBRK):
	return syscall_sbrk(args[0]);
    case (SWI_WRITE):
	return (void *)syscall_write(args[0], (char*)args[1], args[2]);
    case (SWI_READ):
	return (void *)syscall_read(args[0], (char*)args[1], args[2]);
    case (SWI_EXIT):
	syscall_exit(args[0]);
	return (void*)-1;
    case (SWI_CLOSE):
	return (void *)syscall_close(args[0]);
    case (SWI_FSTAT):
	return (void *)syscall_fstat(args[0], (void*)args[1]);
    case (SWI_ISATTY):
	return (void *)syscall_isatty(args[0]);
    case (SWI_LSEEK):
	return (void *)syscall_lseek(args[0], args[1], args[2]);
    case (SWI_ADC_START):
    case (SWI_ADC_STOP):
    case (SWI_THR_INIT):
	return (void *)thread_init((thread_fn)args[0], (uint32_t *)args[1]);
    case (SWI_THR_CREATE):
	return (void *)thread_create((thread_fn)args[0], (uint32_t *)args[1],
		args[2], args[3], more);
    case (SWI_MUT_INIT):
	return (void*)mutex_init((mutex_t *)args[0], args[1]);
    case (SWI_MUT_LOK):
	mutex_lock((mutex_t *)args[0]);
	return (void *)-1;
    case (SWI_MUT_ULK):
	mutex_unlock((mutex_t *)args[0]);
	return (void *)-1;
    case (SWI_WAIT):
	wait_until_next_period();
	return (void *)-1;
    case (SWI_TIME):
	return (void*)get_time();
    case (SWI_SCHD_START):
	return (void *)scheduler_start();
    case (SWI_PRIORITY):
	return (void*)get_priority();
    case (SWI_SPIN_WAIT):
	spin_wait(args[0]);
	return (void *)-1;
    default: 
	return (void *)-1;
  }
}
