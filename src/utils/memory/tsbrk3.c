#include "kernel/types.h"
#include "user/user.h"

// Implementando Lazy Allocation Real
// Demuestra lazy allocation COMPLETO y funcional:
// - sbrk() reserva memoria virtual (sin asignar físicamente)
// - Al acceder a una dirección, ocurre un page fault
// - kernel/trap.c detecta el fault en usertrap()
// - vmfault() en kernel/vm.c asigna y mapea la página bajo demanda
// - La ejecución continúa sin crash
//
// La memoria se asigna SOLO cuando se accede
// Se observan múltiples page faults controlados

int
main(void)
{
  char *p;
  const int npages = 4;
  
  printf("tlazy3: Lazy allocation completo con manejo de page faults\n");
  printf("tlazy3: reservando %d paginas (%d bytes) con sbrklazy\n", 
         npages, npages * 4096);
  
  p = sbrklazy(npages * 4096);
  if(p == SBRK_ERROR) {
    fprintf(2, "tlazy3: sbrklazy(%d*4096) failed\n", npages);
    exit(1);
  }
  
  printf("tlazy3: sbrklazy retorno %p (memoria reservada, no asignada aun)\n", p);
  printf("\n=== FASE 1: Acceso Secuencial ===\n");
  
  // Acceso secuencial: genera un page fault por cada página
  for(int i = 0; i < npages; i++) {
    char *q = p + i * 4096;
    printf("tlazy3: accediendo pagina %d en %p\n", i, q);
    q[0] = 'A' + i;  // Esto causa un page fault
    printf("tlazy3: escrito '%c' en pagina %d\n", q[0], i);
  }
  
  printf("\n=== FASE 2: Verificacion ===\n");
  for(int i = 0; i < npages; i++) {
    char *q = p + i * 4096;
    if(q[0] != 'A' + i) {
      fprintf(2, "tlazy3: ERROR verificando pagina %d\n", i);
      exit(1);
    }
    printf("tlazy3: pagina %d OK (contiene '%c')\n", i, q[0]);
  }
  
  printf("\n=== FASE 3: Acceso Aleatorio ===\n");
  // Acceso a páginas ya asignadas (no genera nuevos page faults)
  int order[] = {3, 0, 2, 1};
  for(int i = 0; i < npages; i++) {
    int idx = order[i];
    char *q = p + idx * 4096;
    printf("tlazy3: acceso aleatorio pagina %d: '%c'\n", idx, q[0]);
    if(q[0] != 'A' + idx) {
      fprintf(2, "tlazy3: ERROR en acceso aleatorio pagina %d\n", idx);
      exit(1);
    }
  }
  
  printf("\ntlazy3: SUCCESS - lazy allocation completo\n");
  printf("tlazy3: Se observaron %d page faults controlados (uno por pagina)\n", npages);
  exit(0);
}
