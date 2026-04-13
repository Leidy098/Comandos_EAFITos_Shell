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
  return 42;
}

uint64
sys_trace(void)
{
  int mask;

  argint(0, &mask);
  myproc()->trace_mask = mask;
  return 0;
}

uint64
sys_dumpvm(void)
{
  struct proc *p = myproc();

  printf("dumpvm: pid=%d name=%s sz=%p\n", p->pid, p->name, (void *)p->sz);
  vmprint(p->pagetable);
  return 0;
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
