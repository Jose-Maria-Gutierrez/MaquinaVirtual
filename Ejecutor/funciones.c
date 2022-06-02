#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tipos.h"

//2 operandos

void setCC(int a){ //setea registro CC
  int reg;
  if(a == 0)
    reg = 0x00000001;
  else
    reg = 0x00000000;
  if(a < 0)
    reg = reg | 0x80000000;
  registros[CC] = reg;
}

int valor(int tipo,int operador){ //devuelve el valor para operar
  int valorRetorno,aux,seg,dsp,tam,registro,offset,valorRegistro;
  if(tipo==1){ //registro
    if((operador&0x00f)>=0 && (operador&0x00f)<=0xF){
      aux = operador & 0xf; //accedo registro
      valorRetorno = registros[aux];
      aux = (operador >> 4) & 0x3; //sector de registro

      switch(aux){
      case 0: valorRetorno = valorRetorno & E;
              break;
      case 1: valorRetorno = valorRetorno & L;
              valorRetorno = valorRetorno<<24; //propago signo
              valorRetorno = valorRetorno>>24;
              break;
      case 2: valorRetorno = (valorRetorno & H)>>8;
              valorRetorno = valorRetorno<<24; //propago signo
              valorRetorno = valorRetorno>>24;
              break;
      case 3: valorRetorno = valorRetorno & X;
              valorRetorno = valorRetorno<<16; //propago signo
              valorRetorno = valorRetorno>>16;
              break;
    }

    }else if((operador&0x00f)>=0x0 && (operador&0x00f)<=0x9){
      if((operador&0x00f)>=5 && (operador&0x00f)<=7){
        valorRetorno = registros[operador] & 0x0000ffff;
      }else
        valorRetorno = registros[operador];
    }

  }else if(tipo==2){ //memoria
   int tamDs = (registros[DS] & 0xffff0000)>>16;
   if(operador<tamDs)
    valorRetorno = memoria[(registros[DS] & X) + operador];
   else{
    //printf("valor");
    segmentationFault();
    valorRetorno = -1;
   }
  }else if(tipo==0)//inmediato
    valorRetorno = operador;
  else if(tipo==3){ //indirecto
    registro = operador & 0x0000000f; //cual registro

    offset = (((operador & 0x000000ff0) >> 4) << 24) >> 24; //propago signo del offset

    valorRegistro = valor(1,registro); //EXTENDED SIEMPRE ESTE REGISTRO

    dsp = ((valorRegistro & 0x0000ffff) << 16) >> 16; //desplazamiento dentro segmento //propago signo
    seg = (valorRegistro & 0xffff0000)>>16; //code del segmento
    if(seg>=DS && seg<=HP){
      switch(seg){//DS SS ES CS HP
        case DS:
          aux = registros[DS] & (0x0000ffff); //donde arranca el segmento
          tam = ( registros[DS] & (0xffff0000) ) >> 16; //tamano del segmento
          break;
        case SS:
          aux = registros[SS] & (0x0000ffff);
          tam = ( registros[SS] & (0xffff0000) ) >> 16; //tamano del segmento
          break;
        case ES:
          aux = registros[ES] & (0x0000ffff);
          tam = ( registros[ES] & (0xffff0000) ) >> 16; //tamano del segmento
          break;
        case CS:
          aux = registros[CS] & (0x0000ffff);
          tam = ( registros[CS] & (0xffff0000) ) >> 16; //tamano del segmento
          break;
        case HP:
          aux = registros[HP] & (0x0000ffff);
          tam = ( registros[HP] & (0xffff0000) ) >> 16; //tamano del segmento
          break;
        }
        if(((dsp + offset)>=0) && ((dsp + offset) <= tam)) //no exceda del segmento y no sea negativo
          valorRetorno = memoria[dsp+offset + aux]; //retorna el valor de esa direccion absolutaaaa
        else{
          segmentationFault();
          valorRetorno = -1;
        }
      }else{ //no existe el code de segmento
        segmentationFault();
        valorRetorno = -1;
      }
  }
  return valorRetorno;
}

void modificarRegistro(int a,int valor){
  int esp,aux,reg;
  esp = a & 0x0f;  //accedo registro
  if(esp>=0xA && esp<=0xF){
    aux = (a>>4) & 0x3;  //sector de registro
    switch(aux){
      case 0: registros[esp] = registros[esp] & (~E);
              reg = valor & E;
              break;
      case 1: registros[esp] = registros[esp] & (~L);
              reg = valor & L;
              break;
      case 2: registros[esp] = registros[esp] & (~H);
              reg = (valor & L)<<8;
              break;
      case 3: registros[esp] = registros[esp] & (~X);
              reg = valor & X;
              break;
    }
    registros[esp] = registros[esp] | reg;
  }else if(esp>=0x0 && esp<=0x9){
    if(esp>=5 && esp<=7){
      valor = valor & 0x0000ffff;
      registros[esp] = (registros[esp] & 0xffff0000) | valor;
    }else
      registros[esp] = valor;
  }
}

