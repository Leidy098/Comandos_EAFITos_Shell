#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// mmap_fault: maneja un page fault dentro de una región VMA.
// Asigna una página física, carga el contenido del archivo y la mapea.
// Retorna 0 en éxito, -1 si la dirección no pertenece a ninguna VMA.
static int
mmap_fault(struct proc *p, uint64 va)
{
  // Buscar la VMA que contiene va
  struct vma *v = 0;
  for(int i = 0; i < MAXVMA; i++){
    if(p->vmas[i].used &&
       va >= p->vmas[i].addr &&
       va <  p->vmas[i].addr + p->vmas[i].length){
      v = &p->vmas[i];
      break;
    }
  }
  if(v == 0)
    return -1;  // no es una región mapeada

  // Asignar página física y limpiarla
  char *mem = kalloc();
  if(mem == 0)
    return -1;
  memset(mem, 0, PGSIZE);

  // Calcular offset en el archivo para esta página
  uint64 page_va  = PGROUNDDOWN(va);
  uint64 file_off = v->offset + (page_va - v->addr);

  // Leer contenido del archivo en la página física
  ilock(v->file->ip);
  readi(v->file->ip, 0, (uint64)mem, file_off, PGSIZE);
  iunlock(v->file->ip);

  // Construir permisos PTE según prot de la VMA
  int perm = PTE_U;
  if(v->prot & PROT_READ)  perm |= PTE_R;
  if(v->prot & PROT_WRITE) perm |= PTE_W;
  if(v->prot & PROT_EXEC)  perm |= PTE_X;

  // Mapear la página en la tabla de páginas del proceso
  if(mappages(p->pagetable, page_va, PGSIZE, (uint64)mem, perm) < 0){
    kfree(mem);
    return -1;
  }

  p->pf_count++;
  return 0;
}

//
// handle an interrupt, exception, or system call from user space.
// called from, and returns to, trampoline.S
// return value is user satp for trampoline.S to switch to.
//
uint64
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);  //DOC: kernelvec

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call
    // printf("usertrap: ecall sepc=0x%lx syscall=%ld\n",
    //        p->trapframe->epc, p->trapframe->a7);
    if(killed(p))
      kexit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else if(r_scause() == 15 || r_scause() == 13) {
    uint64 fault_va = r_stval();
    int handled = 0;

    // Ej.5: comprobar si el fault cae en una vregion antes que el lazy normal.
    for(int i = 0; i < NVREG; i++){
      struct vregion *vr = &p->vregions[i];
      if(!vr->used)
        continue;
      if(fault_va >= vr->start && fault_va < vr->start + vr->size){
        uint64 pg = PGROUNDDOWN(fault_va);
        if(!ismapped(p->pagetable, pg)){
          char *mem = kalloc();
          if(mem == 0){
            setkilled(p);
          } else {
            memset(mem, 0x41, PGSIZE);  // patrón 'A'
            if(mappages(p->pagetable, pg, PGSIZE, (uint64)mem,
                        PTE_W|PTE_U|PTE_R) != 0){
              kfree(mem);
              setkilled(p);
            } else {
              p->pf_count++;
              handled = 1;
            }
          }
        } else {
          handled = 1;  // ya mapeada (ej. segundo fault en la misma página)
        }
        break;
      }
    }

    // Proyecto 4: mmap page fault — tiene prioridad sobre heap lazy
    // porque mmap extiende p->sz y vmfault lo trataría como heap normal.
    if(!handled && !killed(p)){
      if(mmap_fault(p, fault_va) == 0){
        handled = 1;
      }
    }

    // Ej.3: lazy allocation normal del heap si no fue una vregion ni mmap.
    if(!handled && !killed(p)){
      if(vmfault(p->pagetable, fault_va, (r_scause() == 13)? 1 : 0) != 0){
        p->pf_count++;
        handled = 1;
        printf("lazy page fault handled pid=%d va=%p scause=%ld\n",
               p->pid, (void *)fault_va, r_scause());
      }
    }

    if(!handled && !killed(p)){
      // Fault no recuperable: dirección inválida o sin memoria.
      printf("page fault: pid=%d scause=%ld stval=%p\n",
             p->pid, r_scause(), (void *)fault_va);
      uint64 pg = PGROUNDDOWN(fault_va);
      pte_t *pte = walk(p->pagetable, pg, 0);
      if(pte && (*pte & PTE_V))
        uvmunmap(p->pagetable, pg, 1, 1);
      setkilled(p);
    }
  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    kexit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  prepare_return();

  // the user page table to switch to, for trampoline.S
  uint64 satp = MAKE_SATP(p->pagetable);

  // return to trampoline.S; satp value in a0.
  return satp;
}

//
// set up trapframe and control registers for a return to user space
//
void
prepare_return(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(). because a trap from kernel
  // code to usertrap would be a disaster, turn off interrupts.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000005L){
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}
