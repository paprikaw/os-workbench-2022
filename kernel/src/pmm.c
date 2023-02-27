#ifdef TEST
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <klib-macros.h>
#include <assert.h>
#else
#include <klib.h>
#endif
#include <os.h>
// typedef struct __memo
// {
//   Area *memArea;
//   spinlock_t *lock;
// } memo_t;

/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
/* ~0x0007 = 0xfff8 */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* rounds down to the nearest multiple of ALIGNMENT */
/* ~0x0007 = 0xfff8 */
#define ALIGN_DOWN(size) (((size) - (ALIGNMENT - 1)) & ~0x7)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* heap checking */
#define HEAP_CHECK(lineno) checkheap(lineno)
// #define HEAP_CHECK(lineno)

/* some assertion */

// Assert that the address is valid
// #define ASSERT_VALID_ADDRESS(addr)
#define ASSERT_VALID_ADDRESS(addr)            \
  {                                           \
    assert(mem_sbrk <= memArea->end);         \
    assert(addr > memArea->start);            \
    assert(FTRP(addr) <= (memArea->end - 3)); \
  }

/* Private global variables */
Area *memArea;          /* Get from os package */
static spinlock_t lock; /* Lock for heap */

/* Private global variable for sbrk function */

size_t mem_max_space;
static void *mem_max_addr; /* Max legal heap addr plus 1*/
static void *mem_sbrk;     /* break pointer for avaible address space */

/* Private global variable for implicit heap data structure*/
static void *mem_heap; /* starter of the heap */

/* Helper function */
static void *extend_heap(size_t words);
static void *coalesce(char *bp);
static void set_block(char *pos, size_t size, int is_allocate);
static void allocate_block(char *pos, size_t size);
static void free_block(char *pos);
static void split_space(char *cur_pos, size_t size);
static void checkheap(int lineno);

/* Memory management */
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
static void *mm_sbrk(size_t incr);

static void *kalloc(size_t size)
{

  void *space;
  kmt->spin_lock(&lock);
  if ((space = mm_malloc(size)) == (void *)(-1))
  {
    printf("kalloc: malloc is failed\n");
    kmt->spin_unlock(&lock);
    return (void *)(-1);
  }
  printf("The allocated heap space is %d\n", GET_SIZE(HDRP(space)));
  ASSERT_VALID_ADDRESS(space);
  kmt->spin_unlock(&lock);
  return space;
}

static void kfree(void *ptr)
{
}

/*
 * mm_init - initialize the space and heap data structure
 */
static void pmm_init()
{

#ifdef TEST
  // Test框架: 需要使用malloc分配内存
  char *ptr = malloc(HEAP_SIZE);
  Area *heap = malloc(sizeof(Area));

  heap->start = ptr;
  heap->end = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap->start, heap->end);
  memArea = heap;
#else
  // Abstract machine: heap数据结构直接被返回
  // Memory area for [@start, @end)
  // 框架代码中的 pmm_init (在 AbstractMachine 中运行)
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap\n", pmsize >> 20, heap.start, heap.end);
  memArea = &heap;
