/**
 * @file   syscalls.c
 *
 * @brief  Implementation of syscalls needed for newlib and 349 custom syscalls
 *
 * @date   03.08.2018
 * @author yanyingz
 */

#include <kstdint.h>
#include <uart.h>
#include <timer.h>
#include <printk.h>
#include <arm.h>
#include <syscalls.h>

/** @brief Global variable to keep track of where our heap ends */
char *heap_end = 0;

void syscall_exit(int status) {
  //print out exit status for the user program
  printk("Exit Status: %d\n", status);
  //hang with interrupts disabled
  disable_interrupts();
  while (1);
  return;
}


int syscall_write(int file, char *ptr, int len) {
  if (file != 1) return -1;
  int c = 0;
  while (c < len){
    uart_put_byte(ptr[c++]);
  }
  return 0;
}


/**
 * Reads until all len bytes are read or newline/return is found.
 * Detects backspace and EOL characters and handles them appropriately.
 *
 * The idea is to mimic normal terminal reading input, that is,
 * if the user hits backspace it should erase the previously read character.
 */
/*
int syscall_read(int file, char *ptr, int len) {
  if (file != 0) return -1;
  int count = 0;
  
  while (count < len){
    uint8_t ch = uart_get_byte();
    if (ch == 4){				//end-of-transmission character
      return count;
    }else if (ch == 8 || ch == 27){		//backspace or delete character
      count--;
      uart_put_byte('\b');
      uart_put_byte(' ');
      uart_put_byte('\b');
    }else if (ch == 10 || ch == 13){		//newline or carriage return
      ptr[count] = 10;
      printk("\n");
      return count;
    }else{					//read character
      ptr[count++] = ch;
      uart_put_byte(ch);
    }
  }
  return 0;
}*/
int syscall_read(int file, char *ptr, int len) {
  if (file != 0) return -1;
  int ct = 0;
  while(ct < len){
    uint8_t chr = uart_get_byte();
    switch (chr) {
      case 4: //EOL
        return ct;
        break; //backspace
      case 8:
      case 127:
        if(ct > 0) {
          ct --;
          uart_put_byte('\b');
          uart_put_byte(' ');
          uart_put_byte('\b');
        }
        break;
      case 13:
        chr = uart_get_byte(); //dump the /r
      case 10:
        //new line
        ptr[ct++] = 10;
        printk("\n");
        return ct;
        break;
      default:
        ptr[ct++] = chr;
        uart_put_byte(chr);
    }
  }
  return ct;
}

int syscall_servo_enable(uint8_t channel, uint8_t enabled) {
  return -1;
}

int syscall_servo_set(uint8_t channel, uint8_t angle) {
  return -1;
}

/*****************************************************************************/
/* TA system call implementations:                                           */
/*****************************************************************************/

void *syscall_sbrk(int incr) {
  extern char __heap_low; // Defined by the linker
  extern char __heap_top; // Defined by the linker
  char *prev_heap_end;

  if (heap_end == 0) {
    // Initialize heap_end if this is the first time sbrk was called
    heap_end = &__heap_low;
  }

  prev_heap_end = heap_end;
  if (heap_end + incr > &__heap_top) {
    // Heap and stack collision, return error
    return (void *) -1;
  }

  // update heap_end and return old heap_end
  heap_end += incr;
  return (void *) prev_heap_end;
}

int syscall_close(int file) {
  return -1;
}

int syscall_fstat(int file, void *st) {
  return 0;
}

int syscall_isatty(int file) {
  return 1;
}

int syscall_lseek(int file, int ptr, int dir) {
  return 0;
}
