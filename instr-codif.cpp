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
#include "classe.h"
#include "mudarclasse.h"
#include "misc.h"

/** @defgroup codif Instr::Codif - Algoritmo para codificar instru��es

@par Princ�pio

1. Come�a com uma das palavras reservadas seguido de espa�o:
   - � defini��o de vari�vel

2. Come�a com const:
   - � constante

3. Come�a com herda:
   - Instru��o especial herda

4. Verifica instru��es de controle de fluxo

5. Nenhum dos anteriores:
   - Processa como express�o num�rica pura

@par Princ�pio - express�es

Uma express�o do tipo "1+2-3" � codificada da seguinte forma:\n
1 2 + 3 -

E guardada na mem�ria da seguinte forma:
- Instr::ex_num1
- Instr::ex_num8p, 0x02
- Instr::exo_add
- Instr::ex_num8p, 0x03
- Instr::exo_sub
- Instr::ex_fim

Nesse caso, ao executar s�o realizadas as opera��es:
- Coloca 1 na pilha
- Coloca 2 na pilha
- Soma os dois valores do topo da pilha (vai ficar 3 na pilha)
- Coloca 3 na pilha
- Subtrai os dois valores do topo da pilha (vai ficar 0 na pilha)

@par Algoritmo - codifica de express�o

@verbatim
Vari�vel modo
    Um valor qualquer de Instr::Expressao

Vari�vel arg
    falso = espera operador unit�rio ou valor qualquer
    verdadeiro = espera operador bin�rio

Pilha de vari�veis
    PUSH alguma coisa -> coloca no topo da pilha
    POP alguma coisa -> tira do topo da pilha

arg=falso
modo=0

(1)
// Verificando nome de vari�vel
Se modo = ex_var1 ou modo = ex_var2 ou modo = ex_var3
  Se *origem for uma letra de A a Z ou um n�mero de 0 a 9:
    Anota *origem, avan�a origem
    Se modo = ex_var3:
      modo = ex_var2
    Vai para (1)
  Se *origem for ':':
    Se modo != ex_var1:
      Erro na express�o
    Caso contr�rio:
      Anota ex_doispontos
      modo = ex_var2
      Vai para (1)
  Se *origem for '.':
    se modo != ex_var3
      Anota ex_ponto
      modo = ex_var3
    Vai para (1)
  Se *origem for '[':
    Anota ex_abre
    arg=falso
    se modo == ex_var3
      modo = ex_var2
    PUSH modo
    modo = ex_colchetes
    Vai para (1)
  Se *origem for '(':
    arg=falso
    se modo == ex_var3
      modo = ex_var2
    PUSH modo
    modo = ex_parenteses
    Vai para (1)
  se modo != ex_var3
    Anota ex_ponto
  POP modo
  Anota modo
  arg=verdadeiro
  Vai para (1)

// Ignora espa�o entre operadores e operandos
Se *origem for espa�o:
  Avan�a at� encontrar algo diferente de espa�o

// Operando � texto
Se *origem for aspas duplas:
  Se arg=verdadeiro:
    Erro na express�o
  arg=verdadeiro
  Copia texto, avan�a at� encontrar aspas duplas novamente
  Vai para (1)

// Operando num�rico
Se *origem for um d�gito de 0 a 9:
  Se arg=verdadeiro:
    Erro na express�o
  arg=verdadeiro
  Anota como constante
  Vai para (1)

Se *origem for um h�fen seguido de um d�gito de 0 a 9:
  Se arg=falso:
    arg=verdadeiro
    Anota como constante
    Vai para (1)

// In�cio de nome de vari�vel
Se *origem for '$', '[' ou uma letra de A a Z:
  Se arg=verdadeiro:
    Erro na express�o
  arg=verdadeiro
  Se for "nulo" seguido de alguma coisa diferente de A-Z, 0-9 e [:
    Anota ex_nulo
    Avan�a origem para depois da palavra "nulo"
    Vai para (1)
  PUSH modo
  modo = ex_var1
  Se *origem for '[':
    Anota ex_varabre
    Avan�a origem
    PUSH modo
    modo = ex_colchetes
    arg = false
    Vai para (1)
  Caso contr�rio:
    *** Algoritmo de otimiza��o (vide abaixo)
    Anota ex_varini
    Anota *origem, avan�a origem
    Vai para (1)

// Par�nteses
Se *origem for abre par�nteses:
  Se arg=verdadeiro
    Erro na express�o
  PUSH modo
  modo = ex_parenteses
  Vai para (1)

Se *origem for fecha par�nteses:
  Se arg=falso
    Erro na express�o
  Enquanto modo for operador:
    Anota modo
    POP modo
  Se modo n�o for ex_parenteses:
    Erro na express�o
  POP modo
  // Par�nteses com o significado de par�metros de uma fun��o
  Se modo for ex_var1 ou ex_var2 ou ex_var3:
    Anota ex_ponto
    Se *origem for '.':
      Avan�a origem
    Caso contr�rio:
      Anota ex_varfim
      POP modo
  Vai para (1)

// Fecha colchetes - volta a processar nome de vari�vel
Se *origem for fecha colchetes:
  Se arg=falso
    Erro na express�o
  Enquanto modo for operador:
    Anota modo
    POP modo
  Se modo n�o for ex_colchetes:
    Erro na express�o
  POP modo
  Anota ex_fecha
  Vai para (1)

// Verifica operadores e preced�ncia de operadores
Se for operador:
  Se for operador '-' e arg=falso:
    � operador '-' unit�rio
  Se o tipo de operador (bin�rio ou unit�rio) n�o combinar com arg:
    Erro na express�o
  Enquanto modo for operador e operador encontrado tiver menos prioridade
              que a vari�vel modo:
    Anota modo
    POP modo
  PUSH modo
  modo=operador encontrado
  arg=falso
  Vai para (1)

// Fim da senten�a
Se for \0:
  Enquanto modo for operador:
    Anota modo
    POP modo
  Se (modo=0 e arg=verdadeiro):
    Acabou a string
  Caso contr�rio:
    Erro na express�o
  Fim

Mensagem de erro:
  Caracter inv�lido na express�o
Fim

O algoritmo de otimiza��o funciona assim:
  Faz: const char * p = origem
  Avan�a p enquanto *p for um caracter v�lido para nomes de vari�veis
  Se encontrou algo diferente de ':' e de '[', pode otimizar:
    Se o texto for nome de uma fun��o predefinida:
      Anota ex_varfunc
      Anota o n�mero da fun��o
      Faz origem = p
      Vai para (1)
    Caso contr�rio:
      Anota ex_varlocal
      Anota 255
      Vai para (1)

@endverbatim
*/

