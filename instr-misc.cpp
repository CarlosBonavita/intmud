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
#include <string.h>
#include <assert.h>
#include "instr.h"
#include "instr-enum.h"
#include "instr-func.h"
#include "variavel.h"
#include "misc.h"

int Instr::ChecaLinha::ChecaErro = 0;

//------------------------------------------------------------------------------
// Lista de fun��es predefinidas
// Deve obrigatoriamente estar em ordem alfab�tica
const Instr::TListaFunc Instr::ListaFunc[] = {
    { "apagar",       InstrFunc::FuncApagar },
    { "arg0",         InstrFunc::FuncArg0 },
    { "arg1",         InstrFunc::FuncArg1 },
    { "arg2",         InstrFunc::FuncArg2 },
    { "arg3",         InstrFunc::FuncArg3 },
    { "arg4",         InstrFunc::FuncArg4 },
    { "arg5",         InstrFunc::FuncArg5 },
    { "arg6",         InstrFunc::FuncArg6 },
    { "arg7",         InstrFunc::FuncArg7 },
    { "arg8",         InstrFunc::FuncArg8 },
    { "arg9",         InstrFunc::FuncArg9 },
    { "args",         InstrFunc::FuncArgs },
    { "criar",        InstrFunc::FuncCriar },
    { "este",         InstrFunc::FuncEste },
    { "int",          InstrFunc::FuncInt },
    { "intabs",       InstrFunc::FuncIntAbs },
    { "intbit",       InstrFunc::FuncIntBit },
    { "intbith",      InstrFunc::FuncIntBitH },
    { "intchr",       InstrFunc::FuncIntChr },
    { "intdist",      InstrFunc::FuncIntDist },
    { "intdistdif",   InstrFunc::FuncIntDistDif },
    { "intdistmai",   InstrFunc::FuncIntDistMai },
    { "intdiv",       InstrFunc::FuncIntDiv },
    { "intmax",       InstrFunc::FuncIntMax },
    { "intmin",       InstrFunc::FuncIntMin },
    { "intnome",      InstrFunc::FuncIntNome },
    { "intpos",       InstrFunc::FuncIntPos },
    { "intsenha",     InstrFunc::FuncIntSenha },
    { "intsub",       InstrFunc::FuncIntSub },
    { "intsublin",    InstrFunc::FuncIntSubLin },
    { "inttotal",     InstrFunc::FuncTotal },
    { "matacos",      InstrFunc::FuncMatAcos },
    { "matasin",      InstrFunc::FuncMatAsin },
    { "matatan",      InstrFunc::FuncMatAtan },
    { "matbaixo",     InstrFunc::FuncMatBaixo },
    { "matcima",      InstrFunc::FuncMatCima },
    { "matcos",       InstrFunc::FuncMatCos },
    { "matdeg",       InstrFunc::FuncMatDeg },
    { "matexp",       InstrFunc::FuncMatExp },
    { "matlog",       InstrFunc::FuncMatLog },
    { "matpi",        InstrFunc::FuncMatPi },
    { "matpow",       InstrFunc::FuncMatPow },
    { "matrad",       InstrFunc::FuncMatRad },
    { "matraiz",      InstrFunc::FuncMatRaiz },
    { "matsin",       InstrFunc::FuncMatSin },
    { "mattan",       InstrFunc::FuncMatTan },
    { "objantes",     InstrFunc::FuncObjAntes },
    { "objdepois",    InstrFunc::FuncObjDepois },
    { "rand",         InstrFunc::FuncRand },
    { "ref",          InstrFunc::FuncRef },
    { "txt",          InstrFunc::FuncTxt },
    { "txt1",         InstrFunc::FuncTxt1 },
    { "txt2",         InstrFunc::FuncTxt2 },
    { "txtbit",       InstrFunc::FuncTxtBit },
    { "txtbith",      InstrFunc::FuncTxtBitH },
    { "txtchr",       InstrFunc::FuncTxtChr },
    { "txtcod",       InstrFunc::FuncTxtCod },
    { "txtconv",      InstrFunc::FuncTxtConv },
    { "txtcopiamai",  InstrFunc::FuncTxtCopiaMai },
    { "txtcor",       InstrFunc::FuncTxtCor },
    { "txtdec",       InstrFunc::FuncTxtDec },
    { "txte",         InstrFunc::FuncTxtE },
    { "txtesp",       InstrFunc::FuncEsp },
    { "txtfiltro",    InstrFunc::FuncTxtFiltro },
    { "txtfim",       InstrFunc::FuncTxtFim },
    { "txthex",       InstrFunc::FuncTxtHex },
    { "txtinvis",     InstrFunc::FuncTxtInvis },
    { "txtmai",       InstrFunc::FuncTxtMai },
    { "txtmaiini",    InstrFunc::FuncTxtMaiIni },
    { "txtmaimin",    InstrFunc::FuncTxtMaiMin },
    { "txtmd5",       InstrFunc::FuncTxtMd5 },
    { "txtmin",       InstrFunc::FuncTxtMin },
    { "txtmudamai",   InstrFunc::FuncTxtMudaMai },
    { "txtnome",      InstrFunc::FuncTxtNome },
    { "txtnum",       InstrFunc::FuncTxtNum },
    { "txtproc",      InstrFunc::FuncTxtProc },
    { "txtprocdif",   InstrFunc::FuncTxtProcDif },
    { "txtproclin",   InstrFunc::FuncTxtProcLin },
    { "txtproclindif",InstrFunc::FuncTxtProcLinDif },
    { "txtproclinmai",InstrFunc::FuncTxtProcLinMai },
    { "txtprocmai",   InstrFunc::FuncTxtProcMai },
    { "txtremove",    InstrFunc::FuncTxtRemove },
    { "txtrepete",    InstrFunc::FuncTxtRepete },
    { "txtrev",       InstrFunc::FuncTxtRev },
    { "txts",         InstrFunc::FuncTxtS },
    { "txtsepara",    InstrFunc::FuncTxtSepara },
    { "txtsha1",      InstrFunc::FuncTxtSha1 },
    { "txtsha1bin",   InstrFunc::FuncTxtSha1Bin },
    { "txtsub",       InstrFunc::FuncTxtSub },
    { "txtsublin",    InstrFunc::FuncTxtSubLin },
    { "txttroca",     InstrFunc::FuncTxtTroca },
    { "txttrocadif",  InstrFunc::FuncTxtTrocaDif },
    { "txttrocamai",  InstrFunc::FuncTxtTrocaMai },
    { "txturlcod",    InstrFunc::FuncTxtUrlCod },
    { "txturldec",    InstrFunc::FuncTxtUrlDec },
    { "txtvis",       InstrFunc::FuncTxtVis },
    { "vartroca",     InstrFunc::FuncVarTroca },
    { "vartrocacod",  InstrFunc::FuncVarTrocaCod }
};

