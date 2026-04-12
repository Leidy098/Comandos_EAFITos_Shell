#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  const int growth = 2 * 4096;

  printf("tdumpvm: pid=%d\n", getpid());
  dumpvm();

  char *probe = sbrklazy(growth);
  if (probe == SBRK_ERROR) {
    fprintf(2, "tdumpvm: sbrklazy fallo\n");
    exit(1);
  }

  for (int i = 0; i < growth; i += 4096)
    probe[i] = 'A' + (i / 4096);

  printf("tdumpvm: despues de tocar %d bytes nuevos en heap\n", growth);
  dumpvm();

  exit(0);
}