using namespace Instr;

//------------------------------------------------------------------------------
class TListaInstr {
public:
    const char * Nome;
    const char Instr;
};

// Lista de instru��es, exceto cExpr
// Deve obrigatoriamente estar em ordem alfab�tica
static const TListaInstr ListaInstr[] = {
    { "arqdir",    cArqDir },
    { "arqexec",   cArqExec },
    { "arqlog",    cArqLog },
    { "arqmem",    cArqMem },
    { "arqprog",   cArqProg },
    { "arqsav",    cArqSav },
    { "arqtxt",    cArqTxt },
    { "casofim",   cCasoFim },
    { "casose",    cCasoSe },
    { "casovar",   cCasoVar },
    { "const",     cConstExpr }, // Qualquer tipo de const
    { "continuar", cContinuar1 }, // Pode ser cContinuar1 ou cContinuar2
    { "datahora",  cDataHora },
    { "debug",     cDebug },
    { "efim",      cEFim },
    { "enquanto",  cEnquanto },
    { "epara",     cEPara },
    { "fimse",     cFimSe },
    { "func",      cFunc },
    { "herda",     cHerda },
    { "indiceitem", cIndiceItem },
    { "indiceobj", cIndiceObj },
    { "int1",      cInt1 },
    { "int16",     cInt16 },
    { "int32",     cInt32 },
    { "int8",      cInt8 },
    { "intdec",    cIntDec },
    { "intexec",   cIntExec },
    { "intinc",    cIntInc },
    { "inttempo",  cIntTempo },
    { "listaitem", cListaItem },
    { "listaobj",  cListaObj },
    { "nomeobj",   cNomeObj },
    { "prog",      cProg },
    { "real",      cReal },
    { "real2",     cReal2 },
    { "ref",       cRef },
    { "refvar",    cRefVar },
    { "ret",       cRet1 }, // Pode ser cRet1 ou cRet2
    { "sair",      cSair1 }, // Pode ser cSair1 ou cSair2
    { "se",        cSe },
    { "senao",     cSenao1 }, // Pode ser cSenao1 ou cSenao2
    { "serv",      cServ },
    { "socket",    cSocket },
    { "telatxt",   cTelaTxt },
    { "terminar",  cTerminar },
    { "textoobj",  cTextoObj },
    { "textopos",  cTextoPos },
    { "textotxt",  cTextoTxt },
    { "textovar",  cTextoVar },
    { "uint16",    cUInt16 },
    { "uint32",    cUInt32 },
    { "uint8",     cUInt8 },
    { "varconst",  cConstVar },
    { "varfunc",   cVarFunc }
};

//------------------------------------------------------------------------------
static int comparaNome(const char * string1, const char * string2)
{
    for (;; string1++, string2++)
    {
        unsigned char ch1,ch2;
        ch1 = (*string1 == ' ' ? 0 : tabNOMES1[*(unsigned char *)string1]);
        ch2 = (*string2 == ' ' ? 0 : tabNOMES1[*(unsigned char *)string2]);
        if (ch1 == 0 || ch2 == 0)
            return (ch1 ? 1 : ch2 ? -1 : 0);
        if (ch1 != ch2)
            return (ch1<ch2 ? -1 : 1);
    }
}

//------------------------------------------------------------------------------
static char * anotaModo(char * destino, int modo)
{
    *destino++ = modo;
    switch (modo)
    {
    case exo_i_mul:
        *destino++ = exo_mul;
        *destino++ = exo_atrib;
        break;
    case exo_i_div:
        *destino++ = exo_div;
        *destino++ = exo_atrib;
        break;
    case exo_i_porcent:
        *destino++ = exo_porcent;
        *destino++ = exo_atrib;
        break;
    case exo_i_add:
        *destino++ = exo_add;
        *destino++ = exo_atrib;
        break;
    case exo_i_sub:
        *destino++ = exo_sub;
        *destino++ = exo_atrib;
        break;
    case exo_i_b_shl:
        *destino++ = exo_b_shl;
        *destino++ = exo_atrib;
        break;
    case exo_i_b_shr:
        *destino++ = exo_b_shr;
        *destino++ = exo_atrib;
        break;
    case exo_i_b_e:
        *destino++ = exo_b_e;
        *destino++ = exo_atrib;
        break;
    case exo_i_b_ouou:
        *destino++ = exo_b_ouou;
        *destino++ = exo_atrib;
        break;
    case exo_i_b_ou:
        *destino++ = exo_b_ou;
        *destino++ = exo_atrib;
        break;
    }
    return destino;
}

//------------------------------------------------------------------------------
/// Codifica uma instru��o
/**
 *  @param destino Endere�o destino (instru��o codificada)
 *  @param origem  Endere�o origem (string ASCIIZ)
 *  @param tamanho Tamanho do buffer em destino
 *  @retval true Conseguiu codificar com sucesso
 *  @retval false Erro, destino cont�m a mensagem de erro
 *  @sa codif
 */
