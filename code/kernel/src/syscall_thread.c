/** @file syscall_thread.c
 *  @brief Implementation of multi-threading system calls
 *
 *  @author yanyingz
 */

#include <kstdint.h>
#include "mutex.h"
#include <printk.h>
#include <uart.h>
#include <arm.h>
#include <uart.h>
#include <timer.h>
#include <supervisor.h>
#include <swi_num.h>
#include <syscalls.h>

/**@brief total thread numbers: 31 tasks + 1 idle function*/
#define THREAD_NUM	32
/**@brief stack size for one task*/
#define TCB_STACK_SIZE	1024
/**@brief save context for one task*/
#define TCB_REG_NUM	20
/**@brief define runnable status*/
#define RUNNABLE	1
/**@brief define waiting status*/
#define WAITING		0
/**@brief define running status*/
#define RUNNING		2
/**@brief define the index for spsr in svc mode*/
#define SPSR_SVC	0
/**@brief define the index for sp in svc mode*/
#define SP_SVC		1
/**@brief define the index for lr in svc mode*/
#define LR_SVC		2
/**@brief define the index for sp in user mode*/
#define SP_USER		3
/**@brief define the index for lr in user mode*/
#define LR_USER		4
/**@brief define the index for lr in irq mode*/
#define LR_IRQ		18
/**@brief define the index for spsr in irq mode*/
#define SPSR_IRQ	19


typedef struct TCB{
  
  uint32_t tcb_stack[TCB_STACK_SIZE];
  //In order: spsr_svc, sp_svc, lr_svc, sp_user, lr_user, r0-r12, lr_irq, spsr_irq
  uint32_t tcb_regs[TCB_REG_NUM];
  uint32_t wakeup;
  uint32_t execution;
  uint32_t sleep;
  uint32_t computation;
  uint32_t period;
  uint32_t priority;
  uint32_t curr_priority;
  uint32_t status;

} tcb_t;

/**@brief tcb pool*/
tcb_t tcb_list[THREAD_NUM];
/**@brief mutex pool*/
uint32_t mutex_list[THREAD_NUM];
/**@brief count the number of mutexes*/
uint32_t mutex_index = 0;
/**@brief pointer to the current running tcb block*/
tcb_t* current_task;
/**@brief using 32-bit integers to represent runnable pool and waiting pool*/
uint32_t runnable_pool = 0;
uint32_t waiting_pool = 0;
uint32_t mutex_ceiling = 31;

/**@brief system timer*/
uint32_t time;
/**@brief hard-coded utilization table for 0 to 32 tasks*/
float utilization_list[33] = 
		  {0.0, 1.0, 0.828427, 0.779763, 0.756828, 0.743492, 0.734772, 0.728627, 0.724062,
		   0.720538, 0.717735, 0.715452, 0.713557, 0.711959, 0.710593, 0.709412, 0.708381,
		   0.707472, 0.706666, 0.705946, 0.705298, 0.704713, 0.704182, 0.703698, 0.703254, 
		   0.702846, 0.702469, 0.702121, 0.701798, 0.701497, 0.701217, 0.700955, 0.700709};

int is_runnable(uint32_t prio){
  return ((runnable_pool >> prio) & 1);
}
void set_run_pool(uint32_t prio){
  runnable_pool |= (1 << prio);
}
void clear_run_pool(uint32_t prio){
  runnable_pool &= (~(1 << prio));
}
int is_waiting(uint32_t prio){
  return ((waiting_pool >> prio) & 1);
}
void set_wait_pool(uint32_t prio){
  waiting_pool |= (1 << prio);
}
void clear_wait_pool(uint32_t prio){
  waiting_pool &= (~(1 << prio));
}

int thread_init(thread_fn idle_fn, uint32_t *idle_stack_start) {
  if (idle_fn == NULL || idle_stack_start == NULL) return -1;

  tcb_t *c_tcb = &tcb_list[31];
  c_tcb->priority = 31;
  c_tcb->curr_priority = 31;
  c_tcb->computation = 100000;
  c_tcb->period = 1;
  c_tcb->status = RUNNABLE;
  c_tcb->wakeup = 0;
  c_tcb->execution = 0;
  c_tcb->sleep = 0;
  c_tcb->tcb_regs[SP_USER] = (uint32_t)idle_stack_start;
  c_tcb->tcb_regs[SPSR_IRQ] = 0x10;
  c_tcb->tcb_regs[SPSR_SVC] = 0x10;
  c_tcb->tcb_regs[LR_IRQ] = (uint32_t) idle_fn;
  c_tcb->tcb_regs[LR_USER] = (uint32_t) idle_fn;
  c_tcb->tcb_regs[SP_SVC] = (uint32_t)(c_tcb->tcb_stack + 1023);
  return 0;
}

