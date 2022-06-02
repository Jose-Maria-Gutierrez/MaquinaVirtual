#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "auxiliares.h"

typedef struct
{
    char simbolo[255];
    int posSimbolo;
}TSimbolo;

typedef struct
{
    char mnem[5];
    int cod;
}TMnemonico;

typedef struct
{
    char oper[4];
    int cod;
}TOperando;

typedef struct
{
    char nombreConst[255];
    char textoConst[1000];
}TConstString;

TSimbolo simbYPos[255];
int  contSimb;
char segmento[255];
int tamano;

int buscarSimbolo(TConstString VConsString[255], int N, char nomSim[255]);
void leerSimbolos(char AsmFileName[255], TConstString VConsString[255], int *cantString, int *cantErrores, int *DATA, int *EXTRA, int *STACK);
void traducir(char AsmFileName[255], int *tamCod, int VInstrucciones[4096], int *cantErrores, int output, TConstString VConsString[255], int cantString);
int traduceMnemonico(char *mnemonico);
void extraer(char auxOpe[255], char operando[255], int lon, int i);
void traduceOperando(char *operando, int *tipo, int *valor, int *error);
int esRegistro(char *operando);
int opeInmediato(char *operando,int *esError );
int generaInstruccion(int codop, int tipo1, int valor1, int tipo2, int valor2, int error, int *trunca);
void escrirArchivoBinario(char BinFileName[255], int tamanoCod, int VInstrucciones[], int cantErrores, int DATA, int EXTRA, int STACK);

int main(int argc, char* argv[])
{
    int VInstrucciones[4096];
    int tamanoCod = 0, cantErrores = 0;
    int output = 1, cantString;
    char AsmFileName[255], BinFileName[255];

    int DATA = 1024, EXTRA = 1024, STACK = 1024;

    TConstString VConsString[255];

    if (argc > 1)
    {
        if(argc > 3 && strcmp(argv[3], "-o") == 0)
            output = 0;

       strcpy(AsmFileName, argv[1]);
       strcpy(BinFileName, argv[2]);
    }
    else
    {
       strcpy(AsmFileName, "codigo.asm");
       strcpy(BinFileName, "binario.mv1");
    }

    leerSimbolos(AsmFileName,VConsString,&cantString, &cantErrores, &DATA, &EXTRA, &STACK);

    traducir(AsmFileName, &tamanoCod, VInstrucciones, &cantErrores, output,VConsString, cantString);
    escrirArchivoBinario(BinFileName, tamanoCod, VInstrucciones, cantErrores, DATA, EXTRA, STACK);

    return 0;
}

int buscarSimbolo(TConstString VConsString[255], int N, char nomSim[255])
{
    int i = 0;

    while(i < N && strcmp(simbYPos[i].simbolo, nomSim)!= 0) //busco el simbolo en el vector de simbolos
        i++;

    if(i < N)// lo encontre
        return i;
    else
        return -1;
}