void modificarMemoria(int a,int elemento){ //pone elemento en directo a
  int tam,empieza; //tamanio y donde empieza el segmeneto DS
  tam = (registros[DS] & 0xffff0000)>>16;
  empieza = registros[DS] & 0x0000ffff;
  if(a<tam)
    memoria[empieza + a] = elemento;
  else{
     //printf("modificar memoria");
     segmentationFault();
  }
}

void indirectoDireccion(int a,int *direccion){ //recibis indirecto y te devuelve la direccion de memoria
  int contRegistro,reg,segmento,desp,offset,aux,tam;
  reg = a & 0x0000000f;
  offset = (((a & 0x000000ff0) >> 4) << 24) >> 24; //propago signo del offset
  contRegistro = valor(1,reg);
  desp = ((contRegistro & 0x0000ffff) << 16) >> 16; //desplazamiento dentro segmento //propago signo
  segmento = (contRegistro & 0xffff0000) >> 16;

  if(segmento>=DS && segmento<=BP){ //segmentos permitidos
      switch(segmento){//DS SS ES CS HP IP SP BP
        case DS:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        case SS:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        case ES:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        case CS:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        //
        case HP:
          tamComienzoReg(2,&aux,&tam);
          break;
        case IP:
          tamComienzoReg(3,&aux,&tam);
          break;
        case SP:
          tamComienzoReg(1,&aux,&tam);
          break;
        case BP:
          tamComienzoReg(1,&aux,&tam);
          break;
        }
        if(((desp + offset)>=0) && ((desp + offset) <= tam)) //no exceda del segmento y no sea negativo
          *direccion = desp+offset + aux; //direccion absolutaaaa
        else //excede el segmento
          segmentationFault();
      }else //no existe el code de segmento
        segmentationFault();
  //
}

void tamComienzoReg(int registro,int *aux,int *tam){ //segmento y te devuelve donde empieza y el tamaño
  switch(registro){
     case DS:
       *aux = registros[DS] & (0x0000ffff); //donde arranca el segmento
       *tam = ( registros[DS] & (0xffff0000) ) >> 16; //tamano del segmento
       break;
     case SS:
        *aux = registros[SS] & (0x0000ffff);
        *tam = ( registros[SS] & (0xffff0000) ) >> 16; //tamano del segmento
        break;
     case ES:
        *aux = registros[ES] & (0x0000ffff);
        *tam = ( registros[ES] & (0xffff0000) ) >> 16; //tamano del segmento
        break;
     case CS:
        *aux = registros[CS] & (0x0000ffff);
        *tam = ( registros[CS] & (0xffff0000) ) >> 16; //tamano del segmento
        break;
  }
}

void modificarIndirecto(int a,int b){ //a es el indirecto y b ya esta su valor
   int contRegistro,reg,segmento,desp,offset,aux,tam;
  reg = a & 0x0000000f;
  offset = (((a & 0x000000ff0) >> 4) << 24) >> 24; //propago signo del offset
  contRegistro = valor(1,reg);
  desp = ((contRegistro & 0x0000ffff) << 16) >> 16; //desplazamiento dentro segmento //propago signo
  segmento = (contRegistro & 0xffff0000) >> 16;

  if(segmento>=DS && segmento<=BP){ //segmentos permitidos
      switch(segmento){//DS SS ES CS HP IP SP BP
        case DS:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        case SS:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        case ES:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        case CS:
          tamComienzoReg(segmento,&aux,&tam);
          break;
        //
        case HP:
          tamComienzoReg(2,&aux,&tam);
          break;
        case IP:
          tamComienzoReg(3,&aux,&tam);
          break;
        case SP:
          tamComienzoReg(1,&aux,&tam);
          break;
        case BP:
          tamComienzoReg(1,&aux,&tam);
          break;
        }
        if(((desp + offset)>=0) && ((desp + offset) <= tam)) //no exceda del segmento y no sea negativo
          memoria[desp+offset + aux] = b; //pone b en esa direccion absolutaaaa
        else //excede el segmento
          segmentationFault();
      }else //no existe el code de segmento
        segmentationFault();
}