//------------------------------------------------------------------------------
int Instr::InfoFunc(const char * nome)
{
// Procura a fun��o correspondente
    int ini = 0;
    int fim = sizeof(ListaFunc) / sizeof(ListaFunc[0]) - 1;
    char mens[80];
    copiastrmin(mens, nome, sizeof(mens));
    while (ini <= fim)
    {
        int meio = (ini+fim)/2;
        int resultado = strcmp(mens, ListaFunc[meio].Nome);
        if (resultado == 0) // Se encontrou...
            return meio;
        if (resultado < 0)
            fim = meio - 1;
        else
            ini = meio + 1;
    }
    return -1;
}

//------------------------------------------------------------------------------
/// Retorna um n�mero que corresponde � prioridade do operador
/** @param operador Operador em Instr::Expressao
    @retval 2-0x2F N�mero que corresponde � prioridade do operador;
            2 � operador unit�rio (� tamb�m o de maior prioridade);
            3 � operador unit�rio que vem depois do operando
            20 � atribui��o (processado da direita para a esquerda)
    @retval 0 Operador inv�lido */
int Instr::Prioridade(int operador)
{
    switch (operador)
    {
    case exo_virgula:    return 22;
    case exo_virg_expr:  return 22;
    case exo_neg:        return 2;
    case exo_exclamacao: return 2;
    case exo_b_comp:     return 2;
    case exo_add_antes:  return 2;
    case exo_sub_antes:  return 2;
    case exo_add_depois: return 3;
    case exo_sub_depois: return 3;
    case exo_add_sub1:   return 3;
    case exo_add_sub2:   return 3;
    case exo_mul:        return 4;
    case exo_div:        return 4;
    case exo_porcent:    return 4;
    case exo_add:        return 5;
    case exo_sub:        return 5;
    case exo_b_shl:      return 6;
    case exo_b_shr:      return 6;
    case exo_b_e:        return 7;
    case exo_b_ouou:     return 8;
    case exo_b_ou:       return 9;
    case exo_menor:      return 10;
    case exo_menorigual: return 10;
    case exo_maior:      return 10;
    case exo_maiorigual: return 10;
    case exo_igual:      return 11;
    case exo_igual2:     return 11;
    case exo_diferente:  return 11;
    case exo_diferente2: return 11;
    case exo_e:          return 12;
    case exo_ou:         return 13;
    case exo_int1:       return 14;
    case exo_int2:       return 14;
    case exo_intint1:    return 14;
    case exo_intint2:    return 14;
    case exo_dponto1:    return 14;
    case exo_dponto2:    return 14;
    case exo_atrib:      return 20;
    case exo_i_mul:      return 20;
    case exo_i_div:      return 20;
    case exo_i_porcent:  return 20;
    case exo_i_add:      return 20;
    case exo_i_sub:      return 20;
    case exo_i_b_shl:    return 20;
    case exo_i_b_shr:    return 20;
    case exo_i_b_e:      return 20;
    case exo_i_b_ouou:   return 20;
    case exo_i_b_ou:     return 20;
    }
    return 0;
}

//------------------------------------------------------------------------------
/// Verifica se instru��o codificada � herda e cont�m a classe especificada
bool Instr::ChecaHerda(const char * instr, const char * nomeclasse)
{
    if (instr[0] == 0 && instr[1] == 0)
        return false;
    if (instr[2] != cHerda)
        return false;
    int x = (unsigned char)instr[3];
    for (instr += 4; x; x--)
    {
        if (comparaVar(instr, nomeclasse) == 0)
            return true;
        while (*instr++);
    }
    return false;
}

//----------------------------------------------------------------------------
/// Passa para a pr�xima fun��o ou vari�vel
/**
 * @param instr Instru��o atual (fun��o ou vari�vel)
 * @param texto O nome da pr�xima fun��o/vari�vel deve come�ar com esse texto
 * @param tamanho Quantos caracteres o texto tem
 * @return Endere�o da pr�xima instru��o ou 0 se n�o houver
 */
const char * Instr::ProximaInstr(const char * instr, const char * texto, int tamanho)
{
inicio:
// Avan�ar para pr�xima vari�vel
    if (instr[2] != Instr::cFunc && instr[2] != Instr::cVarFunc)
    {
        for (instr += Num16(instr); instr[0] || instr[1]; instr += Num16(instr))
            if (instr[2] > Instr::cVariaveis)
            {
                if (tamanho && compara(texto, instr + Instr::endNome, tamanho) != 0)
                    goto inicio;
                return instr;
            }
        return nullptr;
    }
// Avan�ar para pr�xima fun��o
    for (instr += Num16(instr); instr[0] || instr[1]; instr += Num16(instr))
        if (instr[2] > Instr::cVariaveis)
            switch (instr[2])
            {
            case Instr::cConstNulo:
            case Instr::cConstTxt:
            case Instr::cConstNum:
            case Instr::cConstExpr:
            case Instr::cConstVar:
            case Instr::cFunc:
            case Instr::cVarFunc:
                if (tamanho && compara(texto, instr + Instr::endNome, tamanho) != 0)
                    goto inicio;
                return instr;
            }
    return nullptr;
}

//----------------------------------------------------------------------------
/// Cria vari�vel int com um valor definido
/**
 *  @param valor Valor num�rico da vari�vel
 *  @return true se conseguiu criar, false se mem�ria insuficiente */
