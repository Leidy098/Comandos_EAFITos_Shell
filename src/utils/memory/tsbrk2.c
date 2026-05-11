#include "kernel/types.h"
#include "user/user.h"

// Transformando sbrk() - Introducción a Lazy Allocation
// Este programa demuestra que sbrk(LAZY) reserva memoria VIRTUAL
// sin asignar memoria FÍSICA. La página no existe en el page table
// hasta que se accede a ella.
//
// Sin vmfault() en kernel: El programa sufre un page fault fatal
// Con vmfault() (enunciado 3): El page fault es manejado y la página
//se asigna bajo demanda

int
main(void)
{
  char *p;
  
  printf("tlazy2: reservando 1 pagina con sbrk(4096) en modo LAZY\n");
  p = sbrklazy(4096);
  
  if(p == SBRK_ERROR) {
    fprintf(2, "tlazy2: sbrklazy(4096) fallo\n");
    exit(1);
  }
  
  printf("tlazy2: sbrk retorno %p (NO asignada fisicamente aun)\n", p);
  printf("tlazy2: intentando acceder a la memoria en %p...\n", p);
  
  // Esta linea causa un page fault
  // Si vmfault() no esta implementada (enunciado 3), el programa falla aqui
  // Si vmfault() esta implementada, la pagina se asigna bajo demanda
  p[0] = 'X';
  printf("tlazy2: acceso exitoso! Valor escrito: %c\n", p[0]);
  
  // Verificar que la pagina esta realmente asignada
  p[0] = 'A';
  if(p[0] != 'A') {
    fprintf(2, "tlazy2: ERROR verificando contenido\n");
    exit(1);
  }
  
  printf("tlazy2: success - memoria asignada bajo demanda\n");
  exit(0);
}
