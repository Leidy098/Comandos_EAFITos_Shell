#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define TICKS_PER_SEC 10

int
main(void)
{
  // Buffer de entrada y arreglo de punteros para argumentos parseados.
  char line[128];
  char *args[6];

  printf("\n===============================================================\n");
  printf("                    Bienvenido a EAFITos                    \n");
  printf("===============================================================\n");
  printf("\nEscribe ayuda para ver comandos.\n");

  while (1) {
    // Reinicia estado de parseo para cada nueva linea.
    int argc = 0;
    int inword = 0;

    printf("\nEAFITos> ");
    if (gets(line, sizeof(line)) == 0)
      break; // EOF Por si la lectura falla

    // Recorre la línea de entrada y separa las palabras en función de los espacios reemplazándolos por 0 y guardando punteros a cada palabra en args.
    for (int i = 0; line[i] != 0; i++) {
      if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r') {
        line[i] = 0;
        inword = 0;
      } else if (!inword) {
        if (argc < 6 - 1) // Limita a 5 argumentos para evitar overflow
          args[argc++] = &line[i];
        inword = 1;
      }
    }
    args[argc] = 0; // Indica el final de los argumentos

    // Que continue si presionamos Enter sin escribir nada o solo espacios.
    if (argc == 0)
      continue;

      
    // ----------------------------------------
    // Comando 'Salir' para terminar el shell.
    // ----------------------------------------

    if (strcmp(args[0], "salir") == 0) {
      break;
    }

    // ----------------------------------------
    // Comando 'Ayuda' muestra lista de comandos.
    // ----------------------------------------

    if (strcmp(args[0], "ayuda") == 0) {
      printf("\nComandos:\n");
      printf("  listar\n");
      printf("  leer <archivo>\n");
      printf("  tiempo\n");
      printf("  calc <num1> <operador> <num2>\n");
      printf("  crear <archivo>\n");
      printf("  eliminar <archivo>\n");
      printf("  limpiar\n");
      printf("  ayuda\n");
      printf("  salir\n");
      continue;
    }

    // ----------------------------------------
    // Comando 'Listar' muestra contenido del directorio actual.
    // ----------------------------------------

    if (strcmp(args[0], "listar") == 0) {
      // Ejecuta "ls" en un proceso hijo y espera a que termine.
      int pid = fork();
      if (pid == 0) {
        char *v[] = {"ls", 0};
        exec("ls", v);
        fprintf(2, "No se pudo ejecutar ls\n");
        exit(1);
      }
      if (pid > 0)
        wait(0);
      continue;
    }

    // ----------------------------------------
    // Comando 'Leer <archivo>' muestra contenido de un archivo de texto
    // ----------------------------------------

    if (strcmp(args[0], "leer") == 0) {
      if (argc < 2) {
        printf("Uso: leer <archivo>\n");
        continue;
      }

      // Ejecuta "cat <archivo>" en hijo para mostrar el contenido.
      int pid = fork();
      if (pid == 0) {
        char *v[] = {"cat", args[1], 0};
        exec("cat", v);
        fprintf(2, "No se pudo ejecutar cat\n");
        exit(1);
      }
      if (pid > 0)
        wait(0);
      continue;
    }

    // ----------------------------------------
    // Comando 'Tiempo' muestra el tiempo transcurrido desde el arranque.
    // ----------------------------------------

    if (strcmp(args[0], "tiempo") == 0) {
      // Convierte ticks desde arranque a horas/minutos/segundos.
      int ticks = uptime();
      int total = ticks / TICKS_PER_SEC;
      int hora = total / 3600;
      int minuto = (total % 3600) / 60;
      int segundo = total % 60;
      printf("Tiempo desde arranque: %d%d:%d%d:%d%d\n",
             hora / 10, hora % 10,
             minuto / 10, minuto % 10,
             segundo / 10, segundo % 10);
      continue;
    }

    // ----------------------------------------
    // Comando 'Limpiar' simula limpiar la pantalla.
    // ----------------------------------------

    if (strcmp(args[0], "limpiar") == 0) {
      // Simula limpiar pantalla imprimiendo varias lineas en blanco.
      for (int i = 0; i < 30; i++)
        printf("\n");
      continue;
    }

    // ----------------------------------------
    // Comando 'Crear <archivo>' crea un nuevo archivo vacío.
    // ----------------------------------------

    if (strcmp(args[0], "crear") == 0) {
      int fd;

      if (argc != 2) {
        printf("Uso: crear <archivo>\n");
        continue;
      }

      // Crea archivo vacio (o lo abre si ya existe) y lo cierra.
      fd = open(args[1], O_CREATE | O_WRONLY);
      if (fd < 0) {
        printf("No se pudo crear el archivo\n");
        continue;
      }

      close(fd);
      printf("Archivo creado: %s\n", args[1]);
      continue;
    }

    // ----------------------------------------
    // Comando 'Eliminar <archivo>' elimina archivo con confirmación
    // ----------------------------------------

    if (strcmp(args[0], "eliminar") == 0) {
      char confirm[16];

      if (argc != 2) {
        printf("Uso: eliminar <archivo>\n");
        continue;
      }

      // Confirmacion simple antes de borrar.
      printf("Seguro que quieres eliminar '%s'? (si/no): ", args[1]);
      if (gets(confirm, sizeof(confirm)) == 0) {
        printf("Cancelado\n");
        continue;
      }

      // Acepta "si"
      if (!((confirm[0] == 's') &&
            (confirm[1] == 'i' || confirm[1] == '\n' || confirm[1] == 0))) {
        printf("Cancelado\n");
        continue;
      }

      if (unlink(args[1]) < 0) {
        printf("No se pudo eliminar el archivo\n");
        continue;
      }

      printf("Archivo eliminado: %s\n", args[1]);
      continue;
    }

    // ----------------------------------------
    // Comando 'Calc <num1> <operador> <num2>' calculadora básica.
    // ----------------------------------------

    if (strcmp(args[0], "calc") == 0) {
      int a, b, r;
      char op;

      if (argc != 4) {
        printf("Uso: calc <num1> <operador> <num2>\n");
        continue;
      }

      // Interpreta operandos y operador desde argumentos de texto.
      a = atoi(args[1]);
      b = atoi(args[3]);
      op = args[2][0];

      // Evalua operaciones basicas con validacion de division por cero.
      if (op == '+')
        r = a + b;
      else if (op == '-')
        r = a - b;
      else if (op == '*')
        r = a * b;
      else if (op == '/') {
        if (b == 0) {
          printf("Error: division por cero\n");
          continue;
        }
        r = a / b;
      } else {
        printf("Operador invalido. Usa + - * /\n");
        continue;
      }

      printf("Resultado: %d\n", r);
      continue;
    }

    // Cae aqui cuando el comando no coincide con ninguno de los soportados.
    printf("Comando no reconocido. Escribe ayuda.\n");
  }

  printf("Saliendo de EAFITos \n");
  exit(0);
}