bool Instr::CriarVarInt(int valor)
{
    if (VarAtual >= VarFim - 1)
        return false;
    VarAtual++;
    VarAtual->defvar = InstrVarInt;
    VarAtual->nomevar = InstrVarInt;
    VarAtual->valor_int = valor;
    VarAtual->tamanho = 0;
    VarAtual->indice = 0;
    return true;
}

//----------------------------------------------------------------------------
/// Cria vari�vel double com um valor definido
/**
 *  @param valor Valor num�rico da vari�vel
 *  @return true se conseguiu criar, false se mem�ria insuficiente */
bool Instr::CriarVarDouble(double valor)
{
    int tam = TVariavel::Tamanho(InstrDouble);
// Acerta alinhamento do endere�o da vari�vel
    char * p = Instr::DadosTopo;
    p += ((p - Instr::DadosPilha) & 1);
    p += ((p - Instr::DadosPilha) & 2);
// Verifica se mem�ria suficiente
    if (VarAtual >= VarFim - 1 || p + tam > Instr::DadosFim)
      return false;
// Prepara vari�vel
    VarAtual++;
    VarAtual->endvar = p;
    VarAtual->defvar = InstrDouble;
    VarAtual->nomevar = InstrDouble;
    VarAtual->tamanho = tam;
    VarAtual->indice = 0;
// Cria vari�vel
    Instr::DadosTopo = p + tam;
    VarAtual->setDouble(valor);
    return true;
}

//----------------------------------------------------------------------------
/// Cria vari�vel de texto no topo da pilha de vari�veis (Instr::VarPilha)
/**
 *  @param mens A mensagem que ser� retornada;
 *               Se for 0, apenas aloca mem�ria para o texto
 *  @param tammens Tamanho da mensagem sem o 0 final;
 *                 Se for < 0, assume o valor de strlen(mens)
 *  @return true se conseguiu criar, false se mem�ria insuficiente
 *  @note Pode copiar apenas parte do texto se a mem�ria for insuficiente */
bool Instr::CriarVarTexto(const char * mens, int tammens)
{
// Acerta vari�veis
    if (VarAtual >= VarFim - 1)
        return false;
// Verifica espa�o dispon�vel (sem o 0 no final do texto)
    if (tammens < 0)
    {
        if (mens == nullptr)
            return false;
        tammens = strlen(mens);
    }
    int disp = DadosFim - DadosTopo - 1;
    if (disp<0)
        return false;
    if (tammens > disp)
        tammens = disp;
// Copia texto
    if (tammens > 0 && mens)
        memcpy(DadosTopo, mens, tammens);
    DadosTopo[tammens] = 0;
// Acerta vari�veis
    VarAtual++;
    VarAtual->defvar = InstrTxtFixo;
    VarAtual->nomevar = InstrTxtFixo;
    VarAtual->endvar = DadosTopo;
    VarAtual->tamanho = tammens + 1;
    VarAtual->indice = 0;
    DadosTopo += tammens + 1;
    return true;
}

//----------------------------------------------------------------------------
/// Cria vari�vel ref com um valor definido
/**
 *  @param obj Objeto correspondente � vari�vel
 *  @return true se conseguiu criar, false se mem�ria insuficiente */
bool Instr::CriarVarObj(TObjeto * obj)
{
    if (VarAtual >= VarFim - 1)
        return false;
    VarAtual++;
    if (obj)
    {
        VarAtual->defvar = InstrVarObjeto;
        VarAtual->nomevar = InstrVarObjeto;
    }
    else
    {
        VarAtual->defvar = InstrNulo;
        VarAtual->nomevar = InstrNulo;
    }
    VarAtual->endvar = obj;
    VarAtual->tamanho = 0;
    VarAtual->indice = 0;
    return true;
}

//------------------------------------------------------------------------------
/// Cria vari�vel no topo da pilha de vari�veis (Instr::VarPilha)
/** @return true se conseguiu criar, false se mem�ria insuficiente */
bool Instr::CriarVar(const char * def)
{
    if (VarAtual >= VarFim - 1)
        return false;
// Verifica se tamanho da vari�vel � nulo
    int tam = TVariavel::Tamanho(def);
    if (tam == 0)
    {
        VarAtual++;
        VarAtual->defvar = def;
        VarAtual->nomevar = def;
        VarAtual->endvar = 0;
        VarAtual->tamanho = 0;
        VarAtual->indice = (def[endVetor]==0 ? 0 : 0xFF);
        VarAtual->numbit = 0;
        VarAtual->numfunc = 0;
        return true;
    }
// Acerta alinhamento do endere�o da vari�vel
    char * p = Instr::DadosTopo;
    if ((tam & 1) == 0)
        p += ((p - Instr::DadosPilha) & 1);
    if ((tam & 3) == 0)
        p += ((p - Instr::DadosPilha) & 2);
// Prepara vari�vel
    VarAtual++;
    VarAtual->endvar = p;
    VarAtual->defvar = def;
    VarAtual->nomevar = def;
    VarAtual->indice = (def[endVetor] == 0 ? 0 : 0xFF);
    VarAtual->numbit = 0;
    VarAtual->numfunc = 0;
    VarAtual->tamanho = VarAtual->TamanhoVetor();
// Verifica se mem�ria suficiente
    if (p + VarAtual->tamanho > Instr::DadosFim)
    {
        VarAtual--;
        return false;
    }
// Cria vari�vel
    Instr::DadosTopo = p + VarAtual->tamanho;
    VarAtual->Criar(0, 0);
    return true;
}

//----------------------------------------------------------------------------
/// Apaga vari�veis na pilha a partir da vari�vel v
void Instr::ApagarVar(TVariavel * v)
{
    while (VarAtual >= v)
    {
        if (VarAtual->tamanho)
        {
            if (VarAtual->endvar)
                DadosTopo = (char*)VarAtual->endvar;
            VarAtual->Apagar(); // Pode alterar VarAtual->endvar
        }
        VarAtual--;
    }
}