bool Instr::Codif(char * destino, const char * origem, int tamanho)
{
    char * dest_ini = destino;
    char * dest_fim = destino + tamanho;
    bool proc_expr = false; // true = vai processar express�o num�rica
                            // false = checar se tem coment�rio ap�s instru��o

// Verifica tamanho m�nimo
    if (tamanho < 10)
    {
        *dest_ini = 0;
        return false;
    }

// Verifica se � coment�rio
    while (*origem==' ')
        origem++;
    if (*origem=='#')
    {
        destino[2] = cComent;
        destino[3] = 0;
        for (origem++; *origem == ' '; origem++);
        destino += endVar;
        while (true)
        {
            if (destino >= dest_fim - 1)
            {
                copiastr(dest_ini, "Instru��o muito grande", tamanho);
                return false;
            }
            if (*origem == 0)
                break;
            switch (*origem)
            {
            case ex_barra_n: *destino++ = ' ';
            case ex_barra_b: origem++; break;
            case ex_barra_c:
            case ex_barra_d:
                origem++;
                if (origem[0])
                    origem++;
                break;
            default:
                *destino++ = *origem++;
            }
        }
        while (destino[-1] == ' ')
            destino--;
        *destino++ = 0;
        dest_ini[0] = (destino - dest_ini);
        dest_ini[1] = (destino - dest_ini) >> 8;
        return true;
    }

// Obt�m a vari�vel/instru��o em destino[2] a destino[6]
    destino[2] = cExpr;
    destino[3] = 0;
    destino[4] = 0;
    destino[5] = 0;
    destino[6] = 0;
    destino[7] = 0;
    bool testar_def = true; // Se deve testar se � defini��o de vari�vel
    {
        const char * p = origem;
        while (*p == ' ') p++;
        while (*p != ' ' && tabNOMES1[*(unsigned char*)p]) p++;
        while (*p == ' ') p++;
        if (tabNOMES1[*(unsigned char*)p] == 0)
            testar_def=false;
        //printf("%d   %s\n", testar_def, origem); fflush(stdout);
    }
    while (true)
    {
    // Avan�a enquanto for espa�o
        while (*origem==' ')
            origem++;
    // Verifica atributos de vari�veis: comum e sav
        if (testar_def && comparaNome(origem, "comum") == 0)
        {
            destino[endProp] |= 1;
            origem += 5;
            continue;
        }
        if (testar_def && comparaNome(origem, "sav") == 0)
        {
            destino[endProp] |= 2;
            origem += 3;
            continue;
        }
    // Verifica se instru��o n�o � nula
        if (*origem==0)
        {
            copiastr(dest_ini, "Faltou a instru��o", tamanho);
            return false;
        }
    // Verifica instru��o txt1 a txt512
        if (testar_def && (origem[0] | 0x20) == 't' &&
                (origem[1] | 0x20) == 'x' &&
                (origem[2] | 0x20) == 't' && origem[3] != '0')
        {
            int valor = 0;
            const char * p = origem + 3;
            for (; *p >= '0' && *p <= '9'; p++)
            {
                valor = valor*10 + *p-'0';
                if (valor > 0x1000)
                    break;
            }
            if (valor >= 1 && comparaNome(p, "") == 0)
            {
                if (valor <= 256) // 1 a 256
                {
                    destino[2] = cTxt1;
                    destino[endExtra] = valor - 1;
                    origem = p;
                    break;
                }
                if (valor <= 512) // 257 a 512
                {
                    destino[2] = cTxt2;
                    destino[endExtra] = valor - 257;
                    origem = p;
                    break;
                }
            }
        }
    // Obt�m a instru��o
        int ini = 0;
        int fim = sizeof(ListaInstr) / sizeof(ListaInstr[0]) - 1;
        while (true)
        {
            if (ini > fim) // N�o encontrou
            {
                if (destino[endProp]==0)
                    break;
                // Tem atribui��o const ou sav: deveria ser tipo de vari�vel
                char mens[30];
                char * p = mens;
                copiastr(mens, origem, sizeof(mens));
                while (*p && *p != ' ')
                    p++;
                *p = 0;
                mprintf(dest_ini, tamanho, "Vari�vel n�o existe: %s", mens);
                return false;
            }
            int meio = (ini + fim ) / 2;
            int resultado = comparaNome(origem, ListaInstr[meio].Nome);
        // Verifica se encontrou
            if (resultado == 0)  // Se encontrou
            {
                int instr = ListaInstr[meio].Instr;
                if (!testar_def && (instr >= cVariaveis || instr == cHerda ||
                                    instr == cRefVar))
                    break;
                destino[2] = instr;
                origem += strlen(ListaInstr[meio].Nome);
                //while (*origem && *origem!=' ')
                //    origem++;
                if (destino[endProp] && (destino[2] < cVariaveis ||
                            destino[2] == cRefVar ||
                            destino[2] == cConstExpr ||
                            destino[2] == cConstVar ||
                            destino[2] == cFunc ||
                            destino[2] == cVarFunc))
                {
                    copiastr(dest_ini, "Atribui��es comum e sav s� podem "
                                    "ser usadas em vari�veis", tamanho);
                    return false;
                }
                break;
            }
            if (resultado < 0)
                fim = meio - 1;
            else
                ini = meio + 1;
        }
        break;
    }
    while (*origem == ' ')
        origem++;

// Instru��o Herda
    if (destino[2] == cHerda)
    {
        destino[endVar] = 0;
        destino += endVar + 1;
    // Obt�m as classes
        const char * classes[HERDA_TAM]; // nomes das classes
        unsigned int total = 0;
        while (*origem)
        {
            char nome[80];
            char * p = nome;
        // Avan�a enquanto for espa�o
            while (*origem == ' ')
                origem++;
            if (*origem == 0)
                break;
        // Copia o nome
            for (; *origem && *origem != ','; origem++)
            {
                if (p < nome + sizeof(nome) - 2)
                {
                    *p++ = *origem;
                    continue;
                }
                else if (*p == ' ')
                    continue;
                p[-1] = 0;
                mprintf(dest_ini, tamanho, "Classe n�o existe: %s", nome);
                return false;
            }
        // Apaga espa�os no fim do nome
            while (p > nome)
            {
                if (p[-1] != ' ')
                    break;
                p--;
            }
            *p++ = 0;
        // Verifica se existe classe com o nome encontrado
            const char * obj = nullptr;
            TMudarClasse * cl1 = TMudarClasse::Procurar(nome);
            TClasse * cl2 = TClasse::Procura(nome);
            if (cl1 == nullptr)
            {
                if (cl2)
                    obj = cl2->Nome;
            }
            else if (!cl1->InfoApagar())
                obj = (cl2 ? cl2->Nome : cl1->Nome);
            if (obj == nullptr)
            {
                if (*nome)
                    mprintf(dest_ini, tamanho, "Classe n�o existe: %s", nome);
                else
                    copiastr(dest_ini, "Faltou o nome da classe", tamanho);
                return false;
            }
        // Verifica n�mero de classes
            if (total >= sizeof(classes) / sizeof(classes[0]))
            {
                mprintf(dest_ini, tamanho,
                        "Deve definir no m�ximo %d classes",
                        sizeof(classes) / sizeof(classes[0]));
                return false;
            }
        // Verifica se classe repetida
            classes[total] = obj;
            for (unsigned int x = 0; x < total; x++)
                if (classes[x] == obj)
                {
                    mprintf(dest_ini, tamanho, "Classe repetida: %s", nome);
                    return false;
                }
            total++;
        // Copia o nome da classe
            if (destino + strlen(obj) + 1 > dest_fim)
            {
                copiastr(dest_ini, "Instru��o muito grande", tamanho);
                return false;
            }
            destino = copiastr(destino, obj);
            destino++;
            dest_ini[endVar]++; // Aumenta o n�mero de classes
        // Pula a v�rgula
            if (*origem==',')
                origem++;
        }
    // Checa n�mero de classes encontradas
        if (total == 0)
        {
            copiastr(dest_ini, "Deve definir pelo menos uma classe", tamanho);
            return false;
        }
        dest_ini[0] = (destino - dest_ini);
        dest_ini[1] = (destino - dest_ini) >> 8;
        return true;
    }

// Processa vari�veis
// Vari�veis const: avan�a origem e destino
// Outras vari�veis: acerta dados em destino e retorna
    if (destino[2] > cVariaveis || destino[2] == cRefVar)
    {
        char nome[VAR_NOME_TAM];
        char * p = nome;
        int  indice = -1;
    // Avan�a enquanto for espa�o
        while (*origem==' ')
            origem++;
    // Copia o nome
        for (; *origem && *origem != '=' && *origem != '#'; origem++)
        {
            if (*origem == ' ')
            {
                while (*origem == ' ')
                    origem++;
                if (*origem && *origem != '=' && *origem != '#')
                {
                    copiastr(dest_ini, "Nome de vari�vel inv�lido", tamanho);
                    return false;
                }
                break;
            }
            if (indice >= 0)
            {
                if (*origem < '0' || *origem > '9')
                {
                    if (dest_ini[2] == cConstExpr || dest_ini[2] == cConstVar)
                        copiastr(dest_ini, "Constantes n�o podem ser vetores",
                                 tamanho);
                    else if (dest_ini[2] == cRefVar)
                        copiastr(dest_ini, "Refvar n�o pode ser vetor",
                                 tamanho);
                    else
                        copiastr(dest_ini, "Tamanho inv�lido do vetor",
                                 tamanho);
                    return false;
                }
                indice = indice * 10 + *origem - '0';
                if (indice < 0x100)
                    continue;
                copiastr(dest_ini, "M�ximo 255 vari�veis", tamanho);
                return false;
            }
            if (p < nome + sizeof(nome) - 1)
            {
                if (*origem == '.')
                    indice = 0;
                else
                    *p++ = *origem;
                continue;
            }
            else if (*p == ' ')
                continue;
            copiastr(dest_ini, "Nome de vari�vel muito grande", tamanho);
            return false;
        }
        if (indice >= 0)
        {
            dest_ini[endVetor] = indice;
            if (dest_ini[2] == cFunc || dest_ini[2] == cVarFunc)
            {
                copiastr(dest_ini, "Fun��es n�o podem ser vetores",
                                 tamanho);
                return false;
            }
        }
    // Apaga espa�os no fim do nome
        while (p > nome)
        {
            if (p[-1] != ' ')
                break;
            p--;
        }
        *p++ = 0;
    // Verifica se nome v�lido
        if (*nome == 0)
        {
            copiastr(dest_ini, "Faltou o nome da vari�vel", tamanho);
            return false;
        }
        for (const char * p = nome; *p; p++)
            if (tabNOMES1[*(unsigned char*)p] == 0)
            {
                copiastr(dest_ini, "Nome de vari�vel inv�lido", tamanho);
                return false;
            }
    // Copia o nome da vari�vel, avan�a destino
        if (destino+strlen(nome) + endNome + 1 > dest_fim)
        {
            copiastr(dest_ini, "Instru��o muito grande", tamanho);
            return false;
        }
        destino = copiastr(destino+endNome, nome);
        destino++;
    // Vari�vel Const
        if (dest_ini[2] == cConstExpr || dest_ini[2] == cConstVar ||
            dest_ini[2] == cRefVar)
        {
            if (*origem == 0 || *origem == '#')
            {
                if (dest_ini[2] == cRefVar)
                    copiastr(dest_ini, "Faltou atribuir um valor "
                                       "� vari�vel refvar", tamanho);
                else
                    copiastr(dest_ini, "Faltou atribuir um valor "
                                       "� vari�vel const", tamanho);
                return false;
            }
            origem++;
            dest_ini[endIndice] = destino - dest_ini;
            proc_expr = true;
        }
    // Outros tipos de vari�veis
        else if (*origem && *origem != '#')
        {
            origem++;
            dest_ini[endIndice] = destino - dest_ini;
            proc_expr = true;
        }
    }

// Verifica instru��es de controle de fluxo
// Acerta destino e proc_expr
    switch (dest_ini[2])
    {
    case cSe:       // Requerem express�o num�rica
    case cEnquanto:
    case cCasoVar:
        destino += endVar + 2;
        proc_expr = true;
        break;
    case cEPara:    // Requer express�o num�rica
        memset(destino+endVar, 0, 6);
        destino += endVar + 6;
        proc_expr = true;
        break;
    case cSenao1:   // Pode ter ou n�o express�o num�rica
        if (*origem == 0 || *origem == '#')
        {
            destino += endVar + 2;
            break;
        }
        destino[2] = cSenao2;
        destino += endVar+2;
        proc_expr=true;
        break;
    case cRet1:     // Pode ter ou n�o express�o num�rica
        if (*origem == 0 || *origem == '#')
        {
            destino += endVar;
            break;
        }
        destino[2] = cRet2;
        destino += endVar;
        proc_expr = true;
        break;
    case cSair1:    // Pode ter ou n�o express�o num�rica
        if (*origem == 0 || *origem == '#')
        {
            destino += endVar + 2;
            break;
        }
        destino[2] = cSair2;
        destino += endVar + 2;
        proc_expr = true;
        break;
    case cContinuar1:  // Pode ter ou n�o express�o num�rica
        if (*origem == 0 || *origem == '#')
        {
            destino += endVar + 2;
            break;
        }
        destino[2] = cContinuar2;
        destino += endVar + 2;
        proc_expr = true;
        break;
    case cEFim:     // Dois bytes de dados
        destino += endVar + 2;
        break;
    case cFimSe:    // Nenhum byte de dados
    case cCasoFim:
    case cTerminar:
        destino += endVar;
        break;
    case cExpr:     // Express�o num�rica pura
        destino += endVar;
        proc_expr = true;
        break;
    case cRefVar: // RefVar - j� foi verificado acima
    case cConstExpr: // Vari�vel Const - j� foi verificado acima
    case cConstVar: // Vari�vel VarConst - j� foi verificado acima
        break;
    case cCasoSe:
        if (*origem == 0 || *origem == '#')
        {
            dest_ini[2] = cCasoSePadrao;
            destino += endVar;
            break;
        }
        if (*origem != '\"')
        {
            copiastr(dest_ini, "Op��o inv�lida ap�s CASOSE", tamanho);
            return false;
        }
        memset(destino+endVar, 0, 4);
        destino += endVar + 4;
        for (origem++; *origem != '\"'; origem++)
        {
            if (*origem == 0)
            {
                copiastr(dest_ini, "Faltou fechar aspas", tamanho);
                return false;
            }
            if (destino >= dest_fim - 2)
            {
                copiastr(dest_ini, "Instru��o muito grande", tamanho);
                return false;
            }
            if (*origem == '\\')
            {
                origem++;
                switch (*origem)
                {
                case 0:
                    copiastr(dest_ini, "Faltou fechar aspas", tamanho);
                    return false;
                case 'N':
                case 'n': *destino++ = ex_barra_n; break;
                case 'B':
                case 'b': *destino++ = ex_barra_b; break;
                case 'C':
                case 'c': *destino++ = ex_barra_c; break;
                case 'D':
                case 'd': *destino++ = ex_barra_d; break;
                default:  *destino++ = *origem;
                }
                continue;
            }
            *destino++ = *origem;
        }
        *destino++ = 0;
        origem++;
        while (*origem == ' ')
            origem++;
        break;
    default: // Nenhum dos anteriores
        if (dest_ini[2] > cVariaveis)
            break;
        copiastr(dest_ini, "Instru��o desconhecida", tamanho);
        return false;
    }

// Anota coment�rio, se houver
    if (!proc_expr)
    {
        if (*origem == '#')
        {
            const char * final = destino;
            origem++;
            while (*origem == ' ')
                origem++;
        // Verifica se tem espa�o
            if (destino + strlen(origem) + 2 >= dest_fim)
            {
                copiastr(dest_ini, "Instru��o muito grande", tamanho);
                return false;
            }
            while (*origem)
                switch (*origem)
                {
                case ex_barra_n: *destino++ = ' ';
                case ex_barra_b: origem++; break;
                case ex_barra_c:
                case ex_barra_d:
                    origem++;
                    if (origem[0])
                        origem++;
                    break;
                default:
                    *destino++ = *origem++;
                }
            while (destino > final && destino[-1] == ' ')
                destino--;
        }
        dest_ini[0] = (destino - dest_ini);
        dest_ini[1] = (destino - dest_ini) >> 8;
        return true;
    }

// Processa express�o num�rica
    char pilha[512];
    char * topo = pilha;
    int  modo = 0;
    bool arg = false;
    *topo++ = 0;
    while (true)
    {
    // Verifica se tem espa�o suficiente
        if (destino >= dest_fim - 2)
        {
            copiastr(dest_ini, "Instru��o muito grande", tamanho);
            return false;
        }

    // Processando nome de vari�vel
        if (modo == ex_var1 || modo == ex_var2 || modo == ex_var3)
        {
            if ((tabNOMES1[*(unsigned char*)origem] && *origem != ' ') ||
                *origem == '$')
            {
                *destino++ = *origem++;
                if (modo == ex_var3)
                    modo = ex_var2;
            }
            else if (*origem == ':')
            {
                if (modo != ex_var1)
                {
                    mprintf(dest_ini, tamanho, "Dois pontos duas vezes");
                    return false;
                }
                origem++;
                *destino++ = ex_doispontos;
                modo = ex_var2;
            }
            else if (*origem == '.')
            {
                origem++;
                if (modo != ex_var3)
                    *destino++ = ex_ponto, modo = ex_var3;
            }
            else if (*origem == '[')
            {
                origem++;
                *destino++ = ex_abre;
                arg=false;
                *topo++ = (modo == ex_var3 ? ex_var2 : modo);
                modo = ex_colchetes;
            }
            else if (*origem == '(')
            {
                origem++;
                *destino++ = ex_arg;
                arg=false;
                *topo++ = (modo == ex_var3 ? ex_var2 : modo);
                modo = ex_parenteses;
            }
            else
            {
                if (modo != ex_var3)
                    *destino++ = ex_ponto;
                *destino++ = ex_varfim;
                modo = *--topo;
                arg=true;
            }
            continue;
        }

    // Ignora espa�o entre operadores e operandos
        while (*origem == ' ')
            origem++;

    // Texto
        if (*origem == '\"')
        {
            if (arg)
            {
                mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
                return false;
            }
            arg=true;
            *destino++ = ex_txt;
            for (origem++; destino<dest_fim - 2; origem++)
            {
                if (*origem == 0)
                {
                    copiastr(dest_ini, "Faltou fechar aspas", tamanho);
                    return false;
                }
                if (*origem == '\"')
                {
                    origem++;
                    while (*origem == ' ')
                        origem++;
                    if (*origem == '\"')
                        continue;
                    if (*origem != '+')
                        break;
                    const char * p = origem+1;
                    while (*p == ' ')
                        p++;
                    if (*p != '\"')
                        break;
                    origem=p;
                    continue;
                }
                if (*origem == '\\')
                {
                    origem++;
                    switch (*origem)
                    {
                    case 0:
                        copiastr(dest_ini, "Faltou fechar aspas", tamanho);
                        return false;
                    case 'N':
                    case 'n': *destino++ = ex_barra_n; break;
                    case 'B':
                    case 'b': *destino++ = ex_barra_b; break;
                    case 'C':
                    case 'c': *destino++ = ex_barra_c; break;
                    case 'D':
                    case 'd': *destino++ = ex_barra_d; break;
                    default:  *destino++ = *origem;
                    }
                    continue;
                }
                *destino++ = *origem;
            }
            if (destino < dest_fim - 1)
                *destino++ = 0;
            continue;
        }

    // N�mero
        if ((*origem >= '0' && *origem <= '9') ||
             (!arg && *origem == '-' && origem[1] >= '0' && origem[1] <= '9'))
        {
            if (arg)
            {
                mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
                return false;
            }
            arg=true;
            bool negativo=false; // Se � negativo
            int  virgula = 0;    // Casas ap�s a v�rgula
            long long valor = 0; // Valor lido
        // Obt�m o sinal
            if (*origem == '-')
                negativo = true, origem++;
        // Checa n�mero em hexadecimal
            if (origem[0] == '0' && origem[1] == 'x')
            {
                origem += 2;
            // Primeiro d�gito
                if (*origem >= '0' && *origem <= '9')
                    valor = *origem - '0';
                else if (*origem >= 'A' && *origem <= 'F')
                    valor = *origem - 'A' + 10;
                else if (*origem >= 'a' && *origem <= 'f')
                    valor = *origem - 'a' + 10;
                else
                {
                    mprintf(dest_ini, tamanho,
                            "N�mero inv�lido a partir de: %s", origem-2);
                    return false;
                }
            // Outros d�gitos
                while (true)
                {
                    origem++;
                    if (*origem >= '0' && *origem <= '9')
                        valor = valor * 16 + *origem - '0';
                    else if (*origem >= 'A' && *origem <= 'F')
                        valor = valor * 16 + *origem - 'A' + 10;
                    else if (*origem >= 'a' && *origem <= 'f')
                        valor = valor * 16 + *origem - 'a' + 10;
                    else
                        break;
                    if (valor > 0xFFFFFFFFLL)
                    {
                        copiastr(dest_ini, "N�mero muito grande", tamanho);
                        return false;
                    }
                }
            // Anota o n�mero
                if (valor < 0x100) // N�mero de 8 bits
                {
                    if (destino >= dest_fim - 2)
                    {
                        destino = dest_fim;
                        continue;
                    }
                    *destino++ = (negativo ? ex_num8hexn : ex_num8hexp);
                    *destino++ = valor;
                }
                else if (valor < 0x10000) // N�mero de 16 bits
                {
                    if (destino >= dest_fim - 3)
                    {
                        destino = dest_fim;
                        continue;
                    }
                    *destino++ = (negativo ? ex_num16hexn : ex_num16hexp);
                    *destino++ = valor;
                    *destino++ = valor >> 8;
                }
                else // N�mero de 32 bits
                {
                    if (destino >= dest_fim - 5)
                    {
                        destino = dest_fim;
                        continue;
                    }
                    *destino++ = (negativo ? ex_num32hexn : ex_num32hexp);
                    *destino++ = valor;
                    *destino++ = valor >> 8;
                    *destino++ = valor >> 16;
                    *destino++ = valor >> 24;
                }
                continue;
            }
        // Obt�m o n�mero (negativo, virgula, valor)
            for (;; origem++)
            {
                if (*origem == '.')
                {
                    if (virgula)
                    {
                        copiastr(dest_ini,
                            "N�meros n�o podem ter mais de um ponto", tamanho);
                        return false;
                    }
                    virgula = 1;
                    continue;
                }
                if (*origem < '0' || *origem > '9')
                    break;
                if (virgula)
                    virgula++;
                valor = (valor * 10 + *origem - '0');
                if (valor > 0xFFFFFFFFLL)
                {
                    copiastr(dest_ini, "N�mero muito grande", tamanho);
                    return false;
                }
            }
            if (virgula)
                virgula--;
        // Anota o n�mero
            if (valor==0) // zero � zero
            {
                *destino++ = ex_num0;
                continue; // Porque n�o tem casas decimais
            }
            if (valor == 1 && negativo == false) // Um
                *destino++ = ex_num1;
            else if (valor < 0x100) // N�mero de 8 bits
            {
                if (destino >= dest_fim - 2)
                {
                    destino = dest_fim;
                    continue;
                }
                *destino++ = (negativo ? ex_num8n : ex_num8p);
                *destino++ = valor;
            }
            else if (valor < 0x10000) // N�mero de 16 bits
            {
                if (destino >= dest_fim - 3)
                {
                    destino = dest_fim;
                    continue;
                }
                *destino++ = (negativo ? ex_num16n : ex_num16p);
                *destino++ = valor;
                *destino++ = valor >> 8;
            }
            else // N�mero de 32 bits
            {
                if (destino >= dest_fim - 5)
                {
                    destino = dest_fim;
                    continue;
                }
                *destino++ = (negativo ? ex_num32n : ex_num32p);
                *destino++ = valor;
                *destino++ = valor >> 8;
                *destino++ = valor >> 16;
                *destino++ = valor >> 24;
            }
        // Anota as casas decimais
            while (virgula > 0 && destino < dest_fim - 1)
            {
                switch (virgula)
                {
                case 1:  *destino++ = ex_div1;  break;
                case 2:  *destino++ = ex_div2;  break;
                case 3:  *destino++ = ex_div3;  break;
                case 4:  *destino++ = ex_div4;  break;
                case 5:  *destino++ = ex_div5;  break;
                default: *destino++ = ex_div6;
                }
                virgula -= 6;
            }
            continue;
        }

    // In�cio de nome de vari�vel
        if (*origem == '$' || *origem == '[' ||
                    tabNOMES1[*(unsigned char*)origem] != 0)
        {
            if (arg)
            {
                mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
                return false;
            }
            arg=true;
        // Verifica "nulo"
            if ((origem[0] | 0x20) == 'n' && (origem[1] | 0x20) == 'u' &&
                (origem[2] | 0x20) == 'l' && (origem[3] | 0x20) == 'o' &&
                 tabNOMES1[*(unsigned char*)(origem + 4)] == 0 &&
                 origem[4] != '.')
            {
                *destino++ = ex_nulo;
                origem += 4;
                continue;
            }
        // Verifica se come�a com colchetes
            *topo++ = modo;
            modo = ex_var1;
            if (*origem == '[')
            {
                *destino++ = ex_varabre;
                arg=false;
                *topo++ = modo;
                modo = ex_colchetes;
                origem++;
                continue;
            }
#ifdef OTIMIZAR_VAR
        // Verifica se pode otimizar o c�digo
        // Otimiza��o: obt�m o nome
            const char * p = origem;
            while (tabNOMES1[*(unsigned char*)p] != 0 && *p != ' ')
                p++;
        // Otimiza��o: checa se encontrou ':' ou '[' ou se nome nulo
        // Nesse caso n�o pode otimizar
            if (p == origem || *p == ':' || *p == '[' || p-origem > 79)
            {
                *destino++ = ex_varini;
                *destino++ = *origem++;
                continue;
            }
        // Otimiza��o: checa se � uma fun��o predefinida
            char mens[80];
            memcpy(mens, origem, p-origem);
            mens[p-origem] = 0;
            int NumFunc = InfoFunc(mens);
            if (NumFunc >= 0) // Fun��o predefinida
            {
                *destino++ = ex_varfunc;
                *destino++ = NumFunc;
                origem = p;
                continue;
            }
        // Otimiza��o: vari�vel/fun��o local, da classe ou do objeto
            *destino++ = ex_varlocal;
            *destino++ = 0xFF;
            continue;
#else
            *destino++ = ex_varini;
            *destino++ = *origem++;
            continue;
#endif
        }

    // Par�nteses
        if (*origem == '(')
        {
            origem++;
            if (arg)
            {
                mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
                return false;
            }
            *topo++ = modo;
            modo = ex_parenteses;
            continue;
        }
        if (*origem == ')')
        {
            origem++;
            if (!arg)
            {
                mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
                return false;
            }
        // Anota operadores de menor preced�ncia
            while (modo > exo_ini && modo < exo_fim && destino < dest_fim - 3)
            {
                destino = anotaModo(destino, modo);
                modo = *--topo;
            }
            if (destino >= dest_fim - 1)
                continue;
        // Verifica se tinha abre par�nteses
            if (modo != ex_parenteses)
            {
                mprintf(dest_ini, tamanho, ") sem (");
                return false;
            }
            modo = *--topo;
        // Par�nteses como par�metro de fun��o
            if (modo == ex_var1 || modo == ex_var2 || modo == ex_var3)
            {
                *destino++ = ex_ponto;
                if (*origem == '.')
                    origem++;
                else if (destino < dest_fim - 1)
                {
                    *destino++ = ex_varfim;
                    modo = *--topo;
                }
            }
            continue;
        }

    // Fecha colchetes
        if (*origem == ']')
        {
            origem++;
            if (!arg)
            {
                mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
                return false;
            }
        // Anota operadores de menor preced�ncia
            while (modo>exo_ini && modo<exo_fim && destino < dest_fim - 3)
            {
                destino = anotaModo(destino, modo);
                modo = *--topo;
            }
            if (destino >= dest_fim - 1)
                continue;
        // Verifica se tinha abre colchetes
            if (modo != ex_colchetes)
            {
                mprintf(dest_ini, tamanho, "] sem [");
                return false;
            }
            modo = *--topo;
            *destino++ = ex_fecha;
            continue;
        }

    // Fim da senten�a
        if (*origem == 0 || *origem == '#')
        {
            while (modo > exo_ini && modo < exo_fim && destino < dest_fim - 3)
            {
                destino = anotaModo(destino, modo);
                modo = *--topo;
            }
            if (destino >= dest_fim - 1)
                continue;
            if (modo != 0 || !arg)
            {
                copiastr(dest_ini, "Erro na instru��o", tamanho);
                return false;
            }
        // EPARA: verifica se tem 3 argumentos
            if (dest_ini[2] == cEPara && dest_ini[endVar + 4] == 0 &&
                    dest_ini[endVar + 5] == 0)
            {
                copiastr(dest_ini, "EPARA requer 3 argumentos", tamanho);
                return false;
            }
        // Verifica se tem coment�rio
            if (*origem == '#')
            {
                *destino++ = ex_coment;
                origem++;
                while (*origem == ' ')
                    origem++;
                if (destino + strlen(origem) + 2 >= dest_fim)
                {
                    copiastr(dest_ini, "Instru��o muito grande", tamanho);
                    return false;
                }
                while (*origem)
                    switch (*origem)
                    {
                    case ex_barra_n: *destino++ = ' ';
                    case ex_barra_b: origem++; break;
                    case ex_barra_c:
                    case ex_barra_d:
                        origem++;
                        if (origem[0])
                            origem++;
                        break;
                    default:
                        *destino++ = *origem++;
                    }
                while (destino[-1] == ' ')
                    destino--;
            }
        // Marca o fim da instru��o
            *destino++ = 0;
            dest_ini[0] = (destino - dest_ini);
            dest_ini[1] = (destino - dest_ini) >> 8;
        // Acerta vari�veis const
            if (dest_ini[2] != cConstExpr)
                return true;
            const char * p = dest_ini + (unsigned char)dest_ini[endIndice];
            int tipo = 0;
            while (true)
            {
                assert(p<destino);
                switch (*p)
                {
                case ex_fim:
                case ex_coment:
                    if (tipo)
                        dest_ini[2] = tipo;
                    return true;
                case ex_nulo:
                    if (tipo)
                        return true;
                    tipo = cConstNulo;
                    p++;
                    break;
                case ex_txt:
                    if (tipo)
                        return true;
                    tipo = cConstTxt;
                    while (*p++);
                    break;
                case ex_num32p:
                case ex_num32n:
                    p+=2;
                case ex_num16p:
                case ex_num16n:
                    p++;
                case ex_num8p:
                case ex_num8n:
                    p++;
                case ex_num0:
                case ex_num1:
                    p++;
                    if (tipo)
                        return true;
                    tipo = cConstNum;
                    break;
                case ex_div1:
                case ex_div2:
                case ex_div3:
                case ex_div4:
                case ex_div5:
                case ex_div6:
                    if (tipo != cConstNum)
                        return true;
                    p++;
                    break;
                default:
                    return true;
                }
            }
        }

    // Verifica operadores
        unsigned char sinal=0; // Operador
        bool bsinal = true; // Verdadeiro se operador bin�rio
        switch (*origem)
        {
        case ',': sinal=exo_virgula; break;

        case '!': if (origem[1] != '=')
                      sinal = exo_exclamacao, bsinal=false;
                  else if (origem[2] != '=')
                      sinal = exo_diferente, origem++;
                  else
                      sinal = exo_diferente2, origem+=2;
                  break;
        case '~': sinal = exo_b_comp, bsinal = false;
                  break;

        case '*': sinal=exo_mul;
                  if (origem[1] == '=')
                      sinal = exo_i_mul, origem++;
                  break;
        case '/': sinal=exo_div;
                  if (origem[1] == '=')
                      sinal = exo_i_div, origem++;
                  break;
        case '%': sinal=exo_porcent;
                  if (origem[1] == '=')
                      sinal = exo_i_porcent, origem++;
                  break;

        case '+': sinal = exo_add;
                  if (origem[1] == '=')
                      sinal = exo_i_add, origem++;
                  else if (origem[1] == '+')
                  {
                      sinal = (arg ? exo_add_depois : exo_add_antes);
                      bsinal = arg;
                      origem++;
                  }
                  break;
        case '-': if (origem[1] == '=')
                      sinal=exo_i_sub,origem++;
                  else if (origem[1] == '-')
                  {
                      sinal=(arg ? exo_sub_depois : exo_sub_antes);
                      bsinal=arg;
                      origem++;
                  }
                  else if (arg)
                      sinal = exo_sub;
                  else
                      sinal = exo_neg, bsinal=false;
                  break;

        case '<': sinal = exo_menor;
                  if (origem[1] == '=')
                      sinal = exo_menorigual , origem++;
                  else if (origem[1] == '<')
                  {
                      if (origem[2] == '=')
                          sinal = exo_i_b_shl, origem += 2;
                      else
                          sinal = exo_b_shl, origem++;
                  }
                  break;
        case '>': sinal = exo_maior;
                  if (origem[1] == '=')
                       sinal = exo_maiorigual, origem++;
                  else if (origem[1] == '>')
                  {
                      if (origem[2] == '=')
                          sinal = exo_i_b_shr, origem += 2;
                      else
                          sinal = exo_b_shr, origem++;
                  }
                  break;

        case '=': if (origem[1] != '=')
                      sinal = exo_atrib;
                  else if (origem[2] != '=')
                      sinal = exo_igual, origem++;
                  else
                      sinal = exo_igual2, origem += 2;
                  break;

        case '&': if (origem[1] == '&')
                       sinal = exo_e, origem++;
                  else if (origem[1] == '=')
                      sinal = exo_i_b_e, origem++;
                  else
                      sinal = exo_b_e;
                  break;
        case '|': if (origem[1] == '|')
                       sinal = exo_ou, origem++;
                  else if (origem[1] == '=')
                      sinal = exo_i_b_ou, origem++;
                  else
                      sinal = exo_b_ou;
                  break;
        case '^': if (origem[1] == '=')
                      sinal = exo_i_b_ouou, origem++;
                  else
                      sinal = exo_b_ouou;
                  break;
        case '?': sinal = exo_int2;
                  if (origem[1] == '?')
                      sinal = exo_intint2, origem++;
                  break;
        case ':': sinal = exo_dponto2; break;

        default:
            mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
            return false;
        }
        origem++;

    // Verifica tipo de operador
        if (arg != bsinal)
        {
            mprintf(dest_ini, tamanho, "Erro a partir de: %s", origem);
            return false;
        }

    // Anota operadores que t�m mais prioridade sobre o operador encontrado
        int pri_sinal = Instr::Prioridade(sinal);
        if (pri_sinal == 20)
            pri_sinal--;
        while (true)
        {
            if (modo <= exo_ini || modo >= exo_fim || destino >= dest_fim - 3)
                break;
            int pri_modo = Instr::Prioridade(modo);
            // Checar (pri_sinal==2 && pri_modo==2) resolve bug causado com
            // dois ou mais operadores unit�rios consecutivos, como: !!1
            if (pri_sinal < pri_modo || (pri_sinal == 2 && pri_modo == 2))
                break;
            destino = anotaModo(destino, modo);
            modo = *--topo;
        }
        if (destino >= dest_fim - 2)
            continue;

        switch (sinal)
        {
        case exo_e: *destino++ = exo_ee; break;
        case exo_ou: *destino++ = exo_ouou; break;
        case exo_int2: *destino++ = exo_int1; break;
        case exo_intint2: *destino++ = exo_intint1; break;
        case exo_dponto2: *destino++ = exo_dponto1; break;
        case exo_virgula:
            if (topo != pilha + 1)
                break;
            *destino++ = exo_virg_expr;
            if (dest_ini[2] == cEPara)
            {
                if (dest_ini[endVar + 2] == 0 && dest_ini[endVar + 3] == 0)
                {
                    dest_ini[endVar + 2] = (destino - dest_ini);
                    dest_ini[endVar + 3] = (destino - dest_ini) >> 8;
                }
                else if (dest_ini[endVar + 4] == 0 && dest_ini[endVar + 5] == 0)
                {
                    dest_ini[endVar + 4] = (destino - dest_ini);
                    dest_ini[endVar + 5] = (destino - dest_ini) >> 8;
                }
                else
                {
                    copiastr(dest_ini, "EPARA requer apenas 3 argumentos",
                             tamanho);
                    return false;
                }
            }
            break;
        }

        *topo++ = modo;
        modo = sinal;
        arg = (pri_sinal == 3);
    }
}
