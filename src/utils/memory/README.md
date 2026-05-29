# Memory Utils — Proyecto 4: mmap/munmap con Lazy Allocation

Implementación de memoria mapeada a archivos (`mmap`/`munmap`) en el kernel de xv6. La memoria física **no se asigna al llamar `mmap`** — solo se carga la página exacta cuando el proceso la toca por primera vez (lazy allocation).

---

## Archivos de esta carpeta

| Archivo | Rol |
|---------|-----|
| `tmmapfile.c` | Programa de prueba con 3 casos: lectura, escritura MAP_SHARED y multipágina |
| `memory_syscalls.c` | Implementaciones de syscalls del kernel (proyectos anteriores) |

---

## Archivos del kernel modificados

| Archivo | Qué se cambió |
|---------|---------------|
| `kernel/proc.h` | `struct vma` y `vmas[MAXVMA]` en `struct proc` |
| `kernel/sysproc.c` | `sys_mmap()` y `sys_munmap()` |
| `kernel/trap.c` | `mmap_fault()`: atiende page faults de regiones VMA |
| `kernel/proc.c` | Copia de VMAs en `fork()`, liberación en `exit()` |
| `kernel/fcntl.h` | Constantes `PROT_READ/WRITE/EXEC`, `MAP_SHARED/PRIVATE` |
| `kernel/syscall.h` / `syscall.c` | Registra `SYS_mmap` (#28) y `SYS_munmap` (#29) |
| `user/user.h` / `user/usys.pl` | Declaraciones y stubs para espacio de usuario |
| `Makefile` | Regla y entrada en UPROGS para `tmmapfile` |

---

## Nuevas syscalls

### `mmap(addr, length, prot, flags, fd, offset)`

Mapea un archivo en el espacio de memoria virtual del proceso sin asignar memoria física.

| Parámetro | Descripción |
|-----------|-------------|
| `addr` | Dirección sugerida (0 = el kernel elige) |
| `length` | Tamaño en bytes del mapeo |
| `prot` | Permisos: `PROT_READ`, `PROT_WRITE`, `PROT_EXEC` |
| `flags` | `MAP_PRIVATE` (cambios locales) o `MAP_SHARED` (cambios van al archivo) |
| `fd` | Descriptor del archivo a mapear |
| `offset` | Byte de inicio dentro del archivo |

Retorna la dirección virtual del inicio del mapeo, o `-1` en error.

### `munmap(addr, length)`

Desmapea una región creada con `mmap`. Si era `MAP_SHARED`, vuelca los cambios al archivo antes de liberar.

Retorna `0` en éxito, `-1` si la dirección no corresponde a ninguna VMA.

---

## Cómo funciona paso a paso

```
1. mmap(fd, 4096, PROT_READ, MAP_PRIVATE, fd, 0)
   ├── Valida parámetros (length > 0, fd válido)
   ├── Busca ranura libre en vmas[]
   ├── Reserva espacio virtual (p->sz += 4096)
   ├── Registra la VMA (used, addr, length, prot, file, offset)
   └── Retorna dirección virtual  ← NO hay RAM asignada

2. char c = addr[0]  ← proceso accede por primera vez
   └── PAGE FAULT (scause=13 lectura / scause=15 escritura)
       └── usertrap() → mmap_fault():
             ├── Busca la VMA que contiene la dirección
             ├── kalloc()   → asigna página física (4096 bytes)
             ├── readi()    → carga datos del archivo en esa página
             ├── mappages() → conecta dirección virtual ↔ física
             └── p->pf_count++

3. munmap(addr, 4096)
   ├── Busca la VMA correspondiente
   ├── Si MAP_SHARED + PROT_WRITE: writei() → vuelca cambios al disco
   ├── uvmunmap() → libera la página física
   ├── fileclose() → decrementa ref-count del archivo
   └── v->used = 0 → ranura VMA libre
```

---

## Programa de prueba: `tmmapfile`

Compilar y arrancar xv6:

```bash
make qemu
```

Dentro de xv6:

```sh
tmmapfile
```

### Caso 1 — Lectura (`PROT_READ`, `MAP_PRIVATE`)

Crea `mmap_test.txt` con `"Hola mmap xv6!"`, lo mapea y verifica que el page fault carga el contenido correctamente.

```
[Caso 1] Lectura desde archivo mapeado
  mmap retorno 0x....  (sin memoria fisica aun)
  primer byte leido: 'H' (page fault resuelto OK)
  contenido correcto: "Hola mmap xv6!"
  munmap OK
```

### Caso 2 — Escritura (`PROT_READ|PROT_WRITE`, `MAP_SHARED`)

Mapea `mmap_rw.txt` en modo compartido, modifica los primeros bytes y verifica que el archivo en disco quedó actualizado tras `munmap`.

```
[Caso 2] Escritura en memoria mapeada (MAP_SHARED)
  byte inicial: 'A'
  escritura OK: addr[0]='Z' addr[1]='X'
  munmap OK (cambios volcados al archivo)
  archivo actualizado correctamente: 'ZX'
```

### Caso 3 — Múltiples páginas (lazy por demanda)

Crea un archivo de 3 páginas (12 KB) y accede a cada una por separado, demostrando que cada acceso genera su propio page fault.

```
[Caso 3] Acceso multipagina (lazy por demanda)
  pagina 0: primer byte = 'X' (page fault #1 resuelto)
  pagina 1: primer byte = 'X' (page fault #2 resuelto)
  pagina 2: primer byte = 'X' (page fault #3 resuelto)
  munmap multipagina OK

=== tmmapfile: SUCCESS ===
```

---

## Decisiones de diseño

- **Sin `kalloc()` en `mmap`:** La RAM se asigna únicamente cuando el proceso accede a la página. Si nunca accede, nunca se gasta memoria física.
- **`filedup` al crear la VMA:** El archivo no se cierra mientras exista el mapeo, aunque el proceso cierre el descriptor `fd` original.
- **`fork` copia VMAs + `filedup`:** El hijo hereda todos los mapeos con referencias independientes al archivo. Ambos pueden hacer `munmap` o terminar sin invalidar al otro.
- **`exit` libera todas las VMAs:** Se recorren todas las entradas activas, se vuelcan cambios `MAP_SHARED` y se liberan páginas para evitar memory leaks.
- **`mmap_fault` tiene prioridad sobre el heap lazy:** En `usertrap()`, el handler de mmap se invoca antes que `vmfault()` porque `mmap` extiende `p->sz` y el handler de heap lo confundiría con memoria anónima.