//----------------------------------------------------------------------------
/// Apaga vari�veis na pilha de v a VarAtual-1
void Instr::ApagarRet(TVariavel * v)
{
    for (TVariavel * var = VarAtual - 1; var >= v; var--)
    {
        if (var->tamanho == 0)
            continue;
        DadosTopo = (char*)var->endvar;
        if (var->endvar == VarAtual->endvar) // Se forem a mesma vari�vel
            VarAtual->tamanho = var->tamanho; // Acerta o tamanho de VarAtual
                                // porque est� com 0 (� c�pia de var)
        else
            var->Apagar(); // Caso contr�rio, apaga a vari�vel
    }
    *v = *VarAtual;
    VarAtual = v;
    if (VarAtual->tamanho)
    {
        char * p = DadosTopo;
        if (VarAtual->defvar[2] != cTxtFixo)
        {
            if ((VarAtual->tamanho & 1) == 0)
                p += ((p-Instr::DadosPilha) & 1);
            if ((VarAtual->tamanho & 3) == 0)
                p += ((p-Instr::DadosPilha) & 2);
        }
        if (p < VarAtual->endvar)
            VarAtual->MoverEnd(p, nullptr, nullptr);
        DadosTopo = (char*)VarAtual->endvar + VarAtual->tamanho;
    }
}