void mov(int opA,int a,int opB,int b){
  int valorB;
  if(opA!=0){   //si es valido
   valorB = valor(opB,b);
   if(opA==1)   //registro
     modificarRegistro(a,valorB);
   else if(opA==2)   //memoria
     modificarMemoria(a,valorB);
   else if(opA==3){  //operando indirecto
     modificarIndirecto(a,valorB);
   }
  }
}

void add(int opA,int a,int opB,int b){
  int suma,valorA,valorB;
  if(opA!=0){//si es valido
    valorA = valor(opA,a);
    valorB = valor(opB,b);
    suma = valorA + valorB;
    if(opA==1)  //registro
      modificarRegistro(a,suma);
    else if(opA==2)  //memoria
      modificarMemoria(a,suma);
    else if(opA==3){ //indirecto
      modificarIndirecto(a,suma);
    }

    setCC(suma);
  }
}

void sub(int opA,int a,int opB,int b){
  int resta;
  if(opA!=0){//si es valido
    resta = valor(opA,a) - valor(opB,b);
    if(opA==1)  //registro
      modificarRegistro(a,resta);
    else if(opA==2) //memoria
      modificarMemoria(a,resta);
    else if(opA==3)
      modificarIndirecto(a,resta);
    setCC(resta);
  }
}

void mul(int opA,int a,int opB,int b){
  int multiplicacion;
  if(opA!=0){//si es valido
    multiplicacion = valor(opA,a) * valor(opB,b);
    if(opA==1)  //registro
      modificarRegistro(a,multiplicacion);
    else if(opA==2) //memoria
      modificarMemoria(a,multiplicacion);
    else if(opA==3)
      modificarIndirecto(a,multiplicacion);
    setCC(multiplicacion);
  }
}

void division(int opA,int a,int opB,int b){
  if(opA!=0){//si es valido
    div_t resultado = div(valor(opA,a),valor(opB,b));
    if(opA==1) //registro
      modificarRegistro(a,resultado.quot); //resultado.quot cociente entero
    else if(opA==2)  //memoria
      modificarMemoria(a,resultado.quot);
    else if(opA==3)
      modificarIndirecto(a,resultado.quot); //FIJARME
    modificarRegistro(AC,abs(resultado.rem));  //resto entero positivo
    setCC(resultado.quot);
  }
}

void swap(int opA,int a,int opB,int b){
  int valorA,valorB;
  if(opA!=0 && opB!=0){//si es valido
    valorA = valor(opA,a);
    valorB = valor(opB,b);

    if(opA==1)   //registro
      modificarRegistro(a,valorB);
    else if(opA==2)  //memoria
      modificarMemoria(a,valorB);
    else if(opA==3)
      modificarIndirecto(a,valorB);

    if(opB==1){   //registro
      modificarRegistro(b,valorA);
    }else if(opB==2)  //memoria
      modificarMemoria(b,valorA);
    else if(opB==3)
      modificarIndirecto(b,valorA);
  }
}

void cmp(int opA,int a,int opB,int b){ //supongo que a puede ser inmediato
  int aux = valor(opA,a) - valor(opB,b);
  setCC(aux);
}

void and(int opA,int a,int opB,int b){
  int andBit;
  if(opA!=0){//si es valido
    andBit = valor(opA,a) & valor(opB,b);
    if(opA==1) //registro
      modificarRegistro(a,andBit);
    else if(opA==2)      //memoria
      modificarMemoria(a,andBit);
    else if(opA==3)
      modificarIndirecto(a,andBit);

  setCC(andBit);
  }
}

void or(int opA,int a,int opB,int b){
  int orBit;
  if(opA!=0){//si es valido
    orBit = valor(opA,a) | valor(opB,b);
    if(opA==1) //registro
      modificarRegistro(a,orBit);
    else if(opA==2)      //memoria
      modificarMemoria(a,orBit);
    else if(opA==3)
      modificarIndirecto(a,orBit);

  setCC(orBit);
  }
}

void xor(int opA,int a,int opB,int b){
  int xorBit;
  if(opA!=0){//si es valido
    xorBit = valor(opA,a) ^ valor(opB,b);
    if(opA==1) //registro
      modificarRegistro(a,xorBit);
    else if(opA==2)       //memoria
      modificarMemoria(a,xorBit);
    else if(opA==3)
      modificarIndirecto(a,xorBit);

  setCC(xorBit);
  }
}

void shl(int opA,int a,int opB,int b){
  int corrimiento,valorA,valorB;
  if(opA!=0){//si es valido
    valorA = valor(opA,a);
    valorB = valor(opB,b);
    corrimiento =  valorA << valorB;
    if(opA==1)  //registro
      modificarRegistro(a,corrimiento);
    else if(opA==2)        //memoria
      modificarMemoria(a,corrimiento);
    else if(opA==3)
      modificarIndirecto(a,corrimiento);

  setCC(corrimiento);
  }
}

