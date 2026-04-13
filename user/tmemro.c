#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  // Dirección de prueba alineada a página
  char *va = (char *)0x40000000;

  if(map_ro(va) < 0){
    fprintf(2, "tmemro: map_ro fallo\n");
    exit(1);
  }

  // Lectura: debe funcionar
  printf("tmemro: contenido leido: %s\n", va);

  // Escritura: debe provocar page fault y matar el proceso
  printf("tmemro: intentando escribir (debe fallar)...\n");
  va[0] = 'X';  // store page fault aqui

  printf("tmemro: ERROR — no deberia llegar aqui\n");
  exit(1);
}