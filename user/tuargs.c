#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static void
check(const char *label, int got, int expected)
{
  if(got == expected)
    printf("tuargs: %s -> %d OK\n", label, got);
  else
    printf("tuargs: %s -> %d esperado %d\n", label, got, expected);
}

int
main(void)
{
  int r;
  int fds[2];
  char msg[] = "ok\n";
  uint64 bad = (uint64)-1;

  r = write(1, msg, 3);
  check("write puntero valido", r, 3);

  r = write(1, (char *)bad, 1);
  check("write puntero invalido", r, -1);

  r = open("README", O_RDONLY);
  if(r < 0){
    fprintf(2, "tuargs: no pudo abrir README\n");
    exit(1);
  }
  close(r);

  r = open((char *)bad, O_RDONLY);
  check("open string invalido", r, -1);

  r = pipe(fds);
  if(r < 0){
    fprintf(2, "tuargs: pipe valido fallo\n");
    exit(1);
  }
  check("pipe puntero valido", r, 0);
  close(fds[0]);
  close(fds[1]);

  r = pipe((int *)bad);
  check("pipe puntero invalido", r, -1);

  exit(0);
}
