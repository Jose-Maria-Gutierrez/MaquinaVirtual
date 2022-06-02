#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tipos.h"

void procesar(){
  int f,instruccion,mnemo,opA,a,opB,b,in;
  definirFunciones();
  configurarRegistros();  //configurar registros

  guardarVDD();

  for(f=2;f<cantArg;f++){//si esta la flag [-c] la ejecuto
     if(strcmp(argGlobales[f],"-c") == 0)
      system("cls");
  }

  for(f=2;f<cantArg;f++){
    if(strcmp(argGlobales[f],"-d") == 0) //disassembler
      ejecutarDisassembler();
  }

  //guardarVDD();

  //(*fun1[0x0])(0,0xF); //hardcodeo breakpoint

  in = registros[IP] & X; //donde arranca el IP dentro del CS
  while(in>=0 && in<cantInstrucciones){
    instruccion = memoria[in];  //leo instruccion
    leerInstruccion(instruccion,&mnemo,&opA,&a,&opB,&b); //decodifico instruccion y operandos
    instruccionActual = registros[IP]++; //aumento IP
    if(mnemo>=0x00000000 && mnemo<=0x0000000e)//2 operandos
        (*fun2[mnemo])(opA,a,opB,b);           //ejecutar instruccion 2 operandos
    else if(mnemo>=0x000000f0 && mnemo<=0x000000fe){
        mnemo = mnemo & 0x0000000f;
        (*fun1[mnemo])(opA,a); //ejecutar instruccion 1 operando
    }else if(mnemo==0x00000ff0 || mnemo==0x00000ff1){ //no operandos
      if(mnemo==0x00000ff0)
        (*fun0[0])();
      else
        (*fun0[1])();
    }

    if(breakpointActivado){
      if(((instruccion&0xff000000)>>24)!=0xf0) //si no es un sys
        breakpoint();
      else if(((instruccion&0xff000000)>>24)==0xf0 && valor(opA,a)!=0xf) //si es sys pero no breakpoint
        breakpoint();
    } //si esta activado breakpoint dsp de cada operacion

    in = registros[IP] & X;
  }

}

void leerArchivo(){
  int instruccion;
  int tamanoCodigo,i,header[6];
  char filename[MAXSTRING];
  strcpy(filename,argGlobales[1]);
  FILE *archivo;

  if((archivo = fopen(filename,"rb")) != NULL){ //existe el archivo
    registros[DS] = 0x00000000;
    registros[ES] = 0x00000000;
    registros[SS] = 0x00000000;
    registros[CS] = 0x00000000;

    for(i=0;i<6;i++){ //leo el header
      if(fread(&instruccion,sizeof(int),1,archivo)) //supongo que el header esta completo
       header[i] = instruccion;
      //printf("%0X\n",instruccion);
    }

    if(header[0]==0x4D562D32 && header[5]==0x562E3232){ //"MV-2" y "V.22"
        tamanoCodigo = 0;

        ldl(0,tamanoCodigo); //inicia en 0
        ldh(0,header[4]); //tamaño
        registros[CS] = registros[AC];

        tamanoCodigo = header[4] & X;
        ldl(0,tamanoCodigo);
        ldh(0,header[1]);
        registros[DS] = registros[AC];

        tamanoCodigo += header[1] & X;
        ldl(0,tamanoCodigo);
        ldh(0,header[3]);
        registros[ES] = registros[AC];

        tamanoCodigo += header[3] & X;
        ldl(0,tamanoCodigo);
        ldh(0,header[2]);
        registros[SS] = registros[AC];

        tamanoCodigo += header[2] & X;

        if(tamanoCodigo < MAXMEMORIA){ //capaz de almacenar las intrucciones
        i = 0;
          while(fread(&instruccion,sizeof(int),1,archivo)){ //lee archivo binario
            memoria[i++] = instruccion;
            //leerInstruccion(instruccion,&mnemo,&opA,&a,&opB,&b);
            //printf("%08X %04X %d %d %d %d\n",instruccion,mnemo,opA,a,opB,b);
            //disassembler(i++,instruccion,mnemo,opA,a,opB,b); //disassembler de la instruccion
          }
          cantInstrucciones = i;
        }else{
          registros[CS] = 0x00000000;
          printf("el proceso no puede ser cargado por memoria insuficiente");
        }
    }else{
      registros[CS] = 0x00000000;
      printf("el formato de archivo %s no es correcto",filename);
    }
  }else //no pudo abrir el archivo
    registros[CS] = 0x00000000;
  fclose(archivo);
}

