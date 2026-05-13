#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  char *p;
  int size = 3 * 4096;  // 3 paginas

  printf("tmmap_sim: inicio\n");

  p = mapzero(size);
  if(p == (char *)-1){
    fprintf(2, "tmmap_sim: mapzero(%d) fallo\n", size);
    exit(1);
  }
  printf("tmmap_sim: mapzero(%d) retorno %p\n", size, p);
  printf("tmmap_sim: region virtual reservada, sin paginas fisicas aun\n");

  // Acceso gradual: un byte por pagina
  printf("\n--- acceso gradual (1 byte por pagina) ---\n");
  for(int i = 0; i < 3; i++){
    char *q = p + i * 4096;
    printf("tmmap_sim: accediendo pagina %d en %p\n", i, q);
    char val = q[0];  // page fault -> usertrap rellena con 'A'
    printf("tmmap_sim: pagina %d primer byte = '%c' (0x%x)\n", i, val, (unsigned char)val);
    if(val != 'A'){
      fprintf(2, "tmmap_sim: ERROR esperaba 'A' (0x41), obtuve 0x%x\n",
              (unsigned char)val);
      exit(1);
    }
  }

  // Verificar que toda la primera pagina tiene 'A'
  printf("\n--- verificacion completa primera pagina ---\n");
  int ok = 1;
  for(int i = 0; i < 4096; i++){
    if(p[i] != 'A'){
      fprintf(2, "tmmap_sim: ERROR byte %d = 0x%x\n", i, (unsigned char)p[i]);
      ok = 0;
      break;
    }
  }
  if(ok)
    printf("tmmap_sim: primera pagina completa OK (4096 bytes = 'A')\n");

  // Escritura y re-lectura
  printf("\n--- escritura y re-lectura ---\n");
  p[0] = 'Z';
  p[4096] = 'Y';
  p[8192] = 'X';
  if(p[0] == 'Z' && p[4096] == 'Y' && p[8192] == 'X')
    printf("tmmap_sim: escritura/lectura OK\n");
  else {
    fprintf(2, "tmmap_sim: ERROR en escritura\n");
    exit(1);
  }

  printf("\ntmmap_sim: SUCCESS\n");
  exit(0);
}