int thread_create(thread_fn fn, uint32_t *stack_start,
                  unsigned int prio, unsigned int C, unsigned int T) {
  if (fn == NULL || stack_start == NULL) return -1;

  tcb_t *c_tcb = &tcb_list[prio];
  c_tcb->priority = prio;
  c_tcb->curr_priority = prio;
  c_tcb->computation = C;
  c_tcb->period = T;
  c_tcb->status = RUNNABLE;
  c_tcb->wakeup = 0;
  c_tcb->sleep = 0;
  c_tcb->execution = 0;
  c_tcb->tcb_regs[SP_USER] = (uint32_t)stack_start;
  c_tcb->tcb_regs[SPSR_IRQ] = 0x10;
  c_tcb->tcb_regs[SPSR_SVC] = 0x10;
  c_tcb->tcb_regs[LR_IRQ] = (uint32_t) fn;
  c_tcb->tcb_regs[LR_USER] = (uint32_t) fn;
  c_tcb->tcb_regs[SP_SVC] = (uint32_t) (c_tcb->tcb_stack + 1023);
  set_run_pool(prio);
  //printk("c = %d, co = %d, t = %d, to = %d\n", C, c_tcb->computation, T, c_tcb->period);
  return 0;
}

uint32_t find_next_task(){
  uint32_t prio = current_task->priority;
  uint32_t period = current_task->period;
  uint32_t exec = current_task->execution;

  if (current_task->status == RUNNING){
    current_task->execution++;
    current_task->sleep++;
    //finished one round of task
    if (exec >= current_task->computation){	
      current_task->status = WAITING;
      clear_run_pool(prio);
      set_wait_pool(prio);
      current_task->execution = 0;
      current_task->wakeup += period;
    }else{
      set_run_pool(prio);
      clear_wait_pool(prio);
    }
  }else if (current_task->status == WAITING){		
   //suspended to wait until next period
   set_wait_pool(prio);
   clear_run_pool(prio);
   current_task->execution = 0;
   current_task->wakeup += period;
  }
  //tasks in the waiting pool whose period has ended
  int i;
  for (i = 0; i < 31; i++){
    if (is_waiting(i)&&(time >= tcb_list[i].wakeup)){
      tcb_list[i].status = RUNNABLE;
      tcb_list[i].execution = 0;
      set_run_pool(i);
      clear_wait_pool(i);
    }
  }
  //tasks in the runnable pool that has higher priority
  int j;
  for (j = 0; j < 31; j++){
    if (is_runnable(j)&&(j <= prio)){
      return j;
    }
  }
  return 31;
}

uint32_t* call_scheduler(uint32_t *sp) {

  time++;
  //printk("time:%d,runpool:%d, waitpool:%d, now:%d, nowxe:%d\n", time,runnable_pool,waiting_pool,current_task->priority, current_task->execution);
  uint32_t next = find_next_task();

  //save register content into tcb
  int i;
  for (i = 0; i < TCB_REG_NUM; i++){
    current_task->tcb_regs[i] = sp[i];
  }
  if(current_task != NULL && next != current_task->priority && (!is_waiting(current_task->priority))){
  current_task->status = RUNNABLE;
  clear_wait_pool(current_task->priority);
  set_run_pool(current_task->priority);
  }
  //switch task
  current_task = &tcb_list[next];
  current_task->status = RUNNING;
  clear_wait_pool(current_task->priority);
  clear_run_pool(current_task->priority);
  
  return (current_task->tcb_regs);
}

int mutex_init(mutex_t *mutex, unsigned int max_prio) {
    if (mutex == NULL) return -1;

    mutex_list[mutex_index] = (uint32_t)mutex;
    mutex_index++;

    mutex->lock = 0;
    mutex->ceiling = max_prio;
    mutex->thread = -1;
    return 0;
}

void mutex_lock(mutex_t *mutex) {
    while (mutex->lock){;}

    disable_interrupts();
    if ((current_task->curr_priority >= mutex->ceiling) && 
        (current_task->curr_priority < mutex_ceiling)){
      mutex->lock = 1;
      mutex->thread = current_task->priority;
      if (mutex->ceiling < mutex_ceiling){
        mutex_ceiling = mutex->ceiling;
      }
    }
    enable_interrupts();
    return;
}

void mutex_unlock(mutex_t *mutex) {
<<<<<<< HEAD
    disable_interrupts();
    mutex->lock = 0;
    mutex->thread = -1;
    current_task->curr_priority = current_task->priority;

    int i;
    int sum = 0;
    for (i = 0; i < mutex_index; i++){
      mutex_t *m = (mutex_t *)mutex_list[i];
      if (m->lock) sum += m->ceiling;
    }
    mutex_ceiling = sum;
    enable_interrupts();
    return;
}

void wait_until_next_period(void) {
    current_task->status = WAITING;
    volatile uint32_t *status = &current_task->status;
    while(*status == WAITING){;}
    return;
}

unsigned int get_time(void) {
    return time;
}

int scheduler_start(void) {
    current_task = &tcb_list[31];      
    current_task->execution = 0;
    current_task->status = RUNNING;

    float utest = 0.0;
    int i;
    int thr_count = 0;
    for (i = 0; i < 31; i++){
      if (is_runnable(i)){
	thr_count++;
	//printk("i = %d, c = %d, T = %d", i, tcb_list[i].computation, tcb_list[i].period);
        float u = ((float)tcb_list[i].computation)/((float)(tcb_list[i].period));
	utest += u;
        printk("i = %d, u = %d, utest = %d\n", i, u, utest);
      }
    }
    if (utest > utilization_list[thr_count]) return -1;

    time = 0;
    enable_interrupts();
    timer_start(1000);
    while(1){;}
    return 0;
}

unsigned int get_priority(void) {
    return current_task->priority;
}

void spin_wait(unsigned ms) {
    current_task->sleep = 0;
    volatile uint32_t *s = &current_task->sleep;
    while(*s < ms){;}
    return;
}
