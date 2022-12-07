#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#define MAX_LINE 1024
char *line_buf[MAX_LINE];

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
  dir_stream = opendir("/proc/1/fd");

  if (dir_stream != NULL)
  {
    while ((dir = readdir(dir_stream)) != NULL)
      printf("%s\n", dir->d_name);
  }
  else
  {
    printf("%d", errno);
  }
  closedir(dir_stream);
  return 0;
}