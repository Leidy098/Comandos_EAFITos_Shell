#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    // Eager allocation for shrink/grow or when the caller requests it.
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazy allocation: reserve the address range in the process size,
    // but do not allocate physical pages yet. The page fault handler
    // will allocate pages on demand when the program accesses them.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

uint64
sys_getpfcount(void)
{
  return myproc()->pf_count;
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// mmap: mapea un archivo en memoria virtual sin asignar memoria física (lazy).
// Argumentos: addr, length, prot, flags, fd, offset
// Retorna la dirección virtual de inicio del mapeo, o -1 en error.
uint64
sys_mmap(void)
{
  uint64 addr, length, offset;
  int prot, flags, fd;
  struct proc *p = myproc();
  struct file *f;

  // Leer argumentos del trapframe
  argaddr(0, &addr);
  argaddr(1, &length);
  argint(2, &prot);
  argint(3, &flags);
  argint(4, &fd);
  argaddr(5, &offset);

  // Validar parámetros básicos
  if(length == 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f = p->ofile[fd]) == 0)
    return -1;

  // Si el mapeo es compartido y escribible, el archivo debe ser escribible
  if((prot & PROT_WRITE) && (flags & MAP_SHARED) && !f->writable)
    return -1;

  // Buscar una entrada VMA libre
  struct vma *v = 0;
  for(int i = 0; i < MAXVMA; i++){
    if(!p->vmas[i].used){
      v = &p->vmas[i];
      break;
    }
  }
  if(v == 0)
    return -1;  // sin slots libres

  // Ubicar el mapeo al final del espacio virtual actual del proceso
  uint64 va = PGROUNDUP(p->sz);
  uint64 rounded = PGROUNDUP(length);

  // Verificar que no supere TRAPFRAME
  if(va + rounded > TRAPFRAME)
    return -1;

  // Reservar el espacio virtual (sin asignar memoria física — lazy allocation)
  p->sz = va + rounded;

  // Registrar la VMA
  v->used   = 1;
  v->addr   = va;
  v->length = rounded;
  v->prot   = prot;
  v->flags  = flags;
  v->file   = filedup(f);  // incrementa ref-count del archivo
  v->offset = offset;

  return va;  // retornar dirección virtual; NO se llama kalloc()
}

// munmap: desmapea una región previamente mapeada con mmap.
// Argumentos: addr, length
// Retorna 0 en éxito, -1 en error.
uint64
sys_munmap(void)
{
  uint64 addr, length;
  struct proc *p = myproc();

  argaddr(0, &addr);
  argaddr(1, &length);

  // Buscar la VMA que contiene la dirección indicada
  struct vma *v = 0;
  for(int i = 0; i < MAXVMA; i++){
    if(p->vmas[i].used &&
       addr >= p->vmas[i].addr &&
       addr <  p->vmas[i].addr + p->vmas[i].length){
      v = &p->vmas[i];
      break;
    }
  }
  if(v == 0)
    return -1;  // no existe VMA para esa dirección

  // Redondear addr hacia abajo a límite de página
  uint64 va_start = PGROUNDDOWN(addr);
  uint64 va_end   = PGROUNDUP(addr + length);

  // Liberar páginas físicas que hayan sido asignadas (lazy: puede que no todas)
  for(uint64 va = va_start; va < va_end; va += PGSIZE){
    // walk() devuelve el PTE; si la página fue mapeada, liberarla
    pte_t *pte = walk(p->pagetable, va, 0);
    if(pte && (*pte & PTE_V)){
      // Si MAP_SHARED y tiene escritura, volcar cambios al archivo
      if((v->flags & MAP_SHARED) && (v->prot & PROT_WRITE)){
        uint64 file_off = v->offset + (va - v->addr);
        ilock(v->file->ip);
        writei(v->file->ip, 1, va, file_off, PGSIZE);
        iunlock(v->file->ip);
      }
      uvmunmap(p->pagetable, va, 1, 1);  // 1 página, do_free=1
    }
  }

  // Cerrar referencia al archivo y marcar VMA como libre
  fileclose(v->file);
  v->used = 0;

  return 0;
}
