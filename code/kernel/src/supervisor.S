/**
 * @file   supervisor.S
 *
 * @brief  Assembly implementation to setup interrupts and enter/exit user mode
 *
 * @date   03.07.2018
 * @author yanyingz
 */

.section ".text"

/**
 * vector table that is loaded by install_vector_table to address 0x0
 */
interrupt_vector_table:
  // ARM core does the following on interrupt:
  // 1) save cpsr into spsr
  // 2) set mode bits in current cpsr
  // 3) disable further interrupts of lower/same priority
  // 4) store return address in lr
  // 5) set pc to correct vector table entry below
  ldr pc, _reset_asm_handler                 // priority: 1st
  ldr pc, _undefined_instruction_asm_handler // priority: 6th
  ldr pc, _swi_asm_handler                   // priority: 6th
  ldr pc, _prefetch_abort_asm_handler        // priority: 5th
  ldr pc, _data_abort_asm_handler            // priority: 2nd
  ldr pc, _reset_asm_handler                 // priority: undefined
  ldr pc, _irq_asm_handler                   // priority: 4th
  ldr pc, _fiq_asm_handler                   // priority: 3rd

/**
 * SOFT vector table that is loaded by install_vector_table ABOVE
 * interrupt_vector_table
 */
// to make sure the addresses are aligned this must follow the
// actual vector table above!
_reset_asm_handler:                 .word reset_asm_handler
_undefined_instruction_asm_handler: .word undefined_instruction_asm_handler
_swi_asm_handler:                   .word swi_asm_handler
_prefetch_abort_asm_handler:        .word prefetch_abort_asm_handler
_data_abort_asm_handler:            .word data_abort_asm_handler
_irq_asm_handler:                   .word irq_asm_handler
_fiq_asm_handler:                   .word fiq_asm_handler

/**
 * called to install the vector table at address 0x0
 */
.global install_interrupt_table
install_interrupt_table:
  ldr r0, =interrupt_vector_table

  ldm r0!, {r1-r8}		//load hard vector
  mov r9, #0
  stm r9!, {r1-r8}
  
  ldm r0!, {r1-r7}		//load soft vector
  stm r9!, {r1-r7}
  bx lr
  

/************************************************************/
/* Assembly handlers for interrupts in the vector table     */
/************************************************************/

swi_asm_handler:
  sub sp, sp, #4
  push {r0-r12, lr}
  mrs r2, spsr
  str r2, [sp, #14*4]
  mov r1, sp
  ldr r0, [lr, #-4]
  bic r0, r0, #0xff000000 

  msr cpsr_c, #0x1f
  ldr r2, [sp]
  msr cpsr_c, #0x13

  mrs r3, cpsr
  bic r3, r3, #0x80
  msr cpsr, r3

  bl swi_c_handler

  ldr r2, [sp, #14*4]
  msr spsr, r2
  add sp, sp, #4
  pop {r1-r12, lr}
  add sp, sp, #4
  movs pc, lr


irq_asm_handler:
  ldr sp, =__irq_stack_top
  sub sp, sp, #4
  sub lr, lr, #4
  stmfd sp!, {r0-r12, lr}
  mrs r2, spsr
  str r2, [sp, #14*4]

  mrs r2, cpsr
  mov r0, sp
 
  orr r2, r2, #0x1f
  msr cpsr, r2
  stmfd r0!, {sp, lr}
 
  bic r2, r2, #0xC
  msr cpsr, r2
  mrs r3, spsr
  stmfd r0!, {r3, sp, lr}

  bic r2, r2, #1
  msr cpsr, r2
  mov sp, r0
  bl irq_c_handler

  mrs r2, cpsr

  orr r2, r2, #1
  msr cpsr, r2
  ldmfd r0!, {r1, sp, lr}
  msr spsr, r1

  orr r2, r2, #0x1f
  msr cpsr, r2
  ldmfd r0!, {sp, lr}

  bic r2, r2, #0xD
  msr cpsr, r2
  mov sp, r0

  ldr r2, [sp, #14*4]
  msr spsr, r2
  ldmfd sp!, {r0-r12, lr}
  add sp, sp, #4
  movs pc, lr

reset_asm_handler:
  b _start // just reset the kernel


/************************************************************
 * If you fall into one of these handlers something bad is
 * happening. bkpt will drop back into gdb so you can debug.
 ************************************************************/
undefined_instruction_asm_handler:
  bkpt

prefetch_abort_asm_handler:
  bkpt

data_abort_asm_handler:
  bkpt

fiq_asm_handler:
  bkpt


/************************************************************/
/* Initializes cpsr for user mode and jumps to user program */
/************************************************************/
.global enter_user_mode

enter_user_mode:
  push {r0-r12, lr}
  mrs r0, cpsr
  bic r0, r0, #0x9f
  orr r0, r0, #0x90
  msr cpsr, r0 
  ldr sp, =__user_stack_top
  bl __user_program
  pop {r0-r12, lr}  
  bx lr