//primer lectura del archivo para rotulos y constantes (se agregan las constantes)
//cambia el nombre de la funcion leerRotulos a leerSimbolos (= rotulo y constantes), tambien cambian los nombres de las variables
void leerSimbolos(char AsmFileName[255], TConstString VConsString[255], int *cantString, int *cantErrores, int *DATA, int *EXTRA, int *STACK)
{
    char linea[1000];
    char **parsed;
    char simbolo[255], auxTexto[255];
    FILE *arch;
    int contPos = 0, auxValor, k = 0, lon, valorSegmento;
    int auxSimb = 0, esError = 0, contErrores = 0;

    //contPos -> posicion de la linea en el codigo maquina NO en el codigo assembler!!!!
    //contSimb -> poscion del simbolo dentro del vector

    contSimb = 0;

    arch = fopen(AsmFileName,"rt");

    if(arch == NULL)
        printf("Error: No existe archivo");
    else
    {
        while(fgets(linea, 1000, arch) != NULL)
        {
            parsed = parseline(linea);

            if(parsed[5] != NULL)
            {
                strcpy(segmento, parsed[5]);
                aMayus(segmento);
                valorSegmento = decToDec(parsed[6]);

                if(strcmp(segmento, "DATA") == 0)
                    *DATA = valorSegmento;
                else if(strcmp(segmento, "EXTRA") == 0)
                    *EXTRA = valorSegmento;
                else if(strcmp(segmento, "STACK") == 0)
                    *STACK = valorSegmento;
            }

            /*necesito: parsed[1] -> rotulos
                        parsed[7] -> nombre constenta
                        parsed[8] -> valor constante
            hay dos tipos de constantes implicitas (valor es un nro que puede estar en cualquier base) y String(lo diferencio por " ")*/

            if(parsed[0] != NULL)//para ROTULOS
            {
                //primero busco para ver qque no sea duplicado
                auxSimb = buscarSimbolo(VConsString, contSimb, parsed[0]);
                printf("%s", VConsString[auxSimb].nombreConst);

                if (auxSimb == -1)
                {
                    strcpy(simbolo, parsed[0]);
                    aMayus(simbolo);

                    //agrego al vector de simbolos
                    strcpy(simbYPos[contSimb].simbolo, simbolo);
                    simbYPos[contSimb].posSimbolo = contPos;

                    contSimb++;
                }
                else
                {
                    printf("Error:: Rotulo duplicado\n");
                    contErrores++;
                }

            }
            else if (parsed[7] != NULL)  //ANALIZAR LAS CONSTE //nombre constante
            {
                //primero busco para ver qque no sea duplicado
                auxSimb = buscarSimbolo(VConsString, contSimb, parsed[7]);

                if (auxSimb == -1)//no esta duplicada
                { //ver si es implicita o string -> analizo el valor

                    strcpy(simbolo, parsed[7]);
                    aMayus(simbolo);

                    if(parsed[8][0] == '"')//constante String
                    {
                        //primero cargo un vector de String y desp al finalizar todo lo cargo al vector de simbolos
                        strcpy(VConsString[k].nombreConst, simbolo); //cargo el nombre

                        //copio el texto sin las comillas -> extraigo
                        lon = strlen(parsed[8]) - 2;
                        strncpy(auxTexto, parsed[8] + 1, lon);
                        auxTexto[lon] = '\0';

                        strcpy(VConsString[k].textoConst, auxTexto);//cargo el texto sin comillas

                        k++;
                    }
                    else
                    {//|PARA COSNTES IMPLICITAS

                        //agrego al vector de simbolos
                        strcpy(simbYPos[contSimb].simbolo, simbolo);

                        //en lugar de guadar la posicion de la linea en el codigo, hay que guardar el valor
                        //el valor de la constante puede venir en cualquer base, entoces uso la funcion operando inmediato
                        auxValor = opeInmediato(parsed[8], &esError);
                        simbYPos[contSimb].posSimbolo = auxValor;
                    }

                    contSimb++;
                }
                else //duplicadi
                {
                    printf("Error:: Rotulo duplicado\n");
                    contErrores++;
                }

            }

            //SOLO TENGO QUE CONTAR EN LOS ROTULOS, xq las constaes no generan codiigo maquina -> descarto todo lo que no tenga mnemonico
            if(parsed[1]!= NULL)
                contPos++;

            freeline(parsed);
        }
    }
    fclose(arch);
    *cantErrores = contErrores;

    //int valor = contPos;// agrego las ctes cuando termina el codigo ///// DIRECTAMENTE utilizo contPos xq agrego celdas!
    //las ctes son simbolos -> uso contSimb;
    for(int j = 0; j < k; j++)
    {
        strcpy(simbYPos[contSimb].simbolo, VConsString[j].nombreConst);
        simbYPos[contSimb].posSimbolo = contPos;//va el valor
        //modifico valor para la siguente
        contPos += strlen(VConsString[j].textoConst)+1; // +1 por el \0

        contSimb++;
    }

    *cantString = k;

}