void shr(int opA,int a,int opB,int b){
  int corrimiento;
  if(opA!=0){//si es valido
    corrimiento = valor(opA,a) >> valor(opB,b);
    if(opA==1) //registro
      modificarRegistro(a,corrimiento);
    else if(opA==2)    //memoria
      modificarMemoria(a,corrimiento);
    else if(opA==3)
      modificarIndirecto(a,corrimiento);

  setCC(corrimiento);
  }
}

void slen(int opA,int a,int opB,int b){
  int i,letra,dir;
  if(opA!=0 && opB!=0 && opB!=1){ //valido
    i = 0;
    letra = valor(opB,b) & 0x000000FF; //ultimo byte
    if(opB==2) //directo
      dir = (registros[DS] & 0x0000ffff) + b;
    else //indirecto
      indirectoDireccion(b,&dir);
    while(letra!=0x00000000){
      i++;
      dir++;
      letra = memoria[dir];
    }
    if(opA==1) //registro
      modificarRegistro(a,i);
    else if(opA==2) //memoria
      modificarMemoria(a,i);
    else        //indirecto
      modificarIndirecto(a,i);
  }
}

void smov(int opA,int a,int opB,int b){
  int car,o,d;
  if(opA!=0 && opA!=1 && opB!=0 && opB!=1){
    if(opA==2)
      d = (registros[DS] & 0x0000ffff) + a;
    else
      indirectoDireccion(a,&d);
    if(opB==2)//directo
      o = (registros[DS] & 0x0000ffff) + b;
    else //indirecto
      indirectoDireccion(b,&o);

    car = memoria[o] & 0x000000FF; //ultimo byte
    while(car!=0x00000000){
      memoria[d] = car;
      d++;
      o++;
      car = memoria[o] & 0x000000ff;
    }
    memoria[d] = car;
  }
}

void scmp(int opA,int a,int opB,int b){
  int pri,seg,resta,i,valorPri,valorSeg;
  if(opA!=0 && opA!=1 && opB!=0 && opB!=1){ //valido
    if(opA==2)
      pri = (registros[DS] & 0x0000ffff) + a;
    else
      indirectoDireccion(a,&pri);
    if(opB==2)
      seg = (registros[DS] & 0x0000ffff) + b;
    else
      indirectoDireccion(b,&seg);
    //printf("pri: %d segu: %d\n",pri,seg);
    valorPri = memoria[pri] & 0x000000ff;
    valorSeg = memoria[seg] & 0x000000ff;
    resta = valorPri - valorSeg;
    //printf("pri: %d segu: %d\n",valorPri,valorSeg);
    setCC(resta);
    while((valorPri!=0x00000000) && (valorSeg!=0x00000000) && ((registros[CC]&0x00000001) == 0x00000001)){
      resta = valorPri - valorSeg;
      //printf("resta: %d\n",resta);
      setCC(resta);
      pri++; seg++;
      valorPri = memoria[pri] & 0x000000FF;
      valorSeg = memoria[seg] & 0x000000FF;
      //printf("pri: %d segu: %d\n",valorPri,valorSeg);
    }
    if((valorPri==0 && valorSeg!=0) || (valorPri!=0 && valorSeg==0)){
      resta = valorPri - valorSeg;
      setCC(resta);
    }
  }
}

//1 operando

void push(int opA,int a){
  int elemento,opSP,dir;
  elemento = valor(opA,a);
  opSP = registros[SP] & 0x0000ffff; //donde esta el SP dentro de la pila
  if(opSP==0) //pila llena
    stackOverflow();
  else{
    registros[SP] -= 1; //decremento el sp
    indirectoDireccion(0x6,&dir);
    memoria[dir] = elemento;
  }
}

void pop(int opA,int a){
  int elemento,opSP,tam,dir;
  if(opA!=0){ //valido
    opSP = registros[SP] & 0x0000ffff; //donde esta el SP dentro de la pila
    tam = (registros[SS] & 0xffff0000)>>16; //tamanio del Stack Segment
    if(opSP >= tam) //pila vacia
      stackUnderflow();
    else{
      indirectoDireccion(0x6,&dir);
      elemento = memoria[dir];
      registros[SP] += 1; //incremento el SP
      if(opA==1) //registro
        modificarRegistro(a,elemento);
      else if(opA==2) //memoria
        modificarMemoria(a,elemento);
      else if(opA==3)
        modificarIndirecto(a,elemento);
    }
  }
}

