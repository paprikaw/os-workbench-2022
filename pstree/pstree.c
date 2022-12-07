#include <stdio.h>
#include <assert.h>
#define MAX_BUF 1024

int main(int argc, char *argv[]) {
  int process_id;

  // for (int i = 0; i < argc; i++) {
  //   assert(argv[i]);
  //   printf("argv[%d] = %s\n", i, argv[i]);
  // }
  // assert(!argv[argc]);
  FILE *stream = fopen("/proc", "r");
  while ((fscanf(stream, "%s", &process_id) > 0)) {
    printf("%d\n", process_id);
  }
  return 0;
}