void traducir(char AsmFileName[255], int *tamCod, int VInstrucciones[4096], int *cantErrores, int output, TConstString VConsString[255], int cantString)
{
    char linea[1000];
    char **parsed;
    char rotulo[255];
    FILE *arch;
    int tipo1, valor1, tipo2, valor2, codop, instruccion, error, trunca;
    int contPos = 0, contErrores = 0;

    arch = fopen(AsmFileName, "rt");

    if(arch == NULL)
        printf("Error: No existe archivo");
    else
    {
        while(fgets(linea, 1000, arch) != NULL)
        {
            parsed = parseline(linea); //linea parseada

            if (parsed[1] != NULL)  // mnemonico
            {
                codop = traduceMnemonico(parsed[1]);

                if(codop != -1) //si tengo codigo operando entonces voy a traducir
                {
                    if(parsed[2] != NULL) //operando 1
                    traduceOperando(parsed[2], &tipo1, &valor1, &error);

                    if(parsed[3] != NULL) // operando 2
                        traduceOperando(parsed[3], &tipo2, &valor2, &error);

                }
                else //Error de sintaxis -> no se traduc
                {
                    contErrores++; // si es != 0 entonces no genero archivo binario
                    printf("Error: Error de sintaxis\n");
                }

                if(error == 1)
                {
                    contErrores++;
                    printf("Error: No se encuntra rotulo\n");
                }

                instruccion = generaInstruccion(codop, tipo1, valor1, tipo2, valor2, error, &trunca);

                //guardo en vector para desp grabar
                VInstrucciones[contPos] = instruccion;

                if(trunca)
                    printf("Warning: Truncado de operando\n");

                if( output == 1)
                {
                    //formato para mostrar por pantalla
                    if (parsed[0] != NULL) //rotulo
                        strcpy(rotulo, parsed[0]);
                    else
                        sprintf(rotulo, "%d", contPos + 1);// +1?

                    printf(
                        "[%04d] %02X %02X %02X %02X %10s: %10s ",
                        contPos,
                        (instruccion >> 24) & 0xFF,
                        (instruccion >> 16) & 0xFF,
                        (instruccion >>  8) & 0xFF,
                        (instruccion >>  0) & 0xFF,
                        rotulo,
                        parsed[1] //mnemonico
                    );

                    if (parsed[2] != NULL) //operando
                    {
                        printf("%10s", parsed[2]);

                        if (parsed[3] != NULL) //operando
                            printf(", %10s", parsed[3]);
                        else
                            printf("%12s", "");
                    }
                    else
                        printf("%22s", "");

                    if (parsed[4] != NULL) //comentario
                        printf(" ;%s", parsed[4]);

                    printf("\n");
                }

                // para contar ignoro todo lo que no tenga mnemonico
                contPos++; //cuenta la cantidad de lineas de codigo maquina

                error = 0;//inicializa para la proxima instruccion
            }
            else // no es mnemonico
            {
                printf("%s",linea);
            }


            freeline(parsed);
        }
    }
    fclose(arch);
    *cantErrores = contErrores;

    //agregado String al final del cod
    // cada letra en un celda del VInstrucciones

    int N;
    for(int i = 0; i < cantString; i++)//para recorrer el vector de String
    {
        N = strlen(VConsString[i].textoConst);
        for(int j = 0; j < N; j++)//recorre el text
        {
            VInstrucciones[contPos] = VConsString[i].textoConst[j];

            contPos++;
        }

        contPos++;
        VInstrucciones[contPos] = '\0';

    }


    *tamCod = contPos; //cambio desp de agregar los string
}

//problema cuando hay un renglon vacio entre las instrucciones -> SOLUCIÓN verificar mnemonico == NULL -> lo hago en la segunda lectura xq no lo tengo que mandar a traducir si es null
int traduceMnemonico(char *mnemonico)// retorna un valor
{
    static TMnemonico VMnem[28] = {
        {"MOV", 0x0}, {"ADD", 0x1}, {"SUB", 0x2}, {"SWAP", 0x3},
        {"MUL", 0x4}, {"DIV", 0x5}, {"CMP", 0x6}, {"SHL", 0x7},
        {"SHR", 0x8}, {"AND", 0x9}, {"OR", 0xA}, {"XOR", 0xB},
        {"SLEN", 0xC}, {"SMOV", 0xD}, {"SCMP", 0xE}, {"SYS", 0xF0},
        {"JMP", 0xF1}, {"JZ", 0xF2}, {"JP", 0xF3}, {"JN", 0xF4},
        {"JNZ", 0xF5}, {"JNP", 0xF6}, {"JNN", 0xF7}, {"LDL", 0xF8},
        {"LDH", 0xF9}, {"RND", 0xFA}, {"NOT", 0xFB}, {"STOP", 0xFF1},
    };
    int i = 0;

    char auxiliar[255];

    strcpy(auxiliar, mnemonico);
    aMayus(auxiliar);

    while(i < 28 && strcmp (auxiliar, VMnem[i].mnem) != 0)
        i++;

    if (i < 28)
        return VMnem[i].cod;
    else
        return 0xFFFFFFFF;
}

