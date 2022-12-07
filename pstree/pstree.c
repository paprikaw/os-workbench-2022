#include <stdio.h>
#include <assert.h>
#define MAX_BUF 1024

int main(int argc, char *argv[])
{
  int process_id;

  // for (int i = 0; i < argc; i++) {
  //   assert(argv[i]);
  //   printf("argv[%d] = %s\n", i, argv[i]);
  // }
  // assert(!argv[argc]);
  FILE *stream = fopen("/proc", "r");
  int number1;
  while (((number1 = fscanf(stream, "%d", &process_id)) > 0))
  {
    printf("%d\n", process_id);
  }
  fclose(stream);
  return 0;
}
