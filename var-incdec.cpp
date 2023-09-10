/* Este arquivo � software livre; voc� pode redistribuir e/ou
 * modificar nos termos das licen�as GPL ou LGPL. Vide arquivos
 * COPYING e COPYING2.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GPL or the LGP licenses.
 * See files COPYING e COPYING2.
 */

#include <stdio.h>
#include <stdlib.h>
#include "variavel.h"
#include "instr.h"
#include "instr-enum.h"
#include "var-incdec.h"
#include "misc.h"

//------------------------------------------------------------------------------
int TVarIncDec::getInc(int numfunc)
{
    if (valor < 0)
        return (numfunc ? -valor : valor);
    unsigned int v = TempoIni + INTTEMPO_MAX * INTTEMPO_MAX - valor;
    return (v >= INTTEMPO_MAX * INTTEMPO_MAX ? INTTEMPO_MAX * INTTEMPO_MAX - 1 : v);
}

//------------------------------------------------------------------------------
int TVarIncDec::getDec(int numfunc)
{
    if (valor < 0)
        return (numfunc ? -valor : valor);
    unsigned int v = valor - TempoIni;
    return (v >= INTTEMPO_MAX * INTTEMPO_MAX ? 0 : v);
}

//------------------------------------------------------------------------------
void TVarIncDec::setInc(int numfunc, int v)
{
// Acerta o sinal se for fun��o abs
    if (numfunc && (getInc(0) < 0) != (v < 0))
        v = -v;
// Contagem parada
    if (v < 0)
    {
        if (v <= -INTTEMPO_MAX * INTTEMPO_MAX)
            valor = -INTTEMPO_MAX * INTTEMPO_MAX + 1;
        else
            valor = v;
        return;
    }
// Contagem andando
    if (v >= INTTEMPO_MAX * INTTEMPO_MAX)
        v = INTTEMPO_MAX * INTTEMPO_MAX - 1;
    valor = TempoIni + INTTEMPO_MAX * INTTEMPO_MAX - v;
}

//------------------------------------------------------------------------------
void TVarIncDec::setDec(int numfunc, int v)
{
// Acerta o sinal se for fun��o abs
    if (numfunc && (getDec(0) < 0) != (v < 0))
        v = -v;
// Contagem parada
    if (v < 0)
    {
        if (v <= -INTTEMPO_MAX * INTTEMPO_MAX)
            valor = -INTTEMPO_MAX * INTTEMPO_MAX + 1;
        else
            valor = v;
        return;
    }
// Contagem andando
    if (v >= INTTEMPO_MAX * INTTEMPO_MAX)
        v = INTTEMPO_MAX * INTTEMPO_MAX - 1;
    valor = TempoIni + v;
}

//------------------------------------------------------------------------------
bool TVarIncDec::FuncInc(TVariavel * v, const char * nome)
{
    if (comparaZ(nome, "abs") == 0)
    {
        Instr::ApagarVar(v + 1);
        Instr::VarAtual->numfunc = 1;
        return true;
    }
    if (comparaZ(nome, "pos") == 0)
    {
        int valor = getInc(0);
        if (valor<0)
            setInc(0, -valor);
        return false;
    }
    if (comparaZ(nome, "neg") == 0)
    {
        int valor = getInc(0);
        if (valor>0)
            setInc(0, -valor);
        return false;
    }
    return false;
}

//------------------------------------------------------------------------------
bool TVarIncDec::FuncDec(TVariavel * v, const char * nome)
{
    if (comparaZ(nome, "abs") == 0)
    {
        Instr::ApagarVar(v + 1);
        Instr::VarAtual->numfunc = 1;
        return true;
    }
    if (comparaZ(nome, "pos") == 0)
    {
        int valor = getDec(0);
        if (valor<0)
            setDec(0, -valor);
        return false;
    }
    if (comparaZ(nome, "neg") == 0)
    {
        int valor = getDec(0);
        if (valor>0)
            setDec(0, -valor);
        return false;
    }
    return false;
}

//------------------------------------------------------------------------------
bool TVarIncDec::FuncVetorInc(TVariavel * v, const char * nome)
{
    if (comparaZ(nome, "limpar") != 0)
        return false;
    const int total = (unsigned char)v->defvar[Instr::endVetor];
    int numero = 0;
    if (Instr::VarAtual > v)
    {
        numero = v[1].getInt();
        if (numero >= INTTEMPO_MAX * INTTEMPO_MAX)
            numero = INTTEMPO_MAX * INTTEMPO_MAX - 1;
    }
    numero = TempoIni + INTTEMPO_MAX * INTTEMPO_MAX - numero;
    for (int x = 0; x < total; x++)
        this[x].valor = numero;
    return false;
}

//------------------------------------------------------------------------------
bool TVarIncDec::FuncVetorDec(TVariavel * v, const char * nome)
{
    if (comparaZ(nome, "limpar") != 0)
        return false;
    const int total = (unsigned char)v->defvar[Instr::endVetor];
    int numero = 0;
    if (Instr::VarAtual > v)
    {
        numero = v[1].getInt();
        if (numero >= INTTEMPO_MAX * INTTEMPO_MAX)
            numero = INTTEMPO_MAX * INTTEMPO_MAX - 1;
    }
    numero = TempoIni + numero;
    for (int x = 0; x < total; x++)
        this[x].valor = numero;
    return false;
}
