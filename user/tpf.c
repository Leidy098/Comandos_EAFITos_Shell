#include "kernel/types.h"
#include "user/user.h"

static void
run_load_fault(void)
{
  volatile char *p = (char *)((uint64)sbrk(0) + 2 * 4096);
  volatile char value;

  printf("tpf: load fault desde %p\n", (void *)p);
  value = *p;
  printf("tpf: ERROR load no fallo, valor=%d\n", value);
  exit(1);
}

static void
run_store_fault(void)
{
  volatile char *p = (char *)((uint64)sbrk(0) + 2 * 4096);

  printf("tpf: store fault hacia %p\n", (void *)p);
  *p = 'X';
  printf("tpf: ERROR store no fallo\n");
  exit(1);
}

static void
run_child(void (*fn)(void), char *name)
{
  int pid;
  int status;

  pid = fork();
  if(pid < 0){
    fprintf(2, "tpf: fork fallo para %s\n", name);
    exit(1);
  }

  if(pid == 0)
    fn();

  wait(&status);
  printf("tpf: hijo %s termino con status %d\n", name, status);
}

int
main(void)
{
  run_child(run_load_fault, "load");
  run_child(run_store_fault, "store");
  printf("tpf: prueba terminada\n");
  exit(0);
}
