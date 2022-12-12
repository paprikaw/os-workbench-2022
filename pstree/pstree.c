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
  struct pnode *next;
  char *pname; // needs to be freed
} Pnode;

typedef struct pprev
{
  pid_t pid;
  pid_t prev;
  char *pname; // needs to be freed
} Pprev;

void recursive_buid_tree(Pprev *prevList, Pnode *curNode);
int get_pname(pid_t cur_pid, char *buf);
void init_pnode(Pnode *node);
pid_t get_ppid(pid_t cur_pid);
char *line_buf[MAX_LINE];
Pnode *root_pnode;

int main(int argc, char *argv[])
{
  struct dirent *dir;
  DIR *dir_stream;
  int process_id;
  Pprev id_array[MAX_PROCESS];
  Pnode process_tree = {0, NULL, NULL, "root"};
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
    char *pname = NULL;

    while ((dir = readdir(dir_stream)) != NULL)
    {
      scanf_num = sscanf(dir->d_name, "%d", &process_id);
      if (scanf_num == EOF)
      {
        printf("sscanf error: %s", strerror(errno));
        exit(1);
      }
      else if (scanf_num < 1) // 当前的
      {
        continue;
      }

      // get process parent and name
      if (((cur_prev = get_ppid(process_id)) < 0) || (get_pname(process_id, pname) < 0))
      {
        continue;
      }

      cur->pid = process_id;
      cur->prev = cur_prev;
      cur->pname = pname;
      cur++;
    }
    cur = NULL;
  }
  else
  {
    printf("%s\n", strerror(errno));
  }

  Pprev *cur = id_array;
  // build tree
  while (cur != NULL)
  {
    printf("pname: %s, pid: %d,  ppid: %d\n", cur->pname, cur->pid, cur->prev);
    cur++;
  }

  recursive_buid_tree(id_array, &process_tree);
  closedir(dir_stream);
  return 0;
}
/*
get ppid given a pid
 */
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

  sucess_match = fscanf(file_stat, "%*d %*s %*c %d ", &pid);
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

/*
get ppid given a pid
 */
int get_pname(pid_t cur_pid, char *buf)
{
  int sucess_match;
  FILE *file_stat;
  char path_buf[MAX_LINE];
  if (cur_pid == 0)
  {
    sprintf(buf, "%s", "root");
    return 1;
  }

  // 构建对应的stat
  sprintf(path_buf, "/proc/%d/stat", cur_pid);
  if (!(file_stat = fopen(path_buf, "r")))
  {
    return -1;
  }

  sucess_match = fscanf(file_stat, "%*d (%m[a-zA-Z])", &buf);
  // check for successful mathces
  if (sucess_match == EOF)
  {
    strerror(errno);
    return -1;
  }
  return sucess_match;
}

/*
routine for operate process tree
*/
void recursive_buid_tree(Pprev *prevList, Pnode *curNode)
{
  pid_t ppid = curNode->pid;
  Pprev *curPrev = prevList;
  Pnode **curPnodePtr = &(curNode->childs);

  while (curPrev != NULL)
  {
    if (curPrev->prev == ppid)
    {
      // 创建一个新的node
      Pnode *new_node = malloc(sizeof(Pnode));
      init_pnode(new_node);
      // 将ppid和目前的node匹配的node的信息复制到child里面
      new_node->pid = curPrev->pid;
      new_node->pname = curPrev->pname;

      // 对于这个新的node，建树
      // 终止条件: 当前tree node无法找到children
      recursive_buid_tree(prevList, new_node);

      // 将当前这个new node赋给pointer的指针
      *curPnodePtr = new_node;
      curPnodePtr = &(new_node->next);
    }
    curPrev++;
  }
}

void init_pnode(Pnode *node)
{
  node->childs = NULL;
  node->next = NULL;
}