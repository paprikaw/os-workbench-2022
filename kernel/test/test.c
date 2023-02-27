#include <common.h>
#include <thread.h>

static void goodbye() { printf("End.\n"); }

/* hello word */
static void entry0(int tid) { pmm->alloc(128); }
void test0()
{
    printf("Test 1: hello world");
    pmm->init();
    for (int i = 0; i < 4; i++)
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
    }
}
