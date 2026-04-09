#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int value;

  value = hello();
  printf("hello() returned %d\n", value);
  exit(0);
}
