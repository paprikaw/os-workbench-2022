#include <common.h>
#include <thread.h>

/* 这个测试并不是黑箱测试，我们会暴露一些pmm模组内部的api，以便我们判断address的大小 */
static void goodbye() { printf("End.\n"); }

/* hello word */
static void entry0(int tid)
{
    char *space = pmm->alloc(128);
    printf("thread %d is allocate address space: %p\n", tid, space);
    // printf("%d: haha\n", tid);
}
void test0()
{
    printf("Test 0: hello world\n");
    pmm->init();
    for (int i = 0; i < 1; i++)
        create(entry0);
    join(goodbye);
}

void test1()
{
    printf("Test 1: hello world\n");
    pmm->init();
    for (int i = 0; i < 4; i++)
        create(entry0);
    join(goodbye);
}

/* 压力测试 */
void test2()
{
    printf("Test 2: hello world\n");
    pmm->init();
    for (int i = 0; i < 32; i++)
        create(entry0);
    join(goodbye);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        exit(1);
    switch (atoi(argv[1]))
    {
    case 0:
        test0();
        break;
    case 1:
        test1();
        break;
    case 2:
        test2();
    }
}
