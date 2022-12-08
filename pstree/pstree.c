#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define MAX_LINE 1024
#define MAX_CHILD_PROCESS 32
#define MAX_PROCESS 1024

typedef struct pnode
{
  pid_t pid;
  struct pnode *childs;
  char pname[MAX_LINE];
} Pnode;

typedef struct pprev
{
  pid_t pid;
  pid_t prev;
} Pprev;

pid_t get_ppid(pid_t cur_pid);
char *line_buf[MAX_LINE];
Pnode *root_pnode;

int main(int argc, char *argv[])
{
  struct dirent *dir;
  DIR *dir_stream;
  int process_id;
  Pprev id_array[MAX_PROCESS];

  /*
  从/proc目录中去读取每一个进程的信息, 并且创建一个列表。包含
  当前内存中的processid和parent process id
   */
  dir_stream = opendir("/proc");
  if (dir_stream != NULL)
  {
    pid_t process_id;
    int scanf_num;
    Pprev *cur = id_array;
    int cur_index = 0;
    int cur_prev;
    while ((dir = readdir(dir_stream)) != NULL)
    {
      scanf_num = sscanf(dir->d_name, "%d", &process_id);
      if (scanf_num == EOF)
      {
        printf("%s", strerror(errno));
        exit(1);
      }
      else if (scanf_num < 1)
      {
        continue;
      }

      if ((cur_prev = get_ppid(process_id) == -1))
      {
        continue;
      }
      cur->pid = process_id;
      cur->prev = cur_prev;
      cur++;
    }
    cur->pid = 0;
    cur->prev = 0;
  }
  else
  {
    printf("%s\n", strerror(errno));
  }
  Pprev *cur = id_array;
  while (cur->pid != 0)
  {
    printf("pid: %d prev: %d\n", cur->pid, cur->prev);
    cur++;
  }
  closedir(dir_stream);
  return 0;
}

pid_t get_ppid(pid_t cur_pid)
{
  char buf[MAX_LINE];
  pid_t pid;
  int sucess_match;
  FILE *file_stat;
  // 构建对应的stat
  sprintf(buf, "/proc/%d/stat", cur_pid);
  if (!(file_stat = fopen(buf, "r")))
  {
    return -1;
  }

  sucess_match = fscanf(file_stat, "%*d %*s %*c %d", &pid);

  // check for successful mathces
  if (sucess_match == EOF)
  {
    strerror(errno);
    return -1;
  }
  else if (sucess_match < 1)
  {
    return -1;
  }

  return pid;
}