void call(int opA,int a){ //direccion de memoria a donde saltar
  int opSP,dir;
  opSP = registros[SP] & 0x0000ffff; //donde esta el SP dentro de la pila
  if(opSP==0) //pila llena
    stackOverflow();
  else{
    push(1,IP);
    /*registros[SP] -= 1; //decremento el sp
    indirectoDireccion(0x6,&dir);
    memoria[dir] = (registros[IP] & 0x0000ffff); //guardo en la pila el IP*/
    jmp(opA,a); //salto al valor pasado por parametro
  }
}

void ret(){
  int dir,opSP,tam,elemento;
  opSP = registros[SP] & 0x0000ffff; //donde esta el SP dentro de la pila
  tam = (registros[SS] & 0xffff0000)>>16;
  if(opSP >= tam) //pila vacia
    stackUnderflow();
  else{
    indirectoDireccion(0x6,&dir);
    elemento = memoria[dir];
    registros[SP] += 1; //incremento el SP
    jmp(0,elemento);
  }
}

void read(){
  char car,lectura[MAXSTRING];
  int i,ingreso,conf,base,dir;
  //int alm = (registros[0xD] & E); //almaceno desde posicion en memoria que esta en registro EDX
  indirectoDireccion(0xD,&dir);
  conf = registros[0xA] & X;
  if(((conf>>8) & 0x00000001 ) == 0){ //interpreta dsp del enter
    for(i=0;i<(registros[0xC]&X);i++){
      if(((conf>>11) & 0x00000001) == 0) //escribe el prompt
        printf("[%04d]: ",dir); //dir = posicion de memoria en decimal
      scanf("%d",&ingreso); //leo
      printf("\n");
      if(((conf>>3) & 0x0001) == 1) //hexa
        base = 16;
      else if(((conf>>2) & 0x0001) == 1) //octal
        base = 8;
      else if((conf & 0x0001 ) == 1) //decimal
        base = 10;
      itoa(ingreso,lectura,base); //de decimal a string en base x
      ingreso = atoi(lectura); //de string a numero
      //memoria[(registros[DS] & 0x0000ffff) + alm++] = ingreso;
      memoria[dir++] = ingreso;
    }
  }else{ //lee caracter a caracter dsp del enter
    i = 0;
    if(((conf>>11) & 0x0001) == 0) //escribe el prompt
      printf("[%04d]: ",dir); //alm a binario
    scanf("%s",lectura);
    car = lectura[i]; //caracter
    while(car!=' ' && i<(registros[0xC]&X)){
      memoria[dir++] = car;
      car = lectura[++i];
    }
    memoria[dir] = 0x00000000;
  }
}

void write(){
  char porcentaje;
  int escribe,prompt,endline,i,caracter,conf,dir;
  //int alm = registros[0xD] & E; //leo desde posicion en memoria que esta en registro EDX
  indirectoDireccion(0xD,&dir);
  porcentaje = '%';
  conf = registros[0xA] & X;
  endline = !((conf>>8) &0x00000001); //0 agregar endline
  prompt = !((conf>>11) & 0x00000001);//agregar prompt
  for(i=0;i<(registros[0xC]&X);i++){
   if(prompt) printf("[%04d]: ",dir);
   escribe = memoria[dir];
   if(((conf>>4) & 0x00000001) == 1){ //caracter
    caracter = escribe & 0x000000ff; //byte menos significativo
    if(caracter<32 || caracter>126)
      caracter = '.';               //ascii no imprimible
    printf("%c ",caracter);
   }
   if(((conf>>3) & 0x00000001) == 1){ //hexadecimal
    printf("%c%0X ",porcentaje,escribe);
   }
   if(((conf>>2) & 0x00000001 )== 1){ //octal
    printf("@%o ",escribe);
   }
   if((conf & 0x00000001) == 1){//decimal
    printf("#%d ",escribe);
   }
   if(endline) printf("\n");
   dir++;
  }
}

void stringRead(){
  char car,lectura[MAXSTRING];
  int i,conf,dir,prompt;
  indirectoDireccion(0xD,&dir);
  //int alm = (registros[0xD] & E); //almaceno desde posicion en memoria que esta en registro EDX
  conf = registros[0xA] & X;
  prompt = !((conf>>11) & 0x00000001);//agregar prompt
  if(prompt) //escribe el prompt
    printf("[%04d]: ",dir); //dir donde arranca en binario
  gets(lectura);
  i = 0;
  car = lectura[i]; //caracter
  while(car!='\0' && i<(registros[0xC] & X)-1){
    //printf("%c\n",car);
    memoria[dir++] = car;
    car = lectura[++i];
  }
  memoria[dir] = 0x00000000;
}

