/**
 * @file   main.c
 *
 * @brief  Test that emulates deadlock avoidance example from lectire
 * @author Neil Ryan <nryan@andrew.cmu.edu>
 */

#include <stdio.h>
#include <syscall_thread.h>
#include <stdlib.h>
#include <mutex.h>

/** @brief thread user space stack size - 4KB */
#define USR_STACK_WORDS 1024

uint32_t idle_stack[USR_STACK_WORDS];
uint32_t thread1_stack[USR_STACK_WORDS];
uint32_t thread2_stack[USR_STACK_WORDS];
mutex_t mutex1;
mutex_t mutex2;

/** @brief Prints basic status information of a thread
 *
 *  @param name   the thread's base priority
 *  @param count  the thread's counter variable
 */
void print_status(int name, int counter) {
  printf("t = %d --- Task: %d Count: %d, Curr_Prio; %d\n",
        get_time(), name, counter, get_priority());
}

/** @brief Default idle thread which just loops infinitely
 */
void idle_thread(void) {
  while(1);
}

/** @brief Basic thread which just increments a counter
 */
void thread_1(void) {
  int cnt = 0;
  print_status(1, cnt++);
  wait_until_next_period(); // Let t2 run first
  print_status(1, cnt++);
  mutex_lock(&mutex1);
  print_status(1, cnt++);
  spin_wait(10);
  print_status(1, cnt++);
  mutex_lock(&mutex2);
  print_status(1, cnt++);
  spin_wait(10);
  mutex_unlock(&mutex2);
  mutex_unlock(&mutex1);
  exit(1);
}

/** @brief Basic thread which just increments a counter
 */
void thread_2(void) {
  int cnt = 0;
  print_status(2, cnt++);
  mutex_lock(&mutex2);
  print_status(2, cnt++);
  spin_wait(595);  // Interrupted by T1
  print_status(2, cnt++); // Priority elevated
  mutex_lock(&mutex1);
  print_status(2, cnt++);
  spin_wait(95);
  mutex_unlock(&mutex1);
  print_status(2, cnt++);
  mutex_unlock(&mutex2);
  spin_wait(5);
  print_status(2, cnt++); // Priority restore
  wait_until_next_period();
  exit(2);
}

int main(void) {
  int status;
  status = thread_init(&idle_thread, &idle_stack[USR_STACK_WORDS-1]);
  if (status) {
    printf("Failed to initialize thread library: %d\n", status);
    return 1;
  }

  status = thread_create(&thread_1, &thread1_stack[USR_STACK_WORDS-1],
          1, 200, 500);
  if (status) {
    printf("Failed to create thread 1: %d\n", status);
    return 1;
  }

  status = thread_create(&thread_2, &thread2_stack[USR_STACK_WORDS-1],
          2, 900, 9000);
  if (status) {
    printf("Failed to create thread 2: %d\n", status);
    return 1;
  }

  status = mutex_init(&mutex1, 1);
  if (status) {
    printf("Mutex 1 initialization failed: %d\n", status);
    return 1;
  }

  status = mutex_init(&mutex2, 1);
  if (status) {
    printf("Mutex 2 initialization failed: %d\n", status);
    return 1;
  }

  printf("Successfully created threads! Starting scheduler...\n");

  status = scheduler_start();
  if (status) {
    printf("Threads are unschedulable! %d\n", status);
    return 1;
  }

  // Should never get here.
  return 2;
}

