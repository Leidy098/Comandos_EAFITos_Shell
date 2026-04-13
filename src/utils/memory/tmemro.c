#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  char *va = (char *)0x40000000;

  if(map_ro(va) < 0){
    fprintf(2, "tmemro: map_ro fallo\n");
    exit(1);
  }

  printf("tmemro: contenido leido: %s\n", va);
  printf("tmemro: intentando escribir (debe fallar)...\n");
  va[0] = 'X';

  printf("tmemro: ERROR — no deberia llegar aqui\n");
  exit(1);
}
