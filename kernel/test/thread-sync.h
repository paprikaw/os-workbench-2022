#include <semaphore.h>
#include <stdint.h>
#include <common.h>
#include <string.h>
// Spinlock

struct spinlock
{
  char *name;
  spinlock_t *lock;
};

void *spin_init(spinlock_t *lk, const char *name)
{
  lk->lock = 0;
  strcpy(lk->name, name);
}

static inline int atomic_xchg(volatile int *addr, int newval)
{
  int result;
  asm volatile("lock xchg %0, %1"
               : "+m"(*addr), "=a"(result)
               : "1"(newval)
               : "memory");
  return result;
}

void spin_lock(spinlock_t *lk)
{
  while (1)
  {

    intptr_t value = atomic_xchg(lk->lock 1);
    if (value == 0)
    {
      break;
    }
  }
}
void spin_unlock(spinlock_t *lk)
{
  atomic_xchg(lk->lock, 0);
}

// Mutex
typedef pthread_mutex_t mutex_t;
#define MUTEX_INIT() PTHREAD_MUTEX_INITIALIZER
void mutex_lock(mutex_t *lk)
{
  pthread_mutex_lock(lk);
}
void mutex_unlock(mutex_t *lk) { pthread_mutex_unlock(lk); }

// Conditional Variable
typedef pthread_cond_t cond_t;
#define COND_INIT() PTHREAD_COND_INITIALIZER
#define cond_wait pthread_cond_wait
#define cond_broadcast pthread_cond_broadcast
#define cond_signal pthread_cond_signal

// Semaphore
#define P sem_wait
#define V sem_post
#define SEM_INIT(sem, val) sem_init(sem, 0, val)