void leerInstruccion(int num,int *mnemo,int *opA,int *a,int *opB,int *b){
  *mnemo = num >> 28;          //leo el mnemonico
  *mnemo = *mnemo & 0x0000000f;
  if(*mnemo<=0xe){ //2 operandos
    *opA = num >> 26;
    *opA = *opA & 0x00000003;
    *a = (num>>12) & 0x00000fff;
    if(*opA==1){    //registro
      if((*a&0x0000000f)>=0x0 && (*a&0x0000000f)<=0x9){ //registros de 0 a 9
        *a = *a & 0x0000000f;
      }else if((*a&0x0000000f)>=0xA && (*a&0x0000000f)<=0xF){ //registros de EAX a EFX
        *a = *a & 0x0000003f;
      }
    }else if(*opA==2 || *opA==3) //memoria o indirecto
      *a = *a & 0x00000fff;

    *opB = (num>>24) & 0x00000003;
    *b = num & 0x00000fff;
    if(*opB==1){//registro
     if((*b&0x0000000f)>=0x0 && (*b&0x0000000f)<=0x9){ //registros de 0 a 9
        *b = *b & 0x0000000f;
      }else if((*b&0x0000000f)>=0xA && (*b&0x0000000f)<=0xF){ //registros de EAX a EFX
        *b = *b & 0x0000003f;
      }
    }else if(*opB==2 || *opB==3)
      *b = *b & 0x00000fff;
    //propago signo si son inmediatos
    if(*opA==0){
     *a = *a<<20;
     *a = *a>>20;
    }
    if(*opB==0){
     *b = *b<<20;
     *b = *b>>20;
    }

  }else if(*mnemo == 0x0000000f){ //1 operando o ninguno
     *mnemo = (*mnemo<<4) & 0x000000f0;
     *mnemo = *mnemo | (0x0000000f & (num >> 24));
     *mnemo = *mnemo & 0x000000ff;
     if((*mnemo & 0x0000000f) <= 0xe){ //1 operando
      *opA = (num >> 22) & 0x00000003;
      *a = num & 0x0000ffff;
      if(*opA==1){//registro
       if((*a&0x0000000f)>=0x0 && (*a&0x0000000f)<=0x9){ //registros de 0 a 9
        *a = *a & 0x0000000f;
       }else if((*a&0x0000000f)>=0xA && (*a&0x0000000f)<=0xF){ //registros de EAX a EFX
        *a = *a & 0x0000003f;
       }
      }else if(*opA==2 || *opA==3) //memoria o indirecto
        *a = num & 0x0000ffff;
      //propago signo si es inmediato
      if(*a==0){
        *a = *a<<16;
        *a = *a>>16;
      }

     }else if(*mnemo==0x000000ff){ //no operandos
      *mnemo = (num>>20) & 0x00000fff; //DASDJASODHAS
      //no seteo operandos FIJARME
     }
  }

}

void configurarRegistros(){
  for(int i=0xA;i<=0xF;i++)
    registros[i] = 0x00000000;
  registros[HP] = 0x00020000;
  registros[IP] = 0x00030000;//instruction pointer arranca en 0
  ldl(0,(registros[SS] & 0xffff0000)>>16);
  ldh(0,1);
  registros[SP] = registros[AC];
  //ldl(0,registros[SS] & 0x0000ffff);
  ldl(0,(registros[SS] & 0xffff0000)>>16);
  ldh(0,1);
  registros[BP] = registros[AC];
  registros[CC] = 0x00000000;
  registros[AC] = 0x00000000;
  srand (time(NULL)); //para rnd()
}


void guardarVDD(){
    cantVDD = 0;
    FILE *arch;
    while(cantArg>cantVDD+2 && strlen(argGlobales[cantVDD+2])>3){
        arch = fopen(argGlobales[cantVDD+2],"rb");
        if(arch!=NULL){
            vdd[cantVDD].estado = 0x00;
            strcpy(vdd[cantVDD].archivo,argGlobales[cantVDD+2]);
            fseek(arch,33,SEEK_SET);
            fread(&(vdd[cantVDD].cantCilindros),1,1,arch);
            fread(&(vdd[cantVDD].cantCabezas),1,1,arch);
            fread(&(vdd[cantVDD].cantSectores),1,1,arch);
            fread(&(vdd[cantVDD].tamSector),4,1,arch);
        }else
            crearVDD(cantVDD,argGlobales[cantVDD+2]);
        cantVDD++;
        fclose(arch);
    }
}

void crearVDD(int i,char *nombre){
    int *dato = NULL;
    FILE *arch;
    strcpy(vdd[i].archivo,nombre);
    vdd[i].cantCilindros = 128;
    vdd[i].cantCabezas = 128;
    vdd[i].cantSectores = 128;
    vdd[i].tamSector = 512;
    vdd[i].estado = 0x00;
    arch = fopen(nombre,"wb");
    if(arch!=NULL){
        dato = (int*) malloc (4);
        *dato = 0x56444430;
        fwrite(dato,4,1,arch);
        free(dato);
        dato = (int*) malloc (4);
        *dato = 1;
        fwrite(dato,4,1,arch);
        free(dato);
        dato = (int*) malloc (16);
        *dato = rand()%0xffffffffffffffff;
        fwrite(dato,16,1,arch);
        free(dato);
        dato = (int*) malloc (4);
        *dato = 0x07E6051E;
        fwrite(dato,4,1,arch);
        free(dato);
        dato = (int*) malloc (4);
        *dato = 0;
        fwrite(dato,4,1,arch);
        free(dato);
        dato = (int*) malloc (1);
        *dato = 1;
        fwrite(dato,1,1,arch);
        free(dato);
        dato = (int*) malloc (1);
        *dato = 128;
        fwrite(dato,1,1,arch);
        free(dato);
        dato = (int*) malloc (1);
        *dato = 128;
        fwrite(dato,1,1,arch);
        free(dato);
        dato = (int*) malloc (1);
        *dato = 128;
        fwrite(dato,1,1,arch);
        free(dato);
        dato = (int*) malloc (4);
        *dato = 512;
        fwrite(dato,4,1,arch);
        free(dato);
        dato = (int*) malloc (211);
        *dato = 0;
        fwrite(dato,221,1,arch);
        free(dato);
    }
    fclose(arch);
}
