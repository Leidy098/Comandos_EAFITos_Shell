#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int mask;

  if(argc < 3){
    fprintf(2, "Uso: ttrace <mask> <programa> [args...]\n");
    fprintf(2, "Ejemplo: ttrace 32 grep hello README\n");
    exit(1);
  }

  mask = atoi(argv[1]);
  if(trace(mask) < 0){
    fprintf(2, "ttrace: error activando trace\n");
    exit(1);
  }

  exec(argv[2], &argv[2]);
  fprintf(2, "ttrace: exec fallo para '%s'\n", argv[2]);
  exit(1);
}
