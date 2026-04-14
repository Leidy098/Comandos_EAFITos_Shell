# README - EAFITos

EAFITos es una shell educativa implementada en C sobre xv6, desarrollada como proyecto del curso de Sistemas Operativos de la Universidad EAFIT. Combina comandos básicos de interacción con el sistema y comandos avanzados de gestión de memoria mediante syscalls personalizadas.

---

## Cómo ejecutarlo en xv6

1. Compila y arranca xv6:

   make qemu

2. En la consola de xv6, ejecuta la shell:

   EAFITos

Verás el prompt:

   ===============================================================
                       Bienvenido a EAFITos
   ===============================================================
   EAFITos>

---

## Comandos de la shell (Proyecto 1)

ayuda          - Muestra la lista de comandos disponibles
listar         - Lista el contenido del directorio actual (ls)
leer <archivo> - Muestra el contenido de un archivo (cat)
tiempo         - Muestra el tiempo desde el arranque en HH:MM:SS
calc <n1> <op> <n2> - Calculadora básica con + - * /
crear <archivo>     - Crea un archivo vacío
eliminar <archivo>  - Elimina un archivo con confirmación
limpiar        - Simula limpiar la pantalla
salir          - Cierra la shell EAFITos

Ejemplo de sesión básica:

   EAFITos> ayuda
   EAFITos> listar
   EAFITos> leer EAFITos.txt
   EAFITos> tiempo
   EAFITos> calc 10 * 4
   Resultado: 40
   EAFITos> crear notas.txt
   EAFITos> eliminar notas.txt
   EAFITos> salir

---

## Comandos de gestión de memoria (Proyecto 2)
--- 1: Primera syscall de punta a punta - hello() ---

Se creó una syscall mínima llamada hello() que retorna el número 42.
El objetivo fue entender el camino completo que recorre una syscall:
desde que el usuario la invoca hasta que el kernel la procesa y retorna
el resultado. También se instrumentó temporalmente el manejador de traps
para ver en consola cuándo ocurre una ecall desde usuario.

Cómo correrlo:
   thello

Salida esperada:
   thello: hello() retorno 42

--- Punto 2: Mini strace - trazado selectivo de syscalls ---

Se implementó la syscall trace(int mask) que permite trazar selectivamente
las llamadas al sistema de un proceso mediante una máscara de bits.

Uso:
   ttrace <mascara> <programa> [args...]

Ejemplos:
   ttrace 128 echo hola       (traza exec, syscall 7)
   ttrace 65536 echo hola     (traza write, syscall 16)

Salida esperada:
   [strace] pid=3 name=ttrace syscall#=7 a0=16336 a1=16288 a2=9
   hola

--- Punto 3: Explorar la tabla de páginas - dumpvm() ---

Se creó la syscall dumpvm() que imprime la tabla de páginas del proceso
que la invoca. Muestra cada entrada válida con su nivel de indentación,
dirección física y permisos de memoria

Cómo correrlo:
   tdumpvm

Salida esperada (ejemplo):
   tdumpvm: PID=3 - tabla ANTES de asignacion:
   page table 0x0000000087f6b000
    ..0: pte=0x... pa=0x... [RU]
    ..1: pte=0x... pa=0x... [RWU]
    ..2: pte=0x... pa=0x... [XU]
   
   tdumpvm: tabla DESPUES de asignacion local:
   page table 0x0000000087f6b000
    ..0: pte=0x... pa=0x... [RU]
    ..1: pte=0x... pa=0x... [RWU]
    ..2: pte=0x... pa=0x... [XU]
    ..3: pte=0x... pa=0x... [RWU]

--- Punto 4: Permisos de memoria - página solo-lectura con page fault ---

Se implementó la syscall map_ro(void *va) que mapea una página de memoria
con permisos solo-lectura (PTE_R | PTE_U, sin PTE_W). El kernel copia un
mensaje corto a esa página. 

Uso:
   tmemro

Salida esperada:
   tmemro: contenido leido: Pagina solo lectura desde kernel
   tmemro: intentando escribir (debe fallar)...
   [map_ro] page fault: scause=15 stval=0x0000000040000000 pid=4

El proceso es terminado por el kernel al detectar la escritura ilegal
y el shell recupera el control sin pánico.

  --- Punto 5: Robustez con punteros inválidos - tuargs ---

Se creó el programa tuargs que prueba qué pasa cuando se le pasan
punteros inválidos a las syscalls. El objetivo fue verificar que el
kernel detecta correctamente los errores y retorna -1 sin caerse ni
entrar en pánico. También se agregaron impresiones de depuración para
ver cómo los registros a0..a5 se transforman en argumentos de alto nivel.

Cómo correrlo:
   tuargs

Salida esperada:
    ok
    tuargs: write puntero valido -> 3 OK
    tuargs: write puntero invalido -> -1 OK
    tuargs: open string invalido -> -1 OK
    tuargs: pipe puntero valido -> 0 OK
    tuargs: pipe puntero invalido -> -1 OK  

## Detalles técnicos

- La shell usa fork() + exec() para delegar listar y leer a ls y cat.
- trace_mask se hereda de padre a hijo en kfork(), por lo que el trazado
  persiste después de exec.
- map_ro asigna una página física con kalloc(), copia un mensaje desde el
  kernel y la mapea con mappages() sin el bit PTE_W.
- El manejo de page fault en trap.c usa walk() para verificar que la página
  existe antes de desmapearla, evitando pánico en freewalk.
- El parseo de entrada de la shell admite hasta 5 argumentos separados
  por espacios o tabulaciones.

---

## Limitaciones conocidas

- calc solo trabaja con enteros.
- limpiar no borra la terminal realmente, solo imprime saltos de línea.
- El buffer de entrada de la shell es de 128 caracteres.
- No soporta comillas ni rutas con espacios.

---

## Referencias

- MIT. (2023). A simple, Unix-like teaching operating system.
  https://pdos.csail.mit.edu/6.828/2025/xv6.html
- Silberschatz, A., Galvin, P. B., & Gagne, G. (2020).
  Operating System Concepts (10th ed.). Wiley.
- Tanenbaum, A. S., & Bos, H. (2015).
  Modern Operating Systems (4th ed.). Pearson Education.
- Kerrisk, M. (2010). The Linux Programming Interface. No Starch Press.

---

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6). xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

Para compilar y correr xv6 necesitas el toolchain RISC-V newlib desde
https://github.com/riscv/riscv-gnu-toolchain y qemu compilado para
riscv64-softmmu. Una vez instalados corre: make qemu

Reportar errores a: kaashoek,rtm@mit.edu