#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  const int npages = 6;
  char *p;

  printf("tlazy: inicio, pf_count=%d\n", getpfcount());

  p = sbrk(npages * 4096);
  if(p == (char *)-1){
    fprintf(2, "tlazy: sbrk fallo\n");
    exit(1);
  }
  printf("tlazy: sbrk(%d) retorno %p (lazy, sin paginas fisicas aun)\n",
         npages * 4096, p);
  printf("tlazy: pf_count tras sbrk=%d (debe ser 0)\n", getpfcount());

  printf("\n--- acceso secuencial ---\n");
  for(int i = 0; i < npages; i++){
    p[i * 4096] = 'A' + i;
    printf("tlazy: pagina %d escrita, pf_count=%d\n", i, getpfcount());
  }

  printf("\n--- verificacion ---\n");
  for(int i = 0; i < npages; i++){
    if(p[i * 4096] != 'A' + i){
      fprintf(2, "tlazy: ERROR pagina %d\n", i);
      exit(1);
    }
  }
  printf("tlazy: verificacion OK, pf_count=%d\n", getpfcount());

  printf("\n--- acceso aleatorio ---\n");
  int order[] = {5, 1, 3, 0, 4, 2};
  for(int i = 0; i < npages; i++){
    int idx = order[i];
    if(p[idx * 4096] != 'A' + idx){
      fprintf(2, "tlazy: ERROR aleatorio pagina %d\n", idx);
      exit(1);
    }
    printf("tlazy: pagina %d OK '%c'\n", idx, p[idx * 4096]);
  }

  printf("\ntlazy: pf_count final=%d (esperado %d)\n", getpfcount(), npages);
  printf("tlazy: SUCCESS\n");
  exit(0);
}
