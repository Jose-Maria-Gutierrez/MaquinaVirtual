//tamaños posta
#define MAXREGISTRO 16
#define MAXMEMORIA 8192 //32Kib
#define MAXSTRING 100
#define MAXVDD 255
#define CUATROBYTES 32
//registros
#define DS 0
#define SS 1
#define ES 2
#define CS 3
#define HP 4
#define IP 5
#define SP 6
#define BP 7
#define CC 8
#define AC 9
//mascaras para registros
#define E 0xffffffff
#define L 0x000000ff
#define H 0x0000ff00
#define X 0x0000ffff

#define nulo -1
//headers
void procesar();
void leerArchivo();
void leerInstruccion(int ,int *,int *,int *,int *,int *);
void configurarRegistros();
int stringToInt(char [],int );
void definirFunciones();
void ejecutarDisassembler();
void disassembler(int,int,int,int,int,int,int);
void mostrarRegistros();
int valor(int,int);
//2 operandos
void mov(int,int,int,int);
void add(int,int,int,int);
void sub(int,int,int,int);
void swap(int,int,int,int);
void mul(int,int,int,int);
void division(int,int,int,int);
void cmp(int,int,int,int);
void shl(int,int,int,int);
void shr(int,int,int,int);
void and(int,int,int,int);
void or(int,int,int,int);
void xor(int,int,int,int);
void slen(int,int,int,int);
void smov(int,int,int,int);
void scmp(int,int,int,int);
//1 operando
void sys(int,int);
void jmp(int,int);
void jz(int,int);
void jp(int,int);
void jumpn(int,int);
void jnz(int,int);
void jnp(int,int);
void jnn(int,int);
void ldl(int,int);
void ldh(int,int);
void rnd(int,int);
void not(int,int);
void breakpoint();
void push(int,int);
void pop(int,int);
void call(int,int);
//0 operandos
void stop();
void ret();

void tamComienzoReg(int ,int *,int *);

void segmentationFault();
void stackOverflow();
void stackUnderflow();

int numIntruccion;
int cantArg;
char* argGlobales[200];
int cantInstrucciones;

typedef void funciones0();
typedef void funciones1(int,int);
typedef void funciones2(int,int,int,int);


funciones0* fun0[MAXREGISTRO];
funciones1* fun1[MAXREGISTRO];
funciones2* fun2[MAXREGISTRO];

int registros[MAXREGISTRO];
int memoria[MAXMEMORIA];
int breakpointActivado;
int instruccionActual;

typedef struct {
    int numUnidad, cantCilindros, cantCabezas, cantSectores, tamSector, estado;
    char archivo[15];
} VDD;
VDD vdd[MAXVDD];
int cantVDD;

void guardarVDD();

void accesoVDD();
void leerVDD();
void escribirVDD();
void obtenerParametrosVDD();
void crearVDD(int ,char *);


