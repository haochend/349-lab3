/** @file mutex.h
 *  @brief Type definitions for mutexes
 *
 *  @author ???
 */

#ifndef _MUTEX_H
#define _MUTEX_H

/** Mutex datatype */
typedef struct mutex_t {
  int lock;
  unsigned int ceiling;
  int thread;
} mutex_t;

#endif // __MUTEX_TYPE_H
