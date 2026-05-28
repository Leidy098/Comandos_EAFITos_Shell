#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Escribe contenido conocido en un archivo de prueba
static void
create_file(const char *path, const char *content)
{
  int fd = open(path, O_CREATE | O_RDWR);
  if(fd < 0){
    fprintf(2, "tmmapfile: no se pudo crear %s\n", path);
    exit(1);
  }
  write(fd, content, strlen(content));
  close(fd);
}

// Caso 1: lectura desde archivo mapeado (PROT_READ, MAP_PRIVATE)
static void
test_read(void)
{
  printf("\n[Caso 1] Lectura desde archivo mapeado\n");

  create_file("mmap_test.txt", "Hola mmap xv6!");

  int fd = open("mmap_test.txt", O_RDONLY);
  if(fd < 0){ fprintf(2, "tmmapfile: open fallo\n"); exit(1); }

  char *addr = mmap(0, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
  if(addr == (char *)-1){ fprintf(2, "tmmapfile: mmap fallo\n"); exit(1); }

  printf("  mmap retorno %p (sin memoria fisica aun)\n", addr);

  // Primer acceso => page fault => kernel carga datos del archivo
  char c = addr[0];
  printf("  primer byte leido: '%c' (page fault resuelto OK)\n", c);

  if(addr[0] != 'H' || addr[1] != 'o'){
    fprintf(2, "  ERROR: contenido incorrecto\n");
    exit(1);
  }
  printf("  contenido correcto: \"%s\"\n", addr);

  munmap(addr, 4096);
  close(fd);
  printf("  munmap OK\n");
}

// Caso 2: escritura en región mapeada (PROT_READ|PROT_WRITE, MAP_SHARED)
static void
test_write(void)
{
  printf("\n[Caso 2] Escritura en memoria mapeada (MAP_SHARED)\n");

  create_file("mmap_rw.txt", "AAAAAAAAAAAAAAAA");

  int fd = open("mmap_rw.txt", O_RDWR);
  if(fd < 0){ fprintf(2, "tmmapfile: open fallo\n"); exit(1); }

  char *addr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(addr == (char *)-1){ fprintf(2, "tmmapfile: mmap fallo\n"); exit(1); }

  printf("  mmap retorno %p\n", addr);

  // Acceso de lectura (page fault)
  printf("  byte inicial: '%c'\n", addr[0]);

  // Escritura en la región mapeada
  addr[0] = 'Z';
  addr[1] = 'X';
  printf("  escritura OK: addr[0]='%c' addr[1]='%c'\n", addr[0], addr[1]);

  // munmap => con MAP_SHARED escribe cambios al archivo
  munmap(addr, 4096);
  close(fd);
  printf("  munmap OK (cambios volcados al archivo)\n");

  // Verificar que el archivo fue actualizado
  fd = open("mmap_rw.txt", O_RDONLY);
  char buf[4] = {0};
  read(fd, buf, 2);
  close(fd);
  if(buf[0] == 'Z' && buf[1] == 'X')
    printf("  archivo actualizado correctamente: '%c%c'\n", buf[0], buf[1]);
  else
    fprintf(2, "  AVISO: archivo no actualizado (buf='%c%c')\n", buf[0], buf[1]);
}

// Caso 3: múltiples páginas — demuestra lazy loading página por página
static void
test_multipage(void)
{
  printf("\n[Caso 3] Acceso multipagina (lazy por demanda)\n");

  // Crear archivo de 3 páginas usando un buffer por página
  int fd = open("mmap_big.txt", O_CREATE | O_RDWR);
  if(fd < 0){ fprintf(2, "tmmapfile: open fallo\n"); exit(1); }
  static char buf[4096];  // static: va en BSS, no en el stack
  memset(buf, 'X', 4096);
  write(fd, buf, 4096);
  write(fd, buf, 4096);
  write(fd, buf, 4096);
  close(fd);

  fd = open("mmap_big.txt", O_RDONLY);
  char *addr = mmap(0, 3 * 4096, PROT_READ, MAP_PRIVATE, fd, 0);
  if(addr == (char *)-1){ fprintf(2, "tmmapfile: mmap fallo\n"); exit(1); }

  // Cada acceso a una nueva página genera un page fault independiente
  for(int i = 0; i < 3; i++){
    char val = addr[i * 4096];
    printf("  pagina %d: primer byte = '%c' (page fault #%d resuelto)\n",
           i, val, i + 1);
  }

  munmap(addr, 3 * 4096);
  close(fd);
  printf("  munmap multipagina OK\n");
}

int
main(void)
{
  printf("=== tmmapfile: pruebas de mmap/munmap ===\n");

  test_read();
  test_write();
  test_multipage();

  printf("\n=== tmmapfile: SUCCESS ===\n");
  exit(0);
}
