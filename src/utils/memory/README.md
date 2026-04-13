# Memory Utils

Esta carpeta centraliza el trabajo adicional que se hizo para los 5 comandos
relacionados con syscalls, memoria y robustez.

## Codigo fuente movido aqui

### Kernel

- `memory_syscalls.c`
  - Contiene las implementaciones de:
    - `sys_hello()`
    - `sys_trace()`
    - `sys_dumpvm()`
    - `sys_map_ro()`

### User space

- `thello.c`
  - Prueba `hello()`
- `ttrace.c`
  - Prueba `trace(int mask)`
- `tdumpvm.c`
  - Prueba `dumpvm()`
- `tmemro.c`
  - Prueba `map_ro(void *va)`
- `tuargs.c`
  - Prueba robustez de argumentos, punteros válidos e inválidos

## Que sigue viviendo fuera de esta carpeta

Algunas partes no conviene moverlas porque forman parte del flujo base de xv6:

- `kernel/syscall.c`
  - Registro de syscalls
  - impresión de argumentos crudos `trapframe->a0..a5`
- `kernel/syscall.h`
  - números de syscall
- `kernel/defs.h`
  - prototipos del kernel
- `kernel/proc.h`
  - campo `trace_mask`
- `kernel/console.c`
  - propagación correcta de errores en `copyin/copyout`
- `kernel/vm.c`
  - `copyin`, `copyout`, `copyinstr`
- `user/user.h`
  - declaraciones de syscalls visibles para usuario
- `user/usys.pl`
  - generación de stubs de syscalls
- `Makefile`
  - reglas para compilar desde `src/utils/memory`

## Los 5 comandos

### 1. hello()

Syscall básica de prueba.

- Retorna `42`
- Implementación: `memory_syscalls.c`
- Prueba: `thello.c`

Ejecutar en xv6:

```sh
thello
```

Salida esperada:

```text
hello() returned 42
```

### 2. trace(int mask)

Activa el trazado de syscalls usando una máscara de bits.

- Guarda la máscara en `trace_mask`
- Imprime los registros crudos `a0..a5` antes del handler
- Implementación principal: `memory_syscalls.c`
- Integración del trace: `kernel/syscall.c`
- Prueba: `ttrace.c`

Ejemplo:

```sh
ttrace 33554431 tuargs
```

Salida esperada:

```text
[strace] pid=... name=tuargs syscall#=... raw a0=... a1=... a2=... a3=... a4=... a5=...
```

### 3. dumpvm()

Imprime la tabla de páginas del proceso actual.

- Muestra PID, nombre y tamaño
- Llama a `vmprint(...)`
- Implementación: `memory_syscalls.c`
- Prueba: `tdumpvm.c`

Ejecutar:

```sh
tdumpvm
```

### 4. map_ro(void *va)

Mapea una página de usuario como solo lectura.

- Redondea dirección con `PGROUNDDOWN`
- Reserva memoria física
- Mapea con `PTE_R | PTE_U`
- Implementación: `memory_syscalls.c`
- Prueba: `tmemro.c`

Ejecutar:

```sh
tmemro
```

Resultado esperado:

- la lectura funciona
- la escritura provoca un page fault controlado

### 5. Robustez de argumentos de syscalls

No es una syscall nueva, sino una mejora del sistema.

Incluye:

- impresión de `trapframe->a0..a5`
- validación de punteros
- retorno `-1` para punteros inválidos
- manejo correcto de errores sin `panic`

Archivos importantes:

- `kernel/syscall.c`
- `kernel/console.c`
- `kernel/vm.c`
- `tuargs.c`

Ejecutar:

```sh
tuargs
```

Salida esperada:

```text
ok
tuargs: write puntero valido -> 3 OK
tuargs: write puntero invalido -> -1 OK
tuargs: open string invalido -> -1 OK
tuargs: pipe puntero valido -> 0 OK
tuargs: pipe puntero invalido -> -1 OK
```

## Como compilar y probar

Desde la raiz del proyecto:

```bash
make
make qemu
```

Dentro de xv6 puedes probar:

```sh
thello
tdumpvm
tmemro
tuargs
ttrace 33554431 tuargs
```

## Nota de organizacion

Los programas de prueba y las syscalls nuevas ahora viven en
`src/utils/memory`, pero siguen compilando como comandos normales de xv6
porque el `Makefile` fue ajustado para construirlos desde esta carpeta.
