# Memory Utils — Proyecto 3: Lazy Allocation

Implementación de Lazy Allocation sobre xv6-riscv (RISC-V, MIT).

## Archivos de esta carpeta

| Archivo | Rol |
|---------|-----|
| `memory_syscalls.c` | Implementaciones de syscalls del kernel |
| `tlazy.c` | Prueba Ej.4: sbrk + accesos + getpfcount |
| `tmmap_sim.c` | Prueba Ej.5: mapzero, páginas rellenas con `'A'` |
| `tsbrk2.c` / `tsbrk3.c` | Pruebas Ej.2/3: lazy allocation básica |

---

## Ejercicios implementados

### Ej.1 — Observar Page Faults (`kernel/trap.c`)
Detección de `scause == 13` (load) y `scause == 15` (store) en `usertrap()`.
Faults inválidos imprimen `page fault: pid=... scause=... stval=...` y matan el proceso.
Prueba: `tpf` (en `user/tpf.c`).

### Ej.2 — `sbrk()` Lazy (`kernel/sysproc.c`)
Para `n > 0`: solo incrementa `p->sz`, sin asignar páginas físicas.
Para `n < 0`: usa `growproc` (comportamiento original).

### Ej.3 — Lazy Allocation real (`kernel/vm.c`, `kernel/trap.c`)
`vmfault()` asigna la página física cuando el proceso la toca por primera vez.
- `uvmunmap` y `uvmcopy` toleran PTEs no mapeadas (`continue`).
- `copyin`, `copyout` y `copyinstr` llaman `vmfault()` si la página no está mapeada.
- Guard page manejada implícitamente vía `ismapped()`.

### Ej.4 — Contador de page faults (`kernel/proc.h`, `kernel/trap.c`, syscall `getpfcount`)
- Campo `int pf_count` en `struct proc`; se inicializa a 0 en `allocproc` y en cada hijo de `fork`.
- `p->pf_count++` solo en faults lazy atendidos exitosamente.
- Syscall `getpfcount()` (#26) retorna el contador del proceso actual.
- Prueba: `tlazy`

```
$ tlazy
tlazy: inicio, pf_count=0
tlazy: sbrk(24576) retorno 0x... (lazy, sin paginas fisicas aun)
tlazy: pf_count tras sbrk=0 (debe ser 0)
...
tlazy: pf_count final=6 (esperado 6)
tlazy: SUCCESS
```

### Ej.5 — Simulación mmap (`kernel/proc.h`, `kernel/trap.c`, syscall `mapzero`)
- `struct vregion {start, size, used}` + `vregions[NVREG]` (NVREG=4) en `struct proc`.
- Syscall `mapzero(size)` (#27): reserva rango virtual sin mapear, registra la vregión.
- `usertrap()` revisa vregiones **antes** de `vmfault()`; rellena cada página con `0x41` (`'A'`).
- Prueba: `tmmap_sim`

```
$ tmmap_sim
tmmap_sim: mapzero(12288) retorno 0x...
tmmap_sim: pagina 0 primer byte = 'A' (0x41)
tmmap_sim: pagina 1 primer byte = 'A' (0x41)
tmmap_sim: pagina 2 primer byte = 'A' (0x41)
tmmap_sim: primera pagina completa OK (4096 bytes = 'A')
tmmap_sim: SUCCESS
```

---

## Archivos del kernel modificados

| Archivo | Qué se cambió |
|---------|---------------|
| `kernel/trap.c` | Manejo de PF: vregion → vmfault → error |
| `kernel/vm.c` | `vmfault()`, `uvmunmap`, `uvmcopy`, `copyin`, `copyout`, `copyinstr` |
| `kernel/proc.h` | `struct vregion`, `pf_count`, `vregions[NVREG]` |
| `kernel/proc.c` | Init en `allocproc` y `kfork` |
| `kernel/sysproc.c` | `sys_sbrk()` lazy, `sys_getpfcount()` |
| `kernel/syscall.h` | Números 26 (`getpfcount`) y 27 (`mapzero`) |
| `kernel/syscall.c` | Externs y entradas en tabla |
| `kernel/param.h` | `NVREG 4` |
| `user/user.h` | Declaraciones `getpfcount`, `mapzero` |
| `user/usys.pl` | Stubs de las nuevas syscalls |
| `Makefile` | Reglas y UPROGS para `tlazy`, `tmmap_sim` |
| `src/utils/memory/memory_syscalls.c` | `sys_mapzero()` |

---

## Compilar y probar

```bash
make qemu
```

Dentro de xv6:

```sh
tpf        # Ej.1: page faults inválidos
tsbrk2     # Ej.2/3: lazy sbrk
tsbrk3     # Ej.3: lazy con múltiples páginas
tlazy      # Ej.4: getpfcount
tmmap_sim  # Ej.5: mapzero con patrón 'A'
```
