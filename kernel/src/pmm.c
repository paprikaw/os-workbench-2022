#ifdef TEST
#include <stdio.h>
#include <stdlib.h>
#endif
#include <os.h>

typedef struct memo
{
  Area *memArea;
  spinlock_t *lock;
} memo_t;

static memo_t memo;
static spinlock_t lock;

static void *kalloc(size_t size)
{
  kmt->spin_lock(memo.lock);
  printf("hello\n");
  kmt->spin_unlock(memo.lock);
  return NULL;
}

static void kfree(void *ptr)
{
}

static void pmm_init()
{

#ifdef TEST
  // Test框架: 需要使用malloc分配内存
  char *ptr = malloc(HEAP_SIZE);
  Area *heap = malloc(sizeof(Area));

  heap->start = ptr;
  heap->end = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap->start, heap->end);
#else
  // Abstract machine: heap数据结构直接被返回
  // Memory area for [@start, @end)
  // 框架代码中的 pmm_init (在 AbstractMachine 中运行)
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
#endif
  memo.memArea = heap;
  kmt->spin_init(&lock, "mem_lock");
  memo.lock = &lock;
}

MODULE_DEF(pmm) = {
    .init = pmm_init,
    .alloc = kalloc,
    .free = kfree,
};
