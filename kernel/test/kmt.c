// #include <thread-sync.h>
// #include <common.h>
#include <os.h>
// void(*init)();
// int (*create)(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
// void (*teardown)(task_t *task);

// void *spin_init(spinlock_t *lk, const char *name)
// {
// }
// void *spin_lock(spinlock_t *lk)
// {
// }
// void *spin_unlock(spinlock_t *lk)
// {
// }

// void (*sem_init)(sem_t *sem, const char *name, int value);
// void (*sem_wait)(sem_t *sem);
// void (*sem_signal)(sem_t *sem);

static inline int atomic_xchg(volatile int *addr, int newval)
{
    int result;
    asm volatile("lock xchg %0, %1"
                 : "+m"(*addr), "=a"(result)
                 : "1"(newval)
                 : "memory");
    return result;
}

void spin_init(spinlock_t *lk, const char *name)
{
    lk->lock = 0;
    lk->name = NULL;
}
// static inline int atomic_xchg(volatile int *addr, int newval)
// {
//   int result;
//   asm volatile("lock xchg %0, %1"
//                : "+m"(*addr), "=a"(result)
//                : "1"(newval)
//                : "memory");
//   return result;
// }

void spin_lock(spinlock_t *lk)
{
    while (1)
    {
        int value = atomic_xchg(&(lk->lock), 1);
        if (value == 0)
        {
            break;
        }
    }
}

void spin_unlock(spinlock_t *lk)
{
    atomic_xchg(&(lk->lock), 0);
}

MODULE_DEF(kmt) = {
    .spin_lock = spin_lock,
    .spin_init = spin_init,
    .spin_unlock = spin_unlock};