#include <common.h>

struct task
{
  // TODO
};

struct spinlock
{
  char *name;
  int lock;
};

struct semaphore
{
  // TODO
};

void panic(char *s)
{
  printf("size should not be zero");
  exit(0);
}