void stringWrite(){
  int prompt,endline,conf,dir,car;
  indirectoDireccion(0xD,&dir);
  conf = registros[0xA] & X;
  prompt = !((conf>>11) & 0x00000001);//agregar prompt
  endline = !((conf>>8) &0x00000001); //0 agregar endline

  if(prompt) printf("[%04d]: ",dir);
  car = memoria[dir] & 0x000000FF;
  while(car!=0x00000000){
    printf("%c",car);
    car = memoria[++dir] & 0x000000FF;
  }
  if(endline) printf("\n");
}

void breakpoint(){
  int h,f,dir;
  int operando1,operando2;
  char opA[MAXSTRING],opB[MAXSTRING],input[MAXSTRING];
  FILE* archivo;
  for(f=2;f<cantArg;f++){     //si esta la flag [-c] la ejecuto
    if(strcmp(argGlobales[f],"-c") == 0)
      system("cls");
  }
  for(f=2;f<cantArg;f++){
    if(strcmp(argGlobales[f],"-d") == 0){ //disassembler
      ejecutarDisassembler();
    }
  }
  for(f=2;f<cantArg;f++){
    if(strcmp(argGlobales[f],"-b") == 0){
      archivo = fopen("input.txt","wt"); //abre para escritura
      fflush(stdin);
      //indirectoDireccion(instruccionActual,&dir);
      dir = instruccionActual & 0x0000ffff;
      printf("[%04d] cmd: ",dir);
      fgets(input,100,stdin);  //entrada desde teclado
      fputs(input,archivo);    //escribe en el archivo la entrada
      fclose(archivo);
      archivo = fopen("input.txt","r"); //abre para lectura

      if(fscanf(archivo,"%s %s", opA,opB)==2){ //si son 2 operandos
        operando1 = atoi(opA);
        operando2 = atoi(opB);
        //printf("a %d b %d",operando1,operando2);
        if(!((opA[0]!='0' && operando1 == 0) || (opB[0]!='0' && operando2 == 0))){ // son numeros
          //printf("operando1: %d operando2: %d\n",operando1,operando2);
          breakpointActivado = 0;
          for(h=operando1;h<=operando2;h++)
            printf("[%04d]: %04X %04X %d\n",h,(memoria[h]&0xffff0000)>>16,memoria[h]&0x0000ffff,memoria[h]);
        }
      }else{      //1 operando
        //printf("operandoA: %s\n",opA);
        if(strcmp(opA,"r")==0){
          breakpointActivado = 0; //desactiva breakpoint la MV continua la ejecucion
          printf("\n");
        }else if(strcmp(opA,"p")==0){
          printf("\n");
          breakpointActivado = 1; //activa breakpoint dsp de cada instruccion
        }else{
          operando1 = atoi(opA);
          if(!(opA[0]!='0' && operando1==0)){ //si es numero
            //printf("NUMERO");
            breakpointActivado= 0;
            if(operando1>=0 && operando1<=4096)
              printf("[%04d]: %04X %04X %d\n",operando1,((memoria[operando1])&0xffff0000)>>16,
                     (memoria[operando1])&0x0000ffff,memoria[operando1]);
          }
        }
      }
    }
    //system("pause");
  }
}

void sys(int opA,int a){
  if(a==0x00000001)//lectura
    read();
  else if(a==0x00000002) //escritura
    write();
  else if(a==0x0000000f)// breakpoint
    breakpoint();
  else if(a==0x00000003)
    stringRead();
  else if(a==0x00000004)
    stringWrite();
  else if(a==0x00000007)
    system("cls");
  else if(a==0x0000000D)
    accesoVDD();
}

void jmp(int opA,int a){
  int salto = valor(opA,a) & 0x0000ffff;
  registros[IP] = (registros[IP] & 0xffff0000) | salto;
}

void jp(int opA,int a){
  int salto = valor(opA,a) & 0x0000ffff;
  if(registros[CC] == 0x00000000)
    registros[IP] = (registros[IP] & 0xffff0000) | salto;
}

void jumpn(int opA,int a){
  int salto = valor(opA,a) & 0x0000ffff;
  if(registros[CC] == 0x80000000)
    registros[IP] = (registros[IP] & 0xffff0000) | salto;
}

void jz(int opA,int a){
  int salto = valor(opA,a) & 0x0000ffff;
  if(registros[CC] == 0x00000001)
    registros[IP] = (registros[IP] & 0xffff0000) | salto;
}

void jnz(int opA,int a){ //jump not zero
  int salto = valor(opA,a) & 0x0000ffff;
  if((registros[CC] & 0x00000001) == 0x0)
    registros[IP] = (registros[IP] & 0xffff0000) | salto;
}