#endif

  /* Initialize lock */
  kmt->spin_init(&lock, "mem_lock");

  /* Initialize mem_sbrk */
  mem_sbrk = (void *)memArea->start;
  /* Initialize mem_max_space, this is for sbrk to estimate if the
    current break pointer is bigger than the maximum available address
    in the system
   */
  mem_max_addr = (void *)memArea->end;

  /* initializeheap内的数据结构，包括头节点和伪节点 */
  mem_max_space = 2 * WSIZE;

  /*构建padding block， 头节点和尾节点*/
  if ((mem_heap = mm_sbrk(4 * WSIZE)) == (void *)(-1))
  {
    panic("Failed to initialize the heap data structure");
    return;
  }
  PUT(mem_heap, 0);
  PUT(mem_heap + WSIZE, PACK(DSIZE, 1));
  PUT(mem_heap + (2 * WSIZE), PACK(DSIZE, 1));
  PUT(mem_heap + (3 * WSIZE), PACK(0, 1));
  mem_heap += (2 * WSIZE); // moved to the true start of the heap;
  mem_max_space = 2 * WSIZE;

  if ((extend_heap(CHUNKSIZE / WSIZE)) == (void *)(-1))
  {
    panic("Failed to allocate the initial data block");
  }

  HEAP_CHECK(__LINE__);
  return;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
  HEAP_CHECK(__LINE__);
  size = ALIGN(size + 2 * WSIZE);
  char *cur_pos = mem_heap;
  char *new_space;
  size_t cur_size = 0;
  // interate throught the whole heap list
  while (GET_SIZE(HDRP(cur_pos)) != 0)
  {
    cur_size = GET_SIZE(HDRP(cur_pos));
    // 找到合适的位置
    if (GET_ALLOC(HDRP(cur_pos)) == 0 &&
        (size <= cur_size))
    {
      HEAP_CHECK(__LINE__);
      split_space(cur_pos, size);
      new_space = cur_pos;
      break;
    }
    cur_pos = NEXT_BLKP(cur_pos);
  }

  if ((void *)cur_pos >= mem_max_addr)
  {

    size_t allocate_size = size < CHUNKSIZE ? CHUNKSIZE : size;
    if ((new_space = extend_heap(allocate_size / WSIZE)) == (void *)(-1))
    {
      printf("mm_malloc: space is not enough\n");
      return (void *)-1;
    }
    new_space = coalesce(new_space);
    split_space(new_space, size);
    HEAP_CHECK(__LINE__);
  }
  ASSERT_VALID_ADDRESS(new_space);
  return new_space;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  HEAP_CHECK(__LINE__);
  set_block(ptr, GET_SIZE(HDRP(ptr)), 0);
  coalesce(ptr);
  HEAP_CHECK(__LINE__);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  size_t oldsize;
  void *newptr;

  /* If size == 0 then this is just free, and we return NULL. */
  if (size == 0)
  {
    mm_free(ptr);
    return 0;
  }

  /* If oldptr is NULL, then this is just malloc. */
  if (ptr == NULL)
  {
    return mm_malloc(size);
  }

  newptr = mm_malloc(size);

  /* If realloc() fails the original block is left untouched  */
  if (!newptr)
  {
    return 0;
  }

  /* Copy the old data. */

  oldsize = GET_SIZE(HDRP(ptr));
  if (size < oldsize)
    oldsize = size;
  memcpy(newptr, ptr, oldsize);

  /* Free the old block. */
  mm_free(ptr);

  return newptr;
}

// There is no need for extend heap because we have a whole block of
// memory space at the beginning
/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words)
{
  HEAP_CHECK(__LINE__);
  char *bp;
  size_t size;
  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long)(bp = mm_sbrk(size)) == -1)
  {
    return (void *)-1;
  }
  /* Initialize free block header/footer and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0)); /* Free block header */           // line:vm:mm:freeblockhdr
  PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */           // line:vm:mm:freeblockftr
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ // line:vm:mm:newepihdr
  mem_max_addr = FTRP(bp) - 1;
  mem_max_space += size;
  /* Coalesce if the previous block was free */
  return coalesce(bp);
}

