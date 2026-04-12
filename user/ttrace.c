#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc < 3){
        fprintf(2, "Uso: ttrace <mask> <programa> [args...]\n");
        fprintf(2, "Ejemplo: ttrace 32 grep hello README\n");
        exit(1);
    }

    int mask = atoi(argv[1]);

    if(trace(mask) < 0){
        fprintf(2, "ttrace: error activando trace\n");
        exit(1);
    }

    // exec reemplaza este proceso — la mascara se mantiene
    // porque trace_mask ya esta guardado en struct proc
    exec(argv[2], &argv[2]);

    fprintf(2, "ttrace: exec fallo para '%s'\n", argv[2]);
    exit(1);
}