//----------------------------------------------------------------------------
/// Inicializa��o de varfunc
/** Prepara para executar varfunc
   @param varini Procura vari�veis varfunc a partir de varini
   @return true se vai executar varfunc, false se n�o vai
*/
bool Instr::VarFuncIni(TVariavel * varini)
{
    for (TVariavel * v=VarAtual; v>=varini; v--)
    {
        const char * defvar = v->defvar;
        if (defvar[2] != cVarFunc && defvar[2] != cConstVar)
            continue;
        if (FuncAtual+1 >= FuncFim)
            return false;
    // Cria fun��o
        FuncAtual++;
        FuncAtual->nome = defvar;
        if (defvar[2] == cVarFunc)
        {
            FuncAtual->linha = defvar + Num16(defvar);
            FuncAtual->expr = nullptr;
        }
        else
        {
            FuncAtual->linha = defvar;
            FuncAtual->expr = defvar + defvar[endIndice];
        }
        FuncAtual->este = (TObjeto*)v->endvar;
        FuncAtual->inivar = VarAtual + 1;
        FuncAtual->fimvar = VarAtual + 1;
        FuncAtual->numarg = 0;
        FuncAtual->tipo = 1;
        FuncAtual->indent = 0;
        if (FuncAtual >= FuncPilha)
        {
            FuncAtual->objdebug = FuncAtual[-1].objdebug;
            FuncAtual->funcdebug = FuncAtual[-1].funcdebug;
        }
        else
            FuncAtual->funcdebug = nullptr;
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------
/// Finaliza��o de varfunc
/** Coloca �ltima vari�vel da pilha no lugar da �ltima vari�vel cVarFunc
    @return true se conseguiu, false se mem�ria insuficiente em DadosPilha
*/
bool Instr::VarFuncFim()
{
    TVariavel * vfunc = VarAtual;
    char * endini = nullptr;

// Procura �ltima vari�vel cVarFunc da pilha
    while (true)
    {
        vfunc--;
        assert(vfunc >= VarPilha);
        if (vfunc->defvar[2] == cVarFunc || // Se achou varfunc...
            vfunc->defvar[2] == cConstVar)
            break;
        if (vfunc->tamanho == 0) // Se est� definida em outro lugar...
            continue;
        endini = (char*)vfunc->endvar;
        if (endini != VarAtual->endvar) // Se n�o � a mesma vari�vel...
            continue;
        VarAtual->tamanho = vfunc->tamanho; // VarAtual � a primeira
        vfunc->tamanho = 0; // A outra � c�pia
        endini = 0;         // Por enquanto est� na ordem correta em DadosPilha
    }
    assert(vfunc->tamanho == 0);

// Verifica se precisa acertar DadosPilha
    if (VarAtual->tamanho == 0 || // Se vari�vel n�o est� em DadosPilha
            endini == 0)  // Se vari�veis ent�o na ordem correta em DadosPilha
    {
        *vfunc = *VarAtual;
        VarAtual--;
        return true;
    }

// Move vari�veis VarAtual a (vfunc+1) para o fim da �rea de dados
    char * destino = DadosFim;
    for (TVariavel * v = VarAtual; v>vfunc; v--)
        if (v->tamanho)
        {
            char * origem = (char*)v->endvar;
            destino -= (v->tamanho * 4) & ~3;
            if (v != VarAtual)
                DadosTopo = origem;
            if (DadosTopo > destino)
                return false;
            v->MoverEnd(destino, nullptr, nullptr);
            for (TVariavel * vtemp = VarAtual; vtemp > vfunc; vtemp--)
                if (vtemp->endvar == origem)
                    vtemp->endvar = destino;
        }

// Move VarAtual para vfunc em VarPilha
    *vfunc = *VarAtual;
    VarAtual--;

// Move vari�veis vfunc a VarAtual para onde dever�o ficar
    for (TVariavel * v = vfunc; v <= VarAtual; v++)
        if (v->tamanho)
        {
            if (v->defvar[2] != cTxtFixo) // Alinhamento conforme vari�vel
            {
                if ((v->tamanho & 1) == 0)
                    endini += ((endini - Instr::DadosPilha) & 1);
                if ((v->tamanho & 3) == 0)
                    endini += ((endini - Instr::DadosPilha) & 2);
            }
            char * origem = (char*)v->endvar;
            v->MoverEnd(endini, nullptr, nullptr);
            for (TVariavel * vtemp = vfunc; vtemp <= VarAtual; vtemp++)
                if (vtemp->endvar == origem)
                    vtemp->endvar = endini;
            endini += v->tamanho;
        }
    DadosTopo = endini;
    return true;
}

//------------------------------------------------------------------------------
/// Verifica se duas instru��es codificadas s�o iguais
/** @return true se s�o iguais, false se s�o diferentes */
bool Instr::ComparaInstr(const char * instr1, const char * instr2)
{
    int total = Num16(instr1); // Quantidade de bytes em instr1
    int inicio = Instr::endVar;
    const char * expr = nullptr; // Express�o num�rica que est� analisando
               // Se 0, n�o checa express�o; apenas compara bytes a partir
               // de instr1+inicio com instr2+inicio
    if (total == 0) // Fim das instru��es em instr1: checa fim em instr2
        return (instr2[0] == 0 && instr2[1] == 0);
    if (memcmp(instr1, instr2, 3) != 0)
        return false;
    switch (instr1[2])
    {
    case cExpr:         expr = instr1 + Instr::endVar;     break;
    case cSe:           expr = instr1 + Instr::endVar + 2; break;
    case cSenao2:       expr = instr1 + Instr::endVar + 2; break;
    case cEnquanto:     expr = instr1 + Instr::endVar + 2; break;
    case cEPara:        expr = instr1 + Instr::endVar + 6; break;
    case cCasoVar:      expr = instr1 + Instr::endVar + 2; break;
    case cRet2:         expr = instr1 + Instr::endVar;     break;
    case cSair2:        expr = instr1 + Instr::endVar + 2; break;
    case cContinuar2:   expr = instr1 + Instr::endVar + 2; break;
    case cSenao1:       inicio = endVar + 2; break;
    case cFimSe:        inicio = endVar;     break;
    case cEFim:         inicio = endVar + 2; break;
    case cCasoSe:       inicio = endVar + 4; break;
    case cCasoSePadrao: inicio = endVar;     break;
    case cCasoFim:      inicio = endVar;     break;
    case cRet1:         inicio = endVar;     break;
    case cSair1:        inicio = endVar + 2; break;
    case cContinuar1:   inicio = endVar + 2; break;
    case cTerminar:     inicio = endVar;     break;
    case cRefVar:
        if (strcmp(instr1 + endNome, instr2 + endNome) != 0)
            return false;
        expr = instr1 + endNome;
        while (*expr++);
        break;
    default:
        if (instr1[2] < cVariaveis || instr1[endIndice] == 0)
            break;
        if (strcmp(instr1 + endNome, instr2 + endNome) != 0)
            return false;
        expr = instr1 + endNome;
        while (*expr++);
        break;
    }
    if (inicio >= total)
        return true;
    if (expr == 0)
        return (memcmp(instr1 + inicio, instr2 + inicio, total - inicio) == 0);

    {
        int valor = expr - instr1;
        instr1 += valor;
        instr2 += valor;
        total -= valor;
    }
    while (*expr != ex_fim && *expr != ex_coment)
    {
        switch (*expr++)
        {
        case ex_varlocal:
            if (expr != instr1)
            {
                int valor = expr - instr1;
                if (memcmp(instr1, instr2, valor) != 0)
                    return false;
                instr1 += valor + 1;
                instr2 += valor + 1;
                total -= valor + 1;
            }
        case ex_varfunc:
            expr++;
        case ex_varini: // Ser� fechado com ex_varfim
        case ex_fecha:  // Aberto com ex_abre
        case ex_ponto:  // Aberto com ex_arg
        case ex_doispontos:
            while (*expr != ex_arg && *expr != ex_abre && *expr != ex_varfim)
            {
                assert(*expr != 0);
                expr++;
            }
            break;
        case ex_varfim:
        case ex_nulo:
            break;
        case ex_txt:
            while (*expr)
                expr++;
            expr++;
            break;
        case ex_num32p:
        case ex_num32n:
        case ex_num32hexp:
        case ex_num32hexn:
            expr+=2;
        case ex_num16p:
        case ex_num16n:
        case ex_num16hexp:
        case ex_num16hexn:
            expr++;
        case ex_num8p:
        case ex_num8n:
        case ex_num8hexp:
        case ex_num8hexn:
            expr++;
        case ex_num0:
        case ex_num1:
            while (*expr >= ex_div1 &&
                   *expr <= ex_div6)
                expr++;
            break;
        }
    }
    if (total>0)
        if (memcmp(instr1, instr2, total) != 0)
            return false;
    return true;
}

//----------------------------------------------------------------------------
/// Procura valor (vide Instr::Expressao) em express�o num�rica
/** @return endere�o do valor encontrado ou 0 se n�o encontrou
 *  @note N�o procura entre ex_varini e ex_varfim
 */
const char * Instr::ProcuraExpr(const char * expr, int valor)
{
    int contagem = 0;
    while (true)
    {
        //printf(">> %d %s\n", contagem, NomeExpr(*expr)); fflush(stdout);
        if (*expr == valor && contagem <= 0)
            return expr;
        switch (*expr++)
        {
        case ex_fim:
        case ex_coment:
            return nullptr;
        case ex_arg:    // In�cio da lista de argumentos
        case ex_abre:   // Abre colchetes; segue express�o num�rica + ex_fecha
            break;
        case ex_varabre:
            contagem++;
            break;
        case ex_varfunc:
        case ex_varlocal:
            expr++;
        case ex_varini: // Ser� fechado com ex_varfim
            contagem++;
        case ex_fecha:  // Aberto com ex_abre
        case ex_ponto:  // Aberto com ex_arg
        case ex_doispontos:
            while (*expr != ex_arg && *expr != ex_abre && *expr != ex_varfim)
            {
                assert(*expr != 0);
                expr++;
            }
            break;
        case ex_varfim:
            contagem--;
            break;

        case ex_nulo:
            break;
        case ex_txt:
            while (*expr)
                expr++;
            expr++;
            break;
        case ex_num32p:
        case ex_num32n:
        case ex_num32hexp:
        case ex_num32hexn:
            expr+=2;
        case ex_num16p:
        case ex_num16n:
        case ex_num16hexp:
        case ex_num16hexn:
            expr++;
        case ex_num8p:
        case ex_num8n:
        case ex_num8hexp:
        case ex_num8hexn:
            expr++;
        case ex_num0:
        case ex_num1:
            while (*expr >= ex_div1 && *expr <= ex_div6)
                expr++;
            break;

        case exo_virgula:
        case exo_virg_expr:
        case exo_neg:
        case exo_exclamacao:
        case exo_b_comp:
        case exo_add_antes:
        case exo_add_depois:
        case exo_sub_antes:
        case exo_sub_depois:
        case exo_add_sub1:
        case exo_add_sub2:
        case exo_mul:
        case exo_div:
        case exo_porcent:
        case exo_add:
        case exo_sub:
        case exo_b_shl:
        case exo_b_shr:
        case exo_b_e:
        case exo_b_ouou:
        case exo_b_ou:
        case exo_menor:
        case exo_menorigual:
        case exo_maior:
        case exo_maiorigual:
        case exo_igual:
        case exo_igual2:
        case exo_diferente:
        case exo_diferente2:
        case exo_atrib:
        case exo_i_mul:
        case exo_i_div:
        case exo_i_porcent:
        case exo_i_add:
        case exo_i_sub:
        case exo_i_b_shl:
        case exo_i_b_shr:
        case exo_i_b_e:
        case exo_i_b_ouou:
        case exo_i_b_ou:
            break;
        case exo_e:
        case exo_ou:
        case exo_int2:
        case exo_intint2:
        case exo_dponto2:
            contagem--;
            break;
        case exo_ee:
        case exo_ouou:
        case exo_int1:
        case exo_intint1:
        case exo_dponto1:
            contagem++;
            break;
        default:
            assert(0);
        }
    }
}

//----------------------------------------------------------------------------
/// Obt�m o nome de uma instru��o
/** @param  instr Instru��o codificada
 *  @return nome, ou "" se n�o encontrou
 */
const char * Instr::NomeInstr(const char * instr)
{
    static char nome[16];
    switch ((Instr::Comando)instr[2])
    {
    case cHerda:            return "herda";
    case cExpr:             return "";
    case cComent:           return "#";
    case cRefVar:           return "refvar";

    case cSe:               return "se";
    case cSenao1:           return "senao1";
    case cSenao2:           return "senao2";
    case cFimSe:            return "fimse";
    case cEnquanto:         return "enquanto";
    case cEPara:            return "epara";
    case cEFim:             return "efim";
    case cCasoVar:          return "casovar";
    case cCasoSe:           return "casose";
    case cCasoSePadrao:     return "casosepadrao";
    case cCasoFim:          return "casofim";
    case cRet1:             return "ret";
    case cRet2:             return "ret";
    case cSair1:            return "sair1";
    case cSair2:            return "sair2";
    case cContinuar1:       return "continuar1";
    case cContinuar2:       return "continuar2";
    case cTerminar:         return "terminar";

    case cVariaveis:        return "";
    case cTxt1:
        sprintf(nome, "txt%d", (unsigned char)instr[endExtra] + 1);
        return nome;
    case cTxt2:
        sprintf(nome, "txt%d", (unsigned char)instr[endExtra] + 257);
        return nome;
    case cInt1:             return "int1";
    case cInt8:             return "int8";
    case cUInt8:            return "uint8";
    case cInt16:            return "int16";
    case cUInt16:           return "uint16";
    case cInt32:            return "int32";
    case cUInt32:           return "uint32";
    case cIntInc:           return "intinc";
    case cIntDec:           return "intdec";
    case cReal:             return "real";
    case cReal2:            return "real2";
    case cRef:              return "ref";
    case cConstNulo:
    case cConstTxt:
    case cConstNum:
    case cConstExpr:        return "const";
    case cConstVar:         return "varconst";
    case cFunc:             return "func";
    case cVarFunc:          return "varfunc";

    case cListaObj:         return "listaobj";
    case cListaItem:        return "listaitem";
    case cTextoTxt:         return "textotxt";
    case cTextoPos:         return "textopos";
    case cTextoVar:         return "textovar";
    case cTextoObj:         return "textoobj";
    case cNomeObj:          return "nomeobj";
    case cArqDir:           return "arqdir";
    case cArqLog:           return "arqlog";
    case cArqProg:          return "arqprog";
    case cArqExec:          return "arqexec";
    case cArqSav:           return "arqsav";
    case cArqTxt:           return "arqtxt";
    case cArqMem:           return "arqmem";
    case cIntTempo:         return "inttempo";
    case cIntExec:          return "intexec";
    case cTelaTxt:          return "telatxt";
    case cSocket:           return "socket";
    case cServ:             return "serv";
    case cProg:             return "prog";
    case cDebug:            return "debug";
    case cIndiceObj:        return "indiceobj";
    case cIndiceItem:       return "indiceitem";
    case cDataHora:         return "datahora";

    case cTxtFixo:
    case cVarNome:
    case cVarInicio:
    case cVarIniFunc:
    case cVarClasse:
    case cVarObjeto:
    case cVarInt:
    case cTextoVarSub:
    case cTextoObjSub:      return "";
    case cTotalComandos:    break;
    }
    return "";
}

//----------------------------------------------------------------------------
/// Obt�m o nome de um valor de Instr::Comando
/** @param  valor Valor de Instr::Comando
 *  @return nome, ou 0 se n�o encontrou
 */
const char * Instr::NomeComando(int valor)
{
    switch ((Instr::Comando)valor)
    {
    case cHerda:            return "cHerda";
    case cExpr:             return "cExpr";
    case cComent:           return "cComent";
    case cRefVar:           return "cRefVar";

    case cSe:               return "cSe";
    case cSenao1:           return "cSenao1";
    case cSenao2:           return "cSenao2";
    case cFimSe:            return "cFimSe";
    case cEnquanto:         return "cEnquanto";
    case cEPara:            return "cEPara";
    case cEFim:             return "cEFim";
    case cCasoVar:          return "cCasoVar";
    case cCasoSe:           return "cCasoSe";
    case cCasoSePadrao:     return "cCasoSePadrao";
    case cCasoFim:          return "cCasoFim";
    case cRet1:             return "cRet1";
    case cRet2:             return "cRet2";
    case cSair1:            return "cSair1";
    case cSair2:            return "cSair2";
    case cContinuar1:       return "cContinuar1";
    case cContinuar2:       return "cContinuar2";
    case cTerminar:         return "cTerminar";

    case cVariaveis:        return "cVariaveis";
    case cTxt1:             return "cTxt1";
    case cTxt2:             return "cTxt2";
    case cInt1:             return "cInt1";
    case cInt8:             return "cInt8";
    case cUInt8:            return "cUInt8";
    case cInt16:            return "cInt16";
    case cUInt16:           return "cUInt16";
    case cInt32:            return "cInt32";
    case cUInt32:           return "cUInt32";
    case cIntInc:           return "cIntInc";
    case cIntDec:           return "cIntDec";
    case cReal:             return "cReal";
    case cReal2:            return "cReal2";
    case cRef:              return "cRef";
    case cConstNulo:        return "cConstNulo";
    case cConstTxt:         return "cConstTxt";
    case cConstNum:         return "cConstNum";
    case cConstExpr:        return "cConstExpr";
    case cConstVar:         return "cConstVar";
    case cFunc:             return "cFunc";
    case cVarFunc:          return "cVarFunc";

    case cListaObj:         return "cListaObj";
    case cListaItem:        return "cListaItem";
    case cTextoTxt:         return "cTextoTxt";
    case cTextoPos:         return "cTextoPos";
    case cTextoVar:         return "cTextoVar";
    case cTextoObj:         return "cTextoObj";
    case cNomeObj:          return "cNomeObj";
    case cArqDir:           return "cArqDir";
    case cArqLog:           return "cArqLog";
    case cArqProg:          return "cArqProg";
    case cArqExec:          return "cArqExec";
    case cArqSav:           return "cArqSav";
    case cArqTxt:           return "cArqTxt";
    case cArqMem:           return "cArqMem";
    case cIntTempo:         return "cIntTempo";
    case cIntExec:          return "cIntExec";
    case cTelaTxt:          return "cTelaTxt";
    case cSocket:           return "cSocket";
    case cServ:             return "cServ";
    case cProg:             return "cProg";
    case cDebug:            return "cDebug";
    case cIndiceObj:        return "cIndiceObj";
    case cIndiceItem:       return "cIndiceItem";
    case cDataHora:         return "cDataHora";

    case cTxtFixo:          return "cTxtFixo";
    case cVarNome:          return "cVarNome";
    case cVarInicio:        return "cVarInicio";
    case cVarIniFunc:       return "cVarIniFunc";
    case cVarClasse:        return "cVarClasse";
    case cVarObjeto:        return "cVarObjeto";
    case cVarInt:           return "cVarInt";
    case cTextoVarSub:      return "cTextoVarSub";
    case cTextoObjSub:      return "cTextoObjSub";
    case cTotalComandos:    break;
    }
    return 0;
}

//----------------------------------------------------------------------------
/// Obt�m o nome de um valor de Instr::Expressao
/** @param  valor Valor de Instr::Expressao
 *  @return nome, ou 0 se n�o encontrou
 */
const char * Instr::NomeExpr(int valor)
{
    switch ((Instr::Expressao)valor)
    {
    case ex_fim:            return "ex_fim";
    case ex_coment:         return "ex_coment";

    case ex_barra_n:        return "ex_barra_n";
    case ex_barra_b:        return "ex_barra_b";
    case ex_barra_c:        return "ex_barra_c";
    case ex_barra_d:        return "ex_barra_d";

    case ex_varini:         return "ex_varini";
    case ex_varfim:         return "ex_varfim";
    case ex_doispontos:     return "ex_doispontos";
    case ex_ponto:          return "ex_ponto";
    case ex_arg:            return "ex_arg";
    case ex_varabre:        return "ex_varabre";
    case ex_abre:           return "ex_abre";
    case ex_fecha:          return "ex_fecha";
    case ex_varfunc:        return "ex_varfunc";
    case ex_varlocal:       return "ex_varlocal";

    case ex_nulo:           return "ex_nulo";
    case ex_txt:            return "ex_txt";
    case ex_num0:           return "ex_num0";
    case ex_num1:           return "ex_num1";
    case ex_num8p:          return "ex_num8p";
    case ex_num16p:         return "ex_num16p";
    case ex_num32p:         return "ex_num32p";
    case ex_num8n:          return "ex_num8n";
    case ex_num16n:         return "ex_num16n";
    case ex_num32n:         return "ex_num32n";
    case ex_num8hexp:       return "ex_num8hexp";
    case ex_num16hexp:      return "ex_num16hexp";
    case ex_num32hexp:      return "ex_num32hexp";
    case ex_num8hexn:       return "ex_num8hexn";
    case ex_num16hexn:      return "ex_num16hexn";
    case ex_num32hexn:      return "ex_num32hexn";
    case ex_div1:           return "ex_div1";
    case ex_div2:           return "ex_div2";
    case ex_div3:           return "ex_div3";
    case ex_div4:           return "ex_div4";
    case ex_div5:           return "ex_div5";
    case ex_div6:           return "ex_div6";

    case exo_ini:           return "exo_ini";
    case exo_virgula:       return "exo_virgula";
    case exo_virg_expr:     return "exo_virg_expr";
    case exo_neg:           return "exo_neg";
    case exo_exclamacao:    return "exo_exclamacao";
    case exo_b_comp:        return "exo_b_comp";
    case exo_add_antes:     return "exo_add_antes";
    case exo_sub_antes:     return "exo_sub_antes";
    case exo_add_depois:    return "exo_add_depois";
    case exo_sub_depois:    return "exo_sub_depois";
    case exo_add_sub1:      return "exo_add_sub1";
    case exo_add_sub2:      return "exo_add_sub2";
    case exo_mul:           return "exo_mul";
    case exo_div:           return "exo_div";
    case exo_porcent:       return "exo_porcent";
    case exo_add:           return "exo_add";
    case exo_sub:           return "exo_sub";
    case exo_b_shl:         return "exo_b_shl";
    case exo_b_shr:         return "exo_b_shr";
    case exo_b_e:           return "exo_b_e";
    case exo_b_ouou:        return "exo_b_ouou";
    case exo_b_ou:          return "exo_b_ou";
    case exo_menor:         return "exo_menor";
    case exo_menorigual:    return "exo_menorigual";
    case exo_maior:         return "exo_maior";
    case exo_maiorigual:    return "exo_maiorigual";
    case exo_igual:         return "exo_igual";
    case exo_igual2:        return "exo_igual2";
    case exo_diferente:     return "exo_diferente";
    case exo_diferente2:    return "exo_diferente2";
    case exo_e:             return "exo_e";
    case exo_ou:            return "exo_ou";
    case exo_atrib:         return "exo_atrib";
    case exo_i_mul:         return "exo_i_mul";
    case exo_i_div:         return "exo_i_div";
    case exo_i_porcent:     return "exo_i_porcent";
    case exo_i_add:         return "exo_i_add";
    case exo_i_sub:         return "exo_i_sub";
    case exo_i_b_shl:       return "exo_i_b_shl";
    case exo_i_b_shr:       return "exo_i_b_shr";
    case exo_i_b_e:         return "exo_i_b_e";
    case exo_i_b_ouou:      return "exo_i_b_ouou";
    case exo_i_b_ou:        return "exo_i_b_ou";
    case exo_fim:           return "exo_fim";
    case exo_ee:            return "exo_ee";
    case exo_ouou:          return "exo_ouou";
    case exo_int1:          return "exo_int1";
    case exo_int2:          return "exo_int2";
    case exo_intint1:       return "exo_intint1";
    case exo_intint2:       return "exo_intint2";
    case exo_dponto1:       return "exo_dponto1";
    case exo_dponto2:       return "exo_dponto2";

    case ex_var1:           return "ex_var1";
    case ex_var2:           return "ex_var2";
    case ex_var3:           return "ex_var3";
    case ex_colchetes:      return "ex_colchetes";
    case ex_parenteses:     return "ex_parenteses";
    }
    return 0;
}

//------------------------------------------------------------------------------
Instr::ChecaLinha::ChecaLinha()
{
    esperando = 0;
    pbuf = 0;
}

//------------------------------------------------------------------------------
void Instr::ChecaLinha::Inicio()
{
    esperando = 0;
    pbuf = 0;
}

//------------------------------------------------------------------------------
const char * Instr::ChecaLinha::Instr(const char * instr)
{
    if (instr[0] == 0 && instr[1] == 0)
        return nullptr;
    unsigned char cod = *(unsigned char*)(instr+2);
// Instru��o Herda
    if (cod == cHerda)
    {
        if (esperando != 0)
            return "Instru��o Herda deve ser a primeira da classe";
        esperando=1;
        return nullptr;
    }
    if (esperando == 0)
        esperando = 1;
// Coment�rio
    if (cod == cComent)
        return nullptr;
// Fun��o
    if (cod == cFunc || cod == cVarFunc)
    {
        esperando = 2;
        return Fim();
    }
// Constante
    if (cod == cConstNulo || cod == cConstTxt ||
        cod == cConstNum  || cod == cConstExpr || cod == cConstVar)
    {
        if (esperando != 1)
            esperando = 3;
        return Fim();
    }
// Vari�vel
    if (cod >= cVariaveis)
    {
        if (esperando == 3)
            return "Vari�vel n�o pertence a uma classe ou uma fun��o";
        else if (esperando == 2 && InfoFunc(instr + endNome) >= 0)
            return "Vari�vel tem o nome de uma fun��o da linguagem, "
                    "por isso n�o pode pertencer a uma fun��o";
        else if (esperando < 2 && instr[endIndice] != 0)
            switch (cod)
            {
            case cConstNulo:
            case cConstTxt:
            case cConstNum:
            case cConstExpr:
            case cConstVar:
            case cFunc:
            case cVarFunc:
                break;
            default:
                return "Somente vari�veis definidas em fun��es permitem "
                        "atribui��o de valor";
            }
        return nullptr;
    }
// Instru��o
    if (esperando != 2)
        return "Instru��o n�o pertence a uma fun��o";
// Blocos de instru��es
    if (ChecaErro <= 0)
        return nullptr;
    switch (cod)
    {
    case cSe:
        if (pbuf < sizeof(buf))
            buf[pbuf++] = 0;
        break;
    case cSenao1:
    case cSenao2:
        if (pbuf ? buf[pbuf - 1] != 0 : true)
            return "Sen�o sem Se";
        break;
    case cFimSe:
        if (pbuf ? buf[pbuf - 1] != 0 : true)
            return "FimSe sem Se";
        pbuf--;
        break;
    case cEnquanto:
    case cEPara:
        if (pbuf < sizeof(buf))
            buf[pbuf++] = 1;
        break;
    case cEFim:
        if (pbuf ? buf[pbuf - 1] != 1 : true)
            return "EFim sem Enquanto ou EPara";
        pbuf--;
        break;
    case cCasoVar:
        if (pbuf < sizeof(buf))
            buf[pbuf++] = 2;
        break;
    case cCasoSe:
    case cCasoSePadrao:
        if (pbuf ? buf[pbuf - 1] != 2 : true)
            return "CasoSe sem CasoVar";
        break;
    case cCasoFim:
        if (pbuf ? buf[pbuf - 1] != 2 : true)
            return "CasoFim sem CasoVar";
        pbuf--;
        break;
    case cSair1:
    case cSair2:
        if (pbuf)
        {
            for (int x = pbuf - 1; x >= 0; x--)
                if (buf[x] != 0)
                    return nullptr;
        }
        return "Sair sem Enquanto, EPara ou CasoVar";
    case cContinuar1:
    case cContinuar2:
        if (pbuf)
        {
            for (int x = pbuf - 1; x >= 0; x--)
                if (buf[x] == 1)
                    return nullptr;
        }
        return "Continuar sem Enquanto ou EPara";
    }
    return nullptr;
}

//------------------------------------------------------------------------------
const char * Instr::ChecaLinha::Fim()
{
    if (ChecaErro <= 0)
        return nullptr;
    if (ChecaErro == 1)
        for (; pbuf > 0; pbuf--)
            if (buf[pbuf - 1] != 0)
                break;
    if (pbuf == 0)
        return nullptr;
    int result = buf[pbuf - 1];
    pbuf = 0;
    switch (result)
    {
    case 0: return "Falta FimSe antes dessa linha";
    case 1: return "Falta EFim antes dessa linha";
    default: return "Falta CasoFim antes dessa linha";
    }
}
