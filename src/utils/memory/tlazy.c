#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  char *p;

  p = sbrk(4096);
  if(p == SBRK_ERROR){
    fprintf(2, "tlazy: sbrk(4096) failed\n");
    exit(1);
  }

  p[0] = 'A';
  p[4095] = 'Z';

  if(p[0] != 'A' || p[4095] != 'Z'){
    fprintf(2, "tlazy: memory access failed\n");
    exit(1);
  }

  printf("tlazy: success, addr=%p\n", p);
  exit(0);
}