void extraer(char auxOpe[255], char operando[255], int lon, int i)
{
    strncpy(auxOpe, operando + i, lon);
    auxOpe[lon] = '\0';
}

void traduceOperando(char *operando, int *tipoOpe, int *valorOpe, int *error)// devuelve el tipo y el valor
{
    int esError = 0, i;
    char auxOpe[255];
    int valorReg, reg, offset, auxReg;
    char parte1[255], parte2[255];

    valorReg = esRegistro(operando);
    if(valorReg != -1)//es registro
    {
        *tipoOpe = 0b01;
        *valorOpe = valorReg;
    }
    else
    {
        if(operando[0] == '[')//es directo o indirecto
        {
            i = 0;
            extraer(auxOpe, operando, strlen(operando) - 2, 1);//.. extraer el numero sin los [ ]

            //recorrer con while en busca de - o +
            while(i < strlen(auxOpe) && (auxOpe[i] != '+'))
            {
                i++;
                //printf("i:%d",i);
            }

            auxReg = esRegistro(auxOpe);
            if(i < strlen(auxOpe)) // indirecto
            {
                *tipoOpe = 0b11;

                extraer(parte1, auxOpe,  i, 0);
                extraer(parte2 , auxOpe, strlen(auxOpe)-i, i+1);

                //siempre dan valores posibles
                reg = esRegistro(parte1);
                offset = opeInmediato(parte2, &esError);

                if(auxOpe[i] == '-')//considerar signo -
                    offset *=-1;

                *valorOpe = ((offset & 0xFF) << 4)|(reg & 0xF);
            }
            else
                if(auxReg != -1)//indirecto de solo registro
                {
                    *tipoOpe = 0b11;
                    *valorOpe = auxReg & 0xF;
                }
                else // directo
                {
                    *tipoOpe = 0b10;
                    *valorOpe = opeInmediato(auxOpe, &esError);
                }
        }
        else // es inmediato
        {
            *tipoOpe = 0b00;
            *valorOpe = opeInmediato(operando, &esError);
        }
        *error = esError;
    }
}

int esRegistro(char *operando)
{
    static TOperando VOper[34] = {
        {"DS", 0X00}, {"SS", 0X01},{"ES", 0X02}, {"CS", 0X03},
        {"HP", 0X04}, {"IP", 0X35}, {"SP", 0X36}, {"BP", 0X37},
        {"CC", 0X08}, {"AC", 0X09}, {"EAX", 0X0A}, {"EBX", 0X0B},
        {"ECX", 0X0C}, {"EDX", 0X0D}, {"EEX", 0X0E}, {"EFX", 0X0F},
        {"AX", 0x3A}, {"BX", 0x3B}, {"CX", 0x3C}, {"DX", 0x3D},
        {"EX", 0x3E}, {"FX", 0x3F}, {"AH", 0x2A}, {"BH", 0x2B},
        {"CH", 0x2C}, {"DH", 0x2D}, {"EH", 0x2E}, {"FH", 0x2F},
        {"AL", 0x1A}, {"BL", 0x1B}, {"CL", 0x1C}, {"DL", 0x1D},
        {"EL", 0x1E}, {"FL", 0x1F}
    };
    char auxiliar[255];
     int i = 0;

    strcpy(auxiliar, operando);
    aMayus(auxiliar);

    while(i < 34 && strcmp(VOper[i].oper, auxiliar) != 0)//comparo el registro en may
        i++;

    if(i < 34)//es un registro
        return VOper[i].cod;
    else
        return -1;//valor no posible
}


