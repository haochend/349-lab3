/**
 * @file   main.c
 *
 * @brief  Basic test for mutex implementation
 *
 * t = 1 --- Task: 1, 1 locked
 * t = 7 --- Task: 1, 1 unlocked
 * t = 10 --- Task: 2, 1 locked
 * t = 12 --- Task: 2, 2 locked
 * t = 18 --- Task: 2, 2 unlocked
 * t = 20 --- Task: 2, 1 unlocked
 * t = 104 --- Task: 1, 1 locked
 * t = 110 --- Task: 1, 1 unlocked
 * t = 200 --- Task: 2, 1 locked
 * t = 202 --- Task: 2, 2 locked
 * t = 209 --- Task: 2, 2 unlocked
 */

#include <stdio.h>
#include <syscall_thread.h>
#include <mutex.h>

/** @brief 3x time required to print status information to reduce spin wait */
#define PRINT_STATUS_TIME_MS 6

/** @brief thread user space stack size - 4KB */
#define USR_STACK_WORDS 1024

uint32_t idle_stack[USR_STACK_WORDS];
uint32_t thread1_stack[USR_STACK_WORDS];
uint32_t thread2_stack[USR_STACK_WORDS];

mutex_t mutex0;
mutex_t mutex1;

/** @brief Prints lock status information of a thread
 *
 *  @param name       name of the thread
 *  @param is_locked  whether the thread locked or released the mutex
 */
void print_status(const char *name, int is_locked, int mutex_num) {
  if (is_locked) {
    printf("t = %d --- Task: %s, %d locked\n", get_time(), name, mutex_num);
  } else {
    printf("t = %d --- Task: %s, %d unlocked\n", get_time(), name, mutex_num);
  }
}

/** @brief Default idle thread which just loops infinitely
 */
void idle_thread(void) {
  while(1);
}

/** @brief Basic thread which locks and unlocks a mutex
 */
void thread_1(void) {
  while(1) {
    mutex_lock(&mutex0);
    print_status("1", 1, 1);

    spin_wait(10-PRINT_STATUS_TIME_MS);

    mutex_unlock(&mutex0);
    print_status("1", 0, 1);

    wait_until_next_period();
  }
}

/** @brief Basic thread which locks and unlocks a mutex
 */
void thread_2(void) {
  while(1) {
    mutex_lock(&mutex0);
    print_status("2", 1, 1);
    mutex_lock(&mutex1);
    print_status("2", 1, 2);

    spin_wait(10-PRINT_STATUS_TIME_MS);

    mutex_unlock(&mutex1);
    print_status("2", 0, 2);
    mutex_unlock(&mutex0);
    print_status("2", 0, 1);

    wait_until_next_period();
  }
}

int main(void) {
  int status;
  status = thread_init(&idle_thread, &idle_stack[USR_STACK_WORDS-1]);
  if (status) {
    printf("Failed to initialize thread library: %d\n", status);
    return 1;
  }

  status = mutex_init(&mutex0, 1);
  if (status) {
    printf("Mutex0 initialization failed: %d\n", status);
    return 1;
  }

  status = mutex_init(&mutex1, 2);
  if (status) {
    printf("Mutex1 initialization failed: %d\n", status);
    return 1;
  }

  status = thread_create(&thread_1, &thread1_stack[USR_STACK_WORDS-1],
          1, 20, 104);
  if (status) {
    printf("Failed to create thread 1: %d\n", status);
    return 1;
  }

  status = thread_create(&thread_2, &thread2_stack[USR_STACK_WORDS-1],
          2, 30, 200);
  if (status) {
    printf("Failed to create thread 2: %d\n", status);
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