void jnp(int opA,int a){ //jump no positive
  int salto = valor(opA,a) & 0x0000ffff;
  if(registros[CC] == 0x80000000 || registros[CC] == 0x00000001) //negativo o 0
    registros[IP] = (registros[IP] & 0xffff0000) | salto;
}

void jnn(int opA,int a){ //jump no negative
  int salto = valor(opA,a) & 0x0000ffff;
  if(registros[CC] == 0x00000000 || registros[CC] == 0x00000001) //positivo o 0
    registros[IP] = (registros[IP] & 0xffff0000) | salto;
}

void ldh(int opA,int a){
  int operando,aux;
  operando = valor(opA,a) & 0x0000ffff; //los 2 bytes menos significativos del operando
  aux = registros[AC] & 0x0000ffff; //hace hueco entre los 2 bytes mas significativo de AC
  registros[AC] = (operando << 16) | aux;
}

void ldl(int opA,int a){
  int operando,aux;
  operando = valor(opA,a) & 0x0000ffff; //los 2 bytes menos significativos del operando
  aux = registros[AC] & 0xffff0000;     //los 2 bytes mas significativos de AC
  registros[AC] = operando | aux;
}

void rnd(int opA,int a){
  int random = rand() % valor(opA,a); //de 0 hasta a
  registros[AC] = random;
}

void not(int opA,int a){
  int negacion;
  if(opA!=0){
    negacion = ~valor(opA,a);
    if(opA==1) //registros
      modificarRegistro(a,negacion);
    else if(opA==2) //memoria
      modificarMemoria(a,negacion);
    else if(opA==3)
      modificarIndirecto(a,negacion);

  setCC(negacion);
  }
}

//0 operandos

void stop(){ //detiene programa
  ldh(0,3);
  ldl(0,cantInstrucciones);
  registros[IP] = registros[AC];
}



void segmentationFault(){
  printf("SEGMENTATION FAULT");
  stop();
}

void stackOverflow(){
  printf("STACK OVERFLOW");
  stop();
}

void stackUnderflow(){
  printf("STACK UNDERFLOW");
  stop();
}

void accesoVDD(){
    int AH = (registros[0xA] & 0x0000ff00) >> 8;
    int DL = (registros[0xD] & 0x000000ff);
    if (cantVDD > DL){
        if(AH == 0x00) // consulta ultimo estado
            printf("\nAH: %02X", vdd[DL].estado);
        else if (AH == 0x02) // leer del disco
            leerVDD();
        else if (AH == 0x03) // escribir en el disco
            escribirVDD();
        else if (AH == 0x08) // obtener los parametros del disco
            obtenerParametrosVDD();
        else // funcion invalida
        {
            registros[0xA] &= 0xffff00ff;
            registros[0xA] += 0x00000100;
            vdd[DL].estado = (registros[0xA] & 0x0000ff00) >> 8;
        }
    }else{ // no existe el disco
        registros[0xA] &= 0xffff00ff;
        registros[0xA] += 0x00003100;
    }
}

void obtenerParametrosVDD(){
    int DL = registros[0xD] & 0x000000ff;
    registros[0xC] &= 0xffff00ff;
    registros[0xC] += (vdd[DL].cantCilindros << 8);
    registros[0xC] &= 0xffffff00;
    registros[0xC] += (vdd[DL].cantCabezas);
    registros[0xD] &= 0xffff00ff;
    registros[0xD] += (vdd[DL].cantSectores << 8);
    registros[0xA] &= 0xffff00ff; // operacion exitosa
    vdd[DL].estado = (registros[0xA] & 0x0000ff00) >> 8;
}

