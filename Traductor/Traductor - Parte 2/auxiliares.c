#include <string.h>
#include <ctype.h>
#include "auxiliares.h"

int pot(int b, int e)
{
    int res = 1, i;
    for (i = 1; i <= e; i++)
    {
        res *= b;
    }
    return res;
}

short int hexToDec(char hex[])
{
    int i;
    int res = 0, cifra = 0;
    for (i = strlen(hex) - 1; i >= 0; i--)
    {
        if (hex[i] >= '0' && hex[i] <= '9')
            res += (hex[i] & 0x0F) * pot(16, cifra);
            //la mascara aplicada convierte el caracter ascii numerico en su representacion numerica
        else
            res += ((hex[i] & 0xDF) - 55) * pot(16, cifra);
            //la mascara aplicada asegura que el caracter alfabetico sea mayuscula
        cifra++;
    }
    return res;
}

short int octToDec(char oct[])
{
    int res = 0, cifra = 0, i;
    for (i = strlen(oct) - 1; i >= 0; i--)
    {
        res += (oct[i] & 0x0F) * pot(8, cifra);
        cifra++;
    }
    return res;
}

short int decToDec(char dec[])
{
    //Puede venir un negativo como parametro de esta funcion
    int res = 0, cifra = 1, i, a;
	a = (dec[0] == '-') ? 1 : 0;
    for (i = strlen(dec) - 1; i >= a; i--)
    {
        res += (dec[i] & 0x0F) * cifra;
        cifra *= 10;
    }
	if (a)
		res *= -1;
    return res;
}

void aMayus(char palabra[])
{
    int i = 0;

    while (palabra[i] != '\0')
    {
        palabra[i] = toupper(palabra[i]);
        i++;
    }
}

