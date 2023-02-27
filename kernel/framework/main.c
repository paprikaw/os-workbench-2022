#include <kernel.h>
#include <klib.h>

int main()
{
  ioe_init();
  os->init();
  mpe_init(os->run);
  return 0;
}