void leerVDD(){
    int direccion, DL = registros[0xD] & 0x000000ff, CH , CL, DH, AL, celdas, dato[5000], tamDisco;
    FILE *disco;
    CH = (registros[0xC] & 0x0000ff00) >> 8; // numero de cilindro
    CL = registros[0xC] & 0x000000ff;        // numero de cabeza
    DH = (registros[0xD] & 0x0000ff00) >> 8; // numero de sector
    AL = registros[0xA] & 0x000000ff;        // cantidad de sectores
    if (CH > vdd[DL].cantCilindros){
        registros[0xA] &= 0xffff00ff;
        registros[0xA] += 0x00000B00; // numero invalido de cilindro
    }else if (CL > vdd[DL].cantCabezas){
        registros[0xA] &= 0xffff00ff;
        registros[0xA] += 0x00000C00; // numero invalido de cabeza
    }else if (DH > vdd[DL].cantSectores){
        registros[0xA] &= 0xffff00ff;
        registros[0xA] += 0x00000D00; // numero invalido de sector
    }else{
        direccion = 512 +  CH * vdd[DL].cantCilindros * vdd[DL].cantSectores * vdd[DL].tamSector + CL * vdd[DL].cantSectores * vdd[DL].tamSector + DH * vdd[DL].tamSector;
        disco = fopen(vdd[DL].archivo,"rb");
        if (disco != NULL){
            fseek(disco, direccion , SEEK_SET);
            celdas = 512 * AL / 4;
            tamDisco = 512 + vdd[DL].cantCilindros * vdd[DL].cantCilindros * vdd[DL].cantSectores * vdd[DL].tamSector + vdd[DL].cantCabezas * vdd[DL].cantSectores * vdd[DL].tamSector + vdd[DL].cantSectores * vdd[DL].tamSector;
            while(tamDisco < direccion+celdas*4){
                celdas -= 128;
                registros[0xA]--;
            }
            if ((celdas + (registros[0xB] & 0x0000ffff)) <= ((registros[(registros[0xB] & 0xffff0000) >> 16] & (0xffff0000)) >> 16)){
                fread(&dato,4,celdas,disco);
                for(int i = 0 ; i<celdas ; i++ , registros[0xB]++)
                    modificarIndirecto(0xB,dato[i]);
                registros[0xA] &= 0xffff00ff; // operacion exitosa
            }else{
                registros[0xA] &= 0xffff00ff;
                registros[0xA] += 0x00000400; // error en la lectura (no hay espacio suficiente)
            }
        }else{
            registros[0xA] &= 0xffff00ff;
            registros[0xA] += 0x0000ff00; // error de operacion
        }
        fclose(disco);
    }
    vdd[DL].estado = (registros[0xA] & 0x0000ff00) >> 8;
}

void escribirVDD(){
    int direccion, DL = registros[0xD] & 0x000000ff, CH , CL, DH, AL, celdas, tamDisco, dato[5000];
    FILE *disco;
    CH = (registros[0xC] & 0x0000ff00) >> 8; // numero de cilindro
    CL = registros[0xC] & 0x000000ff;        // numero de cabeza
    DH = (registros[0xD] & 0x0000ff00) >> 8; // numero de sector
    AL = registros[0xA]&0x000000ff;          // cantidad de sectores

    if (CH > vdd[DL].cantCilindros){
        registros[0xA] &= 0xffff00ff;
        registros[0xA] += 0x00000B00; // numero invalido de cilindro
    }else if (CL > vdd[DL].cantCabezas){
        registros[0xA] &= 0xffff00ff;
        registros[0xA] += 0x00000C00; // numero invalido de cabeza
    }else if (DH > vdd[DL].cantSectores){
        registros[0xA] &= 0xffff00ff;
        registros[0xA] += 0x00000D00; // numero invalido de sector
    }else{
        tamDisco = 512 + vdd[DL].cantCilindros * vdd[DL].cantCilindros * vdd[DL].cantSectores * vdd[DL].tamSector + vdd[DL].cantCabezas * vdd[DL].cantSectores * vdd[DL].tamSector + vdd[DL].cantSectores * vdd[DL].tamSector;
        direccion = 512 +  CH * vdd[DL].cantCilindros * vdd[DL].cantSectores * vdd[DL].tamSector + CL * vdd[DL].cantSectores * vdd[DL].tamSector + DH * vdd[DL].tamSector;
        disco = fopen(vdd[DL].archivo,"wb");
        if (disco != NULL){
            fseek(disco, direccion , SEEK_SET);
            celdas = 512 * AL / 4;
            if (tamDisco >= direccion+512*AL){
                if ((celdas + (registros[0xB] & 0x0000ffff)) <= ((registros[(registros[0xB] & 0xffff0000) >> 16] & (0xffff0000)) >> 16)){
                    for(int i = 0 ; i<celdas ; i++ , registros[0xB]++)
                        dato[i] = valor(3,0xB);
                    fwrite(dato,4,celdas,disco);
                    registros[0xA] &= 0xffff00ff; // operacion exitosa
                }else{
                    registros[0xA] &= 0xffff00ff;
                    registros[0xA] += 0x0000CC00; // error en la escritura (no hay espacio suficiente)
                }
            }else{
                registros[0xA] &= 0xffff00ff;
                registros[0xA] += 0x0000ff00; // error de operacion
            }
        }else{
            registros[0xA] &= 0xffff00ff;
            registros[0xA] += 0x0000ff00; // error de operacion
        }
        fclose(disco);
    }
    vdd[DL].estado = (registros[0xA] & 0x0000ff00) >> 8;
}
