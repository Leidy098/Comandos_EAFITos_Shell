# README - `EAFITos.c`

## Como ejecutarlo en xv6

`EAFITos` ya aparece en `UPROGS` dentro del `Makefile`, por lo que se compila con el sistema.

1. Compila y arranca xv6 con `make qemu`.
2. En la consola de xv6, ejecuta:

```sh
EAFITos
```

Veras el prompt:

```text
EAFITos>
```

## Comandos disponibles

- `ayuda`
  - Muestra la lista de comandos soportados.

- `listar`
  - Lista el contenido del directorio actual (internamente ejecuta `ls`).

- `leer <archivo>`
  - Creé un archivo de ejemplo usa: `leer EAFITos.txt`
  - Muestra el contenido de un archivo (internamente ejecuta `cat`).
  
- `tiempo`
  - Muestra el tiempo transcurrido desde el arranque de xv6 en formato `HH:MM:SS`.

- `calc <num1> <operador> <num2>`
  - Calculadora basica con `+`, `-`, `*`, `/`.
  - Ejemplos:
    - `calc 2 + 3`
    - `calc 8 / 2`

- `crear <archivo>`
  - Crea un archivo vacio (o lo abre si ya existe).
  - Ejemplo: `crear notas.txt`

- `eliminar <archivo>`
  - Elimina un archivo con confirmacion (`si/no`).
  - Ejemplo: `eliminar notas.txt`

- `limpiar`
  - Simula limpiar pantalla imprimiendo lineas en blanco.

- `salir`
  - Cierra el shell `EAFITos`.

## Ejemplo de sesion

```text
$ EAFITos
EAFITos> ayuda
EAFITos> listar
EAFITos> leer EAFITos.txt
EAFITos> tiempo
EAFITos> crear notas.txt
EAFITos> eliminar notas.txt
EAFITos> calc 10 * 4
Resultado: 40
EAFITos> limpiar
EAFITos> salir
```

## Detalles tecnicos (implementacion)

- Usa `fork()` + `exec()` para delegar `listar` y `leer` a `ls` y `cat`.
- Usa `uptime()` para calcular el tiempo desde el arranque.
- Usa `open(..., O_CREATE | O_WRONLY)` para crear archivos.
- Usa `unlink()` para eliminar archivos.
- El parseo de entrada separa argumentos por espacios/tabulaciones y admite hasta **5 argumentos**.

## Limitaciones conocidas

- No soporta comillas ni rutas con espacios.
- `calc` solo trabaja con enteros.
- `limpiar` no borra la terminal realmente; solo imprime saltos de linea.
- El buffer de entrada es de 128 caracteres.



xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Abhinavpatel00, Takahiro Aoyagi, Marcelo Arroyo, Hirbod Behnam, Silas
Boyd-Wickizer, Anton Burtsev, carlclone, Ian Chen, clivezeng, Dan
Cross, Cody Cutler, Mike CAT, Tej Chajed, Asami Doi,Wenyang Duan,
echtwerner, eyalz800, Nelson Elhage, Saar Ettinger, Alice Ferrazzi,
Nathaniel Filardo, flespark, Peter Froehlich, Yakir Goaron, Shivam
Handa, Matt Harvey, Bryan Henry, jaichenhengjie, Jim Huang, Matúš
Jókay, John Jolly, Alexander Kapshuk, Anders Kaseorg, kehao95,
Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt, Eddie Kohler, Vadim
Kolontsov, Austin Liew, l0stman, Pavan Maddamsetti, Imbar Marinescu,
Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi Merimovich,
mes900903, Mark Morrissey, mtasm, Joel Nider, Hayato Ohhashi,
OptimisticSide, papparapa, phosphagos, Harry Porter, Greg Price, Zheng
qhuo, Quancheng, RayAndrew, Jude Rich, segfault, Ayan Shafqat, Eldar
Sehayek, Yongming Shen, Fumiya Shigemitsu, snoire, Taojie, Cam Tenny,
tyfkda, Warren Toomey, Stephen Tu, Alissa Tung, Rafael Ubal, unicornx,
Amane Uehara, Pablo Ventura, Luc Videau, Xi Wang, WaheedHafez, Keiichi
Watanabe, Lucas Wolf, Nicolas Wolovick, wxdao, Grant Wu, x653, Andy
Zhang, Jindong Zhang, Icenowy Zheng, ZhUyU1997, and Zou Chang Wei.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu).  The main purpose of xv6 is as a teaching
operating system for MIT's 6.1810, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu.  Once they are installed, and in your shell
search path, you can run "make qemu".
