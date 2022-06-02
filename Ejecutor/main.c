#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tipos.h"

int main(int argc, char* argv[]) //argc cantidad de argumentos / argv argumentos
{
    int i;
    cantArg = argc;
    for(i=0;i<argc;i++)
      argGlobales[i] = argv[i];
    leerArchivo();
    //configurarRegistros();  //configurar registros
    procesar();

    //system("cls");
    //ejecutarDisassembler();
    return 0;
}