static void *coalesce(char *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  if (prev_alloc && next_alloc)
  { /* Case 1 */
    return bp;
  }

  else if (prev_alloc && !next_alloc)
  { /* Case 2 */
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  else if (!prev_alloc && next_alloc)
  { /* Case 3 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  else
  { /* Case 4 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  HEAP_CHECK(__LINE__);
  return bp;
}

/*********************************************************
 * Heap checker
 ********************************************************/
static void checkheap(int lineno)

{
  char *cur_pos = mem_heap;
  int is_prev_empty = 0;
  int is_coalesce = 1;
  int is_head_tail_equal = 1;
  int is_cloased_boundary = 1;
  int is_good_prologue = 1;
  int is_good_epilogue = 1;
  size_t total_space = 0;
  size_t size = 0;

  /* 检查prologue tag*/
  if ((GET_SIZE(HDRP(mem_heap)) != DSIZE) || !GET_ALLOC(HDRP(mem_heap)))
    is_good_prologue = 0;

  // interate throught the whole heap list
  while (GET_SIZE(HDRP(cur_pos)) != 0)
  {
    int is_alloc = GET_ALLOC(HDRP(cur_pos));
    /*检查是否coalesce*/
    if (is_prev_empty == 1 && (!is_alloc))
    {

      is_coalesce = 0;
    }
    is_prev_empty = is_alloc ? 0 : 1;

    /*检查头尾tag是否相同*/
    for (int i = 0; i < WSIZE; i++)
    {
      if (*((char *)HDRP(cur_pos) + i) != *((char *)FTRP(cur_pos) + i))
      {
        is_head_tail_equal = 0;
        break;
      }
    }
    /*检查一个block之前是否有一个boundary tag */
    if ((cur_pos != mem_heap) && NEXT_BLKP(PREV_BLKP(cur_pos)) != cur_pos)
    {

      is_cloased_boundary = 0;
    }

    /*检查alignment */
    if ((size_t)cur_pos % 8)
      printf("Error: %p is not doubleword aligned\n", cur_pos);

    /*记录总共在标签里面的size的总和 */
    total_space += GET_SIZE(HDRP(cur_pos));
    cur_pos = NEXT_BLKP(cur_pos);
  }

  /* 检查epilogue tag */
  if ((GET_SIZE(HDRP(cur_pos)) != 0) || !(GET_ALLOC(HDRP(cur_pos))))
    is_good_epilogue = 0;

  if (!(is_coalesce &&
        is_head_tail_equal &&
        is_cloased_boundary &&
        (total_space == mem_max_space) &&
        is_good_epilogue &&
        is_good_prologue))
    panic("line");

  /*检查是否coalesce*/
  if (!is_coalesce)
    panic("Free block is not coalescing in block");

  /*检查头尾tag是否相同*/
  if (!is_head_tail_equal)
    panic("head block is not the same with tail block ");

  /*检查blocz之前的boundary tag是否是valid的*/
  if (!is_cloased_boundary)
    panic("boundary is not closed");

  /* 检查memory space是否都可以被retreive到*/
  if (total_space != mem_max_space)
    panic("space size error");

  /* 检查epilogue tag和prologue tag*/
  if (!is_good_epilogue)
    panic("Bad epilogue header");

  if (!is_good_prologue)
    panic("Bad prologue header");
  /* 检查sbrk目前的值是否是有效的 */
  if ((mem_sbrk <= 0) || (mem_sbrk > memArea->end))
  {
    panic("Bad sbrk value");
  }
}

static void set_block(char *pos, size_t size, int is_allocate)
{
  // head
  PUT(HDRP(pos), PACK(size, is_allocate));
  // tail
  PUT(FTRP(pos), PACK(size, is_allocate));
}

static void allocate_block(char *pos, size_t size)
{
  set_block(pos, size, 1);
}

static void free_block(char *pos)
{
  set_block(pos, GET_SIZE(HDRP(pos)), 0);
}

static void split_space(char *bp, size_t asize)
{
  HEAP_CHECK(__LINE__);
  size_t csize = GET_SIZE(HDRP(bp));

  if ((csize - asize) >= (2 * DSIZE))
  {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    PUT(FTRP(bp), PACK(csize - asize, 0));
  }
  else
  {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
  HEAP_CHECK(__LINE__);
}

/*
 * mem_sbrk - Simple model of the sbrk function. Extends the heap
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
static void *mm_sbrk(size_t incr)
{
  char *old_brk = mem_sbrk;
  if ((incr < 0) || ((mem_sbrk + incr) > mem_max_addr))
  {
    printf("ERROR: mm_sbrk failed. Ran out of memory...\n");
    return (void *)-1;
  }
  mem_sbrk += incr;
  return (void *)old_brk;
}
// size_t align(size_t size)
// {
//   {
//     size_t aligned_size;
//     int bit_size = sizeof(size_t) * 8;
//     for (int i = bit_size - 1; i >= 0; i--)
//     {
//       aligned_size = 1 << i;
//       if (aligned_size < size)
//       {
//         return aligned_size << 1;
//       }
//       else if (aligned_size == size)
//       {
//         return aligned_size;
//       }
//     }
//     // should not reach this place
//     panic("alignment function goes wrong");
//   }
// }

MODULE_DEF(pmm) = {
    .init = pmm_init,
    .alloc = kalloc,
    .free = kfree,
};
