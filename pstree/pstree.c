#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_BUF 1024
int main(int argc, char *argv[])
{
  int process_id;
  struct dirent *dir;
  DIR *dir_stream;

  // for (int i = 0; i < argc; i++) {
  //   assert(argv[i]);
  //   printf("argv[%d] = %s\n", i, argv[i]);
  // }
  // assert(!argv[argc]);
  dir_stream = opendir("/proc");
  if (dir_stream != NULL)
  {
    while ((dir = readdir(dir_stream)) != NULL)
      printf(dir->d_name);
  }
  close(dir);
  return 0;
}