int opeInmediato(char *operando, int *esError)
{
    int longOpe;
    char auxOpe[255];
    int i;
    int valorOperando = 0;

    *esError = 0;
    //recorto el string
    longOpe = strlen(operando) - 1;
    strncpy(auxOpe, operando + 1, longOpe);
    auxOpe[longOpe] = '\0';

    //strncpy(donde lo guardo, lo que recorto + o, cantidad que quiero copiar)
    if(operando[0] == '@' )//octal -> extraigo
        valorOperando = octToDec(auxOpe);
    else
        if(operando[0] == '%')//hexa -> extraigo
            valorOperando = hexToDec(auxOpe);
        else
            if(operando[0] == '\'')//ASCII -> utilizo el de posicion 1
            {
                valorOperando = operando[1];
            }
            else
                if( (operando[0] == '#') || (operando[0] == '-') || (operando[0] >= '0' && operando[0] <= '9'))//DEcimal -> recorto
                    {
                        if(operando[0] == '#') // extraigo
                            valorOperando = decToDec(auxOpe);
                        else // no extraigo
                            valorOperando = decToDec(operando);
                    }
                else // es rotulo
                {
                    strcpy(auxOpe, operando);
                    aMayus(auxOpe);

                    i = 0;
                    //(cambie el nombre de las var de rotulo a simbolo)
                    //contSimb es var global al igual que simbYPos
                    while(i < contSimb && strcmp(simbYPos[i].simbolo, auxOpe)!= 0) //busco el rotulo en el vector de simbolos
                        i++;

                    if(i == contSimb) //error -> No se encuentra el rotulo
                    {
                        valorOperando = -1;
                        *esError = 1;
                    }
                    else
                        if(i < contSimb)
                            valorOperando = simbYPos[i].posSimbolo;
                }
    return valorOperando;
}


//Guardo la traduccion en un entero (por cada instruccion)
int generaInstruccion(int codop, int tipo1, int valor1, int tipo2, int valor2, int error, int *trunca ) // va a devolver un vector de enteros
{
    int instruccion;

    *trunca = 0;

    if(codop == -1)//error de sintaxis
        instruccion = codop;
    else
        if (codop >= 0xFF0) // 0 operandos
            instruccion = codop << 20;
        else
            if (codop >= 0xF0) // 1 operando
            {
                if(tipo1 == 0b00)//es opeInmediato
                    if(valor1 > 32767 || valor1 < -32768) //veo si hay
                        *trunca = 1;

                if(error == 1)//error no encuntra rotulo
                    valor1 = -1;

                instruccion = codop << 24;
                instruccion |= (tipo1 & 0b11) << 22;
                instruccion |= valor1 & 0xFFFF;
            }
            else // 2 operandos
            {
                if(tipo1 == 0b00)//es opeInmediato
                    if(valor1 > 4095 || valor1 < -2048)
                        *trunca = 1;
                if(tipo2 == 0b00)//es opeInmediato
                    if(valor2 > 4095 || valor2 < -2048)
                        *trunca = 1;

                instruccion = codop << 28;
                instruccion |= (tipo1 & 0b11) << 26;
                instruccion |= (tipo2 & 0b11) << 24;
                instruccion |= (valor1 & 0xFFF) << 12;
                instruccion |= valor2 & 0xFFF;
            }
    return instruccion;
}

void escrirArchivoBinario(char BinFileName[255], int tamanoCod, int VInstrucciones[], int cantErrores, int DATA, int EXTRA, int STACK)// grabo en el archivo binario
{
    int Vheader[6];
    FILE *arch;
    int i;

    if(cantErrores == 0) //no tengo errores -> creo archivo, si tengo errores NO se crea
    {
        //armo el header
        Vheader[0] = 'M'<<24 | 'V'<<16 | '-'<<8 | '2'<<0;
        Vheader[1] = DATA;
        Vheader[2] = STACK;
        Vheader[3] = EXTRA;
        Vheader[4] = tamanoCod;//tamano del codigo + String
        Vheader[5] = 'V'<<24 | '.'<<16 | '2'<<8 | '2'<<0;

        //grabo en archivo
        //en las primeras 6 el header, apartir de la 7 las instrucciones

        arch = fopen(BinFileName, "wb");

        for(i = 0; i < 6; i++)
            fwrite(&Vheader[i], sizeof(int), 1, arch);

        for(i = 0; i < tamanoCod; i++)
            fwrite(&VInstrucciones[i], sizeof(int), 1, arch);

        fclose(arch);

    }
    else
        printf("NO SE GENERO ARCHIVO");
}

