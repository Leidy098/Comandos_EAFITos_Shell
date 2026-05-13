#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/defs.h"
#include "kernel/param.h"
#include "kernel/memlayout.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "kernel/vm.h"

uint64
sys_hello(void)
{
  // Syscall de comprobacion simple: devuelve un valor constante.
  return 42;
}

uint64
sys_trace(void)
{
  int mask;

  // Configura la mascara de trazado de syscalls para el proceso actual.
  argint(0, &mask);
  myproc()->trace_mask = mask;
  return 0;
}

uint64
sys_dumpvm(void)
{
  struct proc *p = myproc();

  // Imprime informacion del proceso y la tabla de paginas.
  printf("dumpvm: pid=%d name=%s sz=%p\n", p->pid, p->name, (void *)p->sz);
  vmprint(p->pagetable);
  return 0;
}

// mapzero(size): reserva una region virtual de 'size' bytes, sin mapear
// fisicamente. Al acceder, usertrap rellena cada pagina con 0x41 ('A').
// Retorna la direccion virtual de inicio, o -1 si no hay slots libres
// o el tamano es invalido.
uint64
sys_mapzero(void)
{
  int size;
  struct proc *p = myproc();

  argint(0, &size);
  if(size <= 0)
    return -1;

  // Redondear al multiplo de pagina
  size = PGROUNDUP(size);

  // Buscar slot libre
  int slot = -1;
  for(int i = 0; i < NVREG; i++){
    if(!p->vregions[i].used){
      slot = i;
      break;
    }
  }
  if(slot < 0)
    return -1;

  // Proteccion de desbordamiento y limite superior
  if(p->sz + size < p->sz)
    return -1;
  if(p->sz + size > TRAPFRAME)
    return -1;

  uint64 start = p->sz;
  p->sz += size;

  p->vregions[slot].start = start;
  p->vregions[slot].size  = size;
  p->vregions[slot].used  = 1;

  return start;
}

uint64
sys_map_ro(void)
{
  uint64 va;
  struct proc *p = myproc();
  char *msg = "Pagina solo lectura desde kernel\n";
  char *mem;

  argaddr(0, &va);
  va = PGROUNDDOWN(va);

  mem = kalloc();
  if(mem == 0)
    return -1;
  memset(mem, 0, PGSIZE);
  memmove(mem, msg, strlen(msg));

  if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, PTE_R | PTE_U) < 0){
    kfree(mem);
    return -1;
  }

  return 0;
}
