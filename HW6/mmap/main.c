#include <stdio.h>
#include <string.h>
#include "memlib.h"
#include "chaos.h"

int main()
{
  size_t sz = 10 * mem_pagesize();
  void *p = mem_map(sz);

  *(int *)p = 10;
  printf("%d\n", *(int *)p);

  mem_unmap(p, sz);
  
  return 0;
}
