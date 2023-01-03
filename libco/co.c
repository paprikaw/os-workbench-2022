#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE 8192
#define MAX_NAME_SIZE 256
#define CO_POOL_SIZE 8
#define SET_JUMP_TRUE_RETURN 0

/* 协程库的主要数据结构 */
enum co_status
{
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

typedef struct co
{
  char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;     // 协程的状态
  struct co *waiter;         // 是否有其他协程在等待当前协程
  jmp_buf context;           // 寄存器现场 (setjmp.h)
  uint8_t stack[STACK_SIZE]; // 协程的堆栈
} CO;

CO *co_pool[CO_POOL_SIZE]; // 用来储存所有的协程
CO *current;

/* Helper function */
static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg);
int rand_index(int length);
CO *create_co(const char *name, void (*func)(void *), void *arg, enum co_status status, CO *waiter);

/* 协程库主要的routine */
CO *co_start(const char *name, void (*func)(void *), void *arg)
{
  // 创建一个新的携程context
  CO *new_context = create_co(name, func, arg, CO_NEW, NULL);
  int is_added_to_pool = 0;
  for (int i = 0; i < CO_POOL_SIZE; i++)
  {
    if (co_pool[i] == NULL)
    {
      co_pool[i] = new_context;
      is_added_to_pool = 1;
      break;
    }
  }

  // 如果协程池已经满了，则返回NULL
  if (!is_added_to_pool)
  {
    free(new_context);
    return NULL;
  }
  return new_context;
}

void co_wait(CO *co)
{
  // 如果协程已经结束了, 释放资源
  if (co->status == CO_DEAD)
  {
    free(co);
    return;
  }
  // 现在还在main函数里面，需要在线程库里面创建main函数协程
  if (current == NULL)
  {
    current = create_co("main", NULL, NULL, CO_WAITING, NULL);
    co->waiter = current;
  }

  current->status = CO_WAITING;
  // 开始执行协程
  co_yield ();
}

void co_yield ()
{
  // 每一次调用co_yield的时候，都进行一次wait_process的checking工作

  // 进行当前协程的现场保存
  int val = setjmp(current->context);
  // 第一次执行co_yield, setjump返回真的value
  if (val == SET_JUMP_TRUE_RETURN)
  {
    // 随机挑选一个状态为RUNNING或者NEW的协程
    int index = rand_index(CO_POOL_SIZE);
    CO *next_co = co_pool[index];

    while ((next_co == NULL) ||
           ((next_co->status != CO_RUNNING) &&
            (next_co->status != CO_NEW)))
    {
      index = rand_index(CO_POOL_SIZE);
      next_co = co_pool[index];
    }

    // 跳转到这个协程
    current = next_co;
    if (current->status == CO_NEW)
    {
      current->status = CO_RUNNING;
      stack_switch_call(current->stack, current->func, (uintptr_t)current->arg);
    }
    else
    {
      longjmp(current->context, 1);
    }
  }
  else
  {
    // 跳转到新的协程，直接开始运行程序
    return;
  }
}

/* Implementations of helper functions */
static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg)
{
  asm volatile(
#if __x86_64__
      "movq %%rsp, %%r12; movq %0, %%rsp; movq %2, %%rdi; call *%1; movq %%r12 %%rsp"
      :
      : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
      : "memory"
#else
      "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      :
      : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
      : "memory"
#endif
  );
}

// Given a length of an array, return a random index
int rand_index(int length)
{

  return rand() % length;
}

// Create a new corouting
CO *create_co(const char *name, void (*func)(void *), void *arg, enum co_status status, CO *waiter)
{
  // 创建一个新的携程context
  CO *new_context = malloc(sizeof(CO));
  // Allocate space for name
  char *name_buf = malloc(MAX_NAME_SIZE * sizeof(char));
  strncpy(name_buf, name, MAX_NAME_SIZE);
  new_context->name = name_buf;
  new_context->func = func;
  new_context->arg = arg;
  new_context->status = status;
  new_context->waiter = waiter;
  return new_context;
}